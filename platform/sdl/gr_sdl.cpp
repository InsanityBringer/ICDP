/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/
/*
*	Code for SDL integration of the GR library
*
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef USE_SDL

#include "SDL.h"
#include "SDL_video.h"
#include "SDL_surface.h"
#include "SDL_pixels.h"
#include "SDL_mouse.h"
#include "SDL_render.h"

#include "2d/gr.h"
#include "platform/platform.h"
#include "misc/error.h"
#include "misc/types.h"

#include "platform/joy.h"
#include "platform/mouse.h"
#include "platform/key.h"
#include "platform/timer.h"

#include "platform/sdl/gl_sdl.h"

#define FITMODE_BEST 1
#define FITMODE_FILTERED 2

const char* titleMsg = "unnamed Descent port (UDP) (" __DATE__ ")";

int WindowWidth = 1600, WindowHeight = 900;
int CurWindowWidth, CurWindowHeight;
SDL_Window* gameWindow = NULL;
grs_canvas* screenBuffer;

int BestFit = 0;
int Fullscreen = 0;
int SwapInterval = 0;

SDL_Rect screenRectangle, sourceRectangle;

uint32_t localPal[256];

//TODO: temp hack for easy readback, replace with saner code
SDL_Color colors[256];

int refreshDuration = US_70FPS;

int plat_init()
{
	int res;

	res = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
	if (res)
	{
		Error("Error initalizing SDL: %s\n", SDL_GetError());
		return res;
	}
	//Ensure a library capable of modern functions is available. 
	/*res = SDL_GL_LoadLibrary(NULL);
	if (res)
	{
		Error("I_Init(): Cannot load default OpenGL library: %s\n", SDL_GetError());
		return res;
	}*/
	plat_read_chocolate_cfg();
	return 0;
}

int plat_create_window()
{
	//Attributes like this must be set before windows are created, apparently. 
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	CurWindowWidth = WindowWidth;
	CurWindowHeight = WindowHeight;
	int flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL;

	if (Fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_BORDERLESS;
	//SDL is good, create a game window
	gameWindow = SDL_CreateWindow(titleMsg, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WindowWidth, WindowHeight, flags);
	//int result = SDL_CreateWindowAndRenderer(WindowWidth, WindowHeight, flags, &gameWindow, &renderer);

	if (!gameWindow)
	{
		Error("Error creating game window: %s\n", SDL_GetError());
		return 1;
	}
	//where else do i do this...
	I_InitSDLJoysticks();

	if (I_InitGLContext(gameWindow))
	{
		//Failed to initialize OpenGL, try simple surface code instead
		SDL_DestroyWindow(gameWindow);

		Error("Error creating OpenGL context: %s\n", SDL_GetError());
		return 1;
	}

	SDL_ShowWindow(gameWindow);

	if (Fullscreen)
		SDL_GetWindowSize(gameWindow, &CurWindowWidth, &CurWindowHeight);

	return 0;
}

void plat_close_window()
{
	if (gameWindow)
	{
		I_ShutdownGL();

		SDL_DestroyWindow(gameWindow);
		gameWindow = NULL;
	}
}

void plat_toggle_fullscreen()
{
	if (Fullscreen)
	{
		SDL_SetWindowFullscreen(gameWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
		SDL_GetWindowSize(gameWindow, &CurWindowWidth, &CurWindowHeight);
	}
	else
	{
		SDL_SetWindowFullscreen(gameWindow, 0);
		SDL_SetWindowSize(gameWindow, WindowWidth, WindowHeight);
		CurWindowWidth = WindowWidth; CurWindowHeight = WindowHeight;
		SDL_SetWindowPosition(gameWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	}
}

void plat_update_window()
{
	SDL_SetWindowSize(gameWindow, WindowWidth, WindowHeight);
	SDL_SetWindowPosition(gameWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	plat_toggle_fullscreen();
}

void I_SetScreenRect(int w, int h, float aspect)
{
	//Create the destination rectangle for the game screen
	float inv_aspect = 1.f / aspect;
	int bestWidth = CurWindowHeight * inv_aspect;
	if (CurWindowWidth < bestWidth) bestWidth = CurWindowWidth;
	sourceRectangle.x = sourceRectangle.y = 0;
	sourceRectangle.w = w; sourceRectangle.h = h;

	if (BestFit == FITMODE_FILTERED && h <= 400)
	{
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
		w *= 2; h *= 2;
	}
	else
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

	if (BestFit == FITMODE_BEST)
	{
		int numWidths = bestWidth / w;
		screenRectangle.w = numWidths * w;
		screenRectangle.h = (screenRectangle.w * aspect);
		screenRectangle.x = (CurWindowWidth - screenRectangle.w) / 2;
		screenRectangle.y = (CurWindowHeight - screenRectangle.h) / 2;
	}
	else
	{
		screenRectangle.w = bestWidth;
		screenRectangle.h = (screenRectangle.w * aspect);
		screenRectangle.x = screenRectangle.y = 0;
		screenRectangle.x = (CurWindowWidth - screenRectangle.w) / 2;
		screenRectangle.y = (CurWindowHeight - screenRectangle.h) / 2;
	}

	//GL_SetVideoMode(w, h, &screenRectangle);
}

void I_ScaleMouseToWindow(int* x, int* y)
{
	//printf("in: (%d, %d) ", *x, *y);
	*x = (*x * screenRectangle.w / CurWindowWidth);
	*y = (*y * screenRectangle.h / CurWindowHeight);
	if (*x < 0) *x = 0; if (*x >= screenRectangle.w) *x = screenRectangle.w - 1;
	if (*y < 0) *y = 0; if (*y >= screenRectangle.h) *y = screenRectangle.h - 1;
	//printf("out: (%d, %d)\n", *x, *y);
}

void plat_do_events()
{
	SDL_Event ev;
	while (SDL_PollEvent(&ev))
	{
		switch (ev.type)
		{
			//Flush input if you click the window, so that you don't abort your game when clicking back in at the ESC menu. heh...
		case SDL_WINDOWEVENT:
		{
			SDL_WindowEvent winEv = ev.window;
			switch (winEv.event)
			{
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				SDL_FlushEvents(SDL_MOUSEBUTTONDOWN, SDL_MOUSEBUTTONUP);
				break;
			}
		}
		break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			I_MouseHandler(ev.button.button, ev.button.state);
			break;
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			if (ev.key.keysym.scancode == SDL_SCANCODE_RETURN && ev.key.state == SDL_PRESSED && ev.key.keysym.mod & KMOD_ALT)
			{
				Fullscreen ^= 1;
				plat_toggle_fullscreen();
			}
			else
				I_KeyHandler(ev.key.keysym.scancode, ev.key.state);
			break;

		case SDL_JOYDEVICEADDED:
			plat_joystick_attached(ev.jdevice.which);
			break;
		case SDL_JOYDEVICEREMOVED:
			plat_joystick_detached(ev.jdevice.which);
			break;
		}
	}

	I_JoystickHandler();
	I_ControllerHandler();
}

void plat_set_mouse_relative_mode(int state)
{
	SDL_bool formerState = SDL_GetRelativeMouseMode();
	SDL_SetRelativeMouseMode((SDL_bool)state);
	if (state && !formerState)
	{
		int bogusX, bogusY;
		SDL_GetRelativeMouseState(&bogusX, &bogusY);
	}
	else if (!state && formerState)
	{
		SDL_WarpMouseInWindow(gameWindow, CurWindowWidth / 2, CurWindowHeight / 2);
	}
}

void plat_write_palette(int start, int end, uint8_t* data)
{
	int i;

	//TODO: don't waste time storing in the SDL color array
	for (i = 0; i <= end-start; i++)
	{
		colors[i].r = (Uint8)(data[i * 3 + 0] * 255 / 63);
		colors[i].g = (Uint8)(data[i * 3 + 1] * 255 / 63);
		colors[i].b = (Uint8)(data[i * 3 + 2] * 255 / 63);
		localPal[start+i] = (255 << 24) | (colors[i].r << 16) | (colors[i].g << 8) | (colors[i].b);
	}
	GL_SetPalette(localPal);
}

void plat_blank_palette()
{
	uint8_t pal[768];
	memset(&pal[0], 0, 768 * sizeof(uint8_t));
	plat_write_palette(0, 255, &pal[0]);
}

void plat_read_palette(uint8_t* dest)
{
	int i;
	SDL_Color color;
	for (i = 0; i < 256; i++)
	{
		color = colors[i];
		dest[i * 3 + 0] = (uint8_t)(color.r * 63 / 255);
		dest[i * 3 + 1] = (uint8_t)(color.g * 63 / 255);
		dest[i * 3 + 2] = (uint8_t)(color.b * 63 / 255);
	}
}

void plat_wait_for_vbl()
{
	//Now what is a VBL, anyways?
	//SDL_Delay(1000 / 70);
	I_MarkEnd(refreshDuration);
	I_MarkStart();
}

extern uint8_t* gr_video_memory;

void plat_clear_screen()
{
	GL_Clear();
}

void plat_present_canvas(int sync)
{
	if (sync)
	{
		SDL_Delay(1000 / 70);
	}

	GL_Clear();
	I_SetScreenRect(grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h, 3.f / 4.f);
	GL_DrawCanvas(*grd_curcanv, screenRectangle);

	SDL_GL_SwapWindow(gameWindow);
}

void plat_present_canvas(grs_canvas& canvas, float aspect)
{
	GL_Clear();
	I_SetScreenRect(canvas.cv_bitmap.bm_w, canvas.cv_bitmap.bm_h, aspect);
	GL_DrawCanvas(canvas, screenRectangle);

	SDL_GL_SwapWindow(gameWindow);
}

void plat_present_canvas(grs_canvas& canvas)
{
	float aspect = (float)canvas.cv_bitmap.bm_h / canvas.cv_bitmap.bm_w;
	plat_present_canvas(canvas, aspect);
}

void plat_blit_canvas(grs_canvas *canv)
{
}

void plat_close()
{
	//SDL_GL_UnloadLibrary();
	plat_close_window();
	SDL_Quit();
}

void plat_display_error(const char* msg)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Game Error", msg, gameWindow);
}

int plat_get_num_cpus()
{
	return SDL_GetCPUCount();
}

#endif

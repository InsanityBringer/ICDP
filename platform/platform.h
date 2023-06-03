/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/

#pragma once

#include "misc/types.h"
#include "2d/gr.h"

extern int WindowWidth, WindowHeight;
extern int BestFit;
extern int Fullscreen;
extern int SwapInterval;

extern bool NoOpenGL;

//-----------------------------------------------------------------------------
//	Graphics initalization and shutdown
//-----------------------------------------------------------------------------

//Init the framework
int plat_init();

//Load configuration
int plat_read_chocolate_cfg();

//Init graphics library and create a window
int plat_create_window();

//Close any active windows
void plat_close_window();

//Shutdown framework;
void plat_close();

//Display an error message
void plat_display_error(const char* msg);

//Gets the amount of logical CPUs for this machine. 
int plat_get_num_cpus();

//Updates the window, if settings changed in game
void plat_update_window();

//-----------------------------------------------------------------------------
//	Screen palettes
//-----------------------------------------------------------------------------

//Copy a block of palette information into the current palette
void plat_write_palette(int start, int end, uint8_t* data);

//Blank the palette in screen memory
void plat_blank_palette();

//Read the palette back from screen memory.
void plat_read_palette(uint8_t* dest);

//-----------------------------------------------------------------------------
//	Screen operations
//-----------------------------------------------------------------------------

constexpr float ASPECT_4_3 = 3.f / 4.f;
constexpr float ASPECT_5_4 = 4.f / 5.f;
constexpr float ASPECT_16_9 = 9.f / 16.f;
constexpr float ASPECT_16_10 = 10.f / 16.f;

//I have no idea how this is going to work... attempt to wait on a VBL if possible.
void plat_wait_for_vbl();

//Draws the contents of the currently assigned graphics canvas to the current window and peform buffer swap
//Set sync to wait for v-sync while drawing.
void plat_present_canvas(int sync);

//Composition nightmare: Blit given canvas to window buffer, don't trigger redraw. This is needed for paged graphics modes in Descent 1. 
void plat_blit_canvas(grs_canvas *canv);

//New screen drawing commands. Unlike the original Chocolate video system, there are no virtual screen modes, instead software canvases are explicitly drawn using the following functions
//Clears the screen. Call when you're drawing a new screen.
void plat_clear_screen();
//Presents the contents of a canvas in the center of the screen
//The rowsize of the canvas must match its width or weird things will happen.
void plat_present_canvas(grs_canvas& canv);
//Presents the contents of a canvas in the center of the screen, contorted to a box that fits the specified aspect ratio.
//Aspect ratio is height / width.
//The rowsize of the canvas must match its width or weird things will happen.
void plat_present_canvas(grs_canvas& canv, float aspect);

//temporary function
//presents a canvas but doesn't flip the native back buffer. 
void plat_present_canvas_no_flip(grs_canvas& canv, float aspect);

//Masked canvases are similar to normal canvases, but any usage of palette index 255 will render as transparent. This can be used to overlay graphics
void plat_present_canvas_masked(grs_canvas& canv, float aspect);

//Draws canvas canv on top of base, matching its rectangle
void plat_present_canvas_masked_on(grs_canvas& canv, grs_canvas& base, float aspect);

//Flips the native back buffer. This is no longer implicit because the new presentation system lets you present multiple canvases
void plat_flip();

//-----------------------------------------------------------------------------
//	Control operations
//-----------------------------------------------------------------------------

//Run all pending events. 
void plat_do_events();

//Put the mouse in relative mode
void plat_set_mouse_relative_mode(int state);

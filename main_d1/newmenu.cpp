/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <stack>
#include <memory>

#include "platform/platform_filesys.h"
#include "platform/posixstub.h"
#include "platform/findfile.h"
#include "misc/error.h"
#include "misc/types.h"
#include "2d/gr.h"
#include "platform/mono.h"
#include "platform/key.h"
#include "2d/palette.h"
#include "game.h"
#include "stringtable.h"
#include "newmenu.h"
#include "newmenu_windows.h"
#include "gamefont.h"
#include "network.h"
#include "iff/iff.h"
#include "2d/pcx.h"
#include "mem/mem.h"
#include "platform/mouse.h"
#include "platform/joy.h"
#include "main_shared/digi.h"
#include "multi.h"
#include "endlevel.h"
#include "screens.h"
#include "kconfig.h"
#include "player.h"
#include "platform/platform.h"

#define TITLE_FONT  	(Gamefonts[GFONT_BIG_1])

#define SUBTITLE_FONT	(Gamefonts[GFONT_MEDIUM_3])
#define CURRENT_FONT  	(Gamefonts[GFONT_MEDIUM_2])
#define NORMAL_FONT  	(Gamefonts[GFONT_MEDIUM_1])
#define TEXT_FONT  		(Gamefonts[GFONT_MEDIUM_3])
#define SMALL_FONT		(Gamefonts[GFONT_SMALL])

#define NORMAL_CHECK_BOX	"\x81"
#define CHECKED_CHECK_BOX	"\x82"
#define NORMAL_RADIO_BOX	"\x7F"
#define CHECKED_RADIO_BOX	"\x80"
#define CURSOR_STRING		"_"
#define SLIDER_LEFT			"\x83"		// 131
#define SLIDER_RIGHT		"\x84"		// 132
#define SLIDER_MIDDLE		"\x85"		// 133
#define SLIDER_MARKER		"\x86"		// 134

int Newmenu_first_time = 1;

grs_canvas* nm_canvas;

constexpr int MESSAGEBOX_TEXT_SIZE = 300;	// How many characters in messagebox

grs_bitmap nm_background;

extern void gr_bm_bitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);

std::stack<grs_canvas*> menu_canvas_stack;

//Stack of all available menus.
//The menu on top will be the one that gets all input, but prior menus will remain in order to return to the previous window.
std::stack <std::unique_ptr<nm_window>> nm_open_windows;

//Queue of windows that are going to be opening.
//This is done so that I can clean up all closed windows on top of the stack, and then add and draw new menus
std::queue<std::unique_ptr<nm_window>> nm_queued_windows;

grs_canvas* nm_get_top_canvas()
{
	return nm_canvas;
}

void newmenu_close(void) 
{
	if (nm_background.bm_data)
		free(nm_background.bm_data);
	if (nm_canvas)
		gr_free_canvas(nm_canvas);
	Newmenu_first_time = 1;
}

void newmenu_init(void)
{
	nm_canvas = gr_create_canvas(320, 200);
	atexit(newmenu_close);
}

void nm_draw_background1(const char* filename)
{
	int pcx_error;
	grs_bitmap* bmp;
	int x, y;

	gr_clear_canvas(BM_XRGB(0, 0, 0));
	x = (grd_curcanv->cv_bitmap.bm_w - 320) / 2;
	y = (grd_curcanv->cv_bitmap.bm_h - 200) / 2;
	bmp = gr_create_sub_bitmap(&grd_curcanv->cv_bitmap, x, y, 320, 200);
	pcx_error = pcx_read_bitmap(filename, bmp, bmp->bm_type, NULL);
	Assert(pcx_error == PCX_ERROR_NONE);

	gr_free_sub_bitmap(bmp);
}

void nm_draw_background(int x1, int y1, int x2, int y2)
{
	if (Newmenu_first_time) 
	{
		int pcx_error;
		uint8_t newpal[768];
		Newmenu_first_time = 0;

		nm_background.bm_data = NULL;
		pcx_error = pcx_read_bitmap("SCORES.PCX", &nm_background, BM_LINEAR, newpal);
		Assert(pcx_error == PCX_ERROR_NONE);

		gr_remap_bitmap_good(&nm_background, newpal, -1, -1);
	}

	if (x1 < 0) x1 = 0;
	if (y1 < 0) y1 = 0;

	int w = x2 - x1 + 1;
	int h = y2 - y1 + 1;

	if (w > nm_background.bm_w) w = nm_background.bm_w;
	if (h > nm_background.bm_h) h = nm_background.bm_h;

	x2 = x1 + w - 1;
	y2 = y1 + h - 1;

	gr_bm_bitblt(w, h, x1, y1, 0, 0, &nm_background, &(grd_curcanv->cv_bitmap));

	Gr_scanline_darkening_level = 2 * 7;

	gr_setcolor(BM_XRGB(0, 0, 0));
	gr_urect(x2 - 5, y1 + 5, x2 - 5, y2 - 5);
	gr_urect(x2 - 4, y1 + 4, x2 - 4, y2 - 5);
	gr_urect(x2 - 3, y1 + 3, x2 - 3, y2 - 5);
	gr_urect(x2 - 2, y1 + 2, x2 - 2, y2 - 5);
	gr_urect(x2 - 1, y1 + 1, x2 - 1, y2 - 5);
	gr_urect(x2 + 0, y1 + 0, x2 - 0, y2 - 5);

	gr_urect(x1 + 5, y2 - 5, x2, y2 - 5);
	gr_urect(x1 + 4, y2 - 4, x2, y2 - 4);
	gr_urect(x1 + 3, y2 - 3, x2, y2 - 3);
	gr_urect(x1 + 2, y2 - 2, x2, y2 - 2);
	gr_urect(x1 + 1, y2 - 1, x2, y2 - 1);
	gr_urect(x1 + 0, y2, x2, y2 - 0);

	Gr_scanline_darkening_level = GR_FADE_LEVELS;
}

void nm_restore_background(int x, int y, int w, int h)
{
	int x1 = x; int x2 = x + w - 1;
	int y1 = y; int y2 = y + h - 1;

	if (x1 < 0) x1 = 0;
	if (y1 < 0) y1 = 0;

	if (x2 >= nm_background.bm_w) x2 = nm_background.bm_w - 1;
	if (y2 >= nm_background.bm_h) y2 = nm_background.bm_h - 1;

	w = x2 - x1 + 1;
	h = y2 - y1 + 1;

	gr_bm_bitblt(w, h, x1, y1, x1, y1, &nm_background, &(grd_curcanv->cv_bitmap));
}

// Draw a left justfied string
void nm_string(bkg* b, int w1, int x, int y, char* s)
{
	char* s1 = NULL;

	char* p = strchr(s, '\t');
	if (p && (w1 > 0)) {
		*p = '\0';
		s1 = p + 1;
	}

	int w, h, aw;
	gr_get_string_size(s, &w, &h, &aw);

	if (w1 > 0)
		w = w1;

	gr_bm_bitblt(b->background->bm_w - 15, h, 5, y, 5, y, b->background, &(grd_curcanv->cv_bitmap));

	gr_string(x, y, s);

	if (p && (w1 > 0)) 
	{
		if (s1)
		{
			gr_get_string_size(s1, &w, &h, &aw);
			gr_string(x + w1 - w, y, s1);
			*p = '\t';
		}
	}

}

// Draw a slider and it's string
void nm_string_slider(bkg* b, int w1, int x, int y, char* s)
{
	char* s1 = NULL;

	char* p = strchr(s, '\t');
	if (p) 
	{
		*p = '\0';
		s1 = p + 1;
	}

	int w, h, aw;
	gr_get_string_size(s, &w, &h, &aw);
	gr_bm_bitblt(b->background->bm_w - 15, h, 5, y, 5, y, b->background, &(grd_curcanv->cv_bitmap));

	gr_string(x, y, s);

	if (p) 
	{
		if (s1)
		{
			gr_get_string_size(s1, &w, &h, &aw);

			// CHANGED
			gr_bm_bitblt(w, 1, x + w1 - w, y, x + w1 - w, y, b->background, &(grd_curcanv->cv_bitmap));
			// CHANGED
			gr_bm_bitblt(w, 1, x + w1 - w, y + h - 1, x + w1 - w, y, b->background, &(grd_curcanv->cv_bitmap));

			gr_string(x + w1 - w, y, s1);

			*p = '\t';
		}
	}
}

// Draw a left justfied string with black background.
void nm_string_black(bkg* b, int w1, int x, int y, char* s)
{
	int w, h, aw;
	gr_get_string_size(s, &w, &h, &aw);
	b = b;
	if (w1 == 0) w1 = w;

	gr_setcolor(BM_XRGB(0, 0, 0));
	gr_rect(x, y, x + w1 - 1, y + h - 1);

	gr_string(x, y, s);
}


// Draw a right justfied string
void nm_rstring(bkg* b, int w1, int x, int y, const char* s)
{
	int w, h, aw;
	gr_get_string_size(s, &w, &h, &aw);
	x -= 3;

	if (w1 == 0) w1 = w;

	//mprintf( 0, "Width = %d, string='%s'\n", w, s );

	gr_bm_bitblt(w1, h, x - w1, y, x - w1, y, b->background, &(grd_curcanv->cv_bitmap));

	gr_string(x - w, y, s);
}

#include "platform/timer.h"

//for text items, constantly redraw cursor (to achieve flash)
void update_cursor(newmenu_item* item)
{
	int w, h, aw;
	fix time = timer_get_approx_seconds();
	const char* text = item->text;

	Assert(item->type == NM_TYPE_INPUT_MENU || item->type == NM_TYPE_INPUT);

	while (*text) 
	{
		gr_get_string_size(text, &w, &h, &aw);
		if (w > item->w - 10)
			text++;
		else
			break;
	}
	if (*text == 0)
		w = 0;
	int x = item->x + w; int y = item->y;

	if (time & 0x8000)
		gr_string(x, y, CURSOR_STRING);
	else 
	{
		gr_setcolor(BM_XRGB(0, 0, 0));
		gr_rect(x, y, x + grd_curcanv->cv_font->ft_w - 1, y + grd_curcanv->cv_font->ft_h - 1);
	}

}

void nm_string_inputbox(bkg* b, int w, int x, int y, char* text, int current)
{
	int w1, h1, aw;

	while (*text) 
	{
		gr_get_string_size(text, &w1, &h1, &aw);
		if (w1 > w - 10)
			text++;
		else
			break;
	}
	if (*text == 0)
		w1 = 0;
	nm_string_black(b, w, x, y, text);

	if (current) 
	{
		gr_string(x + w1, y, CURSOR_STRING);
	}
}


char* Newmenu_allowed_chars = NULL;

//returns true if char is allowed
int char_allowed(char c)
{
	char* p = Newmenu_allowed_chars;

	if (!p)
		return 1;

	while (*p) 
	{
		Assert(p[1]);

		if (c >= p[0] && c <= p[1])
			return 1;

		p += 2;
	}

	return 0;
}

void strip_end_whitespace(char* text)
{
	int l = strlen(text);
	for (int i = l - 1; i >= 0; i--)
	{
		if (isspace(text[i]))
			text[i] = 0;
		else
			return;
	}
}

int newmenu_do(const char* title, const char* subtitle, int nitems, newmenu_item* item, void (*subfunction)(int nitems, newmenu_item* items, int* last_key, int citem))
{
	return newmenu_do3(title, subtitle, nitems, item, subfunction, 0, NULL, -1, -1);
}

int newmenu_do1(const char* title, const char* subtitle, int nitems, newmenu_item* item, void (*subfunction)(int nitems, newmenu_item* items, int* last_key, int citem), int citem)
{
	return newmenu_do3(title, subtitle, nitems, item, subfunction, citem, NULL, -1, -1);
}

int newmenu_do2(const char* title, const char* subtitle, int nitems, newmenu_item* item, void (*subfunction)(int nitems, newmenu_item* items, int* last_key, int citem), int citem, const char* filename)
{
	return newmenu_do3(title, subtitle, nitems, item, subfunction, citem, filename, -1, -1);
}


void newmenu_open(const char* title, const char* subtitle, std::vector<newmenu_item>& items, void (*subfunction)(int nitems, newmenu_item* items, int* last_key, int citem), bool (*choicefunc)(int choice, int nitems, newmenu_item* item), int citem, const char* filename, int width, int height, bool tiny_mode)
{
	//TODO: Centralize this
	if (Function_mode == FMODE_GAME && !(Game_mode & GM_MULTI))
		game_pause(true);
	nm_queued_windows.push(std::make_unique<nm_menu>(items, title, subtitle, subfunction, choicefunc, citem, filename, width, height, tiny_mode));
}

void newmenu_open(const char* title, const char* subtitle, int nitems, newmenu_item* items, void (*subfunction)(int nitems, newmenu_item* items, int* last_key, int citem), bool (*choicefunc)(int choice, int nitems, newmenu_item* item), int citem, const char* filename, int width, int height, bool tiny_mode)
{
	std::vector<newmenu_item> itemclone;
	for (int i = 0; i < nitems; i++)
		itemclone.push_back(items[i]);

	//TODO: Centralize this
	if (Function_mode == FMODE_GAME && !(Game_mode & GM_MULTI))
		game_pause(true);
	nm_queued_windows.push(std::make_unique<nm_menu>(itemclone, title, subtitle, subfunction, choicefunc, citem, filename, width, height, tiny_mode));
}

//Simple callback for purely informative message boxes
bool newmenu_messagebox_informative_callback(int choice, int nitems, newmenu_item* item)
{
	return false; //never remain open. 
}

void nm_open_messagebox(const char* title, bool (*callback)(int choice, int nitems, newmenu_item* item), int nchoices, ...)
{
	va_list args;
	char nm_text[MESSAGEBOX_TEXT_SIZE];
	std::vector<newmenu_item> nm_message_items;

	va_start(args, nchoices);

	Assert(nchoices <= 5);

	if (callback == nullptr)
		callback = newmenu_messagebox_informative_callback;

	nm_message_items.resize(nchoices);
	for (int i = 0; i < nchoices; i++)
	{
		char* s = va_arg(args, char*);
		nm_message_items[i] = {};
		nm_message_items[i].type = NM_TYPE_MENU; nm_message_items[i].text = s;
	}
	char* format = va_arg(args, char*);
	sprintf(nm_text, "");
	vsnprintf(nm_text, MESSAGEBOX_TEXT_SIZE, format, args);
	nm_text[MESSAGEBOX_TEXT_SIZE - 1] = '\0';
	va_end(args);

	Assert(strlen(nm_text) < MESSAGEBOX_TEXT_SIZE);

	newmenu_open(title, nm_text, nm_message_items, nullptr, callback);
}


void newmenu_open_filepicker(const char* title, const char* filespec, int allow_abort_flag, void (*callback)(std::string& str, int num))
{
	nm_queued_windows.push(std::make_unique<nm_filelist>(title, filespec, callback, allow_abort_flag));
	//TODO: Centralize this
	if (Function_mode == FMODE_GAME && !(Game_mode & GM_MULTI))
		game_pause(true);
}

void newmenu_frame()
{
	bool did_frame = false;
	//Cleanup and pop any closed windows from the stack
	bool cleanup = true;

	bool was_open = false;

	while (!nm_open_windows.empty() && cleanup)
	{
		std::unique_ptr<nm_window>& top = nm_open_windows.top();
		if (top->is_closing()) //Has the window closed?
		{
			top->cleanup();
			nm_open_windows.pop(); //kill it, and then loop to check the next window.
			was_open = true; 
		}
		else
		{
			cleanup = false; //done cleaning for now. 
		}
	}

	//Add queued windows to the stack
	while (!nm_queued_windows.empty())
	{
		std::unique_ptr<nm_window>& front = nm_queued_windows.front();
		nm_open_windows.push(std::move(front));
		nm_queued_windows.pop();
	}

	while (!nm_open_windows.empty() && !did_frame)
	{
		std::unique_ptr<nm_window>& top = nm_open_windows.top();
		
		//Drawing should be delayed, otherwise opening a window while closing the current won't work right
		if (top->must_draw())
			top->draw();

		top->frame(); //Run the window frame. 
		did_frame = true;
	}

	//If any windows were killed this frame, check if the game should be unpaused. 
	if (was_open && nm_queued_windows.empty() && nm_open_windows.empty())
		if (Function_mode == FMODE_GAME && !(Game_mode & GM_MULTI))
			game_pause(false);
}

void newmenu_present()
{
	plat_present_canvas_masked(*nm_canvas, 3.f / 4.f);
}

bool newmenu_empty()
{
	return nm_open_windows.empty() && nm_queued_windows.empty();
}

void newmenu_close_all()
{
	while (!nm_open_windows.empty())
	{
		std::unique_ptr<nm_window>& top = nm_open_windows.top();
		if (!top->is_closing())
			top->close();

		nm_open_windows.pop();
	}

	while (!nm_queued_windows.empty())
	{
		nm_queued_windows.pop(); //Queued windows shouldn't have drawn yet. 
	}

	if (Function_mode == FMODE_GAME && !(Game_mode & GM_MULTI))
		game_pause(false);
	
	gr_set_current_canvas(nm_canvas);
	//Since we probably want to draw something else, clear the menu canvas to transparent
	gr_clear_canvas(255);
}

int newmenu_do3(const char* title, const char* subtitle, int nitems, newmenu_item* item, void (*subfunction)(int nitems, newmenu_item* items, int* last_key, int citem), int citem, const char* filename, int width, int height, bool tiny_mode)
{
	Int3();
	return -1;
}

int nm_messagebox1(const char* title, void (*subfunction)(int nitems, newmenu_item* items, int* last_key, int citem), int nchoices, ...)
{
	va_list args;
	char nm_text[MESSAGEBOX_TEXT_SIZE];
	newmenu_item nm_message_items[5];

	va_start(args, nchoices);

	Assert(nchoices <= 5);

	for (int i = 0; i < nchoices; i++) 
	{
		char* s = va_arg(args, char*);
		nm_message_items[i].type = NM_TYPE_MENU; nm_message_items[i].text = s;
	}
	char* format = va_arg(args, char*);
	sprintf(nm_text, "");
	vsnprintf(nm_text, MESSAGEBOX_TEXT_SIZE, format, args);
	nm_text[MESSAGEBOX_TEXT_SIZE - 1] = '\0';
	va_end(args);

	Assert(strlen(nm_text) < MESSAGEBOX_TEXT_SIZE);

	return newmenu_do(title, nm_text, nchoices, nm_message_items, subfunction);
}

int nm_messagebox(const char* title, int nchoices, ...)
{
	va_list args;
	char nm_text[MESSAGEBOX_TEXT_SIZE];
	newmenu_item nm_message_items[5];

	va_start(args, nchoices);

	Assert(nchoices <= 5);

	for (int i = 0; i < nchoices; i++) 
	{
		char* s = va_arg(args, char*);
		nm_message_items[i].type = NM_TYPE_MENU; nm_message_items[i].text = s;
	}
	char* format = va_arg(args, char*);
	sprintf(nm_text, "");
	vsnprintf(nm_text, MESSAGEBOX_TEXT_SIZE, format, args);
	nm_text[MESSAGEBOX_TEXT_SIZE - 1] = '\0';
	va_end(args);

	Assert(strlen(nm_text) < MESSAGEBOX_TEXT_SIZE);

	return newmenu_do(title, nm_text, nchoices, nm_message_items, NULL);
}

#define MAX_FILES 100

void MakeNewPlayerFile(int allow_abort);

int newmenu_get_filename(const char* title, const char* filespec, char* filename, int allow_abort_flag)
{
	Int3();
	return 0;
}

void newmenu_open_listbox(const char* title, int nitems, char* items[], bool allow_abort_flag, void (*callback)(int choice), int default_item)
{
	std::vector<char*> itemsclone;
	for (int i = 0; i < nitems; i++)
	{
		itemsclone.push_back(items[i]);
	}

	nm_queued_windows.push(std::make_unique<nm_list>(title, itemsclone, allow_abort_flag, default_item, callback));
}

void newmenu_open_listbox(const char* title, std::vector<char*> items, bool allow_abort_flag, void (*callback)(int choice), int default_item)
{
	nm_queued_windows.push(std::make_unique<nm_list>(title, items, allow_abort_flag, default_item, callback));
}

#define LB_ITEMS_ON_SCREEN 8

int newmenu_listbox(const char* title, int nitems, char* items[], int allow_abort_flag, int (*listbox_callback)(int* citem, int* nitems, char* items[], int* keypress))
{
	return newmenu_listbox1(title, nitems, items, allow_abort_flag, 0, listbox_callback);
}

int newmenu_listbox1(const char* title, int nitems, char* items[], int allow_abort_flag, int default_item, int (*listbox_callback)(int* citem, int* nitems, char* items[], int* keypress))
{
	Int3();
	return 0;
}

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

struct bkg
{
	grs_canvas* menu_canvas;
	grs_bitmap* saved;			// The background under the menu.
	grs_bitmap* background;
};

grs_canvas* nm_canvas;

constexpr int MESSAGEBOX_TEXT_SIZE = 300;	// How many characters in messagebox
constexpr int MAX_TEXT_WIDTH = 200;			// How many pixels wide a input box can be

grs_bitmap nm_background;

extern void gr_bm_bitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);

std::stack<grs_canvas*> menu_canvas_stack;

//Stack of all available menus.
//The menu on top will be the one that gets all input, but prior menus will remain in order to return to the previous window.
std::stack <std::unique_ptr<nm_window>> nm_open_windows;

grs_canvas* nm_get_top_canvas()
{
	if (menu_canvas_stack.empty())
		return nullptr;

	return menu_canvas_stack.top();
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

void draw_item(bkg* b, newmenu_item* item, int is_current, bool tiny)
{
	if (tiny)
	{
		if (is_current)
			gr_set_fontcolor(gr_find_closest_color_current(57, 49, 20), -1);
		else
			gr_set_fontcolor(gr_find_closest_color_current(29, 29, 47), -1);

		//Not quite sure why Descent 2 does this
		if (item->text[0] == '\t')
			gr_set_fontcolor(gr_find_closest_color_current(63, 63, 63), -1);
	}
	else
	{
		if (is_current)
			grd_curcanv->cv_font = CURRENT_FONT;
		else if (item->type == NM_TYPE_TEXT)
			grd_curcanv->cv_font = TEXT_FONT; //Need to do this here for tiny rendering to work right
		else
			grd_curcanv->cv_font = NORMAL_FONT;
	}

	switch (item->type) 
	{
	case NM_TYPE_TEXT:
	case NM_TYPE_MENU:
		nm_string(b, item->w, item->x, item->y, item->text);
		break;
	case NM_TYPE_SLIDER: 
	{
		if (item->value < item->min_value) item->value = item->min_value;
		if (item->value > item->max_value) item->value = item->max_value;
		sprintf(item->saved_text, "%s\t%s", item->text, SLIDER_LEFT);
		for (int j = 0; j < (item->max_value - item->min_value + 1); j++) 
		{
			strcat(item->saved_text, SLIDER_MIDDLE);
		}
		strcat(item->saved_text, SLIDER_RIGHT);

		item->saved_text[item->value + 1 + strlen(item->text) + 1] = SLIDER_MARKER[0];

		nm_string_slider(b, item->w, item->x, item->y, item->saved_text);
	}
						 break;
	case NM_TYPE_INPUT_MENU:
		if (item->group == 0) 
		{
			nm_string(b, item->w, item->x, item->y, item->text);
		}
		else 
		{
			nm_string_inputbox(b, item->w, item->x, item->y, item->text, is_current);
		}
		break;
	case NM_TYPE_INPUT:
		nm_string_inputbox(b, item->w, item->x, item->y, item->text, is_current);
		break;
	case NM_TYPE_CHECK:
		nm_string(b, item->w, item->x, item->y, item->text);
		if (item->value)
			nm_rstring(b, item->right_offset, item->x, item->y, CHECKED_CHECK_BOX);
		else
			nm_rstring(b, item->right_offset, item->x, item->y, NORMAL_CHECK_BOX);
		break;
	case NM_TYPE_RADIO:
		nm_string(b, item->w, item->x, item->y, item->text);
		if (item->value)
			nm_rstring(b, item->right_offset, item->x, item->y, CHECKED_RADIO_BOX);
		else
			nm_rstring(b, item->right_offset, item->x, item->y, NORMAL_RADIO_BOX);
		break;
	case NM_TYPE_NUMBER: 
	{
		char text[10];
		if (item->value < item->min_value) item->value = item->min_value;
		if (item->value > item->max_value) item->value = item->max_value;
		nm_string(b, item->w, item->x, item->y, item->text);
		sprintf(text, "%d", item->value);
		nm_rstring(b, item->right_offset, item->x, item->y, text);
	}
		break;
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

//-----------------------------------------------------------------------------
// nm_menu
//
// Implements a normal menu that can have a couple of different widgets
//-----------------------------------------------------------------------------

class nm_menu : public nm_window
{
	std::string title, subtitle, bg_filename;
	std::vector<newmenu_item> items;
	int choice, old_choice;
	int tw, th;
	int w, h, x, y;
	bool tiny, all_text;
	bkg bg;

	//Callback executed when a choice is selected.
	//Returns true if the menu should remain open, or false if it should close. 
	bool (*choice_callback)(int choice);
	//Callback executed each frame, to update and redraw contents. 
	void (*newmenu_callback)(int nitems, newmenu_item* items, int* last_key, int citem);
public:
	nm_menu(std::vector<newmenu_item>& source_items, const char* new_title, const char* new_subtitle,
		void (*subfunction)(int nitems, newmenu_item* items, int* last_key, int citem), bool (*choicefunc)(int choice),
		int citem, const char* filename, int width, int height, bool tiny_mode)
	{
		for (newmenu_item& item : source_items)
			items.push_back(item);

		grs_canvas* save_canvas = grd_curcanv;
		gr_set_current_canvas(nm_canvas);

		choice = old_choice = 0;

		Assert(choicefunc != nullptr);

		bg = {};
		all_text = false;
		choice_callback = choicefunc;
		newmenu_callback = subfunction;

		if (new_title)
			title = std::string(new_title);
		if (new_subtitle)
			subtitle = std::string(new_subtitle);

		//Temp variables
		int string_width, string_height, average_width;

		tw = th = 0;

		//Find the size of the title and subtitle.
		if (title.size())
		{
			grd_curcanv->cv_font = TITLE_FONT;
			gr_get_string_size(title.c_str(), &string_width, &string_height, &average_width);
			tw = string_width;
			th = string_height;
		}
		if (subtitle.size())
		{
			grd_curcanv->cv_font = SUBTITLE_FONT;
			gr_get_string_size(subtitle.c_str(), &string_width, &string_height, &average_width);
			if (string_width > tw)
				tw = string_width;
			th += string_height;
		}

		th += 8;		//put some space between titles & body

		tiny = tiny_mode;
		if (tiny)
			grd_curcanv->cv_font = SMALL_FONT;
		else
			grd_curcanv->cv_font = NORMAL_FONT;

		w = 0; h = th;
		int aw = 0;

		//Find ths size of all the menu items
		for (newmenu_item& item : items)
		{
			item.redraw = 1;
			item.y = h;
			gr_get_string_size(item.text, &string_width, &string_height, &average_width);
			item.right_offset = 0;

			item.saved_text[0] = '\0';

			if (item.type == NM_TYPE_SLIDER)
			{
				int w1, h1, aw1;
				sprintf(item.saved_text, "%s", SLIDER_LEFT);
				for (int j = 0; j < (item.max_value - item.min_value + 1); j++)
				{
					sprintf(item.saved_text, "%s%s", item.saved_text, SLIDER_MIDDLE);
				}
				sprintf(item.saved_text, "%s%s", item.saved_text, SLIDER_RIGHT);
				gr_get_string_size(item.saved_text, &w1, &h1, &aw1);
				string_width += w1 + aw;
			}

			if (item.type == NM_TYPE_CHECK)
			{
				int w1, h1, aw1;
				gr_get_string_size(NORMAL_CHECK_BOX, &w1, &h1, &aw1);
				item.right_offset = w1;
				gr_get_string_size(CHECKED_CHECK_BOX, &w1, &h1, &aw1);
				if (w1 > item.right_offset)
					item.right_offset = w1;
			}

			if (item.type == NM_TYPE_RADIO)
			{
				int w1, h1, aw1;
				gr_get_string_size(NORMAL_RADIO_BOX, &w1, &h1, &aw1);
				item.right_offset = w1;
				gr_get_string_size(CHECKED_RADIO_BOX, &w1, &h1, &aw1);
				if (w1 > item.right_offset)
					item.right_offset = w1;
			}

			if (item.type == NM_TYPE_NUMBER)
			{
				int w1, h1, aw1;
				char test_text[20];
				sprintf(test_text, "%d", item.max_value);
				gr_get_string_size(test_text, &w1, &h1, &aw1);
				item.right_offset = w1;
				sprintf(test_text, "%d", item.min_value);
				gr_get_string_size(test_text, &w1, &h1, &aw1);
				if (w1 > item.right_offset)
					item.right_offset = w1;
			}

			if (item.type == NM_TYPE_INPUT)
			{
				Assert(strlen(item.text) < NM_MAX_TEXT_LEN);
				strcpy(item.saved_text, item.text);
				string_width = item.text_len * grd_curcanv->cv_font->ft_w + item.text_len;
				if (string_width > MAX_TEXT_WIDTH)
					string_width = MAX_TEXT_WIDTH;
				item.value = -1;
			}

			if (item.type == NM_TYPE_INPUT_MENU)
			{
				Assert(strlen(item.text) < NM_MAX_TEXT_LEN);
				strcpy(item.saved_text, item.text);
				string_width = item.text_len * grd_curcanv->cv_font->ft_w + item.text_len;
				item.value = -1;
				item.group = 0;
			}

			item.w = string_width;
			item.h = string_height;

			if (string_width > w)
				w = string_width;		// Save maximum width
			if (average_width > aw)
				aw = average_width;
			h += string_height + 1;		// Find the height of all strings
		}

		int right_offset = 0;

		if (width > -1)
			w = width;

		if (height > -1)
			h = height;

		for (newmenu_item& item : items)
		{
			item.w = w;
			if (item.right_offset > right_offset)
				right_offset = item.right_offset;
		}
		if (right_offset > 0)
			right_offset += 3;

		//mprintf( 0, "Right offset = %d\n", right_offset );

		//gr_get_string_size("",&string_width,&string_height,&average_width );

		w += right_offset;

		int twidth = 0;
		if (tw > w)
		{
			twidth = (tw - w) / 2;
			w = tw;
		}

		// Find min point of menu border
		w += 30;
		h += 30;

		if (w > 320) w = 320;
		if (h > 200) h = 200;

		x = (grd_curcanv->cv_bitmap.bm_w - w) / 2;
		y = (grd_curcanv->cv_bitmap.bm_h - h) / 2;

		if (x < 0) x = 0;
		if (y < 0) y = 0;
		if (filename != NULL)
		{
			nm_draw_background1(filename);
			bg_filename = std::string(filename);
		}

		// Save the background of the display
		bg.menu_canvas = gr_create_sub_canvas(nm_canvas, x, y, w, h);
		gr_set_current_canvas(bg.menu_canvas);

		if (filename == NULL)
		{
			// Save the background under the menu...
			bg.saved = gr_create_bitmap(w, h);
			Assert(bg.saved != NULL);
			gr_bm_bitblt(w, h, 0, 0, 0, 0, &grd_curcanv->cv_bitmap, bg.saved);
			gr_set_current_canvas(nm_canvas);
			nm_draw_background(x, y, x + w - 1, y + h - 1);
			bg.background = gr_create_sub_bitmap(&nm_background, 0, 0, w, h);
			gr_set_current_canvas(bg.menu_canvas);
		}
		else
		{
			bg.saved = NULL;
			bg.background = gr_create_bitmap(w, h);
			Assert(bg.background != NULL);
			gr_bm_bitblt(w, h, 0, 0, 0, 0, &grd_curcanv->cv_bitmap, bg.background);
		}

		int ty = 15;

		if (title.size() > 0)
		{
			grd_curcanv->cv_font = TITLE_FONT;
			gr_set_fontcolor(GR_GETCOLOR(31, 31, 31), -1);
			gr_get_string_size(title.c_str(), &string_width, &string_height, &average_width);
			tw = string_width;
			th = string_height;
			gr_printf(0x8000, ty, title.c_str());
			ty += th;
		}

		if (subtitle.size() > 0)
		{
			grd_curcanv->cv_font = SUBTITLE_FONT;
			gr_set_fontcolor(GR_GETCOLOR(21, 21, 21), -1);
			gr_get_string_size(subtitle.c_str(), &string_width, &string_height, &average_width);
			tw = string_width;
			th = string_height;
			gr_printf(0x8000, ty, subtitle.c_str());
			ty += th;
		}

		if (tiny_mode)
			grd_curcanv->cv_font = SMALL_FONT;
		else
			grd_curcanv->cv_font = NORMAL_FONT;

		// Update all item's x & y values.
		for (newmenu_item& item : items)
		{
			item.x = 15 + twidth + right_offset;
			item.y += 15;
			if (item.type == NM_TYPE_RADIO)
			{
				int fm = -1;	// find first marked one
				for (int j = 0; j < items.size(); j++)
				{
					if (items[j].type == NM_TYPE_RADIO && items[j].group == item.group)
					{
						if (fm == -1 && items[j].value)
							fm = j;
						items[j].value = 0;
					}
				}
				if (fm >= 0)
					items[fm].value = 1;
				else
					item.value = 1;
			}
		}

		if (citem == -1)
		{
			choice = -1;
		}
		else
		{
			if (citem < 0) citem = 0;
			if (citem > items.size() - 1) citem = items.size() - 1;
			choice = citem;

			while (items[choice].type == NM_TYPE_TEXT)
			{
				choice++;
				if (choice >= items.size())
				{
					choice = 0;
				}
				if (choice == citem)
				{
					choice = 0;
					all_text = true;
					break;
				}
			}
		}

		gr_set_current_canvas(save_canvas);
	}

	void frame() override
	{
		bool done = false;
		int k = key_inkey();

		gr_set_current_canvas(bg.menu_canvas);

		if (newmenu_callback)
			(*newmenu_callback)(items.size(), items.data(), &k, choice);

		if (k < -1)
		{
			choice = k;
			k = -1;
			done = true;
		}

		old_choice = choice;

		switch (k)
		{
		case KEY_TAB + KEY_SHIFTED:
		case KEY_UP:
		case KEY_PAD8:
			if (all_text) break;
			do
			{
				choice--;
				if (choice >= items.size()) choice = 0;
				if (choice < 0) choice = items.size() - 1;
			} while (items[choice].type == NM_TYPE_TEXT);
			if ((items[choice].type == NM_TYPE_INPUT) && (choice != old_choice))
				items[choice].value = -1;
			if ((old_choice > -1) && (items[old_choice].type == NM_TYPE_INPUT_MENU) && (old_choice != choice))
			{
				items[old_choice].group = 0;
				strcpy(items[old_choice].text, items[old_choice].saved_text);
				items[old_choice].value = -1;
			}
			if (old_choice > -1)
				items[old_choice].redraw = 1;
			items[choice].redraw = 1;
			break;
		case KEY_TAB:
		case KEY_DOWN:
		case KEY_PAD2:
			if (all_text) break;
			do
			{
				choice++;
				if (choice < 0) choice = items.size() - 1;
				if (choice >= items.size()) choice = 0;
			} while (items[choice].type == NM_TYPE_TEXT);
			if ((items[choice].type == NM_TYPE_INPUT) && (choice != old_choice))
				items[choice].value = -1;
			if ((old_choice > -1) && (items[old_choice].type == NM_TYPE_INPUT_MENU) && (old_choice != choice))
			{
				items[old_choice].group = 0;
				strcpy(items[old_choice].text, items[old_choice].saved_text);
				items[old_choice].value = -1;
			}
			if (old_choice > -1)
				items[old_choice].redraw = 1;
			items[choice].redraw = 1;
			break;
		case KEY_SPACEBAR:
			if (choice > -1)
			{
				switch (items[choice].type)
				{
				case NM_TYPE_MENU:
				case NM_TYPE_INPUT:
				case NM_TYPE_INPUT_MENU:
					break;
				case NM_TYPE_CHECK:
					if (items[choice].value)
						items[choice].value = 0;
					else
						items[choice].value = 1;
					items[choice].redraw = 1;
					break;
				case NM_TYPE_RADIO:
					for (int i = 0; i < items.size(); i++)
					{
						if ((i != choice) && (items[i].type == NM_TYPE_RADIO) && (items[i].group == items[choice].group) && (items[i].value))
						{
							items[i].value = 0;
							items[i].redraw = 1;
						}
					}
					items[choice].value = 1;
					items[choice].redraw = 1;
					break;
				}
			}
			break;

		case KEY_ENTER:
		case KEY_PADENTER:
			if ((choice > -1) && (items[choice].type == NM_TYPE_INPUT_MENU) && (items[choice].group == 0))
			{
				items[choice].group = 1;
				items[choice].redraw = 1;
				if (!_strnicmp(items[choice].saved_text, TXT_EMPTY, strlen(TXT_EMPTY)))
				{
					items[choice].text[0] = 0;
					items[choice].value = -1;
				}
				else
				{
					strip_end_whitespace(items[choice].text);
				}
			}
			else
				done = true;
			break;

		case KEY_ESC:
			if ((choice > -1) && (items[choice].type == NM_TYPE_INPUT_MENU) && (items[choice].group == 1))
			{
				items[choice].group = 0;
				strcpy(items[choice].text, items[choice].saved_text);
				items[choice].redraw = 1;
				items[choice].value = -1;
			}
			else
			{
				done = true;
				choice = -1;
			}
			break;

		case KEY_PRINT_SCREEN: 		save_screen_shot(0); break;

#ifndef NDEBUG
		case KEY_BACKSP:
			if ((choice > -1) && (items[choice].type != NM_TYPE_INPUT) && (items[choice].type != NM_TYPE_INPUT_MENU))
				Int3();
			break;
#endif

		}

		if (choice > -1)
		{
			int ascii;

			if (((items[choice].type == NM_TYPE_INPUT) || ((items[choice].type == NM_TYPE_INPUT_MENU) && (items[choice].group == 1))) && (old_choice == choice))
			{
				if (k == KEY_LEFT || k == KEY_BACKSP || k == KEY_PAD4)
				{
					if (items[choice].value == -1) items[choice].value = strlen(items[choice].text);
					if (items[choice].value > 0)
						items[choice].value--;
					items[choice].text[items[choice].value] = 0;
					items[choice].redraw = 1;
				}
				else
				{
					ascii = key_to_ascii(k);
					if ((ascii < 255) && (items[choice].value < items[choice].text_len))
					{
						int allowed;

						if (items[choice].value == -1)
						{
							items[choice].value = 0;
						}

						allowed = char_allowed(ascii);

						if (!allowed && ascii == ' ' && char_allowed('_'))
						{
							ascii = '_';
							allowed = 1;
						}

						if (allowed)
						{
							items[choice].text[items[choice].value++] = ascii;
							items[choice].text[items[choice].value] = 0;
							items[choice].redraw = 1;
						}
					}
				}
			}
			else if ((items[choice].type != NM_TYPE_INPUT) && (items[choice].type != NM_TYPE_INPUT_MENU))
			{
				ascii = key_to_ascii(k);
				if (ascii < 255)
				{
					int choice1 = choice;
					ascii = toupper(ascii);
					do
					{
						int i, ch;
						choice1++;
						if (choice1 >= items.size()) choice1 = 0;
						for (i = 0; (ch = items[choice1].text[i]) != 0 && ch == ' '; i++);
						if (((items[choice1].type == NM_TYPE_MENU) ||
							(items[choice1].type == NM_TYPE_CHECK) ||
							(items[choice1].type == NM_TYPE_RADIO) ||
							(items[choice1].type == NM_TYPE_NUMBER) ||
							(items[choice1].type == NM_TYPE_SLIDER))
							&& (ascii == toupper(ch)))
						{
							k = 0;
							choice = choice1;
							if (old_choice > -1)
								items[old_choice].redraw = 1;
							items[choice].redraw = 1;
						}
					} while (choice1 != choice);
				}
			}

			if ((items[choice].type == NM_TYPE_NUMBER) || (items[choice].type == NM_TYPE_SLIDER))
			{
				int ov = items[choice].value;
				switch (k)
				{
				case KEY_PAD4:
				case KEY_LEFT:
				case KEY_MINUS:
				case KEY_MINUS + KEY_SHIFTED:
				case KEY_PADMINUS:
					items[choice].value -= 1;
					break;
				case KEY_RIGHT:
				case KEY_PAD6:
				case KEY_EQUAL:
				case KEY_EQUAL + KEY_SHIFTED:
				case KEY_PADPLUS:
					items[choice].value++;
					break;
				case KEY_PAGEUP:
				case KEY_PAD9:
				case KEY_SPACEBAR:
					items[choice].value += 10;
					break;
				case KEY_PAGEDOWN:
				case KEY_BACKSP:
				case KEY_PAD3:
					items[choice].value -= 10;
					break;
				}
				if (ov != items[choice].value)
					items[choice].redraw = 1;
			}

		}

		gr_set_current_canvas(bg.menu_canvas);
		// Redraw everything...
		for (int i = 0; i < items.size(); i++)
		{
			if (items[i].redraw)
			{
				draw_item(&bg, &items[i], (i == choice && !all_text), tiny);
				items[i].redraw = 0;
			}
			else if (i == choice && (items[i].type == NM_TYPE_INPUT || (items[i].type == NM_TYPE_INPUT_MENU && items[i].group)))
				update_cursor(&items[i]);
		}

		if (done) //A choice was made
		{
			bool keep_open = choice_callback(choice);
			if (keep_open)
			{
				if (choice == -1) //hit escape, but remaining open?
					choice = old_choice; 
			}
			else
			{
				close(); 
			}
		}
	}

	void cleanup() override
	{
		// Restore everything...
		gr_set_current_canvas(bg.menu_canvas);
		if (bg_filename.size() == 0)
		{
			// Save the background under the menu...
			gr_bitmap(0, 0, bg.saved);
			gr_free_bitmap(bg.saved);
			free(bg.background);
		}
		else
		{
			gr_bitmap(0, 0, bg.background);
			gr_free_bitmap(bg.background);
		}

		gr_free_sub_canvas(bg.menu_canvas);
	}
};

void newmenu_open(const char* title, const char* subtitle, std::vector<newmenu_item>& items, void (*subfunction)(int nitems, newmenu_item* items, int* last_key, int citem), bool (*choicefunc)(int choice), int citem, const char* filename)
{
	nm_open_windows.push(std::make_unique<nm_menu>(items, title, subtitle, subfunction, choicefunc, citem, filename, -1, -1, false));
}

//Simple callback for purely informative message boxes
bool newmenu_messagebox_informative_callback(int choice)
{
	return false; //never remain open. 
}

void nm_open_messagebox(const char* title, bool (*callback)(int choice), int nchoices, ...)
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


//-----------------------------------------------------------------------------
// nm_filelist
//
// Implements a list box that lists files. 
//-----------------------------------------------------------------------------

struct newmenu_fileentry
{
	std::string filename;
	std::string displayname;
	bool truncated;

	newmenu_fileentry()
	{
		truncated = false;
	}

	newmenu_fileentry(std::string_view newfilename, bool noext)
	{
		filename = newfilename;
		truncated = noext;
		if (noext)
		{
			std::string::size_type pos = filename.find_last_of(".");
			if (pos == std::string::npos)
				displayname = filename;
			else
				displayname = std::string(filename, 0, pos);
		}
		else
		{
			displayname = filename;
		}
	}

	//Helper function, truncated entries can have a $ as the first character, this needs to be stored but not displayed
	const char* get_display_name()
	{
		if (!truncated || (displayname.size() > 0 && displayname[0] != '$'))
			return displayname.c_str();

		return displayname.c_str() + 1;
	}
};

void newmenu_file_sort(std::span<newmenu_fileentry> entries)
{
	int n = entries.size();
	int incr = n / 2;
	newmenu_fileentry t;

	while (incr > 0)
	{
		for (int i = incr; i < n; i++)
		{
			int j = i - incr;
			while (j >= 0)
			{
				if (entries[j].filename.compare(entries[j + incr].filename) > 0)
				{
					t = std::move(entries[j]);
					entries[j] = std::move(entries[j + incr]);
					entries[j + incr] = std::move(t);
					j -= incr;
				}
				else
					break;
			}
		}
		incr /= 2;
	}
}

void delete_player_saved_games(const char* name)
{
	int i;
	char filename_full_path[CHOCOLATE_MAX_FILE_PATH_SIZE];
	char filename[16];

	for (i = 0; i < 10; i++)
	{
		snprintf(filename, 16, "%s.sg%d", name, i);
		get_game_full_file_path(filename_full_path, filename, CHOCOLATE_SAVE_DIR);
		_unlink(filename_full_path);
		//sprintf(filename, "%s.sg%d", name, i);
		//_unlink(filename);
	}
}

//This isn't particularly elegant, but there will only be one nm_filelist active with pending deletes
static bool filelist_do_delete = false;

bool nm_delete_callback(int num)
{
	if (num == 0)
		filelist_do_delete = true;

	return false; //Never keep the window open
}

class nm_filelist : public nm_window
{
	std::vector<newmenu_fileentry> files;
	int NumFiles;
	int NumFiles_displayed = 8;
	int first_item = -1;
	bool player_mode = false;
	bool demo_mode = false;
	int demos_deleted = 0;
	int initialized = 0;
	int exit_value = 0;
	int w_x, w_y, w_w, w_h;
	int citem;
	bkg bg;
	bool allow_abort_flag;

	std::string title;
	char localized_filespec[CHOCOLATE_MAX_FILE_PATH_SIZE];

	bool pending_delete;
	int pending_delete_num;

	void (*file_selected)(std::string& filename, int num);

	void create_list()
	{
		files.clear();
		NumFiles = 0;

		if (player_mode)
		{
			files.emplace_back(TXT_CREATE_NEW, false);
			NumFiles++;
		}

		FILEFINDSTRUCT find;
		if (!FileFindFirstLFNTemp(localized_filespec, &find))
			//if (!FileFindFirstLFNTemp(filespec, &find)) 
		{
			do
			{
				if (!player_mode || strlen(find.name) <= 13) //12345678.0123
				{
					files.emplace_back(find.name, player_mode);
					NumFiles++;
				}
			} while (!FileFindNext(&find));
			FileFindClose();
		}

		if ((NumFiles < 1) && demos_deleted)
		{
			exit_value = 0;
			//Just close up if this happened
			close();
			//goto ExitFileMenu;
		}
		if ((NumFiles < 1) && demo_mode)
		{
			Int3();
			nm_messagebox(NULL, 1, TXT_OK, "%s %s\n%s", TXT_NO_DEMO_FILES, TXT_USE_F5, TXT_TO_CREATE_ONE);
			exit_value = 0;
			//goto ExitFileMenu;
		}

		if ((NumFiles < 2) && player_mode)
		{
			citem = 0;
			file_selected(files[0].filename, citem);
			//goto ExitFileMenuEarly;
		}

		if (NumFiles < 1)
		{
			nm_messagebox(NULL, 1, "Ok", "%s\n '%s' %s", TXT_NO_FILES_MATCHING, localized_filespec, TXT_WERE_FOUND);
		}

		if (!player_mode)
		{
			newmenu_file_sort(files);
		}
		else
		{
			newmenu_file_sort(std::span<newmenu_fileentry>(files.begin() + 1, files.end()));

			for (int i = 0; i < files.size(); i++)
			{
				if (!_strfcmp(Players[Player_num].callsign, files[i].displayname.c_str()))
					citem = i;
			}
		}
	}

public:
	nm_filelist(const char* newtitle, const char* filespec, void (*select_callback)(std::string& filename, int num), bool allow_abort)
	{
		title = std::string(newtitle);
		file_selected = select_callback;
		citem = 0;
		allow_abort_flag = allow_abort;

		//Get localized filepath
		const char* wildcard_pos;
		wildcard_pos = strrchr(filespec, '*');
		if (wildcard_pos != NULL)
		{
			if (!_strfcmp(wildcard_pos, "*.nplt"))
				player_mode = true;
			else if (!_strfcmp(wildcard_pos, "*.dem"))
				demo_mode = true;
		}
		strncpy(localized_filespec, filespec, CHOCOLATE_MAX_FILE_PATH_SIZE);
		localized_filespec[CHOCOLATE_MAX_FILE_PATH_SIZE - 1] = '\0';

		create_list();

		gr_set_current_canvas(nm_canvas);

		w_w = 230 - 90 + 1 + 30;
		w_h = 170 - 30 + 1 + 30;

		if (w_w > 320) w_w = 320;
		if (w_h > 200) w_h = 200;

		w_x = (grd_curcanv->cv_bitmap.bm_w - w_w) / 2;
		w_y = (grd_curcanv->cv_bitmap.bm_h - w_h) / 2;

		if (w_x < 0) w_x = 0;
		if (w_y < 0) w_y = 0;

		// Save the background under the menu...
		//TODO: Since the file list doesn't use a sub canvas, this creates a large region than expected. 
		bg.saved = gr_create_bitmap(nm_canvas->cv_bitmap.bm_w, nm_canvas->cv_bitmap.bm_h);
		Assert(bg.saved != NULL);
		gr_set_current_canvas(nm_canvas);
		gr_bm_bitblt(nm_canvas->cv_bitmap.bm_w, nm_canvas->cv_bitmap.bm_h, 0, 0, 0, 0, &grd_curcanv->cv_bitmap, bg.saved);

		nm_draw_background(w_x, w_y, w_x + w_w - 1, w_y + w_h - 1);

		grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_3];
		gr_string(0x8000, w_y + 10, title.c_str());

		pending_delete = false;
		pending_delete_num = 0;
	}

	void frame() override
	{
		bool done = false;
		int ocitem = citem;
		int ofirst_item = first_item;
		int key = key_inkey();
		bool redraw = false;

		gr_set_current_canvas(nm_canvas);

		if (pending_delete)
		{
			if (filelist_do_delete)
			{
				char file_full_path[CHOCOLATE_MAX_FILE_PATH_SIZE];

				if (player_mode)
					get_full_file_path(file_full_path, files[citem].filename.c_str(), CHOCOLATE_PILOT_DIR);
				else if (demo_mode)
					get_game_full_file_path(file_full_path, files[citem].filename.c_str(), CHOCOLATE_DEMOS_DIR);
				int ret = _unlink(file_full_path);
				//ret = _unlink(files[citem].filename.c_str());

				if ((!ret) && player_mode)
				{
					delete_player_saved_games(files[citem].filename.c_str());
				}

				if (ret)
				{
					if (player_mode)
						nm_open_messagebox(nullptr, nullptr, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_PILOT, files[citem].get_display_name());
					else if (demo_mode)
						nm_open_messagebox(nullptr, nullptr, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_DEMO, files[citem].get_display_name());
				}
				else if (demo_mode)
					demos_deleted = 1;
				first_item = -1;
				//Regenerate the list
				create_list();
				filelist_do_delete = false;
				redraw = true;
			}
			pending_delete = false;
		}

		switch (key)
		{
		case KEY_PRINT_SCREEN: 		save_screen_shot(0); break;
		case KEY_CTRLED + KEY_D:
			if (((player_mode) && (citem > 0)) || ((demo_mode) && (citem >= 0)))
			{
				if (player_mode)
				{
					nm_open_messagebox(NULL, nm_delete_callback, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_PILOT, files[citem].get_display_name());
					pending_delete = true;
					pending_delete_num = citem;
				}
				else if (demo_mode)
				{
					nm_open_messagebox(NULL, nm_delete_callback, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_DEMO, files[citem].get_display_name());
					pending_delete = true;
					pending_delete_num = citem;
				}
			}
			break;
		case KEY_HOME:
		case KEY_PAD7:
			citem = 0;
			break;
		case KEY_END:
		case KEY_PAD1:
			citem = NumFiles - 1;
			break;
		case KEY_UP:
		case KEY_PAD8:
			citem--;
			break;
		case KEY_DOWN:
		case KEY_PAD2:
			citem++;
			break;
		case KEY_PAGEDOWN:
		case KEY_PAD3:
			citem += NumFiles_displayed;
			break;
		case KEY_PAGEUP:
		case KEY_PAD9:
			citem -= NumFiles_displayed;
			break;
		case KEY_ESC:
			if (allow_abort_flag)
			{
				citem = -1;
				done = true;
			}
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
			done = true;
			break;
		default:
		{
			int ascii = key_to_ascii(key);
			if (ascii < 255)
			{
				int cc, cc1;
				cc = cc1 = citem + 1;
				if (cc1 < 0)  cc1 = 0;
				if (cc1 >= NumFiles)  cc1 = 0;
				while (1) {
					if (cc < 0) cc = 0;
					if (cc >= NumFiles) cc = 0;
					if (citem == cc) break;

					if (toupper(files[cc].displayname[0]) == toupper(ascii))
					{
						citem = cc;
						break;
					}
					cc++;
				}
			}
		}
		}

		if (done)
		{
			file_selected(files[citem].filename, citem);
			close();
		}
		else
		{
			if (citem < 0)
				citem = 0;

			if (citem >= NumFiles)
				citem = NumFiles - 1;

			if (citem < first_item)
				first_item = citem;

			if (citem >= (first_item + NumFiles_displayed))
				first_item = citem - NumFiles_displayed + 1;

			if (NumFiles <= NumFiles_displayed)
				first_item = 0;

			if (first_item > NumFiles - NumFiles_displayed)
				first_item = NumFiles - NumFiles_displayed;
			if (first_item < 0) first_item = 0;

			if (ofirst_item != first_item || redraw)
			{
				gr_setcolor(BM_XRGB(0, 0, 0));
				for (int i = first_item; i < first_item + NumFiles_displayed; i++)
				{
					int w, h, aw, y;
					y = (i - first_item) * 12 + w_y + 45;
					if (i >= NumFiles)
					{
						gr_setcolor(BM_XRGB(0, 0, 0));
						gr_rect(100, y - 1, 220, y + 11);
					}
					else
					{
						if (i == citem)
							grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_2];
						else
							grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_1];
						gr_get_string_size(files[i].displayname.c_str(), &w, &h, &aw);
						gr_rect(100, y - 1, 220, y + 11);
						//gr_string(105, y, (&filenames[i * 14]) + ((player_mode && filenames[i * 14] == '$') ? 1 : 0));
						gr_string(105, y, files[i].get_display_name());
					}
				}
			}
			else if (citem != ocitem)
			{
				int w, h, aw, y;

				int i = ocitem;
				if ((i >= 0) && (i < NumFiles))
				{
					y = (i - first_item) * 12 + w_y + 45;
					if (i == citem)
						grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_2];
					else
						grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_1];
					gr_get_string_size(files[i].displayname.c_str(), &w, &h, &aw);
					gr_rect(100, y - 1, 220, y + 11);
					//gr_string(105, y, (&filenames[i * 14]) + ((player_mode && filenames[i * 14] == '$') ? 1 : 0));
					gr_string(105, y, files[i].get_display_name());
				}
				i = citem;
				if ((i >= 0) && (i < NumFiles))
				{
					y = (i - first_item) * 12 + w_y + 45;
					if (i == citem)
						grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_2];
					else
						grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_1];
					gr_get_string_size(files[i].displayname.c_str(), &w, &h, &aw);
					gr_rect(100, y - 1, 220, y + 11);
					gr_string(105, y, files[i].get_display_name());
				}
			}
}
	}

	void cleanup() override
	{
		// Restore everything...
		gr_bitmap(0, 0, bg.saved);
		gr_free_bitmap(bg.saved);
	}
};

void newmenu_open_filepicker(const char* title, const char* filespec, int allow_abort_flag, void (*callback)(std::string& str, int num))
{
	nm_open_windows.push(std::make_unique<nm_filelist>(title, filespec, callback, allow_abort_flag));
}

void newmenu_frame()
{
	bool did_frame = false;
	while (!nm_open_windows.empty() && !did_frame)
	{
		std::unique_ptr<nm_window>& top = nm_open_windows.top();
		if (top->is_closing()) //Has the window closed?
		{
			nm_open_windows.pop(); //kill it, and then loop to check the next window.
		}
		else
		{
			top->frame(); //Run the window frame. 
			did_frame = true;
		}
	}
}

void newmenu_present()
{
	plat_present_canvas_masked(*nm_canvas, 3.f / 4.f);
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

int MakeNewPlayerFile(int allow_abort);

int newmenu_get_filename(const char* title, const char* filespec, char* filename, int allow_abort_flag)
{
	Int3();
	return 0;
}


// Example listbox callback function...
// int lb_callback( int * citem, int *nitems, char * items[], int *keypress )
// {
// 	int i;
// 
// 	if ( *keypress = KEY_CTRLED+KEY_D )	{
// 		if ( *nitems > 1 )	{
// 			unlink( items[*citem] );		// Delete the file
// 			for (i=*citem; i<*nitems-1; i++ )	{
// 				items[i] = items[i+1];
// 			}
// 			*nitems = *nitems - 1;
// 			free( items[*nitems] );
// 			items[*nitems] = NULL;
// 			return 1;	// redraw;
// 		}
//			*keypress = 0;
// 	}			
// 	return 0;
// }

#define LB_ITEMS_ON_SCREEN 8

int newmenu_listbox(const char* title, int nitems, char* items[], int allow_abort_flag, int (*listbox_callback)(int* citem, int* nitems, char* items[], int* keypress))
{
	return newmenu_listbox1(title, nitems, items, allow_abort_flag, 0, listbox_callback);
}

int newmenu_listbox1(const char* title, int nitems, char* items[], int allow_abort_flag, int default_item, int (*listbox_callback)(int* citem, int* nitems, char* items[], int* keypress))
{
	int redraw;
	int old_keyd_repeat = keyd_repeat;
	int title_height;
	keyd_repeat = 1;

	grs_canvas* menu_canvas = gr_create_canvas(320, 200);
	menu_canvas_stack.push(menu_canvas);

	set_screen_mode(SCREEN_MENU);
	gr_set_current_canvas(menu_canvas);
	grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_3];

	int width = 0;
	for (int i = 0; i < nitems; i++) 
	{
		int w, h, aw;
		gr_get_string_size(items[i], &w, &h, &aw);
		if (w > width)
			width = w;
	}
	int height = 12 * LB_ITEMS_ON_SCREEN;

	{
		int w, h, aw;
		gr_get_string_size(title, &w, &h, &aw);
		if (w > width)
			width = w;
		title_height = h + 5;
	}

	width += 10;
	if (width > 320 - 30)
		width = 320 - 30;

	int wx = (grd_curcanv->cv_bitmap.bm_w - width) / 2;
	int wy = (grd_curcanv->cv_bitmap.bm_h - (height + title_height)) / 2 + title_height;
	if (wy < title_height)
		wy = title_height;

	gr_bm_bitblt(320, 200, 0, 0, 0, 0, &(grd_curcanv->cv_bitmap), &(VR_offscreen_buffer->cv_bitmap));
	nm_draw_background(wx - 15, wy - title_height - 15, wx + width + 15, wy + height + 15);

	gr_string(0x8000, wy - title_height, title);

	bool done = false;
	int citem = default_item;
	if (citem < 0) citem = 0;
	if (citem >= nitems) citem = 0;

	int first_item = -1;

	while (!done) 
	{
		timer_mark_start();
		plat_do_events();
		int ocitem = citem;
		int ofirst_item = first_item;
		int key = key_inkey();

		if (listbox_callback)
			redraw = (*listbox_callback)(&citem, &nitems, items, &key);
		else
			redraw = 0;

		if (key < -1) 
		{
			citem = key;
			key = -1;
			done = true;
		}

		switch (key) 
		{
		case KEY_PRINT_SCREEN:
			save_screen_shot(0);
			break;
		case KEY_HOME:
		case KEY_PAD7:
			citem = 0;
			break;
		case KEY_END:
		case KEY_PAD1:
			citem = nitems - 1;
			break;
		case KEY_UP:
		case KEY_PAD8:
			citem--;
			break;
		case KEY_DOWN:
		case KEY_PAD2:
			citem++;
			break;
		case KEY_PAGEDOWN:
		case KEY_PAD3:
			citem += LB_ITEMS_ON_SCREEN;
			break;
		case KEY_PAGEUP:
		case KEY_PAD9:
			citem -= LB_ITEMS_ON_SCREEN;
			break;
		case KEY_ESC:
			if (allow_abort_flag) 
			{
				citem = -1;
				done = true;
			}
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
			done = true;
			break;
		default:
			if (key > 0) 
			{
				int ascii = key_to_ascii(key);
				if (ascii < 255) 
				{
					int cc, cc1;
					cc = cc1 = citem + 1;
					if (cc1 < 0)  cc1 = 0;
					if (cc1 >= nitems)  cc1 = 0;
					while (1) 
					{
						if (cc < 0) cc = 0;
						if (cc >= nitems) cc = 0;
						if (citem == cc) break;

						if (toupper(items[cc][0]) == toupper(ascii)) 
						{
							citem = cc;
							break;
						}
						cc++;
					}
				}
			}
		}
		if (done) break;

		if (citem < 0)
			citem = 0;

		if (citem >= nitems)
			citem = nitems - 1;

		if (citem < first_item)
			first_item = citem;

		if (citem >= (first_item + LB_ITEMS_ON_SCREEN))
			first_item = citem - LB_ITEMS_ON_SCREEN + 1;

		if (nitems <= LB_ITEMS_ON_SCREEN)
			first_item = 0;

		if (first_item > nitems - LB_ITEMS_ON_SCREEN)
			first_item = nitems - LB_ITEMS_ON_SCREEN;
		if (first_item < 0) first_item = 0;

		if ((ofirst_item != first_item) || redraw) 
		{
			gr_setcolor(BM_XRGB(0, 0, 0));
			for (int i = first_item; i < first_item + LB_ITEMS_ON_SCREEN; i++) 
			{
				int w, h, aw, y;
				y = (i - first_item) * 12 + wy;
				if (i >= nitems) {
					gr_setcolor(BM_XRGB(0, 0, 0));
					gr_rect(wx, y - 1, wx + width - 1, y + 11);
				}
				else 
				{
					if (i == citem)
						grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_2];
					else
						grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_1];
					gr_get_string_size(items[i], &w, &h, &aw);
					gr_rect(wx, y - 1, wx + width - 1, y + 11);
					gr_string(wx + 5, y, items[i]);
				}
			}
		}
		else if (citem != ocitem) 
		{
			int w, h, aw, y;

			int i = ocitem;
			if ((i >= 0) && (i < nitems))
			{
				y = (i - first_item) * 12 + wy;
				if (i == citem)
					grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_2];
				else
					grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_1];
				gr_get_string_size(items[i], &w, &h, &aw);
				gr_rect(wx, y - 1, wx + width - 1, y + 11);
				gr_string(wx + 5, y, items[i]);
			}
			i = citem;
			if ((i >= 0) && (i < nitems))
			{
				y = (i - first_item) * 12 + wy;
				if (i == citem)
					grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_2];
				else
					grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_1];
				gr_get_string_size(items[i], &w, &h, &aw);
				gr_rect(wx, y - 1, wx + width - 1, y + 11);
				gr_string(wx + 5, y, items[i]);
			}
		}
		plat_present_canvas(*menu_canvas, 3.f/4.f);
		timer_mark_end(US_70FPS);
	}
	keyd_repeat = old_keyd_repeat;

	gr_bm_bitblt(320, 200, 0, 0, 0, 0, &(VR_offscreen_buffer->cv_bitmap), &(grd_curcanv->cv_bitmap));

	menu_canvas_stack.pop();
	gr_free_canvas(menu_canvas);

	return citem;
}

int newmenu_filelist(const char* title, const char* filespec, char* filename)
{
	char* Filenames[MAX_FILES];
	char FilenameText[MAX_FILES][14];
	FILEFINDSTRUCT find;

	int NumFiles = 0;
	if (!FileFindFirst(filespec, &find)) 
	{
		do 
		{
			if (NumFiles < MAX_FILES) 
			{
				strncpy(FilenameText[NumFiles], find.name, FILENAME_LEN);
				Filenames[NumFiles] = FilenameText[NumFiles];
				NumFiles++;
			}
			else 
			{
				break;
			}
		} while (!FileFindNext(&find));
		FileFindClose();
	}

	int i = newmenu_listbox(title, NumFiles, Filenames, 1, NULL);
	if (i > -1) 
	{
		strcpy(filename, Filenames[i]);
		return 1;
	}
	return 0;
}

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

#include "inferno.h"
#include "misc/error.h"
#include "game.h"
#include "gamefont.h"
#include "newmenu.h"
#include "newmenu_windows.h"
#include "platform/key.h"
#include "stringtable.h"
#include "2d/palette.h"

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

constexpr int MAX_TEXT_WIDTH = 200;			// How many pixels wide a input box can be

#include "platform/timer.h"

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

nm_menu::nm_menu(std::vector<newmenu_item>& source_items, const char* new_title, const char* new_subtitle,
	void (*subfunction)(int nitems, newmenu_item* items, int* last_key, int citem), bool (*choicefunc)(int choice, int nitems, newmenu_item* item),
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

	tw = th = 0;

	//Find the size of the title and subtitle.
	if (new_title)
	{
		grd_curcanv->cv_font = TITLE_FONT;
		gr_get_string_size(title.c_str(), &string_width, &string_height, &average_width);
		tw = string_width;
		th = string_height;
	}
	if (new_subtitle)
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

	if (filename)
	{
		bg_filename = std::string(filename);
	}

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

void nm_menu::draw()
{
	nm_window::draw();

	if (!bg_filename.empty())
		nm_draw_background1(bg_filename.c_str());

	// Save the background of the display
	bg.menu_canvas = gr_create_sub_canvas(nm_canvas, x, y, w, h);
	gr_set_current_canvas(bg.menu_canvas);

	if (bg_filename.empty())
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
}

void nm_menu::frame()
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
			if (choice < 0)
				choice = items.size() - 1;
			if (choice >= items.size()) 
				choice = 0;
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
		bool keep_open = choice_callback(choice, items.size(), items.data());
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

void nm_menu::cleanup()
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
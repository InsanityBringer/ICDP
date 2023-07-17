#include <span>

#include "inferno.h"
#include "misc/error.h"
#include "game.h"
#include "gamefont.h"
#include "newmenu.h"
#include "newmenu_windows.h"
#include "platform/key.h"
#include "stringtable.h"
#include "2d/palette.h"
#include "player.h"
#include "platform/findfile.h"
#include "platform/posixstub.h"
#include "platform/event.h"

constexpr int LB_ITEMS_ON_SCREEN = 8;

nm_list::nm_list(const char* title, std::vector<char*>& newitems, bool allow_abort, int default_item, void (*select_callback)(int choice))
{
	int redraw;
	int old_keyd_repeat = keyd_repeat;
	keyd_repeat = 1;

	selected = select_callback;
	//listbox_callback = list_callback;

	if (title)
		title_str = std::string(title);

	allow_abort_flag = allow_abort;

	for (char*& str : newitems)
	{
		items.push_back(str);
	}

	gr_set_current_canvas(nm_canvas);
	grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_3];

	width = 0;
	for (int i = 0; i < items.size(); i++)
	{
		int w, h, aw;
		gr_get_string_size(items[i], &w, &h, &aw);
		if (w > width)
			width = w;
	}
	height = 12 * LB_ITEMS_ON_SCREEN;

	int w, h, aw;
	gr_get_string_size(title, &w, &h, &aw);
	if (w > width)
		width = w;
	title_height = h + 5;

	width += 10;
	if (width > 320 - 30)
		width = 320 - 30;

	wx = (grd_curcanv->cv_bitmap.bm_w - width) / 2;
	wy = (grd_curcanv->cv_bitmap.bm_h - (height + title_height)) / 2 + title_height;
	if (wy < title_height)
		wy = title_height;

	bool done = false;
	citem = default_item;
	if (citem < 0) citem = 0;
	if (citem >= items.size()) citem = 0;

	first_item = -1;
	saved = nullptr;
}

void nm_list::draw()
{
	nm_window::draw();
	gr_set_current_canvas(nm_canvas);
	grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_3];

	saved = gr_create_bitmap(grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h);
	gr_bm_bitblt(320, 200, 0, 0, 0, 0, &(grd_curcanv->cv_bitmap), saved);
	nm_draw_background(wx - 15, wy - title_height - 15, wx + width + 15, wy + height + 15);

	gr_string(0x8000, wy - title_height, title_str.c_str());
}

void nm_list::frame()
{
	bool done = false;
	bool redraw = false;

	int ocitem = citem;
	int ofirst_item = first_item;
	int key = 0;

	//if (listbox_callback) //never used
	//	redraw = (*listbox_callback)(&citem, items.size(), items, &key);

	/*if (key < -1)
	{
		citem = key;
		key = -1;
		done = true;
	}*/

	if (event_available())
	{
		plat_event ev;
		pop_event(ev);

		if (ev.source == EventSource::Keyboard && ev.down)
		{
			key = ev.inputnum;
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
				citem = items.size() - 1;
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
						if (cc1 >= items.size())  cc1 = 0;
						while (1)
						{
							if (cc < 0) cc = 0;
							if (cc >= items.size()) cc = 0;
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
		}
	}
	if (done)
	{
		selected(citem);
		close();
		return;
	}
	
	if (citem < 0)
		citem = 0;

	if (citem >= items.size())
		citem = items.size() - 1;

	if (citem < first_item)
		first_item = citem;

	if (citem >= (first_item + LB_ITEMS_ON_SCREEN))
		first_item = citem - LB_ITEMS_ON_SCREEN + 1;

	if (items.size() <= LB_ITEMS_ON_SCREEN)
		first_item = 0;

	if (first_item > items.size() - LB_ITEMS_ON_SCREEN)
		first_item = items.size() - LB_ITEMS_ON_SCREEN;
	if (first_item < 0) first_item = 0;

	if ((ofirst_item != first_item) || redraw)
	{
		gr_setcolor(BM_XRGB(0, 0, 0));
		for (int i = first_item; i < first_item + LB_ITEMS_ON_SCREEN; i++)
		{
			int w, h, aw, y;
			y = (i - first_item) * 12 + wy;
			if (i >= items.size()) 
			{
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
		if ((i >= 0) && (i < items.size()))
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
		if ((i >= 0) && (i < items.size()))
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
}

void nm_list::cleanup()
{
	gr_bitmap(0, 0, saved);
	gr_free_bitmap(saved);
}

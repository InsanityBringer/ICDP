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

bool nm_delete_callback(int num, int nitems, newmenu_item* item)
{
	if (num == 0)
		filelist_do_delete = true;

	return false; //Never keep the window open
}

void nm_filelist::create_list()
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

nm_filelist::nm_filelist(const char* newtitle, const char* filespec, void (*select_callback)(std::string& filename, int num), bool allow_abort)
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

	pending_delete = false;
	pending_delete_num = 0;
}

void nm_filelist::draw()
{
	nm_window::draw();
	// Save the background under the menu...
	//TODO: Since the file list doesn't use a sub canvas, this creates a large region than expected. 
	bg.saved = gr_create_bitmap(nm_canvas->cv_bitmap.bm_w, nm_canvas->cv_bitmap.bm_h);
	Assert(bg.saved != NULL);
	gr_set_current_canvas(nm_canvas);
	gr_bm_bitblt(nm_canvas->cv_bitmap.bm_w, nm_canvas->cv_bitmap.bm_h, 0, 0, 0, 0, &grd_curcanv->cv_bitmap, bg.saved);

	nm_draw_background(w_x, w_y, w_x + w_w - 1, w_y + w_h - 1);

	grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_3];
	gr_string(0x8000, w_y + 10, title.c_str());
}

void nm_filelist::frame()
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
		if (citem > -1)
			file_selected(files[citem].displayname, citem);

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

void nm_filelist::cleanup()
{
	// Restore everything...
	gr_bitmap(0, 0, bg.saved);
	gr_free_bitmap(bg.saved);
}

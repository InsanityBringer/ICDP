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
#pragma once

#include "newmenu.h"
#include "platform/platform_filesys.h"

//Common functionality for newmenu windows

// Draw a left justfied string
void nm_string(bkg* b, int w1, int x, int y, char* s);

// Draw a slider and it's string
void nm_string_slider(bkg* b, int w1, int x, int y, char* s);

// Draw a left justfied string with black background.
void nm_string_black(bkg* b, int w1, int x, int y, char* s);

// Draw a right justfied string
void nm_rstring(bkg* b, int w1, int x, int y, const char* s);

//for text items, constantly redraw cursor (to achieve flash)
void update_cursor(newmenu_item* item);

void nm_string_inputbox(bkg* b, int w, int x, int y, char* text, int current);

//returns true if char is allowed
int char_allowed(char c);

void strip_end_whitespace(char* text);

class nm_menu : public nm_window
{
	std::string title, subtitle, bg_filename;
	std::vector<newmenu_item> items;
	int choice, old_choice;
	int tw, th;
	int w, h, x, y;
	bool tiny, all_text;
	bkg bg;
	int string_width, string_height, average_width;

	//Callback executed when a choice is selected.
	//Returns true if the menu should remain open, or false if it should close. 
	bool (*choice_callback)(int choice, int nitems, newmenu_item* item);
	//Callback executed each frame, to update and redraw contents. 
	void (*newmenu_callback)(int nitems, newmenu_item* items, int* last_key, int citem);
public:
	nm_menu(std::vector<newmenu_item>& source_items, const char* new_title, const char* new_subtitle,
		void (*subfunction)(int nitems, newmenu_item* items, int* last_key, int citem), bool (*choicefunc)(int choice, int nitems, newmenu_item* item),
		int citem, const char* filename, int width, int height, bool tiny_mode);

	void draw() override;
	void frame() override;
	void cleanup() override;
};

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

	void (*file_selected)(std::string& filename, int num);;

	void create_list();
public:
	nm_filelist(const char* newtitle, const char* filespec, void (*select_callback)(std::string& filename, int num), bool allow_abort);

	void draw() override;
	void frame() override;
	void cleanup() override;
};

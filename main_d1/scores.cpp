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
#include <ctype.h>
#include <stdarg.h>

#include "platform/posixstub.h"
#include "cfile/cfile.h"
#include "platform/platform_filesys.h"
#include "platform/platform.h"
#include "misc/error.h"
#include "misc/types.h"
#include "2d/gr.h"
#include "platform/mono.h"
#include "platform/key.h"
#include "2d/palette.h"
#include "game.h"
#include "gamefont.h"
#include "mem/mem.h"
#include "newmenu.h"
#include "menu.h"
#include "player.h"
#include "screens.h"
#include "platform/mouse.h"
#include "platform/joy.h"
#include "platform/timer.h"
#include "stringtable.h"
#include "scores.h"
#include "platform/event.h"

#define VERSION_NUMBER 		1
#define SCORES_FILENAME 	"descent.hi"
//#define SCORES_FILENAME 	"DESCENT.HI"
constexpr int COOL_MESSAGE_LEN = 50;
constexpr int MAX_HIGH_SCORES = 10;

constexpr int STATS_INFO_SIZEOF = 15 + (CALLSIGN_LEN + 1);
struct stats_info
{
	char	name[CALLSIGN_LEN + 1];
	int		score;
	int8_t	starting_level;
	int8_t	ending_level;
	int8_t  diff_level;
	short 	kill_ratio;		// 0-100
	short	hostage_ratio;	// 
	int		seconds;		// How long it took in seconds...

	void from_file(FILE* fp)
	{
		//I need a way to auto generate serializers..
		fread(name, 1, sizeof(name), fp);
		score = file_read_int(fp);
		starting_level = file_read_byte(fp);
		ending_level = file_read_byte(fp);
		diff_level = file_read_byte(fp);
		kill_ratio = file_read_short(fp);
		hostage_ratio = file_read_short(fp);
		seconds = file_read_int(fp);
	}

	void to_file(FILE* fp)
	{
		fwrite(name, 1, sizeof(name), fp);
		file_write_int(fp, score);
		file_write_byte(fp, starting_level);
		file_write_byte(fp, ending_level);
		file_write_byte(fp, diff_level);
		file_write_short(fp, kill_ratio);
		file_write_short(fp, hostage_ratio);
		file_write_int(fp, seconds);
	}
};

constexpr int ALL_SCORES_SIZEOF = 54 + (STATS_INFO_SIZEOF * MAX_HIGH_SCORES);
struct all_scores
{
	char		signature[3];	// DHS
	int8_t		version;
	char		cool_saying[COOL_MESSAGE_LEN];
	stats_info	stats[MAX_HIGH_SCORES];

	void from_file(FILE* fp)
	{
		fread(signature, 1, sizeof(signature), fp);
		version = file_read_byte(fp);
		fread(cool_saying, 1, sizeof(cool_saying), fp);
		for (int i = 0; i < MAX_HIGH_SCORES; i++)
			stats[i].from_file(fp);
	}

	void to_file(FILE* fp)
	{
		fwrite(signature, 1, sizeof(signature), fp);
		file_write_byte(fp, version);
		fwrite(cool_saying, 1, sizeof(cool_saying), fp);
		for (int i = 0; i < MAX_HIGH_SCORES; i++)
			stats[i].to_file(fp);
	}
};

static all_scores Scores;

stats_info Last_game;

char scores_filename[CHOCOLATE_MAX_FILE_PATH_SIZE];

#define XX  (7)
#define YY  (-3)

char* get_scores_filename()
{
#ifndef RELEASE
	// Only use the MINER variable for internal developement
	char* p = getenv("MINER");
	if (p) 
	{
		char miner_path[CHOCOLATE_MAX_FILE_PATH_SIZE];
		if (strlen(CHOCOLATE_HISCORE_DIR) > 0)
		{
			snprintf(miner_path, CHOCOLATE_MAX_FILE_PATH_SIZE, "%s%c%s%cgame", CHOCOLATE_HISCORE_DIR, PLATFORM_PATH_SEPARATOR, p, PLATFORM_PATH_SEPARATOR);
		}
		else
		{
			snprintf(miner_path, CHOCOLATE_MAX_FILE_PATH_SIZE, "%s%cgame", p, PLATFORM_PATH_SEPARATOR);
		}	
		get_full_file_path(scores_filename, SCORES_FILENAME, miner_path);
		//sprintf(scores_filename, "%s\\game\\%s", p, SCORES_FILENAME);
		Assert(strlen(scores_filename) < 256);
		return scores_filename;
	}
#endif
	get_full_file_path(scores_filename, SCORES_FILENAME, CHOCOLATE_HISCORE_DIR);
	//sprintf(scores_filename, "%s", SCORES_FILENAME);
	return scores_filename;
}

void scores_create_default()
{
	// No error message needed, code will work without a scores file
	sprintf(Scores.cool_saying, TXT_REGISTER_DESCENT);
	sprintf(Scores.stats[0].name, "Parallax");
	sprintf(Scores.stats[1].name, "Mike");
	sprintf(Scores.stats[2].name, "Matt");
	sprintf(Scores.stats[3].name, "John");
	sprintf(Scores.stats[4].name, "Yuan");
	sprintf(Scores.stats[5].name, "Adam");
	sprintf(Scores.stats[6].name, "Mark");
	sprintf(Scores.stats[7].name, "Allender");
	sprintf(Scores.stats[8].name, "Jasen");
	sprintf(Scores.stats[9].name, "Rob");

	for (int i = 0; i < 10; i++)
		Scores.stats[i].score = (10 - i) * 1000;
}

void scores_read()
{
	// clear score array...
	scores_create_default();

	FILE* fp = fopen(get_scores_filename(), "rb");
	if (!fp) 
		return;

	int fsize = _filelength(_fileno(fp));

	if (fsize != ALL_SCORES_SIZEOF) 
	{
		fclose(fp);
		return;
	}
	// Read 'em in...
	Scores.from_file(fp);
	fclose(fp);

	if ((Scores.version != VERSION_NUMBER) || (Scores.signature[0] != 'D') || (Scores.signature[1] != 'H') || (Scores.signature[2] != 'S')) 
		scores_create_default();
}

void scores_write()
{
	FILE* fp = fopen(get_scores_filename(), "wb");
	if (fp == NULL) 
	{
		nm_messagebox(TXT_WARNING, 1, TXT_OK, "%s\n'%s'", TXT_UNABLE_TO_OPEN, get_scores_filename());
		return;
	}

	Scores.signature[0] = 'D';
	Scores.signature[1] = 'H';
	Scores.signature[2] = 'S';
	Scores.version = VERSION_NUMBER;
	Scores.to_file(fp);
	fclose(fp);
}

void int_to_string(int number, char* dest)
{
	int i, l, c;
	char buffer[20], * p;

	sprintf(buffer, "%d", number);

	l = strlen(buffer);
	if (l <= 3) 
	{
		// Don't bother with less than 3 digits
		sprintf(dest, "%d", number);
		return;
	}

	c = 0;
	p = dest;
	for (i = l - 1; i >= 0; i--)
	{
		if (c == 3) 
		{
			*p++ = ',';
			c = 0;
		}
		c++;
		*p++ = buffer[i];
	}
	*p++ = '\0';
	_strrev(dest);
}

void scores_fill_struct(stats_info* stats)
{
	strcpy(stats->name, Players[Player_num].callsign);
	stats->score = Players[Player_num].score;
	stats->ending_level = Players[Player_num].level;
	if (Players[Player_num].num_robots_total > 0)
		stats->kill_ratio = (Players[Player_num].num_kills_total * 100) / Players[Player_num].num_robots_total;
	else
		stats->kill_ratio = 0;

	if (Players[Player_num].hostages_total > 0)
		stats->hostage_ratio = (Players[Player_num].hostages_rescued_total * 100) / Players[Player_num].hostages_total;
	else
		stats->hostage_ratio = 0;

	stats->seconds = f2i(Players[Player_num].time_total) + (Players[Player_num].hours_total * 3600);

	stats->diff_level = Difficulty_level;
	stats->starting_level = Players[Player_num].starting_level;
}

//char * score_placement[10] = { TXT_1ST, TXT_2ND, TXT_3RD, TXT_4TH, TXT_5TH, TXT_6TH, TXT_7TH, TXT_8TH, TXT_9TH, TXT_10TH };

static int add_position;
bool add_first_place_callback(int choice, int nitems, newmenu_item* item)
{
	strncpy(Scores.cool_saying, item[1].text, COOL_MESSAGE_LEN);
	if (strlen(Scores.cool_saying) < 1)
		sprintf(Scores.cool_saying, "No Comment");

	// move everyone down...
	for (int i = MAX_HIGH_SCORES - 1; i > add_position; i--)
	{
		Scores.stats[i] = Scores.stats[i - 1];
	}
	scores_fill_struct(&Scores.stats[add_position]);
	scores_write();

	scores_view(add_position);
	return false;
}

bool add_high_score_callback(int choice, int nitems, newmenu_item* item)
{
	// move everyone down...
	for (int i = MAX_HIGH_SCORES - 1; i > add_position; i--)
	{
		Scores.stats[i] = Scores.stats[i - 1];
	}
	scores_fill_struct(&Scores.stats[add_position]);
	scores_write();

	scores_view(add_position);
	return false;
}

void scores_maybe_add_player(int abort_flag)
{
	static char text1[COOL_MESSAGE_LEN + 10];
	newmenu_item m[10];

	scores_read();

	int position = MAX_HIGH_SCORES;
	for (int i = 0; i < MAX_HIGH_SCORES; i++) 
	{
		if (Players[Player_num].score > Scores.stats[i].score) 
		{
			position = i;
			break;
		}
	}

	if (position == MAX_HIGH_SCORES) 
	{
		if (abort_flag)
			return;
		scores_fill_struct(&Last_game);
	}
	else
	{
		add_position = position;
		if (position == 0)
		{
			strcpy(text1, "");
			m[0].type = NM_TYPE_TEXT; m[0].text = TXT_COOL_SAYING;
			m[1].type = NM_TYPE_INPUT; m[1].text = text1; m[1].text_len = COOL_MESSAGE_LEN - 5;
			newmenu_open(TXT_HIGH_SCORE, TXT_YOU_PLACED_1ST, 2, m, nullptr, add_first_place_callback);
		}
		else
		{
			nm_open_messagebox(TXT_HIGH_SCORE, add_high_score_callback, 1, TXT_OK, "%s %s!", TXT_YOU_PLACED, *(&TXT_1ST + position));
		}
	}
}

#define TEXT_FONT  		(Gamefonts[GFONT_MEDIUM_3])

void scores_rprintf(int x, int y, const char* format, ...)
{
	va_list args;
	char buffer[128];

	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);

	//replace the digit '1' with special wider 1
	for (char* p = buffer; *p; p++)
		if (*p == '1') *p = 132;

	int w, h, aw;
	gr_get_string_size(buffer, &w, &h, &aw);

	gr_string(x - w, y, buffer);
}


void scores_draw_item(int  i, stats_info* stats)
{
	char buffer[20];
	int y = 7 + 70 + i * 9;
	if (i == 0) y -= 8;

	if (i == MAX_HIGH_SCORES) 
	{
		y += 8;
		//scores_rprintf( 17+33+XX, y+YY, "" );
	}
	else 
	{
		scores_rprintf(17 + 33 + XX, y + YY, "%d.", i + 1);
	}

	if (strlen(stats->name) == 0) 
	{
		gr_printf(26 + 33 + XX, y + YY, TXT_EMPTY);
		return;
	}
	gr_printf(26 + 33 + XX, y + YY, "%s", stats->name);
	int_to_string(stats->score, buffer);
	scores_rprintf(109 + 33 + XX, y + YY, "%s", buffer);

	gr_printf(125 + 33 + XX, y + YY, "%s", MENU_DIFFICULTY_TEXT(stats->diff_level));

	if ((stats->starting_level > 0) && (stats->ending_level > 0))
		scores_rprintf(192 + 33 + XX, y + YY, "%d-%d", stats->starting_level, stats->ending_level);
	else if ((stats->starting_level < 0) && (stats->ending_level > 0))
		scores_rprintf(192 + 33 + XX, y + YY, "S%d-%d", -stats->starting_level, stats->ending_level);
	else if ((stats->starting_level < 0) && (stats->ending_level < 0))
		scores_rprintf(192 + 33 + XX, y + YY, "S%d-S%d", -stats->starting_level, -stats->ending_level);
	else if ((stats->starting_level > 0) && (stats->ending_level < 0))
		scores_rprintf(192 + 33 + XX, y + YY, "%d-S%d", stats->starting_level, -stats->ending_level);

	int h = stats->seconds / 3600;
	int s = stats->seconds % 3600;
	int m = s / 60;
	s = s % 60;
	scores_rprintf(311 - 42 + XX, y + YY, "%d:%02d:%02d", h, m, s);
}

bool reset_scores_callback(int choice, int nitems, newmenu_item* item)
{
	if (choice == 1)
	{
		remove(get_scores_filename());
		scores_read();
	}

	return false;
}

class nm_scorewindow : public nm_window
{
	int citem;
	int looper;
	bool background_drawn;
	grs_bitmap* saved;
public:
	nm_scorewindow(int highlight)
	{
		citem = highlight;
		looper = 0;
		background_drawn = false;
		saved = nullptr;

		scores_read();
	}

	void draw() override
	{
		const int8_t fades[64] = { 1,1,1,2,2,3,4,4,5,6,8,9,10,12,13,15,16,17,19,20,22,23,24,26,27,28,28,29,30,30,31,31,31,31,31,30,30,29,28,28,27,26,24,23,22,20,19,17,16,15,13,12,10,9,8,6,5,4,4,3,2,2,1,1 };

		nm_window::draw();

		if (!background_drawn)
		{
			gr_set_current_canvas(nm_canvas);
			saved = gr_create_bitmap(grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h);
			gr_bm_bitblt(nm_canvas->cv_bitmap.bm_w, nm_canvas->cv_bitmap.bm_h, 0, 0, 0, 0, &(grd_curcanv->cv_bitmap), saved);
			background_drawn = true;
		}

		nm_draw_background(0, 0, grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h);

		grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_3];

		gr_string(0x8000, 15, TXT_HIGH_SCORES);

		grd_curcanv->cv_font = Gamefonts[GFONT_SMALL];

		gr_set_fontcolor(BM_XRGB(31, 26, 5), -1);
		gr_string(31 + 33 + XX, 46 + 7 + YY, TXT_NAME);
		gr_string(82 + 33 + XX, 46 + 7 + YY, TXT_SCORE);
		gr_string(127 + 33 + XX, 46 + 7 + YY, TXT_SKILL);
		gr_string(170 + 33 + XX, 46 + 7 + YY, TXT_LEVELS);
		//	gr_string( 202, 46, "Kills" );
		//	gr_string( 234, 46, "Rescues" );
		gr_string(288 - 42 + XX, 46 + 7 + YY, TXT_TIME);

		if (citem < 0)
			gr_string(0x8000, 175, TXT_PRESS_CTRL_R);

		gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);

		gr_printf(0x8000, 31, "%c%s%c  - %s", 34, Scores.cool_saying, 34, Scores.stats[0].name);

		int num_to_draw = citem == MAX_HIGH_SCORES ? MAX_HIGH_SCORES + 1 : MAX_HIGH_SCORES;
		for (int i = 0; i < num_to_draw; i++)
		{
			if (i == citem || i == MAX_HIGH_SCORES) //Check MAX_HIGH_SCORES just in case
			{
				fix t1 = timer_get_fixed_seconds();
				while (timer_get_fixed_seconds() < t1 + F1_0 / 128);

				gr_set_fontcolor(gr_fade_table[fades[looper] * 256 + BM_XRGB(28, 28, 28)], -1);
				if (++looper > 63) 
					looper = 0;

				if (i == MAX_HIGH_SCORES)
					scores_draw_item(MAX_HIGH_SCORES, &Last_game);
				else
					scores_draw_item(citem, &Scores.stats[citem]);
			}
			else
			{
				if (i == 0)
				{
					gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);
				}
				else
				{
					gr_set_fontcolor(gr_fade_table[BM_XRGB(28, 28, 28) + ((28 - i * 2) * 256)], -1);
				}
				scores_draw_item(i, &Scores.stats[i]);
			}
		}
	}

	void frame() override
	{
		bool done = false;

		while (event_available())
		{
			plat_event ev;
			pop_event(ev);
			if (ev.source == EventSource::Keyboard && ev.down)
			{
				int k = event_to_keycode(ev);

				switch (k)
				{
				case KEY_CTRLED + KEY_R:
					if (citem < 0)
					{
						// Reset scores...
						nm_open_messagebox(NULL, reset_scores_callback, 2, TXT_NO, TXT_YES, TXT_RESET_HIGH_SCORES);
					}
					break;
				case KEY_BACKSP:				Int3(); k = 0; break;
				case KEY_PRINT_SCREEN:		save_screen_shot(0); k = 0; break;

				case KEY_ENTER:
				case KEY_SPACEBAR:
				case KEY_ESC:
					done = true;
					break;
				}
			}
		}

		if (done)
		{
			close();
		}
	}

	void cleanup() override
	{
		if (saved)
		{
			gr_bitmap(0, 0, saved);
			gr_free_bitmap(saved);
		}
	}
};

void scores_view(int citem)
{
	newmenu_open_window(std::make_unique<nm_scorewindow>(citem));
	/*
	const int8_t fades[64] = { 1,1,1,2,2,3,4,4,5,6,8,9,10,12,13,15,16,17,19,20,22,23,24,26,27,28,28,29,30,30,31,31,31,31,31,30,30,29,28,28,27,26,24,23,22,20,19,17,16,15,13,12,10,9,8,6,5,4,4,3,2,2,1,1 };

	grs_canvas* scores_canvas = gr_create_canvas(320, 200);

ReshowScores:
	scores_read();

	set_screen_mode(SCREEN_MENU);

	gr_set_current_canvas(scores_canvas);

	nm_draw_background(0, 0, grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h);

	grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_3];

	gr_string(0x8000, 15, TXT_HIGH_SCORES);

	grd_curcanv->cv_font = Gamefonts[GFONT_SMALL];

	gr_set_fontcolor(BM_XRGB(31, 26, 5), -1);
	gr_string(31 + 33 + XX, 46 + 7 + YY, TXT_NAME);
	gr_string(82 + 33 + XX, 46 + 7 + YY, TXT_SCORE);
	gr_string(127 + 33 + XX, 46 + 7 + YY, TXT_SKILL);
	gr_string(170 + 33 + XX, 46 + 7 + YY, TXT_LEVELS);
	//	gr_string( 202, 46, "Kills" );
	//	gr_string( 234, 46, "Rescues" );
	gr_string(288 - 42 + XX, 46 + 7 + YY, TXT_TIME);

	if (citem < 0)
		gr_string(0x8000, 175, TXT_PRESS_CTRL_R);

	gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);

	gr_printf(0x8000, 31, "%c%s%c  - %s", 34, Scores.cool_saying, 34, Scores.stats[0].name);

	for (int i = 0; i < MAX_HIGH_SCORES; i++) 
	{
		if (i == 0) 
		{
			gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);
		}
		else 
		{
			gr_set_fontcolor(gr_fade_table[BM_XRGB(28, 28, 28) + ((28 - i * 2) * 256)], -1);
		}
		scores_draw_item(i, &Scores.stats[i]);
	}

	gr_palette_fade_in(gr_palette, 32, 0);

	game_flush_inputs();

	bool done = false;
	int looper = 0;

	fix time_out_value = timer_get_fixed_seconds() + i2f(60 * 5);
	while (!done) 
	{
		timer_mark_start();
		plat_do_events();
		if (citem > -1) 
		{
			fix t1 = timer_get_fixed_seconds();
			if (t1 > time_out_value) done = true;
			while (timer_get_fixed_seconds() < t1 + F1_0 / 128);

			gr_set_fontcolor(gr_fade_table[fades[looper] * 256 + BM_XRGB(28, 28, 28)], -1);
			looper++;
			if (looper > 63) looper = 0;
			if (citem == MAX_HIGH_SCORES)
				scores_draw_item(MAX_HIGH_SCORES, &Last_game);
			else
				scores_draw_item(citem, &Scores.stats[citem]);
		}

		for (int i = 0; i < 4; i++)
			if (joy_get_button_down_cnt(i) > 0) done = 1;
		for (int i = 0; i < 3; i++)
			if (mouse_button_down_count(i) > 0) done = 1;

		int k = key_inkey();
		switch (k) 
		{
		case KEY_CTRLED + KEY_R:
			if (citem < 0) 
			{
				// Reset scores...
				if (nm_messagebox(NULL, 2, TXT_NO, TXT_YES, TXT_RESET_HIGH_SCORES) == 1) 
				{
					remove(get_scores_filename());
					gr_palette_fade_out(gr_palette, 32, 0);
					goto ReshowScores;
				}
			}
			break;
		case KEY_BACKSP:				Int3(); k = 0; break;
		case KEY_PRINT_SCREEN:		save_screen_shot(0); k = 0; break;

		case KEY_ENTER:
		case KEY_SPACEBAR:
		case KEY_ESC:
			done = true;
			break;
		}
		plat_present_canvas(0);
		timer_mark_end(US_70FPS);
	}

	// Restore background and exit
	gr_palette_fade_out(gr_palette, 32, 0);
	gr_set_current_canvas(VR_screen_buffer);

	game_flush_inputs();

	gr_free_canvas(scores_canvas);*/
}


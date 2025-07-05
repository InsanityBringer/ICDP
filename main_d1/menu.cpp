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

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>

#include "platform/posixstub.h"
#include "menu.h"
#include "inferno.h"
#include "game.h"
#include "2d/gr.h"
#include "platform/key.h"
#include "iff/iff.h"
#include "mem/mem.h"
#include "misc/error.h"
#include "bm.h"
#include "screens.h"
#include "platform/mono.h"
#include "cfile/cfile.h"
#include "platform/joy.h"
#include "vecmat/vecmat.h"
#include "main_shared/effects.h"
#include "slew.h"
#include "gamemine.h"
#include "gamesave.h"
#include "2d/palette.h"
#include "misc/args.h"
#include "newdemo.h"
#include "platform/timer.h"
#include "sounds.h"
#include "gameseq.h"
#include "stringtable.h"
#include "gamefont.h"
#include "newmenu.h"
#include "network.h"
#include "scores.h"
#include "joydefs.h"
#include "playsave.h"
#include "multi.h"
#include "kconfig.h"
#include "titles.h"
#include "credits.h"
#include "3d/3d.h"
#include "polyobj.h"
#include "state.h"
#include "mission.h"
#include "main_shared/songs.h"
#include "config.h"

#include "platform/i_net.h"

#ifdef EDITOR
#include "editor\editor.h"
extern void init_cockpit(); //[ISB] I really should stuff these somewhere formal
#endif

//hack
#define EZERO 0

//char *menu_difficulty_text[] = { "Trainee", "Rookie", "Fighter", "Hotshot", "Insane" };
//char *menu_detail_text[] = { "Lowest", "Low", "Medium", "High", "Highest", "", "Custom..." };

#define MENU_NEW_GAME            0
#define MENU_GAME      				1 
#define MENU_EDITOR					2
#define MENU_VIEW_SCORES			3
#define MENU_QUIT                4
#define MENU_LOAD_GAME				5
#define MENU_SAVE_GAME				6
#define MENU_DEMO_PLAY				8
#define MENU_LOAD_LEVEL				9
#define MENU_START_NETGAME			10
#define MENU_JOIN_NETGAME			11
#define MENU_CONFIG					13
#define MENU_REJOIN_NETGAME		14
#define MENU_DIFFICULTY				15
#define MENU_START_SERIAL			18
#define MENU_HELP						19
#define MENU_NEW_PLAYER				20
#define MENU_MULTIPLAYER			21
#define MENU_STOP_MODEM				22
#define MENU_SHOW_CREDITS			23
#define MENU_ORDER_INFO				24
#define MENU_PLAY_SONG				25

//ADD_ITEM("Start netgame...", MENU_START_NETGAME, -1 );
//ADD_ITEM("Send net message...", MENU_SEND_NET_MESSAGE, -1 );

#define ADD_ITEM(t,value,key)  do { m[num_options].type=NM_TYPE_MENU; m[num_options].text=t; menu_choice[num_options]=value;num_options++; } while (0)

extern int last_joy_time;		//last time the joystick was used
#ifndef NDEBUG
extern int speedtest_on;
#else
#define speedtest_on 0
#endif

uint8_t do_auto_demo = 1;			// Flag used to enable auto demo starting in main menu.
int Player_default_difficulty; // Last difficulty level chosen by the player
int Auto_leveling_on = 0;
int Menu_draw_copyright = 0;

void autodemo_menu_check(int nitems, newmenu_item* items, int* last_key, int citem)
{
	int curtime;

	nitems = nitems;
	items = items;
	citem = citem;

	//draw copyright message
	if (Menu_draw_copyright) 
	{
		Menu_draw_copyright = 0;
		gr_set_current_canvas(nm_get_top_canvas());
		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(BM_XRGB(6, 6, 6), -1);
		gr_printf(0x8000, grd_curcanv->cv_bitmap.bm_h - GAME_FONT->ft_h - 2, TXT_COPYRIGHT);
	}

	// Don't allow them to hit ESC in the main menu.
	if (*last_key == KEY_ESC)* last_key = 0;

	if (do_auto_demo) 
	{
		curtime = timer_get_approx_seconds();
		//if ( ((keyd_time_when_last_pressed+i2f(20)) < curtime) && ((last_joy_time+i2f(20)) < curtime) && (!speedtest_on)  ) {
		if (((keyd_time_when_last_pressed + i2f(45)) < curtime) && (!speedtest_on)) {
			keyd_time_when_last_pressed = curtime;			// Reset timer so that disk won't thrash if no demos.
			newdemo_start_playback(NULL);		// Randomly pick a file
			if (Newdemo_state == ND_STATE_PLAYBACK) 
			{
				Function_mode = FMODE_GAME;
				*last_key = -2;
			}
		}
	}
}

//static int First_time = 1;
static int main_menu_choice = 0;
static int menu_choice[25];

//	-----------------------------------------------------------------------------
//	Create the main menu.
void create_main_menu(newmenu_item* m, int* menu_choice, int* callers_num_options)
{
	int	num_options;

#ifndef DEMO_ONLY
	num_options = 0;

	ADD_ITEM(TXT_NEW_GAME, MENU_NEW_GAME, KEY_N);
	ADD_ITEM(TXT_LOAD_GAME, MENU_LOAD_GAME, KEY_L);
	ADD_ITEM(TXT_MULTIPLAYER_, MENU_MULTIPLAYER, -1);
	ADD_ITEM(TXT_OPTIONS_, MENU_CONFIG, -1);
	ADD_ITEM(TXT_CHANGE_PILOTS, MENU_NEW_PLAYER, unused);
	ADD_ITEM(TXT_VIEW_DEMO, MENU_DEMO_PLAY, 0);
	ADD_ITEM(TXT_VIEW_SCORES, MENU_VIEW_SCORES, KEY_V);
#ifdef SHAREWARE
	ADD_ITEM(TXT_ORDERING_INFO, MENU_ORDER_INFO, -1);
#endif
	ADD_ITEM(TXT_CREDITS, MENU_SHOW_CREDITS, -1);
#endif
	ADD_ITEM(TXT_QUIT, MENU_QUIT, KEY_Q);

#ifndef RELEASE
	if (!(Game_mode & GM_MULTI)) 
	{
		//m[num_options].type=NM_TYPE_TEXT;
		//m[num_options++].text=" Debug options:";

		ADD_ITEM((char*)"  Load level...", MENU_LOAD_LEVEL, KEY_N);
#ifdef EDITOR
		ADD_ITEM((char*)"  Editor", MENU_EDITOR, KEY_E);
#endif
	}

	ADD_ITEM((char*)"  Play song", MENU_PLAY_SONG, -1);
#endif

	* callers_num_options = num_options;
}

bool MainMenuCallback(int choice, int nitems, newmenu_item* item)
{
	main_menu_choice = choice;
	do_option(menu_choice[choice]);
	return true; //Always keep the main menu up if possible
}

//returns number of item chosen
int DoMenu()
{
	newmenu_item m[25];
	int num_options = 0;

	if (Players[Player_num].callsign[0] == 0) 
	{
		RegisterPlayer();
		return 0;
	}

	create_main_menu(m, menu_choice, &num_options);

	keyd_time_when_last_pressed = timer_get_fixed_seconds();		// .. 20 seconds from now!
	if (main_menu_choice < 0)	main_menu_choice = 0;
	Menu_draw_copyright = 1;

	newmenu_open("", NULL, num_options, m, autodemo_menu_check, MainMenuCallback, main_menu_choice, Menu_pcx_name);

	return 0;
}

extern void show_order_form(void);	// John didn't want this in inferno.h so I just externed it.
void do_ip_address_menu();

static void select_demo(std::string& str, int num)
{
	newdemo_start_playback(str.c_str());
}

static bool do_load_level(int choice, int nitems, newmenu_item* items)
{
	if (choice != -1)
	{
		int new_level_num = atoi(items[0].text);

		if (new_level_num != 0 && new_level_num >= Last_secret_level && new_level_num <= Last_level)
		{
			inferno_request_fade_out();
			StartNewGame(new_level_num);
		}
	}
	return false;
}

//returns flag, true means quit menu
void do_option(int select)
{
	switch (select) 
	{
	case MENU_NEW_GAME:
		do_new_game_menu();
		break;
	case MENU_GAME:
		break;
	case MENU_DEMO_PLAY:
	{
		char demo_file[16];
		char localized_demo_query[CHOCOLATE_MAX_FILE_PATH_SIZE];
		//get_platform_localized_query_string(localized_demo_query, CHOCOLATE_DEMOS_DIR, "*.dem");
		get_game_full_file_path(localized_demo_query, "*.dem", CHOCOLATE_DEMOS_DIR);
		newmenu_open_filepicker(TXT_SELECT_DEMO, localized_demo_query, true, &select_demo);
		/*if (newmenu_get_filename(TXT_SELECT_DEMO, localized_demo_query, demo_file, 1))
		{
			newdemo_start_playback(demo_file);
		}*/
	}
	break;
	case MENU_LOAD_GAME:
		state_restore_all(0);
		break;
#ifdef EDITOR
	case MENU_EDITOR:
		Function_mode = FMODE_EDITOR;
		init_cockpit();
		break;
#endif
	case MENU_VIEW_SCORES:
		scores_view(-1);
		break;
#ifdef SHAREWARE
	case MENU_ORDER_INFO:
		show_order_form();
		break;
#endif
	case MENU_QUIT:
#ifdef EDITOR
		if (!SafetyCheck()) break;
#endif
		gr_palette_fade_out(gr_palette, 32, 0);
		Function_mode = FMODE_EXIT;
		break;
	case MENU_NEW_PLAYER:
		RegisterPlayer();		//1 == allow escape out of menu
		break;

	case MENU_HELP:
		do_show_help();
		break;

#ifndef RELEASE

	case MENU_PLAY_SONG: 
	{
		int i;
		char* m[MAX_NUM_SONGS];

		for (i = 0; i < Num_songs; i++) 
		{
			m[i] = Songs[i].filename;
		}
		i = newmenu_listbox("Select Song", Num_songs, m, 1, NULL);

		if (i > -1) {
			songs_play_song(i, 0);
		}
	}
						 break;
	case MENU_LOAD_LEVEL: 
	{
		newmenu_item m;
		static char text[11] = ""; //TODO: newmenu string ownership
		memset(text, 0, sizeof(text));

		m.type = NM_TYPE_INPUT; m.text_len = 10; m.text = text;

		//newmenu_do(NULL, "Enter level to load", 1, &m, NULL);
		newmenu_open(nullptr, "Enter level to load", 1, &m, nullptr, &do_load_level);

		break;
	}
#endif


	case MENU_START_NETGAME:
#ifdef NETWORK
		//temp!
		load_mission(0);
		network_start_game();
#endif
		break;
	case MENU_JOIN_NETGAME:
		//temp!
#ifdef NETWORK
		load_mission(0);
		network_join_game();
#endif
		break;
	case MENU_START_SERIAL:
#ifdef NETWORK
		load_mission(0);
		do_ip_address_menu();
#endif
		break;
	case MENU_MULTIPLAYER:
		do_multi_player_menu();
		break;
	case MENU_CONFIG:
		do_options_menu();
		break;
	case MENU_SHOW_CREDITS:
		gr_palette_fade_out(gr_palette, 32, 0);
		credits_show();
		break;
	default:
		Error("Unknown option %d in do_option", select);
		break;
	}

}


int	Max_debris_objects, Max_objects_onscreen_detailed;
int	Max_linear_depth_objects;

int8_t	Object_complexity = 2, Object_detail = 2;
int8_t	Wall_detail = 2, Wall_render_depth = 2, Debris_amount = 2, SoundChannels = 2;

int8_t	Render_depths[NUM_DETAIL_LEVELS - 1] = { 6,  9, 12, 15, 20 };
int8_t	Max_perspective_depths[NUM_DETAIL_LEVELS - 1] = { 1,  2,  3,  5,  8 };
int8_t	Max_linear_depths[NUM_DETAIL_LEVELS - 1] = { 3,  5,  7, 10, 17 };
int8_t	Max_linear_depths_objects[NUM_DETAIL_LEVELS - 1] = { 1,  2,  3,  5, 12 };
int8_t	Max_debris_objects_list[NUM_DETAIL_LEVELS - 1] = { 2,  4,  7, 10, 15 };
int8_t	Max_objects_onscreen_detailed_list[NUM_DETAIL_LEVELS - 1] = { 2,  4,  7, 10, 15 };
int8_t	Smts_list[NUM_DETAIL_LEVELS - 1] = { 2,  4,  8, 16, 50 };	//	threshold for models to go to lower detail model, gets multiplied by obj->size
int8_t	Max_sound_channels[NUM_DETAIL_LEVELS - 1] = { 2,  4,  8, 12, 16 };

//	-----------------------------------------------------------------------------
//	Set detail level based stuff.
//	Note: Highest detail level (detail_level == NUM_DETAIL_LEVELS-1) is custom detail level.
void set_detail_level_parameters(int detail_level)
{
	Assert((detail_level >= 0) && (detail_level < NUM_DETAIL_LEVELS));

	if (detail_level < NUM_DETAIL_LEVELS - 1) 
	{
		Render_depth = Render_depths[detail_level];
		Max_perspective_depth = Max_perspective_depths[detail_level];
		Max_linear_depth = Max_linear_depths[detail_level];
		Max_linear_depth_objects = Max_linear_depths_objects[detail_level];

		Max_debris_objects = Max_debris_objects_list[detail_level];
		Max_objects_onscreen_detailed = Max_objects_onscreen_detailed_list[detail_level];

		Simple_model_threshhold_scale = Smts_list[detail_level];

		digi_set_max_channels(Max_sound_channels[detail_level]);

		//	Set custom menu defaults.
		Object_complexity = detail_level;
		Wall_render_depth = detail_level;
		Object_detail = detail_level;
		Wall_detail = detail_level;
		Debris_amount = detail_level;
		SoundChannels = detail_level;
	}
}

bool detail_level_menu_callback(int choice, int nitems, newmenu_item* item)
{
	if (choice >= 0 && choice < 5)
	{
		Detail_level = choice;
		mprintf((0, "Detail level set to %i\n", Detail_level));
		set_detail_level_parameters(Detail_level);
	}
	else if (choice == 6)
	{
		Detail_level = 5;
		do_detail_level_menu_custom();
	}
	return false;
}

//	-----------------------------------------------------------------------------
void do_detail_level_menu(void)
{
	newmenu_item m[7];

	m[0].type = NM_TYPE_MENU; m[0].text = MENU_DETAIL_TEXT(0);
	m[1].type = NM_TYPE_MENU; m[1].text = MENU_DETAIL_TEXT(1);
	m[2].type = NM_TYPE_MENU; m[2].text = MENU_DETAIL_TEXT(2);
	m[3].type = NM_TYPE_MENU; m[3].text = MENU_DETAIL_TEXT(3);
	m[4].type = NM_TYPE_MENU; m[4].text = MENU_DETAIL_TEXT(4);
	m[5].type = NM_TYPE_TEXT; m[5].text = (char*)"";
	m[6].type = NM_TYPE_MENU; m[6].text = MENU_DETAIL_TEXT(5);

	newmenu_open(nullptr, TXT_DETAIL_LEVEL, NDL + 2, m, nullptr, detail_level_menu_callback, Detail_level);
}

//	-----------------------------------------------------------------------------
void do_detail_level_menu_custom_menuset(int nitems, newmenu_item* items, int* last_key, int citem)
{
	nitems = nitems;
	*last_key = *last_key;
	citem = citem;

	Object_complexity = items[0].value;
	Object_detail = items[1].value;
	Wall_detail = items[2].value;
	Wall_render_depth = items[3].value;
	Debris_amount = items[4].value;
	SoundChannels = items[5].value;

}

void set_custom_detail_vars(void)
{
	Render_depth = Render_depths[Wall_render_depth];

	Max_perspective_depth = Max_perspective_depths[Wall_detail];
	Max_linear_depth = Max_linear_depths[Wall_detail];

	Max_debris_objects = Max_debris_objects_list[Debris_amount];

	Max_objects_onscreen_detailed = Max_objects_onscreen_detailed_list[Object_complexity];
	Simple_model_threshhold_scale = Smts_list[Object_complexity];
	Max_linear_depth_objects = Max_linear_depths_objects[Object_detail];

	digi_set_max_channels(Max_sound_channels[SoundChannels]);
}

bool custom_detail_level_menu_callback(int choice, int nitems, newmenu_item* item)
{
	if (choice == -1)
	{
		set_custom_detail_vars();
		return false;
	}

	return true;
}

//	-----------------------------------------------------------------------------
void do_detail_level_menu_custom(void)
{
	newmenu_item m[7];

	m[0].type = NM_TYPE_SLIDER;
	m[0].text = TXT_OBJ_COMPLEXITY;
	m[0].value = Object_complexity;
	m[0].min_value = 0;
	m[0].max_value = NDL - 1;

	m[1].type = NM_TYPE_SLIDER;
	m[1].text = TXT_OBJ_DETAIL;
	m[1].value = Object_detail;
	m[1].min_value = 0;
	m[1].max_value = NDL - 1;

	m[2].type = NM_TYPE_SLIDER;
	m[2].text = TXT_WALL_DETAIL;
	m[2].value = Wall_detail;
	m[2].min_value = 0;
	m[2].max_value = NDL - 1;

	m[3].type = NM_TYPE_SLIDER;
	m[3].text = TXT_WALL_RENDER_DEPTH;
	m[3].value = Wall_render_depth;
	m[3].min_value = 0;
	m[3].max_value = NDL - 1;

	m[4].type = NM_TYPE_SLIDER;
	m[4].text = TXT_DEBRIS_AMOUNT;
	m[4].value = Debris_amount;
	m[4].min_value = 0;
	m[4].max_value = NDL - 1;

	m[5].type = NM_TYPE_SLIDER;
	m[5].text = TXT_SOUND_CHANNELS;
	m[5].value = SoundChannels;
	m[5].min_value = 0;
	m[5].max_value = NDL - 1;

	m[6].type = NM_TYPE_TEXT;
	m[6].text = TXT_LO_HI;

	newmenu_open(nullptr, TXT_DETAIL_CUSTOM, 7, m, do_detail_level_menu_custom_menuset, custom_detail_level_menu_callback);
}

static int new_level_num;
static int player_highest_level;

bool difficulty_callback(int choice, int nitems, newmenu_item* items)
{
	if (choice > -1)
	{
		if (choice != Difficulty_level)
		{
			Player_default_difficulty = choice;
			write_player_file();
		}
		Difficulty_level = choice;
		mprintf((0, "%s %s %i\n", TXT_DIFFICULTY_LEVEL, TXT_SET_TO, Difficulty_level));

		inferno_request_fade_out();
		StartNewGame(new_level_num);
	}
	return false; //this never stays up
}

void do_difficulty_menu()
{
	int s;
	newmenu_item m[5];

	m[0].type = NM_TYPE_MENU; m[0].text = MENU_DIFFICULTY_TEXT(0);
	m[1].type = NM_TYPE_MENU; m[1].text = MENU_DIFFICULTY_TEXT(1);
	m[2].type = NM_TYPE_MENU; m[2].text = MENU_DIFFICULTY_TEXT(2);
	m[3].type = NM_TYPE_MENU; m[3].text = MENU_DIFFICULTY_TEXT(3);
	m[4].type = NM_TYPE_MENU; m[4].text = MENU_DIFFICULTY_TEXT(4);

	newmenu_open(nullptr, TXT_DIFFICULTY_LEVEL, NDL, m, nullptr, difficulty_callback, Difficulty_level);
}

static void do_difficulty_selection()
{
	Difficulty_level = Player_default_difficulty;

	do_difficulty_menu();
}

bool initial_level_callback(int choice, int nitems, newmenu_item* items)
{
	if (choice == -1)
		return false;
	if (items[1].text[0] == 0)
		return true;

	new_level_num = atoi(items[1].text);

	if (!(new_level_num > 0 && new_level_num <= player_highest_level))
	{
		items[0].text = TXT_ENTER_TO_CONT;
		nm_open_messagebox(nullptr, nullptr, 1, TXT_OK, TXT_INVALID_LEVEL);
		return true;
	}

	do_difficulty_selection();
	return false; //Got a good level, so close. 
}

static void do_initial_level()
{
	player_highest_level = get_highest_level();

	if (player_highest_level > Last_level)
		player_highest_level = Last_level;

	if (player_highest_level > 1)
	{
		newmenu_item m[2];
		static char info_text[80];
		static char num_text[11];

		sprintf(info_text, "%s %d", TXT_START_ANY_LEVEL, player_highest_level);

		m[0].type = NM_TYPE_TEXT; m[0].text = info_text;
		m[1].type = NM_TYPE_INPUT; m[1].text_len = 10; m[1].text = num_text;

		strcpy(num_text, "1");

		newmenu_open(nullptr, TXT_SELECT_START_LEV, 2, &m[0], nullptr, initial_level_callback);
	}
	else
	{
		new_level_num = 1;
		do_difficulty_selection();
	}
}

void mission_list_callback(int choice)
{
	if (choice == -1) return; //backed out

	int new_mission_num = choice;

	strcpy(config_last_mission, Mission_list[new_mission_num].mission_name);

	if (!load_mission(new_mission_num))
	{
		nm_open_messagebox(nullptr, nullptr, 1, TXT_OK, "Error in Mission file"); 
		return;
	}

	do_initial_level();
}

static void do_new_game_menu()
{
	int n_missions = build_mission_list(false);

	if (n_missions > 1)
	{
		char** m = (char**)malloc(sizeof(*m) * Mission_list.size());
		if (!m)
			Error("do_new_game_menu: failed to allocate string list");

		int default_mission = 0;
		for (int i = 0; i < n_missions; i++) 
		{
			m[i] = Mission_list[i].mission_name;
			if (!_strfcmp(m[i], config_last_mission))
				default_mission = i;
		}

		newmenu_open_listbox("New Game\n\nSelect mission", n_missions, m, true, mission_list_callback);

		free(m); //Safe to free now since the menu code copied it
	}
	else
	{
		//Go straight to highest level if needed
		do_initial_level();
	}
}

void sound_menuset(int nitems, newmenu_item* items, int* last_key, int citem)
{
	nitems = nitems;
	*last_key = *last_key;

	if (Config_digi_volume != items[0].value)
	{
		Config_digi_volume = items[0].value;
		digi_set_digi_volume((Config_digi_volume * 32768) / 8);
		digi_play_sample_once(SOUND_DROP_BOMB, F1_0);
	}

	if (Config_midi_volume != items[1].value)
	{
		Config_midi_volume = items[1].value;
		digi_set_midi_volume((Config_midi_volume * 128) / 8);
	}

	Config_channels_reversed = items[2].value;
}

#include "platform/platform.h"
#include "platform/s_midi.h"

char default_str[] = "Default MIDI device";
//I really need to make newmenu items own their strings.
static std::vector<std::string> mme_device_names;
static std::vector<char*> mme_device_names_hack;

void mme_device_callback(int choice)
{
	if (choice != -1)
	{
		PreferredMMEDevice = choice;
		//restart the sound system
		digi_reset();
		digi_reset();

		if (Function_mode == FMODE_MENU)
			songs_play_song(SONG_TITLE, 1);
		else
			songs_play_level_song(Current_level_num);
	}
}

bool midi_menu_callback(int choice, int nitems, newmenu_item* items)
{
	GenDevices new_preferred_device;
	if (items[2].value)
		new_preferred_device = GenDevices::FluidSynthDevice;
	else if (items[5].value)
		new_preferred_device = GenDevices::MMEDevice;
	else
		new_preferred_device = GenDevices::NullDevice;

	if (choice == -1)
	{
		if (new_preferred_device != PreferredGenDevice)
		{
			PreferredGenDevice = new_preferred_device;
			//restart the sound system
			digi_reset();
			digi_reset();

			if (Function_mode == FMODE_MENU)
				songs_play_song(SONG_TITLE, 1);
			else
				songs_play_level_song(Current_level_num);
		}
		return false;
	}
	else if (choice == 6)
	{
		mme_device_names = music_get_MME_devices();
		mme_device_names_hack.clear();
		for (std::string& name : mme_device_names)
		{
			mme_device_names_hack.push_back((char*)name.c_str()); //Does listbox really need non-const strings?
		}
		newmenu_open_listbox("Available MME devices", mme_device_names_hack, true, mme_device_callback, PreferredMMEDevice);
	}
	return true;
}

void do_chocolate_midi_menu()
{
	newmenu_item m[13];

	m[0].type = NM_TYPE_TEXT; m[0].text = (char*)"Preferred general MIDI device";
	m[1].type = NM_TYPE_RADIO; m[1].text = (char*)"None"; m[1].group = 0; m[1].value = PreferredGenDevice == GenDevices::NullDevice;
	m[2].type = NM_TYPE_RADIO; m[2].text = (char*)"FluidSynth (if available)"; m[2].group = 0; m[2].value = PreferredGenDevice == GenDevices::FluidSynthDevice;
	m[3].type = NM_TYPE_TEXT; m[3].text = (char*)"Soundfont path";
	m[4].type = NM_TYPE_INPUT; m[4].text = SoundFontFilename; m[4].text_len = _MAX_PATH - 1;
	m[5].type = NM_TYPE_RADIO; m[5].text = (char*)"MS/Native MIDI (if available)"; m[5].group = 0; m[5].value = PreferredGenDevice == GenDevices::MMEDevice;
	m[6].type = NM_TYPE_MENU; m[6].text = (char*)"Select MME device";

	newmenu_open(NULL, "MIDI Options", 7, m, nullptr, midi_menu_callback);
}

bool sound_menu_callback(int choice, int nitems, newmenu_item* item)
{
	switch (choice)
	{
	case -1:
		if (Config_midi_volume < 1)
			digi_play_midi_song(NULL, NULL, NULL, 0);
		return false;
	case 4:
		do_chocolate_midi_menu();
		break;
	}

	return true;
}

const char* sound_menu_title = "SOUND OPTIONS";

void do_sound_menu()
{
	newmenu_item m[5];
	m[0].type = NM_TYPE_SLIDER; m[0].text = TXT_FX_VOLUME; m[0].value = Config_digi_volume; m[0].min_value = 0; m[0].max_value = 8;
	m[1].type = NM_TYPE_SLIDER; m[1].text = TXT_MUSIC_VOLUME; m[1].value = Config_midi_volume; m[1].min_value = 0; m[1].max_value = 8;
	m[2].type = NM_TYPE_CHECK; m[2].text = TXT_REVERSE_STEREO; m[2].value = Config_channels_reversed;
	m[3].type = NM_TYPE_TEXT; m[3].text = (char*)"";
	m[4].type = NM_TYPE_MENU; m[4].text = (char*)"MIDI Options";

	newmenu_open(NULL, sound_menu_title, 5, m, sound_menuset, sound_menu_callback);
}

//this change was made in DESCENT.TEX, but since we're not including that
//file in the v1.1 update, we're making the change in the code here also
#ifdef SHAREWARE
#undef	TXT_JOYS_SENSITIVITY
#define	TXT_JOYS_SENSITIVITY "Joystick/Mouse\nSensitivity"
#endif

void do_video_menu();

static const char* Autoselect_mode_names[] =
{
	"Always",
	"Never",
	"When not firing"
};

static const char* Message_level_names[] =
{
	"All",
	"No redundant messages",
	"Only player chat"
};

void gameplay_options_menuset(int nitems, newmenu_item* items, int* last_key, int citem)
{
	//I need some way to generalize this. 
	if (citem == 1)
	{
		if (*last_key == KEY_LEFT)
		{
			Primary_autoselect_mode--; if (Primary_autoselect_mode < 0) Primary_autoselect_mode = AS_NUM_MODES - 1;
		}
		else if (*last_key == KEY_RIGHT)
			Primary_autoselect_mode = (Primary_autoselect_mode + 1) % AS_NUM_MODES;

		items[citem + 1].text = (char*)Autoselect_mode_names[Primary_autoselect_mode];
		items[citem + 1].redraw = true;
	}
	else if (citem == 3)
	{
		if (*last_key == KEY_LEFT)
		{
			Secondary_autoselect_mode--; if (Secondary_autoselect_mode < 0) Secondary_autoselect_mode = AS_NUM_MODES - 1;
		}
		else if (*last_key == KEY_RIGHT)
			Secondary_autoselect_mode = (Secondary_autoselect_mode + 1) % AS_NUM_MODES;

		items[citem + 1].text = (char*)Autoselect_mode_names[Secondary_autoselect_mode];
		items[citem + 1].redraw = true;
	}
	else if (citem == 5)
	{
		if (*last_key == KEY_LEFT)
		{
			Player_message_level--; if (Player_message_level < 0) Player_message_level = MSG_NUM_MODES - 1;
		}
		else if (*last_key == KEY_RIGHT)
			Player_message_level = (Player_message_level + 1) % MSG_NUM_MODES;

		items[citem + 1].text = (char*)Message_level_names[Player_message_level];
		items[citem + 1].redraw = true;
	}

	Auto_leveling_on = items[0].value;
}

bool gameplay_options_callback(int choice, int nitems, newmenu_item* item)
{
	switch (choice)
	{
	case -1:
		return false;
	case 1:
		Primary_autoselect_mode = (Primary_autoselect_mode + 1) % AS_NUM_MODES;
		item[choice + 1].text = (char*)Autoselect_mode_names[Primary_autoselect_mode]; item[choice + 1].redraw = true;
		break;
	case 3:
		Secondary_autoselect_mode = (Secondary_autoselect_mode + 1) % AS_NUM_MODES;
		item[choice + 1].text = (char*)Autoselect_mode_names[Secondary_autoselect_mode]; item[choice + 1].redraw = true;
		break;
	case 5:
		Player_message_level = (Player_message_level + 1) % MSG_NUM_MODES;
		item[choice + 1].text = (char*)Message_level_names[Player_message_level]; item[choice + 1].redraw = true;
		break;
	}

	return true;
}

void do_gameplay_options_menu()
{
	newmenu_item m[7];
	m[0].type = NM_TYPE_CHECK; m[0].text = (char*)"Ship auto-leveling"; m[0].value = Auto_leveling_on;
	m[1].type = NM_TYPE_MENU; m[1].text = (char*)"Primary autoselect on pickup:";
	m[2].type = NM_TYPE_TEXT; m[2].text = (char*)Autoselect_mode_names[Primary_autoselect_mode];
	m[3].type = NM_TYPE_MENU; m[3].text = (char*)"Secondary autoselect on pickup:";
	m[4].type = NM_TYPE_TEXT; m[4].text = (char*)Autoselect_mode_names[Secondary_autoselect_mode];
	m[5].type = NM_TYPE_MENU; m[5].text = (char*)"Message level:";
	m[6].type = NM_TYPE_TEXT; m[6].text = (char*)Message_level_names[Player_message_level];

	newmenu_open(nullptr, "GAME OPTIONS", 7, m, gameplay_options_menuset, gameplay_options_callback);
}

void joydef_menuset(int nitems, newmenu_item* items, int* last_key, int citem)
{
	nitems = nitems;
	*last_key = *last_key;
}

bool joydef_callback(int choice, int nitems, newmenu_item* item)
{
	switch (choice)
	{
	case -1:
		write_player_file();
		return false; //Done here
	case 0:
		do_sound_menu();
		break;
	case 1:
		do_video_menu();
		break;
	case 3:
		joydefs_config();
		break;
	case 4:
		do_detail_level_menu();
		break;
	case 6:
		do_gameplay_options_menu();
		break;
	}
	return true;
}

void do_options_menu()
{
	newmenu_item m[10];
	m[0].type = NM_TYPE_MENU; m[0].text = (char*)"SOUND OPTIONS...";
	m[1].type = NM_TYPE_MENU; m[1].text = (char*)"VIDEO OPTIONS...";
	m[2].type = NM_TYPE_TEXT; m[2].text = (char*)"";
	m[3].type = NM_TYPE_MENU; m[3].text = TXT_CONTROLS_;
	m[4].type = NM_TYPE_MENU; m[4].text = TXT_DETAIL_LEVELS;
	m[5].type = NM_TYPE_TEXT; m[5].text = (char*)"";
	m[6].type = NM_TYPE_MENU; m[6].text = (char*)"GAMEPLAY OPTIONS...";

	newmenu_open(NULL, TXT_OPTIONS, 7, m, joydef_menuset, joydef_callback);
}

char direct_join_str[40] = "CONNECT TO IP ADDRESS...";

void do_multi_player_menu()
{
	int menu_choice[3];
	newmenu_item m[3];
	int choice = 0, num_options = 0;
	int old_game_mode;

	do 
	{
		old_game_mode = Game_mode;
		num_options = 0;

#ifdef NETWORK
		//go back to the old port when starting new games
		NetChangeDefaultSocket(Current_Port);
#endif

		ADD_ITEM(TXT_START_NET_GAME, MENU_START_NETGAME, -1);
		ADD_ITEM(TXT_JOIN_NET_GAME, MENU_JOIN_NETGAME, -1);
		ADD_ITEM(direct_join_str, MENU_START_SERIAL, -1);

		choice = newmenu_do1(NULL, TXT_MULTIPLAYER, num_options, m, NULL, choice);

		if (choice > -1)
			do_option(menu_choice[choice]);

		if (old_game_mode != Game_mode)
			break;		// leave menu

	} while (choice > -1);
}

#ifdef NETWORK
void do_ip_address_menu()
{
	newmenu_item m;
	static char text[256] = "";
	uint8_t address[4];
	char* ptr, *oldptr;
	int i;
	int value;

	char buf[256];

	uint16_t oldPort = NetGetCurrentPort();
	snprintf(buf, 255, "Enter IP address\nCurrent port is %d", oldPort);
	buf[255] = '\0';

	m.type = NM_TYPE_INPUT; m.text_len = 255; m.text = text;

	int opt = newmenu_do(NULL, buf, 1, &m, NULL);

	if (opt == -1) return;

	//new_level_num = atoi(m.text);
	char* colonPtr = strchr(text, ':');
	if (colonPtr)
	{
		*colonPtr = '\0';
		char* portString = colonPtr+1;
		int newPort = atoi(portString);
		NetChangeDefaultSocket(newPort);
	}
	ptr = oldptr = text;
	for (i = 0; i < 4; i++)
	{
		if (i != 3)
		{
			ptr = strchr(ptr, '.');
			if (!ptr)
			{
				nm_messagebox(NULL, 1, TXT_OK, "Address is formatted incorrectly");
				return;
			}
			*ptr = '\0';
		}
		value = atoi(oldptr);
		if (value > 255)
		{
			nm_messagebox(NULL, 1, TXT_OK, "Invalid number in address");
			return;
		}
		address[i] = value;
		if (i != 3)
		{
			*ptr = '.';
			ptr++;
		}
		oldptr = ptr;
	}

	network_join_game_at(address);

	if (colonPtr)
	{
		*colonPtr = ':';
	}

	return;
}
#endif

#include "platform/platform.h"

std::vector<const char*> resolution_menu_strings =
{
	"320x200",
	"320x240",
	"640x480",
	"800x600",
	"1024x768",
	"1280x1024",
	"1366x768",
	"1600x900",
	"1920x1080",
	"2560x1440",
};

//TODO: I'm going to need to change up how menus are generated to avoid these globals
static newmenu_item* vid_res_buffer;

bool video_resolution_callback(int choice, int nitems, newmenu_item* items)
{
	if (choice >= 0 && choice < resolution_menu_strings.size())
	{
		strncpy(vid_res_buffer->text, resolution_menu_strings[choice], 63);
		vid_res_buffer->text[63] = '\0';
		vid_res_buffer->redraw = true;
	}

	return false;
}

void get_video_resolution(char* buf)
{
	std::vector<newmenu_item> items;

	for (const char* str : resolution_menu_strings)
	{
		newmenu_item item = {};
		item.type = NM_TYPE_MENU; item.text = (char*)str; 
		items.push_back(item);
	}

	newmenu_open(nullptr, "Select resolution", items, nullptr, video_resolution_callback);
}

const char* select_window_res_text = "SELECT WINDOW SIZE...";
const char* select_render_res_text = "SELECT RENDER RESOLUTION...";
const char* fullscreen_text = "FULLSCREEN";
const char* vsync_text = "VSYNC";
const char* aspect_text = "ASPECT RATIO";

const char* vsync_status[] = { "OFF", "ON", "ADAPTIVE" };
const char* aspect_status[] = { "AUTO", "4:3", "5:4", "16:10", "16:9", "21:9" };

static int set_swap_interval;

void video_menuset(int nitems, newmenu_item* items, int* last_key, int citem)
{
	if (citem == 3)
	{
		snprintf(items[3].text, 64, "%s: %s", vsync_text, vsync_status[SwapInterval]);
		items[3].text[63] = '\0';

		items[3].redraw = true;
	}
	else if (citem == 7)
	{
		if (*last_key == KEY_LEFT)
		{
			cfg_aspect_ratio--; if (cfg_aspect_ratio < 0) cfg_aspect_ratio = GAMEASPECT_COUNT - 1;
		}
		else if (*last_key == KEY_RIGHT)
			cfg_aspect_ratio = (cfg_aspect_ratio + 1) % GAMEASPECT_COUNT;

		snprintf(items[7].text, 64, "%s: %s", aspect_text, aspect_status[cfg_aspect_ratio]);
		items[7].text[63] = '\0';

		items[7].redraw = true;
	}
	else if (citem == 9)
	{
		gr_palette_set_gamma(items[9].value);
	}
}

bool video_callback(int choice, int nitems, newmenu_item* item)
{
	if (choice == -1)
	{
		Fullscreen = item[2].value;

		char* x_ptr = strchr(item[1].text, 'x');
		if (!x_ptr)
			x_ptr = strchr(item[1].text, 'X');
		if (!x_ptr)
			x_ptr = strchr(item[1].text, '*');

		if (!x_ptr)
			nm_open_messagebox(nullptr, nullptr, 1, TXT_OK, "Can't read window size");
		else
		{
			*x_ptr = '\0';
			int new_width = atoi(item[1].text);
			int new_height = atoi(x_ptr + 1);
			if (new_width < 320 || new_height < 200)
				nm_open_messagebox(nullptr, nullptr, 1, TXT_OK, "Window size is invalid");
			else
			{
				WindowWidth = new_width;
				WindowHeight = new_height;
			}

			*x_ptr = 'x';
		}

		x_ptr = strchr(item[6].text, 'x');
		if (!x_ptr)
			x_ptr = strchr(item[6].text, 'X');
		if (!x_ptr)
			x_ptr = strchr(item[6].text, '*');

		if (!x_ptr)
			nm_open_messagebox(nullptr, nullptr, 1, TXT_OK, "Can't read render size");
		else
		{
			*x_ptr = '\0';
			int new_width = atoi(item[6].text);
			int new_height = atoi(x_ptr + 1);
			if (new_width < 320 || new_height < 200)
				nm_open_messagebox(nullptr, nullptr, 1, TXT_OK, "Render size is invalid");
			else
			{
				cfg_render_width = new_width;
				cfg_render_height = new_height;
			}

			*x_ptr = 'x';
		}

		plat_update_window();

		//The platform code will have changed SwapInterval if an option was picked that wasn't supported
		if (SwapInterval != 2 && set_swap_interval == 2)
			nm_open_messagebox(TXT_ERROR, nullptr, 1, TXT_OK, "Adaptive vsync is not available.\nNormal vsync has been enabled.");

		return false;
	}
	if (choice == 0)
	{
		vid_res_buffer = &item[1];
		get_video_resolution(item[1].text);
	}
	else if (choice == 5)
	{
		vid_res_buffer = &item[6];
		get_video_resolution(item[6].text);
	}

	else if (choice == 3)
	{
		set_swap_interval = SwapInterval = (SwapInterval + 1) % 3;
	}

	else if (choice == 7)
	{
		cfg_aspect_ratio = (cfg_aspect_ratio + 1) % GAMEASPECT_COUNT;
	}

	item[choice].redraw = true;
	return true;
}

void do_video_menu()
{
	newmenu_item m[10];
	static char window_res_string[64];
	static char render_res_string[64];
	static char vsync_buffer[64];
	static char aspect_buffer[64];
	set_swap_interval = 0;

	snprintf(window_res_string, 64, "%dx%d", WindowWidth, WindowHeight);
	window_res_string[63] = '\0';

	snprintf(render_res_string, 64, "%dx%d", cfg_render_width, cfg_render_height);
	render_res_string[63] = '\0';

	snprintf(vsync_buffer, 64, "%s: %s", vsync_text, vsync_status[SwapInterval]);
	vsync_buffer[63] = '\0';
	snprintf(aspect_buffer, 64, "%s: %s", aspect_text, aspect_status[cfg_aspect_ratio]);
	aspect_buffer[63] = '\0';
	m[0].type = NM_TYPE_MENU; m[0].text = (char*)select_window_res_text;
	m[1].type = NM_TYPE_INPUT; m[1].text = window_res_string; m[1].text_len = 63;
	m[2].type = NM_TYPE_CHECK; m[2].text = (char*)fullscreen_text; m[2].value = Fullscreen;
	m[3].type = NM_TYPE_MENU; m[3].text = (char*)vsync_buffer;
	m[4].type = NM_TYPE_TEXT; m[4].text = (char*)"";
	m[5].type = NM_TYPE_MENU; m[5].text = (char*)select_render_res_text;
	m[6].type = NM_TYPE_INPUT; m[6].text = render_res_string; m[6].text_len = 63;
	m[7].type = NM_TYPE_MENU; m[7].text = (char*)aspect_buffer;
	m[8].type = NM_TYPE_TEXT; m[8].text = (char*)"";
	m[9].type = NM_TYPE_SLIDER; m[9].text = TXT_BRIGHTNESS; m[9].value = gr_palette_get_gamma(); m[9].min_value = 0; m[9].max_value = 8;
	
	newmenu_open(NULL, "VIDEO OPTIONS", 10, m, video_menuset, video_callback);
}

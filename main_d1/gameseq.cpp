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

#if defined(__linux__) || defined(_WIN32) || defined(_WIN64)
#include <malloc.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>

#include "misc/rand.h"
#include "inferno.h"
#include "game.h"
#include "platform/key.h"
#include "object.h"
#include "physics.h"
#include "misc/error.h"
#include "platform/joy.h"
#include "platform/mono.h"
#include "iff/iff.h"
#include "2d/pcx.h"
#include "platform/timer.h"
#include "render.h"
#include "laser.h"
#include "screens.h"
#include "textures.h"
#include "slew.h"
#include "gauges.h"
#include "texmap/texmap.h"
#include "3d/3d.h"
#include "main_shared/effects.h"
#include "menu.h"
#include "gameseg.h"
#include "wall.h"
#include "ai.h"
#include "hostage.h"
#include "fuelcen.h"
#include "switch.h"
#include "main_shared/digi.h"
#include "gamesave.h"
#include "scores.h"
#include "2d/ibitblt.h"
#include "mem/mem.h"
#include "2d/palette.h"
#include "morph.h"
#include "lighting.h"
#include "newdemo.h"
#include "titles.h"
#include "collide.h"
#include "weapon.h"
#include "sounds.h"
#include "misc/args.h"
#include "gameseq.h"
#include "gamefont.h"
#include "newmenu.h"
#include "endlevel.h"
#include "network.h"
#include "playsave.h"
#include "ctype.h"
#include "multi.h"
#include "fireball.h"
#include "kconfig.h"
#include "config.h"
#include "robot.h"
#include "automap.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "stringtable.h"
#include "cfile/cfile.h"
#include "main_shared/piggy.h"
#include "main_shared/texmerge.h"
#include "paging.h"
#include "mission.h"
#include "state.h"
#include "main_shared/songs.h"
#ifdef NETWORK
#include "netmisc.h"
#endif

#ifdef EDITOR
#include "editor\editor.h"
#endif

//Current_level_num starts at 1 for the first level
//-1,-2,-3 are secret levels
//0 means not a real level loaded
int	Current_level_num = 0, Next_level_num;
char	Current_level_name[LEVEL_NAME_LEN];

int Last_level, Last_secret_level;

// Global variables describing the player
int 			N_players = 1;						// Number of players ( >1 means a net game, eh?)
int 			Player_num = 0;						// The player number who is on the console.
player			Players[MAX_PLAYERS];			// Misc player info
obj_position	Player_init[MAX_PLAYERS];

// Global variables telling what sort of game we have
int MaxNumNetPlayers = -1;
int NumNetPlayerPositions = -1;

// Extern from game.c to fix a bug in the cockpit!
extern int last_drawn_cockpit;
extern int Last_level_path_created;

void HUD_clear_messages(); // From hud.c

int Game_sub_mode = SUB_INDETERMINATE;
int Pending_sub_mode = SUB_INDETERMINATE;

//Level the game wants to load when briefings or intermissions are done. 
int Pending_level; 

void verify_console_object()
{
	Assert(Player_num > -1);
	Assert(Players[Player_num].objnum > -1);
	ConsoleObject = &Objects[Players[Player_num].objnum];
	Assert(ConsoleObject->type == OBJ_PLAYER);
	Assert(ConsoleObject->id == Player_num);
}

int count_number_of_robots()
{
	int robot_count;
	int i;

	robot_count = 0;
	for (i = 0; i <= Highest_object_index; i++) 
	{
		if (Objects[i].type == OBJ_ROBOT)
			robot_count++;
	}

	return robot_count;
}


int count_number_of_hostages()
{
	int count;
	int i;

	count = 0;
	for (i = 0; i <= Highest_object_index; i++) 
	{
		if (Objects[i].type == OBJ_HOSTAGE)
			count++;
	}

	return count;
}


void gameseq_init_network_players()
{
	int i, k, j;

	// Initialize network player start locations and object numbers

	ConsoleObject = &Objects[0];
	k = 0;
	j = 0;
	for (i = 0; i <= Highest_object_index; i++) 
	{
		if ((Objects[i].type == OBJ_PLAYER) || (Objects[i].type == OBJ_GHOST) || (Objects[i].type == OBJ_COOP))
		{
			if ((!(Game_mode & GM_MULTI_COOP) && ((Objects[i].type == OBJ_PLAYER) || (Objects[i].type == OBJ_GHOST))) ||
				((Game_mode & GM_MULTI_COOP) && ((j == 0) || (Objects[i].type == OBJ_COOP))))
			{
				mprintf((0, "Created Cooperative multiplayer object\n"));
				Objects[i].type = OBJ_PLAYER;
				mprintf((0, "Player init %d is ship %d.\n", k, j));
				Player_init[k].pos = Objects[i].pos;
				Player_init[k].orient = Objects[i].orient;
				Player_init[k].segnum = Objects[i].segnum;
				Players[k].objnum = i;
				Objects[i].id = k;
				k++;
			}
			else
				obj_delete(i);
			j++;
		}
	}
	NumNetPlayerPositions = k;

#ifndef NDEBUG
	if (((Game_mode & GM_MULTI_COOP) && (NumNetPlayerPositions != 4)) ||
		(!(Game_mode & GM_MULTI_COOP) && (NumNetPlayerPositions != 8)))
	{
		mprintf((1, "--NOT ENOUGH MULTIPLAYER POSITIONS IN THIS MINE!--\n"));
		//Int3(); // Not enough positions!!
	}
#endif
}

void gameseq_remove_unused_players()
{
	int i;

	// 'Remove' the unused players
#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		for (i = 0; i < NumNetPlayerPositions; i++)
		{
			if ((!Players[i].connected) || (i >= N_players))
			{
				mprintf((0, "Ghosting player ship %d.\n", i+1));
				multi_make_player_ghost(i);
			}
		}
	}
	else
#endif
	{		// Note link to above if!!!
#ifndef NDEBUG
		mprintf((0, "Removing player objects numbered %d-%d.\n", 1, NumNetPlayerPositions));
#endif
		for (i = 1; i < NumNetPlayerPositions; i++)
		{
			obj_delete(Players[i].objnum);
		}
	}
}

// Setup player for new game
void init_player_stats_game()
{
	Players[Player_num].score = 0;
	Players[Player_num].last_score = 0;
	Players[Player_num].lives = INITIAL_LIVES;
	Players[Player_num].level = 1;

	Players[Player_num].time_level = 0;
	Players[Player_num].time_total = 0;
	Players[Player_num].hours_level = 0;
	Players[Player_num].hours_total = 0;

	Players[Player_num].energy = MAX_ENERGY;
	Players[Player_num].shields = MAX_SHIELDS;
	Players[Player_num].killer_objnum = -1;

	Players[Player_num].net_killed_total = 0;
	Players[Player_num].net_kills_total = 0;

	Players[Player_num].num_kills_level = 0;
	Players[Player_num].num_kills_total = 0;
	Players[Player_num].num_robots_level = 0;
	Players[Player_num].num_robots_total = 0;

	Players[Player_num].hostages_rescued_total = 0;
	Players[Player_num].hostages_level = 0;
	Players[Player_num].hostages_total = 0;

	Players[Player_num].laser_level = 0;
	Players[Player_num].flags = 0;

	init_player_stats_new_ship();
}

void init_ammo_and_energy(void)
{
	if (Players[Player_num].energy < MAX_ENERGY)
		Players[Player_num].energy = MAX_ENERGY;
	if (Players[Player_num].shields < MAX_SHIELDS)
		Players[Player_num].shields = MAX_SHIELDS;

	if (Players[Player_num].secondary_ammo[0] < 2 + NDL - Difficulty_level)
		Players[Player_num].secondary_ammo[0] = 2 + NDL - Difficulty_level;
}

// Setup player for new level (After completion of previous level)
void init_player_stats_level()
{
	// int	i;

	Players[Player_num].last_score = Players[Player_num].score;
	Players[Player_num].level = Current_level_num;

#ifdef NETWORK
	if (!Network_rejoined)
#endif
		Players[Player_num].time_level = 0;	//Note link to above if !!!!!!

	init_ammo_and_energy();

	Players[Player_num].killer_objnum = -1;

	Players[Player_num].num_kills_level = 0;
	Players[Player_num].num_robots_level = count_number_of_robots();
	Players[Player_num].num_robots_total += Players[Player_num].num_robots_level;

	Players[Player_num].hostages_level = count_number_of_hostages();
	Players[Player_num].hostages_total += Players[Player_num].hostages_level;
	Players[Player_num].hostages_on_board = 0;

	Players[Player_num].flags &= (~KEY_BLUE);
	Players[Player_num].flags &= (~KEY_RED);
	Players[Player_num].flags &= (~KEY_GOLD);

	Players[Player_num].flags &= (~PLAYER_FLAGS_INVULNERABLE);
	Players[Player_num].flags &= (~PLAYER_FLAGS_CLOAKED);

	Players[Player_num].cloak_time = 0;
	Players[Player_num].invulnerable_time = 0;

	if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
		Players[Player_num].flags |= (KEY_BLUE | KEY_RED | KEY_GOLD);

	Player_is_dead = 0; // Added by RH
	Players[Player_num].homing_object_dist = -F1_0; // Added by RH

	Last_laser_fired_time = Next_laser_fire_time = GameTime; // added by RH, solved demo playback bug

	init_gauges();
}

// Setup player for a brand-new ship
void init_player_stats_new_ship()
{
	int	i;

	if (Newdemo_state == ND_STATE_RECORDING) 
	{
		newdemo_record_laser_level(Players[Player_num].laser_level, 0);
		newdemo_record_player_weapon(0, 0);
		newdemo_record_player_weapon(1, 0);
	}

	Players[Player_num].energy = MAX_ENERGY;
	Players[Player_num].shields = MAX_SHIELDS;
	Players[Player_num].laser_level = 0;
	Players[Player_num].killer_objnum = -1;
	Players[Player_num].hostages_on_board = 0;

	for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
		Players[Player_num].primary_ammo[i] = 0;

	for (i = 1; i < MAX_SECONDARY_WEAPONS; i++)
		Players[Player_num].secondary_ammo[i] = 0;
	Players[Player_num].secondary_ammo[0] = 2 + NDL - Difficulty_level;

	Players[Player_num].primary_weapon_flags = HAS_LASER_FLAG;
	Players[Player_num].secondary_weapon_flags = HAS_CONCUSSION_FLAG;

	Primary_weapon = 0;
	Secondary_weapon = 0;

	Players[Player_num].flags &= ~(PLAYER_FLAGS_QUAD_LASERS | PLAYER_FLAGS_AFTERBURNER | PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE);

	Players[Player_num].cloak_time = 0;
	Players[Player_num].invulnerable_time = 0;

	Player_is_dead = 0;		//player no longer dead

	Players[Player_num].homing_object_dist = -F1_0; // Added by RH
}

#ifdef NETWORK
void reset_network_objects()
{
	memset(local_to_remote, -1, MAX_OBJECTS * sizeof(short));
	memset(remote_to_local, -1, MAX_NUM_NET_PLAYERS * MAX_OBJECTS * sizeof(short));
	memset(object_owner, -1, MAX_OBJECTS);
}
#endif

#ifdef EDITOR

//reset stuff so game is semi-normal when playing from editor
void editor_reset_stuff_on_level()
{
	gameseq_init_network_players();
	init_player_stats_level();
	Viewer = ConsoleObject;
	ConsoleObject = Viewer = &Objects[Players[Player_num].objnum];
	ConsoleObject->id = Player_num;
	ConsoleObject->control_type = CT_FLYING;
	ConsoleObject->movement_type = MT_PHYSICS;
	Game_suspended = 0;
	verify_console_object();
	Fuelcen_control_center_destroyed = 0;
	if (Newdemo_state != ND_STATE_PLAYBACK)
		gameseq_remove_unused_players();
	init_cockpit();
	init_robots_for_level();
	init_ai_objects();
	init_morphs();
	init_all_matcens();
	init_player_stats_new_ship();
}
#endif

void reset_player_object();


static fix time_out_value;
void DoEndLevelScoreGlitzPoll(int nitems, newmenu_item* menus, int* key, int citem)
{
	if (timer_get_approx_seconds() > time_out_value)
	{
		*key = -2;
	}
}

//do whatever needs to be done when a player dies in multiplayer

void DoGameOver()
{
	//time_out_value = timer_get_approx_seconds() + i2f(60 * 5);
	//nm_messagebox1(TXT_GAME_OVER, DoEndLevelScoreGlitzPoll, 1, TXT_OK, "");

	if (Current_mission_num == 0)
		scores_maybe_add_player(0);

	Function_mode = FMODE_MENU;
	Game_mode = GM_GAME_OVER;
	longjmp(LeaveGame, 0);		// Exit out of game loop
}

//update various information about the player
void update_player_stats()
{
	Players[Player_num].time_level += FrameTime;	//the never-ending march of time...
	if (Players[Player_num].time_level > i2f(3600)) 
	{
		Players[Player_num].time_level -= i2f(3600);
		Players[Player_num].hours_level++;
	}

	Players[Player_num].time_total += FrameTime;	//the never-ending march of time...
	if (Players[Player_num].time_total > i2f(3600)) 
	{
		Players[Player_num].time_total -= i2f(3600);
		Players[Player_num].hours_total++;
	}
}

//go through this level and start any eclip sounds
void set_sound_sources()
{
	int segnum, sidenum;
	segment* seg;

	digi_init_sounds();		//clear old sounds

	for (seg = &Segments[0], segnum = 0; segnum <= Highest_segment_index; seg++, segnum++)
		for (sidenum = 0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++) 
		{
			int tm, ec, sn;

			if ((tm = seg->sides[sidenum].tmap_num2) != 0)
				if ((ec = TmapInfo[tm & 0x3fff].eclip_num) != -1)
					if ((sn = Effects[ec].sound_num) != -1) 
					{
						vms_vector pnt;

						compute_center_point_on_side(&pnt, seg, sidenum);
						digi_link_sound_to_pos(sn, segnum, sidenum, &pnt, 1, F1_0 / 2);

					}
		}
}


//fix flash_dist=i2f(1);
fix flash_dist = fl2f(.9);

//create flash for player appearance
void create_player_appearance_effect(object* player_obj)
{
	vms_vector pos;
	object* effect_obj;

#ifndef NDEBUG
	{
		int objnum = player_obj - Objects;
		if ((objnum < 0) || (objnum > Highest_object_index))
			Int3(); // See Rob, trying to track down weird network bug
	}
#endif

	if (player_obj == Viewer)
		vm_vec_scale_add(&pos, &player_obj->pos, &player_obj->orient.fvec, fixmul(player_obj->size, flash_dist));
	else
		pos = player_obj->pos;

	effect_obj = object_create_explosion(player_obj->segnum, &pos, player_obj->size, VCLIP_PLAYER_APPEARANCE);

	if (effect_obj) 
	{
		effect_obj->orient = player_obj->orient;

		if (Vclip[VCLIP_PLAYER_APPEARANCE].sound_num > -1)
			digi_link_sound_to_object(Vclip[VCLIP_PLAYER_APPEARANCE].sound_num, effect_obj - Objects, 0, F1_0);
	}
}

//
// New Game sequencing functions
//

//pairs of chars describing ranges
char playername_allowed_chars[] = "azAZ09__--";

static char new_pilot_name[CALLSIGN_LEN + 1];

void MakeNewPlayerFile(int allow_abort);
bool MakeNewPlayerFileCallback(int choice, int nitems, newmenu_item* item)
{
	char filename[14];

	if (new_pilot_name[0] == 0)	//null string
		return true; //keep open

	sprintf(filename, "%s.nplt", new_pilot_name);

	FILE* fp = fopen(filename, "rb");

	//if the callsign is the name of a tty device, prepend a char
	/*if (fp && isatty(fileno(fp))) //[ISB] fixme pls
	{
		fclose(fp);
		sprintf(filename, "$%.7s.plr", text);
		fp = fopen(filename, "rb");
	}*/

	if (fp)
	{
		nm_messagebox(NULL, 1, TXT_OK, "%s '%s' %s", TXT_PLAYER, new_pilot_name, TXT_ALREADY_EXISTS);
		fclose(fp);
		return true;
	}

	//Unlike before, this never fails. 
	new_player_config();

	strncpy(Players[Player_num].callsign, new_pilot_name, CALLSIGN_LEN);

	write_player_file();

	Auto_leveling_on = Default_leveling_on;

	WriteConfigFile();		// Update lastplr

	return false; //no need to keep the window up
}

void MakeNewPlayerFile(int allow_abort)
{
	newmenu_item m;

	strncpy(new_pilot_name, Players[Player_num].callsign, CALLSIGN_LEN);

	m.type = NM_TYPE_INPUT; m.text_len = 8; m.text = new_pilot_name;

	Newmenu_allowed_chars = playername_allowed_chars;
	newmenu_open(NULL, TXT_ENTER_PILOT_NAME, 1, &m, nullptr, MakeNewPlayerFileCallback);
	Newmenu_allowed_chars = NULL;
}

void RegisterPlayerCallback(std::string& str, int choice)
{
	int allow_abort_flag = 1;
	if (config_last_player[0] == 0)
		allow_abort_flag = 0;

	if (choice == 0) 
	{
		// They selected 'create new pilot'
		MakeNewPlayerFile(allow_abort_flag);
	}
	else
	{
		strncpy(Players[Player_num].callsign, str.c_str(), CALLSIGN_LEN);
		read_player_file();

		Auto_leveling_on = Default_leveling_on;

		WriteConfigFile();		// Update lastplr
	}
}

//Inputs the player's name, without putting up the background screen
int RegisterPlayer()
{
	int i, j;
	char filename[14];
	int allow_abort_flag = 1;

	if (Players[Player_num].callsign[0] == 0) 
	{
		//---------------------------------------------------------------------
		// Set default config options in case there is no config file
		// kc_keyboard, kc_joystick, kc_mouse are statically defined.
		Config_joystick_sensitivity = 8;
		Config_control_type = CONTROL_NONE;
		for (i = 0; i < CONTROL_MAX_TYPES; i++)
			for (j = 0; j < MAX_CONTROLS; j++)
				kconfig_settings[i][j] = default_kconfig_settings[i][j];
		kc_set_controls();
		//----------------------------------------------------------------

		// Read the last player's name from config file, not lastplr.txt
		strncpy(Players[Player_num].callsign, config_last_player, CALLSIGN_LEN);

		if (config_last_player[0] == 0)
			allow_abort_flag = 0;
	}


	char localized_pilot_query[CHOCOLATE_MAX_FILE_PATH_SIZE];
	//get_platform_localized_query_string(localized_pilot_query, CHOCOLATE_PILOT_DIR, "*.nplt");
	get_full_file_path(localized_pilot_query, "*.nplt", CHOCOLATE_PILOT_DIR); //Possibly a bad idea, but I need the search string relative to the basedir. 
	//newmenu_get_filename(TXT_SELECT_PILOT, localized_pilot_query, filename, allow_abort_flag)
	newmenu_open_filepicker(TXT_SELECT_PILOT, localized_pilot_query, allow_abort_flag, RegisterPlayerCallback);

	return 1;
}

extern int descent_critical_error;

//load a level off disk. level numbers start at 1.  Secret levels are -1,-2,-3
void LoadLevel(int level_num)
{
	char* level_name;
	player save_player;

	save_player = Players[Player_num];

	Assert(level_num <= Last_level && level_num >= Last_secret_level && level_num != 0);

#ifdef SHAREWARE
	{
		static char t[13];
		sprintf(t, "LEVEL%02d.SDL", level_num);
		level_name = t;
	}
#else
	if (level_num < 0)		//secret level
		level_name = Secret_level_names[-level_num - 1];
	else					//normal level
		level_name = Level_names[level_num - 1];
#endif

	show_boxed_message(TXT_LOADING);

	if (!load_level(level_name))
		Current_level_num = level_num;

#ifdef NETWORK
	my_segments_checksum = netmisc_calc_checksum(Segments, sizeof(segment) * (Highest_segment_index + 1));
#endif

	load_endlevel_data(level_num);
	clear_boxed_message();

#ifdef NETWORK
	reset_network_objects();
#endif

	Players[Player_num] = save_player;
	set_sound_sources();
	songs_play_level_song(Current_level_num);
}

//sets up Player_num & ConsoleObject  
void InitPlayerObject()
{
	Assert(Player_num >= 0 && Player_num < MAX_PLAYERS);

	if (Player_num != 0)
	{
		Players[0] = Players[Player_num];
		Player_num = 0;
	}

	Players[Player_num].objnum = 0;

	ConsoleObject = &Objects[Players[Player_num].objnum];

	ConsoleObject->type = OBJ_PLAYER;
	ConsoleObject->id = Player_num;
	ConsoleObject->control_type = CT_FLYING;
	ConsoleObject->movement_type = MT_PHYSICS;
}

extern void game_disable_cheats();

//starts a new game on the given level
void StartNewGame(int start_level)
{
	Game_mode = GM_NORMAL;
	Function_mode = FMODE_GAME;

	Next_level_num = 0;

	InitPlayerObject();				//make sure player's object set up

	init_player_stats_game();		//clear all stats

	N_players = 1;
#ifdef NETWORK
	Network_new_game = 0;
#endif

	StartNewLevel(start_level);

	Players[Player_num].starting_level = start_level;		// Mark where they started

	game_disable_cheats();
}

#ifdef NETWORK
extern void network_endlevel_poll2(int nitems, newmenu_item* menus, int* key, int citem); // network.c
#endif

static bool IntermissionStarted = false;

bool EndLevelHandler(int choice, int nitems, newmenu_item* items)
{
	IntermissionStarted = false;
	if (!inferno_is_screen_faded())
		inferno_request_fade_out();

	return true; //menu will be closed by transition
}


//	-----------------------------------------------------------------------------
//	Does the bonus scoring.
//	Call with dead_flag = 1 if player died, but deserves some portion of bonus (only skill points), anyway.
void DoEndLevelScoreGlitz(int network)
{
	int level_points, skill_points, energy_points, shield_points, hostage_points;
	int	all_hostage_points;
	int	endgame_points;
	char	all_hostage_text[64];
	char	endgame_text[64];
#define N_GLITZITEMS 9
	static char		m_str[N_GLITZITEMS][30]; //TODO: I need to make menu items copy their text
	newmenu_item	m[9];
	int				i, c;
	char				title[128];
	int				is_last_level;

	IntermissionStarted = true;

	level_points = Players[Player_num].score - Players[Player_num].last_score;

	if (!Cheats_enabled) 
	{
		if (Difficulty_level > 1) 
		{
			skill_points = level_points * (Difficulty_level - 1) / 2;
			skill_points -= skill_points % 100;
		}
		else
			skill_points = 0;

		shield_points = f2i(Players[Player_num].shields) * 10 * (Difficulty_level + 1);
		energy_points = f2i(Players[Player_num].energy) * 5 * (Difficulty_level + 1);
		hostage_points = Players[Player_num].hostages_on_board * 500 * (Difficulty_level + 1);
	}
	else 
	{
		skill_points = 0;
		shield_points = 0;
		energy_points = 0;
		hostage_points = 0;
	}

	all_hostage_text[0] = 0;
	endgame_text[0] = 0;

	if (!Cheats_enabled && (Players[Player_num].hostages_on_board == Players[Player_num].hostages_level)) 
	{
		all_hostage_points = Players[Player_num].hostages_on_board * 1000 * (Difficulty_level + 1);
		sprintf(all_hostage_text, "%s%i\n", TXT_FULL_RESCUE_BONUS, all_hostage_points);
	}
	else
		all_hostage_points = 0;

	if (!Cheats_enabled && !(Game_mode & GM_MULTI) && (Players[Player_num].lives) && (Current_level_num == Last_level)) //player has finished the game!
	{
		endgame_points = Players[Player_num].lives * 10000;
		sprintf(endgame_text, "%s%i\n", TXT_SHIP_BONUS, endgame_points);
		is_last_level = 1;
	}
	else
		endgame_points = is_last_level = 0;

	add_bonus_points_to_score(skill_points + energy_points + shield_points + hostage_points + all_hostage_points + endgame_points);

	c = 0;
	sprintf(m_str[c++], "%s%i", TXT_SHIELD_BONUS, shield_points);		// Return at start to lower menu...
	sprintf(m_str[c++], "%s%i", TXT_ENERGY_BONUS, energy_points);
	sprintf(m_str[c++], "%s%i", TXT_HOSTAGE_BONUS, hostage_points);
	sprintf(m_str[c++], "%s%i", TXT_SKILL_BONUS, skill_points);

	sprintf(m_str[c++], "%s", all_hostage_text);
	if (!(Game_mode & GM_MULTI) && (Players[Player_num].lives) && (Current_level_num == Last_level))
		sprintf(m_str[c++], "%s", endgame_text);

	sprintf(m_str[c++], "%s%i\n", TXT_TOTAL_BONUS, shield_points + energy_points + hostage_points + skill_points + all_hostage_points + endgame_points);
	sprintf(m_str[c++], "%s%i", TXT_TOTAL_SCORE, Players[Player_num].score);

	for (i = 0; i < c; i++) 
	{
		m[i].type = NM_TYPE_TEXT;
		m[i].text = m_str[i];
	}

	if (Current_level_num < 0)
		sprintf(title, "%s%s %d %s\n %s %s", is_last_level ? "\n\n\n" : "\n", TXT_SECRET_LEVEL, -Current_level_num, TXT_COMPLETE, Current_level_name, TXT_DESTROYED);
	else
		sprintf(title, "%s%s %d %s\n%s %s", is_last_level ? "\n\n\n" : "\n", TXT_LEVEL, Current_level_num, TXT_COMPLETE, Current_level_name, TXT_DESTROYED);

	Assert(c <= N_GLITZITEMS);

	//gr_palette_fade_out(gr_palette, 32, 0);

	time_out_value = timer_get_approx_seconds() + i2f(60 * 5);

#ifdef NETWORK
	if (network && (Game_mode & GM_NETWORK))
		newmenu_open(NULL, title, c, m, network_endlevel_poll2, EndLevelHandler, 0, "MENU.PCX");
	else
#endif	// Note link!
		newmenu_open(NULL, title, c, m, DoEndLevelScoreGlitzPoll, EndLevelHandler, 0, "MENU.PCX");
}

//True if the intermission is for a secret level, and should show the message. 
static bool Inter_secret_flag; 

static bool Inter_died_in_mine;

int AdvanceLevel(int secret_flag);

extern bool do_end_game(void);

//called when the player has finished a level
void PlayerFinishedLevel(int secret_flag)
{
	int	rval;
	int 	was_multi = 0;

	Inter_secret_flag = !!secret_flag;
	//Handle the last level. Don't do this in multiplayer, though.
	if (Current_level_num == Last_level && !(Game_mode & GM_MULTI))
	{
		if ((Newdemo_state == ND_STATE_RECORDING) || (Newdemo_state == ND_STATE_PAUSED))
			newdemo_stop_recording();

		songs_play_song(SONG_ENDGAME, 0);
		if (do_end_game())
			Pending_sub_mode = SUB_BRIEFING;
		else
			Pending_sub_mode = SUB_INTERMISSION;
	}
	else
	{
		Pending_sub_mode = SUB_INTERMISSION;
	}
	newmenu_close_all();

	//credit the player for hostages
	Players[Player_num].hostages_rescued_total += Players[Player_num].hostages_on_board;

	if (Game_mode & GM_NETWORK)
		if (secret_flag)
			Players[Player_num].connected = 4; // Finished and went to secret level
		else
			Players[Player_num].connected = 2; // Finished but did not die

	last_drawn_cockpit = -1;

	/*if (Current_level_num == Last_level)
	{
	}*/

	/*
	if (!(Game_mode & GM_MULTI) && (secret_flag)) 
	{
		newmenu_item	m[1];

		m[0].type = NM_TYPE_TEXT;
		m[0].text = (char*)" ";			//TXT_SECRET_EXIT;

		newmenu_do2(NULL, TXT_SECRET_EXIT, 1, m, NULL, 0, "MENU.PCX");
	}

	if (Game_mode & GM_NETWORK)
		if (secret_flag)
			Players[Player_num].connected = 4; // Finished and went to secret level
		else
			Players[Player_num].connected = 2; // Finished but did not die

	last_drawn_cockpit = -1;

	if (Current_level_num == Last_level) 
	{
#ifdef NETWORK
		if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
		{
			was_multi = 1;
			multi_endlevel_score();
			rval = AdvanceLevel(secret_flag);				//now go on to the next one (if one)
		}
		else
#endif
		{	// Note link to above else!
			rval = AdvanceLevel(secret_flag);				//now go on to the next one (if one)
			DoEndLevelScoreGlitz(0);		//give bonuses
		}
	}
	else 
	{
#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_endlevel_score();
		else
#endif	// Note link!!
			DoEndLevelScoreGlitz(0);		//give bonuses
		rval = AdvanceLevel(secret_flag);				//now go on to the next one (if one)
	}

	if (!was_multi && rval) 
	{
		if (Current_mission_num == 0)
			scores_maybe_add_player(0);
		longjmp(LeaveGame, 0);		// Exit out of game loop
	}
	else if (rval)
		longjmp(LeaveGame, 0);
	*/
}

//from which level each do you get to each secret level 
int Secret_level_table[MAX_SECRET_LEVELS_PER_MISSION];

//called to go to the next level (if there is one)
//if secret_flag is true, advance to secret level, else next normal one
//	Return true if game over.
int AdvanceLevel(int secret_flag)
{
	Fuelcen_control_center_destroyed = 0;
#ifdef EDITOR
	if (Current_level_num == 0)
		return 0;		//not a real level
#endif
	
	//TODO: This needs to be done elsewhere. 
	/*
#ifdef NETWORK
	int result;
	if (Game_mode & GM_MULTI)
	{
		result = multi_endlevel(&secret_flag); // Wait for other players to reach this point
		if (result) // failed to sync
			return (Current_level_num == Last_level);
	}
#endif*/

	if (Current_level_num == Last_level) //player has finished the game!
	{
		Function_mode = FMODE_MENU;
		Game_mode = GM_GAME_OVER;
		/*if ((Newdemo_state == ND_STATE_RECORDING) || (Newdemo_state == ND_STATE_PAUSED))
			newdemo_stop_recording();

		songs_play_song(SONG_ENDGAME, 0);

		do_end_game();*/
		return 1;
	}
	else 
	{
		Next_level_num = Current_level_num + 1;		//assume go to next normal level

		if (secret_flag) //go to secret level instead
		{
			int i;

			for (i = 0; i < -Last_secret_level; i++)
				if (Secret_level_table[i] == Current_level_num) 
				{
					Next_level_num = -(i + 1);
					break;
				}
			Assert(i < -Last_secret_level);		//couldn't find which secret level
		}

		if (Current_level_num < 0) //on secret level, where to go?
		{			
			Assert(!secret_flag);				//shouldn't be going to secret level
			Assert(Current_level_num <= -1 && Current_level_num >= Last_secret_level);

			Next_level_num = Secret_level_table[(-Current_level_num) - 1] + 1;
		}

		StartNewLevel(Next_level_num);
	}
	return 0;
}

bool died_in_mine_handler(int choice, int nitems, newmenu_item* items)
{
	DoEndLevelScoreGlitz(0);
	return false;
}

void died_in_mine_message(void)
{
	// Tell the player he died in the mine, explain why
	int pcx_error;

	if (Game_mode & GM_MULTI)
		return;

	gr_set_current_canvas(nm_canvas);

	pcx_error = pcx_read_bitmap("STARS.PCX", &grd_curcanv->cv_bitmap, grd_curcanv->cv_bitmap.bm_type, NULL);
	Assert(pcx_error == PCX_ERROR_NONE);
	nm_open_messagebox(nullptr, died_in_mine_handler, 1, TXT_OK, TXT_DIED_IN_MINE);
}

//called when the player has died
void DoPlayerDead()
{
	reset_palette_add();

	gr_palette_load(gr_palette);

	dead_player_end();		//terminate death sequence (if playing)

#ifdef HOSTAGE_FACES
	stop_all_hostage_clips();
#endif

#ifdef EDITOR
	if (Game_mode == GM_EDITOR) {			//test mine, not real level
		object* player = &Objects[Players[Player_num].objnum];
		//nm_messagebox( "You're Dead!", 1, "Continue", "Not a real game, though." );
		load_level("gamesave.lvl");
		init_player_stats_new_ship();
		player->flags &= ~OF_SHOULD_BE_DEAD;
		StartLevel(0);
		return;
	}
#endif

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		multi_do_death(Players[Player_num].objnum);
	}
	else
#endif		
	{				//Note link to above else!
		Players[Player_num].lives--;
		if (Players[Player_num].lives == 0)
		{
			DoGameOver();
			return;
		}
	}

	if (Fuelcen_control_center_destroyed) 
	{
		int	rval;

		Inter_secret_flag = false;
		Inter_died_in_mine = true;
		Pending_sub_mode = SUB_INTERMISSION;
		newmenu_close_all();

		//clear out stuff so no bonus
		Players[Player_num].hostages_on_board = 0;
		Players[Player_num].energy = 0;
		Players[Player_num].shields = 0;
		Players[Player_num].connected = 3;

		//init_player_stats_new_ship();
		last_drawn_cockpit = -1;

		inferno_request_fade_out(); //will be faded in by intermission code

		/*died_in_mine_message(); // Give them some indication of what happened

		if (Current_level_num == Last_level) 
		{
#ifdef NETWORK
			if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
			{
				multi_endlevel_score();
				rval = AdvanceLevel(0);			//if finished, go on to next level
			}
			else
#endif	
			{			// Note link to above else!
				rval = AdvanceLevel(0);			//if finished, go on to next level
				DoEndLevelScoreGlitz(0);
			}
			init_player_stats_new_ship();
			last_drawn_cockpit = -1;
		}
		else {
#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_endlevel_score();
			else
#endif
				DoEndLevelScoreGlitz(0);		// Note above link!

			rval = AdvanceLevel(0);			//if finished, go on to next level
			init_player_stats_new_ship();
			last_drawn_cockpit = -1;
		}

		if (rval)
		{
			if (Current_mission_num == 0)
				scores_maybe_add_player(0);
			longjmp(LeaveGame, 0);		// Exit out of game loop
		}*/
	}
	else 
	{
		init_player_stats_new_ship();
		StartLevel(1);
	}

}

void init_cockpit();

//called when the player is starting a new level for normal game mode and restore state
void StartNewLevelSub(int level_num, int page_in_textures)
{
	newmenu_close_all();
	if (!(Game_mode & GM_MULTI)) 
	{
		last_drawn_cockpit = -1;
	}

	if (Newdemo_state == ND_STATE_PAUSED)
		Newdemo_state = ND_STATE_RECORDING;

	if (Newdemo_state == ND_STATE_RECORDING) 
	{
		newdemo_set_new_level(level_num);
		newdemo_record_start_frame(FrameCount, FrameTime);
	}

	Debug_objnum = -1;

	if (Game_mode & GM_MULTI)
		Function_mode = FMODE_MENU; // Cheap fix to prevent problems with errror dialogs in loadlevel.

	LoadLevel(level_num);

	if (page_in_textures)
	{
		piggy_load_level_data();
	}

	Assert(Current_level_num == level_num);	//make sure level set right

	gameseq_init_network_players(); // Initialize the Players array for 
											  // this level

#ifdef NETWORK
	if (Game_mode & GM_NETWORK)
	{
		if (network_level_sync()) // After calling this, Player_num is set
			return;
	}
#endif

	Assert(Function_mode == FMODE_GAME);

#ifndef NDEBUG
	mprintf((0, "Player_num = %d, N_players = %d.\n", Player_num, N_players)); // DEBUG
#endif

	HUD_clear_messages();
	automap_clear_visited();

#ifdef NETWORK
	if (Network_new_game == 1)
	{
		Network_new_game = 0;
		init_player_stats_new_ship();
	}
#endif
	init_player_stats_level();

#ifndef SHAREWARE
#ifdef NETWORK
	if ((Game_mode & GM_MULTI_COOP) && Network_rejoined)
	{
		int i;
		for (i = 0; i < N_players; i++)
			Players[i].flags |= Netgame.player_flags[i];
	}
#endif
#endif

	Viewer = &Objects[Players[Player_num].objnum];

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		multi_prep_level(); // Removes robots from level if necessary
		Network_recieved_objects = false; //no longer need this flag. 
	}
#endif

	gameseq_remove_unused_players();

	Game_suspended = 0;

	Fuelcen_control_center_destroyed = 0;

	init_cockpit();
	init_robots_for_level();
	init_ai_objects();
	init_morphs();
	init_all_matcens();
	reset_palette_add();
	Laser_init_for_level();

	if (!(Game_mode & GM_MULTI) && !Cheats_enabled)
		set_highest_level(Current_level_num);

	reset_special_effects();

#ifdef NETWORK
	if (Network_rejoined == 1)
	{
#ifndef NDEBUG
		mprintf((0, "Restarting - joining in random location.\n"));
#endif
		Network_rejoined = 0;
		StartLevel(1);
	}
	else
#endif
		StartLevel(0);		// Note link to above if!

	copy_defaults_to_robot_all();
	init_controlcen_for_level();

	//	Say player can use FLASH cheat to mark path to exit.
	Last_level_path_created = -1;
}

//called when the player is starting a new level for normal game model
void StartNewLevel(int level_num)
{
	Pending_level = level_num;
	if (!(Game_mode & GM_MULTI)) 
	{
		if (do_briefing_screens(level_num))
		{
			Pending_sub_mode = SUB_BRIEFING;
			return;
		}
	}
	Pending_sub_mode = SUB_GAME;
	//StartNewLevelSub(level_num, 1);
}

//initialize the player object position & orientation (at start of game, or new ship)
void InitPlayerPosition(int random)
{
	int NewPlayer;

	if (!((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))) // If not deathmatch
		NewPlayer = Player_num;
#ifdef NETWORK
	else if (random == 1)
	{
		int i, closest = -1, trys = 0;
		fix closest_dist = 0x7ffffff, dist;

		P_SRand(clock());

#ifndef NDEBUG
		if (NumNetPlayerPositions != MAX_NUM_NET_PLAYERS)
		{
			mprintf((1, "WARNING: There are only %d start positions!\n"));
			//Int3();
		}
#endif

		do {
			if (trys > 0)
			{
				mprintf((0, "Can't start in location %d because its too close to player %d\n", NewPlayer, closest));
			}
			trys++;
			NewPlayer = P_Rand() % NumNetPlayerPositions;

			closest = -1;
			closest_dist = 0x7fffffff;

			for (i = 0; i < N_players; i++) {
				if ((i != Player_num) && (Objects[Players[i].objnum].type == OBJ_PLAYER)) {
					dist = find_connected_distance(&Objects[Players[i].objnum].pos, Objects[Players[i].objnum].segnum, &Player_init[NewPlayer].pos, Player_init[NewPlayer].segnum, 5, WID_FLY_FLAG);
					//					mprintf((0, "Distance from start location %d to player %d is %f.\n", NewPlayer, i, f2fl(dist)));
					if ((dist < closest_dist) && (dist >= 0)) {
						closest_dist = dist;
						closest = i;
					}
				}
			}
			mprintf((0, "Closest from pos %d is %f to plr %d.\n", NewPlayer, f2fl(closest_dist), closest));
		} while ((closest_dist < i2f(10 * 20)) && (trys < MAX_NUM_NET_PLAYERS * 2));
	}
#endif
	else {
		mprintf((0, "Starting position is not being changed.\n"));
		goto done; // If deathmatch and not random, positions were already determined by sync packet
	}
	Assert(NewPlayer >= 0);
	Assert(NewPlayer < NumNetPlayerPositions);

	ConsoleObject->pos = Player_init[NewPlayer].pos;
	ConsoleObject->orient = Player_init[NewPlayer].orient;

	mprintf((0, "Re-starting in location %d of %d.\n", NewPlayer + 1, NumNetPlayerPositions));

	obj_relink(ConsoleObject - Objects, Player_init[NewPlayer].segnum);

done:
	reset_player_object();
	reset_cruise();
}

//	-----------------------------------------------------------------------------------------------------
//	Initialize default parameters for one robot, copying from Robot_info to *objp.
//	What about setting size!?  Where does that come from?
void copy_defaults_to_robot(object* objp)
{
	robot_info* robptr;
	int			objid;

	Assert(objp->type == OBJ_ROBOT);
	objid = objp->id;
	Assert(objid < N_robot_types);

	robptr = &Robot_info[objid];

	objp->shields = robptr->strength;

}

//	-----------------------------------------------------------------------------------------------------
//	Copy all values from the robot info structure to all instances of robots.
//	This allows us to change bitmaps.tbl and have these changes manifested in existing robots.
//	This function should be called at level load time.
void copy_defaults_to_robot_all(void)
{
	int	i;

	for (i = 0; i <= Highest_object_index; i++)
		if (Objects[i].type == OBJ_ROBOT)
			copy_defaults_to_robot(&Objects[i]);
}

int	Do_appearance_effect = 0;

extern int Rear_view;

//	-----------------------------------------------------------------------------------------------------
//called when the player is starting a level (new game or new ship)
void StartLevel(int random)
{
	Assert(!Player_is_dead);

	InitPlayerPosition(random);

	verify_console_object();

	ConsoleObject->control_type = CT_FLYING;
	ConsoleObject->movement_type = MT_PHYSICS;

	disable_matcens();

	clear_transient_objects(0);		//0 means leave proximity bombs

	// create_player_appearance_effect(ConsoleObject);
	Do_appearance_effect = 1;

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
#ifndef SHAREWARE
		if (Game_mode & GM_MULTI_COOP)
			multi_send_score();
#endif
		multi_send_position(Players[Player_num].objnum);
		multi_send_reappear();
	}

	if (Game_mode & GM_NETWORK)
		network_do_frame(1, 1);
#endif

	ai_reset_all_paths();
	ai_init_boss_for_ship();
	reset_time();

	reset_rear_view();
	Auto_fire_fusion_cannon_time = 0;
	Fusion_charge = 0;

	Robot_firing_enabled = true;
}

void StartIntermission();
void FinishIntermission();

void StartPendingMode()
{
	//Do any cleanup that needs to be done.
	switch (Game_sub_mode)
	{
	case SUB_INTERMISSION:
		FinishIntermission();
		break;
	}
	Game_sub_mode = Pending_sub_mode;
	Pending_sub_mode = SUB_INDETERMINATE;

	switch (Game_sub_mode)
	{
	case SUB_BRIEFING:
		//Set pending mode based on whether or not the player has won the game.
		if (Current_level_num == Last_level)
			Pending_sub_mode = SUB_INTERMISSION;
		else
			Pending_sub_mode = SUB_GAME;
		break;
	case SUB_INTERMISSION:
		StartIntermission();
		break;
	case SUB_GAME:
		//When switching to SUB_GAME, load a level alongside it
		if (Pending_level != 0) //if 0 is pending, Level is already loaded from savegame or editor. Or bug. 
		{
			StartNewLevelSub(Pending_level, 1);
			Pending_level = 0;
		}
		break;
	}
}

bool SecretLevelDialogCallback(int choice, int nitems, newmenu_item* item)
{
	DoEndLevelScoreGlitz(0);
	return false;
}

void StartIntermission()
{
	IntermissionStarted = true;
	if (!(Game_mode & GM_MULTI) && Inter_secret_flag)
	{
		newmenu_item	m[1];

		m[0].type = NM_TYPE_TEXT;
		m[0].text = (char*)" ";			//TXT_SECRET_EXIT;

		newmenu_open(nullptr, TXT_SECRET_EXIT, 1, m, nullptr, SecretLevelDialogCallback, 0, "MENU.PCX");
	}
	else if (!(Game_mode & GM_MULTI) && Inter_died_in_mine)
	{
		died_in_mine_message();
	}
	else //If you did go to the secret level or died, the callback will start the intermission screen
	{
		DoEndLevelScoreGlitz(0);
	}
}

bool IntermissionFinished()
{
	/*if (IntermissionStarted) //this hack may not be needed anymore, new solution implemented
		return inferno_is_screen_faded();

	return false;*/
	return IntermissionStarted == false && inferno_is_screen_faded();
}

void IntermissionFrame()
{
	if (inferno_is_screen_faded())
		inferno_request_fade_in(gr_palette);
}

void FinishIntermission()
{
	IntermissionStarted = false;
	//Now it's safe to advance the level
	AdvanceLevel(Inter_secret_flag);

	if (Inter_died_in_mine)
	{
		init_player_stats_new_ship();
		Inter_died_in_mine = false;
	}

	newmenu_close_all(); //Close the window that serves as the intermission screen. 
}

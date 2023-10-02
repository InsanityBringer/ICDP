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

#include "player.h"
#include "mission.h"

#ifdef SHAREWARE
#define Last_level
#define Last_level			7			//the number of the very last level for shareware
#define Last_secret_level	0 			// No secret levels!
#else
extern int Last_level, Last_secret_level;	//set by mission code
#endif

extern int Secret_level_table[MAX_SECRET_LEVELS_PER_MISSION];

#define LEVEL_NAME_LEN 36		//make sure this is multipe of 4!

//Current_level_num starts at 1 for the first level
//-1,-2,-3 are secret levels
//0 means not a real level loaded
extern int Current_level_num, Next_level_num;
extern char Current_level_name[];
extern obj_position	Player_init[MAX_PLAYERS];

//Sub modes for the game frame. 
//Not doing anything. Bad if you're in this state without something pending. 
constexpr int SUB_INDETERMINATE = -1;
//Run the game ticker each frame. 
constexpr int SUB_GAME = 0;
//Run a briefing frame each frame.
constexpr int SUB_BRIEFING = 1;
//Between levels, showing the intermission screen. 
constexpr int SUB_INTERMISSION = 2;
//Quit out of the game and go into FMODE_MENU.
constexpr int SUB_ABORT = 3;
//Waiting for sync in a multiplayer game.
constexpr int SUB_PENDING = 4;

//The current sub mode, driving what will happen each time game_frame is called.
extern int Game_sub_mode;
//A pending sub mode, will be switched to when the current sub mode indicates completion
extern int Pending_sub_mode;


//This is the highest level the player has ever reached
extern int Player_highest_level;

//
//  New game sequencing functions
//

//called once at program startup to get the player's name
int RegisterPlayer();

//starts a new game on the given level
void StartNewGame(int start_level);

//starts the next level
void StartNewLevel(int level_num);

//When the current mode is done, start the pending mode.
void StartPendingMode();

//Returns true if the intermission screen is finished. 
bool IntermissionFinished();

//Runs a frame of the intermission screen.
void IntermissionFrame();

// Actually does the work to start new level
void StartNewLevelSub(int level_num, int page_in_textures);

void InitPlayerObject();				//make sure player's object set up
void init_player_stats_game();		//clear all stats

//called when the player has finished a level
//if secret flag is true, advance to secret level, else next normal level
void PlayerFinishedLevel(int secret_flag);

//called when the player has died
void DoPlayerDead();

//load a level off disk. level numbers start at 1.  
//Secret levels are -1,-2,-3
void LoadLevel(int level_num);

extern void gameseq_remove_unused_players();

extern void show_help();
extern void update_player_stats();

//from scores.c
extern void init_player_stats_level();

//create flash for player appearance
extern void create_player_appearance_effect(object* player_obj);

//reset stuff so game is semi-normal when playing from editor
void editor_reset_stuff_on_level();

//Show endlevel bonus scores
extern void DoEndLevelScoreGlitz(int network);

//stuff for multiplayer
extern int MaxNumNetPlayers;
extern int NumNetPlayerPositions;

void init_player_stats_new_ship();
void copy_defaults_to_robot_all(void);
void StartLevel(int random);

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

#include <csetjmp>

#include "2d/gr.h"
#include "vecmat/vecmat.h"
#include "object.h"
#include "misc/types.h"
#include "main_shared/game_shared.h"

//constants for ft_preference
#define FP_RIGHT		0
#define FP_UP			1
#define FP_FORWARD	2		//this is the default
#define FP_LEFT		3
#define FP_DOWN		4
#define FP_FIRST_TIME	5

extern int ft_preference;

//	The following bits define the game modes.
#define GM_EDITOR						1			//	You came into the game from the editor
//#define GM_SERIAL						2			// You are in serial mode. Unused in Chocolate. 
#define GM_NETWORK					4			// You are in network mode
#define GM_MULTI_ROBOTS				8			//	You are in a multiplayer mode with robots.
#define GM_MULTI_COOP				16			//	You are in a multiplayer mode and can't hurt other players.
//#define GM_MODEM						32       // You are in a modem (serial) game. Unused in Chocolate.

#define GM_UNKNOWN					64			//	You are not in any mode, kind of dangerous...
#define GM_GAME_OVER					128		//	Game has been finished

#define GM_TEAM						256		// Team mode for network play

#define GM_NORMAL						0			//	You are in normal play mode, no multiplayer stuff
#define GM_MULTI						38			//	You are in some type of multiplayer game

//	Examples:
//	Deathmatch mode on a network is GM_NETWORK
//	Deathmatch mode via modem with robots is GM_MODEM | GM_MULTI_ROBOTS
// Cooperative mode via serial link is GM_SERIAL | GM_MULTI_COOP

#define	NDL	5		//	Number of difficulty levels.
#define	NUM_DETAIL_LEVELS	6

extern float Game_aspect;
//The difference of the viewport's aspect ratio vs the box it is expected to fit. Used to keep rendering square. 
extern float Game_render_aspect;

extern int Game_mode;

extern bool Game_paused;
extern int gauge_message_on;

extern grs_bitmap background_bitmap; //Used by gauges to draw the background

#ifndef NDEBUG		//if debugging, these are variables
extern int Slew_on;							//in slew or sim mode?
#else					//if not debugging, these are constants
#define Slew_on 				0		//no slewing in real game
#endif

//Suspend flags

#define SUSP_NONE			0			//Everything moving normally 
#define SUSP_ROBOTS		1			//Robot AI doesn't move
#define SUSP_WEAPONS		2			//Lasers, etc. don't move		

extern int Game_suspended;			//if non-zero, nothing moves but player

extern int FPSLimit; //[ISB] timer controls

//from game.c

void init_game(void);

//Performs all the initialization needed to start a game. 
void game_start();

//Runs a single frame of the game
void game_frame();

void game_present();

//Cleans up the game's state before switching to another game.  
void game_end();

void close_game(void);

void calc_frame_time(void);

//Returns the amount of microseconds to spend between frames.
uint64_t game_fps_limit_time();

#ifdef EDITOR
void do_flythrough(object* obj, int first_time);
void init_cockpit();
#endif

extern jmp_buf LeaveGame;					// Do a long jump to this when game is over.
extern int	Difficulty_level;				//	Difficulty level in 0..NDL-1, 0 = easiest, NDL-1 = hardest
extern int	Detail_level;					//	Detail level in 0..NUM_DETAIL_LEVELS-1, 0 = boringest, NUM_DETAIL_LEVELS = coolest
extern int	Global_laser_firing_count;
extern int	Global_missile_firing_count;
extern int	Render_depth;
extern fix	Auto_fire_fusion_cannon_time, Fusion_charge;

extern int PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd;

#define	MAX_PALETTE_ADD	30

//adds to rgb values for palette flash
#define PALETTE_FLASH_ADD(_dr,_dg,_db) 						\
	do {																	\
		if ((PaletteRedAdd+=(_dr)) > MAX_PALETTE_ADD)		\
			PaletteRedAdd = MAX_PALETTE_ADD;						\
		if ((PaletteGreenAdd+=(_dg)) > MAX_PALETTE_ADD)		\
			PaletteGreenAdd = MAX_PALETTE_ADD;					\
		if ((PaletteBlueAdd+=(_db)) > MAX_PALETTE_ADD)		\
			PaletteBlueAdd = MAX_PALETTE_ADD;					\
		if (PaletteRedAdd < -MAX_PALETTE_ADD)					\
			PaletteRedAdd = -MAX_PALETTE_ADD;					\
		if (PaletteGreenAdd < -MAX_PALETTE_ADD)				\
			PaletteGreenAdd = -MAX_PALETTE_ADD;					\
		if (PaletteBlueAdd < -MAX_PALETTE_ADD)					\
			PaletteBlueAdd = -MAX_PALETTE_ADD;					\
	} while (0)

//sets the rgb values for palette flash
#define PALETTE_FLASH_SET(_r,_g,_b) PaletteRedAdd=(_r), PaletteGreenAdd=(_g), PaletteBlueAdd=(_b)

extern int draw_gauges_on;

//extern void init_game_screen(void); //[ISB] cut from current game.c

extern void game_flush_inputs();		// clear all inputs

extern int Playing_game;		// True if playing game
extern int Auto_flythrough;	//if set, start flythough automatically
extern int Mark_count;			// number of debugging marks set
extern char faded_in;

extern void stop_time(void);
extern void start_time(void);
extern void reset_time(void);		//called when starting level

void game_pause(bool paused);

//	If automap_flag == 1, then call automap routine to write message.
extern void save_screen_shot(int automap_flag);

extern grs_canvas* get_current_game_screen();

//valid modes for cockpit
#define CM_FULL_COCKPIT 	0	//normal screen with cockput
#define CM_REAR_VIEW		1	//looking back with bitmap
#define CM_STATUS_BAR		2	//small status bar, w/ reticle
#define CM_FULL_SCREEN		3	//full screen, no cockpit (w/ reticle)
#define CM_LETTERBOX		4	//half-height window (for cutscenes)

extern int Cockpit_mode;		//what sort of cockpit or window is up?
extern int Game_window_w, Game_window_h;		//width and height of player's game window

extern int Rear_view;			//if true, looking back.

//selects a given cockpit (or lack of one).
void select_cockpit(int mode);

//force cockpit redraw next time. call this if you've trashed the screen
void reset_cockpit(void);		//called if you've trashed the screen

//functions to save, clear, and resture palette flash effects
void palette_save(void);
void reset_palette_add(void);
void palette_restore(void);

//put up the help message
void do_show_help();

//show a message in a nice little box
void show_boxed_message(char* msg);

//erases message drawn with show_boxed_message()
void clear_boxed_message();

//turns off rear view & rear view cockpit
void reset_rear_view(void);

//Returns true if the mouse should be captured while fmode is FMODE_GAME, false if not.
bool game_should_capture_mouse(void);

#define VR_NONE			0	//viewing the game screen
#define VR_AREA_DET		1	//viewing with the stereo area determined method
#define VR_INTERLACED	2	//viewing with the stereo interlaced method

extern int			VR_screen_mode;
extern int			VR_render_width;
extern int			VR_render_height;
extern int			VR_compatible_menus;
extern grs_canvas* VR_offscreen_buffer;		// The offscreen data buffer
extern grs_canvas	VR_render_buffer;					//  Two offscreen buffers for left/right eyes.
extern grs_canvas	VR_render_sub_buffer;			//  Two sub buffers for left/right eyes.
extern grs_canvas*	VR_screen_buffer;

extern int cfg_render_width, cfg_render_height;
extern int cfg_aspect_ratio;

constexpr int GAMEASPECT_AUTO = 0;
constexpr int GAMEASPECT_4_3 = 1;
constexpr int GAMEASPECT_5_4 = 2;
constexpr int GAMEASPECT_16_10 = 3;
constexpr int GAMEASPECT_16_9 = 4;
constexpr int GAMEASPECT_21_9 = 5;
constexpr int GAMEASPECT_COUNT = 6;

void game_init_render_buffers(int screen_mode, int render_max_w, int render_max_h, int compatible_menus);

void fill_background(int x, int y, int w, int h, int dx, int dy);
void game_init_render_sub_buffers(int x, int y, int w, int h);
void GameLoop(bool RenderFlag, bool ReadControlsFlag);
void powerup_grab_cheat_all(void);

//New cheat input stuff
void init_cheats();
void do_cheat_key(int keycode);

//Cheat flags
extern bool Ugly_robot_cheat;
extern int Ugly_robot_texture;
extern bool Laser_rapid_fire;
extern bool Physics_cheat_flag;
extern bool Game_turbo_mode;

int create_special_path();
extern bool framerate_on;

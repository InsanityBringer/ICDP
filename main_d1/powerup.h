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

#include "object.h"
#include "vclip.h"
#include "fix/fix.h"

#define	POW_EXTRA_LIFE 				0
#define	POW_ENERGY					1
#define	POW_SHIELD_BOOST			2
#define	POW_LASER					3

#define	POW_KEY_BLUE				4
#define	POW_KEY_RED					5
#define	POW_KEY_GOLD				6

#define	POW_RADAR_ROBOTS			7
#define	POW_RADAR_POWERUPS			8
#define	POW_FULL_MAP				9

#define	POW_MISSILE_1				10
#define	POW_MISSILE_4				11

#define	POW_QUAD_FIRE				12
#define	POW_VULCAN_WEAPON			13
#define	POW_SPREADFIRE_WEAPON		14
#define	POW_PLASMA_WEAPON			15
#define	POW_FUSION_WEAPON			16
#define	POW_PROXIMITY_WEAPON		17
#define	POW_SMARTBOMB_WEAPON		20
#define	POW_MEGA_WEAPON				21
#define	POW_VULCAN_AMMO				22
#define	POW_HOMING_AMMO_1			18
#define	POW_HOMING_AMMO_4			19
#define	POW_CLOAK					23
#define	POW_TURBO					24
#define	POW_INVULNERABILITY			25
#define	POW_HEADLIGHT				26
#define	POW_MEGAWOW					27

#define	VULCAN_AMMO_MAX				(392*2)
#define	VULCAN_WEAPON_AMMO_AMOUNT	196
#define	VULCAN_AMMO_AMOUNT			(49*2)

constexpr int POW_CLASS_UNCLASSIFIED	= 0;
constexpr int POW_CLASS_PRIMARY			= 1;
constexpr int POW_CLASS_SECONDARY		= 2;
constexpr int POW_CLASS_KEY				= 3;

#define MAX_POWERUP_TYPES			29

#define	POWERUP_NAME_LENGTH	16		//	Length of a robot or powerup name.
extern char	Powerup_names[MAX_POWERUP_TYPES][POWERUP_NAME_LENGTH];

struct powerup_type_info
{
	int	vclip_num;
	int	hit_sound;
	fix	size;			//3d size of longest dimension
	fix	light;		//	amount of light cast by this powerup, set in bitmaps.tbl

	//New keys for generalizing features..
	int multi_replacement; //Replace this powerup with another in multiplayer games. -1 if not active. 
	int anarchy_limit; //Amount of this powerup that can be present in an anarchy game. 0 is unlimited.
	int powerup_class; //Used to determine the class of this powerup for things like duplication. 
};

extern int N_powerup_types;
extern powerup_type_info Powerup_info[MAX_POWERUP_TYPES];

//Initializes all the powerups. Call before loading descent.pig or bitmaps.tbl.
void powerup_init_all();

void draw_powerup(object* obj);

//returns true if powerup consumed
bool do_powerup(object* obj);

//process (animate) a powerup for one frame
void do_powerup_frame(object* obj);

extern void do_megawow_powerup(int quantity);

extern void powerup_basic(int redadd, int greenadd, int blueadd, int score, const char* format, ...);

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

#include "fix/fix.h"
#include "vecmat/vecmat.h"
#include "object.h"

#define	CONCUSSION_ID			8
#define	FLARE_ID				9			//	NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
#define	LASER_ID				10
#define	VULCAN_ID				11			//	NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
#define	XSPREADFIRE_ID			12			//	NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
#define	PLASMA_ID				13			//	NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
#define	FUSION_ID				14			//	NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
#define	HOMING_ID				15
#define	PROXIMITY_ID			16
#define	SMART_ID				17
#define	MEGA_ID					18
#define	PLAYER_SMART_HOMING_ID	19
#define	SPREADFIRE_ID			20
#define	SUPER_MECH_MISS			21
#define	REGULAR_MECH_MISS		22
#define	SILENT_SPREADFIRE_ID	23
#define	ROBOT_SMART_HOMING_ID	29

 // These are new defines for the value of 'flags' passed to do_laser_firing.
 // The purpose is to collect other flags like QUAD_LASER and Spreadfire_toggle
 // into a single 8-bit quantity so it can be easily used in network mode.

#define LASER_QUAD 1
#define LASER_SPREADFIRE_TOGGLED 2

#define MAX_LASER_LEVEL		3		//	Note, laser levels are numbered from 0.
#define MAX_LASER_BITMAPS	6

//	For muzzle firing casting light.
#define	MUZZLE_QUEUE_MAX	8

//	Constants governing homing missile behavior.
//	MIN_TRACKABLE_DOT gets inversely scaled by FrameTime and stuffed in Min_trackable_dot
#define	MIN_TRACKABLE_DOT				(3*F1_0/4)
#define	MAX_TRACKABLE_DIST				(F1_0*250)
#define	HOMING_MISSILE_STRAIGHT_TIME	(F1_0/8)			//	Changed as per request of John, Adam, Yuan, but mostly John

extern fix Min_trackable_dot;				//	MIN_TRACKABLE_DOT inversely scaled by FrameTime

//Resets laser parameters for a new game.
void Laser_init_for_level();

void Laser_render(object* obj);
void Laser_player_fire(object* obj, int type, int gun_num, int make_sound, int harmless_flag);
void Laser_player_fire_spread(object* obj, int laser_type, int gun_num, fix spreadr, fix spreadu, int make_sound, int harmless);
void Laser_do_weapon_sequence(object* obj);
void Flare_create(object* obj);
int laser_are_related(int o1, int o2);

extern int do_laser_firing_player(void);
extern void do_missile_firing(void);
extern void net_missile_firing(int player, int weapon, int flags);

int Laser_create_new(vms_vector* direction, vms_vector* position, int segnum, int parent, int type, int make_sound);

//	Fires a laser-type weapon (a Primary weapon)
//	Fires from object objnum, weapon type weapon_id.
//	Assumes that it is firing from a player object, so it knows which gun to fire from.
//	Returns the number of shots actually fired, which will always be 1.
extern int do_laser_firing(int objnum, int weapon_id, int level, int flags);

//	Easier to call than Laser_create_new because it determines the segment containing the firing point
//	and deals with it being stuck in an object or through a wall.
//	Fires a laser of type "weapon_type" from an object (parent) in the direction "direction" from the position "position"
//	Returns object number of laser fired or -1 if not possible to fire laser.
int Laser_create_new_easy(vms_vector* direction, vms_vector* position, int parent, int weapon_type, int make_sound);

extern void create_smart_children(object* objp);
extern int object_to_object_visibility(object* obj1, object* obj2, int trans_type);

extern int Muzzle_queue_index;

struct muzzle_info 
{
	fix			create_time;
	short			segnum;
	vms_vector	pos;
};

extern muzzle_info Muzzle_data[MUZZLE_QUEUE_MAX];

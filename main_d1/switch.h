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

#include "inferno.h"
#include "segment.h"

#define	MAX_WALLS_PER_LINK		10

#define	TRIGGER_DEFAULT			2*F1_0

#define	MAX_TRIGGERS				100
#define	MAX_WALL_SWITCHES			50
#define	MAX_WALL_LINKS				100

 // Trigger flags	  
#define	TRIGGER_CONTROL_DOORS		1	// Control Trigger
#define	TRIGGER_SHIELD_DAMAGE		2	// Shield Damage Trigger
#define	TRIGGER_ENERGY_DRAIN	   	4	// Energy Drain Trigger
#define	TRIGGER_EXIT					8	// End of level Trigger
#define	TRIGGER_ON					  16	// Whether Trigger is active
#define	TRIGGER_ONE_SHOT			  32	// If Trigger can only be triggered once
#define	TRIGGER_MATCEN				  64	// Trigger for materialization centers
#define	TRIGGER_ILLUSION_OFF		 128	// Switch Illusion OFF trigger
#define	TRIGGER_ILLUSION_ON		 512	// Switch Illusion ON trigger
#define	TRIGGER_SECRET_EXIT		 256	// Exit to secret level

// Trigger delay times before they can be retriggered (Recharge time)
#define	TRIGGER_DELAY_DOOR		F1_0*1	// 1 second for doors
#define	TRIGGER_DELAY_ZAPS		F1_0/10	// 1/10 second for quickie stuff

// New unimplemented trigger ideas 
#define	TRIGGER_CONTROL_ROBOTS	  64	// If Trigger is a Door control trigger (Linked)
#define	CONTROL_ROBOTS					8	// If Trigger modifies robot behavior
#define	CONTROL_LIGHTS_ON			  16	// If Trigger turns on lights in a certain area
#define	CONTROL_LIGHTS_OFF		  32	// If Trigger turns off lights in a certain area

struct trigger
{
	int8_t		type;
	short		flags;
	fix		value;
	fix		time;
	int8_t		link_num;
	short 	num_links;
	short 	seg[MAX_WALLS_PER_LINK];
	short		side[MAX_WALLS_PER_LINK];
};

extern trigger Triggers[MAX_TRIGGERS];

extern int Num_triggers;

extern void trigger_init();

extern void check_trigger(segment* seg, short side, short objnum);
extern bool check_trigger_sub(int trigger_num, int player_num);

extern void triggers_frame_process();

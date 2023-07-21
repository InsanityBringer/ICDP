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
#include <math.h>
#include <string.h>
#include "gauges.h"
#include "game.h"
#include "switch.h"
#include "inferno.h"
#ifdef EDITOR
#include "editor\editor.h"
#endif
#include "segment.h"
#include "misc/error.h"
#include "gameseg.h"
#include "platform/mono.h"
#include "wall.h"
#include "fuelcen.h"
#include "newdemo.h"
#include "player.h"
#include "endlevel.h"
#include "gameseq.h"
#include "multi.h"
#include "network.h"
#include "2d/palette.h"

trigger Triggers[MAX_TRIGGERS];
int Num_triggers;

//-----------------------------------------------------------------
// Initializes all the switches.
void trigger_init()
{
	Num_triggers = 0;

	for (int i = 0; i < MAX_TRIGGERS; i++)
	{
		Triggers[i].type = 0;
		Triggers[i].flags = 0;
		Triggers[i].value = 0;
		Triggers[i].link_num = -1;
		Triggers[i].time = -1;
	}
}

//-----------------------------------------------------------------
// Executes a link, attached to a trigger.
// Toggles all walls linked to the switch.
// Opens doors, Blasts blast walls, turns off illusions.
void do_link(int8_t trigger_num)
{
	mprintf((0, "Door link!\n"));

	if (trigger_num != -1) 
	{
		for (int i = 0; i < Triggers[trigger_num].num_links; i++) 
		{
			wall_toggle(&Segments[Triggers[trigger_num].seg[i]], Triggers[trigger_num].side[i]);
			mprintf((0, " trigger_num %d : seg %d, side %d\n",
				trigger_num, Triggers[trigger_num].seg[i], Triggers[trigger_num].side[i]));
		}
	}
}

void do_matcen(int8_t trigger_num)
{
	mprintf((0, "Matcen link!\n"));

	if (trigger_num != -1) 
	{
		for (int i = 0; i < Triggers[trigger_num].num_links; i++) 
		{
			trigger_matcen(Triggers[trigger_num].seg[i]);
			mprintf((0, " trigger_num %d : seg %d\n",
				trigger_num, Triggers[trigger_num].seg[i]));
		}
	}
}


void do_il_on(int8_t trigger_num)
{
	mprintf((0, "Illusion ON\n"));

	if (trigger_num != -1) 
	{
		for (int i = 0; i < Triggers[trigger_num].num_links; i++) 
		{
			wall_illusion_on(&Segments[Triggers[trigger_num].seg[i]], Triggers[trigger_num].side[i]);
			mprintf((0, " trigger_num %d : seg %d, side %d\n",
				trigger_num, Triggers[trigger_num].seg[i], Triggers[trigger_num].side[i]));
		}
	}
}

void do_il_off(int8_t trigger_num)
{
	mprintf((0, "Illusion OFF\n"));

	if (trigger_num != -1) 
	{
		for (int i = 0; i < Triggers[trigger_num].num_links; i++) 
		{
			wall_illusion_off(&Segments[Triggers[trigger_num].seg[i]], Triggers[trigger_num].side[i]);
			mprintf((0, " trigger_num %d : seg %d, side %d\n",
				trigger_num, Triggers[trigger_num].seg[i], Triggers[trigger_num].side[i]));
		}
	}
}

bool check_trigger_sub(int trigger_num, int pnum)
{
	if (pnum == Player_num) 
	{
		if (Triggers[trigger_num].flags & TRIGGER_SHIELD_DAMAGE) 
		{
			Players[Player_num].shields -= Triggers[trigger_num].value;
			mprintf((0, "BZZT!\n"));
		}

		if (Triggers[trigger_num].flags & TRIGGER_EXIT) 
		{
			start_endlevel_sequence();
			mprintf((0, "WOOHOO! (leaving the mine!)\n"));
		}

		if (Triggers[trigger_num].flags & TRIGGER_SECRET_EXIT) 
		{
			if (Newdemo_state == ND_STATE_RECORDING)		// stop demo recording
				Newdemo_state = ND_STATE_PAUSED;

			Fuelcen_control_center_destroyed = 0;
			mprintf((0, "Exiting to secret level\n"));
#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_endlevel_start(1);
#endif
#ifdef NETWORK
			if (Game_mode & GM_NETWORK)
				network_do_frame(1, 1);
#endif
			//gr_palette_fade_out(gr_palette, 32, 0); TODO FADE
			PlayerFinishedLevel(1);		//1 means go to secret level
			return true;
		}

		if (Triggers[trigger_num].flags & TRIGGER_ENERGY_DRAIN) 
		{
			Players[Player_num].energy -= Triggers[trigger_num].value;
			mprintf((0, "SLURP!\n"));
		}
	}

	if (Triggers[trigger_num].flags & TRIGGER_CONTROL_DOORS) 
	{
		mprintf((0, "D"));
		do_link(trigger_num);
	}

	if (Triggers[trigger_num].flags & TRIGGER_MATCEN) 
	{
		if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_ROBOTS))
			do_matcen(trigger_num);
	}

	if (Triggers[trigger_num].flags & TRIGGER_ILLUSION_ON) 
	{
		mprintf((0, "I"));
		do_il_on(trigger_num);
	}

	if (Triggers[trigger_num].flags & TRIGGER_ILLUSION_OFF) 
	{
		//		Triggers[trigger_num].time = TRIGGER_DELAY_DOOR;
		mprintf((0, "i"));
		do_il_off(trigger_num);
	}
	return false;
}

//-----------------------------------------------------------------
// Checks for a trigger whenever an object hits a trigger side.
void check_trigger(segment* seg, short side, short objnum)
{
	if (objnum == Players[Player_num].objnum) 
	{
		if (Newdemo_state == ND_STATE_PLAYBACK)
			return;

		int wall_num = seg->sides[side].wall_num;
		if (wall_num == -1) return;

		int trigger_num = Walls[wall_num].trigger;

		if (trigger_num == -1)
			return;

		if (check_trigger_sub(trigger_num, Player_num))
			return;

		if (Triggers[trigger_num].flags & TRIGGER_ONE_SHOT) 
		{
			Triggers[trigger_num].flags &= ~TRIGGER_ON;

			segment* csegp = &Segments[seg->children[side]];
			int cside = find_connect_side(seg, csegp);
			Assert(cside != -1);

			wall_num = csegp->sides[cside].wall_num;
			if (wall_num == -1) return;

			int ctrigger_num = Walls[wall_num].trigger;

			Triggers[ctrigger_num].flags &= ~TRIGGER_ON;
		}
#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_send_trigger(trigger_num);
#endif
	}
}

void triggers_frame_process()
{
	for (int i = 0; i < Num_triggers; i++)
		if (Triggers[i].time >= 0)
			Triggers[i].time -= FrameTime;
}

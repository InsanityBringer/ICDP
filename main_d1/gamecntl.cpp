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

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "inferno.h"
#include "platform/key.h"
#include "platform/mono.h"
#include "game.h"
#include "gameseq.h"
#include "misc/error.h"
#include "gauges.h"
#include "stringtable.h"
#include "powerup.h"
#include "player.h"
#include "newdemo.h"
#include "laser.h"
#include "newmenu.h"
#include "ai.h"
#include "cntrlcen.h"

enum class cheat_effect
{
	EnableCheats,
	AllWeaponsSW,
	AllWeapons,
	Invulnerability,
	Cloak,
	AllKeys,
	FullShield,
	LevelWarp,
	Immaterial,
	Turbo,
	ExtraLife,
	ShowExitPath,
	RobotFire,
	DestroyReactor,
	AllWeaponsSuper,
	Lunacy,
	RobotPainting, //needs special parsing
	Frametime,
	Acid
};

struct cheat_code
{
	const char* input_string;
	cheat_effect effect;
	bool informational; //If true, can be used in multiplayer and won't trigger cheater. 
	bool requires_enable; //If true, cheat needs to have been initialized with "gabbagabbahey"
};

//I kinda wish there was a fixed-size container in C++ who's size was inferred by the initializer.  
//std::array is good but the size is fixed by the template, and I want to grow this without worrying about that.
cheat_code all_cheat_codes[] =
{
	{"gabbagabbahey", cheat_effect::EnableCheats, false, false},
	{"scourge", cheat_effect::AllWeaponsSW, false, true},
	{"bigred", cheat_effect::AllWeapons, false, true},
	{"racerx", cheat_effect::Invulnerability, false, true},
	{"guile", cheat_effect::Cloak, false, true},
	{"mitzi", cheat_effect::AllKeys, false, true},
	{"twilight", cheat_effect::FullShield, false, true},
	{"farmerjoe", cheat_effect::LevelWarp, false, true},
	{"astral", cheat_effect::Immaterial, false, true},
	{"buggin", cheat_effect::Turbo, false, true},
	{"bruin", cheat_effect::ExtraLife, false, true},
	{"flash", cheat_effect::ShowExitPath, false, true},
	{"ahimsa", cheat_effect::RobotFire, false, true},
	{"poboys", cheat_effect::DestroyReactor, false, true},
	{"porgys", cheat_effect::AllWeaponsSuper, false, true},
	{"lunacy", cheat_effect::Lunacy, false, true},
	{"pletchxxx", cheat_effect::RobotPainting, false, true},
	{"frametime", cheat_effect::Frametime, true, false},
};

#define NUM_CHEAT_CODES sizeof(all_cheat_codes) / sizeof(all_cheat_codes[0])

int cheat_buffer_size;

char* cheat_input_buffer;

void init_cheats()
{
	if (!cheat_input_buffer)
	{
		int buffer_len = 0;
		for (int i = 0; i < NUM_CHEAT_CODES; i++)
		{
			int len = strlen(all_cheat_codes[i].input_string);
			if (len > buffer_len)
				buffer_len = len;
		}

		cheat_buffer_size = buffer_len;
		cheat_input_buffer = (char*)malloc(cheat_buffer_size * sizeof(char));

		if (!cheat_input_buffer)
			Error("init_cheats: why did a 13 byte allocation fail?");
	}
}

void do_pletch(int pletch_num)
{
	if (!Cheats_enabled)
		return; 

	Ugly_robot_texture = pletch_num;

	if (Ugly_robot_texture == 999)
	{
		Ugly_robot_cheat = false;
		HUD_init_message(TXT_ROBOT_PAINTING_OFF);
	}
	else
	{
		HUD_init_message(TXT_ROBOT_PAINTING_ON, Ugly_robot_texture);
		Ugly_robot_cheat = true;
	}
	mprintf((0, "Paint value = %i\n", Ugly_robot_texture));

	Cheats_enabled = 1;
	digi_play_sample(SOUND_CHEATER, F1_0);
	Players[Player_num].score = 0;
}

void do_generic_cheat(cheat_code& code)
{
	if (code.requires_enable && !Cheats_enabled || !code.informational && (Game_mode & GM_MULTI) || !code.informational && Newdemo_state == ND_STATE_PLAYBACK)
		return;

	switch (code.effect)
	{
	case cheat_effect::EnableCheats:
		HUD_init_message(TXT_CHEATS_ENABLED);
		break;
	case cheat_effect::AllWeaponsSW:
	case cheat_effect::AllWeapons:
		if (code.effect == cheat_effect::AllWeaponsSW)
			HUD_init_message(TXT_WOWIE_ZOWIE);
		else
			HUD_init_message("SUPER %s", TXT_WOWIE_ZOWIE);

		Players[Player_num].primary_weapon_flags |= 0xff;
		Players[Player_num].secondary_weapon_flags |= 0xff;

		if (code.effect == cheat_effect::AllWeaponsSW)
		{
			Players[Player_num].primary_weapon_flags ^= (HAS_PLASMA_FLAG | HAS_FUSION_FLAG);
			Players[Player_num].secondary_weapon_flags ^= (HAS_SMART_FLAG | HAS_MEGA_FLAG);
		}

		Players[Player_num].primary_ammo[VULCAN_INDEX] = Primary_ammo_max[VULCAN_INDEX];

		for (int i = 0; i < 5; i++)
			Players[Player_num].secondary_ammo[i] = Secondary_ammo_max[i];

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_laser_level(Players[Player_num].laser_level, MAX_LASER_LEVEL);

		Players[Player_num].energy = MAX_ENERGY;
		Players[Player_num].laser_level = MAX_LASER_LEVEL;
		Players[Player_num].flags |= PLAYER_FLAGS_QUAD_LASERS;
		update_laser_weapon_info();
		break;
	case cheat_effect::Invulnerability:
		Players[Player_num].flags ^= PLAYER_FLAGS_INVULNERABLE;
		HUD_init_message("%s %s!", TXT_INVULNERABILITY, (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE) ? TXT_ON : TXT_OFF);
		Players[Player_num].invulnerable_time = GameTime + i2f(1000);
		break;
	case cheat_effect::Cloak:
		Players[Player_num].flags ^= PLAYER_FLAGS_CLOAKED;
		HUD_init_message("%s %s!", TXT_CLOAK, (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) ? TXT_ON : TXT_OFF);
		if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)
		{
			ai_do_cloak_stuff();
			Players[Player_num].cloak_time = GameTime;
		}
		break;
	case cheat_effect::AllKeys:
		HUD_init_message(TXT_ALL_KEYS);
		Players[Player_num].flags |= PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY;
		break;
	case cheat_effect::FullShield:
		HUD_init_message(TXT_FULL_SHIELDS);
		Players[Player_num].shields = MAX_SHIELDS;
		break;
	case cheat_effect::LevelWarp:
	{
		newmenu_item m;
		char text[10] = "";
		m.type = NM_TYPE_INPUT; m.text_len = 9; m.text = text;
		int item = newmenu_do(NULL, TXT_WARP_TO_LEVEL, 1, &m, NULL);
		if (item != -1) 
		{
			int new_level_num = atoi(m.text);
			if (new_level_num != 0 && new_level_num >= 0 && new_level_num <= Last_level)
				StartNewLevel(new_level_num);
		}
	}
		break;

	case cheat_effect::Immaterial:
		Physics_cheat_flag = !Physics_cheat_flag;
		HUD_init_message("%s %s!", "Ghosty mode", Physics_cheat_flag ? TXT_ON : TXT_OFF);
		break;
	case cheat_effect::Turbo:
		Game_turbo_mode = !Game_turbo_mode;
		HUD_init_message("%s %s!", "Turbo mode", Game_turbo_mode ? TXT_ON : TXT_OFF);
		break;
	case cheat_effect::ExtraLife:
		if (Players[Player_num].lives < 50)
		{
			Players[Player_num].lives++;
			HUD_init_message("Extra life!");
		}
		break;
	case cheat_effect::ShowExitPath:
		if (create_special_path())
			HUD_init_message("Exit path illuminated!");
		break;
	case cheat_effect::RobotFire:
		Robot_firing_enabled = !Robot_firing_enabled;
		HUD_init_message("%s %s!", "Robot firing", Robot_firing_enabled ? TXT_ON : TXT_OFF);
		break;
	case cheat_effect::DestroyReactor:
		do_controlcen_destroyed_stuff(nullptr);
		break;
	case cheat_effect::AllWeaponsSuper:
		Laser_rapid_fire = true;
		do_megawow_powerup(200);
		break;
	case cheat_effect::Lunacy:
		if (Lunacy)
		{
			do_lunacy_off();
			HUD_init_message(TXT_NO_LUNACY);
			return;
		}
		else
		{
			do_lunacy_on();
			HUD_init_message(TXT_LUNACY);
		}
		break;
	case cheat_effect::Frametime:
		framerate_on = !framerate_on;
		break;
	}

	//Do penalty for non-informational cheats
	if (!code.informational)
	{
		Cheats_enabled = 1; //This is done redundantly for supporting Descent 2 codes.
		digi_play_sample(SOUND_CHEATER, F1_0);
		Players[Player_num].score = 0;
	}
}

void do_cheat_key(int keycode)
{
	int ch = key_to_ascii(keycode);
	if (ch == 255) return; //non-ascii input

	//Shift the buffer backwards
	for (int i = 0; i < cheat_buffer_size - 1; i++)
		cheat_input_buffer[i] = cheat_input_buffer[i + 1];

	//Add the current character
	cheat_input_buffer[cheat_buffer_size - 1] = ch;

	//Now see if there's a match
	for (int i = 0; i < NUM_CHEAT_CODES; i++)
	{
		//Robot painting needs special parsing
		if (all_cheat_codes[i].effect == cheat_effect::RobotPainting)
		{
			size_t len = strlen(all_cheat_codes[i].input_string);
			if (!strnicmp(&cheat_input_buffer[cheat_buffer_size - len], all_cheat_codes[i].input_string, len - 3)) //pletch matched, now check the number
			{
				if (isdigit(cheat_input_buffer[cheat_buffer_size - 1]) && isdigit(cheat_input_buffer[cheat_buffer_size - 2]) && isdigit(cheat_input_buffer[cheat_buffer_size - 3]))
				{
					do_pletch(atoi(&cheat_input_buffer[cheat_buffer_size - 3]));
					return;
				}
			}
		}
		else
		{
			size_t len = strlen(all_cheat_codes[i].input_string);
			if (!strnicmp(&cheat_input_buffer[cheat_buffer_size - len], all_cheat_codes[i].input_string, len))
			{
				do_generic_cheat(all_cheat_codes[i]);
				return;
			}
		}
	}
}

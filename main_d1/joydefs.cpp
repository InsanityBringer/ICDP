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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "platform/mono.h"
#include "platform/key.h"
#include "platform/joy.h"
#include "platform/timer.h"
#include "misc/error.h"

#include "inferno.h"
#include "game.h"
#include "object.h"
#include "player.h"

#include "controls.h"
#include "joydefs.h"
#include "render.h"
#include "2d/palette.h"
#include "newmenu.h"
#include "misc/args.h"
#include "stringtable.h"
#include "kconfig.h"
#include "main_shared/digi.h"
#include "playsave.h"

extern int WriteConfigFile();

const char *control_text[CONTROL_MAX_TYPES] = { "Keyboard only", "Joystick (1-2)", "Gamepad", "Joystick w/ throttle", "-", "Mouse", "-"};
int choco_menu_remap[CONTROL_MAX_TYPES] = { 0, 1, 2, 3, 5, 0, 0 }; //Remaps the new options to the old input ID
int choco_id_to_menu_remap[CONTROL_MAX_TYPES] = { 0, 1, 2, 3, 0, 4, 0 }; //Remaps an old ID to the new menu option

void joydef_menuset_1(int nitems, newmenu_item* items, int* last_key, int citem)
{
	int i;
	int oc_type = Config_control_type;

	nitems = nitems;
	last_key = last_key;
	citem = citem;

	if (oc_type != Config_control_type) 
	{
		kc_set_controls();
	}

}

void joydefs_select_stick_callback(int choice)
{
	if (choice != -1)
	{
		std::vector<joy_info> attached_joysticks;
		joy_get_attached_joysticks(attached_joysticks);

		//TODO: kconfig is still using a separate loop.
		//should be integrated with the menuing system. 
		Kconfig_joy_binding_handle = attached_joysticks[choice].handle;
		kconfig(KConfigMode::Joystick, (const char*)attached_joysticks[choice].name.c_str());
	}
}

bool joydefs_callback(int choice, int nitems, newmenu_item* items)
{
	if (choice == -1)
	{
		Config_joystick_sensitivity = items[9].value;
		Kconfig_use_mouse = items[2].value != 0;
		Kconfig_use_joystick = items[4].value != 0;
		Kconfig_use_gamepad = items[6].value != 0;
		return false;
	}

	switch (choice)
	{
	case 0:
		kconfig(KConfigMode::Keyboard, TXT_KEYBOARD);
		break;
	case 3:
		kconfig(KConfigMode::Mouse, "Mouse");
		break;
	case 5:
	{
		static std::vector<joy_info> attached_joysticks; //TODO: Menus need to own strings
		joy_get_attached_joysticks(attached_joysticks);

		if (attached_joysticks.size() == 0)
		{
			nm_open_messagebox("Note", nullptr, 1, "OK", "No attached joysticks\nwere detected.");
		}
		else
		{
			std::vector<char*> nameptrs;
			for (int i = 0; i < attached_joysticks.size(); i++)
				nameptrs.push_back((char*)attached_joysticks[i].name.c_str());

			newmenu_open_listbox("Select a joystick to bind", nameptrs, true, joydefs_select_stick_callback);
		}
	}
	}
	return true;
}

void joydefs_config()
{
	newmenu_item m[13];
	int nitems = 10;
	m[0].type = NM_TYPE_MENU; m[0].text = TXT_CUST_KEYBOARD;
	m[1].type = NM_TYPE_TEXT; m[1].text = (char*)"";
	m[2].type = NM_TYPE_CHECK; m[2].text = (char*)"Enable mouse"; m[2].value = Kconfig_use_mouse;
	m[3].type = NM_TYPE_MENU; m[3].text = (char*)"Customize mouse...";
	m[4].type = NM_TYPE_CHECK; m[4].text = (char*)"Enable joystick"; m[4].value = Kconfig_use_joystick;
	m[5].type = NM_TYPE_MENU; m[5].text = (char*)"Customize joysticks...";
	m[6].type = NM_TYPE_CHECK; m[6].text = (char*)"Enable gamepad"; m[6].value = Kconfig_use_gamepad;
	m[7].type = NM_TYPE_MENU; m[7].text = (char*)"Customize gamepad...";
	m[8].type = NM_TYPE_TEXT; m[8].text = (char*)"";
	m[9].type = NM_TYPE_SLIDER; m[9].text = TXT_JOYS_SENSITIVITY; m[9].value = Config_joystick_sensitivity; m[9].min_value = 0; m[9].max_value = 8;

	newmenu_open(NULL, TXT_CONTROLS, nitems, m, joydef_menuset_1, joydefs_callback);
}

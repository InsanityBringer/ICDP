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

void joydefs_config()
{
	char xtext[128];
	int i, old_masks, masks;
	newmenu_item m[13];
	int i1 = 0;
	int nitems;

	do 
	{
		nitems = 10;
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

		i1 = newmenu_do1(NULL, TXT_CONTROLS, nitems, m, joydef_menuset_1, i1);

		/*switch (i1) 
		{
		case 5: 
		{
			old_masks = 0;
			for (i = 0; i < 4; i++)
			{
				if (kconfig_is_axes_used(i))
					old_masks |= (1 << i);
			}
			if (Config_control_type == 0)
				// nothing...
				Config_control_type = 0;
			else if (Config_control_type < 5)
				kconfig(1, const_cast<char*>(control_text[Config_control_type]));
			else
				kconfig(2, const_cast<char*>(control_text[Config_control_type]));

			masks = 0;
			for (i = 0; i < 4; i++)
			{
				if (kconfig_is_axes_used(i))
					masks |= (1 << i);
			}

			switch (Config_control_type) 
			{
			case	CONTROL_JOYSTICK:
			case	CONTROL_FLIGHTSTICK_PRO:
			case	CONTROL_THRUSTMASTER_FCS:
			{
				for (i = 0; i < 4; i++) 
				{
					if ((masks & (1 << i)) && (!(old_masks & (1 << i))))
						joydefs_calibrate_flag = 1;
				}
			}
			break;
			}
		}
				break;
		case 7:
			kconfig(0, TXT_KEYBOARD);
			break;
		}*/

		switch (i1)
		{
		case 0:
			kconfig(KConfigMode::Keyboard, TXT_KEYBOARD);
			break;
		case 3:
			kconfig(KConfigMode::Mouse, "Mouse");
			break;
		}

		Config_joystick_sensitivity = m[9].value;
		Kconfig_use_mouse = m[2].value != 0;
		Kconfig_use_joystick = m[4].value != 0;
		Kconfig_use_gamepad = m[6].value != 0;
	} while (i1 > -1);
}

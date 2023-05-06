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

#include <span>
#include <string>
#include <vector>
#include "misc/types.h"
#include "fix/fix.h"

#define JOY_1_BUTTON_A	1
#define JOY_1_BUTTON_B	2
#define JOY_2_BUTTON_A	4
#define JOY_2_BUTTON_B	8
#define JOY_ALL_BUTTONS	(1+2+4+8)

#define JOY_1_X_AXIS		1
#define JOY_1_Y_AXIS		2
#define JOY_2_X_AXIS		4
#define JOY_2_Y_AXIS		8
#define JOY_ALL_AXIS		(1+2+4+8)

 //==========================================================================
 // This initializes the joy and does a "quick" calibration which
 // assumes the stick is centered and sets the minimum value to 0 and
 // the maximum value to 2 times the centered reading. Returns 0 if no
 // joystick was detected, 1 if everything is ok.
 // joy_init() is called.

extern int joy_init();
extern void joy_close();

//A guid is used to identify a type of joystick, but cannot identify specific
//instances of that device type. 
struct joy_guid
{
	uint8_t guid[16];
};

//Information about attached joysticks that are currently valid, to allow one
//to select which device to do bindings for
struct joy_info
{
	int handle;
	std::string name;
};

typedef void(*joy_device_callback)(int handle, joy_guid guid);

//==========================================================================
//Event callbacks. 
//These are fired by the platform code when new devices are attached and
//removed. This must be called before the first call to plat_do_events, or else
//initially connected devices won't be identified.
void joy_set_device_callbacks(joy_device_callback attached, joy_device_callback removed);

extern char joy_installed;
extern char joy_present;

struct JoystickButton
{
	bool down;
	int up_count;
	int down_count;
	fix down_time;
};

//==========================================================================
// This reads the joystick. X and Y will be between -128 and 127.
// Takes about 1 millisecond in the worst case when the stick
// is in the lower right hand corner. Always returns 0,0 if no stick
// is present.

extern void joy_get_pos(int* x, int* y);

//==========================================================================
// This just reads the buttons and returns their status.  When bit 0
// is 1, button 1 is pressed, when bit 1 is 1, button 2 is pressed.
extern int joy_get_btns();

//==========================================================================
// This returns the number of times a button went either down or up since
// the last call to this function.
extern int joy_get_button_up_cnt(int btn);
extern int joy_get_button_down_cnt(int btn);

//==========================================================================
// This returns how long (in approximate milliseconds) that each of the
// buttons has been held down since the last call to this function.
// It is the total time... say you pressed it down for 3 ticks, released
// it, and held it down for 6 more ticks. The time returned would be 9.
extern fix joy_get_button_down_time(int btn);

extern uint8_t joystick_read_raw_axis(uint8_t mask, int* axis);
extern void joy_flush();
extern uint8_t joy_get_present_mask();

extern int joy_get_button_state(int btn);
extern void joy_set_btn_values(int btn, int state, fix timedown, int downcount, int upcount);
extern int joy_get_scaled_reading(int raw, int axn);

void JoystickInput(int buttons, int axes[4], int presentmask);

void I_InitSDLJoysticks();
void I_ControllerHandler();
void I_JoystickHandler();

void joy_get_attached_joysticks(std::vector<joy_info>& info);

//==========================================================================
//Platform specific functions

//Callback when a new joystick is attached to the system.
void plat_joystick_attached(int device_num);
//Callback when a new joystick is detached from the system. 
void plat_joystick_detached(int device_num);
//Returns true if handle is a valid joystick handle, and will stick the current
//state of the joystick into axises, buttons, and hats
bool joy_get_state(int handle, std::span<int>& axises, std::span<JoystickButton>& buttons, std::span<int>& hats);

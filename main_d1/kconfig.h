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

#include <vector>

#include "misc/types.h"
#include "fix/fix.h"

enum class CtrlType
{
	PitchForward,
	PitchBackward,
	TurnLeft,
	TurnRight,
	SlideOn,
	SlideLeft,
	SlideRight,
	SlideUp,
	SlideDown,
	BankOn,
	BankLeft,
	BankRight,
	FirePrimary,
	FireSecondary,
	FireFlare,
	Accelerate,
	Reverse,
	DropBomb,
	RearView,
	CruiseFaster,
	CruiseSlower,
	CruiseOff,
	Automap,
	NumCtrls
};

enum class AxisType
{
	Pitch,
	Yaw,
	Roll,
	Throttle,
	SlideLR,
	SlideUD,
	NumAxises
};

struct control_info
{
	fix	pitch_time;
	fix	vertical_thrust_time;
	fix	heading_time;
	fix	sideways_thrust_time;
	fix	bank_time;
	fix	forward_thrust_time;
#ifdef RESTORE_AFTERBURNER
	fix afterburner_time;
#endif

	uint8_t	rear_view_down_count;
	uint8_t	rear_view_down_state;

	uint8_t	fire_primary_down_count;
	uint8_t	fire_primary_state;
	uint8_t	fire_secondary_state;
	uint8_t	fire_secondary_down_count;
	uint8_t	fire_flare_down_count;

	uint8_t	drop_bomb_down_count;

	uint8_t	automap_down_count;
	uint8_t	automap_state;

};

//Set to true if the first button in a kc_button_binding is a hat. button1 is the bit that must be set
constexpr int KC_BUTTON_B1_HAT = 1;
//Set to true if the second button in a kc_button_binding is a hat. button2 is the bit that must be set
constexpr int KC_BUTTON_B2_HAT = 2;

constexpr int KC_BUTTON_B1_AXIS = 4;
constexpr int KC_BUTTON_B2_AXIS = 8;

struct kc_button_binding
{
	//The interpretation of button1 and button2 is slightly strange. -32768 is no binding.
	//<0 is bound to the axis (-button - 1). This is needed for xinput controllers.
	//>0 is bound straight to a button. 
	//For keyboard keys, this is a scancode, for all others this is an index. 
	int16_t button1, button2;
	uint8_t flags;

	kc_button_binding()
	{
		button1 = button2 = -32768;
		flags = 0;
	}
};

struct kc_axis_binding
{
	int8_t axis;
	bool invert;

	kc_axis_binding()
	{
		axis = -1;
		invert = false;
	}
};

//The amount of joystick bindings varies based on how many sticks are attached.
struct kc_joyinfo
{
	//Guids are used to identify a specific type of device, but sadly won't identify specific instances of a device
	uint8_t guid[16];
	//Due to guids not being unique per instance of a device, store a handle for usage with the joystick libs.
	int handle;

	//These are variable length because the same bindings are shared across Descent 1 and 2, with 2 having more bindings.
	//If it is less than the amount of controls a game has, it gets expanded with defaults. If it is greater, the extras are preserved as-is.
	std::vector<kc_button_binding> buttons;
	std::vector<kc_axis_binding> axises;
	
	kc_joyinfo()
	{
		memset(guid, 0, sizeof(guid));
		handle = -1;
	}
};

struct kc_item
{
	short id;				// The id of this item
	short x, y;
	short w1;
	short w2;
	short u, d, l, r;
	short text_num1;
	uint8_t type;
	uint8_t value;		// what key,button,etc
};

extern control_info Controls;
extern void controls_read_all();
extern void kconfig(int n, char* title);

extern uint8_t Config_digi_volume;
extern uint8_t Config_midi_volume;
extern uint8_t Config_control_type;
extern uint8_t Config_channels_reversed;
extern uint8_t Config_joystick_sensitivity;

#define CONTROL_NONE 0
#define CONTROL_JOYSTICK 1
#define CONTROL_FLIGHTSTICK_PRO 2
#define CONTROL_THRUSTMASTER_FCS 3
#define CONTROL_GRAVIS_GAMEPAD 4
#define CONTROL_MOUSE 5
#define CONTROL_CYBERMAN 6
#define CONTROL_MAX_TYPES 7

#define NUM_KEY_CONTROLS 46
#define NUM_OTHER_CONTROLS 27
#define MAX_CONTROLS 50

extern uint8_t kconfig_settings[CONTROL_MAX_TYPES][MAX_CONTROLS];
extern uint8_t default_kconfig_settings[CONTROL_MAX_TYPES][MAX_CONTROLS];

extern const char* control_text[CONTROL_MAX_TYPES];

extern void kc_set_controls();

void kconfig_init_defaults();

// Tries to use vfx1 head tracking.
void kconfig_sense_init();

//set the cruise speed to zero
extern void reset_cruise(void);

extern int kconfig_is_axes_used(int axis);

extern void kconfig_init_external_controls(int intno, int address);
void kc_drawitem(kc_item* item, int is_current);
void kc_change_key(kc_item* item);
void kc_change_joybutton(kc_item* item);
void kc_change_mousebutton(kc_item* item);
void kc_change_joyaxis(kc_item* item);
void kc_change_mouseaxis(kc_item* item);
void kc_change_invert(kc_item* item);

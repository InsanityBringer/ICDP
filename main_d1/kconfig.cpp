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
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "misc/error.h"
#include "misc/types.h"
#include "2d/gr.h"
#include "platform/mono.h"
#include "platform/key.h"
#include "2d/palette.h"
#include "game.h"
#include "gamefont.h"
#include "iff/iff.h"
#include "mem/mem.h"
#include "platform/joy.h"
#include "platform/mouse.h"
#include "kconfig.h"
#include "gauges.h"
#include "joydefs.h"
#include "render.h"
#include "arcade.h"
#include "main_shared/digi.h"
#include "newmenu.h"
#include "endlevel.h"
#include "multi.h"
#include "platform/timer.h"
#include "stringtable.h"
#include "player.h"
#include "menu.h"
#include "misc/args.h"
#include "platform/platform.h"
#include "platform/event.h"

//#define TABLE_CREATION 1

grs_canvas* kconfig_canvas;

int     sense_function1 = 0;
int	  vfx1_installed = 0;

// Array used to 'blink' the cursor while waiting for a keypress.
int8_t fades[64] = { 1,1,1,2,2,3,4,4,5,6,8,9,10,12,13,15,16,17,19,20,22,23,24,26,27,28,28,29,30,30,31,31,31,31,31,30,30,29,28,28,27,26,24,23,22,20,19,17,16,15,13,12,10,9,8,6,5,4,4,3,2,2,1,1 };

//char * invert_text[2] = { "N", "Y" };
//char * joybutton_text[28] = { "BTN 1", "BTN 2", "BTN 3", "BTN 4", "", "TRIG", "LEFT", "HAT Å", "RIGHT", "", "", "HAT Ä", "MID", "", "", "HAT ", "", "", "", "HAT Ç", "TRIG", "LEFT", "RIGHT", "", "UP","DOWN","LEFT", "RIGHT" };
//char * joyaxis_text[4] = { "X1", "Y1", "X2", "Y2" };
//char * mouseaxis_text[2] = { "L/R", "F/B" };
//char * mousebutton_text[3] = { "Left", "Right", "Mid" };

kc_button_binding default_keyboard_controls[KC_NUM_CONTROLS];
kc_button_binding default_mouse_buttons[KC_NUM_CONTROLS];
kc_axis_binding default_mouse_axises[KC_NUM_AXISES];
kc_button_binding default_joystick_buttons[KC_NUM_CONTROLS];
kc_axis_binding default_joystick_axises[KC_NUM_AXISES];
kc_button_binding default_gamepad_buttons[KC_NUM_CONTROLS];
kc_axis_binding default_gamepad_axises[KC_NUM_AXISES];

std::vector<kc_button_binding> current_keyboard_bindings;
std::vector<kc_button_binding> current_mouse_bindings;
std::vector<kc_axis_binding> current_mouse_axises;

std::vector<kc_joyinfo> current_registered_joysticks;

bool Kconfig_use_mouse;
bool Kconfig_use_joystick;
bool Kconfig_use_gamepad;

//When an event is recieved, it is checked against all keybinds and if any are matched,
//this counter gets incremented if it is down and decremented if it is up.
//When reading controls, this array is used to handle binary controls. 
struct control_down_info
{
	int num_inputs_down;
	fix down_time;
	int down_count;
};
control_down_info control_down_count[KC_NUM_CONTROLS];

const char* choco_gamepad_text[28] =
{ "0", "1", "2", "3",
"RTRIG", "LTRIG", "LB", "\x81",
"RB", "9", "A", "\x80",
"12", "13", "B", "\x7F",
"X", "17", "Y", "\x82",
"20", "21", "22", "23",
"24", "25", "26", "27" };

const char* hat_strings[] = { "\x81", "\x80", "\x7F", "\x82" };

const char* choco_joybutton_text[28] =
//Basic inputs
{ "BTN 1", "BTN 2", "BTN 3", "BTN 4",
//"Extended" Flightstick inputs, default ATM. 
"BTN 1", "BTN 2", "BTN 3", "HAT \x81",
"BTN 4", "BTN 5", "BTN 6", "HAT \x80",
"BTN 7", "BTN 8", "BTN 9", "HAT \x7F",
"BTN 10", "BTN 11", "BTN 12", "HAT \x82",
//[ISB] can't bind above 20...
"-20-", "-21-", "-22-", "-23-",
"-24-", "-25-", "-26-", "-27-" };

int invert_text[2] = { TNUM_N, TNUM_Y };
int joybutton_text[28] =
{ TNUM_BTN_1, TNUM_BTN_2, TNUM_BTN_3, TNUM_BTN_4,
  -1, TNUM_TRIG, TNUM_LEFT, TNUM_HAT_L,
 TNUM_RIGHT, -1, TNUM_HAT2_D, TNUM_HAT_R,
 TNUM_MID, -1, TNUM_HAT2_R, TNUM_HAT_U,
 TNUM_HAT2_L, -1, TNUM_HAT2_U, TNUM_HAT_D,
 TNUM_TRIG, TNUM_LEFT, TNUM_RIGHT, -1,
  TNUM_UP, TNUM_DOWN, TNUM_LEFT, TNUM_RIGHT };
int joyaxis_text[4] = { TNUM_X1, TNUM_Y1, TNUM_X2, TNUM_Y2 };
int mouseaxis_text[2] = { TNUM_L_R, TNUM_F_B };
int mousebutton_text[3] = { TNUM_LEFT, TNUM_RIGHT, TNUM_MID };

const char* key_text[256] = { \
"","ESC","1","2","3","4","5","6","7","8","9","0","-", 			\
"=","BSPC","TAB","Q","W","E","R","T","Y","U","I","O",				\
"P","[","]","É","LCTRL","A","S","D","F",        \
"G","H","J","K","L",";","'","`",        \
"LSHFT","\\","Z","X","C","V","B","N","M",",",      \
".","/","RSHFT","PAD*","LALT","SPC",      \
"CPSLK","F1","F2","F3","F4","F5","F6","F7","F8","F9",        \
"F10","NMLCK","SCLK","PAD7","PAD8","PAD9","PAD-",   \
"PAD4","PAD5","PAD6","PAD+","PAD1","PAD2","PAD3","PAD0", \
"PAD.","","","","F11","F12","","","","","","","","","",         \
"","","","","","","","","","","","","","","","","","","","",     \
"","","","","","","","","","","","","","","","","","","","",     \
"","","","","","","","","","","","","","","","","","",           \
"PADÉ","RCTRL","","","","","","","","","","","","","", \
"","","","","","","","","","","PAD/","","","RALT","",      \
"","","","","","","","","","","","","","HOME","Ç","PGUP",     \
"","Å","","","","END","Ä","PGDN","INS",       \
"DEL","","","","","","","","","","","","","","","","","",     \
"","","","","","","","","","","","","","","","","","","","",     \
"","","","","","","" };

uint8_t system_keys[] = { KEY_ESC, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUAL, KEY_PRINT_SCREEN };

control_info Controls;

uint8_t Config_digi_volume = 16;
uint8_t Config_midi_volume = 16;
uint8_t Config_control_type = 0;
uint8_t Config_channels_reversed = 0;
uint8_t Config_joystick_sensitivity = 8;

fix Cruise_speed = 0;

#define BT_KEY 				0
#define BT_MOUSE_BUTTON 	1
#define BT_MOUSE_AXIS		2
#define BT_JOY_BUTTON 		3
#define BT_JOY_AXIS			4
#define BT_INVERT				5

const char* btype_text[] = { "BT_KEY", "BT_MOUSE_BUTTON", "BT_MOUSE_AXIS", "BT_JOY_BUTTON", "BT_JOY_AXIS", "BT_INVERT" };

#define INFO_Y 28

int Num_items = 23;
kc_item* All_items;

uint8_t kconfig_settings[CONTROL_MAX_TYPES][MAX_CONTROLS];
int Kconfig_joy_binding_handle;

//----------- WARNING!!!!!!! -------------------------------------------
// THESE NEXT FOUR BLOCKS OF DATA ARE GENERATED BY PRESSING DEL+F12 WHEN
// IN THE KEYBOARD CONFIG SCREEN.  BASICALLY, THAT PROCEDURE MODIFIES THE
// U,D,L,R FIELDS OF THE ARRAYS AND DUMPS THE NEW ARRAYS INTO KCONFIG.COD
//-------------------------------------------------------------------------

uint8_t default_kconfig_settings[CONTROL_MAX_TYPES][MAX_CONTROLS] = {
{0xc8,0x48,0xd0,0x50,0xcb,0x4b,0xcd,0x4d,0x38,0xff,0xff,0x4f,0xff,0x51,0xff,0x4a,0xff,0x4e,0xff,0xff,0x10,0x47,0x12,0x49,0x1d,0x9d,0x39,0xff,0x21,0xff,0x1e,0xff,0x2c,0xff,0x30,0xff,0x13,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xf,0xff,0xff,0xff,0xff,0xff},
{0x0,0x1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
{0x5,0xc,0xff,0xff,0xff,0xff,0x7,0xf,0x13,0xb,0xff,0x6,0x8,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
{0x0,0x1,0xff,0xff,0x2,0xff,0x7,0xf,0x13,0xb,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x3,0xff,0x3,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
{0x3,0x0,0x1,0x2,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
{0x0,0x1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
{0x0,0x1,0xff,0xff,0x2,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
};

kc_item kc_keyboard[NUM_KEY_CONTROLS] = {
	{(short)CtrlType::PitchForward, false, 15, 49, 71, 26, 43,  2, 23,  1,TNUM_PITCH_FORWARD, BT_KEY, 255 },
	{(short)CtrlType::PitchForward, true, 15, 49,100, 26, 22,  3,  0, 24,TNUM_PITCH_FORWARD, BT_KEY, 255 },
	{(short)CtrlType::PitchBackward, false, 15, 57, 71, 26,  0,  4, 25,  3,TNUM_PITCH_BACKWARD, BT_KEY, 255 },
	{(short)CtrlType::PitchBackward, true, 15, 57,100, 26,  1,  5,  2, 26,TNUM_PITCH_BACKWARD, BT_KEY, 255 },
	{(short)CtrlType::TurnLeft, false, 15, 65, 71, 26,  2,  6, 27,  5,TNUM_TURN_LEFT, BT_KEY, 255 },
	{(short)CtrlType::TurnLeft, true, 15, 65,100, 26,  3,  7,  4, 28,TNUM_TURN_LEFT, BT_KEY, 255 },
	{(short)CtrlType::TurnRight, false, 15, 73, 71, 26,  4,  8, 29,  7,TNUM_TURN_RIGHT, BT_KEY, 255 },
	{(short)CtrlType::TurnRight, true, 15, 73,100, 26,  5,  9,  6, 34,TNUM_TURN_RIGHT, BT_KEY, 255 },
	{(short)CtrlType::SlideOn, false, 15, 85, 71, 26,  6, 10, 35,  9,TNUM_SLIDE_ON, BT_KEY, 255 },
	{(short)CtrlType::SlideOn, true, 15, 85,100, 26,  7, 11,  8, 36,TNUM_SLIDE_ON, BT_KEY, 255 },
	{(short)CtrlType::SlideLeft, false, 15, 93, 71, 26,  8, 12, 37, 11,TNUM_SLIDE_LEFT, BT_KEY, 255 },
	{(short)CtrlType::SlideLeft, true, 15, 93,100, 26,  9, 13, 10, 44,TNUM_SLIDE_LEFT, BT_KEY, 255 },
	{(short)CtrlType::SlideRight, false, 15,101, 71, 26, 10, 14, 45, 13,TNUM_SLIDE_RIGHT, BT_KEY, 255 },
	{(short)CtrlType::SlideRight, true, 15,101,100, 26, 11, 15, 12, 30,TNUM_SLIDE_RIGHT, BT_KEY, 255 },
	{(short)CtrlType::SlideUp, false, 15,109, 71, 26, 12, 16, 31, 15,TNUM_SLIDE_UP, BT_KEY, 255 },
	{(short)CtrlType::SlideUp, true, 15,109,100, 26, 13, 17, 14, 32,TNUM_SLIDE_UP, BT_KEY, 255 },
	{(short)CtrlType::SlideDown, false, 15,117, 71, 26, 14, 18, 33, 17,TNUM_SLIDE_DOWN, BT_KEY, 255 },
	{(short)CtrlType::SlideDown, true, 15,117,100, 26, 15, 19, 16, 38,TNUM_SLIDE_DOWN, BT_KEY, 255 },
	{(short)CtrlType::BankOn, false, 15,129, 71, 26, 16, 20, 39, 19,TNUM_BANK_ON, BT_KEY, 255 },
	{(short)CtrlType::BankOn, true, 15,129,100, 26, 17, 21, 18, 40,TNUM_BANK_ON, BT_KEY, 255 },
	{(short)CtrlType::BankLeft, false, 15,137, 71, 26, 18, 22, 41, 21,TNUM_BANK_LEFT, BT_KEY, 255 },
	{(short)CtrlType::BankLeft, true, 15,137,100, 26, 19, 23, 20, 42,TNUM_BANK_LEFT, BT_KEY, 255 },
	{(short)CtrlType::BankRight, false, 15,145, 71, 26, 20,  1, 43, 23,TNUM_BANK_RIGHT, BT_KEY, 255 },
	{(short)CtrlType::BankRight, true, 15,145,100, 26, 21, 24, 22,  0,TNUM_BANK_RIGHT, BT_KEY, 255 },
	{(short)CtrlType::FirePrimary, false,158, 49, 83, 26, 23, 26,  1, 25,TNUM_FIRE_PRIMARY, BT_KEY, 255 },
	{(short)CtrlType::FirePrimary, true,158, 49,112, 26, 42, 27, 24,  2,TNUM_FIRE_PRIMARY, BT_KEY, 255 },
	{(short)CtrlType::FireSecondary, false,158, 57, 83, 26, 24, 28,  3, 27,TNUM_FIRE_SECONDARY, BT_KEY, 255 },
	{(short)CtrlType::FireSecondary, true,158, 57,112, 26, 25, 29, 26,  4,TNUM_FIRE_SECONDARY, BT_KEY, 255 },
	{(short)CtrlType::FireFlare, false,158, 65, 83, 26, 26, 34,  5, 29,TNUM_FIRE_FLARE, BT_KEY, 255 },
	{(short)CtrlType::FireFlare, true,158, 65,112, 26, 27, 35, 28,  6,TNUM_FIRE_FLARE, BT_KEY, 255 },
	{(short)CtrlType::Accelerate, false,158,105, 83, 26, 44, 32, 13, 31,TNUM_ACCELERATE, BT_KEY, 255 },
	{(short)CtrlType::Accelerate, true,158,105,112, 26, 45, 33, 30, 14,TNUM_ACCELERATE, BT_KEY, 255 },
	{(short)CtrlType::Reverse, false,158,113, 83, 26, 30, 38, 15, 33,TNUM_REVERSE, BT_KEY, 255 },
	{(short)CtrlType::Reverse, true,158,113,112, 26, 31, 39, 32, 16,TNUM_REVERSE, BT_KEY, 255 },
	{(short)CtrlType::DropBomb, false,158, 73, 83, 26, 28, 36,  7, 35,TNUM_DROP_BOMB, BT_KEY, 255 },
	{(short)CtrlType::DropBomb, true,158, 73,112, 26, 29, 37, 34,  8,TNUM_DROP_BOMB, BT_KEY, 255 },
	{(short)CtrlType::RearView, false,158, 85, 83, 26, 34, 44,  9, 37,TNUM_REAR_VIEW, BT_KEY, 255 },
	{(short)CtrlType::RearView, true,158, 85,112, 26, 35, 45, 36, 10,TNUM_REAR_VIEW, BT_KEY, 255 },
	{(short)CtrlType::CruiseFaster, false,158,125, 83, 26, 32, 40, 17, 39,TNUM_CRUISE_FASTER, BT_KEY, 255 },
	{(short)CtrlType::CruiseFaster, true,158,125,112, 26, 33, 41, 38, 18,TNUM_CRUISE_FASTER, BT_KEY, 255 },
	{(short)CtrlType::CruiseSlower, false,158,133, 83, 26, 38, 42, 19, 41,TNUM_CRUISE_SLOWER, BT_KEY, 255 },
	{(short)CtrlType::CruiseSlower, true,158,133,112, 26, 39, 43, 40, 20,TNUM_CRUISE_SLOWER, BT_KEY, 255 },
	{(short)CtrlType::CruiseOff, false,158,141, 83, 26, 40, 25, 21, 43,TNUM_CRUISE_OFF, BT_KEY, 255 },
	{(short)CtrlType::CruiseOff, true,158,141,112, 26, 41,  0, 42, 22,TNUM_CRUISE_OFF, BT_KEY, 255 },
	{(short)CtrlType::Automap, false,158, 93, 83, 26, 36, 30, 11, 45,TNUM_AUTOMAP, BT_KEY, 255 },
	{(short)CtrlType::Automap, true,158, 93,112, 26, 37, 31, 44, 12,TNUM_AUTOMAP, BT_KEY, 255 },
};
kc_item kc_joystick[NUM_OTHER_CONTROLS] = {
	{(short)CtrlType::FirePrimary, false, 25, 46, 85, 26, 15,  1, 24,  5,TNUM_FIRE_PRIMARY, BT_JOY_BUTTON, 255 },
	{(short)CtrlType::FireSecondary, false, 25, 54, 85, 26,  0,  4,  5,  6,TNUM_FIRE_SECONDARY, BT_JOY_BUTTON, 255 },
	{(short)CtrlType::Accelerate, false, 25, 85, 85, 26, 26,  3,  9, 10,TNUM_ACCELERATE, BT_JOY_BUTTON, 255 },
	{(short)CtrlType::Reverse, false, 25, 93, 85, 26,  2, 25, 10, 11,TNUM_REVERSE, BT_JOY_BUTTON, 255 },
	{(short)CtrlType::FireFlare, false, 25, 62, 85, 26,  1, 26,  6,  7,TNUM_FIRE_FLARE, BT_JOY_BUTTON, 255 },
	{(short)CtrlType::SlideOn, false,180, 46, 59, 26, 23,  6,  0,  1,TNUM_SLIDE_ON, BT_JOY_BUTTON, 255 },
	{(short)CtrlType::SlideLeft, false,180, 54, 59, 26,  5,  7,  1,  4,TNUM_SLIDE_LEFT, BT_JOY_BUTTON, 255 },
	{(short)CtrlType::SlideRight, false,180, 62, 59, 26,  6,  8,  4, 26,TNUM_SLIDE_RIGHT, BT_JOY_BUTTON, 255 },
	{(short)CtrlType::SlideUp, false,180, 70, 59, 26,  7,  9, 26,  9,TNUM_SLIDE_UP, BT_JOY_BUTTON, 255 },
	{(short)CtrlType::SlideDown, false,180, 78, 59, 26,  8, 10,  8,  2,TNUM_SLIDE_DOWN, BT_JOY_BUTTON, 255 },
	{(short)CtrlType::BankOn, false,180, 90, 59, 26,  9, 11,  2,  3,TNUM_BANK_ON, BT_JOY_BUTTON, 255 },
	{(short)CtrlType::BankLeft, false,180, 98, 59, 26, 10, 12,  3, 12,TNUM_BANK_LEFT, BT_JOY_BUTTON, 255 },
	{(short)CtrlType::BankRight, false,180,106, 59, 26, 11, 18, 11, 25,TNUM_BANK_RIGHT, BT_JOY_BUTTON, 255 },
	{(short)AxisType::Pitch, false, 22,146, 51, 26, 24, 15, 25, 14,TNUM_PITCH_UD, BT_JOY_AXIS, 255 },
	{(short)AxisType::Pitch, false, 22,146, 99,  8, 25, 16, 13, 17,TNUM_PITCH_UD, BT_INVERT, 255 },
	{(short)AxisType::Yaw, false, 22,154, 51, 26, 13,  0, 18, 16,TNUM_TURN_LR, BT_JOY_AXIS, 255 },
	{(short)AxisType::Yaw, false, 22,154, 99,  8, 14, 17, 15, 19,TNUM_TURN_LR, BT_INVERT, 255 },
	{(short)AxisType::SlideLR, false,164,146, 58, 26, 16, 19, 14, 18,TNUM_SLIDE_LR, BT_JOY_AXIS, 255 },
	{(short)AxisType::SlideLR, false,164,146,106,  8, 12, 20, 17, 15,TNUM_SLIDE_LR, BT_INVERT, 255 },
	{(short)AxisType::SlideUD, false,164,154, 58, 26, 17, 21, 16, 20,TNUM_SLIDE_UD, BT_JOY_AXIS, 255 },
	{(short)AxisType::SlideUD, false,164,154,106,  8, 18, 22, 19, 21,TNUM_SLIDE_UD, BT_INVERT, 255 },
	{(short)AxisType::Roll, false,164,162, 58, 26, 19, 23, 20, 22,TNUM_BANK_LR, BT_JOY_AXIS, 255 },
	{(short)AxisType::Roll, false,164,162,106,  8, 20, 24, 21, 23,TNUM_BANK_LR, BT_INVERT, 255 },
	{(short)AxisType::Throttle, false,164,174, 58, 26, 21,  5, 22, 24,TNUM_THROTTLE, BT_JOY_AXIS, 255 },
	{(short)AxisType::Throttle, false,164,174,106,  8, 22, 13, 23,  0,TNUM_THROTTLE, BT_INVERT, 255 },
	{(short)CtrlType::RearView, false, 25,109, 85, 26,  3, 14, 12, 13,TNUM_REAR_VIEW, BT_JOY_BUTTON, 255 },
	{(short)CtrlType::DropBomb, false, 25, 70, 85, 26,  4,  2,  7,  8,TNUM_DROP_BOMB, BT_JOY_BUTTON, 255 },
};
kc_item kc_mouse[NUM_OTHER_CONTROLS] = {
	{(short)CtrlType::FirePrimary, false, 25, 46, 85, 26, 12,  1, 24,  5,TNUM_FIRE_PRIMARY, BT_MOUSE_BUTTON, 255 },
	{(short)CtrlType::FireSecondary, false, 25, 54, 85, 26,  0,  4,  5,  6,TNUM_FIRE_SECONDARY, BT_MOUSE_BUTTON, 255 },
	{(short)CtrlType::Accelerate, false, 25, 85, 85, 26, 26,  3,  9, 10,TNUM_ACCELERATE, BT_MOUSE_BUTTON, 255 },
	{(short)CtrlType::Reverse, false, 25, 93, 85, 26,  2, 25, 10, 11,TNUM_REVERSE, BT_MOUSE_BUTTON, 255 },
	{(short)CtrlType::FireFlare, false, 25, 62, 85, 26,  1, 26,  6,  7,TNUM_FIRE_FLARE, BT_MOUSE_BUTTON, 255 },
	{(short)CtrlType::SlideOn, false,180, 46, 59, 26, 24,  6,  0,  1,TNUM_SLIDE_ON, BT_MOUSE_BUTTON, 255 },
	{(short)CtrlType::SlideLeft, false,180, 54, 59, 26,  5,  7,  1,  4,TNUM_SLIDE_LEFT, BT_MOUSE_BUTTON, 255 },
	{(short)CtrlType::SlideRight, false,180, 62, 59, 26,  6,  8,  4, 26,TNUM_SLIDE_RIGHT, BT_MOUSE_BUTTON, 255 },
	{(short)CtrlType::SlideUp, false,180, 70, 59, 26,  7,  9, 26,  9,TNUM_SLIDE_UP, BT_MOUSE_BUTTON, 255 },
	{(short)CtrlType::SlideDown, false,180, 78, 59, 26,  8, 10,  8,  2,TNUM_SLIDE_DOWN, BT_MOUSE_BUTTON, 255 },
	{(short)CtrlType::BankOn, false,180, 90, 59, 26,  9, 11,  2,  3,TNUM_BANK_ON, BT_MOUSE_BUTTON, 255 },
	{(short)CtrlType::BankLeft, false,180, 98, 59, 26, 10, 12,  3, 12,TNUM_BANK_LEFT, BT_MOUSE_BUTTON, 255 },
	{(short)CtrlType::BankRight, false,180,106, 59, 26, 11,  0, 11, 25,TNUM_BANK_RIGHT, BT_MOUSE_BUTTON, 255 },
	{(short)AxisType::Pitch, false,103,138, 58, 26, 25, 15, 25, 14,TNUM_PITCH_UD, BT_MOUSE_AXIS, 255 },
	{(short)AxisType::Pitch, false,103,138,106,  8, 23, 16, 13, 15,TNUM_PITCH_UD, BT_INVERT, 255 },
	{(short)AxisType::Yaw, false,103,146, 58, 26, 13, 17, 14, 16,TNUM_TURN_LR, BT_MOUSE_AXIS, 255 },
	{(short)AxisType::Yaw, false,103,146,106,  8, 14, 18, 15, 17,TNUM_TURN_LR, BT_INVERT, 255 },
	{(short)AxisType::SlideLR, false,103,154, 58, 26, 15, 19, 16, 18,TNUM_SLIDE_LR, BT_MOUSE_AXIS, 255 },
	{(short)AxisType::SlideLR, false,103,154,106,  8, 16, 20, 17, 19,TNUM_SLIDE_LR, BT_INVERT, 255 },
	{(short)AxisType::SlideUD, false,103,162, 58, 26, 17, 21, 18, 20,TNUM_SLIDE_UD, BT_MOUSE_AXIS, 255 },
	{(short)AxisType::SlideUD, false,103,162,106,  8, 18, 22, 19, 21,TNUM_SLIDE_UD, BT_INVERT, 255 },
	{(short)AxisType::Roll, false,103,170, 58, 26, 19, 23, 20, 22,TNUM_BANK_LR, BT_MOUSE_AXIS, 255 },
	{(short)AxisType::Roll, false,103,170,106,  8, 20, 24, 21, 23,TNUM_BANK_LR, BT_INVERT, 255 },
	{(short)AxisType::Throttle, false,103,182, 58, 26, 21, 14, 22, 24,TNUM_THROTTLE, BT_MOUSE_AXIS, 255 },
	{(short)AxisType::Throttle, false,103,182,106,  8, 22,  5, 23,  0,TNUM_THROTTLE, BT_INVERT, 255 },
	{(short)CtrlType::RearView, false, 25,109, 85, 26,  3, 13, 12, 13,TNUM_REAR_VIEW, BT_MOUSE_BUTTON, 255 },
	{(short)CtrlType::DropBomb, false, 25, 70, 85, 26,  4,  2,  7,  8,TNUM_DROP_BOMB, BT_MOUSE_BUTTON, 255 },
};

int kconfig_is_axes_used(int axis)
{
	int i;
	for (i = 0; i < NUM_OTHER_CONTROLS; i++) 
	{
		if ((kc_joystick[i].type == BT_JOY_AXIS) && (kc_joystick[i].value == axis))
			return 1;
	}
	return 0;
}

void kconfig_sub(kc_item* items, int nitems, const char* title)
{
	grs_canvas* save_canvas;
	grs_font* save_font;
	int old_keyd_repeat;

	int i, k, ocitem, citem;
	int time_stopped = 0;

	char* titlebuf;


	All_items = items;
	Num_items = nitems;

	if (!((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME) && (!Endlevel_sequence)))
	{
		time_stopped = 1;
		stop_time();
	}

	save_canvas = grd_curcanv;
	gr_set_current_canvas(kconfig_canvas);
	save_font = grd_curcanv->cv_font;
	game_flush_inputs();
	old_keyd_repeat = keyd_repeat;
	keyd_repeat = 1;

	//gr_clear_canvas( BM_XRGB(0,0,0) );

	nm_draw_background(0, 0, grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h);

	grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_3];

	{
		char* p;

		titlebuf = (char*)malloc(sizeof(char) * (strlen(title)+1));
		strcpy(titlebuf, title);
		p = strchr(titlebuf, '\n');
		if (p)* p = 32;
		gr_string(0x8000, 8, titlebuf);
		if (p)* p = '\n';
		free(titlebuf);
	}


	//	if ( items == kc_keyboard )	{
	//		gr_string( 0x8000, 8, "Keyboard" );
	//	} else if ( items == kc_joystick )	{
	//		gr_string( 0x8000, 8, "Joysticks" );
	//	} else if ( items == kc_mouse )	{
	//		gr_string( 0x8000, 8, "Mouse" );
	//	}

	grd_curcanv->cv_font = GAME_FONT;
	gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);

	gr_string(0x8000, 20, TXT_KCONFIG_STRING_1);
	gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);
	if (items == kc_keyboard) 
	{
		gr_set_fontcolor(BM_XRGB(31, 27, 6), -1);
		gr_setcolor(BM_XRGB(31, 27, 6));

		gr_scanline(98, 106, 42);
		gr_scanline(120, 128, 42);
		gr_pixel(98, 43);
		gr_pixel(98, 44);
		gr_pixel(128, 43);
		gr_pixel(128, 44);

		gr_string(109, 40, "OR");

		gr_scanline(253, 261, 42);
		gr_scanline(274, 283, 42);
		gr_pixel(253, 43);
		gr_pixel(253, 44);
		gr_pixel(283, 43);
		gr_pixel(283, 44);

		gr_string(264, 40, "OR");

	} 
	if (items == kc_joystick) 
	{
		gr_set_fontcolor(BM_XRGB(31, 27, 6), -1);
		gr_setcolor(BM_XRGB(31, 27, 6));
		gr_scanline(18, 135, 37);
		gr_scanline(181, 294, 37);
		gr_scanline(18, 144, 119 + 10);
		gr_scanline(174, 294, 119 + 10);
		gr_string(0x8000, 35, TXT_BUTTONS);
		gr_string(0x8000, 117 + 10, TXT_AXES);
		gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);
		gr_string(81, 137, TXT_AXIS);
		gr_string(111, 137, TXT_INVERT);
		gr_string(222, 137, TXT_AXIS);
		gr_string(252, 137, TXT_INVERT);
	}
	else if (items == kc_mouse) 
	{
		gr_set_fontcolor(BM_XRGB(31, 27, 6), -1);
		gr_setcolor(BM_XRGB(31, 27, 6));
		gr_scanline(18, 135, 37);
		gr_scanline(181, 294, 37);
		gr_scanline(18, 144, 119 + 5);
		gr_scanline(174, 294, 119 + 5);
		gr_string(0x8000, 35, TXT_BUTTONS);
		gr_string(0x8000, 117 + 5, TXT_AXES);
		gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);
		gr_string(169, 129, TXT_AXIS);
		gr_string(199, 129, TXT_INVERT);
	}

	for (i = 0; i < nitems; i++) 
	{
		kc_drawitem(&items[i], 0);
	}

	citem = 0;
	kc_drawitem(&items[citem], 1);

	while (1) 
	{
		timer_mark_start();
		plat_do_events();
		k = key_inkey();
		if (!time_stopped)
		{
#ifdef NETWORK
			if (multi_menu_poll() == -1)
				k = -2;
#endif
		}
		ocitem = citem;
		switch (k) 
		{
		case KEY_BACKSP:
			Int3();
			break;
		case KEY_PRINT_SCREEN:
			save_screen_shot(0);
			break;
		case KEY_CTRLED + KEY_D:
			items[citem].value = -32768;
			kc_drawitem(&items[citem], 1);
			break;
		case KEY_CTRLED + KEY_R:
			if (items == kc_keyboard)
			{
				for (i = 0; i < NUM_KEY_CONTROLS; i++) 
				{
					items[i].value = default_kconfig_settings[0][i];
					kc_drawitem(&items[i], 0);
				}
			}
			else {
				for (i = 0; i < NUM_OTHER_CONTROLS; i++) 
				{
					items[i].value = default_kconfig_settings[Config_control_type][i];
					kc_drawitem(&items[i], 0);
				}
			}
			kc_drawitem(&items[citem], 1);
			break;
		case KEY_UP:
		case KEY_PAD8:
#ifdef TABLE_CREATION
			if (items[citem].u == -1) items[citem].u = find_next_item_up(items, nitems, citem);
#endif
			citem = items[citem].u;
			break;
		case KEY_DOWN:
		case KEY_PAD2:
#ifdef TABLE_CREATION
			if (items[citem].d == -1) items[citem].d = find_next_item_down(items, nitems, citem);
#endif
			citem = items[citem].d;
			break;
		case KEY_LEFT:
		case KEY_PAD4:
#ifdef TABLE_CREATION
			if (items[citem].l == -1) items[citem].l = find_next_item_left(items, nitems, citem);
#endif
			citem = items[citem].l;
			break;
		case KEY_RIGHT:
		case KEY_PAD6:
#ifdef TABLE_CREATION
			if (items[citem].r == -1) items[citem].r = find_next_item_right(items, nitems, citem);
#endif
			citem = items[citem].r;
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
			switch (items[citem].type) 
			{
			case BT_KEY:				kc_change_key(&items[citem]); break;
			case BT_MOUSE_BUTTON:	kc_change_mousebutton(&items[citem]); break;
			case BT_MOUSE_AXIS: 		kc_change_mouseaxis(&items[citem]); break;
			case BT_JOY_BUTTON: 		kc_change_joybutton(&items[citem]); break;
			case BT_JOY_AXIS: 		kc_change_joyaxis(&items[citem]); break;
			case BT_INVERT: 			kc_change_invert(&items[citem]); break;
			}
			break;
		case -2:
		case KEY_ESC:
			grd_curcanv->cv_font = save_font;
			gr_set_current_canvas(save_canvas);
			keyd_repeat = old_keyd_repeat;
			game_flush_inputs();
			if (time_stopped)
				start_time();
			return;
#ifdef TABLE_CREATION
		case KEY_DEBUGGED + KEY_F12: {
			FILE* fp;
			for (i = 0; i < NUM_KEY_CONTROLS; i++) {
				kc_keyboard[i].u = find_next_item_up(kc_keyboard, NUM_KEY_CONTROLS, i);
				kc_keyboard[i].d = find_next_item_down(kc_keyboard, NUM_KEY_CONTROLS, i);
				kc_keyboard[i].l = find_next_item_left(kc_keyboard, NUM_KEY_CONTROLS, i);
				kc_keyboard[i].r = find_next_item_right(kc_keyboard, NUM_KEY_CONTROLS, i);
			}
			for (i = 0; i < NUM_OTHER_CONTROLS; i++) {
				kc_joystick[i].u = find_next_item_up(kc_joystick, NUM_OTHER_CONTROLS, i);
				kc_joystick[i].d = find_next_item_down(kc_joystick, NUM_OTHER_CONTROLS, i);
				kc_joystick[i].l = find_next_item_left(kc_joystick, NUM_OTHER_CONTROLS, i);
				kc_joystick[i].r = find_next_item_right(kc_joystick, NUM_OTHER_CONTROLS, i);
			}
			for (i = 0; i < NUM_OTHER_CONTROLS; i++) {
				kc_mouse[i].u = find_next_item_up(kc_mouse, NUM_OTHER_CONTROLS, i);
				kc_mouse[i].d = find_next_item_down(kc_mouse, NUM_OTHER_CONTROLS, i);
				kc_mouse[i].l = find_next_item_left(kc_mouse, NUM_OTHER_CONTROLS, i);
				kc_mouse[i].r = find_next_item_right(kc_mouse, NUM_OTHER_CONTROLS, i);
			}
			fp = fopen("kconfig.cod", "wt");

			fprintf(fp, "ubyte default_kconfig_settings[CONTROL_MAX_TYPES][MAX_CONTROLS] = {\n");
			for (i = 0; i < CONTROL_MAX_TYPES; i++) {
				int j;
				fprintf(fp, "{0x%x", kconfig_settings[i][0]);
				for (j = 1; j < MAX_CONTROLS; j++)
					fprintf(fp, ",0x%x", kconfig_settings[i][j]);
				fprintf(fp, "},\n");
			}
			fprintf(fp, "};\n");

			fprintf(fp, "\nkc_item kc_keyboard[NUM_KEY_CONTROLS] = {\n");
			for (i = 0; i < NUM_KEY_CONTROLS; i++) {
				fprintf(fp, "\t{ %2d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%c%s%c, %s, 255 },\n",
					kc_keyboard[i].id, kc_keyboard[i].x, kc_keyboard[i].y, kc_keyboard[i].w1, kc_keyboard[i].w2,
					kc_keyboard[i].u, kc_keyboard[i].d, kc_keyboard[i].l, kc_keyboard[i].r,
					34, Text_string[kc_keyboard[i].text_num1], 34, btype_text[kc_keyboard[i].type]);
			}
			fprintf(fp, "};");

			fprintf(fp, "\nkc_item kc_joystick[NUM_OTHER_CONTROLS] = {\n");
			for (i = 0; i < NUM_OTHER_CONTROLS; i++) {
				fprintf(fp, "\t{ %2d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%c%s%c, %s, 255 },\n",
					kc_joystick[i].id, kc_joystick[i].x, kc_joystick[i].y, kc_joystick[i].w1, kc_joystick[i].w2,
					kc_joystick[i].u, kc_joystick[i].d, kc_joystick[i].l, kc_joystick[i].r,
					34, Text_string[kc_joystick[i].text_num1], 34, btype_text[kc_joystick[i].type]);
			}
			fprintf(fp, "};");

			fprintf(fp, "\nkc_item kc_mouse[NUM_OTHER_CONTROLS] = {\n");
			for (i = 0; i < NUM_OTHER_CONTROLS; i++) {
				fprintf(fp, "\t{ %2d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%c%s%c, %s, 255 },\n",
					kc_mouse[i].id, kc_mouse[i].x, kc_mouse[i].y, kc_mouse[i].w1, kc_mouse[i].w2,
					kc_mouse[i].u, kc_mouse[i].d, kc_mouse[i].l, kc_mouse[i].r,
					34, Text_string[kc_mouse[i].text_num1], 34, btype_text[kc_mouse[i].type]);
			}
			fprintf(fp, "};");

			fclose(fp);

		}
									 break;
#endif
		}
		if (ocitem != citem) 
		{
			kc_drawitem(&items[ocitem], 0);
			kc_drawitem(&items[citem], 1);
		}
		plat_present_canvas(*kconfig_canvas, ASPECT_4_3);
		timer_mark_end(US_70FPS);
	}
}


void kc_drawitem(kc_item* item, int is_current)
{
	int x, w, h, aw;
	char btext[10];

	if (is_current)
		gr_set_fontcolor(BM_XRGB(20, 20, 29), -1);
	else
		gr_set_fontcolor(BM_XRGB(15, 15, 24), -1);
	gr_string(item->x, item->y, Text_string[item->text_num1]);

	if (item->value < 0) 
	{
		sprintf(btext, "");
	}
	else 
	{
		switch (item->type) 
		{
		case BT_KEY:
			strncpy(btext, key_text[item->value], 10); break;
		case BT_MOUSE_BUTTON:
			if (item->value < 3)
				strncpy(btext, Text_string[mousebutton_text[item->value]], 10);
			else
				snprintf(btext, 10, "BTN %d", item->value);
			break;
		case BT_MOUSE_AXIS:
			strncpy(btext, Text_string[mouseaxis_text[item->value]], 10); break;
		case BT_JOY_BUTTON:
			{
				if (item->extra == KC_BUTTON_TYPE_AXIS)
					snprintf(btext, 10, "AXIS%d", item->value);
				else if (item->extra == KC_BUTTON_TYPE_HAT)
					snprintf(btext, 10, "H%d %s", item->value / 4 + 1, hat_strings[item->value % 4]);
				else
					snprintf(btext, 10, "BTN%d", item->value);

				btext[9] = '\0';
			}
			break;
		case BT_JOY_AXIS:
			if (item->value < 4)
				strncpy(btext, Text_string[joyaxis_text[item->value]], 10); 
			else
				snprintf(btext, 10, "%d", item->value);
			break;
		case BT_INVERT:
			strncpy(btext, Text_string[invert_text[item->value]], 10); break;
		}
	}
	gr_get_string_size(btext, &w, &h, &aw);

	if (is_current)
		gr_setcolor(BM_XRGB(21, 0, 24));
	else
		gr_setcolor(BM_XRGB(16, 0, 19));
	gr_urect(item->w1 + item->x, item->y - 1, item->w1 + item->x + item->w2, item->y + h);

	gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);

	x = item->w1 + item->x + ((item->w2 - w) / 2);

	gr_string(x, item->y, btext);
}


static int looper = 0;

void kc_drawquestion(kc_item* item)
{
	int c, x, w, h, aw;

	gr_get_string_size("?", &w, &h, &aw);

	c = BM_XRGB(21, 0, 24);

	gr_setcolor(gr_fade_table[fades[looper] * 256 + c]);
	looper++;
	if (looper > 63) looper = 0;

	gr_urect(item->w1 + item->x, item->y - 1, item->w1 + item->x + item->w2, item->y + h);

	gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);

	x = item->w1 + item->x + ((item->w2 - w) / 2);

	gr_string(x, item->y, "?");
}

void kc_change_key(kc_item* item)
{
	int i, n, f, k;
	uint8_t keycode;

	gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);

	gr_string(0x8000, INFO_Y, TXT_PRESS_NEW_KEY);

	bool old_event_state = are_events_enabled();
	set_events_enabled(true);

	game_flush_inputs();
	keycode = 255;
	k = 255;

	while ((k != KEY_ESC) && (keycode == 255)) 
	{
		plat_do_events();
#ifdef NETWORK
		if ((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME) && (!Endlevel_sequence))
			multi_menu_poll();
#endif

		k = key_inkey();
		timer_delay(10); //[ISB] TODO find something here
		kc_drawquestion(item);

		while (!event_queue.empty())
		{
			plat_event ev = event_queue.front(); event_queue.pop();

			if (ev.source == EventSource::Keyboard && ev.down && strlen(key_text[ev.inputnum]) > 0)
			{
				f = 0;
				for (n = 0; n < sizeof(system_keys); n++)
				{
					if (system_keys[n] == ev.inputnum)
						f = 1;
				}

				if (!f)
				{
					keycode = ev.inputnum;
					break;
				}
			}
		}

		plat_present_canvas(*kconfig_canvas, ASPECT_4_3);
	}

	if (k != KEY_ESC) 
	{
		for (i = 0; i < Num_items; i++) 
		{
			n = item - All_items;
			if ((i != n) && (All_items[i].type == BT_KEY) && (All_items[i].value == keycode)) 
			{
				All_items[i].value = -32768;
				kc_drawitem(&All_items[i], 0);
			}
		}
		item->value = keycode;
	}
	kc_drawitem(item, 1);
	gr_set_fontcolor(BM_XRGB(28, 28, 28), BM_XRGB(0, 0, 0));

	nm_restore_background(0, INFO_Y, 310, grd_curcanv->cv_font->ft_h);

	set_events_enabled(old_event_state);
	game_flush_inputs();

}

void kc_change_joybutton(kc_item* item)
{
	int n, i, k;
	int extra = 0;
	uint8_t code;

	gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);

	gr_string(0x8000, INFO_Y, TXT_PRESS_NEW_JBUTTON);

	bool old_event_state = are_events_enabled();
	set_events_enabled(true);

	game_flush_inputs();
	code = 255;
	k = 255;
	while ((k != KEY_ESC) && (code == 255)) 
	{
		plat_do_events();
#ifdef NETWORK
		if ((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME) && (!Endlevel_sequence))
			multi_menu_poll();
#endif
		k = key_inkey();
		timer_delay(10);

		if (k == KEY_PRINT_SCREEN)
			save_screen_shot(0);

		kc_drawquestion(item);

		while (!event_queue.empty())
		{
			plat_event ev = event_queue.front(); event_queue.pop();

			if (ev.source == EventSource::Joystick && ev.down && ev.handle == Kconfig_joy_binding_handle)
			{
				code = ev.inputnum;
				if (ev.flags & EV_FLAG_AXIS)
					extra = KC_BUTTON_TYPE_AXIS;
				else if (ev.flags & EV_FLAG_HAT)
				{
					//have to find which bit was active...
					int mask = 8;
					int bitnum = 3;
					while (mask > 0)
					{
						if (ev.inputnum & mask)
						{
							code = bitnum;
							break;
						}
						mask >>= 1;
						bitnum--;
					}
					extra = KC_BUTTON_TYPE_HAT;
				}
			}
		}

		//else if (Config_control_type == CONTROL_FLIGHTSTICK_PRO) 
		{
			for (i = 4; i < 20; i++)
			{
				if (joy_get_button_state(i)) 
				{
					code = i;
					mprintf((0, "JB: %d\n", code));
				}
			}
		}
		plat_present_canvas(*kconfig_canvas, ASPECT_4_3);
	}
	if (code != 255) 
	{
		for (i = 0; i < Num_items; i++) 
		{
			n = item - All_items;
			if ((i != n) && (All_items[i].type == BT_JOY_BUTTON) && (All_items[i].value == code) 
				&& All_items[i].extra == extra) 
			{
				All_items[i].value = -32768;
				kc_drawitem(&All_items[i], 0);
			}
		}
		item->value = code;
		item->extra = extra;
	}
	kc_drawitem(item, 1);
	nm_restore_background(0, INFO_Y, 310, grd_curcanv->cv_font->ft_h);
	set_events_enabled(old_event_state);
	game_flush_inputs();
}

void kc_change_mousebutton(kc_item* item)
{
	int n, i, b, k;
	uint8_t code;

	gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);

	gr_string(0x8000, INFO_Y, TXT_PRESS_NEW_MBUTTON);

	bool old_event_state = are_events_enabled();
	set_events_enabled(true);

	game_flush_inputs();
	code = 255;
	k = 255;
	while ((k != KEY_ESC) && (code == 255)) 
	{
		plat_do_events();
#ifdef NETWORK
		if ((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME) && (!Endlevel_sequence))
			multi_menu_poll();
#endif
		k = key_inkey();
		timer_delay(10);

		if (k == KEY_PRINT_SCREEN)
			save_screen_shot(0);

		kc_drawquestion(item);
		while (!event_queue.empty())
		{
			plat_event ev = event_queue.front(); event_queue.pop();

			if (ev.source == EventSource::Mouse && ev.down)
			{
				code = ev.inputnum;
				break;
			}
		}

		plat_present_canvas(*kconfig_canvas, ASPECT_4_3);
	}
	if (code != 255) 
	{
		for (i = 0; i < Num_items; i++) 
		{
			n = item - All_items;
			if ((i != n) && (All_items[i].type == BT_MOUSE_BUTTON) && (All_items[i].value == code))
			{
				All_items[i].value = -32768;
				kc_drawitem(&All_items[i], 0);
			}
		}
		item->value = code;
	}
	kc_drawitem(item, 1);
	nm_restore_background(0, INFO_Y, 310, grd_curcanv->cv_font->ft_h);
	set_events_enabled(old_event_state);
	game_flush_inputs();

}

void kc_change_joyaxis(kc_item* item)
{
	int n, i, k;
	uint8_t code;

	gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);

	gr_string(0x8000, INFO_Y, TXT_MOVE_NEW_JOY_AXIS);

	game_flush_inputs();
	code = 255;
	k = 255;

	//joystick_read_raw_axis(JOY_ALL_AXIS, old_axis);
	std::vector<int> old_axis, axis;
	if (!joy_get_axis_state(Kconfig_joy_binding_handle, old_axis))
	{
		nm_restore_background(0, INFO_Y, 310, grd_curcanv->cv_font->ft_h);
		game_flush_inputs();
		return;
	}

	while ((k != KEY_ESC) && (code == 255)) 
	{
		plat_do_events();
#ifdef NETWORK
		if ((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME) && (!Endlevel_sequence))
			multi_menu_poll();
#endif
		k = key_inkey();
		timer_delay(10);

		if (k == KEY_PRINT_SCREEN)
			save_screen_shot(0);

		kc_drawquestion(item);

		if (!joy_get_axis_state(Kconfig_joy_binding_handle, axis))
		{
			nm_restore_background(0, INFO_Y, 310, grd_curcanv->cv_font->ft_h);
			game_flush_inputs();
			return;
		}

		for (i = 0; i < old_axis.size(); i++)
		{
			if (abs(axis[i] - old_axis[i]) > 20)
			{
				code = i;
			}
			old_axis[i] = axis[i];
		}

		plat_present_canvas(*kconfig_canvas, ASPECT_4_3);
	}
	if (code != 255) 
	{
		for (i = 0; i < Num_items; i++) 
		{
			n = item - All_items;
			if ((i != n) && (All_items[i].type == BT_JOY_AXIS) && (All_items[i].value == code))
			{
				All_items[i].value = -32768;
				kc_drawitem(&All_items[i], 0);
			}
		}

		item->value = code;
	}
	kc_drawitem(item, 1);
	nm_restore_background(0, INFO_Y, 310, grd_curcanv->cv_font->ft_h);
	game_flush_inputs();
}

void kc_change_mouseaxis(kc_item* item)
{
	int i, n, k;
	uint8_t code;
	int dx, dy;

	gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);

	gr_string(0x8000, INFO_Y, TXT_MOVE_NEW_MSE_AXIS);

	game_flush_inputs();
	code = 255;
	k = 255;

	mouse_get_delta(&dx, &dy);

	while ((k != KEY_ESC) && (code == 255)) 
	{
		plat_do_events();
#ifdef NETWORK
		if ((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME) && (!Endlevel_sequence))
			multi_menu_poll();
#endif

		k = key_inkey();
		timer_delay(10);

		if (k == KEY_PRINT_SCREEN)
			save_screen_shot(0);

		kc_drawquestion(item);

		mouse_get_delta(&dx, &dy);
		if (abs(dx) > 20) code = 0;
		if (abs(dy) > 20)	code = 1;
		plat_present_canvas(*kconfig_canvas, ASPECT_4_3);
	}
	if (code != 255) 
	{
		for (i = 0; i < Num_items; i++) 
		{
			n = item - All_items;
			if ((i != n) && (All_items[i].type == BT_MOUSE_AXIS) && (All_items[i].value == code)) 
			{
				All_items[i].value = 255;
				kc_drawitem(&All_items[i], 0);
			}
		}
		item->value = code;
	}
	kc_drawitem(item, 1);
	nm_restore_background(0, INFO_Y, 310, grd_curcanv->cv_font->ft_h);
	game_flush_inputs();
}


void kc_change_invert(kc_item* item)
{
	game_flush_inputs();

	if (item->value)
		item->value = 0;
	else
		item->value = 1;

	kc_drawitem(item, 1);
}

#include "screens.h"

void kconfig_reset_joystick_bindings(kc_joyinfo& info)
{
	for (int i = 0; i < NUM_OTHER_CONTROLS; i++)
	{
		//kc_mouse[i].value = ;
		if (kc_joystick[i].type == BT_INVERT)
		{
			kc_joystick[i].value = info.axises[kc_joystick[i].id].invert;
		}
		else if (kc_joystick[i].type == BT_JOY_AXIS)
		{
			kc_joystick[i].value = info.axises[kc_joystick[i].id].axis;
		}
		else
		{
			kc_joystick[i].value = info.buttons[kc_joystick[i].id].button1;
			kc_joystick[i].extra = info.buttons[kc_joystick[i].id].type1;
		}
	}
}

void kconfig(KConfigMode control_mode, const char* title)
{
	int i;
	kc_joyinfo* joyinfo = nullptr;

	if (control_mode == KConfigMode::Joystick)
	{
		for (i = 0; i < current_registered_joysticks.size(); i++)
		{
			kc_joyinfo* check = &current_registered_joysticks[i];

			if (check->handle == Kconfig_joy_binding_handle)
			{
				joyinfo = check;
				break;
			}
		}

		if (!joyinfo)
		{
			nm_messagebox(TXT_ERROR, 1, TXT_OK, "Unable to find selected joystick.\nIt may have been disconnected");
			return;
		}

		kconfig_reset_joystick_bindings(*joyinfo);
	}

	set_screen_mode(SCREEN_MENU);
	kconfig_canvas = gr_create_canvas(320, 200);

	kc_set_controls();

	switch (control_mode)
	{
	case KConfigMode::Keyboard:
		kconfig_sub(kc_keyboard, NUM_KEY_CONTROLS, title);
		break;
	case KConfigMode::Joystick:
		kconfig_sub(kc_joystick, NUM_OTHER_CONTROLS, title);
		break;
	case KConfigMode::Mouse:
		kconfig_sub(kc_mouse, NUM_OTHER_CONTROLS, title);

		for (i = 0; i < NUM_OTHER_CONTROLS; i++)
		{
			if (kc_mouse[i].type == BT_INVERT)
				current_mouse_axises[kc_mouse[i].id].invert = kc_mouse[i].value;
			else if (kc_mouse[i].type == BT_MOUSE_AXIS)
				current_mouse_axises[kc_mouse[i].id].axis = kc_mouse[i].value;
			else
				current_mouse_bindings[kc_mouse[i].id].button1 = kc_mouse[i].value;
		}
		break;
	default:
		Int3();
		gr_free_canvas(kconfig_canvas); //TODO: I really need RAII
		return;
	}

	reset_cockpit();		//force cockpit redraw next time

	// Update save values...

	for (i = 0; i < NUM_KEY_CONTROLS; i++)
	{
		if (kc_keyboard[i].bindalt)
			current_keyboard_bindings[kc_keyboard[i].id].button2 = kc_keyboard[i].value;
		else
			current_keyboard_bindings[kc_keyboard[i].id].button1 = kc_keyboard[i].value;
	}

	for (i = 0; i < NUM_OTHER_CONTROLS; i++)
	{
		if (kc_mouse[i].type == BT_INVERT)
			current_mouse_axises[kc_mouse[i].id].invert = kc_mouse[i].value;
		else if (kc_mouse[i].type == BT_MOUSE_AXIS)
			current_mouse_axises[kc_mouse[i].id].axis = kc_mouse[i].value;
		else
			current_mouse_bindings[kc_mouse[i].id].button1 = kc_mouse[i].value;
	}

	if (control_mode == KConfigMode::Joystick && joyinfo)
	{
		for (i = 0; i < NUM_OTHER_CONTROLS; i++)
		{
			//The joystick may have been disconnected at this point, but the bindings will remain in the player file
			if (kc_joystick[i].type == BT_INVERT)
				joyinfo->axises[kc_joystick[i].id].invert = kc_joystick[i].value;
			else if (kc_joystick[i].type == BT_JOY_AXIS)
				joyinfo->axises[kc_joystick[i].id].axis = kc_joystick[i].value;
			else
			{
				joyinfo->buttons[kc_joystick[i].id].button1 = kc_joystick[i].value;
				joyinfo->buttons[kc_joystick[i].id].type1 = kc_joystick[i].extra;
			}
		}
	}

	gr_free_canvas(kconfig_canvas);
}

fix Last_angles_p = 0;
fix Last_angles_b = 0;
fix Last_angles_h = 0;
uint8_t Last_angles_read = 0;

int VR_sense_range[3] = { 25, 50, 75 };

void read_head_tracker()
{
}

#define	PH_SCALE	8
#define	JOYSTICK_READ_TIME	(F1_0/10)		//	Read joystick at 10 Hz.
fix	LastReadTime = 0;

fix	joy_axis[4];

fix max_magnitude(fix a, fix b)
{
	if (abs(a) >= abs(b))
		return a;

	return b;
}

void kconfig_clear_down_counts()
{
	for (int i = 0; i < KC_NUM_CONTROLS; i++)
	{
		control_down_count[i].down_count = 0;
	}
}

void kconfig_flush_inputs()
{
	for (int i = 0; i < KC_NUM_CONTROLS; i++)
	{
		control_down_count[i].num_inputs_down = 0;
	}
}

//Joysticks are more complicated since there can be many joysticks attached,
//and inputs can be axises or hats
bool kc_check_joystick_event(plat_event& ev, CtrlType& control)
{
	if (Kconfig_use_joystick)
	{
		for (kc_joyinfo& info : current_registered_joysticks)
		{
			if (info.handle == ev.handle)
			{
				for (int i = 0; i < KC_NUM_CONTROLS; i++)
				{
					if (ev.flags & EV_FLAG_AXIS)
					{
						if ((info.buttons[i].type1 == KC_BUTTON_TYPE_AXIS && info.buttons[i].button1 == ev.inputnum)
							|| (info.buttons[i].type2 == KC_BUTTON_TYPE_AXIS && info.buttons[i].button2 == ev.inputnum))
						{
							control = (CtrlType)i;
							return true;
						}
					}
					else if (ev.flags & EV_FLAG_HAT)
					{
						if ((info.buttons[i].type1 == KC_BUTTON_TYPE_HAT && info.buttons[i].button1 == ev.inputnum)
							|| (info.buttons[i].type2 == KC_BUTTON_TYPE_HAT && info.buttons[i].button2 == ev.inputnum))
						{
							control = (CtrlType)i;
							return true;
						}
					}
					else
					{
						if ((info.buttons[i].type1 == KC_BUTTON_TYPE_BUTTON && info.buttons[i].button1 == ev.inputnum)
							|| (info.buttons[i].type2 == KC_BUTTON_TYPE_BUTTON && info.buttons[i].button2 == ev.inputnum))
						{
							control = (CtrlType)i;
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

bool kc_check_keyboard_event(plat_event& ev, CtrlType& control)
{
	for (int i = 0; i < KC_NUM_CONTROLS; i++)
	{
		if (ev.inputnum == current_keyboard_bindings[i].button1 ||
			ev.inputnum == current_keyboard_bindings[i].button2)
		{
			control = (CtrlType)i;
			return true;
		}
	}

	return false;
}

bool kc_check_mouse_event(plat_event& ev, CtrlType& control)
{
	if (Kconfig_use_mouse)
	{
		for (int i = 0; i < KC_NUM_CONTROLS; i++)
		{
			if (ev.inputnum == current_mouse_bindings[i].button1 ||
				ev.inputnum == current_mouse_bindings[i].button2)
			{
				control = (CtrlType)i;
				return true;
			}
		}
	}

	return false;
}


void kconfig_process_events()
{
	while (!event_queue.empty())
	{
		plat_event ev = event_queue.front(); event_queue.pop();

		bool found = false;
		CtrlType control = CtrlType::PitchForward;

		switch (ev.source)
		{
		case EventSource::Keyboard:
			found = kc_check_keyboard_event(ev, control);
			break;
		case EventSource::Joystick:
			found = kc_check_joystick_event(ev, control);
			break;
		case EventSource::Mouse:
			found = kc_check_mouse_event(ev, control);
			break;
		}

		if (found)
		{
			if (ev.down)
			{
				//For key ramping, if this is the first time the key was down, record the time
				if (control_down_count[(int)control].num_inputs_down == 0)
					control_down_count[(int)control].down_time = timer_get_fixed_seconds();

				control_down_count[(int)control].num_inputs_down++;
				control_down_count[(int)control].down_count++;
			}
			else
			{
				control_down_count[(int)control].num_inputs_down--;

				//Clamp the decrement. This can occasionally go negative if you hold a button, enter the menu, close it, and then release it
				//since the controls were flushed during this time.
				if (control_down_count[(int)control].num_inputs_down < 0)
					control_down_count[(int)control].num_inputs_down = 0;
			}
		}
	}
}

//Reads the axis for control controlnum, with the specified deadzone, and adjusting for sensitivity. 
fix check_axis(kc_joyinfo& info, std::vector<int>& axises, AxisType controlnum, int deadzone, bool apply_sensitivity)
{
	int axis_num = info.axises[(int)controlnum].axis;
	if (axis_num < 0) return 0;
	fix scaled_value = axises[axis_num];

	//Scale the reading based on the deadzone
	if (scaled_value > deadzone)
		scaled_value = ((scaled_value - deadzone) * 128) / (128 - deadzone);
	else if (scaled_value < -deadzone)
		scaled_value = ((scaled_value + deadzone) * 128) / (128 - deadzone);
	else
		scaled_value = 0;

	//Then scale to FrameTime
	scaled_value = (scaled_value * FrameTime) / 128;

	//Invert it
	if (info.axises[(int)controlnum].invert)
		scaled_value = -scaled_value;

	//Now scale based on sensitivity
	if (apply_sensitivity)
		return scaled_value * Config_joystick_sensitivity / 8;

	return scaled_value;
}

void controls_read_joystick(kc_joyinfo& info, control_info& controls, bool slide_on, bool bank_on)
{
	std::span<JoystickButton> buttons;
	std::vector<int> axises;
	std::span<int> hats;

	if (Kconfig_use_joystick && joy_get_state(info.handle, axises, buttons, hats))
	{
		//Read axis inputs
		//Read pitch
		fix temp = -check_axis(info, axises, AxisType::Pitch, 10, !slide_on);

		if (!slide_on)
			controls.pitch_time += temp;
		else
			controls.vertical_thrust_time += temp;

		//Read vertical thrust
		controls.vertical_thrust_time += check_axis(info, axises, AxisType::SlideUD, 10, false);

		//Read heading
		temp = check_axis(info, axises, AxisType::Yaw, 10, !slide_on);

		if (!slide_on && !bank_on)
			controls.heading_time += temp;
		else if (slide_on)
			controls.sideways_thrust_time += temp;

		//Read sideways thrust
		controls.sideways_thrust_time += check_axis(info, axises, AxisType::SlideLR, 10, false);

		//Read banking
		if (bank_on)
			controls.bank_time += temp;

		controls.bank_time += check_axis(info, axises, AxisType::Roll, 10, true);

		//Read throttle
		controls.forward_thrust_time += -check_axis(info, axises, AxisType::Throttle, 20, false);
	}
}

fix check_mouse_axis(fix axises[], AxisType controlnum, bool apply_sensitivity)
{
	int axis_num = current_mouse_axises[(int)controlnum].axis;
	if (axis_num < 0) return 0;

	fix scaled_value = axises[axis_num];

	//Invert it
	if (current_mouse_axises[(int)controlnum].invert)
		scaled_value = -scaled_value;

	//Now scale based on sensitivity
	if (apply_sensitivity)
		return scaled_value * Config_joystick_sensitivity / 8;

	return scaled_value;
}

void controls_read_mouse(control_info& controls, bool slide_on, bool bank_on)
{
	int dx, dy;
	fix mouse_axis[2];

	if (!Kconfig_use_mouse) return;

	mouse_get_delta(&dx, &dy);
	mouse_axis[0] = (dx * FrameTime) / 35;
	mouse_axis[1] = (dy * FrameTime) / 25;

	//Read pitch
	fix temp = -check_mouse_axis(mouse_axis, AxisType::Pitch, !slide_on);

	if (!slide_on)
		controls.pitch_time += temp;
	else
		controls.vertical_thrust_time += temp;

	//Read vertical_thrust_time
	controls.vertical_thrust_time += check_mouse_axis(mouse_axis, AxisType::SlideUD, false);

	//Read heading
	temp = check_mouse_axis(mouse_axis, AxisType::Yaw, !slide_on);

	if (!slide_on && !bank_on)
		controls.heading_time += temp;
	else if (slide_on)
		controls.sideways_thrust_time += temp;

	//Read sideways_thrust_time
	controls.sideways_thrust_time += check_mouse_axis(mouse_axis, AxisType::SlideLR, false);

	//Read bank
	if (bank_on)
		controls.bank_time += temp;

	controls.bank_time += check_mouse_axis(mouse_axis, AxisType::Roll, true);

	//Read thrust
	controls.forward_thrust_time += check_mouse_axis(mouse_axis, AxisType::Throttle, true);
}

//Reads a ramped binary input for the control specified in control.
//The result equals speed_factor * time_held_down / divisor if the control is currently engaged.
constexpr fix scaled_reading(CtrlType control, int speed_factor, fix ctime, int divisor = 1)
{
	if (control >= CtrlType::NumCtrls || control < CtrlType::PitchForward)
		return 0;
	if (control_down_count[(int)control].num_inputs_down > 0)
		return speed_factor * (ctime - control_down_count[(int)control].down_time) / divisor;

	return 0;
}

void controls_read_all()
{
	fix ctime;
	int use_mouse, use_joystick = 0;
	int speed_factor = 1;

	if (Game_turbo_mode)
		speed_factor = 2;

	{
		fix temp = Controls.heading_time;
		fix temp1 = Controls.pitch_time;
		memset(&Controls, 0, sizeof(control_info));
		Controls.heading_time = temp;
		Controls.pitch_time = temp1;
	}
	bool slide_on = false;
	bool bank_on = false;

	ctime = timer_get_fixed_seconds();

	kconfig_clear_down_counts();
	kconfig_process_events();

	//------------- Read slide_on -------------
	slide_on = control_down_count[(int)CtrlType::SlideOn].num_inputs_down > 0;

	//------------- Read bank_on ---------------
	bank_on = control_down_count[(int)CtrlType::BankOn].num_inputs_down > 0;

	//------------ Read pitch_time -----------
	if (!slide_on)
	{
		fix kp = scaled_reading(CtrlType::PitchForward, speed_factor, ctime, 2 * PH_SCALE);
		kp -= scaled_reading(CtrlType::PitchBackward, speed_factor, ctime, 2 * PH_SCALE);

		if (kp == 0)
			Controls.pitch_time = 0;
		else if (kp > 0) 
		{
			if (Controls.pitch_time < 0)
				Controls.pitch_time = 0;
		}
		else // kp < 0
			if (Controls.pitch_time > 0)
				Controls.pitch_time = 0;
		Controls.pitch_time += kp;
	}
	else 
	{
		Controls.pitch_time = 0;
	}


	//----------- Read vertical_thrust_time -----------------

	if (slide_on) 
	{
		Controls.vertical_thrust_time += scaled_reading(CtrlType::PitchForward, speed_factor, ctime);
		Controls.vertical_thrust_time -= scaled_reading(CtrlType::PitchBackward, speed_factor, ctime);
	}

	Controls.vertical_thrust_time += scaled_reading(CtrlType::SlideUp, speed_factor, ctime);
	Controls.vertical_thrust_time -= scaled_reading(CtrlType::SlideDown, speed_factor, ctime);

	//---------- Read heading_time -----------
	if (!slide_on && !bank_on) 
	{
		//mprintf((0, "heading: %7.3f %7.3f: %7.3f\n", f2fl(k4), f2fl(k6), f2fl(Controls.heading_time)));
		fix kh = -scaled_reading(CtrlType::TurnLeft, speed_factor, ctime, PH_SCALE);
		kh += scaled_reading(CtrlType::TurnRight, speed_factor, ctime, PH_SCALE);

		if (kh == 0)
			Controls.heading_time = 0;
		else if (kh > 0) 
		{
			if (Controls.heading_time < 0)
				Controls.heading_time = 0;
		}
		else // kh < 0
			if (Controls.heading_time > 0)
				Controls.heading_time = 0;
		Controls.heading_time += kh;
	}
	else
	{
		Controls.heading_time = 0;
	}

	//----------- Read sideways_thrust_time -----------------
	if (slide_on)
	{
		Controls.sideways_thrust_time -= scaled_reading(CtrlType::TurnLeft, speed_factor, ctime);
		Controls.sideways_thrust_time += scaled_reading(CtrlType::TurnRight, speed_factor, ctime);
	}

	// From keyboard...
	Controls.sideways_thrust_time -= scaled_reading(CtrlType::SlideLeft, speed_factor, ctime);
	Controls.sideways_thrust_time += scaled_reading(CtrlType::SlideRight, speed_factor, ctime);

	//----------- Read bank_time -----------------
	if (bank_on) 
	{
		Controls.bank_time += scaled_reading(CtrlType::TurnLeft, speed_factor, ctime);
		Controls.bank_time -= scaled_reading(CtrlType::TurnRight, speed_factor, ctime);
	}

	Controls.bank_time += scaled_reading(CtrlType::BankLeft, speed_factor, ctime);
	Controls.bank_time -= scaled_reading(CtrlType::BankRight, speed_factor, ctime);


	//----------- Read forward_thrust_time -------------
	Controls.forward_thrust_time += scaled_reading(CtrlType::Accelerate, speed_factor, ctime);
	Controls.forward_thrust_time -= scaled_reading(CtrlType::Reverse, speed_factor, ctime);

	//Add analog inputs later
	for (kc_joyinfo& info : current_registered_joysticks)
		controls_read_joystick(info, Controls, slide_on, bank_on);

	controls_read_mouse(Controls, slide_on, bank_on);

	//----------- Read fire_primary_down_count
	Controls.fire_primary_down_count = control_down_count[(int)CtrlType::FirePrimary].down_count;
	Controls.fire_primary_state = control_down_count[(int)CtrlType::FirePrimary].num_inputs_down != 0;

	//----------- Read fire_secondary_down_count
	Controls.fire_secondary_down_count = control_down_count[(int)CtrlType::FireSecondary].down_count;
	Controls.fire_secondary_state = control_down_count[(int)CtrlType::FireSecondary].num_inputs_down != 0;

	//----------- Read fire_flare_down_count
	Controls.fire_flare_down_count = control_down_count[(int)CtrlType::FireFlare].down_count;

	//----------- Read drop_bomb_down_count
	Controls.drop_bomb_down_count = control_down_count[(int)CtrlType::DropBomb].down_count;

	//----------- Read rear_view_down_count
	Controls.rear_view_down_count = control_down_count[(int)CtrlType::RearView].down_count != 0;
	Controls.rear_view_down_state = control_down_count[(int)CtrlType::RearView].num_inputs_down != 0;

	//----------- Read automap_down_count
	Controls.automap_down_count = control_down_count[(int)CtrlType::Automap].down_count;
	Controls.automap_state = control_down_count[(int)CtrlType::Automap].num_inputs_down != 0;

	//----------- Read stupid-cruise-control-type of throttle.
	{
		Cruise_speed += fixdiv(scaled_reading(CtrlType::CruiseFaster, speed_factor, ctime) * 5, FrameTime);
		Cruise_speed -= fixdiv(scaled_reading(CtrlType::CruiseSlower, speed_factor, ctime) * 5, FrameTime);

		if (control_down_count[(int)CtrlType::CruiseOff].num_inputs_down)
			Cruise_speed = 0;

		if (Cruise_speed > i2f(100)) Cruise_speed = i2f(100);
		if (Cruise_speed < 0) Cruise_speed = 0;

		if (Controls.forward_thrust_time == 0)
			Controls.forward_thrust_time = fixmul(Cruise_speed, FrameTime) / 100;
	}

	read_head_tracker();

	//----------- Clamp values between -FrameTime and FrameTime
	if (FrameTime > F1_0)
		mprintf((1, "Bogus frame time of %.2f seconds\n", f2fl(FrameTime)));

	if (Controls.pitch_time > FrameTime / 2) Controls.pitch_time = FrameTime / 2;
	if (Controls.vertical_thrust_time > FrameTime) Controls.vertical_thrust_time = FrameTime;
	if (Controls.heading_time > FrameTime) Controls.heading_time = FrameTime;
	if (Controls.sideways_thrust_time > FrameTime) Controls.sideways_thrust_time = FrameTime;
	if (Controls.bank_time > FrameTime) Controls.bank_time = FrameTime;
	if (Controls.forward_thrust_time > FrameTime) Controls.forward_thrust_time = FrameTime;
#ifdef RESTORE_AFTERBURNER
	if (Controls.afterburner_time > FrameTime ) Controls.afterburner_time = FrameTime;
#endif

	if (Controls.pitch_time < -FrameTime / 2) Controls.pitch_time = -FrameTime / 2;
	if (Controls.vertical_thrust_time < -FrameTime) Controls.vertical_thrust_time = -FrameTime;
	if (Controls.heading_time < -FrameTime) Controls.heading_time = -FrameTime;
	if (Controls.sideways_thrust_time < -FrameTime) Controls.sideways_thrust_time = -FrameTime;
	if (Controls.bank_time < -FrameTime) Controls.bank_time = -FrameTime;
	if (Controls.forward_thrust_time < -FrameTime) Controls.forward_thrust_time = -FrameTime;
#ifdef RESTORE_AFTERBURNER
	if (Controls.afterburner_time < -FrameTime ) Controls.afterburner_time = -FrameTime;
#endif


	//--------- Don't do anything if in debug mode
#ifndef NDEBUG
	if (keyd_pressed[KEY_DELETE])
	{
		memset(&Controls, 0, sizeof(control_info));
	}
#endif
}

void reset_cruise(void)
{
	Cruise_speed = 0;
}


void kc_set_controls()
{
	int i;

	for (i = 0; i < NUM_KEY_CONTROLS; i++)
	{
		if (kc_keyboard[i].bindalt)
			kc_keyboard[i].value = current_keyboard_bindings[kc_keyboard[i].id].button2;
		else
			kc_keyboard[i].value = current_keyboard_bindings[kc_keyboard[i].id].button1;
	}

	//why did I add support for any number of joysticks again?
	/*for (i = 0; i < NUM_OTHER_CONTROLS; i++) 
	{
		kc_joystick[i].value = kconfig_settings[Config_control_type][i];
		if (kc_joystick[i].type == BT_INVERT) 
		{
			if (kc_joystick[i].value != 1)
				kc_joystick[i].value = 0;
			kconfig_settings[Config_control_type][i] = kc_joystick[i].value;
		}
	}*/

	for (i = 0; i < NUM_OTHER_CONTROLS; i++) 
	{
		//kc_mouse[i].value = ;
		if (kc_mouse[i].type == BT_INVERT) 
		{
			kc_mouse[i].value = current_mouse_axises[kc_mouse[i].id].invert;
		}
		else if (kc_mouse[i].type == BT_MOUSE_AXIS)
		{
			kc_mouse[i].value = current_mouse_axises[kc_mouse[i].id].axis;
		}
		else 
		{
			kc_mouse[i].value = current_mouse_bindings[kc_mouse[i].id].button1;
		}
	}
}

void kconfig_reset_joy_buttons(kc_joyinfo& info)
{
	info.buttons.clear();
	info.axises.clear();

	info.initialized = false;

	for (kc_button_binding& binding : default_joystick_buttons)
		info.buttons.push_back(binding);

	for (kc_axis_binding& binding : default_joystick_axises)
		info.axises.push_back(binding);
}

void kconfig_register_device(int handle, joy_guid guid)
{
	//Find if there's already a joystick with this guid, that doesn't have a registered handle

	for (kc_joyinfo& info : current_registered_joysticks)
	{
		if (!memcmp(info.guid, guid.guid, 16) && info.handle == -1)
		{
			info.handle = handle;
			return;
		}
	}

	//Not there so add a new one
	kc_joyinfo newinfo; memcpy(newinfo.guid, guid.guid, 16); newinfo.handle = handle;
	kconfig_reset_joy_buttons(newinfo);
	current_registered_joysticks.push_back(newinfo);
}

void kconfig_unregister_device(int handle, joy_guid guid)
{
	for (kc_joyinfo& info : current_registered_joysticks)
	{
		if (!memcmp(info.guid, guid.guid, 16) && info.handle == handle)
		{
			//Retain the device in the list, since I still need to save its bindings later. 
			info.handle = -1;

			//Flush events. This is a bit brute-force, but controls would get stuck if you pulled a stick that had a pending release event
			flush_events();
			kconfig_flush_inputs();
			return;
		}
	}
}

void kconfig_reset_keybinds()
{
	current_keyboard_bindings.clear();

	for (kc_button_binding& binding : default_keyboard_controls)
		current_keyboard_bindings.push_back(binding);

	current_mouse_bindings.clear();

	for (kc_button_binding& binding : default_mouse_buttons)
		current_mouse_bindings.push_back(binding);

	for (kc_axis_binding& binding : default_mouse_axises)
		current_mouse_axises.push_back(binding);

	for (kc_joyinfo& joystick : current_registered_joysticks)
		kconfig_reset_joy_buttons(joystick);
}

void kconfig_init_defaults()
{
	joy_set_device_callbacks(kconfig_register_device, kconfig_unregister_device);

	//This is hardcoded at the moment, but it might support loading defaults from elsewhere later. 
	default_keyboard_controls[(int)CtrlType::PitchForward].button1 = KEY_UP;
	default_keyboard_controls[(int)CtrlType::PitchForward].button2 = KEY_PAD8;
	default_keyboard_controls[(int)CtrlType::PitchBackward].button1 = KEY_DOWN;
	default_keyboard_controls[(int)CtrlType::PitchBackward].button2 = KEY_PAD2;
	default_keyboard_controls[(int)CtrlType::TurnLeft].button1 = KEY_LEFT;
	default_keyboard_controls[(int)CtrlType::TurnLeft].button2 = KEY_PAD4;
	default_keyboard_controls[(int)CtrlType::TurnRight].button1 = KEY_RIGHT;
	default_keyboard_controls[(int)CtrlType::TurnRight].button2 = KEY_PAD6;

	default_keyboard_controls[(int)CtrlType::SlideOn].button1 = KEY_LALT;
	default_keyboard_controls[(int)CtrlType::SlideLeft].button2 = KEY_PAD1;
	default_keyboard_controls[(int)CtrlType::SlideRight].button2 = KEY_PAD3;
	default_keyboard_controls[(int)CtrlType::SlideUp].button2 = KEY_PADMINUS;
	default_keyboard_controls[(int)CtrlType::SlideDown].button2 = KEY_PADPLUS;

	default_keyboard_controls[(int)CtrlType::BankLeft].button1 = KEY_Q;
	default_keyboard_controls[(int)CtrlType::BankLeft].button2 = KEY_PAD7;
	default_keyboard_controls[(int)CtrlType::BankRight].button1 = KEY_E;
	default_keyboard_controls[(int)CtrlType::BankRight].button2 = KEY_PAD9;

	default_keyboard_controls[(int)CtrlType::FirePrimary].button1 = KEY_LCTRL;
	default_keyboard_controls[(int)CtrlType::FirePrimary].button2 = KEY_RCTRL;
	default_keyboard_controls[(int)CtrlType::FireSecondary].button1 = KEY_SPACEBAR;
	default_keyboard_controls[(int)CtrlType::FireFlare].button1 = KEY_F;
	default_keyboard_controls[(int)CtrlType::DropBomb].button1 = KEY_B;

	default_keyboard_controls[(int)CtrlType::RearView].button1 = KEY_R;
	default_keyboard_controls[(int)CtrlType::Automap].button1 = KEY_TAB;

	default_keyboard_controls[(int)CtrlType::Accelerate].button1 = KEY_A;
	default_keyboard_controls[(int)CtrlType::Reverse].button1 = KEY_Z;

	default_joystick_buttons[(int)CtrlType::FirePrimary].button1 = 0;
	default_joystick_buttons[(int)CtrlType::FireSecondary].button1 = 1;
	default_joystick_buttons[(int)CtrlType::FireFlare].button1 = 2;
	default_joystick_buttons[(int)CtrlType::DropBomb].button1 = 3;

	default_joystick_buttons[(int)CtrlType::SlideUp].button1 = 0; 
	default_joystick_buttons[(int)CtrlType::SlideUp].type1 = KC_BUTTON_TYPE_HAT;
	default_joystick_buttons[(int)CtrlType::SlideRight].button1 = 1; 
	default_joystick_buttons[(int)CtrlType::SlideRight].type1 = KC_BUTTON_TYPE_HAT;
	default_joystick_buttons[(int)CtrlType::SlideDown].button1 = 2; 
	default_joystick_buttons[(int)CtrlType::SlideDown].type1 = KC_BUTTON_TYPE_HAT;
	default_joystick_buttons[(int)CtrlType::SlideLeft].button1 = 3; 
	default_joystick_buttons[(int)CtrlType::SlideLeft].type1 = KC_BUTTON_TYPE_HAT;

	default_joystick_axises[(int)AxisType::Pitch].axis = 1;
	default_joystick_axises[(int)AxisType::Yaw].axis = 0;

	default_mouse_buttons[(int)CtrlType::FirePrimary].button1 = 0;
	default_mouse_buttons[(int)CtrlType::FireSecondary].button1 = 1;

	default_mouse_axises[(int)AxisType::Pitch].axis = 1;
	default_mouse_axises[(int)AxisType::Pitch].invert = true;
	default_mouse_axises[(int)AxisType::Yaw].axis = 0;

	//temp
	//default_joystick_axises[(int)AxisType::Throttle].axis = 1;
	//default_joystick_axises[(int)AxisType::SlideLR].axis = 0;
	//default_joystick_buttons[(int)CtrlType::FirePrimary].button2 = 5;
	//default_joystick_buttons[(int)CtrlType::FirePrimary].flags |= KC_BUTTON_B2_AXIS;

	kconfig_reset_keybinds();
}

//Code for serializing the kconfig state to NBT tags
CompoundTag* kc_create_compound_for_binding(kc_button_binding& binding)
{
	CompoundTag* newtag = new CompoundTag();

	newtag->list.push_back(std::make_unique<ShortTag>("Button1", binding.button1));
	newtag->list.push_back(std::make_unique<ShortTag>("Button2", binding.button2));
	newtag->list.push_back(std::make_unique<ByteTag>("Type1", binding.type1));
	newtag->list.push_back(std::make_unique<ByteTag>("Type2", binding.type2));

	return newtag;
}

void kc_binding_from_compound(kc_button_binding& binding, CompoundTag& tag)
{
	Tag* bindtag_p = tag.find_tag("Button1");
	if (bindtag_p)
		binding.button1 = nbt_get_integral(bindtag_p, binding.button1);
	bindtag_p = tag.find_tag("Button2");
	if (bindtag_p)
		binding.button2 = nbt_get_integral(bindtag_p, binding.button2);
	bindtag_p = tag.find_tag("Type1");
	if (bindtag_p)
		binding.type1 = nbt_get_integral(bindtag_p, binding.type1);
	bindtag_p = tag.find_tag("Type2");
	if (bindtag_p)
		binding.type2 = nbt_get_integral(bindtag_p, binding.type2);
}

CompoundTag* kc_create_compound_for_axis(kc_axis_binding& binding)
{
	CompoundTag* newtag = new CompoundTag();

	newtag->list.push_back(std::make_unique<ShortTag>("Axis", binding.axis));
	newtag->list.push_back(std::make_unique<ByteTag>("Invert", binding.invert));

	return newtag;
}

void kc_axis_from_compound(kc_axis_binding& binding, CompoundTag& tag)
{
	binding.axis = nbt_get_integral(tag.find_tag("Axis"), binding.axis);
	binding.invert = nbt_get_integral(tag.find_tag("Invert"), binding.invert);
}

CompoundTag* kc_create_compound_for_joystick(kc_joyinfo& joyinfo)
{
	CompoundTag* newtag = new CompoundTag();

	newtag->list.push_back(std::make_unique<ByteArrayTag>("GUID", joyinfo.guid, sizeof(joyinfo.guid)));

	ListTag* innertag_p = new ListTag("Bindings");

	for (kc_button_binding& binding : joyinfo.buttons)
		innertag_p->put_tag(kc_create_compound_for_binding(binding));

	newtag->list.push_back(std::unique_ptr<Tag>(innertag_p));

	innertag_p = new ListTag("Axises");

	for (kc_axis_binding& binding : joyinfo.axises)
		innertag_p->put_tag(kc_create_compound_for_axis(binding));

	newtag->list.push_back(std::unique_ptr<Tag>(innertag_p));

	return newtag;
}

void kc_joystick_from_compound(CompoundTag& tag)
{
	Tag* tag_p = tag.find_tag("GUID");
	if (!tag_p || tag_p->GetType() != NBTTag::ByteArray)
		return;
	ByteArrayTag* bytearray_p = (ByteArrayTag*)tag_p;
	if (bytearray_p->array.size() != 16)
		return;

	kc_joyinfo* found_joystick = nullptr;
	//Find a joystick that isn't initialized and has this guid
	for (kc_joyinfo& joystick : current_registered_joysticks)
	{
		if (!memcmp(bytearray_p->array.data(), joystick.guid, 16) && !joystick.initialized)
		{
			found_joystick = &joystick;
		}
	}

	//Didn't find a joystick?
	if (!found_joystick)
	{
		current_registered_joysticks.emplace_back();
		found_joystick = &current_registered_joysticks[current_registered_joysticks.size() - 1];
		memcpy(found_joystick->guid, bytearray_p->array.data(), 16);
	}

	found_joystick->initialized = true;
	kconfig_reset_joy_buttons(*found_joystick);

	tag_p = tag.find_tag("Bindings");
	if (tag_p && tag_p->GetType() == NBTTag::List) //If these conditions fail, the 
	{
		ListTag* listtag_p = (ListTag*)tag_p;
		if (listtag_p->get_list_type() == NBTTag::Compound)
		{
			int count = std::min(listtag_p->size(), found_joystick->buttons.size());
			for (int i = 0; i < count; i++)
				kc_binding_from_compound(found_joystick->buttons[i], (CompoundTag&)*listtag_p->at(i));
		}
	}

	tag_p = tag.find_tag("Axises");
	if (tag_p && tag_p->GetType() == NBTTag::List)
	{
		ListTag* listtag_p = (ListTag*)tag_p;
		if (listtag_p->get_list_type() == NBTTag::Compound)
		{
			int count = std::min(listtag_p->size(), found_joystick->axises.size());
			for (int i = 0; i < count; i++)
				kc_axis_from_compound(found_joystick->axises[i], (CompoundTag&)*listtag_p->at(i));
		}
	}
}

void kc_read_bindings_from_controlinfo_tag(CompoundTag& compoundtag_p)
{
	kconfig_reset_keybinds();
	Tag* tag_p = compoundtag_p.find_tag("Keyboard_bindings");

	if (tag_p && tag_p->GetType() == NBTTag::List)
	{
		ListTag* listtag_p = (ListTag*)tag_p;
		if (listtag_p->get_list_type() == NBTTag::Compound)
		{
			int count = std::min(listtag_p->size(), current_keyboard_bindings.size());
			for (int i = 0; i < count; i++)
				kc_binding_from_compound(current_keyboard_bindings[i], (CompoundTag&)*listtag_p->at(i));
		}
	}

	tag_p = compoundtag_p.find_tag("Mouse_buttons");

	if (tag_p && tag_p->GetType() == NBTTag::List)
	{
		ListTag* listtag_p = (ListTag*)tag_p;
		if (listtag_p->get_list_type() == NBTTag::Compound)
		{
			int count = std::min(listtag_p->size(), current_mouse_bindings.size());
			for (int i = 0; i < count; i++)
				kc_binding_from_compound(current_mouse_bindings[i], (CompoundTag&)*listtag_p->at(i));
		}
	}

	tag_p = compoundtag_p.find_tag("Mouse_axises");

	if (tag_p && tag_p->GetType() == NBTTag::List)
	{
		ListTag* listtag_p = (ListTag*)tag_p;
		if (listtag_p->get_list_type() == NBTTag::Compound)
		{
			int count = std::min(listtag_p->size(), current_mouse_axises.size());
			for (int i = 0; i < count; i++)
				kc_axis_from_compound(current_mouse_axises[i], (CompoundTag&)*listtag_p->at(i));
		}
	}

	tag_p = compoundtag_p.find_tag("Joystick_bindings");

	if (tag_p && tag_p->GetType() == NBTTag::List)
	{
		ListTag* listtag_p = (ListTag*)tag_p;
		if (listtag_p->get_list_type() == NBTTag::Compound)
		{
			for (int i = 0; i < listtag_p->size(); i++)
				kc_joystick_from_compound((CompoundTag&)*listtag_p->at(i));
		}
	}

	Kconfig_use_mouse = nbt_get_integral(compoundtag_p.find_tag("Mouse_enabled"), 0);
	Kconfig_use_joystick = nbt_get_integral(compoundtag_p.find_tag("Joystick_enabled"), 0);
	Kconfig_use_gamepad = nbt_get_integral(compoundtag_p.find_tag("Gamepad_enabled"), 0);
}

CompoundTag* kc_create_controlinfo_tag()
{
	CompoundTag* tag = new CompoundTag("Control_info");

	std::unique_ptr<ListTag> tag_p = std::make_unique<ListTag>("Keyboard_bindings");

	for (kc_button_binding& binding : current_keyboard_bindings)
		tag_p->put_tag(kc_create_compound_for_binding(binding));

	tag->list.push_back(std::move(tag_p));

	std::unique_ptr<ListTag> mousetag_p = std::make_unique<ListTag>("Mouse_buttons");

	for (kc_button_binding& binding : current_mouse_bindings)
		mousetag_p->put_tag(kc_create_compound_for_binding(binding));

	tag->list.push_back(std::move(mousetag_p));

	std::unique_ptr<ListTag> mouseaxistag_p = std::make_unique<ListTag>("Mouse_axises");

	for (kc_axis_binding& binding : current_mouse_axises)
		mouseaxistag_p->put_tag(kc_create_compound_for_axis(binding));

	tag->list.push_back(std::move(mouseaxistag_p));

	std::unique_ptr<ListTag> joysticktag_p = std::make_unique<ListTag>("Joystick_bindings");

	for (kc_joyinfo& joystick : current_registered_joysticks)
		joysticktag_p->put_tag(kc_create_compound_for_joystick(joystick));

	tag->list.push_back(std::move(joysticktag_p));

	std::unique_ptr<ByteTag> bytetag_p = std::make_unique<ByteTag>("Mouse_enabled", Kconfig_use_mouse);
	tag->list.push_back(std::move(bytetag_p));
	bytetag_p = std::make_unique<ByteTag>("Joystick_enabled", Kconfig_use_joystick);
	tag->list.push_back(std::move(bytetag_p));
	bytetag_p = std::make_unique<ByteTag>("Gamepad_enabled", Kconfig_use_gamepad);
	tag->list.push_back(std::move(bytetag_p));

	return tag;
}

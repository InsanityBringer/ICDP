/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/

#pragma once

#include <queue>

enum class EventSource
{
	Keyboard,
	Mouse,
	Joystick,
	Gamepad
};

//This event is a hat event. 
//The hat number is obtained with inputnum / 4.
//The hat bit can be obtained with 1 << (inputnum % 4)
constexpr int EV_FLAG_HAT = 1; 
//This event is an axis event.
//Axises can create a button-like event when their value crosses a threshold.
constexpr int EV_FLAG_AXIS = 2;

struct plat_event
{
	EventSource source;
	int handle; //Needed for joysticks
	int flags; //Flags about this event
	int inputnum; //Button number of the event, or KEY_ scancode for keyboards.
	bool down; //True if the button was pressed for this event, or false if released.
};

extern std::queue<plat_event> event_queue;

//TEMP: Needed only while the game loop isn't unified. 
bool are_events_enabled();
void set_events_enabled(bool state);
void flush_events();

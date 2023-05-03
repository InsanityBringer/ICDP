#include <stdio.h>
#include <algorithm>
#include <vector>
#include <span>
#include "platform/joy.h"
#include "platform/timer.h"
#include "misc/error.h"
#include "platform/mono.h"

#ifdef USE_SDL

#include "SDL_joystick.h"
#include "SDL_gamecontroller.h"
#include "SDL_events.h"

bool usingGamepad = false; //Gamepads need special handling to contort into the Descent Joystick API

fix last_read_time;

class JoystickInfo
{
	SDL_Joystick* m_Joystick;

	int m_AxisCount;
	//Buffer for reading all the axises into. 
	int* m_Axises;

	int m_HatCount;
	//Buffer for reading all the hat states into.
	int* m_HatStates;

	int m_ButtonCount;
	JoystickButton* m_ButtonStates;

	//GUID for identifying specific joysticks
	SDL_JoystickGUID m_guid;

	SDL_JoystickID m_InstanceID;

public:
	JoystickInfo(SDL_Joystick* joystick);
	JoystickInfo(const JoystickInfo& other);
	~JoystickInfo();

	//TODO: This should be automatic with RAII, but will require a different approach
	void Free()
	{
		if (m_Joystick)
			SDL_JoystickClose(m_Joystick);
	}

	void Read(fix delta);
	void Flush();

	SDL_JoystickID InstanceID() const
	{
		return m_InstanceID;
	}

	SDL_Joystick* Joystick() const
	{
		return m_Joystick;
	}

	void GetData(std::span<int>& axises, std::span<JoystickButton>& buttons, std::span<int>& hats)
	{
		axises = std::span<int>(m_Axises, m_AxisCount);
		buttons = std::span<JoystickButton>(m_ButtonStates, m_ButtonCount);
		hats = std::span<int>(m_HatStates, m_HatCount);
	}
};

JoystickInfo::JoystickInfo(SDL_Joystick* joystick)
{
	m_Joystick = joystick;

	m_guid = SDL_JoystickGetGUID(joystick);
	m_AxisCount = SDL_JoystickNumAxes(joystick);
	m_Axises = nullptr;
	if (m_AxisCount > 0) //I wonder what happens if you plug in a DDR pad... (update my pad has 5 axises)
	{
		m_Axises = new int[m_AxisCount];
		memset(m_Axises, 0, sizeof(*m_Axises) * m_AxisCount);
	}

	m_HatCount = SDL_JoystickNumHats(joystick);
	m_HatStates = nullptr;
	if (m_HatCount > 0)
	{
		m_HatStates = new int[m_HatCount];
		memset(m_HatStates, 0, sizeof(*m_HatStates) * m_HatCount);
	}

	m_ButtonCount = SDL_JoystickNumButtons(joystick);
	m_ButtonStates = nullptr;
	if (m_ButtonCount > 0)
	{
		m_ButtonStates = new JoystickButton[m_ButtonCount];
		memset(m_ButtonStates, 0, sizeof(*m_ButtonStates) * m_ButtonCount);
	}

	m_InstanceID = SDL_JoystickInstanceID(joystick);
}

JoystickInfo::JoystickInfo(const JoystickInfo& other)
{
	m_Joystick = other.m_Joystick;
	m_guid = other.m_guid;
	m_AxisCount = other.m_AxisCount;
	m_Axises = nullptr;
	if (m_AxisCount > 0) //I wonder what happens if you plug in a DDR pad... (update my pad has 5 axises)
	{
		m_Axises = new int[m_AxisCount];
		memset(m_Axises, 0, sizeof(*m_Axises) * m_AxisCount);
	}

	m_HatCount = other.m_HatCount;
	m_HatStates = nullptr;
	if (m_HatCount > 0)
	{
		m_HatStates = new int[m_HatCount];
		memset(m_HatStates, 0, sizeof(*m_HatStates) * m_HatCount);
	}

	m_ButtonCount = other.m_ButtonCount;
	m_ButtonStates = nullptr;
	if (m_ButtonCount > 0)
	{
		m_ButtonStates = new JoystickButton[m_ButtonCount];
		memset(m_ButtonStates, 0, sizeof(*m_ButtonStates) * m_ButtonCount);
	}

	m_InstanceID = other.m_InstanceID;
}

JoystickInfo::~JoystickInfo()
{
	if (m_Axises)
		delete[] m_Axises;

	if (m_HatStates)
		delete[] m_HatStates;

	if (m_ButtonStates)
		delete[] m_ButtonStates;

	//TODO: I can't free this like this with RAII due to the potential of JoystickInfo being copied. 
	//if (m_Joystick)
	//	SDL_JoystickClose(m_Joystick);
}

void JoystickInfo::Read(fix delta)
{
	//Read axises
	for (int i = 0; i < m_AxisCount; i++)
		m_Axises[i] = SDL_JoystickGetAxis(m_Joystick, i) * 127 / 32727;

	//Read hats
	for (int i = 0; i < m_HatCount; i++)
		m_HatStates[i] = SDL_JoystickGetHat(m_Joystick, i);

	//Read buttons
	for (int i = 0; i < m_ButtonCount; i++)
	{
		bool down = SDL_JoystickGetButton(m_Joystick, i);

		if (down)
		{
			if (m_ButtonStates[i].down)
			{
				m_ButtonStates[i].down_time += delta;
			}
			else
			{
				m_ButtonStates[i].down = true;
				m_ButtonStates[i].down_count++;
			}
		}
		else
		{
			if (m_ButtonStates[i].down)
			{
				m_ButtonStates[i].down = false;
				m_ButtonStates[i].up_count++;
				m_ButtonStates[i].down_time = 0;
			}
		}
	}
}

void JoystickInfo::Flush()
{
	for (int i = 0; i < m_ButtonCount; i++)
	{
		m_ButtonStates[i].down = false;
		m_ButtonStates[i].down_count = m_ButtonStates[i].up_count = m_ButtonStates[i].down_time = 0;
	}
}

int numJoysticks;
std::vector<JoystickInfo> sticks;
std::vector<SDL_GameController*> controllers;
SDL_GameController *controller;

void I_InitSDLJoysticks()
{
	//Currently connected controllers get a joystick attached event later down the line,
	//so this isn't needed anymore. 
	/*int i;
	numJoysticks = SDL_NumJoysticks();
	if (numJoysticks == 0) return; 

	//Special gamepad handling
	/*if (SDL_IsGameController(0))
	{
		usingGamepad = true;
		controller = SDL_GameControllerOpen(0);
		if (!controller)
		{
			Warning("I_InitSDLJoysticks: Failed to open game controller: %s\n", SDL_GetError());
		}
	}
	else
	{
		for (i = 0; i < numJoysticks; i++)
		{
			SDL_Joystick* joystick = SDL_JoystickOpen(i);
			if (!joystick)
			{
				Warning("I_InitSDLJoysticks: Failed to open joystick %d: %s\n", i, SDL_GetError());
			}

			sticks.emplace_back(joystick);
		}
	}*/
}

#define joybtn(x) (1 << x)
int rawToDescentMapping[] = { joybtn(1) | joybtn(6), joybtn(2) | joybtn(5), joybtn(3) | joybtn(7), joybtn(4) | joybtn(9), joybtn(5),
							joybtn(10), joybtn(11), joybtn(13), joybtn(14), joybtn(15), joybtn(17), joybtn(18), joybtn(19)};

void I_JoystickHandler()
{
	fix read_time = timer_get_fixed_seconds();
	fix delta = read_time - last_read_time;
	last_read_time = read_time;

	for (JoystickInfo& info : sticks)
	{
		info.Read(read_time);
	}
}

void I_ControllerHandler()
{
	if (!controller || !usingGamepad) return;
	int axisState[4];
	int buttons = 0;
	axisState[0] = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) * 127 / 32767;
	axisState[1] = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) * 127 / 32767;
	axisState[2] = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX) * 127 / 32767;
	axisState[3] = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY) * 127 / 32767;

	//These numbers start at 4. Gamepads are currently treated as a Flightstick Pro, which doesn't support buttons 1-4
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A))
		buttons |= joybtn(11);
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B))
		buttons |= joybtn(15);
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X))
		buttons |= joybtn(17);
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_Y))
		buttons |= joybtn(19);
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
		buttons |= joybtn(12);
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP))
		buttons |= joybtn(20);
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
		buttons |= joybtn(16);
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
		buttons |= joybtn(8);
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))
		buttons |= joybtn(7);
	if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
		buttons |= joybtn(9);
	//XInput trigger hack
	if (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > (32767 / 3))
		buttons |= joybtn(5);
	if (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > (32767 / 3))
		buttons |= joybtn(6);

	//printf("btns: %d axises: %d %d %d %d\n", buttons, axisState[0], axisState[1], axisState[2], axisState[3]);
	JoystickInput(buttons, axisState, JOY_ALL_AXIS);
}

joy_device_callback attached_callback, removed_callback;

void joy_set_device_callbacks(joy_device_callback attached, joy_device_callback removed)
{
	attached_callback = attached;
	removed_callback = removed;
}

void plat_joystick_attached(int device_num)
{
	mprintf((0, "plat_joystick_attached: device num %d attached\n", device_num));

	SDL_Joystick* newJoystick = SDL_JoystickOpen(device_num);
	if (!newJoystick)
		Warning("plat_joystick_attached: Failed to open joystick %d: %s\n", device_num, SDL_GetError());
	else
	{
		sticks.emplace_back(newJoystick);
		if (attached_callback)
		{
			SDL_JoystickGUID guid = SDL_JoystickGetGUID(newJoystick);

			attached_callback(SDL_JoystickInstanceID(newJoystick), *(joy_guid*)&guid);
		}
	}
}

void plat_joystick_detached(int device_num)
{
	mprintf((0, "plat_joystick_detached: device instance %d detached\n", device_num));

	auto it = sticks.begin();

	while (it < sticks.end())
	{
		if (it->InstanceID() == device_num)
		{
			if (removed_callback)
			{
				SDL_JoystickGUID guid = SDL_JoystickGetGUID(it->Joystick());
				removed_callback(it->InstanceID(), *(joy_guid*)&guid);
			}
			it->Free();
			sticks.erase(it);
			return;
		}

		it++;
	}
}

void joy_flush()
{
	for (JoystickInfo& info : sticks)
	{
		info.Flush();
	}
}

bool joy_get_state(int handle, std::span<int>& axises, std::span<JoystickButton>& buttons, std::span<int>& hats)
{
	for (JoystickInfo& info : sticks)
	{
		if (info.InstanceID() == handle)
		{
			info.GetData(axises, buttons, hats);
			return true;
		}
	}
	return false;
}

#endif

/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/

#include "event.h"
#include "key.h"

std::queue<plat_event> event_queue;
bool events_enabled = false;

bool are_events_enabled()
{
	return events_enabled;
}

void set_events_enabled(bool state)
{
	events_enabled = state;
	if (!events_enabled)
		flush_events();
}

void flush_events()
{
	while (!event_queue.empty())
		event_queue.pop();
}

bool event_available()
{
	return !event_queue.empty();
}

void pop_event(plat_event& ev)
{
	if (!event_queue.empty())
	{
		ev = event_queue.front();
		event_queue.pop();
	}
}

int event_to_keycode(plat_event& ev)
{
	if (ev.source != EventSource::Keyboard || !ev.down)
		return 0;

	int key = ev.inputnum;
	if (ev.flags & EV_FLAG_SHIFTED)
		key |= KEY_SHIFTED;
	if (ev.flags & EV_FLAG_CTRLED)
		key |= KEY_CTRLED;
	if (ev.flags & EV_FLAG_ALTED)
		key |= KEY_ALTED;
	if (ev.flags & EV_FLAG_DEBUGGED)
		key |= KEY_DEBUGGED;

	return key;
}

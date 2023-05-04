/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/

#include "event.h"

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

/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/

#include "gameinfo.h"

const char* temp_current_game_name = "Descent";

const char* gameinfo_get_current_game_prefix()
{
	return temp_current_game_name;
}

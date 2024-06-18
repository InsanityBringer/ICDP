/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

#include "inferno.h"
#include "defs/definitions.h"
#include "gamedefs.h"

definition_list gamedefs_list(true);

void gamedefs_init()
{
	gamedefs_list.register_type("robot", SCHEMA_ROBOT);
	gamedefs_list.register_type("weapon", SCHEMA_WEAPON);
	gamedefs_list.register_type("powerup", SCHEMA_POWERUP);
	gamedefs_list.register_type("reactor", SCHEMA_REACTOR);
	gamedefs_list.register_type("ship", SCHEMA_SHIP);
	gamedefs_list.register_type("soundinfo", SCHEMA_SOUNDINFO);
	gamedefs_list.register_type("playerweapon", SCHEMA_PLAYERWEAPON);
	gamedefs_list.register_type("model", SCHEMA_MODEL);
	//gamedefs_list.register_type("physics", SHCMEA_PHYSICS); //Will I actually use? TBD
	//gamedefs_list.read_defs_from_lump("constest.txt");
}

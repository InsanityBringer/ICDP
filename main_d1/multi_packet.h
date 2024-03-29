/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

#pragma once

#ifdef NETWORK
#include "netmisc.h"

struct test_packet
{
	NETMISC_DECLARE_DATA
	NETMISC_DECLARE_MULTI_MESSAGE

	uint8_t f1;
	int f2;
	short f3;
	short f4[5];
};

struct multi_fire_packet
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
	uint8_t gun_num;
	uint8_t laser_level;
	uint8_t flags;
	uint8_t fired;
	short track_target;
};

struct multi_destroy_reactor
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	short objnum;
	uint8_t player_num;
};

struct multi_start_endlevel
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
	uint8_t secret;
};

struct multi_player_death
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
	uint8_t primary_weapon_flags;
	uint8_t secondary_weapon_flags;
	uint8_t laser_level;
	uint8_t num_homing;
	uint8_t num_concussion;
	uint8_t num_smart;
	uint8_t num_mega;
	uint8_t num_prox;
	short num_vulcan_bullets;
	int player_flags;
	uint8_t num_created;
	short create_objnum[MAX_NET_CREATE_OBJECTS];
};

struct multi_message_packet
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
	char message[MAX_MESSAGE_LEN];
};

struct multi_reappear
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	short objnum;
};

struct multi_position
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	shortpos pos;
};

struct multi_kill
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
	short objnum;
	int8_t owner;
};

struct multi_remove_object
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	short objnum;
	int8_t owner;
};

struct multi_quit
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
};

struct multi_segnum_sidenum
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	short segnum;
	int8_t sidenum;
};

struct multi_reactor_fire
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	vms_vector to_dest;
	int8_t gun_num;
	short objnum;
};

struct multi_create_powerup
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
	uint8_t powerup_type;
	short segnum;
	short objnum;
	vms_vector pos;
};

struct multi_play_sound
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
	uint8_t sound_num;
	uint8_t volume;
};

struct multi_score_value
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
	int score;
};

struct multi_triggernum
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
	uint8_t trigger_num;
};

struct multi_wall_damage
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	short wallnum;
	fix damage;
};

struct multi_cloak
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
};

struct multi_decloak
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
};

struct multi_create_explosion
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
};

struct multi_player_drop
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
	uint8_t primary_weapon_flags;
	uint8_t secondary_weapon_flags;
	uint8_t laser_level;
	uint8_t num_homing;
	uint8_t num_concussion;
	uint8_t num_smart;
	uint8_t num_mega;
	uint8_t num_prox;
	short num_vulcan_bullets;
	int player_flags;
	uint8_t num_created;
	short create_objnum[MAX_NET_CREATE_OBJECTS];
};

struct multi_claim_robot
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
	short remote_objnum;
	int8_t owner;
};

struct multi_release_robot
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
	short remote_objnum;
	int8_t owner;
};

struct multi_robot_position
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
	short remote_objnum;
	int8_t owner;
	shortpos pos;
};

struct multi_robot_fire
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
	short remote_objnum;
	int8_t owner;
	int8_t gun_num;
	vms_vector fire;
};

struct multi_explode_robot
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
	short killer_objnum;
	int8_t killer_owner;
	short remote_objnum;
	int8_t owner;
};

struct multi_create_robot
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
	int8_t station;
	short objnum;
	uint8_t type;
};

struct multi_boss_did_thing
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
	short objnum;
	uint8_t action;
	uint8_t secondary;
	short gateobjnum;
	short gateseg;
};

struct multi_drop_robot_loot
{
	NETMISC_DECLARE_DATA
		NETMISC_DECLARE_MULTI_MESSAGE;

	uint8_t packet_type;
	uint8_t player_num;
	int8_t contains_count;
	int8_t contains_type;
	int8_t contains_id;
	short segnum;
	vms_vector pos;
	short objnums[MAX_ROBOT_POWERUPS];
};
#endif

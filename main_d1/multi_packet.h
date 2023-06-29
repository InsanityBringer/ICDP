/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

#pragma once

#include "netmisc.h"

struct test_packet
{
	NETMISC_DECLARE_DATA

	uint8_t f1;
	int f2;
	short f3;
	short f4[5];
};

struct multi_fire_packet
{
	NETMISC_DECLARE_DATA;

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
	NETMISC_DECLARE_DATA;

	uint8_t packet_type;
	short objnum;
	uint8_t player_num;
};

struct multi_start_endlevel
{
	NETMISC_DECLARE_DATA;

	uint8_t packet_type;
	uint8_t player_num;
	uint8_t secret;
};

struct multi_player_death
{
	NETMISC_DECLARE_DATA;

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
	NETMISC_DECLARE_DATA;

	uint8_t packet_type;
	uint8_t player_num;
	char message[MAX_MESSAGE_LEN];
};

struct multi_reappear
{
	NETMISC_DECLARE_DATA;

	uint8_t packet_type;
	short objnum;
};

struct multi_position
{
	NETMISC_DECLARE_DATA;

	uint8_t packet_type;
	shortpos pos;
};

struct multi_kill
{
	NETMISC_DECLARE_DATA;

	uint8_t packet_type;
	uint8_t player_num;
	short objnum;
	int8_t owner;
};

struct multi_remove_object
{
	NETMISC_DECLARE_DATA;

	uint8_t packet_type;
	short objnum;
	int8_t owner;
};

struct multi_playernum
{
	NETMISC_DECLARE_DATA;

	uint8_t packet_type;
	uint8_t player_num;
};

struct multi_segnum_sidenum
{
	NETMISC_DECLARE_DATA;

	uint8_t packet_type;
	short segnum;
	int8_t sidenum;
};

struct multi_reactor_fire
{
	NETMISC_DECLARE_DATA;

	uint8_t packet_type;
	vms_vector to_dest;
	int8_t gun_num;
	short objnum;
};

struct multi_create_powerup
{
	NETMISC_DECLARE_DATA;

	uint8_t packet_type;
	uint8_t player_num;
	uint8_t powerup_type;
	short segnum;
	short objnum;
	vms_vector pos;
};

struct multi_play_sound
{
	NETMISC_DECLARE_DATA;

	uint8_t packet_type;
	uint8_t player_num;
	uint8_t sound_num;
	uint8_t volume;
};

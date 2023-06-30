/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

#include "multi_packet.h"
#include "misc/error.h"

NETMISC_DEFINE_DATA(test_packet)

NETMISC_DEFINE_FIELD(f1, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(f2, netmisc_field_type::INT32)
NETMISC_DEFINE_FIELD(f3, netmisc_field_type::INT16)
NETMISC_DEFINE_ARRAY(f4, netmisc_field_type::INT16, 5)

NETMISC_END_DATA;

void multi_test_packet_serialization()
{
	uint8_t buf[64];

	uint8_t t1 = rand() & 255;
	short t3 = rand() & 32767;
	int t2 = rand() | (rand() << 15);

	test_packet packet;
	short t4[5];
	for (int i = 0; i < 5; i++)
		packet.f4[i] = t4[i] = rand();

	packet.f1 = t1; packet.f2 = t2; packet.f3 = t3;
	packet.to_buf(buf, 0, 64);
	test_packet packet2;
	packet2.from_buf(buf, 0, 64);

	Assert(packet2.f1 == t1);
	Assert(packet2.f2 == t2);
	Assert(packet2.f3 == t3);

	for (int i = 0; i < 5; i++)
		Assert(packet2.f4[i] == t4[i]);

	multi_fire_packet fire_packet;
	size_t test = fire_packet.buf_size();

	//Int3();
}

NETMISC_DEFINE_DATA(multi_fire_packet)

NETMISC_DEFINE_FIELD(packet_type, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(player_num, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(gun_num, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(laser_level, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(flags, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(fired, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(track_target, netmisc_field_type::INT16)

NETMISC_END_DATA;

NETMISC_DEFINE_DATA(multi_destroy_reactor)

NETMISC_DEFINE_FIELD(packet_type, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(objnum, netmisc_field_type::INT16)
NETMISC_DEFINE_FIELD(player_num, netmisc_field_type::BYTE)

NETMISC_END_DATA;

NETMISC_DEFINE_DATA(multi_start_endlevel)

NETMISC_DEFINE_FIELD(packet_type, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(player_num, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(secret, netmisc_field_type::BYTE)

NETMISC_END_DATA;

NETMISC_DEFINE_DATA(multi_player_death)

NETMISC_DEFINE_FIELD(packet_type, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(player_num, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(primary_weapon_flags, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(secondary_weapon_flags, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(laser_level, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(num_homing, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(num_concussion, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(num_smart, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(num_mega, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(num_prox, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(num_vulcan_bullets, netmisc_field_type::INT16)
NETMISC_DEFINE_FIELD(player_flags, netmisc_field_type::INT32)
NETMISC_DEFINE_FIELD(num_created, netmisc_field_type::BYTE)
NETMISC_DEFINE_ARRAY(create_objnum, netmisc_field_type::INT16, MAX_NET_CREATE_OBJECTS)

NETMISC_END_DATA;

NETMISC_DEFINE_DATA(multi_message_packet)

NETMISC_DEFINE_FIELD(packet_type, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(player_num, netmisc_field_type::BYTE)
NETMISC_DEFINE_ARRAY(message, netmisc_field_type::BYTE, MAX_NET_CREATE_OBJECTS)

NETMISC_END_DATA;

NETMISC_DEFINE_DATA(multi_reappear)

NETMISC_DEFINE_FIELD(packet_type, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(objnum, netmisc_field_type::INT16)

NETMISC_END_DATA;

NETMISC_DEFINE_DATA(multi_position)

NETMISC_DEFINE_FIELD(packet_type, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(pos, netmisc_field_type::SHORTPOS)

NETMISC_END_DATA;

NETMISC_DEFINE_DATA(multi_kill)

NETMISC_DEFINE_FIELD(packet_type, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(player_num, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(objnum, netmisc_field_type::INT16)
NETMISC_DEFINE_FIELD(owner, netmisc_field_type::BYTE)

NETMISC_END_DATA;

NETMISC_DEFINE_DATA(multi_remove_object)

NETMISC_DEFINE_FIELD(packet_type, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(objnum, netmisc_field_type::INT16)
NETMISC_DEFINE_FIELD(owner, netmisc_field_type::BYTE)

NETMISC_END_DATA;

NETMISC_DEFINE_DATA(multi_playernum)

NETMISC_DEFINE_FIELD(packet_type, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(player_num, netmisc_field_type::BYTE)

NETMISC_END_DATA;

NETMISC_DEFINE_DATA(multi_segnum_sidenum)

NETMISC_DEFINE_FIELD(packet_type, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(segnum, netmisc_field_type::INT16)
NETMISC_DEFINE_FIELD(sidenum, netmisc_field_type::BYTE)

NETMISC_END_DATA;

NETMISC_DEFINE_DATA(multi_reactor_fire)

NETMISC_DEFINE_FIELD(packet_type, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(to_dest, netmisc_field_type::VECTOR)
NETMISC_DEFINE_FIELD(gun_num, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(objnum, netmisc_field_type::INT16)

NETMISC_END_DATA;

NETMISC_DEFINE_DATA(multi_create_powerup)

NETMISC_DEFINE_FIELD(packet_type, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(player_num, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(powerup_type, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(segnum, netmisc_field_type::INT16)
NETMISC_DEFINE_FIELD(objnum, netmisc_field_type::INT16)
NETMISC_DEFINE_FIELD(pos, netmisc_field_type::VECTOR)

NETMISC_END_DATA;

NETMISC_DEFINE_DATA(multi_play_sound)

NETMISC_DEFINE_FIELD(packet_type, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(player_num, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(sound_num, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(volume, netmisc_field_type::BYTE)

NETMISC_END_DATA;

NETMISC_DEFINE_DATA(multi_score_value)

NETMISC_DEFINE_FIELD(packet_type, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(player_num, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(score, netmisc_field_type::INT32)

NETMISC_END_DATA;

NETMISC_DEFINE_DATA(multi_triggernum)

NETMISC_DEFINE_FIELD(packet_type, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(player_num, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(trigger_num, netmisc_field_type::BYTE)

NETMISC_END_DATA;

NETMISC_DEFINE_DATA(multi_wall_damage)

NETMISC_DEFINE_FIELD(packet_type, netmisc_field_type::BYTE)
NETMISC_DEFINE_FIELD(wallnum, netmisc_field_type::INT16)
NETMISC_DEFINE_FIELD(damage, netmisc_field_type::INT32)

NETMISC_END_DATA;


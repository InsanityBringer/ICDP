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

	Int3();
}

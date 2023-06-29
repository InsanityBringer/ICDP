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

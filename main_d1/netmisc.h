/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#pragma once

#include <string>
#include "misc/types.h"
#include "misc/error.h"
#include "vecmat/vecmat.h"
#include "fix/fix.h"
#include "object.h"
#include "multi.h"
#include "network.h"

 //Returns a checksum of a block of memory.
extern uint16_t netmisc_calc_checksum(void* vptr, int len);

#define netmisc_encode_buffer(ptr, offset, buf, length) do { memcpy(&ptr[*offset], buf, length); *offset+=length; } while(0)
#define netmisc_decode_buffer(ptr, offset, buf, length) do { memcpy(buf, &ptr[*offset], length); *offset+=length; } while(0)

//Functions for encoding values into a block of memory.
//All values are written little-endian indepenedent of alignment. 
void netmisc_encode_int8(uint8_t* ptr, int* offset, uint8_t v);
void netmisc_encode_int16(uint8_t* ptr, int* offset, short v);
void netmisc_encode_int32(uint8_t* ptr, int* offset, int v);
void netmisc_encode_shortpos(uint8_t* ptr, int* offset, shortpos* v);
void netmisc_encode_vector(uint8_t* ptr, int* offset, vms_vector* vec);
void netmisc_encode_matrix(uint8_t* ptr, int* offset, vms_matrix* mat);

//How this interface should have been written for my encoders..
void netmisc_encode_int8(uint8_t * ptr, uint8_t v);
void netmisc_encode_int16(uint8_t * ptr, short v);
void netmisc_encode_int32(uint8_t * ptr, int v);
void netmisc_encode_shortpos(uint8_t * ptr, shortpos * v);
void netmisc_encode_vector(uint8_t * ptr, vms_vector * vec);
void netmisc_encode_matrix(uint8_t * ptr, vms_matrix * mat);

void netmisc_encode_string(uint8_t * ptr, int* offset, int buffer_size, std::string & str); 

//Functions for extracting values from a block of memory, as encoded by the above functions.
void netmisc_decode_int8(uint8_t* ptr, int* offset, uint8_t* v);
void netmisc_decode_int16(uint8_t* ptr, int* offset, short* v);
void netmisc_decode_int32(uint8_t* ptr, int* offset, int* v);
void netmisc_decode_shortpos(uint8_t* ptr, int* offset, shortpos* v);
void netmisc_decode_vector(uint8_t* ptr, int* offset, vms_vector* vec);
void netmisc_decode_matrix(uint8_t* ptr, int* offset, vms_matrix* mat);

//How this interface should have been written...
void netmisc_decode_int8(uint8_t* ptr, uint8_t* v);
void netmisc_decode_int16(uint8_t * ptr, short* v);
void netmisc_decode_int32(uint8_t * ptr, int* v);
void netmisc_decode_shortpos(uint8_t * ptr, shortpos * v);
void netmisc_decode_vector(uint8_t * ptr, vms_vector * vec);
void netmisc_decode_matrix(uint8_t * ptr, vms_matrix * mat);

//Packet length parameter is used to validate that the string can be safely decoded. 
std::string netmisc_decode_string(uint8_t* ptr, int* offset, int packet_length);

//Game-specific functions for encoding packet structures.
void netmisc_encode_netgameinfo(uint8_t* ptr, int* offset, netgame_info* info);
void netmisc_encode_sequence_packet(uint8_t* ptr, int* offset, sequence_packet* info);
void netmisc_encode_frame_info(uint8_t* ptr, int* offset, frame_info* info);
void netmisc_encode_endlevel_info(uint8_t* ptr, int* offset, endlevel_info* info);
void netmisc_encode_object(uint8_t* ptr, int* offset, object* objp);

//Game-specific functions for decoding packet structures.
void netmisc_decode_netgameinfo(uint8_t * ptr, int* offset, int buffer_size, netgame_info * info);
void netmisc_decode_sequence_packet(uint8_t* ptr, int* offset, sequence_packet* info);
void netmisc_decode_frame_info(uint8_t* ptr, int* offset, frame_info* info);
void netmisc_decode_endlevel_info(uint8_t* ptr, int* offset, endlevel_info* info);
void netmisc_decode_object(uint8_t* ptr, int* offset, object* objp);

//New multi packet system: Simplify encoding of packets by giving them a structure annotated with type information
//which can be used to serialize and deserialize 

enum class netmisc_field_type
{
	BYTE,
	INT16,
	INT32,
	VECTOR,
	MATRIX,
	SHORTPOS
};

struct netmisc_field
{
	const char*			name;		//Name, for debugging. 
	size_t				packoffset;	//Packed offset into the encoded buffer for the field.
	size_t				offset;		//Offset into the structure for the field.
	size_t				pack_size;	//Size of an individual element. 
	size_t				host_size;	//Size of an individual element when unpacked
	netmisc_field_type	field_type;	//Type of the field.
	size_t				count;		//Count, for array types. 
	size_t get_size()
	{
		size_t internal_size;
		switch (field_type)
		{
		case netmisc_field_type::BYTE:
			internal_size = 1;
			host_size = sizeof(uint8_t);
			break;
		case netmisc_field_type::INT16:
			internal_size = 2;
			host_size = sizeof(uint16_t);
			break;
		case netmisc_field_type::INT32:
			internal_size = 4;
			host_size = sizeof(uint32_t);
			break;
		case netmisc_field_type::VECTOR:
			internal_size = 12;
			host_size = sizeof(vms_vector);
			break;
		case netmisc_field_type::MATRIX:
			internal_size = 36;
			host_size = sizeof(vms_matrix);
			break;
		case netmisc_field_type::SHORTPOS:
			internal_size = 23;
			host_size = sizeof(shortpos);
			break;
		default:
			//Hmm.. Can't Error here since error system isn't started up..
			Int3();
			internal_size = 1;
			break;
		}

		pack_size = internal_size;
		return internal_size * count;
	}
};

class netmisc_field_list
{
	size_t packed_size;
	std::vector<netmisc_field> fields;
public:
	netmisc_field_list(std::vector<netmisc_field> v)
	{
		packed_size = 0;
		fields = v;

		//Calculate the packed offsets for each field
		for (netmisc_field& field : fields)
		{
			field.packoffset = packed_size;
			packed_size += field.get_size();
		}
	}

	size_t buf_size() const
	{
		return packed_size;
	}

	void from_buf(void* dest, uint8_t* buf, size_t offset, size_t total) const
	{
		Assert(total - offset >= packed_size);

		uint8_t* dest_buf = (uint8_t*)dest;

		for (const netmisc_field& field : fields)
		{
			for (int i = 0; i < field.count; i++)
			{
				switch (field.field_type)
				{
				case netmisc_field_type::BYTE:
					netmisc_decode_int8(&buf[offset + field.packoffset + i * field.pack_size],
						&dest_buf[field.offset + i * field.host_size]);
					break;
				case netmisc_field_type::INT16:
					netmisc_decode_int16(&buf[offset + field.packoffset + i * field.pack_size],
						(short*)&dest_buf[field.offset+ i * field.host_size]);
					break;
				case netmisc_field_type::INT32:
					netmisc_decode_int32(&buf[offset + field.packoffset + i * field.pack_size],
						(int*)&dest_buf[field.offset + i * field.host_size]);
					break;
				case netmisc_field_type::VECTOR:
					netmisc_decode_vector(&buf[offset + field.packoffset + i * field.pack_size],
						(vms_vector*)&dest_buf[field.offset + i * field.host_size]);
					break;
				case netmisc_field_type::MATRIX:
					netmisc_decode_matrix(&buf[offset + field.packoffset + i * field.pack_size],
						(vms_matrix*)&dest_buf[field.offset + i * field.host_size]);
					break;
				case netmisc_field_type::SHORTPOS:
					netmisc_decode_shortpos(&buf[offset + field.packoffset + i * field.pack_size],
						(shortpos*)&dest_buf[field.offset + i * field.host_size]);
					break;
				default:
					Error("netmisc_field_list::deserialize_from_buf: Bad field type");
					break;
				}
			}
		}
	}

	void to_buf(void* dest, uint8_t* buf, size_t offset, size_t total) const
	{
		Assert(total - offset >= packed_size);

		uint8_t* dest_buf = (uint8_t*)dest;

		for (const netmisc_field& field : fields)
		{
			for (int i = 0; i < field.count; i++)
			{
				switch (field.field_type)
				{
				case netmisc_field_type::BYTE:
					netmisc_encode_int8(&buf[offset + field.packoffset + i * field.pack_size],
						dest_buf[field.offset + i * field.host_size]);
					break;
				case netmisc_field_type::INT16:
					netmisc_encode_int16(&buf[offset + field.packoffset + i * field.pack_size],
						*(short*)&dest_buf[field.offset + i * field.host_size]);
					break;
				case netmisc_field_type::INT32:
					netmisc_encode_int32(&buf[offset + field.packoffset + i * field.pack_size],
						*(int*)&dest_buf[field.offset + i * field.host_size]);
					break;
				case netmisc_field_type::VECTOR:
					netmisc_encode_vector(&buf[offset + field.packoffset + i * field.pack_size],
						(vms_vector*)&dest_buf[field.offset + i * field.host_size]);
					break;
				case netmisc_field_type::MATRIX:
					netmisc_encode_matrix(&buf[offset + field.packoffset + i * field.pack_size],
						(vms_matrix*)&dest_buf[field.offset + i * field.host_size]);
					break;
				case netmisc_field_type::SHORTPOS:
					netmisc_encode_shortpos(&buf[offset + field.packoffset + i * field.pack_size],
						(shortpos*)&dest_buf[field.offset + i * field.host_size]);
					break;
				default:
					Error("netmisc_field_list::deserialize_from_buf: Bad field type");
					break;
				}
			}
		}
	}
};

#define NETMISC_DECLARE_DATA \
	static netmisc_field_list netmisc_fields; \
	netmisc_field_list& get_field_map() { return netmisc_fields; } \
	void to_buf(uint8_t* buf, size_t offset, size_t total); \
	void from_buf(uint8_t* buf, size_t offset, size_t total); \
	static size_t buf_size() { size_t size = netmisc_fields.buf_size(); Assert(size != 0); return size; }

#define NETMISC_DEFINE_DATA(classname) \
	static netmisc_field_list classname##_generate_field_list();\
	netmisc_field_list classname::netmisc_fields = classname##_generate_field_list(); \
	void classname::to_buf(uint8_t* buf, size_t offset, size_t total) { \
		netmisc_fields.to_buf(this, buf, offset, total); } \
	void classname::from_buf(uint8_t* buf, size_t offset, size_t total) { \
		netmisc_fields.from_buf(this, buf, offset, total); } \
	static netmisc_field_list classname##_generate_field_list() \
	{ \
		typedef classname netmisc_class_name; \
		netmisc_field_list field_list( {

#define NETMISC_DEFINE_FIELD(name, type) { #name, 0, offsetof(netmisc_class_name, name), 0, 0, type, 1 },
#define NETMISC_DEFINE_ARRAY(name, type, count) { #name, 0, offsetof(netmisc_class_name, name), 0, 0, type, count },

#define NETMISC_END_DATA \
		} ); \
		return field_list;\
	} \

//Call this after NETMISC_DEFINE_DATA/NETMISC_END_DATA
//This will associate the packet with a multi message
#define NETMISC_DECLARE_MULTI_MESSAGE static int multi_message_num;

#define NETMISC_DEFINE_MULTI_MESSAGE(classname, messagenum) \
	static int classname##_get_message_num(int num); \
	int classname::multi_message_num = classname##_get_message_num( messagenum ); \
	static int classname##_get_message_num(int num) \
	{ \
		message_length[num] = classname::buf_size(); \
		return num; \
	}
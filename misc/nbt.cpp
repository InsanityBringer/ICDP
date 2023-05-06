#include <stdexcept>
#include "nbt.h"

void nbt_read_string(FILE* fp, std::string& str)
{
	unsigned short length = file_read_short(fp);
	str.resize((size_t)length);

	for (int i = 0; i < length; i++)
	{
		str[i] = file_read_byte(fp);
	}
}

void nbt_write_string(FILE* fp, std::string& str)
{
	unsigned short length = (unsigned short)str.size();
	for (int i = 0; i < length; i++)
	{
		file_write_byte(fp, str[i]);
	}
}

Tag* Tag::get_tag_of_type(NBTTag type)
{
	Tag* tag = nullptr;

	switch (type)
	{
	case NBTTag::End:
		tag = new EndTag();
		break;
	case NBTTag::Byte:
		tag = new ByteTag();
		break;
	case NBTTag::Short:
		tag = new ShortTag();
		break;
	case NBTTag::Int:
		tag = new IntTag();
		break;
	case NBTTag::Long:
		tag = new LongTag();
		break;
	case NBTTag::Float:
		tag = new FloatTag();
		break;
	case NBTTag::Double:
		tag = new DoubleTag();
		break;
	case NBTTag::String:
		tag = new StringTag();
		break;
	case NBTTag::List:
		tag = new ListTag();
		break;
	case NBTTag::Compound:
		tag = new CompoundTag();
		break;
	default:
		throw std::runtime_error("Tag::get_tag_of_type encountered a unknown tag type");
		break;
	}

	return tag;
}

Tag* Tag::read_tag(FILE* fp)
{
	uint8_t tagtype = file_read_byte(fp);
	Tag* ptr = get_tag_of_type((NBTTag)tagtype);

	if (tagtype != 0) //End tags aren't named and have no data.
	{
		nbt_read_string(fp, ptr->name);
		ptr->read_data(fp);
	}

	return ptr;
}

void Tag::write_tag(FILE* fp)
{
	NBTTag type = GetType();
	file_write_byte(fp, (uint8_t)type);
	if (type != NBTTag::End)
	{
		nbt_write_string(fp, name);
		write_data(fp);
	}
}

#pragma once

#include <stdio.h>
#include <vector>
#include <memory>
#include <string>
#include "cfile/cfile.h"

void nbt_read_string(FILE* fp, std::string& str);
void nbt_write_string(FILE* fp, std::string& str);

enum class NBTTag
{
	End,
	Byte,
	Short,
	Int,
	Long,
	Float,
	Double,
	ByteArray,
	String,
	List,
	Compound
};

struct Tag
{
	std::string name;

	virtual NBTTag GetType() const = 0;
	virtual void read_data(FILE* fp) = 0;
	virtual void write_data(FILE* fp) = 0;

	static Tag* get_tag_of_type(NBTTag type);
	static Tag* read_tag(FILE* fp);
	void write_tag(FILE* fp);
};

struct EndTag : Tag
{
	NBTTag GetType() const override
	{
		return NBTTag::End;
	}
	void read_data(FILE* fp) override {}
	void write_data(FILE* fp) override {}
};

struct ByteTag : Tag
{
	int8_t value;
	NBTTag GetType() const override
	{
		return NBTTag::Byte;
	}

	void read_data(FILE* fp) override
	{
		value = file_read_byte(fp);
	}
	void write_data(FILE* fp) override
	{
		file_write_byte(fp, value);
	}
};

struct ShortTag : Tag
{
	int16_t value;
	NBTTag GetType() const override
	{
		return NBTTag::Short;
	}

	void read_data(FILE* fp) override
	{
		value = file_read_short(fp);
	}
	void write_data(FILE* fp) override
	{
		file_write_short(fp, value);
	}
};

struct IntTag : Tag
{
	int32_t value;
	NBTTag GetType() const override
	{
		return NBTTag::Int;
	}

	void read_data(FILE* fp) override
	{
		value = file_read_int(fp);
	}
	void write_data(FILE* fp) override
	{
		file_write_int(fp, value);
	}
};

struct LongTag : Tag
{
	int64_t value;
	NBTTag GetType() const override
	{
		return NBTTag::Long;
	}

	void read_data(FILE* fp) override
	{
		value = file_read_int64(fp);
	}
	void write_data(FILE* fp) override
	{
		file_write_int64(fp, value);
	}
};

struct FloatTag : Tag
{
	float value;
	NBTTag GetType() const override
	{
		return NBTTag::Float;
	}

	void read_data(FILE* fp) override
	{
		int t = file_read_int(fp);
		value = *(float*)&t;
	}
	void write_data(FILE* fp) override
	{
		int t = *(int*)&value;
		file_write_int(fp, t);
	}
};

struct DoubleTag : Tag
{
	double value;
	NBTTag GetType() const override
	{
		return NBTTag::Double;
	}

	void read_data(FILE* fp) override
	{
		int64_t t = file_read_int64(fp);
		value = *(double*)&t;
	}
	void write_data(FILE* fp) override
	{
		int64_t t = *(int64_t*)&value;
		file_write_int64(fp, t);
	}
};

struct ByteArrayTag : Tag
{
	std::vector<int8_t> array;
	NBTTag GetType() const override
	{
		return NBTTag::ByteArray;
	}

	void read_data(FILE* fp) override
	{
		array.clear();
		int length = file_read_int(fp);
		array.resize(length);

		for (int i = 0; i < length; i++)
		{
			array[i] = file_read_byte(fp);
		}
	}
	void write_data(FILE* fp) override
	{
		file_write_int(fp, array.size());
		for (int i = 0; i < array.size(); i++)
		{
			file_write_byte(fp, array[i]);
		}
	}
};

struct StringTag : Tag
{
	std::string value;
	NBTTag GetType() const override
	{
		return NBTTag::String;
	}

	void read_data(FILE* fp) override
	{
		nbt_read_string(fp, value);
	}
	void write_data(FILE* fp) override
	{
		nbt_write_string(fp, value);
	}
};

struct ListTag : Tag
{
	NBTTag list_type;
	std::vector<std::unique_ptr<Tag>> list;
	NBTTag GetType() const override
	{
		return NBTTag::List;
	}

	void read_data(FILE* fp) override
	{
		int8_t type = file_read_byte(fp);
		list_type = (NBTTag)type;
		list.clear();
		int length = file_read_int(fp);
		for (int i = 0; i < length; i++)
		{
			Tag* tag_p = get_tag_of_type(list_type);
			tag_p->read_data(fp);
			list.push_back(std::unique_ptr<Tag>(tag_p));
		}
	}
	void write_data(FILE* fp) override
	{
		int8_t type = (int8_t)list_type;
		file_write_int(fp, list.size());

		for (int i = 0; i < list.size(); i++)
		{
			list[i]->write_data(fp);
		}
	}
};

struct CompoundTag : Tag
{
	std::vector<std::unique_ptr<Tag>> list;
	NBTTag GetType() const override
	{
		return NBTTag::Compound;
	}

	void read_data(FILE* fp) override
	{
		for (;;)
		{
			Tag* tag_p = read_tag(fp);

			//No reason to actually store the end tag
			if (tag_p->GetType() == NBTTag::End)
				return;

			list.push_back(std::unique_ptr<Tag>(tag_p));
		}
	}
	void write_data(FILE* fp) override
	{
		EndTag end;
		for (std::unique_ptr<Tag>& tag_r : list)
		{
			tag_r->write_tag(fp);
		}

		end.write_tag(fp);
	}
};

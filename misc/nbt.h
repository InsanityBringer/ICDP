#pragma once

#include <stdio.h>
#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <stdexcept>
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

	Tag() {}
	Tag(std::string_view name) : name(name) {}

	virtual NBTTag GetType() const = 0;
	virtual void read_data(FILE* fp) = 0;
	virtual void write_data(FILE* fp) = 0;

	static Tag* get_tag_of_type(NBTTag type);
	static Tag* read_tag(FILE* fp);
	void write_tag(FILE* fp);
};

struct EndTag : public Tag
{
	NBTTag GetType() const override
	{
		return NBTTag::End;
	}
	void read_data(FILE* fp) override {}
	void write_data(FILE* fp) override {}
};

struct ByteTag : public Tag
{
	int8_t value;
	ByteTag() : value(0) {}
	ByteTag(std::string_view name, int8_t value) : Tag(name), value(value) {}

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

struct ShortTag : public Tag
{
	int16_t value;
	ShortTag() : value(0) {}
	ShortTag(std::string_view name, int16_t value) : Tag(name), value(value) {}

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

struct IntTag : public Tag
{
	int32_t value;
	IntTag() : value(0) {}
	IntTag(std::string_view name, int32_t value) : Tag(name), value(value) {}

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

struct LongTag : public Tag
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

struct FloatTag : public Tag
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

struct DoubleTag : public Tag
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

struct ByteArrayTag : public Tag
{
	std::vector<uint8_t> array;

	ByteArrayTag() {}
	ByteArrayTag(std::string_view name) : Tag(name) {}

	ByteArrayTag(uint8_t* data, int length)
	{
		array.reserve(length);
		for (int i = 0; i < length; i++)
			array.push_back(data[i]);
	}

	ByteArrayTag(std::string_view name, uint8_t* data, int length) : Tag(name)
	{
		array.reserve(length);
		for (int i = 0; i < length; i++)
			array.push_back(data[i]);
	}

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

struct StringTag : public Tag
{
	std::string value;

	StringTag() {}
	StringTag(std::string_view name, const char* str) : Tag(name), value(str) {}
	StringTag(std::string_view name, std::string_view str) : Tag(name), value(str) {}
	StringTag(std::string_view name, std::string str) : Tag(name), value(str) {}

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

//This really should be templated, but I'm not sure how to do it cleanly
//When deseralized, you'd have to compare it to a generic type to read out list_type, then cast
//to the correct templated type, which seems a bit messy, but maybe not less messy than this. 
struct ListTag : public Tag
{
private:
	//Unlike the other tags, I'm not exposing this directly to ensure the list remains the right type.
	NBTTag list_type;
	std::vector<std::unique_ptr<Tag>> list;
public:
	ListTag()
	{
		list_type = NBTTag::End;
	}
	ListTag(std::string_view name) : Tag(name)
	{
		list_type = NBTTag::End;
	}

	NBTTag get_list_type() const
	{
		return list_type;
	}

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
		file_write_byte(fp, type);
		file_write_int(fp, list.size());

		for (int i = 0; i < list.size(); i++)
		{
			list[i]->write_data(fp);
		}
	}

	void put_tag(Tag* ptr)
	{
		if (list.size() > 0)
		{
			if (ptr->GetType() != list_type)
				throw std::runtime_error("ListTag::put_tag: Tag type mismatch");
		}

		else
		{
			list_type = ptr->GetType();
		}

		list.push_back(std::unique_ptr<Tag>(ptr));
	}

	size_t size()
	{
		return list.size();
	}

	void clear()
	{
		list.clear();
	}

	void remove_at(int index)
	{
		if (index < 0 || index >= list.size())
			throw std::runtime_error("ListTag:remove_at: Index out of range");
		auto it = list.begin();
		it += index;

		list.erase(it);
	}

	Tag* at(int index)
	{
		if (index < 0 || index >= list.size())
			throw std::runtime_error("ListTag:at: Index out of range");

		return list[index].get();
	}
};

struct CompoundTag : public Tag
{
	std::vector<std::unique_ptr<Tag>> list;

	CompoundTag() {}
	CompoundTag(std::string_view name) : Tag(name) {}

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
		for (std::unique_ptr<Tag>& tag_p : list)
		{
			tag_p->write_tag(fp);
		}

		end.write_tag(fp);
	}

	Tag* find_tag(std::string_view find_name)
	{
		for (std::unique_ptr<Tag>& tag_p : list)
		{
			if (!tag_p->name.compare(find_name))
				return tag_p.get();
		}

		return nullptr;
	}
};

//Helper function to get a value as an integer (up to 32 bits, TODO 64 bit version)
//If the tag isn't an integer type, instead returns def. 
inline int nbt_get_integral(Tag* tag_p, int def)
{
	if (!tag_p)
		return def;

	switch (tag_p->GetType())
	{
	case NBTTag::Byte:
		return ((ByteTag*)tag_p)->value;
	case NBTTag::Short:
		return ((ShortTag*)tag_p)->value;
	case NBTTag::Int:
		return ((IntTag*)tag_p)->value;
	}
	return def;
}

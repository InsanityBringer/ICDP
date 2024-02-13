/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

#include "definitions.h"
#include "misc/error.h"

//DEFINITIONS

//List of all objects found when calling defs_parse_base_file.
//This list is initialized once when a Neptune engine game starts, and represents the base game data before any mods are loaded.
std::vector<def_object> base_objects;

//Just sort of shoving the keys down here.. should move them out this file. 

void def_key::delete_data()
{
	if (array_count <= 1) //not an array?
	{
		if (type == def_field_type::STRING)
		{
			std::string* str = (std::string*)data;
			delete str;
		}
		else if (type == def_field_type::OBJECT)
		{
			def_object* obj = (def_object*)data;
			delete obj;
		}
	}
	else
	{
		if (type == def_field_type::INTEGER)
		{
			int* arr = (int*)data;
			delete[] arr;
		}
		else if (type == def_field_type::FLOATING)
		{
			double* arr = (double*)data;
			delete[] arr;
		}
	}

	data = nullptr;
}

def_key::def_key(std::string& name, const int value) : name(name), type(def_field_type::INTEGER)
{
	ivalue = value;
	fvalue = value;
	array_count = 0;
	data = nullptr;
}

def_key::def_key(std::string& name, const double value) : name(name), type(def_field_type::FLOATING)
{
	fvalue = value;
	ivalue = value;
	array_count = 0;
	data = nullptr;
}

def_key::def_key(std::string& name, const std::string_view value) : name(name), type(def_field_type::STRING)
{
	fvalue = ivalue = 0;
	std::string* str = new std::string(value);
	array_count = 0;
	//This is playing with fire a little, having non-trivial types in a void* pointer. 
	//The destructor will have to cast to the right type before calling delete. 
	data = str; 
}

def_key::def_key(std::string& name, const def_object& value) : name(name), type(def_field_type::OBJECT)
{
	fvalue = ivalue = 0;
	def_object* obj = new def_object(value);
	array_count = 0;
	data = obj;
}

def_key::def_key(std::string& name, const std::vector<int>& values) : name(name), type(def_field_type::INTEGER)
{
	fvalue = ivalue = 0;
	array_count = values.size();
	int* arr = new int[array_count];
	memcpy(arr, values.data(), values.size() * sizeof(int));
	data = arr;
}

def_key::def_key(std::string& name, const std::vector<double>& values) : name(name), type(def_field_type::FLOATING)
{
	fvalue = ivalue = 0;
	array_count = values.size();
	double* arr = new double[array_count];
	memcpy(arr, values.data(), values.size() * sizeof(double));
	data = arr;
}

//String arrays suck a lot. Use sparingly. 
def_key::def_key(std::string& name, const std::vector<std::string>& values) : name(name), type(def_field_type::STRING)
{
	fvalue = ivalue = 0;
	array_count = values.size();
	std::string* arr = new std::string[array_count];
	for (size_t i = 0; i < array_count; i++)
	{
		arr[i] = std::string(values[i]);
	}
	data = arr;
}

def_key::def_key(const def_key& other)
{
	name = other.name;
	type = other.type;
	ivalue = other.ivalue;
	fvalue = other.fvalue;
	array_count = other.array_count;
	data = nullptr;

	if (array_count <= 1) //not an array?
	{
		if (type == def_field_type::STRING)
		{
			std::string* orig = (std::string*)other.data;
			std::string* str = new std::string(*orig);
			data = str;
		}
		else if (type == def_field_type::OBJECT)
		{
			def_object* orig = (def_object*)other.data;
			def_object* obj = new def_object(*orig);
			data = obj;
		}
	}
	else
	{
		if (type == def_field_type::INTEGER)
		{
			int* src = (int*)other.data;
			int* dest = new int[array_count];
			for (int i = 0; i < array_count; i++)
				dest[i] = src[i];

			data = dest;
		}
		else if (type == def_field_type::FLOATING)
		{
			double* src = (double*)other.data;
			double* dest = new double[array_count];
			for (int i = 0; i < array_count; i++)
				dest[i] = src[i];

			data = dest;
		}
		else if (type == def_field_type::STRING)
		{
			std::string* src = (std::string*)other.data;
			std::string* dest = new std::string[array_count];

			for (int i = 0; i < array_count; i++)
				dest[i] = std::string(src[i]);
			data = dest;
		}
		else if (type == def_field_type::OBJECT)
		{
			def_object* src = (def_object*)other.data;
			def_object* dest = new def_object[array_count];

			for (int i = 0; i < array_count; i++)
				dest[i] = src[i];

			data = dest;
		}
	}
}

def_key::def_key(def_key&& other) noexcept
{
	name = other.name;
	type = other.type;
	ivalue = other.ivalue;
	fvalue = other.fvalue;
	array_count = other.array_count;
	data = nullptr;

	if (array_count <= 1) //not an array?
	{
		if (type == def_field_type::STRING)
		{
			//TODO: Can this be moved?
			std::string* orig = (std::string*)other.data;
			std::string* str = new std::string(*orig);
			data = str;
			delete orig;
		}
		else if (type == def_field_type::OBJECT)
		{
			def_object* orig = (def_object*)other.data;
			def_object* obj = new def_object(*orig);
			data = obj;
			delete orig;
		}
	}

	other.data = nullptr;
	other.type = def_field_type::NULLFIELD;
}

def_key& def_key::operator=(const def_key& other)
{
	//Must delete data that already exists.
	delete_data();

	name = other.name;
	type = other.type;
	ivalue = other.ivalue;
	fvalue = other.fvalue;
	array_count = other.array_count;
	data = nullptr;

	if (array_count <= 1) //not an array?
	{
		if (type == def_field_type::STRING)
		{
			std::string* orig = (std::string*)other.data;
			std::string* str = new std::string(*orig);
			data = str;
		}
		else if (type == def_field_type::OBJECT)
		{
			def_object* orig = (def_object*)other.data;
			def_object* obj = new def_object(*orig);
			data = obj;
		}
	}
	else
	{
		if (type == def_field_type::INTEGER)
		{
			int* src = (int*)other.data;
			int* dest = new int[array_count];
			for (int i = 0; i < array_count; i++)
				dest[i] = src[i];

			data = dest;
		}
		else if (type == def_field_type::FLOATING)
		{
			double* src = (double*)other.data;
			double* dest = new double[array_count];
			for (int i = 0; i < array_count; i++)
				dest[i] = src[i];

			data = dest;
		}
		else if (type == def_field_type::STRING)
		{
			std::string* src = (std::string*)other.data;
			std::string* dest = new std::string[array_count];

			for (int i = 0; i < array_count; i++)
				dest[i] = std::string(src[i]);
			data = dest;
		}
		else if (type == def_field_type::OBJECT)
		{
			def_object* src = (def_object*)other.data;
			def_object* dest = new def_object[array_count];

			for (int i = 0; i < array_count; i++)
				dest[i] = src[i];

			data = dest;
		}
	}

	return *this;
}

def_key::~def_key()
{
	delete_data();
}

def_object::def_object()
{
	type = -1;
	parent = nullptr;
}

definition_list::definition_list(bool must_register_types)
{
	need_types = must_register_types;
}

void definition_list::register_type(std::string_view name, int id)
{
	std::string namestr = std::string(name.data(), name.size());
	auto it = typelookup.find(namestr);
	if (it == typelookup.end()) //Type doesn't already exist?
	{
		types.emplace_back(namestr, id);
	}
	else
	{
		Error("definition_list::register_type: %s already registered!", namestr.c_str());
	}
}

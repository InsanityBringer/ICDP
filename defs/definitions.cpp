/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

#include "definitions.h"
#include "misc/error.h"

//SCHEMAS
//A registered schema
struct st_schema
{
	std::string_view name;
	std::span<def_schema_field> fields;
	int id;

	st_schema() 
	{
		id = -1;
	}

	st_schema(std::string_view name, int id, std::span<def_schema_field> fields) : name(name), id(id), fields(fields)
	{
	}
};

static std::vector<st_schema> cur_schemas;

void defs_add_schema(std::string_view name, int id, std::span<def_schema_field> fields)
{
	//See if another schema with this name or ID was already registered
	for (st_schema& schema : cur_schemas)
	{
		if (schema.name == name)
			Error("defs_add_schema: Schema %s is already registered!", name.data());
		else if (schema.id == id)
			Error("defs_add_schema: Schema id %d is already registered!", id);
	}

	//no duplicates, so add it
	cur_schemas.emplace_back(name, id, fields);
}

//DEFINITIONS

//List of all objects found when calling defs_parse_base_file.
//This list is initialized once when a Neptune engine game starts, and represents the base game data before any mods are loaded.
std::vector<def_object> base_objects;

//Just sort of shoving the keys down here.. should move them out this file. 

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
}

def_key::def_key(def_key&& other) noexcept
{
	name = other.name;
	type = other.type;
	ivalue = other.ivalue;
	fvalue = other.fvalue;
	array_count = other.array_count;

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

def_key::~def_key()
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

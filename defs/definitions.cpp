/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

#include <format>
#include "definitions.h"
#include "misc/error.h"
#include "cfile/cfile.h"
#include "platform/mono.h"

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
		else if (type == def_field_type::STRING)
		{
			std::string* str = (std::string*)data;
			delete[] str;
		}
		else if (type == def_field_type::OBJECT)
		{
			def_object* obj = (def_object*)data;
			delete[] obj;
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
		}
		else if (type == def_field_type::OBJECT)
		{
			def_object* orig = (def_object*)other.data;
			def_object* obj = new def_object(*orig);
			data = obj;
		}
	}

	other.data = nullptr;
	other.type = def_field_type::NULLFIELD;
	other.delete_data();
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

def_object::def_object(std::string& name, int type, def_object* parent) : name(name), parent(parent), type(type)
{
}

static def_field_type get_type(sc_token& token)
{
	if (!token.compare("int"))
		return def_field_type::INTEGER;
	else if (!token.compare("float"))
		return def_field_type::FLOATING;
	else if (!token.compare("object"))
		return def_field_type::OBJECT;
	else if (!token.compare("string"))
		return def_field_type::STRING;

	return def_field_type::NULLFIELD;
}

void definition_list::parse_document(scanner& sc, bool add_to_extra)
{
	sc_token token;
	is_parsing_extras = add_to_extra;

	//Parse top level statements.
	while (sc.read_string(token))
	{
		//Handle any specific top level statements now.
		if (!token.compare("const"))
		{
			parse_constant(sc, token);
		}
		else if (!token.compare("include"))
		{
			parse_include(sc, token);
		}
		//If it's not any of the above, then it's gotta be a object. 
		else
		{
			sc.unread_token(); //The object parser will reread the token. 
			maybe_parse_object(sc, token);
		}
	}
}

void definition_list::parse_constant(scanner& sc, sc_token& token)
{
	sc.must_get_identifier(token);
	def_field_type consttype = get_type(token);
	if (consttype != def_field_type::INTEGER && consttype != def_field_type::FLOATING)
	{
		sc.raise_error(std::format("Constant with type {}, but only int or float supported.", token.get_chars()));
		return;
	}

	sc.must_get_identifier(token);
	std::string name = token.get_chars();

	//See if this constant already exists.
	if (find_constant(name))
	{
		sc.raise_error(std::format("Constant {} is already defined.", name));
		return;
	}

	sc.must_get_any_punctuation(token, { "=" });

	sc.must_get_number(token);
	if (consttype == def_field_type::FLOATING)
		add_constant(name, token.get_double()); //can be initialized with either a floating point or integer token.
	else if (consttype == def_field_type::INTEGER)
	{
		//TODO: Need system to properly warn on truncation
		if (token.get_format() == token_format::floating)
		{
			mprintf((1, "warning: truncating integer constant %s\n", name.c_str()));
		}
		add_constant(name, token.get_integer());
	}

	sc.must_get_any_punctuation(token, { ";" });
}

void definition_list::parse_include(scanner& sc, sc_token& token)
{
	sc.must_get_quoted_string(token);

	//TODO: This reading code should really be in scanner itself.
	std::string buffer;
	const char* filename = token.get_chars().c_str();
	CFILE* fp = cfopen_from(archive_handle, filename);
	if (!fp)
		sc.raise_error(std::format("Can't open included document {}", filename));

	size_t len = cfilelength(fp);
	buffer.resize(len);
	if (cfread(buffer.data(), 1, len, fp) != len)
		sc.raise_error(std::format("Failed to read all of included document {}", filename));

	cfclose(fp);

	//feels unambigious enough to go without a semicolon. 
	//sc.must_get_any_punctuation(token, { ";" }); 

	sc.include_document(filename, buffer);
}

void definition_list::maybe_parse_object(scanner& sc, sc_token& token)
{
	std::string type;
	definition_type* typeinfo;
	int typenum = -1;

	//Only read types when the system is using types. 
	if (need_types)
	{
		sc.must_get_identifier(token);
		//Make sure the type is registered
		type = token.get_chars();
		typeinfo = find_type(type);
		if (!typeinfo)
		{
			sc.raise_error(std::format("Unknown type {}", type));
			return; //I suspect this should use exceptions to bring control back to the menus when loading addons. 
		}

		typenum = typeinfo->id;
	}

	//Read name
	sc.must_get_identifier(token);
	std::string name = token.get_chars();
	def_object* parent_obj = nullptr;

	//If types are not used, the scanner has read the initial {, and can start parsing the object.
	if (!need_types)
	{
		sc.must_get_any_punctuation(token, { "{" });
	}
	//A longer process is needed for types, since there could have been inheritance. 
	else
	{
		sc.must_get_any_punctuation(token, { ":", "{" });
		//Doing inheritance
		if (token.get_punctuation_type() == punctuation_type::colon)
		{
			sc.must_get_identifier(token);
			std::string parent_name = token.get_chars();
			parent_obj = find_object_of_type(parent_name, typeinfo->id);
			if (!parent_obj)
				sc.raise_error(std::format("Cannot find parent {} {}", typeinfo->name, parent_name));

			sc.must_get_any_punctuation(token, { "{" }); //Read the start of the object.
		}

		//If this is not the default object, explicitly set the parent to the default object for this type.
		if (name.compare("_default"))
		{
			if (typeinfo->defaults.get_name().size() == 0) //Check that the parent object was defined at some point
				sc.raise_error(std::format("Tried to define {} {}, but a _default wasn't defined!", type, name));

			//_default is defined, so check if it needs to be set as the parent.
			if (!parent_obj)
				parent_obj = &typeinfo->defaults;
		}
		else
		{
			if (parent_obj)
				sc.raise_error("A _default entry cannot have a parent!");
			else if (typeinfo->defaults.get_name().size() != 0) //Already a default in place?
				sc.raise_error(std::format("Attempted to redefine _default for type {}!", type));
		}
	}

	//Shove an object into the list and start operating on it.
	//If this is the default object, this will be a reference to the defaults rather than a type in list. 
	def_object& newobj = create_new_object(name, typenum, parent_obj);
	parse_object_body(sc, token, newobj);
}

void definition_list::parse_object_body(scanner& sc, sc_token& token, def_object& obj)
{
	//temp
	do
	{
		sc.must_get_string(token);
	} while (token.compare("}"));
}

definition_list::definition_list(bool must_register_types)
{
	is_parsing_extras = false;
	need_types = must_register_types;
	archive_handle = 0;
}

void definition_list::register_type(std::string_view name, int id)
{
	Assert(need_types);
	std::string namestr = std::string(name.data(), name.size());
	auto it = typelookup.find(namestr);
	if (it == typelookup.end()) //Type doesn't already exist?
	{
		typelookup[namestr] = types.size();
		types.emplace_back(namestr, id);
	}
	else
	{
		Error("definition_list::register_type: %s already registered!", namestr.c_str());
	}
}

void definition_list::read_defs_from_lump(const char* filename, uint32_t archivehandle)
{
	std::string buffer; 
	CFILE* fp = cfopen_from(archivehandle, filename);
	if (!fp)
		Error("definition_list::read_defs_from_lump: Can't read %s", filename);

	size_t len = cfilelength(fp);
	buffer.resize(len);
	if (cfread(buffer.data(), 1, len, fp) != len)
		Error("definition_list::read_defs_from_lump: Failed to read all of %s", filename);

	cfclose(fp);

	archive_handle = archivehandle;
	scanner sc(filename, buffer);
	parse_document(sc, false);
}

void definition_list::add_defs_from_lump(const char* filename, uint32_t archivehandle)
{
}

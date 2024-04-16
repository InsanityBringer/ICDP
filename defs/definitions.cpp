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
//Just sort of shoving the keys down here.. should move them out this file. 

void def_key::delete_data()
{
	if (!is_array) //not an array?
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
			std::vector<int>* arr = (std::vector<int>*)data;
			delete[] arr;
		}
		else if (type == def_field_type::FLOATING)
		{
			std::vector<double>* arr = (std::vector<double>*)data;
			delete[] arr;
		}
		else if (type == def_field_type::STRING)
		{
			std::vector<std::string>* str = (std::vector<std::string>*)data;
			delete[] str;
		}
		else if (type == def_field_type::OBJECT)
		{
			std::vector<def_object>* obj = (std::vector<def_object>*)data;
			delete[] obj;
		}
	}

	data = nullptr;
}

void def_key::copy_dataptr(const def_key& other)
{
	if (!is_array) //not an array?
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
			std::vector<int>* src = (std::vector<int>*)other.data;
			std::vector<int>* dest = new std::vector<int>();
			dest->assign(src->begin(), src->end());

			data = dest;
		}
		else if (type == def_field_type::FLOATING)
		{
			std::vector<double>* src = (std::vector<double>*)other.data;
			std::vector<double>* dest = new std::vector<double>();
			dest->assign(src->begin(), src->end());

			data = dest;
		}
		else if (type == def_field_type::STRING)
		{
			std::vector<std::string>* src = (std::vector<std::string>*)other.data;
			std::vector<std::string>* dest = new std::vector<std::string>();
			dest->assign(src->begin(), src->end());

			data = dest;
		}
		else if (type == def_field_type::OBJECT)
		{
			std::vector<def_object>* src = (std::vector<def_object>*)other.data;
			std::vector<def_object>* dest = new std::vector<def_object>();
			dest->assign(src->begin(), src->end());

			data = dest;
		}
	}
}

def_key::def_key(std::string& name, def_field_type type) : name(name), type(type)
{
	ivalue = fvalue = 0;
	array_count = 0;
	is_array = false;
	data = nullptr;
	//Check default values
	if (type == def_field_type::STRING)
	{
		std::string* str = new std::string();
		data = str;
	}
	else if (type == def_field_type::OBJECT)
	{
		def_object* obj = new def_object();
		data = obj;
	}
}

def_key::def_key(const def_key& other)
{
	name = other.name;
	type = other.type;
	ivalue = other.ivalue;
	fvalue = other.fvalue;
	array_count = other.array_count;
	data = nullptr;
	is_array = other.is_array;

	copy_dataptr(other);
}

def_key::def_key(def_key&& other) noexcept
{
	name = other.name;
	type = other.type;
	ivalue = other.ivalue;
	fvalue = other.fvalue;
	array_count = other.array_count;
	data = nullptr;
	is_array = other.is_array;

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
	is_array = other.is_array;

	copy_dataptr(other);

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

static const std::string typenames[] =
{
	"int",
	"float",
	"string",
	"object",
	"undefined"
};

static const std::string& get_name_for_type(def_field_type type)
{
	return typenames[(int)type];
}

void def_object::check_property(scanner& sc, def_key& key)
{
	//TODO: Hashtable lookup?
	for (def_property& property : properties)
	{
		//Check if the field is named correctly..
		if (!key.get_name().compare(property.fieldname))
		{
			//Check if it's the right type..
			if (key.get_type() == property.fieldtype)
			{
				//Check if it's the right array count...
				if (key.get_is_array() == property.is_array)
				{
					//Successfully validated
					return;
				}
				else
				{
					if (property.is_array)
						sc.raise_error(std::format("Property {} is an array, but the key is not.", property.fieldname));
					else
						sc.raise_error(std::format("Property {} is not an array, but the key is an array.", property.fieldname));
				}
			}
			else
			{
				sc.raise_error(std::format("Key {} is the wrong type. Expected {}, got {}.", property.fieldname, get_name_for_type(property.fieldtype), get_name_for_type(key.get_type())));
			}
		}
	}

	if (parent)
		parent->check_property(sc, key);
	else
		sc.raise_error(std::format("Unknown property {}", key.get_name()));
}

def_property* def_object::find_property(std::string& name)
{
	for (def_property& property : properties)
	{
		if (!name.compare(property.fieldname))
		{
			return &property;
		}
	}
	if (parent)
		return parent->find_property(name);
	else
		return nullptr;
}

def_property* def_object::add_property(std::string& name)
{
	def_property property = { name };
	properties.push_back(property);
	return &properties.back();
}

void def_object::create_default_key(def_property& property)
{
	if (property.is_array)
	{
		switch (property.fieldtype)
		{
		case def_field_type::INTEGER:
		{
			std::vector<int> hack; hack.resize(property.array_count);
			keys.emplace_back(property.fieldname, hack);
			break;
		}
		case def_field_type::FLOATING:
		{
			std::vector<double> hack; hack.resize(property.array_count);
			keys.emplace_back(property.fieldname, hack);
			break;
		}
		case def_field_type::STRING:
		{
			std::vector<std::string> hack; hack.resize(property.array_count);
			keys.emplace_back(property.fieldname, hack);
			break;
		}
		case def_field_type::OBJECT:
			Int3();
			break;
		}
	}
	else
	{
		switch (property.fieldtype)
		{
		case def_field_type::INTEGER:
			keys.emplace_back(property.fieldname, 0);
			break;
		case def_field_type::FLOATING:
			keys.emplace_back(property.fieldname, 0.0);
			break;
		case def_field_type::STRING:
			keys.emplace_back(property.fieldname, "");
			break;
		case def_field_type::OBJECT:
		{
			def_object temp = def_object();
			keys.emplace_back(property.fieldname, temp);
			break;
		}
		}
	}
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

//Gets a potential type from the token. 
def_field_type definition_list::type_from_token(sc_token& token)
{
	if (token.get_token_type() == token_type::number)
	{
		if (token.get_format() == token_format::floating)
			return def_field_type::FLOATING;
		return def_field_type::INTEGER;
	}
	else if (token.get_token_type() == token_type::string)
	{
		//Potentially a constant, check the constant list
		def_const* constval = find_constant(token.get_chars());
		if (constval)
		{
			if (constval->index() == 1)
				return def_field_type::FLOATING;
			else
				return def_field_type::INTEGER;
		}
	}
	else if (token.get_token_type() == token_type::quoted_string)
		return def_field_type::STRING;
	return def_field_type::NULLFIELD;
}

static bool types_are_compatible(def_field_type left, def_field_type right)
{
	if (left == def_field_type::FLOATING && right == def_field_type::INTEGER)
	{
		//It's acceptable to assign a integer to a floating point key
		return true;
	}

	return left == right;
}

void definition_list::parse_key_value(scanner& sc, sc_token& token, def_object& obj, def_property* property, std::string& name)
{
	sc.must_get_string(token);
	def_field_type type = def_field_type::NULLFIELD;
	bool is_array = false;
	if (token.get_token_type() == token_type::punctuation && token.get_punctuation_type() == punctuation_type::curly_left)
	{
		//I just can't this format was an awful idea. 
		//Imagine an array like {1, 2, 3, 4.5}, the key
		if (!property) 
		{
			sc.raise_error("Array not supported when properties aren't used!");
			return;
		}
		//an array. Determine what type it is
		//sc.must_get_string(token);
		//type = type_from_token(token);
		type = property->fieldtype;
		//Let the value be reread in the loop
		sc.unread_token();
	}
	else
	{
		type = type_from_token(token);
	}

	if (type == def_field_type::NULLFIELD)
		sc.raise_error("Can't identify type of value");

	//Do some validation of the type if a property is provided
	if (property)
	{
		//It is valid to assign an integer to a floating point type
		if (property->fieldtype == def_field_type::FLOATING)
		{
			if (!types_are_compatible(property->fieldtype, type))
				sc.raise_error("Value is of the wrong type!");
		}
	}

	sc.must_get_any_punctuation(token, { ";" });
}

void definition_list::parse_array_value(scanner& sc, sc_token& token, def_object& obj, def_property* property, std::string& name, def_field_type type)
{
}

void definition_list::parse_property(scanner& sc, sc_token& token, def_object& obj)
{
	if (!need_types)
	{
		sc.raise_error("Document is not using property definitions");
		return;
	}

	//will have already parsed the "property", so parse type
	sc.must_get_identifier(token);
	def_field_type type = get_type(token);
	if (type == def_field_type::NULLFIELD)
	{
		sc.raise_error(std::format("Unknown type {}!", token.get_chars()));
		return;
	}
	else if (type == def_field_type::OBJECT)
	{
		sc.raise_error(std::format("Subobjects are not supported yet.", token.get_chars()));
		return;
	}

	//Parse name
	sc.must_get_identifier(token);
	def_property* property = obj.find_property(token.get_chars());
	if (property)
	{
		sc.raise_error(std::format("Property {} is already defined!", token.get_chars()));
		return;
	}

	//Add the property to the object
	property = obj.add_property(token.get_chars());
	property->fieldtype = type;

	//Check for default value, array, or an initializer
	sc.must_get_any_punctuation(token, { "=", ";", "[" });
	switch (token.get_punctuation_type())
	{
	case punctuation_type::semicolon:
		obj.create_default_key(*property);
		return;
	case punctuation_type::equals:
		parse_key_value(sc, token, obj, property, property->fieldname);
		return;
	case punctuation_type::square_left:
		sc.must_get_integer(token);
		property->array_count = token.get_integer();
		if (property->array_count <= 0)
			sc.raise_error("Invalid array count");
		sc.must_get_any_punctuation(token, { "]" });
		property->is_array = true;
		//TODO: Should be able to initialize array types
		sc.must_get_any_punctuation(token, { ";" });
		obj.create_default_key(*property);
		return;
	}
}

void definition_list::parse_object_body(scanner& sc, sc_token& token, def_object& obj)
{
	for (;;)
	{
		//Read a statement
		sc.must_get_string(token);
		//Check for end of body
		if (!token.compare("}"))
			break;
		//Check for property declaration
		else if (!token.compare("property"))
		{
			parse_property(sc, token, obj);
		}
		//Check if it's an identifier. 
		else if (token.get_token_type() == token_type::string)
		{
			std::string name = token.get_chars();
			def_property* property = nullptr;
			if (need_types)
			{
				property = obj.find_property(name);
				if (!property)
				{
					sc.raise_error(std::format("Unknown property {}!", name));
				}
			}
			sc.must_get_any_punctuation(token, { "=" });
			parse_key_value(sc, token, obj, property, name);
		}
		else
		{
			sc.raise_error(std::format("Unexpected {}. Expected \"property\", identifier, or \"}\"", token.get_chars()));
		}
	}
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

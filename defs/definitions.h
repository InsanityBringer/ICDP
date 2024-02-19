/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/
#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <variant>
#include <unordered_map>
#include "misc/scanner.h"
#include "misc/error.h"

//Properties are defined by entities read from a definition file. This is used to validate that the data is in a correct format.
enum class def_field_type
{
	//Field should be an integer.
	INTEGER,
	//Field should be a floating point number.
	FLOATING,
	//Field should be a quoted string.
	STRING,
	//Field should be a subobject, of a type identified by subtype.
	OBJECT,
	//Field has no data storage. A key's type is set to this when moved. 
	NULLFIELD,
};

//A property for an object. When inheriting, all properties of all sub types are made available. 
struct def_property
{
	std::string			fieldname; //Name of the field. Probably case sensitive.
	def_field_type		fieldtype; //Type of the field.
	bool				is_array;
};

//Definition files.
//Definition files are key/value files that are used by the engine to provide data for various information.
//The defintion files are flexible and schemas are registered by the game code to describe which definitions are possible.
/*
The basic form of a defintion file is as follows:

typename object_name
{
	key_1 = 13;
	key_2 = "quoted string";
	array_key = [1, 2, 3, 4, 5];
	subobject = 
	{
		sub_key_1 = 16;
	}
}

typename is the name of a type that is registered. Not all definition lists may need registration. 
object_name is the name of this object. These are namespaced per object type.
key_1 is a schema-registered key of type INTEGER.
key_2 is a key of type STRING
array_key is a key of type INTEGER, in an array of size 5. 
subobject is a key of type OBJECT. 
*/

class def_key;

class def_object
{
	std::string					name; //Object name. Note that subobjects are anonmoyous and have no name. 
	int							type; //Number of the registered type, identifying this object.
	std::vector<def_key>		keys; //Keys for this object, hopefully validated against the schema.
	std::vector<def_property>	properties;
	def_object*					parent; //Parent definiton, will be null for untyped objects or base types. 
	//Depending on the performance, keys should probably have an associated hash set of indicies with it. 

public:
	def_object();
	def_object(std::string& name, int type, def_object* parent);
	//Checks the property, throwing a relevant error if it is not found or in the wrong format. 
	//Will chain to parent entity if exists.
	void check_property(def_key& key);

	std::string& get_name()
	{
		return name;
	}

	int get_type_num()
	{
		return type;
	}
};

//not using this yet, I want to judge performance and memory usage of the initial approach. 
//using def_keydata = std::variant<int, double, std::string, def_object>;

//An individual key in a definition file. This will encapsulate several kinds of data.
//TODO: def_key is waaaaaaay too large due to std::string, but ownership of the name is needed. I will go as far as to make this a char* if it will make things more compact, 
//even if it means I need to manage it myself. 
class def_key
{
	std::string		name;			//The name of the key. 
	def_field_type	type;			//Type of the key. 

	//It's a little extra memory, but since these non-array numbers will be the most common keys, it probably makes sense to store them directly.
	int				ivalue;			//Integer value
	double			fvalue;			//Floating-point value. 

	//The following is used for both arrays of any key type and storing single STRINGs and OBJECTs directly.
	size_t			array_count;	//Number of elements
	void*			data;			//Flexible data type.

private:
	void delete_data();

public:
	def_key(std::string& name, const int value); //Creates a new INTEGER key.
	def_key(std::string& name, const double value); //Creates a new FLOATING key.
	def_key(std::string& name, const std::string_view value); //Creates a new STRING key.
	def_key(std::string& name, const def_object& value); //Creates a new OBJECT key.
	
	def_key(std::string& name, const std::vector<int>& values); //Creates a new INTEGER array key.
	def_key(std::string& name, const std::vector<double>& values); //Creates a new FLOATING array key.
	def_key(std::string& name, const std::vector<std::string>& values); //Creates a new STRING array key.
	//def_key(std::string& name, const std::vector<def_object>& values); //Creates a new OBJECT array key.
	//arrays of subobjects aren't supported. Should they be?

	//These are needed due to the special handling of the data field. 
	def_key(const def_key& other); //copy constructor. 
	def_key(def_key&& other) noexcept; //move constructor.

	def_key& operator=(const def_key& other);

	~def_key(); //Destructor for freeing data if used.

	def_field_type get_type()
	{
		return type;
	}

	std::string& get_name()
	{
		return name;
	}
};

//A registered type in a definition list. This type cannot be used until a "_default" entity is processed.
struct definition_type
{
	std::string	name;		//Name of the type, used when reading definitions.
	int			id;			//Identifier of the type, used to identify which type an object belongs to.
	def_object	defaults;	//The default object for this type. All objects registered will inherit from this object. 
public:
	definition_type(std::string name, int id) : name(name), id(id)
	{
	}
};

//A constant, can only be integer or floating. 
using def_const = std::variant<int, double>;
using const_dict = std::unordered_map<std::string, def_const>;

class definition_list
{
	bool									need_types;
	std::vector<definition_type>			types;
	std::unordered_map<std::string, int>	typelookup;

	std::vector<def_object>					defs;		//Definitions from the core data files
	std::vector<def_object>					addon_defs;	//Definitions from any addons. 

	const_dict								constants;			//Const values loaded from the core defintion files. Apply to all further processed documents.
	const_dict								addon_constants;	//Addon constants. purged after each document is loaded, so addons won't conflict.

	//Internal state during parsing
	bool									is_parsing_extras;	//True if reading addon definitions. 
	uint32_t								archive_handle;

	//Parse a document loaded into the scanner sc. Included files will come from the archive specified by archivehandle.
	//Set add_to_extra to true if loading addon files, all parsed objects will be loaded into addon_defs instead.
	void parse_document(scanner& sc, bool add_to_extra);
	void parse_constant(scanner& sc, sc_token& token);
	void parse_include(scanner& sc, sc_token& token);
	void maybe_parse_object(scanner& sc, sc_token& token);
	void parse_object_body(scanner& sc, sc_token& token, def_object& obj);

	//Finds a constant, returns nullptr on error. 
	def_const* find_constant(std::string& constname)
	{
		//Check core constants
		auto it = constants.find(constname);
		if (it != constants.end())
			return &it->second;

		//Check addon constants
		it = addon_constants.find(constname);
		if (it != addon_constants.end())
			return &it->second;

		return nullptr;
	}

	//Adds a constant, does not check if it's been added before.
	void add_constant(std::string& name, def_const value)
	{
		if (is_parsing_extras)
			addon_constants[name] = value;
		else
			constants[name] = value;
	}

	definition_type* find_type(std::string& name)
	{
		auto it = typelookup.find(name);
		if (it != typelookup.end())
			return &types[it->second];

		return nullptr;
	}

	//Finds a type definition by the ID provided.
	//This wasn't thought out very well..
	definition_type* find_type_by_id(int id)
	{
		for (int i = 0; i < types.size(); i++)
		{
			if (types[i].id == id)
				return &types[i];
		}

		return nullptr;
	}

	//Finds an object named "name" of the type specified by "typenum".
	//This could use a hash table, but this probably should be fine. 
	def_object* find_object_of_type(std::string& name, int typenum)
	{
		//Check core defs first
		for (def_object& obj : defs)
		{
			if (!obj.get_name().compare(name) && obj.get_type_num() == typenum)
				return &obj;
		}

		//Check addon defs second
		for (def_object& obj : addon_defs)
		{
			if (!obj.get_name().compare(name) && obj.get_type_num() == typenum)
				return &obj;
		}

		return nullptr;
	}

	//Emplaces an object at the end of the current list. 
	//Returns a reference to the object.
	def_object& create_new_object(std::string& name, int typenum, def_object* parent)
	{
		if (need_types && !name.compare("_default"))
		{
			definition_type* typeinfo = find_type_by_id(typenum);
			if (!typeinfo)
				Error("definition_list::create_new_object: Attempted to create default for unknown type ID.");

			typeinfo->defaults = def_object(name, typenum, parent);
			return typeinfo->defaults;
		}

		if (is_parsing_extras)
		{
			addon_defs.emplace_back(name, typenum, parent);
			return addon_defs.back();
		}
		else
		{
			defs.emplace_back(name, typenum, parent);
			return defs.back();
		}
	}

public:
	//Constructor for the definition list.
	//Set must_register_type to true if types and properties are to be used. 
	//Otherwise, typeless definitions are allowed. 
	definition_list(bool must_register_types);

	void register_type(std::string_view name, int id);

	//Reads defs from the lump filename in the archive specified by archivehandle.
	//If you don't care where they come from, an archivehandle of 0 can be specified.
	void read_defs_from_lump(const char* filename, uint32_t archivehandle = 0);

	//Adds additional defs from another lump. You probably don't want to specify a handle of 0.
	//These additional defs can then be cleared when changing missions.
	void add_defs_from_lump(const char* filename, uint32_t archivehandle = 0);

	//Frees all addon data to free memory and ready for future addons.
	//Addon constants should already be purged after processing each document.
	void purge_addons()
	{
		addon_defs.clear();
	}
};

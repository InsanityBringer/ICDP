/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

#include <string>
#include <string_view>
#include <vector>
#include <span>

//Schema data.
//Schemas are used to control the parser and tell it what keys to expect, and what types these keys are.
//The schema is used to provide validation for user input, showing errors if a unexpected key is present, 
//or if the key is in an unexpected format.

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

//A definition field for a schema. A field will give the key name expected, and an expected type for it. 
//The expected type can be OBJECT, in which subtype will describe an identifier for that schema block. 
struct def_schema_field
{
	std::string_view	fieldname; //Name of the field. Probably case sensitive.
	def_field_type		fieldtype; //Type of the field.
	int					subtype; //If fieldtype is OBJECT, this is the id of the object type. Otherwise this is an expected array count. These probably should be separate.
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

typename is the name of a type that is registered as a schema.
object_name is the name of this object. These are namespaced per registered schema.
key_1 is a schema-registered key of type INTEGER.
key_2 is a key of type STRING
array_key is a key of type INTEGER, in an array of size 5. 
subobject is a key of type OBJECT. The subobject will also be defined by a schema, as identified by the parent type's schema.
*/

class def_object;

//An individual key in a definition file. This will encapsulate several kinds of data.
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

	~def_key(); //Destructor for freeing data if used.

	def_field_type get_type()
	{
		return type;
	}
};

class def_object
{
	std::string				name; //Object name. Note that subobjects are anonmoyous and have no name. 
	int						type; //Number of the registered type, identifying this object.
	std::vector<def_key>	keys; //Keys for this object, hopefully validated against the schema.
	//Depending on the performance, keys should probably have an associated hash set of indicies with it. 
};


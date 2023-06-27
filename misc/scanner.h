/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

#pragma once

#include <string_view>
#include <string>

//Format of a token. 
enum class token_format
{
	none, //You never read anything or something bad happened
	string, //Token is composed of characters
	integer, //Token is composed of an integer
	floating //Token is composed of a floating-point value.
};

enum class token_type
{
	none, //You never read anything
	number, //Token is composed of a number
	punctuation, //Token is composed of punctuation (., =, ;, and others)
	string, //Token is a string with no spaces
	quoted_string //Token is a quoted string (quotes not included.)
};

//A token read from a scanner
class sc_token
{
	token_format format;
	token_type type;
	std::string chars;

	//Just for convenience
	double dval;
	int ival;

public:
	//Constructs an uninitialized token.
	sc_token()
	{
		format = token_format::none;
		type = token_type::none;
		dval = ival = 0;
	}

	//Constructs an initialized token with the string chars.
	sc_token(std::string chars, token_type type) : chars(chars), type(type)
	{
		if (type == token_type::number)
		{
			if (chars.find('.') != std::string::npos)
			{
				format = token_format::floating;
				dval = std::stod(chars);
				ival = (int)dval;
			}
			else
			{
				format = token_format::integer;
				ival = std::stoi(chars);
				dval = ival;
			}
		}
		else
		{
			format = token_format::string;
			dval = ival = 0;
		}
	}

	int get_integer() const
	{
		return ival;
	}

	double get_double() const
	{
		return dval;
	}

	std::string& get_chars()
	{
		return chars;
	}

	token_format get_format() const
	{
		return format;
	}

	token_type get_token_type() const
	{
		return type;
	}
};

//A token stream vaguely similar to Hexen's sc_man, but from scratch.
//Note: Not all fuctions of this have been tested yet, still needs further development. 
class scanner
{
	int line_num;
	size_t cursor;
	std::string buf;
	token_type last_token_type;
	sc_token last_token;

	std::string error;

	//Advances the cursor. Returns true if EOF, false otherwise. 
	//This will automatically scan for newlines. 
	bool advance(int amount = 1);
	//Advances the cursor to either the start of the next line, or EOF. 
	void clear_to_end_of_line();
	//Reads a quoted string
	void read_quoted_string();
	//Reads a unquoted string
	void read_unquoted_string();
	//Reads a number
	void read_number();
	//Reads punctuation
	void read_punct();

	void raise_error(const char* msg);

public:
	//Initializes the scanner, copying the contents of the string view buf into a new string,
	//so the initial buffer can be freed without problems. 
	scanner(std::string_view buf);

	//Reads one string from the buffer. Returns true if read, false otherwise
	bool read_string();

	//Gets the last token read. 
	sc_token& get_last_token()
	{
		return last_token;
	}
};

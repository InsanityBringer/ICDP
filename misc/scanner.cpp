/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

#include <string.h>
#include <locale>
#include <format>

#include "scanner.h"
#include "error.h"

//For the sake of maximal munch, this will need to be sorted by longest to shortest when longer punctuation sequences are in. 
std::string_view const punctuation_strings[] =
{
	"=",
	",",
	".",
};

//Determine if this character is a part of punctuation
bool is_punct(char ch)
{
	for (std::string_view const& view : punctuation_strings)
	{
		if (view[0] == ch)
			return true;
	}

	return false;
}

//Determine if this character sequence could be part of a number.
//Handles #, .#, -#, -.#
bool is_number_start(std::string_view& start)
{
	if (start.size() == 0)
	{
		Int3();
		return false;
	}

	if (start.size() >= 3) //Can possibly be -.#
	{
		if (start[0] == '-' && start[1] == '.' && isdigit(start[2]))
			return true;
	}

	if (start.size() >= 2) //Can possibly be -# or .#
	{
		if (start[0] == '-' && isdigit(start[1]))
			return true;

		if (start[0] == '.' && isdigit(start[1]))
			return true;
	}

	//Check just #
	if (isdigit(start[0]))
		return true;

	return false;
}

bool is_cpp_comment(std::string_view& start)
{
	if (start.size() == 0)
	{
		Int3();
		return false;
	}

	return start.starts_with("//");
}

scanner::scanner(std::string_view buf) : buf(buf)
{
	cursor = 0;
	line_num = 1;
	last_token_type = token_type::none;
}

void scanner::raise_error(const char* msg)
{
	error = std::format("Script error at line {}: {}", line_num, msg);
}


bool scanner::advance(int amount)
{
	if (cursor == buf.size())
		return true; //already at eof

	for (int i = 0; i < amount; i++)
	{
		if (buf[cursor] == '\n')
			line_num++;
		cursor++;
		if (cursor == buf.size())
			return true;
	}

	return false;
}

void scanner::clear_to_end_of_line()
{
	char ch = buf[cursor];
	while (ch != '\n')
	{
		if (advance())
			return;

		ch = buf[cursor];
	}
}

void scanner::read_quoted_string()
{
	bool last_escaped = false;
	bool found_end = false;

	size_t len = 0;

	std::string new_token;

	while (!found_end)
	{
		//Quoted strings advance first to remove the starting quotation mark.
		if (cursor == buf.size() || advance())
		{
			raise_error("Unclosed string literal");
			return;
		}

		char ch = buf[cursor];

		//If found a unescaped quotation mark, end of string
		if (!last_escaped && ch == '"')
			found_end = true;

		else if (last_escaped)
		{
			last_escaped = false;
			//Handle the escaped character
			if (ch == '"' || ch == '\\')
				new_token += ch; //These add the escaped character directly
			else if (ch == 't')
				new_token += '\t';
			else if (ch == '\n')
				clear_to_end_of_line(); //Escaped newline 
			else
			{
				raise_error("Unknown escape sequence");
			}
		}

		else if (ch == '\\') //Escape sequence
			last_escaped = true;

		else
			new_token += ch; //Nothing special, just append it
	}

	advance(); //Clear the last quotation mark. 
	last_token = sc_token(new_token, token_type::quoted_string);
}

void scanner::read_unquoted_string()
{
	std::string new_token;

	while (true)
	{
		char ch = buf[cursor];

		if (is_punct(ch) || iswspace(ch))
			break; //Punctuation or whitespace characters end strings.
		else
			new_token += ch;

		if (advance())
			break; //hit EOF but it's fine, the last token in a script can be a unquoted string
	}

	last_token = sc_token(new_token, token_type::string);
}

void scanner::read_number()
{
	bool found_dot = false;
	bool found_neg = false;
	std::string new_token;

	while (true)
	{
		char ch = buf[cursor];

		if (found_dot && ch == '.')
			break; //Found more than one dot, so it must be part of punctuation
		else if (ch == '.')
		{
			found_dot = true; //No dots seen before, so its now a punctuation marker
			new_token += ch;
		}
		else if (ch == '-')
		{
			if (found_dot) //neg sign after dot?
			{
				raise_error("Bad numeric constant. Negative sign after .");
				break;
			}
			else if (found_neg)
			{
				raise_error("Bad numeric constant. Multiple negative signs.");
				break;
			}
			else
			{
				found_neg = true;
				new_token += ch;
			}
		}
		else if (!isdigit(ch))
			break; //Check non-digit after dots since otherwise the . would fail this check. 
		else
			new_token += ch;

		if (advance())
			break; //hit EOF but it's fine, the last token in a script can be a number
	}

	last_token = sc_token(new_token, token_type::number);
}

void scanner::read_punct()
{
	//This is slightly silly, but I make a string view from cursor to the end of the buffer, so I don't have to count manually
	std::string_view punct_view = std::string_view(buf.data() + cursor, buf.size() - cursor);

	for (std::string_view const& view : punctuation_strings)
	{
		if (punct_view.starts_with(view))
		{
			last_token = sc_token(std::string(view), token_type::punctuation);
			advance(view.size());
			return;
		}
	}

	//Can't reach here except in an internal error. 
	raise_error("Internal error: Detected punctuation token, but didn't find punctuation sequence.");
	Int3();
}

bool scanner::read_string()
{
	if (cursor == buf.size())
		return false; //At end of buffer, so nothing left to read

	//If the cursor is in whitespace, progress until the first character is found
	while (iswspace(buf[cursor]))
	{
		if (advance())
			return false;
	}

	std::string_view remaining(buf.data() + cursor, buf.size() - cursor);

	//Check if we need to remove a comment
	if (is_cpp_comment(remaining))
	{
		clear_to_end_of_line();
		//Recursively start parsing from the new cursor
		return read_string();
	}

	//Check what kind of token this is
	char ch = buf[cursor];
	if (is_number_start(remaining)) //Number literals can be floating point prefixed with a ., so handle that
	{
		last_token_type = token_type::number;
		read_number();
	}
	else if (is_punct(ch))
	{
		last_token_type = token_type::punctuation;
		read_punct();
	}
	else if (ch == '"')
	{
		last_token_type = token_type::quoted_string;
		read_quoted_string();
	}
	else
	{
		last_token_type = token_type::string;
		read_unquoted_string();
	}

	return true;
}

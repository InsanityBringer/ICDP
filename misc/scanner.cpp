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
punctuation_entry const punctuation_strings[] =
{
	{ punctuation_type::equals, "=" },
	{ punctuation_type::comma, "," },
	{ punctuation_type::period, "." },
	{ punctuation_type::curly_right, "}" },
	{ punctuation_type::curly_left, "{" },
	{ punctuation_type::semicolon, ";" },
	{ punctuation_type::square_left, "[" },
	{ punctuation_type::square_right, "]" },
	{ punctuation_type::colon, ":" },
};

//Determine if this character is a part of punctuation
bool is_punct(char ch)
{
	for (punctuation_entry const& entry : punctuation_strings)
	{
		if (entry.str[0] == ch)
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
	column = 1;
	last_token_type = token_type::none;
	unread = false;
}

scanner::scanner(std::string_view document_name, std::string_view buf) : name(document_name), buf(buf)
{
	cursor = 0;
	line_num = 1;
	column = 1;
	last_token_type = token_type::none;
	unread = false;
}

void scanner::raise_error(const char* msg)
{
	error = std::format("Script error in {} at line {}: {}", name, line_num, msg);

	//TODO: Maybe allow caller to disable fatal errors?
	Error(error.c_str());
}

void scanner::raise_error(const std::string& msg)
{
	error = std::format("Script error in {} at line {}: {}", name, line_num, msg);

	//TODO: Maybe allow caller to disable fatal errors?
	Error(error.c_str());
}

void scanner::must_get_string(sc_token& token)
{
	if (!read_string(token))
		raise_error("Unexpected end of file.");
}

void scanner::must_get_identifier(sc_token& token)
{
	must_get_string(token);
	if (token.get_token_type() != token_type::string)
		raise_error("Expected an identifier.");
}

void scanner::must_get_number(sc_token& token)
{
	must_get_string(token);
	if (token.get_token_type() != token_type::number)
		raise_error(std::format("Expected a number, got {}.", token.get_chars()));
}

void scanner::must_get_integer(sc_token& token)
{
	must_get_string(token);
	if (token.get_token_type() != token_type::number && token.get_format() != token_format::integer)
		raise_error(std::format("Expected an integer, got {}.", token.get_chars()));
}

void scanner::must_get_any_punctuation(sc_token& token, std::initializer_list<const char*> strings)
{
	//wait couldn't this accept punctuation type enums instead?
	must_get_string(token);
	if (token.get_token_type() == token_type::punctuation)
	{
		for (const char* str : strings)
		{
			if (!token.get_chars().compare(str))
			{
				return; //found a match
			}
		}
	}

	bool append_or = false; 
	std::string msg = "Expected ";
	for (const char* str : strings)
	{
		if (append_or)
			msg.append(" or ");
		msg.append(str);
		append_or = true;
	}
	msg.append(std::format(", got {}", token.get_chars()));
	raise_error(msg);
}

void scanner::must_get_quoted_string(sc_token& token)
{
	must_get_string(token);
	if (token.get_token_type() != token_type::quoted_string)
		raise_error(std::format("Expected a string, got {}.", token.get_chars()));
}

bool scanner::advance(int amount)
{
	Assert(cursor <= buf.size());
	if (cursor == buf.size())
		return true; //already at eof

	for (int i = 0; i < amount; i++)
	{
		if (buf[cursor] == '\n')
		{
			line_num++;
			column = 1;
		}
		cursor++;
		column++;
		if (cursor == buf.size())
			return true;
	}

	return false;
}

bool scanner::clear_whitespace()
{
	for (;;)
	{
		//Skip any whitespace
		while (iswspace(buf[cursor]))
		{
			if (advance())
				return true;
		}

		//Check for comments.
		//Does it start with /? Is there enough space to even have a comment?
		if (buf[cursor] == '/' && cursor < buf.size())
		{
			//Check for // line comment. 
			if (buf[cursor + 1] == '/')
			{
				if (clear_to_end_of_line()) //hit EOF?
					return true;

				continue;
			}
			//Check for /* block comment. 
			else if (buf[cursor + 1] == '*')
			{
				//Skip over the "/*"
				if (advance(2))
				{
					raise_error("EOF in block comment");
					return true;
				}

				for (;;)
				{
					if (cursor < buf.size() - 1)
					{
						if (buf[cursor] == '*' && buf[cursor + 1] == '/')
						{
							if (advance(2))
								return true; //hit EOF. 
							break;
						}
					}
					else
					{
						raise_error("EOF in block comment");
						return true;
					}
					advance();
				}

				continue;
			}
		}

		return false;
	}
	return false;
}

bool scanner::clear_to_end_of_line()
{
	char ch = buf[cursor];
	while (ch != '\n')
	{
		if (advance())
			return true;

		ch = buf[cursor];
	}

	if (cursor == buf.size())
		return true;
	return false;
}

bool scanner::clear_to_next_line()
{
	char ch = buf[cursor];
	while (ch != '\n')
	{
		if (advance())
			return true;

		ch = buf[cursor];
	}

	return advance();
}

void scanner::include_document(std::string_view document_name, std::string_view document_buf)
{
	document_stack.emplace(name, buf, cursor, line_num, column);

	name = document_name;
	buf = document_buf;
	cursor = 0;
	line_num = 1;
	column = 1;
}

void scanner::read_quoted_string()
{
	bool last_escaped = false;
	bool found_end = false;

	size_t len = 0;

	std::string new_token;
	int startline = line_num;
	int startcol = column;

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
			else if (ch == '\n' || ch == '\r')
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
	last_token = sc_token(startline, startcol, new_token, token_type::quoted_string);
}

void scanner::read_unquoted_string()
{
	std::string new_token;
	int startline = line_num;
	int startcol = column;

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

	last_token = sc_token(startline, startcol, new_token, token_type::string);
}

void scanner::read_number()
{
	bool found_dot = false;
	std::string new_token;
	int startline = line_num;
	int startcol = column;

	while (true)
	{
		char ch = buf[cursor];

		//TODO: Should any validation of the statement be done here?
		//Suffixes and alternate bases aren't supported, but should they be?
		if (ch == '.')
		{
			if (found_dot)
				break; //Punctuation between operations. Should this be an error at the lexer level?

			found_dot = true;
			new_token += ch;
		}
		//Negative sign can only be at the beginning of a token. Otherwise it could be a minus between operands. 
		else if (ch == '-' && new_token.size() == 0)
		{
			new_token += ch;
		}
		else if (!isdigit(ch))
			break; //Check non-digit after dots since otherwise the . would fail this check. 
		else
			new_token += ch;

		if (advance())
			break; //hit EOF but it's fine, the last token in a script can be a number
	}

	last_token = sc_token(startline, startcol, new_token, token_type::number);
}

void scanner::read_punct()
{
	//This is very silly, but I make a string view from cursor to the end of the buffer, so I don't have to count manually
	std::string_view punct_view = std::string_view(buf.data() + cursor, buf.size() - cursor);

	for (punctuation_entry const& entry : punctuation_strings)
	{
		if (punct_view.starts_with(entry.str))
		{
			last_token = sc_token(line_num, column, std::string(entry.str), entry.type);
			advance(entry.str.size());
			return;
		}
	}

	//Can't reach here except in an internal error. 
	raise_error("Internal error: Detected punctuation token, but didn't find punctuation sequence.");
	Int3();
}

bool scanner::check_or_pop_pending_document()
{
	if (document_stack.empty())
		return true;

	//A document is pending, so pop it back into the scanner's state.
	scanner_stack& entry = document_stack.top();
	buf = std::move(entry.buf);
	name = std::move(entry.name);
	cursor = entry.cursor;
	line_num = entry.line_num;
	
	//This should avoid deallocating things because the strings were moved. 
	document_stack.pop();

	return false;
}

bool scanner::read_string(sc_token& token)
{
	if (unread)
	{
		//last token was unread so give it back. 
		unread = false;
		token = last_token;
		return true;
	}

	Assert(cursor <= buf.size());
	bool skipping_whitespace;

	do
	{
		skipping_whitespace = false;
		if (cursor == buf.size())
			if (check_or_pop_pending_document())
				return false; //At end of buffer, so nothing left to read
			else
				skipping_whitespace = true; //Popped a document off the stack, will need to clear whitespace on this document. 

		//Read to the first non-whitespace character. 
		if (clear_whitespace())
			if (check_or_pop_pending_document())
				return false;
			else
				skipping_whitespace = true; //Popped a document off the stack, will need to clear whitespace on this document. 
	} while (skipping_whitespace);

	std::string_view remaining(buf.data() + cursor, buf.size() - cursor);

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

	token = last_token;
	return true;
}

void scanner::unread_token()
{
	Assert(!unread);
	unread = true;
}

scanner_stack::scanner_stack(std::string& doc_name, std::string& doc_buf, size_t doc_cursor, int doc_line_num, int doc_column)
{
	name = std::move(doc_name);
	buf = std::move(doc_buf);
	cursor = doc_cursor;
	line_num = doc_line_num;
	column = doc_column;
}

scanner_stack::scanner_stack(const scanner_stack& other)
{
	name = other.name;
	buf = other.buf;
	cursor = other.cursor;
	line_num = other.line_num;
	column = other.column;
}

scanner_stack::scanner_stack(scanner_stack&& other) noexcept
{
	name = std::move(other.name);
	buf = std::move(other.buf);
	cursor = other.cursor;
	line_num = other.line_num;
	column = other.column;
}

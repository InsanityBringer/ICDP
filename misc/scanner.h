/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

#pragma once

#include <string_view>
#include <string>
#include <stack>
#include <span>

enum class punctuation_type
{
	none,
	equals,
	comma,
	period,
	curly_right,
	curly_left,
	semicolon,
	square_right,
	square_left,
	colon,
};

struct punctuation_entry
{
	punctuation_type type;
	std::string_view str;
};

//Format of a token. 
enum class token_format
{
	none, //You never read anything or something bad happened
	string, //Token is composed of characters
	integer, //Token is composed of an integer
	floating, //Token is composed of a floating-point value.
	empty, //Token is empty. Used for punctuation since there's no need to store characters ATM. 
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
	int linenum, colnum;
	token_format format;
	token_type type;
	punctuation_type punctuation; //For punctuation, type of the punctuation encountered. 
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
		punctuation = punctuation_type::none;
		linenum = colnum = 0;
	}

	//Constructs an initialized token with the string chars.
	sc_token(int line, int col, std::string chars, token_type type) : linenum(line), colnum(col), chars(chars), type(type)
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
		punctuation = punctuation_type::none;
	}

	//Constructs an initialized punctuation token
	sc_token(int line, int col, std::string chars, punctuation_type punctuation) : linenum(line), colnum(col), chars(chars), punctuation(punctuation)
	{
		format = token_format::empty;
		type = token_type::punctuation;
		dval = ival = 0;
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

	punctuation_type get_punctuation_type() const
	{
		return punctuation;
	}

	int compare(const char* str)
	{
		return chars.compare(str);
	}

	int compare(std::string& str)
	{
		return chars.compare(str);
	}
};

//Entry of a document that needs to be processed after finishing the current document. 
struct scanner_stack
{
	std::string name;		//Name of the document, for diagonstics.
	std::string buf;		//Contents of the documents. 
	size_t		cursor;		//Position in the buffer that this document will resume at. 
	int			line_num;	//Line number that this document will resume at. 
	int			column;

	//Creates a new scanner stack entry. 
	//WARNING: This moves the passed strings, so they are invalid after creation!
	//Thus, this should only ever be called from scanner::include_document. 
	scanner_stack(std::string& doc_name, std::string& doc_buf, size_t doc_cursor, int doc_line_num, int doc_column);
	scanner_stack(const scanner_stack& other);
	scanner_stack(scanner_stack&& other) noexcept;
};

//A token stream vaguely similar to Hexen's sc_man, but from scratch.
//Note: Not all fuctions of this have been tested yet, still needs further development. 
class scanner
{
	int			line_num;
	int			column;
	size_t		cursor;
	std::string name;				//Name of the document, for diagonstics. 
	std::string buf;
	token_type	last_token_type;
	sc_token	last_token;
	bool		unread;				//If true, read_string will initialize token with last_token.

	std::string error;

	std::stack<scanner_stack> document_stack;

	//Advances the cursor. Returns true if EOF, false otherwise. 
	//This will automatically scan for newlines. 
	bool advance(int amount = 1);
	//Reads a quoted string
	void read_quoted_string();
	//Reads a unquoted string
	void read_unquoted_string();
	//Reads a number
	void read_number();
	//Reads punctuation
	void read_punct();
	//At EOF, checks if there are pending documents to be processed.
	//Returns true if EOF is reached, or false if there's still documents to process.
	bool check_or_pop_pending_document();

public:
	//Initializes the scanner with no document name, copying the contents of the string view buf into a new string,
	//so the initial buffer can be freed without problems. 
	scanner(std::string_view buf);
	//Initializes the scanner, copying the contents of the string view buf into a new string,
	//so the initial buffer can be freed without problems. 
	scanner(std::string_view document_name, std::string_view buf);

	//Reads one string from the buffer. Returns true if read, false otherwise
	bool read_string(sc_token& token);

	//Unreads the previous token, allowing it to be read again
	void unread_token();

	//Reads all whitespace. Returns true if EOF, false otherwise.
	bool clear_whitespace();
	//Advances the cursor to either the end of the line, or EOF. 
	bool clear_to_end_of_line();
	//Advances the cursor to either the start of the next line, or EOF.
	bool clear_to_next_line();

	//Includes another document onto the stack. After calling this, all parsing will start parsing from the included document,
	//and will only return to the current position of the initial document once the document is parsed to EOF.
	void include_document(std::string_view document_name, std::string_view buf);

	//Gets the last token read. 
	sc_token& get_last_token()
	{
		return last_token;
	}

	int get_line_num()
	{
		return line_num;
	}

	void raise_error(const char* msg);
	void raise_error(const std::string& msg);

	//Gets the next string, errors if no more tokens available.
	void must_get_string(sc_token& token); 
	//Gets the next string, errors if it is not a unquoted string
	void must_get_identifier(sc_token& token);
	//Gets the next string, errors if it is not a number. The number can be either integer or floating point. 
	void must_get_number(sc_token& token);
	//Gets the next string, errors if it is not an integer.
	void must_get_integer(sc_token& token);
	//Gets the next string, errors if it is not the specified punctuation
	void must_get_any_punctuation(sc_token& token, std::initializer_list<const char*> strings);
	//Gets the next string, errors if it is not a quoted string
	void must_get_quoted_string(sc_token& token);
};

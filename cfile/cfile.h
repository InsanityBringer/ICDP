/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#pragma once

#include <stdio.h>
#include <string>
#include <vector>
#include <stdexcept>
#include "misc/types.h"
#include "misc/error.h"
#include "fix/fix.h"
#include "vecmat/vecmat.h"

struct CFILE
{
	FILE*	file;
	int		size;
	int		lib_offset;
	int		raw_position;
};


bool cfile_add_hogfile(const char* hogname);

CFILE* cfopen(const char* filename, const char* mode);
int cfilelength(CFILE* fp);							// Returns actual size of file...
size_t cfread(void* buf, size_t elsize, size_t nelem, CFILE* fp);
void cfclose(CFILE* cfile);
int cfgetc(CFILE* fp);
int cfseek(CFILE* fp, long int offset, int where);
int cftell(CFILE* fp);
char* cfgets(char* buf, size_t n, CFILE* fp);

int cfexist(const char* filename);	// Returns true if file exists on disk (1) or in hog (2).

//[ISB] little endian reading functions
uint8_t cfile_read_byte(CFILE* fp);
short cfile_read_short(CFILE* fp);
int cfile_read_int(CFILE* fp);;

#define cfile_read_fix(a) ((fix)cfile_read_int(a))

//[ISB] normal file versions of these because why not
uint8_t file_read_byte(FILE* fp);
short file_read_short(FILE* fp);
int file_read_int(FILE* fp);
int64_t file_read_int64(FILE* fp);
void file_write_byte(FILE* fp, uint8_t b);
void file_write_short(FILE* fp, short s);
void file_write_int(FILE* fp, int i);
void file_write_int64(FILE* fp, int64_t i);
//Unlike cfgets, this will only end at null terminators, not newlines. 
void cfile_get_string(char* buffer, int count, CFILE* fp);

// Allows files to be gotten from an alternate hog file.
// Passing NULL disables this.
bool cfile_use_alternate_hogfile(const char* name);

//Adds an additional search dir for searching for files.
//This can be with or without a separator on the end. 
bool cfile_add_alternate_searchdir(const char* dir);

void cfile_read_vector(vms_vector *vec, CFILE* fp);
void cfile_read_angvec(vms_angvec *vec, CFILE* fp);

struct hogfile
{
	char	name[13];
	int		offset;
	int 	length;
};

class hogarchive
{
	std::string archivename;
	std::vector<hogfile> hogfiles;
public:
	hogarchive(const char* filename) : archivename(filename)
	{
		FILE* fp = fopen(filename, "rb");

		if (fp)
		{
			char id[4];
			fread(id, 3, 1, fp);
			id[3] = '\0';

			if (strncmp(id, "DHF", 3))
			{
				fclose(fp);
				throw std::runtime_error("HOG archive has bad signature");
			}

			while (1)
			{
				hogfile file = {};

				int i = fread(file.name, 13, 1, fp);
				if (i != 1) //got all of them..
				{
					fclose(fp);
					return;
				}

				file.length = file_read_int(fp);
				if (file.length < 0)
					Warning("Hogfile length < 0");
				file.offset = ftell(fp);
				hogfiles.push_back(file);

				// Skip over
				i = fseek(fp, file.length, SEEK_CUR);
			}
		}
	}

	size_t num_files() const
	{
		return hogfiles.size();
	}

	CFILE* open(const char* filename)
	{
		//As the filesystem becomes more complex, this probably will have to stop being a linear search
		for (auto it = hogfiles.begin(); it < hogfiles.end(); it++)
		{
			if (!_stricmp(it->name, filename))
			{
				FILE* fp = fopen(archivename.c_str(), "rb");

				fseek(fp, it->offset, SEEK_SET);
				CFILE* cfile = (CFILE*)malloc(sizeof(CFILE));
				if (cfile == NULL)
					return NULL;

				cfile->file = fp;
				cfile->size = it->length;
				cfile->lib_offset = it->offset;
				cfile->raw_position = 0;
				return cfile;
			}
		}

		return nullptr;
	}

	bool exists(const char* filename)
	{
		//As the filesystem becomes more complex, this probably will have to stop being a linear search
		for (auto it = hogfiles.begin(); it < hogfiles.end(); it++)
		{
			if (!_stricmp(it->name, filename))
			{
				return true;
			}
		}

		return false;
	}
};

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

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <vector>
#include <stdexcept>

#include "platform/platform_filesys.h"
#include "platform/posixstub.h"
#include "cfile/cfile.h"
#include "mem/mem.h"
#include "misc/error.h"
#include "fix/fix.h"
#include "vecmat/vecmat.h"

struct hogfile
{
	char	name[13];
	int	offset;
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

std::vector<hogarchive> hogarchives;
std::vector<hogarchive> alternate_hogarchives;

FILE* cfile_get_filehandle(const char* filename, const char* mode)
{
	//char temp[CHOCOLATE_MAX_FILE_PATH_SIZE];

	FILE* fp = fopen(filename, mode);

#ifndef _WINDOWS
	if (!fp)
	{
		//[ISB] Try either uppercase or lowercase conversion if the file isn't found. 
		strncpy(temp, filename, HOG_FILENAME_MAX);
		_strupr(temp);
		fp = fopen(temp, mode);
		if (!fp)
		{
			strncpy(temp, filename, HOG_FILENAME_MAX);
			_strlwr(temp);
			fp = fopen(temp, mode);
		}
		
	}
#endif

	//TODO: I need to be able to merge directories into the cfile filesystem
	//but this system isn't particlarly clean. 
	/*if ((fp == NULL) && (AltHogdir_initialized)) 
	{
		strncpy(temp, AltHogDir, HOG_FILENAME_MAX);
		strncat(temp, AltHogDir, HOG_FILENAME_MAX);
		fp = fopen(temp, mode);
	}*/

	return fp;
}

bool cfile_add_hogfile(const char* hogname)
{
	hogarchive archive(hogname);
	if (archive.num_files() == 0) //either didn't load or no files
		return false;

	hogarchives.push_back(std::move(archive));

	return true;
}

bool cfile_libfile_exists(const char* name)
{
	//Search alternate archives first
	for (int i = alternate_hogarchives.size() - 1; i >= 0; i--)
	{
		if (alternate_hogarchives[i].exists(name))
			return true;
	}

	//Search core archives second, so that data can be overridden by alternate archives
	for (int i = hogarchives.size() - 1; i >= 0; i--)
	{
		if (hogarchives[i].exists(name))
			return true;
	}

	return false;
}

CFILE* cfile_find_libfile(const char* name)
{
	CFILE* fp;
	//Search alternate archives first
	//Search is done in reverse so that later loaded hogfiles (for addons, presumably) take priority over lower priority ones (like the mission hog itself)
	for (int i = alternate_hogarchives.size() - 1; i >= 0; i--)
	{
		fp = alternate_hogarchives[i].open(name);
		if (fp) //found it?
			return fp;
	}

	//Search core archives second, so that data can be overridden by alternate archives
	for (int i = hogarchives.size() - 1; i >= 0; i--)
	{
		fp = hogarchives[i].open(name);
		if (fp) //found it?
			return fp;
	}
	return nullptr;
}

bool cfile_use_alternate_hogfile(const char* name)
{
	if (name)
	{
		hogarchive archive(name);
		if (archive.num_files() == 0)
			return false;

		alternate_hogarchives.push_back(std::move(archive));
		return true;
	}
	else 
	{
		alternate_hogarchives.clear();
		return true;
	}
}

int cfexist(const char* filename)
{
	FILE* fp;

	if (filename[0] != '\x01')
		fp = cfile_get_filehandle(filename, "rb");		// Check for non-hog file first...
	else 
	{
		fp = NULL;		//don't look in dir, only in hogfile
		filename++;
	}

	if (fp) 
	{
		fclose(fp);
		return 1; // file found in filesystem
	}

	if (cfile_libfile_exists(filename))
	{
		return 2;		// file found in hog
	}

	return 0;		// Couldn't find it.
}


CFILE* cfopen(const char* filename, const char* mode)
{
	FILE* fp;

	if (_stricmp(mode, "rb")) 
	{
		Warning("CFILES CAN ONLY BE OPENED WITH RB\n");
		exit(1);
	}

	//[ISB] descent 2 code for handling '\x01'
	if (filename[0] != '\x01')
	{
		fp = cfile_get_filehandle(filename, mode);		// Check for non-hog file first...
	}
	else
	{
		fp = NULL;		//don't look in dir, only in hogfile
		filename++;
	}

	if (!fp) 
	{
		return cfile_find_libfile(filename);
	}
	else
	{
		CFILE* cfile = (CFILE*)malloc(sizeof(CFILE));
		if (cfile == NULL) 
		{
			fclose(fp);
			return NULL;
		}
		cfile->file = fp;
		cfile->size = _filelength(_fileno(fp));
		cfile->lib_offset = 0;
		cfile->raw_position = 0;
		return cfile;
	}
}

int cfilelength(CFILE* fp)
{
	return fp->size;
}

int cfgetc(CFILE* fp)
{
	int c;

	if (fp->raw_position >= fp->size) return EOF;

	c = getc(fp->file);
	if (c != EOF)
		fp->raw_position++;

	//	Assert( fp->raw_position==(ftell(fp->file)-fp->lib_offset) );

	return c;
}

char* cfgets(char* buf, size_t n, CFILE* fp)
{
	char* t = buf;
	int i;
	int c;

	for (i = 0; i < (int)(n - 1); i++) 
	{
		do 
		{
			if (fp->raw_position >= fp->size) 
			{
				*buf = 0;
				return NULL;
			}
			c = fgetc(fp->file);
			fp->raw_position++;
		} while (c == 13);
		*buf++ = c;
		if (c == 10)
			c = '\n';
		if (c == '\n') break;
	}
	*buf++ = 0;
	return  t;
}

size_t cfread(void* buf, size_t elsize, size_t nelem, CFILE* fp)
{
	int i;
	if ((int)(fp->raw_position + (elsize * nelem)) > fp->size) return EOF;
	i = fread(buf, elsize, nelem, fp->file);
	fp->raw_position += i * elsize;
	return i;
}


int cftell(CFILE* fp)
{
	return fp->raw_position;
}

int cfseek(CFILE* fp, long int offset, int where)
{
	int c, goal_position;

	switch (where) 
	{
	case SEEK_SET:
		goal_position = offset;
		break;
	case SEEK_CUR:
		goal_position = fp->raw_position + offset;
		break;
	case SEEK_END:
		goal_position = fp->size + offset;
		break;
	default:
		return 1;
	}
	c = fseek(fp->file, fp->lib_offset + goal_position, SEEK_SET);
	fp->raw_position = ftell(fp->file) - fp->lib_offset;
	return c;
}

void cfclose(CFILE* fp)
{
	fclose(fp->file);
	free(fp);
	return;
}

uint8_t cfile_read_byte(CFILE* fp)
{
	uint8_t b;
	cfread(&b, sizeof(uint8_t), 1, fp);
	return b;
}

short cfile_read_short(CFILE* fp)
{
	uint8_t b[2];
	short v;
	cfread(&b[0], sizeof(uint8_t), 2, fp);
	v = b[0] + (b[1] << 8);
	return v;
}

int cfile_read_int(CFILE* fp)
{
	uint8_t b[4];
	int v;
	cfread(&b[0], sizeof(uint8_t), 4, fp);
	v = b[0] + (b[1] << 8) + (b[2] << 16) + (b[3] << 24);
	return v;
}

uint8_t file_read_byte(FILE* fp)
{
	uint8_t b;
	fread(&b, sizeof(uint8_t), 1, fp);
	return b;
}

short file_read_short(FILE* fp)
{
	uint8_t b[2];
	short v;
	fread(&b[0], sizeof(uint8_t), 2, fp);
	v = b[0] + (b[1] << 8);
	return v;
}

int file_read_int(FILE* fp)
{
	uint8_t b[4];
	int v;
	fread(&b[0], sizeof(uint8_t), 4, fp);
	v = b[0] + (b[1] << 8) + (b[2] << 16) + (b[3] << 24);
	return v;
}

int64_t file_read_int64(FILE* fp)
{
	uint8_t b[8];
	int64_t v;
	fread(&b[0], sizeof(uint8_t), 8, fp);
	v = b[0] + (b[1] << 8) + (b[2] << 16) + (b[3] << 24) + 
		((int64_t)b[4] << 32) + ((int64_t)b[5] << 40) + ((int64_t)b[6] << 48) + ((int64_t)b[7] << 56);
	return v;
}

void file_write_byte(FILE* fp, uint8_t b)
{
	fwrite(&b, sizeof(uint8_t), 1, fp);
}

void file_write_short(FILE* fp, short s)
{
	uint8_t b1 = s & 255;
	uint8_t b2 = (s >> 8) & 255;
	fwrite(&b1, sizeof(uint8_t), 1, fp);
	fwrite(&b2, sizeof(uint8_t), 1, fp);
}

void file_write_int(FILE* fp, int i)
{
	uint8_t b1 = i & 255;
	uint8_t b2 = (i >> 8) & 255;
	uint8_t b3 = (i >> 16) & 255;
	uint8_t b4 = (i >> 24) & 255;
	fwrite(&b1, sizeof(uint8_t), 1, fp);
	fwrite(&b2, sizeof(uint8_t), 1, fp);
	fwrite(&b3, sizeof(uint8_t), 1, fp);
	fwrite(&b4, sizeof(uint8_t), 1, fp);
}

void file_write_int64(FILE* fp, int64_t i)
{
	uint8_t b1 = i & 255;
	uint8_t b2 = (i >> 8) & 255;
	uint8_t b3 = (i >> 16) & 255;
	uint8_t b4 = (i >> 24) & 255;
	uint8_t b5 = (i >> 32) & 255;
	uint8_t b6 = (i >> 40) & 255;
	uint8_t b7 = (i >> 48) & 255;
	uint8_t b8 = (i >> 56) & 255;
	fwrite(&b1, sizeof(uint8_t), 1, fp);
	fwrite(&b2, sizeof(uint8_t), 1, fp);
	fwrite(&b3, sizeof(uint8_t), 1, fp);
	fwrite(&b4, sizeof(uint8_t), 1, fp);
	fwrite(&b5, sizeof(uint8_t), 1, fp);
	fwrite(&b6, sizeof(uint8_t), 1, fp);
	fwrite(&b7, sizeof(uint8_t), 1, fp);
	fwrite(&b8, sizeof(uint8_t), 1, fp);
}

//[ISB] so it turns out there was already a cfgets. OOPS. This is still needed for the level name loader though
void cfile_get_string(char* buffer, int count, CFILE* fp)
{
	int i = 0;
	char c;
	do
	{
		c = cfgetc(fp);
		if (i == count - 1) //At the null terminator, so ensure this character is always null
			* buffer = '\0';
		else if (i < count) //Still space to go
			* buffer++ = c;

		i++;
	} while (c != 0);
}

void cfile_read_vector(vms_vector* vec, CFILE* fp)
{
	vec->x = cfile_read_int(fp);
	vec->y = cfile_read_int(fp);
	vec->z = cfile_read_int(fp);
}

void cfile_read_angvec(vms_angvec *vec, CFILE* fp)
{
	vec->p = cfile_read_short(fp);
	vec->b = cfile_read_short(fp);
	vec->h = cfile_read_short(fp);
}

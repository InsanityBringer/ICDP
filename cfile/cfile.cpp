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


std::vector<hogarchive> hogarchives;
std::vector<hogarchive> alternate_hogarchives;

//This is a bit simple, but give every hogfile a unique handle.
//This handle can then be used to read files from a specific hogfile. 
static uint32_t last_handle = 1;

std::vector<std::string> alternate_searchdirs;

FILE* cfile_get_filehandle(const char* filename, const char* mode)
{
	char temp[CHOCOLATE_MAX_FILE_PATH_SIZE];

	//This is a bit hacky, but if the path is rooted, don't localize it
	//Filesystem localization should eventually be done at the level of cfopen. 
	if (strrchr(filename, PLATFORM_PATH_SEPARATOR))
	{
		strncpy(temp, filename, CHOCOLATE_MAX_FILE_PATH_SIZE);
		temp[CHOCOLATE_MAX_FILE_PATH_SIZE - 1] = '\0';
	}
	else
		get_full_file_path(temp, filename, nullptr);

	FILE* fp = fopen(temp, mode);

	if (!fp)
	{
		const char* ptr = strrchr(filename, PLATFORM_PATH_SEPARATOR);
		if (ptr) //it's rooted, so this can't be found in an alternate searchdir.
			return nullptr;

		//Didn't find it, so try alternate search dirs
		for (std::string& str : alternate_searchdirs)
		{
			temp[0] = '\0';
			snprintf(temp, CHOCOLATE_MAX_FILE_PATH_SIZE, "%s%s", str.c_str(), filename);
			temp[CHOCOLATE_MAX_FILE_PATH_SIZE - 1] = '\0';

			fp = fopen(temp, mode);
			if (fp)
				return fp; //found it. 
		}
	}

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

uint32_t cfile_add_hogfile(const char* hogname)
{
	hogarchive archive(hogname);
	if (archive.num_files() == 0) //either didn't load or no files
		return false;
	
	uint32_t handle = last_handle++;
	archive.handle() = handle;
	hogarchives.push_back(std::move(archive));

	return handle;
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
CFILE* cfile_find_libfile(const char* name, uint32_t handle = 0)
{
	CFILE* fp;
	//Search alternate archives first
	//Search is done in reverse so that later loaded hogfiles (for addons, presumably) take priority over lower priority ones (like the mission hog itself)
	for (int i = alternate_hogarchives.size() - 1; i >= 0; i--)
	{
		if (handle == 0 || handle == alternate_hogarchives[i].handle())
		{
			fp = alternate_hogarchives[i].open(name);
			if (fp) //found it?
				return fp;
		}
	}

	//Search core archives second, so that data can be overridden by alternate archives
	for (int i = hogarchives.size() - 1; i >= 0; i--)
	{
		if (handle == 0 || handle == alternate_hogarchives[i].handle())
		{
			fp = hogarchives[i].open(name);
			if (fp) //found it?
				return fp;
		}
	}
	return nullptr;
}

uint32_t cfile_use_alternate_hogfile(const char* name)
{
	if (name)
	{
		hogarchive archive(name);
		if (archive.num_files() == 0)
			return 0;

		uint32_t handle = last_handle++;
		archive.handle() = handle;

		alternate_hogarchives.push_back(std::move(archive));
		return handle;
	}
	else 
	{
		alternate_hogarchives.clear();
		return 0;
	}
}

uint32_t cfile_add_alternate_searchdir(const char* dir)
{
	if (dir)
	{
		std::string newdir(dir);
		if (newdir[newdir.size() - 1] != '/' && newdir[newdir.size() - 1] != PLATFORM_PATH_SEPARATOR) //thanks windows
			newdir.push_back(PLATFORM_PATH_SEPARATOR);
		
		alternate_searchdirs.push_back(std::move(newdir));
	}
	return last_handle++;
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

CFILE* cfopen_from(uint32_t handle, const char* filename)
{
	//The directory iteration code needs to be modified some to support handles (missions in a directory? could be nice)
	//[ISB] descent 2 code for handling '\x01'
	/*if (filename[0] != '\x01')
	{
		fp = cfile_get_filehandle(filename, "rb");		// Check for non-hog file first...
	}
	else
	{
		fp = NULL;		//don't look in dir, only in hogfile
		filename++;
	}*/

	return cfile_find_libfile(filename, handle);
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

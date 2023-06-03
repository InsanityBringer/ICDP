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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#pragma once

#include <stdint.h>
#include "platform/platform_filesys.h"

#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
#define FF_PATHSIZE CHOCOLATE_MAX_FILE_PATH_SIZE
#else
#define FF_PATHSIZE 256
#endif

#define FF_TYPE_FILE 1
#define FF_TYPE_DIR 2

typedef struct FILEFINDSTRUCT 
{
	uint32_t size;
	uint32_t type;
	char name[FF_PATHSIZE];
	bool temp_lfn_safe;
} FILEFINDSTRUCT;

int FileFindFirst(const char* search_str, FILEFINDSTRUCT* ffstruct);
//Temporary: this will serve parts of the engine that do support LFNs, untill all parts do
int FileFindFirstLFNTemp(const char* search_str, FILEFINDSTRUCT* ffstruct);
int FileFindNext(FILEFINDSTRUCT* ffstruct);
int FileFindClose(void);

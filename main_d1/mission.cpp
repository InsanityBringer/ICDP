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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <vector>

#include "platform/platform_filesys.h"
#include "platform/posixstub.h"
#include "cfile/cfile.h"
#include "platform/findfile.h" 
#include "inferno.h"
#include "mission.h"
#include "gameseq.h"
#include "titles.h"
#include "platform/mono.h"
#include "misc/error.h"

std::vector<mle> Mission_list;

int Current_mission_num;
char* Current_mission_filename, * Current_mission_longname;

//this stuff should get defined elsewhere

char Level_names[MAX_LEVELS_PER_MISSION][13];
char Secret_level_names[MAX_SECRET_LEVELS_PER_MISSION][13];

//strips damn newline from end of line
char* mfgets(char* s, int n, FILE* f)
{
	char* r;

	r = fgets(s, n, f);
	if (r && (s[strlen(s) - 1] == '\n' || s[strlen(s) - 1] == '\r'))
		s[strlen(s) - 1] = 0;

	return r;
}

//compare a string for a token. returns true if match
int istok(char* buf, const char* tok)
{
	return _strnicmp(buf, tok, strlen(tok)) == 0;

}

//adds a terminating 0 after a string at the first white space
void add_term(char* s)
{
	while (*s && !isspace(*s)) s++;

	*s = 0;		//terminate!
}

//returns ptr to string after '=' & white space, or NULL if no '='
//adds 0 after parm at first white space
char* get_value(char* buf)
{
	char* t;

	t = strchr(buf, '=');

	if (t) {
		t = t + 1;
		while (*t && isspace(*t)) t++;

		if (*t)
			return t;
	}

	return NULL;		//error!
}

//reads a line, returns ptr to value of passed parm.  returns NULL if none
char* get_parm_value(const char* parm, FILE* f)
{
	static char buf[80];

	if (!mfgets(buf, 80, f))
		return NULL;

	if (istok(buf, parm))
		return get_value(buf);
	else
		return NULL;
}

int ml_sort_func(mle* e0, mle* e1)
{
	return strcmp(e0->mission_name, e1->mission_name);
}

int get_msn_line(FILE* f, char* msn_line)
{
	memset(msn_line, 0, 256);
	int i = 0;

	int tmp_char = fgetc(f);

	while(tmp_char != '\n' && tmp_char != '\r' && tmp_char != EOF)
	{
		if (i == 255)
			msn_line[i] = '\0';
		else if (i < 255)
			msn_line[i] = (char)tmp_char;

		tmp_char = fgetc(f);
		i++;
	}

	while(tmp_char != EOF && (tmp_char == '\n' || tmp_char == '\r'))
	{
		tmp_char = fgetc(f);
	}

	if(tmp_char != EOF)
	{
		fseek(f, -1, SEEK_CUR);
	}

	return tmp_char;
}

void get_string_before_whitespace(const char* msn_line, char* trimmed_line)
{
	memset(trimmed_line, 0, 256);
	int i;

	for(i = 0; i < strlen(msn_line); i++)
	{
		if(isspace(msn_line[i]))
		{
			return;
		}

		trimmed_line[i] = msn_line[i];
	}
}

//returns true if file read ok, else false
bool read_mission_file(char* filename, int location, bool anarchy_mode)
{
	CFILE* mfile;
	//TODO: localization should be done in advance, but this isn't done yet since we're not LFN clean here. 
	char filename2[CHOCOLATE_MAX_FILE_PATH_SIZE];
	get_game_full_file_path(filename2, filename, CHOCOLATE_MISSIONS_DIR);

	mfile = cfopen(filename2, "rb");

	if (mfile) 
	{
		char* p;
		char temp[FILENAME_LEN], * t;
		char mission_name[MISSION_NAME_LEN + 1];
		bool anarchy_only_flag = false;

		strcpy(temp, filename); //[ISB TODO] This is a 
		if ((t = strchr(temp, '.')) == NULL)
			return false;	//missing extension
		*t = 0;			//kill extension

		//strncpy(Mission_list[count].filename, temp, MISSION_FILENAME_LEN);
		//Mission_list[count].anarchy_only_flag = 0;
		//Mission_list[count].location = location;

		//[ISB] uh, doesn't this desync cfile? Seems benign though...
		p = get_parm_value("name", mfile->file);
		
		if (p) 
		{
			char* t;
			if ((t = strchr(p, ';')) != NULL)
				* t = 0;
			t = p + strlen(p) - 1;
			while (isspace(*t)) t--;
			strncpy(mission_name, p, MISSION_NAME_LEN);
		}
		else 
		{
			cfclose(mfile);
			return false;
		}

		p = get_parm_value("type", mfile->file);

		//get mission type 
		if (p)
			anarchy_only_flag = istok(p, "anarchy");

		//Only append to the list if mission exists
		if (anarchy_mode || !anarchy_only_flag)
			Mission_list.emplace_back(temp, mission_name, anarchy_only_flag);

		cfclose(mfile);

		return true;
	}

	return false;
}


//fills in the global list of missions.  Returns the number of missions
//in the list.  If anarchy_mode set, don't include non-anarchy levels.
//if there is only one mission, this function will call load_mission on it.
int build_mission_list(bool anarchy_mode)
{
	//[ISB] ported from Descent 2, doesn't use system specific calls directly aaaa
	static int num_missions = -1;
	int special_count = 0;
	FILEFINDSTRUCT find;

	char search_name[CHOCOLATE_MAX_FILE_PATH_SIZE];
	char file_path_name[CHOCOLATE_MAX_FILE_PATH_SIZE];
	//get_platform_localized_query_string(search_name, CHOCOLATE_MISSIONS_DIR, "*.msn");
	get_game_full_file_path(search_name, "*.msn", CHOCOLATE_MISSIONS_DIR);

	Mission_list.clear();

	//now search for levels on disk
#ifndef DEST_SAT
	Mission_list.emplace_back("", "Descent: First Strike");
#endif

	special_count = 1;

	if (!FileFindFirst(search_name, &find)) 
	{
		do 
		{
			//get_full_file_path(file_path_name, find.name, CHOCOLATE_MISSIONS_DIR); //TODO: Can't localize here yet. 
			read_mission_file(find.name, 0, anarchy_mode);
		} while (!FileFindNext(&find));
		FileFindClose();
	}

	if (Mission_list.size() > special_count)
		qsort(&Mission_list.data()[special_count], Mission_list.size() - special_count, sizeof(Mission_list[0]), //do I actually need the data() there? Should write a better sort function using C++ means..
		(int (*)(const void*, const void*))ml_sort_func);

	//fill in built-in level

	load_mission(0);			//set built-in mission as default
	num_missions = Mission_list.size();
	return num_missions;
}

//values for built-in mission

#define BIM_LAST_LEVEL			27
#define BIM_LAST_SECRET_LEVEL	-3
#define BIM_BRIEFING_FILE		"briefing.tex"
#define BIM_ENDING_FILE			"endreg.tex"

//loads the specfied mission from the mission list.  build_mission_list()
//must have been called.  If build_mission_list() returns 0, this function
//does not need to be called.  Returns true if mission loaded ok, else false.
int load_mission(int mission_num)
{
	Current_mission_num = mission_num;

	mprintf((0, "Loading mission %d\n", mission_num));

	if (mission_num == 0) //built-in mission
	{
		int i;

		Last_level = BIM_LAST_LEVEL;
		Last_secret_level = BIM_LAST_SECRET_LEVEL;

		//build level names
		for (i = 0; i < Last_level; i++)
			sprintf(Level_names[i], "LEVEL%02d.RDL", i + 1);
		for (i = 0; i < -Last_secret_level; i++)
			sprintf(Secret_level_names[i], "LEVELS%1d.RDL", i + 1);

		Secret_level_table[0] = 10;
		Secret_level_table[1] = 21;
		Secret_level_table[2] = 24;

		strcpy(Briefing_text_filename, BIM_BRIEFING_FILE);
		strcpy(Ending_text_filename, BIM_ENDING_FILE);
		cfile_use_alternate_hogfile(NULL);		//disable alternate
	}
	else
	{		 
		//NOTE LINK TO ABOVE IF!!!!!
		//read mission from file 
		FILE* mfile;
		char buf[80], tmp[80], msn_line[256], trimmed_line[256], * v;
		int eof_check;

		char msn_filename[CHOCOLATE_MAX_FILE_PATH_SIZE], hog_filename[CHOCOLATE_MAX_FILE_PATH_SIZE],
		     msnfile_full_path[CHOCOLATE_MAX_FILE_PATH_SIZE], hogfile_full_path[CHOCOLATE_MAX_FILE_PATH_SIZE];
		snprintf(msn_filename, CHOCOLATE_MAX_FILE_PATH_SIZE, "%s.msn", Mission_list[mission_num].filename);
		snprintf(hog_filename, CHOCOLATE_MAX_FILE_PATH_SIZE, "%s.hog", Mission_list[mission_num].filename);

		get_game_full_file_path(msnfile_full_path, msn_filename, CHOCOLATE_MISSIONS_DIR);
		get_game_full_file_path(hogfile_full_path, hog_filename, CHOCOLATE_MISSIONS_DIR);

		cfile_use_alternate_hogfile(hogfile_full_path);

		mfile = fopen(msnfile_full_path, "rt");

		if (mfile == NULL)
		{
			Current_mission_num = -1;
			return 0;		//error!
		}

		//strcpy(buf, Mission_list[mission_num].filename);
		//strcat(buf, ".MSN");

		/*strcpy(tmp, Mission_list[mission_num].filename);
		strcat(tmp, ".HOG");

		cfile_use_alternate_hogfile(tmp);

		mfile = fopen(buf, "rt");

		if (mfile == NULL)
		{
			Current_mission_num = -1;
			return 0;		//error!
		}*/

		//init vars
		Last_level = 0;
		Last_secret_level = 0;
		Briefing_text_filename[0] = 0;
		Ending_text_filename[0] = 0;

		eof_check = get_msn_line(mfile, msn_line);

		while (eof_check != EOF)
		{
			if (istok(msn_line, "name"))
			{
				eof_check = get_msn_line(mfile, msn_line);
				continue;						//already have name, go to next line
			}
			else if (istok(msn_line, "type"))
			{
				eof_check = get_msn_line(mfile, msn_line);
				continue;						//already have name, go to next line				
			}
			else if (istok(msn_line, "hog")) 
			{
				char* bufp = msn_line;

				while (*(bufp++) != '=')
					;

				if (*bufp == ' ')
					while (*(++bufp) == ' ')
						;
				memset(hogfile_full_path, 0, CHOCOLATE_MAX_FILE_PATH_SIZE);
				get_game_full_file_path(hogfile_full_path, bufp, CHOCOLATE_MISSIONS_DIR);
				cfile_use_alternate_hogfile(hogfile_full_path);
				//cfile_use_alternate_hogfile(bufp);
				mprintf((0, "Hog file override = [%s]\n", bufp));
			}
			else if (istok(msn_line, "briefing")) 
			{
				if ((v = get_value(msn_line)) != NULL) 
				{
					add_term(v);
					if (strlen(v) < 13)
						strcpy(Briefing_text_filename, v);
				}
			}
			else if (istok(msn_line, "ending")) 
			{
				if ((v = get_value(msn_line)) != NULL) 
				{
					add_term(v);
					if (strlen(v) < 13)
						strcpy(Ending_text_filename, v);
				}
			}
			else if (istok(msn_line, "num_levels")) 
			{
				if ((v = get_value(msn_line)) != NULL) 
				{
					int n_levels, i;
					char* ext_idx;

					n_levels = atoi(v);

					for (i = 0; i < n_levels && get_msn_line(mfile, msn_line); i++) 
					{
						if ((v = get_value(msn_line)) != NULL)
						{
							strncpy(trimmed_line, v, strlen(v));
						}
						else
						{
							get_string_before_whitespace(msn_line, trimmed_line);
						}

						ext_idx = strrchr(trimmed_line, '.');

						if(ext_idx != NULL && ext_idx - trimmed_line > 2 && ext_idx[1] == 'h' && ext_idx[2] == 'o' && ext_idx[3] == 'g')
						{
							memset(hogfile_full_path, 0, CHOCOLATE_MAX_FILE_PATH_SIZE);
							get_game_full_file_path(hogfile_full_path, trimmed_line, CHOCOLATE_MISSIONS_DIR);
							cfile_use_alternate_hogfile(hogfile_full_path);
							//cfile_use_alternate_hogfile(trimmed_line);
						}
						else if (strlen(trimmed_line) <= 12 && i < MAX_LEVELS_PER_MISSION) 
						{
							strcpy(Level_names[i], trimmed_line);
							Last_level++;
						}
						else
							break;
					}

				}
			}
			else if (istok(msn_line, "num_secrets")) 
			{
				if ((v = get_value(msn_line)) != NULL)
				{
					int n_secret_levels, i;

					n_secret_levels = atoi(v);

					for (i = 0; i < n_secret_levels && get_msn_line(mfile, msn_line); i++) 
					{
						char* t;

						if ((t = strchr(msn_line, ',')) != NULL)* t++ = 0;
						else
							break;

						add_term(msn_line);
						if (strlen(msn_line) <= 12 && i < MAX_SECRET_LEVELS_PER_MISSION) 
						{
							strcpy(Secret_level_names[i], msn_line);
							Secret_level_table[i] = atoi(t);
							if (Secret_level_table[i]<1 || Secret_level_table[i]>Last_level)
								break;
							Last_secret_level--;
						}
						else
							break;
					}

				}
			}

			eof_check = get_msn_line(mfile, msn_line);
		}

		fclose(mfile);

		if (Last_level <= 0) 
		{
			Current_mission_num = -1;		//no valid mission loaded 
			return 0;
		}
	}

	Current_mission_filename = Mission_list[Current_mission_num].filename;
	Current_mission_longname = Mission_list[Current_mission_num].mission_name;

	return 1;
}

//loads the named mission if exists.
//Returns true if mission loaded ok, else false.
int load_mission_by_name(char* mission_name)
{
	int n, i;

	n = build_mission_list(true);

	if (strlen(mission_name) == 0)
		return load_mission(0);

	for (i = 0; i < n; i++)
		if (!_strfcmp(mission_name, Mission_list[i].filename))
			return load_mission(i);

	return 0;		//couldn't find mission
}

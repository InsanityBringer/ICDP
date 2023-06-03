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
#include <string.h>
#include <errno.h>

#include "platform/platform_filesys.h"
#include "platform/posixstub.h"
#include "misc/error.h"
#include "inferno.h"
#include "gameseq.h"
#include "player.h"
#include "playsave.h"
#include "platform/joy.h"
#include "kconfig.h"
#include "main_shared/digi.h"
#include "newmenu.h"
#include "joydefs.h"
#include "2d/palette.h"
#include "multi.h"
#include "menu.h"
#include "config.h"
#include "stringtable.h"
#include "platform/mono.h"
#include "state.h"
#include "cfile/cfile.h"
#include "misc/nbt.h"

#define SAVE_FILE_ID			'DPLR'

//[ISB] stupid error codes
#define EZERO 0

//this is for version 5 and below
typedef struct save_info_v5 
{
	int	id;
	short	saved_game_version, player_struct_version;
	int 	highest_level;
	int	default_difficulty_level;
	int	default_leveling_on;
} save_info_v5;

//this is for version 6 and above 
typedef struct save_info 
{
	int	id;
	short	saved_game_version, player_struct_version;
	int	n_highest_levels;				//how many highest levels are saved
	int	default_difficulty_level;
	int	default_leveling_on;
} save_info;

typedef struct hli 
{
	char	shortname[9];
	uint8_t	level_num;
} hli;

struct pilot_gameinfo
{
	std::string Game_name;
	std::vector<hli> highest_levels;
};

std::vector<pilot_gameinfo> all_gameinfo;
//This is a pointer out of all_gameinfo, so all_gameinfo shouldn't be modified except when loading pilots
//After all_gameinfo is populated from the pilot file, set this pointer. 
pilot_gameinfo* current_gameinfo;

int n_highest_levels;

hli highest_levels[MAX_MISSIONS];

#define SAVED_GAME_VERSION		7		//increment this every time saved_game struct changes

//version 5 -> 6: added new highest level information
//version 6 -> 7: stripped out the old saved_game array.

//the shareware is level 4

#define COMPATIBLE_SAVED_GAME_VERSION		4
#define COMPATIBLE_PLAYER_STRUCT_VERSION	16

typedef struct saved_game 
{
	char		name[GAME_NAME_LEN + 1];		//extra char for terminating zero
	player	playerinst;
	int		difficulty_level;		//which level game is played at
	int		primary_weapon;		//which weapon selected
	int		secondary_weapon;		//which weapon selected
	int		cockpit_mode;			//which cockpit mode selected
	int		window_w, window_h;	//size of player's window
	int		next_level_num;		//which level we're going to
	int		auto_leveling_on;		//does player have autoleveling on?
} saved_game;

saved_game saved_games[N_SAVE_SLOTS];

int Default_leveling_on = 1;

int D_LoadInfoHeader(FILE* fp, save_info* info)
{
	info->id = file_read_int(fp);
	info->saved_game_version = file_read_short(fp);
	info->player_struct_version = file_read_short(fp);
	info->n_highest_levels = file_read_int(fp);
	info->default_difficulty_level = file_read_int(fp);
	info->default_leveling_on = file_read_int(fp);

	return 1;
}

int D_WriteInfoHeader(FILE* fp, save_info* info)
{
	file_write_int(fp, info->id);
	file_write_short(fp, info->saved_game_version);
	file_write_short(fp, info->player_struct_version);
	file_write_int(fp, info->n_highest_levels);
	file_write_int(fp, info->default_difficulty_level);
	file_write_int(fp, info->default_leveling_on);
	return 1;
}

int D_LoadHighestLevel(FILE* fp, hli* info)
{
	fread(&info->shortname[0], sizeof(char), 9, fp);
	info->level_num = file_read_int(fp);
	return 1;
}

int D_WriteHighestLevel(FILE* fp, hli* info)
{
	fwrite(&info->shortname[0], sizeof(char), 9, fp);
	file_write_int(fp, info->level_num);
	return 1;
}

pilot_gameinfo generate_gameinfo_for_current_game()
{
	pilot_gameinfo gameinfo = {};
	gameinfo.Game_name = "Descent";

	//Generate hli for the built-in mission
	hli default_hli = {};
	default_hli.level_num = 1;
	gameinfo.highest_levels.push_back(default_hli);

	return gameinfo;
}

ListTag* generate_gameinfo_tag()
{
	ListTag* tag_p = new ListTag("Game_info");
	for (pilot_gameinfo& gameinfo : all_gameinfo)
	{
		CompoundTag* gtag_p = new CompoundTag();
		gtag_p->list.push_back(std::make_unique<StringTag>("Game_name", gameinfo.Game_name));
		ListTag* ltag_p = new ListTag("Highest_level_info");

		for (hli& entry : gameinfo.highest_levels)
		{
			CompoundTag* hlitag_p = new CompoundTag();
			hlitag_p->list.push_back(std::make_unique<StringTag>("Mission_name", entry.shortname));
			hlitag_p->list.push_back(std::make_unique<IntTag>("Highest_level", entry.level_num));

			ltag_p->put_tag(hlitag_p);
		}

		gtag_p->list.push_back(std::unique_ptr<Tag>(ltag_p));
		tag_p->put_tag(gtag_p);
	}

	return tag_p;
}

void read_highest_level_entry_tag(hli& hli_entry, CompoundTag& compoundtag)
{
	Tag* tag_p = compoundtag.find_tag("Mission_name");
	
	if (tag_p && tag_p->GetType() == NBTTag::String)
	{
		//TODO: hli::shortname needs to be std::string
		strncpy(hli_entry.shortname, ((StringTag*)tag_p)->value.c_str(), 9);
		hli_entry.shortname[8] = '\0';
	}

	hli_entry.level_num = nbt_get_integral(compoundtag.find_tag("Highest_level"), 0);
}

void read_gameinfo_entry(pilot_gameinfo& gameinfo, CompoundTag& compoundtag)
{
	Tag* tag_p = compoundtag.find_tag("Game_name");

	if (!tag_p || tag_p->GetType() != NBTTag::String)
		throw std::runtime_error("read_gameinfo_entry: Missing or invalid game name for gameinfo.");

	gameinfo.Game_name = ((StringTag*)tag_p)->value;

	tag_p = compoundtag.find_tag("Highest_level_info");

	if (tag_p && tag_p->GetType() == NBTTag::List)
	{
		ListTag* listtag_p = (ListTag*)tag_p;
		gameinfo.highest_levels.resize(listtag_p->size());
		for (int i = 0; i < listtag_p->size(); i++)
			read_highest_level_entry_tag(gameinfo.highest_levels[i], (CompoundTag&)*listtag_p->at(i));
	}
}

void read_gameinfo_tag(ListTag& listtag)
{
	if (listtag.get_list_type() != NBTTag::Compound)
		return;

	all_gameinfo.resize(listtag.size());
	for (int i = 0; i < listtag.size(); i++)
		read_gameinfo_entry(all_gameinfo[i], (CompoundTag&)*listtag.at(i));
}

void find_gameinfo_for_current_game()
{
	for (pilot_gameinfo& gameinfo : all_gameinfo)
	{
		if (!gameinfo.Game_name.compare("Descent"))
			current_gameinfo = &gameinfo;
	}

	//Didn't find it, so generate gameinfo
	if (!current_gameinfo)
	{
		all_gameinfo.push_back(generate_gameinfo_for_current_game());
		current_gameinfo = &all_gameinfo[all_gameinfo.size() - 1];
	}
}

void init_game_list()
{
	int i;

	for (i = 0; i < N_SAVE_SLOTS; i++)
		saved_games[i].name[0] = 0;
}

//[ISB] hack
extern const char* control_text[];
extern int choco_menu_remap[];
extern int choco_id_to_menu_remap[];

int new_player_config()
{
	int i, j, control_choice;
	newmenu_item m[7];

	/*
RetrySelection:
	for (i = 0; i < 5; i++) 
	{
		m[i].type = NM_TYPE_MENU; m[i].text = const_cast<char*>(control_text[choco_menu_remap[i]]);
	}
	m[0].text = TXT_CONTROL_KEYBOARD;

	control_choice = Config_control_type;				// Assume keyboard

	control_choice = newmenu_do1(NULL, TXT_CHOOSE_INPUT, 5, m, NULL, control_choice);

	if (control_choice < 0)
		return 0;

	//[ISB]: Remap chocolate input into the original input method. 
	control_choice = choco_menu_remap[control_choice];

	for (i = 0; i < CONTROL_MAX_TYPES; i++)
		for (j = 0; j < MAX_CONTROLS; j++)
			kconfig_settings[i][j] = default_kconfig_settings[i][j];
	kc_set_controls();

	Config_control_type = control_choice;*/

	kconfig_reset_keybinds();

	Player_default_difficulty = 1;
	Auto_leveling_on = Default_leveling_on = 1;
	n_highest_levels = 1;
	highest_levels[0].shortname[0] = 0;			//no name for mission 0
	highest_levels[0].level_num = 1;				//was highest level in old struct
	Config_joystick_sensitivity = 8;

	// Default taunt macros
#ifdef NETWORK
	strcpy(Network_message_macro[0], TXT_DEF_MACRO_1);
	strcpy(Network_message_macro[1], TXT_DEF_MACRO_2);
	strcpy(Network_message_macro[2], TXT_DEF_MACRO_3);
	strcpy(Network_message_macro[3], TXT_DEF_MACRO_4);
#endif

	//TODO: Once the real gameinfo system is created this will need to be hooked up to that
	current_gameinfo = nullptr; all_gameinfo.clear();
	all_gameinfo.push_back(generate_gameinfo_for_current_game());
	current_gameinfo = &all_gameinfo[0];

	return 1;
}

//read in the player's saved games.  returns errno (0 == no error)
int read_player_file()
{
#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	char filename_full_path[CHOCOLATE_MAX_FILE_PATH_SIZE];
#endif
	char filename[FILENAME_LEN];
	FILE* file;
	save_info info;
	int errno_ret = EZERO;

	Assert(Player_num >= 0 && Player_num < MAX_PLAYERS);

	//sprintf(filename, "%8s.plr", Players[Player_num].callsign);
#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	snprintf(filename, FILENAME_LEN, "%s.nplt", Players[Player_num].callsign);
	get_full_file_path(filename_full_path, filename, CHOCOLATE_PILOT_DIR);
	file = fopen(filename_full_path, "rb");
#else
	snprintf(filename, FILENAME_LEN, "%s.nplt", Players[Player_num].callsign);
	file = fopen(filename, "rb");
#endif

	//check filename
	/*if (file && isatty(fileno(file)))  //[ISB] TODO: fixme
	{
		//if the callsign is the name of a tty device, prepend a char
		fclose(file);
		sprintf(filename, "$%.7s.plr", Players[Player_num].callsign);
		file = fopen(filename, "rb");
	}*/

	if (!file) 
	{
		return errno;
	}

	/*if (D_LoadInfoHeader(file, &info) != 1) 
	{
		errno_ret = errno;
		fclose(file);
		return errno_ret;
	}

	if (info.id != SAVE_FILE_ID) 
	{
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Invalid player file");
		fclose(file);
		return -1;
	}

	if (info.saved_game_version < COMPATIBLE_SAVED_GAME_VERSION || info.player_struct_version < COMPATIBLE_PLAYER_STRUCT_VERSION) 
	{
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_ERROR_PLR_VERSION);
		fclose(file);
		return -1;
	}

	if (info.saved_game_version <= 5) 
	{
		//deal with old-style highest level info

		n_highest_levels = 1;

		highest_levels[0].shortname[0] = 0;							//no name for mission 0
		highest_levels[0].level_num = info.n_highest_levels;	//was highest level in old struct

		//This hack allows the player to start on level 8 if he's made it to
		//level 7 on the shareware.  We do this because the shareware didn't
		//save the information that the player finished level 7, so the most
		//we know is that he made it to level 7.
		if (info.n_highest_levels == 7)
			highest_levels[0].level_num = 8;

	}
	else //read new highest level info
	{
		n_highest_levels = info.n_highest_levels;

		for (int hl = 0; hl < n_highest_levels; hl++)
		{
			if (D_LoadHighestLevel(file, &highest_levels[hl]) != 1)
			{
				errno_ret = errno;
				fclose(file);
				return errno_ret;
			}
		}
	}

	Player_default_difficulty = info.default_difficulty_level;
	Default_leveling_on = info.default_leveling_on;

	if (info.saved_game_version < 7) // Read old saved games.
	{
		if (fread(saved_games, sizeof(saved_games), 1, file) != 1) 
		{
			errno_ret = errno;
			fclose(file);
			return errno_ret;
		}
	}

	//read taunt macros
	{
		int i, len;

		len = (info.saved_game_version == 4) ? SHAREWARE_MAX_MESSAGE_LEN : MAX_MESSAGE_LEN;

#ifdef NETWORK
		for (i = 0; i < 4; i++)
			if (fread(Network_message_macro[i], len, 1, file) != 1)
			{
				errno_ret = errno; break;
			}
#else
		i = 0;
		fseek(file, 4 * len, SEEK_CUR); //[ISB] this presumably shouldn't be 48?
#endif
	}

	//read kconfig data
	{
		if (fread(kconfig_settings, MAX_CONTROLS * CONTROL_MAX_TYPES, 1, file) != 1)
			errno_ret = errno;
		else if (fread(&Config_control_type, sizeof(uint8_t), 1, file) != 1)
			errno_ret = errno;
		else if (fread(&Config_joystick_sensitivity, sizeof(uint8_t), 1, file) != 1)
			errno_ret = errno;

		if (errno_ret == EZERO)
		{
			kc_set_controls();
		}
	}*/

	Tag* tag_p = Tag::read_tag(file);
	if (tag_p->GetType() != NBTTag::Compound)
		errno_ret = 1;

	if (fclose(file) && errno_ret == EZERO)
		errno_ret = errno;

	/*if (info.saved_game_version == COMPATIBLE_SAVED_GAME_VERSION) 
	{
		int i;

		Assert(N_SAVE_SLOTS == 10);

		for (i = 0; i < N_SAVE_SLOTS; i++) 
		{
			if (saved_games[i].name[0]) 
			{
				state_save_old_game(i, saved_games[i].name, &saved_games[i].playerinst,
					saved_games[i].difficulty_level, saved_games[i].primary_weapon,
					saved_games[i].secondary_weapon, saved_games[i].next_level_num);
			}
		}
		write_player_file();
	}*/

	//temp code to copy over the legacy HLI info into the new gameinfo system
	/*current_gameinfo = nullptr; all_gameinfo.clear();
	all_gameinfo.push_back(generate_gameinfo_for_current_game());
	current_gameinfo = &all_gameinfo[0];

	for (int hlinum = 0; hlinum < n_highest_levels; hlinum++)
	{
		current_gameinfo->highest_levels.push_back(highest_levels[hlinum]);
	}*/

	if (errno_ret == EZERO) //file read okay, so process the data
	{
		kconfig_reset_keybinds();

		CompoundTag* roottag_p = (CompoundTag*)tag_p;
		current_gameinfo = nullptr; //Going to reload the gameinfo, so the vector may move around.
		
		tag_p = roottag_p->find_tag("Game_info");
		if (tag_p && tag_p->GetType() == NBTTag::List)
			read_gameinfo_tag((ListTag&)*tag_p);

		tag_p = roottag_p->find_tag("Control_info");
		if (tag_p && tag_p->GetType() == NBTTag::Compound)
			kc_read_bindings_from_controlinfo_tag((CompoundTag&)*tag_p);

		//Read loose toggles
		Player_default_difficulty = nbt_get_integral(roottag_p->find_tag("Default_difficulty_level"), 1);
		Default_leveling_on = nbt_get_integral(roottag_p->find_tag("Auto_leveling"), 1);

		find_gameinfo_for_current_game();
	}

	if (tag_p)
		delete tag_p;

	return errno_ret;
}

//finds entry for this level in table.  if not found, returns ptr to 
//empty entry.  If no empty entries, takes over last one 
int find_hli_entry()
{
	int i;

	for (i = 0; i < n_highest_levels; i++)
		if (!_strfcmp(highest_levels[i].shortname, Mission_list[Current_mission_num].filename))
			break;

	if (i == n_highest_levels) //not found.  create entry
	{
		if (i == MAX_MISSIONS)
			i--;		//take last entry
		else
			n_highest_levels++;

		strcpy(highest_levels[i].shortname, Mission_list[Current_mission_num].filename);
		highest_levels[i].level_num = 0;
	}

	return i;
}

//set a new highest level for player for this mission
void set_highest_level(int levelnum)
{
	int ret, i;

	if ((ret = read_player_file()) != EZERO)
		if (ret != ENOENT)		//if file doesn't exist, that's ok
			return;

	i = find_hli_entry();

	if (levelnum > highest_levels[i].level_num)
		highest_levels[i].level_num = levelnum;

	write_player_file();
}

//gets the player's highest level from the file for this mission
int get_highest_level(void)
{
	int i;
	int highest_saturn_level = 0;
	read_player_file();
#ifndef DEST_SAT
	if (strlen(Mission_list[Current_mission_num].filename) == 0)
	{
		for (i = 0; i < n_highest_levels; i++)
			if (!_strfcmp(highest_levels[i].shortname, "DESTSAT")) 	//	Destination Saturn.
				highest_saturn_level = highest_levels[i].level_num;
	}
#endif
	i = highest_levels[find_hli_entry()].level_num;
	if (highest_saturn_level > i)
		i = highest_saturn_level;
	return i;
}


//write out player's saved games.  returns errno (0 == no error)
int write_player_file()
{
	ListTag* gameinfo_tag = generate_gameinfo_tag();
	CompoundTag* controlinfo_tag = kc_create_controlinfo_tag();

	CompoundTag rootTag = CompoundTag("Pilot");
	rootTag.list.emplace_back(controlinfo_tag);
	rootTag.list.emplace_back(gameinfo_tag);

	//Toggles are currently loose tags, but there may not be any particular reason to group them up. 
	rootTag.list.push_back(std::make_unique<ByteTag>("Default_difficulty_level", Player_default_difficulty));
	rootTag.list.push_back(std::make_unique<ByteTag>("Auto_leveling", Auto_leveling_on));

	//Structure is generated, so now serialize it.
	int errno_ret = WriteConfigFile();

#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	char filename_full_path[CHOCOLATE_MAX_FILE_PATH_SIZE];
#endif
	char filename[_MAX_PATH];

#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	snprintf(filename, FILENAME_LEN, "%s.nplt", Players[Player_num].callsign);
	get_full_file_path(filename_full_path, filename, CHOCOLATE_PILOT_DIR);
	FILE* file = fopen(filename_full_path, "wb");
#else
	sprintf(filename, "%s.nplt", Players[Player_num].callsign);
	FILE* file = fopen(filename, "wb");
#endif

	if (!file)
		return errno;

	errno_ret = EZERO;

	rootTag.write_tag(file);

	if (fclose(file))
		errno_ret = errno;

	if (errno_ret != EZERO)
	{
		remove(filename);			//delete bogus file
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "%s\n\n%s", TXT_ERROR_WRITING_PLR, strerror(errno_ret));
	}

	return errno_ret;
}

//returns errno (0 == no error)
int save_player_game(int slot_num, const char* text)
{
	int ret;

	if ((ret = read_player_file()) != EZERO)
		if (ret != ENOENT)		//if file doesn't exist, that's ok
			return ret;

	Assert(slot_num < N_SAVE_SLOTS);

	strcpy(saved_games[slot_num].name, text);

	saved_games[slot_num].playerinst = Players[Player_num];

	saved_games[slot_num].difficulty_level = Difficulty_level;
	saved_games[slot_num].auto_leveling_on = Auto_leveling_on;
	saved_games[slot_num].primary_weapon = Primary_weapon;
	saved_games[slot_num].secondary_weapon = Secondary_weapon;
	saved_games[slot_num].cockpit_mode = Cockpit_mode;
	saved_games[slot_num].window_w = Game_window_w;
	saved_games[slot_num].window_h = Game_window_h;
	saved_games[slot_num].next_level_num = Next_level_num;

	return write_player_file();
}


//returns errno (0 == no error)
int load_player_game(int slot_num)
{
	char save_callsign[CALLSIGN_LEN + 1];
	int ret;

	Assert(slot_num < N_SAVE_SLOTS);

	if ((ret = read_player_file()) != EZERO)
		return ret;

	Assert(saved_games[slot_num].name[0] != 0);

	strcpy(save_callsign, Players[Player_num].callsign);
	Players[Player_num] = saved_games[slot_num].playerinst;
	strcpy(Players[Player_num].callsign, save_callsign);

	Difficulty_level = saved_games[slot_num].difficulty_level;
	Auto_leveling_on = saved_games[slot_num].auto_leveling_on;
	Primary_weapon = saved_games[slot_num].primary_weapon;
	Secondary_weapon = saved_games[slot_num].secondary_weapon;
	Cockpit_mode = saved_games[slot_num].cockpit_mode;
	Game_window_w = saved_games[slot_num].window_w;
	Game_window_h = saved_games[slot_num].window_h;

	Players[Player_num].level = saved_games[slot_num].next_level_num;

	return EZERO;
}

//fills in a list of pointers to strings describing saved games
//returns the number of non-empty slots
//returns -1 if this is a new player
int get_game_list(char* game_text[N_SAVE_SLOTS])
{
	int i, count, ret;

	ret = read_player_file();

	for (i = count = 0; i < N_SAVE_SLOTS; i++) 
	{
		if (game_text)
			game_text[i] = saved_games[i].name;

		if (saved_games[i].name[0])
			count++;
	}

	return (ret == EZERO) ? count : -1;		//-1 means new file was created

}

//update the player's highest level.  returns errno (0 == no error)
int update_player_file()
{
	int ret;

	if ((ret = read_player_file()) != EZERO)
		if (ret != ENOENT)		//if file doesn't exist, that's ok
			return ret;

	return write_player_file();
}

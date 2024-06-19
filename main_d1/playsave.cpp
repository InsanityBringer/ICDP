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
#include <random>

#include "platform/platform_filesys.h"
#include "platform/posixstub.h"
#include "gameinfo/gameinfo.h"
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

struct hli
{
	char	shortname[9];
	uint8_t	level_num;
};

struct pilot_gameinfo
{
	std::string Game_name;
	std::vector<hli> highest_levels;
};

std::vector<pilot_gameinfo> all_gameinfo;
//This is a pointer out of all_gameinfo, so all_gameinfo shouldn't be modified except when loading pilots
//After all_gameinfo is populated from the pilot file, set this pointer. 
pilot_gameinfo* current_gameinfo;

int Default_leveling_on = 1;
int Primary_autoselect_mode, Secondary_autoselect_mode;

int Player_message_level = MSG_ALL;
uint32_t Player_random_identifier;

static bool Seeded_random_gen = false;
static std::mt19937 Player_random;

static uint32_t get_random_identifier()
{
	if (!Seeded_random_gen)
	{
		//From cppreference, is this enough entropy to ensure a good distribution?
		std::random_device r;
		std::seed_seq seed2{r(), r(), r(), r(), r(), r(), r(), r()};
		Player_random.seed(seed2);

		Seeded_random_gen = true;
	}

	return Player_random();
}

pilot_gameinfo generate_gameinfo_for_current_game()
{
	pilot_gameinfo gameinfo = {};
	gameinfo.Game_name = gameinfo_get_current_game_prefix();

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
		if (!gameinfo.Game_name.compare(gameinfo_get_current_game_prefix()))
			current_gameinfo = &gameinfo;
	}

	//Didn't find it, so generate gameinfo
	if (!current_gameinfo)
	{
		all_gameinfo.push_back(generate_gameinfo_for_current_game());
		current_gameinfo = &all_gameinfo[all_gameinfo.size() - 1];
	}
}

//[ISB] hack
extern const char* control_text[];
extern int choco_menu_remap[];
extern int choco_id_to_menu_remap[];

int new_player_config()
{
	kconfig_reset_keybinds();

	Player_default_difficulty = 1;
	Auto_leveling_on = Default_leveling_on = 1;
	Config_joystick_sensitivity = 8;
	Primary_autoselect_mode = AS_ALWAYS;
	Secondary_autoselect_mode = AS_ALWAYS;

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

	Player_random_identifier = get_random_identifier();

	return 1;
}

//read in the player's saved games.  returns errno (0 == no error)
int read_player_file()
{
	char filename_full_path[CHOCOLATE_MAX_FILE_PATH_SIZE];
	char filename[CHOCOLATE_MAX_FILE_PATH_SIZE];
	int errno_ret = EZERO;

	Assert(Player_num >= 0 && Player_num < MAX_PLAYERS);

	//sprintf(filename, "%8s.plr", Players[Player_num].callsign);
	snprintf(filename, FILENAME_MAX + 1, "%s.nplt", Players[Player_num].callsign);
	get_full_file_path(filename_full_path, filename, CHOCOLATE_PILOT_DIR);
	FILE* file = fopen(filename_full_path, "rb");
	//snprintf(filename, CHOCOLATE_MAX_FILE_PATH_SIZE, "%s.nplt", Players[Player_num].callsign);
	//filename[CHOCOLATE_MAX_FILE_PATH_SIZE - 1] = '\0';
	//FILE* file = fopen(filename, "rb");

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

	Tag* tag_p = Tag::read_tag(file);
	if (tag_p->GetType() != NBTTag::Compound)
		errno_ret = 1;

	if (fclose(file) && errno_ret == EZERO)
		errno_ret = errno;

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
		Primary_autoselect_mode = nbt_get_integral(roottag_p->find_tag("Primary_autoselect_mode"), AS_ALWAYS);
		Secondary_autoselect_mode = nbt_get_integral(roottag_p->find_tag("Secondary_autoselect_mode"), AS_ALWAYS);
		Player_message_level = nbt_get_integral(roottag_p->find_tag("Player_message_level"), MSG_NUM_MODES);

		tag_p = roottag_p->find_tag("Player_random_identifier");
		if (tag_p)
			Player_random_identifier = nbt_get_integral(tag_p, 0);
		else
			Player_random_identifier = get_random_identifier();

		//validation
		Primary_autoselect_mode = std::min(std::max(0, Primary_autoselect_mode), AS_NUM_MODES - 1);
		Secondary_autoselect_mode = std::min(std::max(0, Secondary_autoselect_mode), AS_NUM_MODES - 1);
		Player_message_level = std::min(std::max(0, Player_message_level), MSG_NUM_MODES - 1);

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

	for (i = 0; i < current_gameinfo->highest_levels.size(); i++)
		if (!_strfcmp(current_gameinfo->highest_levels[i].shortname, Mission_list[Current_mission_num].filename))
			break;

	if (i == current_gameinfo->highest_levels.size()) //not found.  create entry
	{
		current_gameinfo->highest_levels.emplace_back();
		strcpy(current_gameinfo->highest_levels[i].shortname, Mission_list[Current_mission_num].filename);
		current_gameinfo->highest_levels[i].level_num = 0;
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

	if (levelnum > current_gameinfo->highest_levels[i].level_num)
		current_gameinfo->highest_levels[i].level_num = levelnum;

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
		for (i = 0; i < current_gameinfo->highest_levels.size(); i++)
			if (!_strfcmp(current_gameinfo->highest_levels[i].shortname, "DESTSAT")) 	//	Destination Saturn.
				highest_saturn_level = current_gameinfo->highest_levels[i].level_num;
	}
#endif
	i = current_gameinfo->highest_levels[find_hli_entry()].level_num;
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
	rootTag.list.push_back(std::make_unique<ByteTag>("Primary_autoselect_mode", Primary_autoselect_mode));
	rootTag.list.push_back(std::make_unique<ByteTag>("Secondary_autoselect_mode", Secondary_autoselect_mode));
	rootTag.list.push_back(std::make_unique<ByteTag>("Player_message_level", Player_message_level));

	rootTag.list.push_back(std::make_unique<IntTag>("Player_random_identifier", Player_random_identifier));

	//Structure is generated, so now serialize it.
	int errno_ret = WriteConfigFile();

	char filename_full_path[CHOCOLATE_MAX_FILE_PATH_SIZE];
	char filename[CHOCOLATE_MAX_FILE_PATH_SIZE];

	snprintf(filename, FILENAME_LEN + 1, "%s.nplt", Players[Player_num].callsign);
	get_full_file_path(filename_full_path, filename, CHOCOLATE_PILOT_DIR);
	FILE* file = fopen(filename_full_path, "wb");
	//sprintf(filename, "%s.nplt", Players[Player_num].callsign);
	//FILE* file = fopen(filename, "wb");

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

//update the player's highest level.  returns errno (0 == no error)
int update_player_file()
{
	int ret;

	if ((ret = read_player_file()) != EZERO)
		if (ret != ENOENT)		//if file doesn't exist, that's ok
			return ret;

	return write_player_file();
}

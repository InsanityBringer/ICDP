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

#include "platform/platform.h"
#include "platform/platform_filesys.h"
#include "platform/s_midi.h"
#include "platform/mouse.h"
#include "misc/types.h"
#include "game.h"
#include "main_shared/digi.h"
#include "kconfig.h"
#include "2d/palette.h"
#include "platform/joy.h"
#include "misc/args.h"
#include "player.h"
#include "mission.h"
#include "misc/error.h"
#include "network.h"

static const char* digi_dev_str = "DigiDeviceID";
static const char* digi_port_str = "DigiPort";
static const char* digi_irq_str = "DigiIrq";
static const char* digi_dma_str = "DigiDma";
static const char* digi_volume_str = "DigiVolume";
static const char* midi_volume_str = "MidiVolume";
static const char* midi_dev_str = "MidiDeviceID";
static const char* midi_port_str = "MidiPort";
static const char* detail_level_str = "DetailLevel";
static const char* gamma_level_str = "GammaLevel";
static const char* stereo_rev_str = "StereoReverse";
static const char* joystick_min_str = "JoystickMin";
static const char* joystick_max_str = "JoystickMax";
static const char* joystick_cen_str = "JoystickCen";
static const char* last_player_str = "LastPlayer";
static const char* last_mission_str = "LastMission";
static const char* port_number_str = "NetworkPort";

static const char* WindowWidthStr = "WindowWidth";
static const char* WindowHeightStr = "WindowHeight";
static const char* FitModeStr = "FitMode";
static const char* FullscreenStr = "Fullscreen";
static const char* SoundFontPath = "SoundFontPath";
static const char* MouseScalarStr = "MouseScalar";
static const char* SwapIntervalStr = "SwapInterval";
static const char* GenDeviceStr = "PreferredGenMidiDevice";
static const char* RenderWidthStr = "RenderWidth";
static const char* RenderHeightStr = "RenderHeight";
static const char* RenderAspectStr = "RenderAspect";
static const char* MMEDeviceStr = "MMEDevice";
static const char* FPSLimitStr = "FPSLimit";

char config_last_player[CALLSIGN_LEN + 1] = "";
char config_last_mission[MISSION_NAME_LEN + 1] = "";

int Config_digi_type = 0;
int Config_midi_type = 0;

int Config_fps_limit = 30;

extern int8_t	Object_complexity, Object_detail, Wall_detail, Wall_render_depth, Debris_amount, SoundChannels;

void set_custom_detail_vars(void);

int ReadConfigFile()
{
	FILE* infile;
	char line[512], * token, * value, * ptr;
	uint8_t gamma;
	int joy_axis_min[4];
	int joy_axis_center[4];
	int joy_axis_max[4];
	int i;

	strcpy(config_last_player, "");

	joy_axis_min[0] = joy_axis_min[1] = joy_axis_min[2] = joy_axis_min[3] = 0;
	joy_axis_max[0] = joy_axis_max[1] = joy_axis_max[2] = joy_axis_max[3] = 0;
	joy_axis_center[0] = joy_axis_center[1] = joy_axis_center[2] = joy_axis_center[3] = 0;

	WindowWidth = 800;
	WindowHeight = 600;

	digi_driver_board = 0;
	digi_driver_port = 0;
	digi_driver_irq = 0;
	digi_driver_dma = 0;

	digi_midi_type = 0;
	digi_midi_port = 0;

	Config_digi_volume = 4;
	Config_midi_volume = 4;
	Config_control_type = 0;
	Config_channels_reversed = 0;

	char filename[CHOCOLATE_MAX_FILE_PATH_SIZE];
	get_full_file_path(filename, "descent.cfg", CHOCOLATE_CONFIG_DIR);
	infile = fopen(filename, "rt");

	//infile = fopen("descent.cfg", "rt");
	if (infile == NULL) 
	{
		return 1;
	}
	while (!feof(infile))
	{
		memset(line, 0, 512);
		fgets(line, 512, infile);
		ptr = &(line[0]);
		while (isspace(*ptr))
			ptr++;
		if (*ptr != '\0') 
		{
			token = strtok(ptr, "=");
			value = strtok(NULL, "=");
			if (!strcmp(token, digi_dev_str))
				digi_driver_board = strtol(value, NULL, 16);
			else if (!strcmp(token, digi_port_str))
				digi_driver_port = strtol(value, NULL, 16);
			else if (!strcmp(token, digi_irq_str))
				digi_driver_irq = strtol(value, NULL, 10);
			else if (!strcmp(token, digi_dma_str))
				digi_driver_dma = strtol(value, NULL, 10);
			else if (!strcmp(token, digi_volume_str))
				Config_digi_volume = (uint8_t)strtol(value, NULL, 10);
			else if (!strcmp(token, midi_dev_str))
				digi_midi_type = strtol(value, NULL, 16);
			else if (!strcmp(token, midi_port_str))
				digi_midi_port = strtol(value, NULL, 16);
			else if (!strcmp(token, midi_volume_str))
				Config_midi_volume = (uint8_t)strtol(value, NULL, 10);
			else if (!strcmp(token, stereo_rev_str))
				Config_channels_reversed = (uint8_t)strtol(value, NULL, 10);
			else if (!strcmp(token, gamma_level_str)) 
			{
				gamma = (uint8_t)strtol(value, NULL, 10);
				gr_palette_set_gamma(gamma);
			}
			else if (!strcmp(token, detail_level_str)) 
			{
				Detail_level = strtol(value, NULL, 10);
				if (Detail_level == NUM_DETAIL_LEVELS - 1)
				{
					int count, dummy, oc, od, wd, wrd, da, sc;

					count = sscanf(value, "%d,%d,%d,%d,%d,%d,%d\n", &dummy, &oc, &od, &wd, &wrd, &da, &sc);

					if (count == 7) 
					{
						Object_complexity = oc;
						Object_detail = od;
						Wall_detail = wd;
						Wall_render_depth = wrd;
						Debris_amount = da;
						SoundChannels = sc;
						set_custom_detail_vars();
					}
				}
			}
			else if (!strcmp(token, last_player_str)) 
			{
				char* p;
				strncpy(config_last_player, value, CALLSIGN_LEN);
				config_last_player[CALLSIGN_LEN] = '\0';
				p = strchr(config_last_player, '\n');
				if (p)* p = 0;
			}
			else if (!strcmp(token, last_mission_str)) 
			{
				char* p;
				strncpy(config_last_mission, value, MISSION_NAME_LEN);
				config_last_mission[MISSION_NAME_LEN] = '\0';
				p = strchr(config_last_mission, '\n');
				if (p)* p = 0;
			}
#ifdef NETWORK
			else if (!strcmp(token, port_number_str))
			{
				Current_Port = (uint16_t)strtol(value, NULL, 10);
			}
#endif

			//New configuration that used to be in chocolatedescent.cfg
			if (!strcmp(token, WindowWidthStr))
				WindowWidth = strtol(value, NULL, 10);
			else if (!strcmp(token, WindowHeightStr))
				WindowHeight = strtol(value, NULL, 10);
			if (!strcmp(token, RenderWidthStr))
				cfg_render_width = strtol(value, NULL, 10);
			else if (!strcmp(token, RenderHeightStr))
				cfg_render_height = strtol(value, NULL, 10);
			else if (!strcmp(token, RenderAspectStr))
				cfg_aspect_ratio = strtol(value, NULL, 10);
			else if (!strcmp(token, FitModeStr))
				BestFit = strtol(value, NULL, 10);
			else if (!strcmp(token, FullscreenStr))
				Fullscreen = strtol(value, NULL, 10);
			else if (!strcmp(token, MouseScalarStr))
				MouseScalar = strtof(value, NULL);
			else if (!strcmp(token, SwapIntervalStr))
				SwapInterval = strtol(value, NULL, 10);
			else if (!strcmp(token, SoundFontPath))
			{
				char* p;
#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
				memset(&SoundFontFilename[0], 0, CHOCOLATE_MAX_FILE_PATH_SIZE);
				get_full_file_path(&SoundFontFilename[0], value, CHOCOLATE_SOUNDFONTS_DIR);
#else
				strncpy(&SoundFontFilename[0], value, CHOCOLATE_MAX_FILE_PATH_SIZE - 1);
				SoundFontFilename[CHOCOLATE_MAX_FILE_PATH_SIZE - 1] = '\0';
#endif
				//[ISB] godawful hack from Descent's config parser, should fix parsing the soundfont path
				p = strchr(SoundFontFilename, '\n');
				if (p)*p = 0;
			}
			else if (!strcmp(token, GenDeviceStr))
				PreferredGenDevice = (GenDevices)strtol(value, NULL, 10);
			else if (!strcmp(token, MMEDeviceStr))
				PreferredMMEDevice = strtol(value, NULL, 10);
			else if (!strcmp(token, FPSLimitStr))
				Config_fps_limit = strtol(value, NULL, 10);
		}
	}

	fclose(infile);

	i = FindArg("-volume");

	if (i > 0) {
		i = atoi(Args[i + 1]);
		if (i < 0) i = 0;
		if (i > 100) i = 100;
		Config_digi_volume = (i * 8) / 100;
		Config_midi_volume = (i * 8) / 100;
	}

	if (Config_digi_volume > 8) Config_digi_volume = 8;
	if (Config_midi_volume > 8) Config_midi_volume = 8;

	digi_set_volume((Config_digi_volume * 32768) / 8, (Config_midi_volume * 128) / 8);

	Config_midi_type = digi_midi_type;
	Config_digi_type = digi_driver_board;

	//some basic validation
	if (WindowWidth <= 320) WindowWidth = 320;
	if (WindowHeight <= 200) WindowHeight = 200;
	if (PreferredMMEDevice < -1) PreferredMMEDevice = -1;

	if (cfg_render_width < 320) cfg_render_width = 320;
	if (cfg_render_height < 200) cfg_render_height = 200;

	if (cfg_aspect_ratio < 0 || cfg_aspect_ratio >= GAMEASPECT_COUNT) cfg_aspect_ratio = GAMEASPECT_AUTO;
	if (Config_fps_limit < 5)
		Config_fps_limit = 5;
	else if (Config_fps_limit > 150)
		Config_fps_limit = 150;

	FPSLimit = Config_fps_limit;

	return 0;
}

int WriteConfigFile()
{
	FILE* infile;
	char str[256];
	int joy_axis_min[4];
	int joy_axis_center[4];
	int joy_axis_max[4];
	uint8_t gamma = gr_palette_get_gamma();

	char filename[CHOCOLATE_MAX_FILE_PATH_SIZE];
	get_full_file_path(filename, "descent.cfg", CHOCOLATE_CONFIG_DIR);
	infile = fopen(filename, "wt");
	//infile = fopen("descent.cfg", "wt");

	if (infile == NULL) 
	{
		return 1;
	}
	sprintf(str, "%s=0x%x\n", digi_dev_str, Config_digi_type);
	fputs(str, infile);
	sprintf(str, "%s=0x%x\n", digi_port_str, digi_driver_port);
	fputs(str, infile);
	sprintf(str, "%s=%d\n", digi_irq_str, digi_driver_irq);
	fputs(str, infile);
	sprintf(str, "%s=%d\n", digi_dma_str, digi_driver_dma);
	fputs(str, infile);
	sprintf(str, "%s=%d\n", digi_volume_str, Config_digi_volume);
	fputs(str, infile);
	sprintf(str, "%s=0x%x\n", midi_dev_str, Config_midi_type);
	fputs(str, infile);
	sprintf(str, "%s=0x%x\n", midi_port_str, digi_midi_port);
	fputs(str, infile);
	sprintf(str, "%s=%d\n", midi_volume_str, Config_midi_volume);
	fputs(str, infile);
	sprintf(str, "%s=%d\n", stereo_rev_str, Config_channels_reversed);
	fputs(str, infile);
	sprintf(str, "%s=%d\n", gamma_level_str, gamma);
	fputs(str, infile);
	if (Detail_level == NUM_DETAIL_LEVELS - 1)
		sprintf(str, "%s=%d,%d,%d,%d,%d,%d,%d\n", detail_level_str, Detail_level,
			Object_complexity, Object_detail, Wall_detail, Wall_render_depth, Debris_amount, SoundChannels);
	else
		sprintf(str, "%s=%d\n", detail_level_str, Detail_level);
	fputs(str, infile);
	sprintf(str, "%s=%s\n", last_player_str, Players[Player_num].callsign);
	fputs(str, infile);
	sprintf(str, "%s=%s\n", last_mission_str, config_last_mission);
	fputs(str, infile);
#ifdef NETWORK
	sprintf(str, "%s=%d\n", port_number_str, Current_Port);
	fputs(str, infile);
#endif

	//New configuration keys that were from chocolatedescent.cfg
	fprintf(infile, "%s=%d\n", WindowWidthStr, WindowWidth);
	fprintf(infile, "%s=%d\n", WindowHeightStr, WindowHeight);
	fprintf(infile, "%s=%d\n", RenderWidthStr, cfg_render_width);
	fprintf(infile, "%s=%d\n", RenderHeightStr, cfg_render_height);
	fprintf(infile, "%s=%d\n", RenderAspectStr, cfg_aspect_ratio);
	fprintf(infile, "%s=%d\n", FitModeStr, BestFit);
	fprintf(infile, "%s=%d\n", FullscreenStr, Fullscreen);
	fprintf(infile, "%s=%f\n", MouseScalarStr, MouseScalar);
	fprintf(infile, "%s=%d\n", SwapIntervalStr, SwapInterval);
	if (SoundFontFilename[0])
		fprintf(infile, "%s=%s\n", SoundFontPath, SoundFontFilename);
	fprintf(infile, "%s=%d\n", GenDeviceStr, (int)PreferredGenDevice);
	fprintf(infile, "%s=%d\n", MMEDeviceStr, PreferredMMEDevice);
	fprintf(infile, "%s=%d\n", FPSLimitStr, Config_fps_limit);

	fclose(infile);
	return 0;
}

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

static char copyright[] = "DESCENT   COPYRIGHT (C) 1994,1995 PARALLAX SOFTWARE CORPORATION";

#include <stdio.h>

#if defined(__linux__) || defined(_WIN32) || defined(_WIN64)
#include <malloc.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "platform/platform_filesys.h"
#include "platform/posixstub.h"

#include "2d/gr.h"
#include "ui/ui.h"
#include "platform/mono.h"
#include "platform/key.h"
#include "platform/timer.h"
#include "3d/3d.h"
#include "bm.h"
#include "inferno.h"
#include "misc/error.h"
#include "cfile/cfile.h"
#include "game.h"
#include "segment.h"		//for Side_to_verts
#include "mem/mem.h"
#include "textures.h"
#include "segpoint.h"
#include "screens.h"
#include "texmap/texmap.h"
#include "main_shared/texmerge.h"
#include "menu.h"
#include "wall.h"
#include "switch.h"
#include "polyobj.h"
#include "main_shared/effects.h"
#include "main_shared/digi.h"
#include "iff/iff.h"
#include "2d/pcx.h"
#include "2d/palette.h"
#include "misc/args.h"
#include "sounds.h"
#include "titles.h"
#include "player.h"
#include "stringtable.h"
#include "render.h"
#ifdef NETWORK
#include "platform/i_net.h"
#endif
#include "newdemo.h"
#include "network.h"
#include "gamefont.h"
#include "kconfig.h"
#ifdef ARCADE
#include "arcade.h"
#include "coindev.h"
#endif
#include "platform/mouse.h"
#include "platform/joy.h"
#include "newmenu.h"
#include "config.h"
#include "joydefs.h"
#include "multi.h"
#include "main_shared/songs.h"
#include "cfile/cfile.h"
#include "gameseq.h"

#ifdef EDITOR
#include "editor\editor.h"
#include "editor\kdefs.h"
#endif

#include "vers_id.h"
#include "platform/platform.h"

int Function_mode = FMODE_MENU;		//game or editor?
int Screen_mode = -1;					//game screen or editor screen?

int WVIDEO_running = 0;		//debugger can set to 1 if running

#ifdef EDITOR
int Inferno_is_800x600_available = 0;
extern int bm_init_use_tbl();
#endif

extern fix fixed_frametime;

extern int piggy_low_memory;

void mem_int_to_string(int number, char* dest)
{
	int i, l, c;
	char buffer[20], * p;

	sprintf(buffer, "%d", number);

	l = strlen(buffer);
	if (l <= 3) {
		// Don't bother with less than 3 digits
		sprintf(dest, "%d", number);
		return;
	}

	c = 0;
	p = dest;
	for (i = l - 1; i >= 0; i--) {
		if (c == 3) {
			*p++ = ',';
			c = 0;
		}
		c++;
		*p++ = buffer[i];
	}
	*p++ = '\0';
	_strrev(dest);
}

int Inferno_verbose = 0;

extern int digi_timer_rate;

int descent_critical_error = 0;
unsigned descent_critical_deverror = 0;
unsigned descent_critical_errcode = 0;

extern int Network_allow_socket_changes;

#ifdef NETWORK
#include "netmisc.h"
#endif

//[ISB] Okay, the trouble is that SDL redefines main. I don't want to include SDL here. Solution is to rip off doom
//and add a separate main function
int D_DescentMain(int argc, const char** argv)
{
	int t;
	uint8_t title_pal[768];

#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	init_all_platform_localized_paths();
	validate_required_files();
#endif
	
	error_init(NULL);

	setbuf(stdout, NULL);	// unbuffered output via printf

	InitArgs(argc, argv);

	int initStatus = plat_init();
	if (initStatus)
	{
		Error("Error initalizing graphics library, code %d\n", initStatus);
		return 1;
	}

	if (FindArg("-verbose"))
		Inferno_verbose = 1;

	load_text(621);

	//	set_exit_message("\n\n%s", TXT_THANKS);

	printf("\nDESCENT   %s %s %s\n", VERSION_NAME, __DATE__, __TIME__);
	printf("%s\n%s\n", TXT_COPYRIGHT, TXT_TRADEMARK);

	if (FindArg("-?") || FindArg("-help") || FindArg("?")) 
	{
		printf("%s\n", TXT_COMMAND_LINE_0);

		printf("  -SimulEyes     %s\n",
			"Enables StereoGraphics SimulEyes VR stereo display");

		printf("  -Iglasses      %s\n", TXT_IGLASSES);
		printf("  -VioTrack <n>  %s n\n", TXT_VIOTRACK);
		printf("  -3dmaxLo       %s\n", TXT_KASAN);
		printf("                 %s\n", TXT_KASAN_2);
		printf("  -3dmaxHi       %s\n", TXT_3DMAX);
		printf("%s\n", TXT_COMMAND_LINE_1);
		printf("%s\n", TXT_COMMAND_LINE_2);
		printf("%s\n", TXT_COMMAND_LINE_3);
		printf("%s\n", TXT_COMMAND_LINE_4);
		printf("%s\n", TXT_COMMAND_LINE_5);
		printf("%s\n", TXT_COMMAND_LINE_6);
		printf("%s\n", TXT_COMMAND_LINE_7);
		printf("%s\n", TXT_COMMAND_LINE_8);
		printf("%s\n", TXT_COMMAND_LINE_9);
		printf("%s\n", TXT_COMMAND_LINE_10);
		printf("%s\n", TXT_COMMAND_LINE_11);
		printf("%s\n", TXT_COMMAND_LINE_12);
		printf("%s\n", TXT_COMMAND_LINE_13);
		printf("%s\n", TXT_COMMAND_LINE_14);
		printf("%s\n", TXT_COMMAND_LINE_15);
		printf("%s\n", TXT_COMMAND_LINE_16);
		printf("%s\n", TXT_COMMAND_LINE_17);
		printf("%s\n", TXT_COMMAND_LINE_18);
		printf("  -DynamicSockets %s\n", TXT_SOCKET);
		printf("  -NoFileCheck    %s\n", TXT_NOFILECHECK);
		printf("  -GamePort       %s\n", "Use Colorado Spectrum's Notebook Gameport");
		printf("  -NoDoubleBuffer %s\n", "Use only one page of video memory");
		printf("  -LCDBios        %s\n", "Enables LCDBIOS for using LCD shutter glasses");
		printf("  -JoyNice        %s\n", "Joystick poller allows interrupts to occur");
		set_exit_message("");
		return(0);
	}

	printf("\n%s\n", TXT_HELP);

#ifdef PASSWORD
	if ((t = FindArg("-pswd")) != 0) {
		int	n;
		int8_t* pp = Side_to_verts;
		int ch;
		for (n = 0; n < 6; n++)
			for (ch = 0; ch < strlen(Args[t + 1]); ch++)
				* pp++ ^= Args[t + 1][ch];
	}
	else
		Error("Invalid processor");		//missing password
#endif

	if (FindArg("-autodemo"))
		Auto_demo = 1;

#ifndef RELEASE
	if (FindArg("-noscreens"))
		Skip_briefing_screens = 1;
#endif

	//[ISB] Allow the user to configure the FPS limit, if desired
	int limitParam = FindArg("-fpslimit");
	if (limitParam && limitParam < (Num_args - 1))
	{
		FPSLimit = atoi(Args[limitParam + 1]);
		if (FPSLimit < 4) FPSLimit = 4; if (FPSLimit > 150) FPSLimit = 150;
	}
	if (Inferno_verbose) printf("Setting FPS Limit %d\n", FPSLimit);

	g3_set_lighting_mode(1);

	strcpy(Menu_pcx_name, "menu.pcx");	//	Used to be menu2.pcx.

#ifdef EDITOR
	Inferno_is_800x600_available = true;

	if (!Inferno_is_800x600_available) 
	{
		printf("The editor will not be available...\n");
		Function_mode = FMODE_MENU;
	}
#endif

#ifndef NDEBUG
	minit();
	mopen(0, 9, 1, 78, 15, "Debug Spew");
	mopen(1, 2, 1, 78, 5, "Errors & Serious Warnings");
#endif

	if (Inferno_verbose) printf("%s", TXT_VERBOSE_1);
	ReadConfigFile();
	if (Inferno_verbose) printf("\n%s", TXT_VERBOSE_2);

	timer_init();

	if (Inferno_verbose) printf("\n%s", TXT_VERBOSE_3);
	key_init();
	if (!FindArg("-nomouse")) {
		if (Inferno_verbose) printf("\n%s", TXT_VERBOSE_4);
		mouse_init(0);
	}
	else 
	{
		if (Inferno_verbose) printf("\n%s", TXT_VERBOSE_5);
	}
	if (!FindArg("-nojoystick")) 
	{
		if (Inferno_verbose) printf("\n%s", TXT_VERBOSE_6);
		joy_init();
	}
	else 
	{
		if (Inferno_verbose) printf("\n%s", TXT_VERBOSE_10);
	}
	if (Inferno_verbose) printf("\n%s", TXT_VERBOSE_11);

	kconfig_init_defaults();

	//------------ Init sound ---------------
	if (!FindArg("-nosound"))
	{
		if (digi_init()) 
		{
			printf("\nFailed to start sound.\n");
		}
	}
	else 
	{
		if (Inferno_verbose) printf("\n%s", TXT_SOUND_DISABLED);
	}

#ifdef NETWORK
	if (!FindArg("-nonetwork")) 
	{
		int showaddress = 0;
		int ipx_error;
		if (Inferno_verbose) printf("\n%s ", TXT_INITIALIZING_NETWORK);
		if ((t = FindArg("-port")))
			Current_Port = atoi(Args[t + 1]);
		if (FindArg("-showaddress")) showaddress = 1;
		if ((ipx_error = NetInit(Current_Port, showaddress)) == 0) 
		{
			if (Inferno_verbose) printf("%s %d.\n", "Using UDP port", Current_Port);
			Network_active = 1;
		}
		else
		{
			switch (ipx_error)
			{
			case 3: 	if (Inferno_verbose) printf("%s\n", TXT_NO_NETWORK); break;
			case -2: if (Inferno_verbose) printf("%s 0x%x.\n", TXT_SOCKET_ERROR, Current_Port); break;
			case -4: if (Inferno_verbose) printf("%s\n", TXT_MEMORY_IPX); break;
			default:
				if (Inferno_verbose) printf("%s %d", TXT_ERROR_IPX, ipx_error);
			}
			if (Inferno_verbose) printf("%s\n", TXT_NETWORK_DISABLED);
			Network_active = 0;		// Assume no network
		}
		if (FindArg("-dynamicsockets"))
			Network_allow_socket_changes = 1;
		else
			Network_allow_socket_changes = 0;
	}
	else 
	{
		if (Inferno_verbose) printf("%s\n", TXT_NETWORK_DISABLED);
		Network_active = 0;		// Assume no network
	}
#endif

	{
		int screen_width = 320;
		int screen_height = 200;
		int screen_compatible = 1;

		if (FindArg("-320x240")) 
		{
			if (Inferno_verbose) printf("Using 320x240 ModeX...\n");
			screen_width = 320;
			screen_height = 240;
			screen_compatible = 0;
		}

		if (FindArg("-320x400")) 
		{
			if (Inferno_verbose) printf("Using 320x400 ModeX...\n");
			screen_width = 320;
			screen_height = 400;
			screen_compatible = 0;
		}

		if (FindArg("-640x400"))
		{
			if (Inferno_verbose) printf("Using 640x400 VESA...\n");
			screen_width = 640;
			screen_height = 400;
			screen_compatible = 0;
		}

		if (FindArg("-640x480")) 
		{
			if (Inferno_verbose) printf("Using 640x480 VESA...\n");
			screen_width = 640;
			screen_height = 480;
			screen_compatible = 0;
		}

		if (FindArg("-1920x1080"))
		{
			if (Inferno_verbose) printf("Using 1920x1080...\n");
			screen_width = 1920;
			screen_height = 1080;
			screen_compatible = 0;
			Game_aspect = ASPECT_16_9;
		}
		if (FindArg("-320x100")) 
		{
			if (Inferno_verbose) printf("Using 320x100 VGA...\n");
			screen_width = 320;
			screen_height = 100;
			screen_compatible = 0;
		}

		game_init_render_buffers(0, screen_width, screen_height, screen_compatible);
	}

#ifdef NETWORK
	//	i = FindArg( "-rinvul" );
	//	if (i > 0) {
	//		int mins = atoi(Args[i+1]);
	//		if (mins > 314)
	//			mins = 314;
	// 	control_invul_time = mins/5;
	//	}
	control_invul_time = 0;
#endif

	if (Inferno_verbose) printf("\n%s\n\n", TXT_INITIALIZING_GRAPHICS);
	if ((t = gr_init()) != 0)
		Error(TXT_CANT_INIT_GFX, t);
	// Load the palette stuff. Returns non-zero if error.
	mprintf((0, "Going into graphics mode..."));
	mprintf((0, "\nInitializing palette system..."));
	gr_use_palette_table("PALETTE.256");
	mprintf((0, "\nInitializing font system..."));
	gamefont_init();	// must load after palette data loaded.
	songs_play_song(SONG_TITLE, 1);

#ifndef RELEASE
	if (!FindArg("-notitles"))
#endif
	{	//NOTE LINK TO ABOVE!
		show_title_screen("iplogo1.pcx", 1);
		show_title_screen("logo.pcx", 1);
	}

	{
		//grs_bitmap title_bm;
		int pcx_error;
		char filename[14];

		strcpy(filename, "descent.pcx");

		grs_canvas* descentcanv = gr_create_canvas(320, 200);
		if ((pcx_error = pcx_read_bitmap(filename, &descentcanv->cv_bitmap, descentcanv->cv_bitmap.bm_type, title_pal)) == PCX_ERROR_NONE) 
		{
			gr_palette_clear();
			//gr_bitmap( 0, 0, &title_bm );
			gr_palette_fade_canvas_in(*descentcanv, ASPECT_4_3, title_pal, 32, 0);
			//free(title_bm.bm_data);
			gr_free_canvas(descentcanv);
		}
		else 
		{
			gr_free_canvas(descentcanv);
			gr_close();
			Error("Couldn't load pcx file '%s', PCX load error: %s\n", filename, pcx_errormsg(pcx_error));
		}
	}

#ifdef EDITOR
	if (!FindArg("-nobm"))
		bm_init_use_tbl();
	else
		bm_init();
#else
	bm_init();
#endif

	if (FindArg("-norun"))
		return(0);

	mprintf((0, "\nInitializing 3d system...\n"));
	g3_init();
	//mprintf((0, "\nInitializing texture caching system..."));
	//texmerge_init(10);		// 10 cache bitmaps
	mprintf((0, "Running game...\n"));
	set_screen_mode(SCREEN_MENU);

	init_game();
	set_detail_level_parameters(Detail_level);

	Players[Player_num].callsign[0] = '\0';
	if (!Auto_demo) 
	{
		key_flush();
		RegisterPlayer();		//get player's name
	}

	gr_palette_fade_out(title_pal, 32, 0);

	//kconfig_load_all();

	Game_mode = GM_GAME_OVER;

	if (Auto_demo) 
	{
#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
		char demo_full_path[CHOCOLATE_MAX_FILE_PATH_SIZE];
		get_full_file_path(demo_full_path, "descent.dem", CHOCOLATE_DEMOS_DIR);
		newdemo_start_playback(demo_full_path);
#else
		newdemo_start_playback("DESCENT.DEM");
#endif
		if (Newdemo_state == ND_STATE_PLAYBACK)
			Function_mode = FMODE_GAME;
	}

	build_mission_list(0);		// This also loads mission 0.

	while (Function_mode != FMODE_EXIT)
	{
		switch (Function_mode) 
		{
		case FMODE_MENU:
			if (Auto_demo) 
			{
				newdemo_start_playback(NULL);		// Randomly pick a file
				if (Newdemo_state != ND_STATE_PLAYBACK)
					Error("No demo files were found for autodemo mode!");
			}
			else 
			{
				DoMenu();
#ifdef EDITOR
				if (Function_mode == FMODE_EDITOR) {
					create_new_mine();
					SetPlayerFromCurseg();
				}
#endif
			}
			break;
		case FMODE_GAME:
#ifdef EDITOR
			keyd_editor_mode = 0;
#endif
			game();
			if (Function_mode == FMODE_MENU)
				songs_play_song(SONG_TITLE, 1);
#ifdef EDITOR
			else if (Function_mode == FMODE_EDITOR) //[ISB] If you do menu->game->editor cursegp won't be valid. Fix this. 
			{
				if (!Cursegp)
					init_editor_data_for_mine();
			}
#endif
			break;
#ifdef EDITOR
		case FMODE_EDITOR:
			keyd_editor_mode = 1;
			editor();
			if (Function_mode == FMODE_GAME) 
			{
				Game_mode = GM_EDITOR;
				editor_reset_stuff_on_level();
				N_players = 1;
			}
			break;
#endif
		default:
			Error("Invalid function mode %d", Function_mode);
		}
	}

	WriteConfigFile();

#ifndef ROCKWELL_CODE
#ifndef RELEASE
	if (!FindArg("-notitles"))
#endif
		//NOTE LINK TO ABOVE!!
#ifndef EDITOR
		show_order_form();
#endif
#endif

#ifndef NDEBUG
	if (FindArg("-showmeminfo"))
		//		show_mem_info = 1;		// Make memory statistics show
#endif

		plat_close();
		return(0);		//presumably successful exit
}

void show_order_form()
{
	int pcx_error;
	uint8_t title_pal[768];
	char	exit_screen[16];
	
	grs_canvas* exit_canvas = gr_create_canvas(320, 200);
	gr_set_current_canvas(exit_canvas);
	gr_palette_clear();

	key_flush();

#ifdef SHAREWARE
	strcpy(exit_screen, "order01.pcx");
#else
#ifdef DEST_SAT
	strcpy(exit_screen, "order01.pcx");
#else
	strcpy(exit_screen, "warning.pcx");
#endif
#endif
	if ((pcx_error = pcx_read_bitmap(exit_screen, &exit_canvas->cv_bitmap, BM_LINEAR, title_pal)) == PCX_ERROR_NONE) 
	{
		gr_palette_fade_in(title_pal, 32, 0);
		{
			int done = 0;
			fix time_out_value = timer_get_approx_seconds() + i2f(60 * 5);
			while (!done) 
			{
				timer_mark_start();
				plat_do_events();
				if (timer_get_approx_seconds() > time_out_value) done = 1;
				if (key_inkey()) done = 1;
				plat_present_canvas(*exit_canvas, ASPECT_4_3);
				timer_mark_end(US_70FPS);
			}
		}
		gr_palette_fade_out(title_pal, 32, 0);
	}

	gr_free_canvas(exit_canvas);
	key_flush();
}

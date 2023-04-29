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

#include <stdlib.h>

#if defined(__linux__) || defined(_WIN32) || defined(_WIN64)
#include <malloc.h>
#endif

#include <stdio.h>
#include <string.h>

#include "misc/types.h"
#include "mem/mem.h"
#include "2d/gr.h"
#include "2d/grdef.h"
#include "misc/error.h"
#include "platform/mono.h"
#include "2d/palette.h"
#include "platform/platform.h"

char gr_pal_default[768];

int gr_installed = 0;

int gr_show_screen_info = 0;

uint8_t is_graphics_mode()
{
	return 1;
}

void gr_close()
{
	if (gr_installed == 1)
	{
		gr_installed = 0;
		free(grd_curscreen);
		//[ISB] Oops, gr_close is an atexit, but plat_close_window was expected to be called on the SDL code before. Keep call for Windows code atm.
		plat_close_window();
	}
}

#define NUMSCREENS 2 

int gr_set_mode(int mode)
{
	return 0;
}

int gr_init()
{
	int org_gamma;
	int retcode;

	// Only do this function once!
	if (gr_installed == 1)
		return 1;

	retcode = plat_create_window();
	if (retcode)
	{
		Error("gr_init: Error initalizing graphics library.");
	}

	MALLOC(grd_curscreen,grs_screen,1);
	memset(grd_curscreen, 0, sizeof(grs_screen));

	grd_curscreen->sc_canvas.cv_color = 0;
	grd_curscreen->sc_canvas.cv_drawmode = 0;
	grd_curscreen->sc_canvas.cv_font = NULL;
	grd_curscreen->sc_canvas.cv_font_fg_color = 0;
	grd_curscreen->sc_canvas.cv_font_bg_color = 0;
	gr_set_current_canvas(&grd_curscreen->sc_canvas);

	// Set flags indicating that this is installed.
	gr_installed = 1;
	atexit(gr_close);
	return 0;
}

//[ISB] come to think of it is there any point of having these stub fuctions 
//that do nothing but call I_ functions
void gr_sync_display()
{
	plat_wait_for_vbl();
}
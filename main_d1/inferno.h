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

#include <stdint.h>

 //	How close two points must be in all dimensions to be considered the same point.
#define	FIX_EPSILON	10

#include "main_shared/inferno_shared.h"

extern int Function_mode;			//in game or editor?
extern int Screen_mode;				//editor screen or game screen?
extern char Menu_pcx_name[13];

void show_order_form();

int D_DescentMain(int argc, const char** argv);

//Call to request a fade out. 
void inferno_request_fade_out();

//Call to request a fade in. This will fade into the specified palette. 
void inferno_request_fade_in(uint8_t* dest_palette);

//Returns true if the screen has been faded out
bool inferno_is_screen_faded();

//Returns true if the screen is currently transitioning
bool inferno_transitioning();

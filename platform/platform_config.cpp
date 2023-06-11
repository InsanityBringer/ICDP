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
//[ISB] This is adapted from the descent.cfg parser, so above notice is needed

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "platform/platform_filesys.h"
#include "platform/platform.h"
#include "platform/s_midi.h"
#include "platform/mouse.h"

static const char* WindowWidthStr = "WindowWidth";
static const char* WindowHeightStr = "WindowHeight";
static const char* FitModeStr = "FitMode";
static const char* FullscreenStr = "Fullscreen";
static const char* SoundFontPath = "SoundFontPath";
static const char* MouseScalarStr = "MouseScalar";
static const char* SwapIntervalStr = "SwapInterval";
static const char* NoOpenGLStr = "NoOpenGL";
static const char* GenDeviceStr = "PreferredGenMidiDevice";

bool NoOpenGL = false;

//[ISB] to be honest, I hate this configuration parser. I should try to create something more flexible at some point.
int plat_read_chocolate_cfg()
{
	return 0;
}

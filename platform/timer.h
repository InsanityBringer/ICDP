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

#include "misc/types.h"
#include "fix/fix.h"

//Not 100% precise, but should do the job
//Call timer_mark_end(US_60FPS) as desired
#define US_60FPS 16667
#define US_70FPS 14286

void timer_init();

fix timer_get_fixed_seconds(); // Rolls about every 9 hours...
fix timer_get_approx_seconds(); // Returns time since program started... accurate to 1/120th of a second

//[ISB] Replacement for the annoying ticker, increments 18 times/sec
uint32_t timer_get_ticks();

uint32_t timer_get_ms();
uint64_t timer_get_us();

//[ISB] replacement for delay?
void timer_delay(int ms);

void timer_delay_us(uint64_t us);

//Quick 'n dirty framerate limiting tools.
//Call I_MarkStart at the beginning of the loop
void timer_mark_start();
//Call I_MarkEnd with the desired delay time to make the thread relax for just long enough
void timer_mark_end(uint64_t numUS);

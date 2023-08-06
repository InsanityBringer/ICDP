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

#include "2d/rle.h"

constexpr int MAX_NUM_CACHE_BITMAPS = 50;

struct TEXTURE_CACHE
{
	grs_bitmap* bitmap;
	grs_bitmap* bottom_bmp;
	grs_bitmap* top_bmp;
	int 		orient;
	int			last_frame_used;
};

class Texmerge
{
	TEXTURE_CACHE Cache[MAX_NUM_CACHE_BITMAPS];

	int	merge_count = 0;
	int num_cache_entries = 0;

	int cache_hits = 0;
	int cache_misses = 0;
public:
	Texmerge();
	~Texmerge();

	grs_bitmap* get_cached_bitmap(RLECache& rle_cache, int tmap_bottom, int tmap_top);
	void flush();
};

grs_bitmap * texmerge_get_cached_bitmap( int tmap_bottom, int tmap_top );
void texmerge_flush();

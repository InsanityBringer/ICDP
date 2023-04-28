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
#include "2d/gr.h"

//Encapsulated version of the bitmap scalar. The global scope functions will invoke operations on gr_global_scalar.
class GrScalar
{
	int clip_left = 0, clip_top = 0, clip_right = 0, clip_bottom = 0;

	int scale_error_term = 0;
	int scale_initial_pixel_count = 0;
	int scale_adj_up = 0;
	int scale_adj_down = 0;
	int scale_final_pixel_count = 0;
	int scale_ydelta_minus_1 = 0;
	int scale_whole_step = 0;
	uint8_t* scale_source_ptr = nullptr;
	uint8_t* scale_dest_ptr = nullptr;

	uint8_t scale_rle_data[640];
	
	grs_canvas* curcanv = nullptr;

	void decode_row(grs_bitmap* bmp, int y);
	void scale_up_bitmap(grs_bitmap* source_bmp, grs_bitmap* dest_bmp, int x0, int y0, int x1, int y1, fix u0, fix v0, fix u1, fix v1);
	void scale_up_bitmap_rle(grs_bitmap* source_bmp, grs_bitmap* dest_bmp, int x0, int y0, int x1, int y1, fix u0, fix v0, fix u1, fix v1);
	void rls_stretch_scanline_setup(int XDelta, int YDelta);
	void rls_stretch_scanline();
	void scale_bitmap_c(grs_bitmap* source_bmp, grs_bitmap* dest_bmp, int x0, int y0, int x1, int y1, fix u0, fix v0, fix u1, fix v1);
	void scale_row_asm_transparent(uint8_t* sbits, uint8_t* dbits, int width, fix u, fix du);
	void scale_bitmap_c_rle(grs_bitmap* source_bmp, grs_bitmap* dest_bmp, int x0, int y0, int x1, int y1, fix u0, fix v0, fix u1, fix v1);
public:
	GrScalar();

	void set_canvas(grs_canvas* canvas)
	{
		curcanv = canvas;
		set_clip_bounds(0, 0, curcanv->cv_bitmap.bm_w - 1, curcanv->cv_bitmap.bm_h - 1);
	}
	void set_clip_bounds(int left, int top, int right, int bottom)
	{
		clip_left = left; clip_top = top; clip_right = right; clip_bottom = bottom;
	}

	void scale_bitmap(grs_bitmap* bp, grs_point* vertbuf, int orientation);
};

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

#include "mem/mem.h"
#include "2d/gr.h"
#include "2d/grdef.h"

int gr_disk(fix xc1, fix yc1, fix r1)
{
	int p, x, y, xc, yc, r;

	r = f2i(r1);
	xc = f2i(xc1);
	yc = f2i(yc1);
	p = 3 - (r * 2);
	x = 0;
	y = r;

	// Big clip
	if ((xc + r) < 0) return 1;
	if ((xc - r) > WIDTH) return 1;
	if ((yc + r) < 0) return 1;
	if ((yc - r) > HEIGHT) return 1;

	while (x < y)
	{
		// Draw the first octant
		gr_scanline(xc - y, xc + y, yc - x);
		gr_scanline(xc - y, xc + y, yc + x);

		if (p < 0)
			p = p + (x << 2) + 6;
		else {
			// Draw the second octant
			gr_scanline(xc - x, xc + x, yc - y);
			gr_scanline(xc - x, xc + x, yc + y);
			p = p + ((x - y) << 2) + 10;
			y--;
		}
		x++;
	}
	if (x == y) {
		gr_scanline(xc - x, xc + x, yc - y);
		gr_scanline(xc - x, xc + x, yc + y);
	}
	return 0;
}

int gr_disk_explicit_clip(fix xc1, fix yc1, fix r1, int color, int xmin, int ymin, int xmax, int ymax)
{
	int p, x, y, xc, yc, r;

	r = f2i(r1);
	xc = f2i(xc1);
	yc = f2i(yc1);
	p = 3 - (r * 2);
	x = 0;
	y = r;

	// Big clip
	if ((xc + r) < xmin) return 1;
	if ((xc - r) > xmax) return 1;
	if ((yc + r) < ymin) return 1;
	if ((yc - r) > ymax) return 1;

	while (x < y)
	{
		// Draw the first octant
		gr_scanline_explicit_clip(xc - y, xc + y, yc - x, color, xmin, ymin, xmax, ymax);
		gr_scanline_explicit_clip(xc - y, xc + y, yc + x, color, xmin, ymin, xmax, ymax);

		if (p < 0)
			p = p + (x << 2) + 6;
		else 
		{
			// Draw the second octant
			gr_scanline_explicit_clip(xc - x, xc + x, yc - y, color, xmin, ymin, xmax, ymax);
			gr_scanline_explicit_clip(xc - x, xc + x, yc + y, color, xmin, ymin, xmax, ymax);
			p = p + ((x - y) << 2) + 10;
			y--;
		}
		x++;
	}
	if (x == y) 
	{
		gr_scanline_explicit_clip(xc - x, xc + x, yc - y, color, xmin, ymin, xmax, ymax);
		gr_scanline_explicit_clip(xc - x, xc + x, yc + y, color, xmin, ymin, xmax, ymax);
	}
	return 0;
}

int gr_udisk(fix xc1, fix yc1, fix r1)
{
	int p, x, y, xc, yc, r;

	r = f2i(r1);
	xc = f2i(xc1);
	yc = f2i(yc1);
	p = 3 - (r * 2);
	x = 0;
	y = r;

	while (x < y)
	{
		// Draw the first octant
		gr_uscanline(xc - y, xc + y, yc - x);
		gr_uscanline(xc - y, xc + y, yc + x);

		if (p < 0)
			p = p + (x << 2) + 6;
		else {
			// Draw the second octant
			gr_uscanline(xc - x, xc + x, yc - y);
			gr_uscanline(xc - x, xc + x, yc + y);
			p = p + ((x - y) << 2) + 10;
			y--;
		}
		x++;
	}
	if (x == y) {
		gr_uscanline(xc - x, xc + x, yc - y);
		gr_uscanline(xc - x, xc + x, yc + y);
	}
	return 0;
}

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
#include <algorithm>
#include "mem/mem.h"
#include "2d/gr.h"
#include "2d/grdef.h"
#include "fix/fix.h"
#include "clip.h"

int modex_line_vertincr;
int modex_line_incr1;
int modex_line_incr2;
int modex_line_x1;
int modex_line_y1;
int modex_line_x2;
int modex_line_y2;
uint8_t modex_line_Color;

//[ISB] Since the line drawer isn't using any globals, this doesn't have an encapsulated class at the moment.
//I may still do this later for speed reasons, so there aren't near as many parameters being passed. 

/*
Symmetric Double Step Line Algorithm
by Brian Wyvill
from "Graphics Gems", Academic Press, 1990
*/

/* non-zero flag indicates the pixels needing EXCHG back. */
void plot(grs_canvas* canvas, int color, int x, int y, int flag)
{
	if (flag)
		canvas->cv_bitmap.bm_data[x * canvas->cv_bitmap.bm_rowsize + y] = color;
	else
		canvas->cv_bitmap.bm_data[y * canvas->cv_bitmap.bm_rowsize + x] = color;
}

int gr_hline(grs_canvas* canvas, int color, int x1, int x2, int y)
{
	int i;

	if (x1 > x2) EXCHG(x1, x2);
	for (i = x1; i <= x2; i++)
		canvas->cv_bitmap.bm_data[y * canvas->cv_bitmap.bm_rowsize + i] = color;
	return 0;
}

int gr_vline(grs_canvas* canvas, int color, int y1, int y2, int x)
{
	int i;
	if (y1 > y2) EXCHG(y1, y2);
	for (i = y1; i <= y2; i++)
		canvas->cv_bitmap.bm_data[i * canvas->cv_bitmap.bm_rowsize + x] = color;
	return 0;
}

void gr_universal_uline_debug(grs_canvas* canvas, int color, int a1, int b1, int a2, int b2)
{
	float x = a1; float y = b1;

	int deltaX = a2 - a1; int deltaY = b2 - b1;
	int count = std::max(abs(deltaX), abs(deltaY));
	float xStep = (float)deltaX / count; float yStep = (float)deltaY / count;

	for (int i = 0; i < count; i++)
	{
		plot(canvas, color, floor(x), floor(y), false);
		x += xStep; y += yStep;
	}
}

void gr_universal_uline(grs_canvas* canvas, int color, int a1, int b1, int a2, int b2)
{
	int dx, dy, incr1, incr2, D, x, y, xend, c, pixels_left;
	int x1, y1;
	int sign_x = 1, sign_y = 1, step, reverse, i;

	if (a1 == a2) {
		gr_vline(canvas, color, b1, b2, a1);
		return;
	}

	if (b1 == b2) {
		gr_hline(canvas, color, a1, a2, b1);
		return;
	}

	dx = a2 - a1;
	dy = b2 - b1;

	if (dx < 0) {
		sign_x = -1;
		dx *= -1;
	}
	if (dy < 0) {
		sign_y = -1;
		dy *= -1;
	}

	/* decide increment sign by the slope sign */
	if (sign_x == sign_y)
		step = 1;
	else
		step = -1;

	if (dy > dx) {          /* chooses axis of greatest movement (make * dx) */
		EXCHG(a1, b1);
		EXCHG(a2, b2);
		EXCHG(dx, dy);
		reverse = 1;
	}
	else
		reverse = 0;
	/* note error check for dx==0 should be included here */
	if (a1 > a2) {          /* start from the smaller coordinate */
		x = a2;
		y = b2;
		x1 = a1;
		y1 = b1;
	}
	else {
		x = a1;
		y = b1;
		x1 = a2;
		y1 = b2;
	}


	/* Note dx=n implies 0 - n or (dx+1) pixels to be set */
	/* Go round loop dx/4 times then plot last 0,1,2 or 3 pixels */
	/* In fact (dx-1)/4 as 2 pixels are already plottted */
	xend = (dx - 1) / 4;
	pixels_left = (dx - 1) % 4;     /* number of pixels left over at the
								 * end */
	plot(canvas, color, x, y, reverse);
	plot(canvas, color, x1, y1, reverse);  /* plot first two points */
	incr2 = 4 * dy - 2 * dx;
	if (incr2 < 0) {        /* slope less than 1/2 */
		c = 2 * dy;
		incr1 = 2 * c;
		D = incr1 - dx;

		for (i = 0; i < xend; i++) {    /* plotting loop */
			++x;
			--x1;
			if (D < 0) {
				/* pattern 1 forwards */
				plot(canvas, color, x, y, reverse);
				plot(canvas, color, ++x, y, reverse);
				/* pattern 1 backwards */
				plot(canvas, color, x1, y1, reverse);
				plot(canvas, color, --x1, y1, reverse);
				D += incr1;
			}
			else {
				if (D < c) {
					/* pattern 2 forwards */
					plot(canvas, color, x, y, reverse);
					plot(canvas, color, ++x, y += step, reverse);
					/* pattern 2 backwards */
					plot(canvas, color, x1, y1, reverse);
					plot(canvas, color, --x1, y1 -= step, reverse);
				}
				else {
					/* pattern 3 forwards */
					plot(canvas, color, x, y += step, reverse);
					plot(canvas, color, ++x, y, reverse);
					/* pattern 3 backwards */
					plot(canvas, color, x1, y1 -= step, reverse);
					plot(canvas, color, --x1, y1, reverse);
				}
				D += incr2;
			}
		}               /* end for */

		/* plot last pattern */
		if (pixels_left) {
			if (D < 0) {
				plot(canvas, color, ++x, y, reverse);  /* pattern 1 */
				if (pixels_left > 1)
					plot(canvas, color, ++x, y, reverse);
				if (pixels_left > 2)
					plot(canvas, color, --x1, y1, reverse);
			}
			else {
				if (D < c) {
					plot(canvas, color, ++x, y, reverse);  /* pattern 2  */
					if (pixels_left > 1)
						plot(canvas, color, ++x, y += step, reverse);
					if (pixels_left > 2)
						plot(canvas, color, --x1, y1, reverse);
				}
				else {
					/* pattern 3 */
					plot(canvas, color, ++x, y += step, reverse);
					if (pixels_left > 1)
						plot(canvas, color, ++x, y, reverse);
					if (pixels_left > 2)
						plot(canvas, color, --x1, y1 -= step, reverse);
				}
			}
		}               /* end if pixels_left */
	}
	/* end slope < 1/2 */
	else {                  /* slope greater than 1/2 */
		c = 2 * (dy - dx);
		incr1 = 2 * c;
		D = incr1 + dx;
		for (i = 0; i < xend; i++) {
			++x;
			--x1;
			if (D > 0) {
				/* pattern 4 forwards */
				plot(canvas, color, x, y += step, reverse);
				plot(canvas, color, ++x, y += step, reverse);
				/* pattern 4 backwards */
				plot(canvas, color, x1, y1 -= step, reverse);
				plot(canvas, color, --x1, y1 -= step, reverse);
				D += incr1;
			}
			else {
				if (D < c) {
					/* pattern 2 forwards */
					plot(canvas, color, x, y, reverse);
					plot(canvas, color, ++x, y += step, reverse);

					/* pattern 2 backwards */
					plot(canvas, color, x1, y1, reverse);
					plot(canvas, color, --x1, y1 -= step, reverse);
				}
				else {
					/* pattern 3 forwards */
					plot(canvas, color, x, y += step, reverse);
					plot(canvas, color, ++x, y, reverse);
					/* pattern 3 backwards */
					plot(canvas, color, x1, y1 -= step, reverse);
					plot(canvas, color, --x1, y1, reverse);
				}
				D += incr2;
			}
		}               /* end for */
		/* plot last pattern */
		if (pixels_left) {
			if (D > 0) {
				plot(canvas, color, ++x, y += step, reverse);  /* pattern 4 */
				if (pixels_left > 1)
					plot(canvas, color, ++x, y += step, reverse);
				if (pixels_left > 2)
					plot(canvas, color, --x1, y1 -= step, reverse);
			}
			else {
				if (D < c) {
					plot(canvas, color, ++x, y, reverse);  /* pattern 2  */
					if (pixels_left > 1)
						plot(canvas, color, ++x, y += step, reverse);
					if (pixels_left > 2)
						plot(canvas, color, --x1, y1, reverse);
				}
				else {
					/* pattern 3 */
					plot(canvas, color, ++x, y += step, reverse);
					if (pixels_left > 1)
						plot(canvas, color, ++x, y, reverse);
					if (pixels_left > 2) {
						if (D > c) /* step 3 */
							plot(canvas, color, --x1, y1 -= step, reverse);
						else /* step 2 */
							plot(canvas, color, --x1, y1, reverse);
					}
				}
			}
		}
	}
}


//unclipped version just calls clipping version for now
int gr_uline(fix _a1, fix _b1, fix _a2, fix _b2)
{
	return gr_uline(grd_curcanv, grd_curcanv->cv_color, _a1, _b1, _a2, _b2);
}

int gr_uline(grs_canvas* canvas, int color, fix _a1, fix _b1, fix _a2, fix _b2)
{
	int a1, b1, a2, b2;
	a1 = f2i(_a1); b1 = f2i(_b1); a2 = f2i(_a2); b2 = f2i(_b2);

	gr_universal_uline(canvas, color, a1, b1, a2, b2);
	return 0;
}

// Returns 0 if drawn with no clipping, 1 if drawn but clipped, and
// 2 if not drawn at all.

int gr_line(fix a1, fix b1, fix a2, fix b2)
{
	return gr_line(grd_curcanv, grd_curcanv->cv_color, a1, b1, a2, b2);
}

int gr_line(grs_canvas* canvas, int color, fix a1, fix b1, fix a2, fix b2)
{
	int x1, y1, x2, y2;
	int clipped = 0;

	x1 = i2f(MINX);
	y1 = i2f(MINY);
	x2 = i2f(MAXX);
	y2 = i2f(MAXY);

	CLIPLINE(a1, b1, a2, b2, x1, y1, x2, y2, return 2, clipped = 1, FSCALE);

	gr_uline(canvas, color, a1, b1, a2, b2);

	return clipped;
}

int gr_line_explicit_clip(grs_canvas* canvas, int color, fix a1, fix b1, fix a2, fix b2, int minx, int miny, int maxx, int maxy)
{
	int x1, y1, x2, y2;
	int clipped = 0;

	x1 = i2f(minx);
	y1 = i2f(miny);
	x2 = i2f(maxx);
	y2 = i2f(maxy);

	CLIPLINE(a1, b1, a2, b2, x1, y1, x2, y2, return 2, clipped = 1, FSCALE);

	gr_uline(canvas, color, a1, b1, a2, b2);

	return clipped;
}

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

#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "fix/fix.h"
#include "platform/mono.h"
#include "2d/gr.h"
#include "2d/grdef.h"
#include "3d/3d.h"
#include "texmapl.h"
#include "scanline.h"

// -------------------------------------------------------------------------------------
//	Texture map current scanline.
//	Uses globals Du_dx and Dv_dx to incrementally compute u,v coordinates
// -------------------------------------------------------------------------------------
void Texmap::ScanlineFlat(int y, float xleft, float xright)
{
	if (xright < xleft)
		return;

	// setup to call assembler scanline renderer
	fx_y = y;
	fx_xleft = xleft;
	fx_xright = xright;

	if (fx_xleft < 0)
		fx_xleft = 0; //[ISB] godawful hack
	if (fx_xright > Window_clip_right)
		fx_xright = Window_clip_right;

	if (Darkening_level >= GR_FADE_LEVELS)
		DrawScanlineFlat();
	else 
	{
		tmap_flat_shade_value = Darkening_level;
		DrawScanlineShaded();
	}
}

// -------------------------------------------------------------------------------------
//	Render a texture map.
// Linear in outer loop, linear in inner loop.
// -------------------------------------------------------------------------------------
void Texmap::TMapFlat(g3ds_tmap* t, int color)
{
	int	vlt, vrt, vlb, vrb;	// vertex left top, vertex right top, vertex left bottom, vertex right bottom
	int	topy, boty, y, dy;
	float	dx_dy_left, dx_dy_right;
	int	max_y_vertex;
	float	xleft, xright;
	float	recip_dy;
	g3ds_vertex* v3d;

	v3d = t->verts;

	tmap_flat_color = color;

	// Determine top and bottom y coords.
	compute_y_bounds(t, &vlt, &vlb, &vrt, &vrb, &max_y_vertex);

	// Set top and bottom (of entire texture map) y coordinates.
	topy = v3d[vlt].y2d;
	boty = v3d[max_y_vertex].y2d;

	if (topy > Window_clip_bot)
		return;
	if (boty > Window_clip_bot)
		boty = Window_clip_bot;

	// Set amount to change x coordinate for each advance to next scanline.
	dy = t->verts[vlb].y2d - t->verts[vlt].y2d;
	if (dy < FIX_RECIP_TABLE_SIZE)
		recip_dy = fix_recip[dy];
	else
		recip_dy = 1.0f / dy;

	dx_dy_left = compute_dx_dy(t, vlt, vlb, recip_dy);

	dy = t->verts[vrb].y2d - t->verts[vrt].y2d;
	if (dy < FIX_RECIP_TABLE_SIZE)
		recip_dy = fix_recip[dy];
	else
		recip_dy = 1.0f / dy;

	dx_dy_right = compute_dx_dy(t, vrt, vrb, recip_dy);

	// Set initial values for x, u, v
	xleft = v3d[vlt].x2d;
	xright = v3d[vrt].x2d;

	// scan all rows in texture map from top through first break.
	// @mk: Should we render the scanline for y==boty?  This violates Matt's spec.

	for (y = topy; y < boty; y++)
	{
		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x,u,v
		if (y == (int)v3d[vlb].y2d) 
		{
			// Handle problem of double points.  Search until y coord is different.  Cannot get
			// hung in an infinite loop because we know there is a vertex with a lower y coordinate
			// because in the for loop, we don't scan all spanlines.
			while (y == (int)v3d[vlb].y2d) 
			{
				vlt = vlb;
				vlb = prevmod(vlb, t->nv);
			}
			dy = t->verts[vlb].y2d - t->verts[vlt].y2d;
			if (dy < FIX_RECIP_TABLE_SIZE)
				recip_dy = fix_recip[dy];
			else
				recip_dy = 1.0f / dy;

			dx_dy_left = compute_dx_dy(t, vlt, vlb, recip_dy);

			xleft = v3d[vlt].x2d;
		}

		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x.  Not necessary to set new values for u,v.
		if (y == (int)v3d[vrb].y2d)
		{
			while (y == (int)v3d[vrb].y2d) 
			{
				vrt = vrb;
				vrb = succmod(vrb, t->nv);
			}

			dy = t->verts[vrb].y2d - t->verts[vrt].y2d;
			if (dy < FIX_RECIP_TABLE_SIZE)
				recip_dy = fix_recip[dy];
			else
				recip_dy = 1.0f / dy;

			dx_dy_right = compute_dx_dy(t, vrt, vrb, recip_dy);

			xright = v3d[vrt].x2d;
		}

		//tmap_scanline_flat(y, xleft, xright);
		if (xleft < 0)
			xleft = 0; //[ISB] godawful hack
		ScanlineFlat(y, xleft, xright);

		xleft += dx_dy_left;
		xright += dx_dy_right;

	}
	//tmap_scanline_flat(y, xleft, xright);
	ScanlineFlat(y, xleft, xright);
}


//	-----------------------------------------------------------------------------------------
//	This is the gr_upoly-like interface to the texture mapper which uses texture-mapper compatible
//	(ie, avoids cracking) edge/delta computation.
void Texmap::DrawFlat(int color, int nverts, int* vert)
{
	g3ds_tmap	my_tmap;
	int			i;

	//--now called from g3_start_frame-- init_interface_vars_to_assembler();

	my_tmap.nv = nverts;

	for (i = 0; i < nverts; i++)
	{
		my_tmap.verts[i].x2d = f2fl(*vert++);
		my_tmap.verts[i].y2d = f2fl(*vert++);
	}

	TMapFlat(&my_tmap, color);
}

#include "3d/3d.h"
#include "misc/error.h"

struct pnt2d 
{
	fix x, y;
};

//this takes the same partms as draw_tmap, but draws a flat-shaded polygon
void Texmap::DrawFlat(grs_bitmap* bp, int nverts, g3s_point** vertbuf)
{
	pnt2d	points[MAX_TMAP_VERTS];
	int	i;
	float	average_light;
	int	color;

	Assert(nverts < MAX_TMAP_VERTS);

	average_light = f2fl(vertbuf[0]->p3_l);
	for (i = 1; i < nverts; i++)
		average_light += f2fl(vertbuf[i]->p3_l);

	if (nverts == 4)
		average_light = average_light * NUM_LIGHTING_LEVELS / 4;
	else
		average_light = average_light * NUM_LIGHTING_LEVELS / nverts;

	if (average_light < 0)
		average_light = 0;
	else if (average_light > NUM_LIGHTING_LEVELS - 1)
		average_light = NUM_LIGHTING_LEVELS - 1;

	color = gr_fade_table[(int)average_light * 256 + bp->avg_color];

	for (i = 0; i < nverts; i++) 
	{
		points[i].x = vertbuf[i]->p3_sx;
		points[i].y = vertbuf[i]->p3_sy;
	}

	DrawFlat(color, nverts, (int*)points);
}


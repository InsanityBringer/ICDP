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
#include <string.h>

#include "platform/mono.h"
#include "fix/fix.h"
#include "3d/3d.h"
#include "2d/gr.h"
#include "misc/error.h"

#include "texmap/texmap.h"
#include "texmapl.h"
#include "2d/rle.h"
#include "scanline.h"

#define	EDITOR_TMAP	1		//if in, include extra stuff

int	y_pointers[MAX_Y_POINTERS];

float fix_recip[FIX_RECIP_TABLE_SIZE];
int	Fix_recip_table_computed = 0;

//[ISB] global for now
int	Max_perspective_depth;
int	Max_linear_depth;
int	Max_flat_depth;

Texmap::Texmap()
{
	memset(&Tmap1, 0, sizeof(Tmap1));
	memset(y_pointers, 0, sizeof(y_pointers));
	write_buffer = nullptr;
	window_left = window_right = window_top = window_bottom = 0;
	window_width = window_height = 0;
	Darkening_level = GR_FADE_LEVELS;
}

// -------------------------------------------------------------------------------------
void init_fix_recip_table(void)
{
	int	i;

	fix_recip[0] = 1.0f;

	for (i = 1; i < FIX_RECIP_TABLE_SIZE; i++)
		fix_recip[i] = 1.0f / i;

	Fix_recip_table_computed = 1;
}

// -------------------------------------------------------------------------------------
//	Initialize interface variables to assembler.
//	These things used to be constants.  This routine is now (10/6/93) getting called for
//	every texture map.  It should get called whenever the window changes, or, preferably,
//	not at all.  I'm pretty sure these variables are only being used for range checking.
void init_interface_vars_to_assembler(void)
{
	if (!Fix_recip_table_computed)
		init_fix_recip_table();
}

void Texmap::SetCanvas(grs_canvas* canv)
{
	grs_bitmap* bp = &(canv->cv_bitmap);

	Assert(bp != NULL);
	Assert(bp->bm_data != NULL);
	Assert(bp->bm_h <= MAX_Y_POINTERS);

	//	If bytes_per_row has changed, create new table of pointers.
	if (bytes_per_row != (int)bp->bm_rowsize)
	{
		int	y_val, i;

		bytes_per_row = (int)bp->bm_rowsize;

		y_val = 0;
		for (i = 0; i < MAX_Y_POINTERS; i++)
		{
			y_pointers[i] = y_val;
			y_val += bytes_per_row;
		}
	}

	write_buffer = bp->bm_data;

	window_left = 0;
	window_right = (int)bp->bm_w - 1;
	window_top = 0;
	window_bottom = (int)bp->bm_h - 1;

	Window_clip_left = window_left;
	Window_clip_right = window_right;
	Window_clip_top = window_top;
	Window_clip_bot = window_bottom;

	window_width = bp->bm_w;
	window_height = bp->bm_h;
}

// -------------------------------------------------------------------------------------
//	Returns number preceding val modulo modulus.
//	prevmod(3,4) = 2
//	prevmod(0,4) = 3
int prevmod(int val, int modulus)
{
	if (val > 0)
		return val - 1;
	else
		return modulus - 1;
	//	return (val + modulus - 1) % modulus;
}


//	Returns number succeeding val modulo modulus.
//	succmod(3,4) = 0
//	succmod(0,4) = 1
int succmod(int val, int modulus)
{
	if (val < modulus - 1)
		return val + 1;
	else
		return 0;

	//	return (val + 1) % modulus;
}

// -------------------------------------------------------------------------------------
//	Select topmost vertex (minimum y coordinate) and bottommost (maximum y coordinate) in
//	texture map.  If either is part of a horizontal edge, then select leftmost vertex for
//	top, rightmost vertex for bottom.
//	Important: Vertex is selected with integer precision.  So, if there are vertices at
//	(0.0,0.7) and (0.5,0.3), the first vertex is selected, because they y coordinates are
//	considered the same, so the smaller x is favored.
//	Parameters:
//		nv		number of vertices
//		v3d	pointer to 3d vertices containing u,v,x2d,y2d coordinates
//	Results in:
//		*min_y_ind
//		*max_y_ind
// -------------------------------------------------------------------------------------
void compute_y_bounds(g3ds_tmap* t, int* vlt, int* vlb, int* vrt, int* vrb, int* bottom_y_ind)
{
	// Scan all vertices, set min_y_ind to vertex with smallest y coordinate.
	int min_y = t->verts[0].y2d;
	int max_y = min_y;
	int min_y_ind = 0;
	int min_x = t->verts[0].x2d;
	*bottom_y_ind = 0;

	for (int i = 1; i < t->nv; i++) 
	{
		if (t->verts[i].y2d < min_y)
		{
			min_y = t->verts[i].y2d;
			min_y_ind = i;
			min_x = t->verts[i].x2d;
		}
		else if (t->verts[i].y2d == min_y) 
		{
			if (t->verts[i].x2d < min_x) 
			{
				min_y_ind = i;
				min_x = t->verts[i].x2d;
			}
		}
		if (t->verts[i].y2d > max_y) 
		{
			max_y = t->verts[i].y2d;
			*bottom_y_ind = i;
		}
	}

	// Set "vertex left top", etc. based on vertex with topmost y coordinate
	*vlt = min_y_ind;
	*vrt = *vlt;
	*vlb = prevmod(*vlt, t->nv);
	*vrb = succmod(*vrt, t->nv);

	// If right edge is horizontal, then advance along polygon bound until it no longer is or until all
	// vertices have been examined.
	// (Left edge cannot be horizontal, because *vlt is set to leftmost point with highest y coordinate.)
	int original_vrt = *vrt;

	while ((int)t->verts[*vrt].y2d == (int)t->verts[*vrb].y2d) 
	{
		if (succmod(*vrt, t->nv) == original_vrt) 
		{
			break;
		}
		*vrt = succmod(*vrt, t->nv);
		*vrb = succmod(*vrt, t->nv);
	}
}

float compute_du_dy_lin(g3ds_tmap* t, int top_vertex, int bottom_vertex, float recip_dy)
{
	//return fixmul(t->verts[bottom_vertex].u - t->verts[top_vertex].u, recip_dy);
	return (t->verts[bottom_vertex].u - t->verts[top_vertex].u) * recip_dy;
}

float compute_dv_dy_lin(g3ds_tmap* t, int top_vertex, int bottom_vertex, float recip_dy)
{
	//return fixmul(t->verts[bottom_vertex].v - t->verts[top_vertex].v, recip_dy);
	return (t->verts[bottom_vertex].v - t->verts[top_vertex].v) * recip_dy;
}

float compute_dl_dy_lin(g3ds_tmap* t, int top_vertex, int bottom_vertex, float recip_dy)
{
	//return fixmul(t->verts[bottom_vertex].l - t->verts[top_vertex].l, recip_dy);
	return (t->verts[bottom_vertex].l - t->verts[top_vertex].l) * recip_dy;
}

float compute_dx_dy(g3ds_tmap* t, int top_vertex, int bottom_vertex, float recip_dy)
{
	return (t->verts[bottom_vertex].x2d - t->verts[top_vertex].x2d) * recip_dy;
}

float compute_du_dy(g3ds_tmap* t, int top_vertex, int bottom_vertex, float recip_dy)
{
	return (t->verts[bottom_vertex].u * t->verts[bottom_vertex].z - t->verts[top_vertex].u * t->verts[top_vertex].z) * recip_dy;
}

float compute_dv_dy(g3ds_tmap* t, int top_vertex, int bottom_vertex, float recip_dy)
{
	return (t->verts[bottom_vertex].v * t->verts[bottom_vertex].z - t->verts[top_vertex].v * t->verts[top_vertex].z) * recip_dy;
}

float compute_dz_dy(g3ds_tmap* t, int top_vertex, int bottom_vertex, float recip_dy)
{
	return (t->verts[bottom_vertex].z - t->verts[top_vertex].z) * recip_dy;
}

// -------------------------------------------------------------------------------------
//	Texture map current scanline in perspective.
// -------------------------------------------------------------------------------------
void Texmap::ScanlinePerspective(grs_bitmap* srcb, int y, float xleft, float xright, float uleft, float uright, float vleft, float vright, float zleft, float zright, float lleft, float lright)
{
	float	u, v, l;
	float	recip_dx;

	float	du_dx, dv_dx, dz_dx, z;

	u = uleft;
	v = vleft;
	l = lleft;

	fx_xright = xright;
	fx_xleft = xleft;

	int dx = fx_xright - fx_xleft;
	if ((dx < 0) || (xright < 0) || (xleft > xright))		// the (xleft > xright) term is not redundant with (dx < 0) because dx is computed using integers
		return;

	if (fx_xright > Window_clip_right)
		fx_xright = Window_clip_right;

	// setup to call assembler scanline renderer
	if (dx < FIX_RECIP_TABLE_SIZE)
		recip_dx = fix_recip[dx];
	else
		recip_dx = 1.f / dx;

	du_dx = (uright - uleft) * recip_dx;
	dv_dx = (vright - vleft) * recip_dx;
	dz_dx = (zright - zleft) * recip_dx;

	z = zleft;

	fx_u = uleft;
	fx_v = vleft;
	fx_z = zleft;
	fx_du_dx = du_dx;
	fx_dv_dx = dv_dx;
	fx_dz_dx = dz_dx;
	fx_y = y;
	pixptr = srcb->bm_data;

	switch (Lighting_enabled) 
	{
	case 0:
		if (fx_xleft > Window_clip_right)
			return;
		if (fx_xright < Window_clip_left)
			return;
		if (fx_xright > Window_clip_right)
			fx_xright = Window_clip_right;

		DrawScanlinePerspectivePer16NoLight();
		break;
	case 1: 
	{
		if (lleft < 0) lleft = 0;
		if (lright < 0) lright = 0;
		//(NUM_LIGHTING_LEVELS * F1_0 - F1_0 / 2)
		if (lleft > (NUM_LIGHTING_LEVELS - .5)) lleft = (NUM_LIGHTING_LEVELS - .5);
		if (lright > (NUM_LIGHTING_LEVELS - .5)) lright = (NUM_LIGHTING_LEVELS - .5);

		fx_l = lleft;
		fx_dl_dx = (lright - lleft) * recip_dx;

		//	This is a pretty ugly hack to prevent lighting overflows.
		float mul_thing = dx * fx_dl_dx;
		if (lleft + mul_thing < 0)
			fx_dl_dx += f2fl(12);
		else if (lleft + mul_thing > (NUM_LIGHTING_LEVELS - .5))
			fx_dl_dx -= f2fl(12);

		if (fx_xleft > Window_clip_right)
			return;
		if (fx_xright < Window_clip_left)
			return;
		if (fx_xright > Window_clip_right)
			fx_xright = Window_clip_right;

		DrawScanlinePerspective();
		break;
	}
	case 2:
#ifdef EDITOR_TMAP
		fx_xright = xright;
		fx_xleft = xleft;

		DrawScanlineEditor();
#else
		Int3();	//	Illegal, called an editor only routine!
#endif
		break;
	}

}

int Do_vertical_scan = 0;

int	Break_on_flat = 0;

// -------------------------------------------------------------------------------------
//	Render a texture map with lighting using perspective interpolation in inner and outer loops.
// -------------------------------------------------------------------------------------
void Texmap::TMapPerspective(grs_bitmap* srcb, g3ds_tmap* t)
{
	int	vlt, vrt, vlb, vrb;	// vertex left top, vertex right top, vertex left bottom, vertex right bottom
	int	topy, boty, y;
	float	dx_dy_left, dx_dy_right;
	float	du_dy_left, du_dy_right;
	float	dv_dy_left, dv_dy_right;
	float	dz_dy_left, dz_dy_right;
	float	dl_dy_left, dl_dy_right;
	float	recip_dyl, recip_dyr;
	int	max_y_vertex;
	float	xleft, xright, uleft, vleft, uright, vright, zleft, zright, lleft, lright;
	int	next_break_left, next_break_right;

	g3ds_vertex* v3d;

	v3d = t->verts;

	//[ISB] more uninitalized memory warnings
	lright = lleft = 0;

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
	int dy = (int)t->verts[vlb].y2d - (int)t->verts[vlt].y2d;
	if (dy < FIX_RECIP_TABLE_SIZE)
		recip_dyl = fix_recip[dy];
	else
		recip_dyl = 1.0f / dy;

	dx_dy_left = compute_dx_dy(t, vlt, vlb, recip_dyl);
	du_dy_left = compute_du_dy(t, vlt, vlb, recip_dyl);
	dv_dy_left = compute_dv_dy(t, vlt, vlb, recip_dyl);
	dz_dy_left = compute_dz_dy(t, vlt, vlb, recip_dyl);

	dy = (int)t->verts[vrb].y2d - (int)t->verts[vrt].y2d;
	if (dy < FIX_RECIP_TABLE_SIZE)
		recip_dyr = fix_recip[dy];
	else
		recip_dyr = 1.0f / dy;

	du_dy_right = compute_du_dy(t, vrt, vrb, recip_dyr);
	dx_dy_right = compute_dx_dy(t, vrt, vrb, recip_dyr);
	dv_dy_right = compute_dv_dy(t, vrt, vrb, recip_dyr);
	dz_dy_right = compute_dz_dy(t, vrt, vrb, recip_dyr);

	if (Lighting_enabled) 
	{
		dl_dy_left = compute_dl_dy_lin(t, vlt, vlb, recip_dyl);
		dl_dy_right = compute_dl_dy_lin(t, vrt, vrb, recip_dyr);

		lleft = v3d[vlt].l;
		lright = v3d[vrt].l;
	}

	// Set initial values for x, u, v
	xleft = v3d[vlt].x2d;
	xright = v3d[vrt].x2d;

	zleft = v3d[vlt].z;
	zright = v3d[vrt].z;

	uleft = v3d[vlt].u * zleft;
	uright = v3d[vrt].u * zright;
	vleft = v3d[vlt].v * zleft;
	vright = v3d[vrt].v * zright;

	// scan all rows in texture map from top through first break.
	next_break_left = v3d[vlb].y2d;
	next_break_right = v3d[vrb].y2d;

	for (y = topy; y < boty; y++) 
	{
		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x,u,v
		if (y == next_break_left)
		{
			// Handle problem of double points.  Search until y coord is different.  Cannot get
			// hung in an infinite loop because we know there is a vertex with a lower y coordinate
			// because in the for loop, we don't scan all spanlines.
			while (y == (int)v3d[vlb].y2d)
			{
				vlt = vlb;
				vlb = prevmod(vlb, t->nv);
			}
			next_break_left = (int)v3d[vlb].y2d;

			dy = (int)t->verts[vlb].y2d - (int)t->verts[vlt].y2d;
			float recip_dy;
			if (dy < FIX_RECIP_TABLE_SIZE)
				recip_dy = fix_recip[dy];
			else
				recip_dy = 1.0f / dy;

			dx_dy_left = compute_dx_dy(t, vlt, vlb, recip_dy);

			xleft = v3d[vlt].x2d;
			zleft = v3d[vlt].z;
			uleft = v3d[vlt].u * zleft;
			vleft = v3d[vlt].v * zleft;

			du_dy_left = compute_du_dy(t, vlt, vlb, recip_dy);
			dv_dy_left = compute_dv_dy(t, vlt, vlb, recip_dy);
			dz_dy_left = compute_dz_dy(t, vlt, vlb, recip_dy);

			if (Lighting_enabled)
			{
				dl_dy_left = compute_dl_dy_lin(t, vlt, vlb, recip_dy);
				lleft = v3d[vlt].l;
			}
		}

		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x.  Not necessary to set new values for u,v.
		if (y == next_break_right) 
		{
			while (y == (int)v3d[vrb].y2d) 
			{
				vrt = vrb;
				vrb = succmod(vrb, t->nv);
			}

			next_break_right = (int)v3d[vrb].y2d;

			dy = (int)t->verts[vrb].y2d - (int)t->verts[vrt].y2d;
			float recip_dy;
			if (dy < FIX_RECIP_TABLE_SIZE)
				recip_dy = fix_recip[dy];
			else
				recip_dy = 1.0f / dy;

			dx_dy_right = compute_dx_dy(t, vrt, vrb, recip_dy);
			xright = v3d[vrt].x2d;
			zright = v3d[vrt].z;
			uright = v3d[vrt].u * zright;
			vright = v3d[vrt].v * zright;
			du_dy_right = compute_du_dy(t, vrt, vrb, recip_dy);
			dv_dy_right = compute_dv_dy(t, vrt, vrb, recip_dy);
			dz_dy_right = compute_dz_dy(t, vrt, vrb, recip_dy);

			if (Lighting_enabled)
			{
				dl_dy_right = compute_dl_dy_lin(t, vrt, vrb, recip_dy);
				lright = v3d[vrt].l;
			}
		}

		if (Lighting_enabled) 
		{
			if (y >= Window_clip_top)
				ScanlinePerspective(srcb, y, xleft, xright, uleft, uright, vleft, vright, zleft, zright, lleft, lright);
			lleft += dl_dy_left;
			lright += dl_dy_right;
		}
		else
			if (y >= Window_clip_top)
				ScanlinePerspective(srcb, y, xleft, xright, uleft, uright, vleft, vright, zleft, zright, lleft, lright);

		uleft += du_dy_left;
		vleft += dv_dy_left;

		uright += du_dy_right;
		vright += dv_dy_right;

		xleft += dx_dy_left;
		xright += dx_dy_right;

		zleft += dz_dy_left;
		zright += dz_dy_right;

	}

	// We can get lleft or lright out of bounds here because we compute dl_dy using fixed point values,
	//	but we plot an integer number of scanlines, therefore doing an integer number of additions of the delta.
	ScanlinePerspective(srcb, y, xleft, xright, uleft, uright, vleft, vright, zleft, zright, lleft, lright);
}


// -------------------------------------------------------------------------------------
//	Texture map current scanline using linear interpolation.
// -------------------------------------------------------------------------------------
void Texmap::ScanlineLinear(grs_bitmap* srcb, int y, float xleft, float xright, float uleft, float uright, float vleft, float vright, float lleft, float lright)
{
	float	u, v, l;
	float	dx, recip_dx;

	float	du_dx, dv_dx, dl_dx;

	u = uleft;
	v = vleft;
	l = lleft;

	dx = xright - xleft;
	if ((dx < 0) || (xright < 0) || (xleft > xright))		// the (xleft > xright) term is not redundant with (dx < 0) because dx is computed using integers
		return;

	// setup to call assembler scanline renderer
	recip_dx = 1.0f / dx;

	du_dx = (uright - uleft) * recip_dx;
	dv_dx = (vright - vleft) * recip_dx;

	fx_u = uleft;
	fx_v = vleft;
	fx_du_dx = du_dx;
	fx_dv_dx = dv_dx;
	fx_y = y;
	fx_xright = xright;
	fx_xleft = xleft;

	if (fx_xright > Window_clip_right)
		fx_xright = Window_clip_right;

	pixptr = srcb->bm_data;

	switch (Lighting_enabled) 
	{
	case 0:
		if (fx_xleft > Window_clip_right)
			return;
		if (fx_xright < Window_clip_left)
			return;
		if (fx_xright > Window_clip_right)
			fx_xright = Window_clip_right;

		DrawScanlineLinearNoLight();
		break;
	case 1:
		if (lleft < 1.0f / 2)
			lleft = 1.0f / 2;
		if (lright < 1.0f / 2)
			lright = 1.0f / 2;

		/*if (lleft > MAX_LIGHTING_VALUE * NUM_LIGHTING_LEVELS)
			lleft = MAX_LIGHTING_VALUE * NUM_LIGHTING_LEVELS;
		if (lright > MAX_LIGHTING_VALUE * NUM_LIGHTING_LEVELS)
			lright = MAX_LIGHTING_VALUE * NUM_LIGHTING_LEVELS;*/                   
		if (lleft > (NUM_LIGHTING_LEVELS - .5)) lleft = (NUM_LIGHTING_LEVELS - .5);
		if (lright > (NUM_LIGHTING_LEVELS - .5)) lright = (NUM_LIGHTING_LEVELS - .5);

		fx_l = lleft;
		dl_dx = (lright - lleft) * recip_dx;
		fx_dl_dx = dl_dx;

		if (fx_xleft > Window_clip_right)
			return;
		if (fx_xright < Window_clip_left)
			return;
		if (fx_xright > Window_clip_right)
			fx_xright = Window_clip_right;

		DrawScanlineLinear();
		break;
	case 2:
#ifdef EDITOR_TMAP
		fx_xright = xright;
		fx_xleft = xleft;
		//asm_tmap_scanline_matt();
#else
		Int3();	//	Illegal, called an editor only routine!
#endif
		break;
	}
}

// -------------------------------------------------------------------------------------
//	Render a texture map with lighting using linear interpolation in inner and outer loops.
// -------------------------------------------------------------------------------------
void Texmap::TMapLinear(grs_bitmap* srcb, g3ds_tmap* t)
{
	int	vlt, vrt, vlb, vrb;	// vertex left top, vertex right top, vertex left bottom, vertex right bottom
	int	topy, boty, y;
	int	dy;
	float	dx_dy_left, dx_dy_right;
	float	du_dy_left, du_dy_right;
	float	dv_dy_left, dv_dy_right;
	float	dl_dy_left, dl_dy_right;
	int	max_y_vertex;
	float	xleft, xright, uleft, vleft, uright, vright, lleft, lright;
	int	next_break_left, next_break_right;
	float	recip_dyl, recip_dyr;

	//[ISB]possibly uninitalized memory?
	lleft = lright = 0;

	g3ds_vertex* v3d;

	v3d = t->verts;

	// Determine top and bottom y coords.
	compute_y_bounds(t, &vlt, &vlb, &vrt, &vrb, &max_y_vertex);

	// Set top and bottom (of entire texture map) y coordinates.
	topy = v3d[vlt].y2d;
	boty = v3d[max_y_vertex].y2d;

	if (topy > Window_clip_bot)
		return;
	if (boty > Window_clip_bot)
		boty = Window_clip_bot;

	dy = (int)t->verts[vlb].y2d - (int)t->verts[vlt].y2d;
	if (dy < FIX_RECIP_TABLE_SIZE)
		recip_dyl = fix_recip[dy];
	else
		recip_dyl = 1.0f / dy;

	dy = (int)t->verts[vrb].y2d - (int)t->verts[vrt].y2d;
	if (dy < FIX_RECIP_TABLE_SIZE)
		recip_dyr = fix_recip[dy];
	else
		recip_dyr = 1.0f / dy;

	// Set amount to change x coordinate for each advance to next scanline.
	dx_dy_left = compute_dx_dy(t, vlt, vlb, recip_dyl);
	dx_dy_right = compute_dx_dy(t, vrt, vrb, recip_dyr);

	du_dy_left = compute_du_dy_lin(t, vlt, vlb, recip_dyl);
	du_dy_right = compute_du_dy_lin(t, vrt, vrb, recip_dyr);

	dv_dy_left = compute_dv_dy_lin(t, vlt, vlb, recip_dyl);
	dv_dy_right = compute_dv_dy_lin(t, vrt, vrb, recip_dyr);

	if (Lighting_enabled) 
	{
		dl_dy_left = compute_dl_dy_lin(t, vlt, vlb, recip_dyl);
		dl_dy_right = compute_dl_dy_lin(t, vrt, vrb, recip_dyr);

		lleft = v3d[vlt].l;
		lright = v3d[vrt].l;
	}

	// Set initial values for x, u, v
	xleft = v3d[vlt].x2d;
	xright = v3d[vrt].x2d;

	uleft = v3d[vlt].u;
	uright = v3d[vrt].u;
	vleft = v3d[vlt].v;
	vright = v3d[vrt].v;

	// scan all rows in texture map from top through first break.
	next_break_left = v3d[vlb].y2d;
	next_break_right = v3d[vrb].y2d;

	for (y = topy; y < boty; y++) 
	{
		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x,u,v
		if (y == next_break_left) 
		{
			float	recip_dy;

			// Handle problem of double points.  Search until y coord is different.  Cannot get
			// hung in an infinite loop because we know there is a vertex with a lower y coordinate
			// because in the for loop, we don't scan all spanlines.
			while (y == (int)v3d[vlb].y2d) 
			{
				vlt = vlb;
				vlb = prevmod(vlb, t->nv);
			}
			next_break_left = v3d[vlb].y2d;

			dy = (int)t->verts[vlb].y2d - (int)t->verts[vlt].y2d;
			if (dy < FIX_RECIP_TABLE_SIZE)
				recip_dy = fix_recip[dy];
			else
				recip_dy = 1.0f / dy;

			dx_dy_left = compute_dx_dy(t, vlt, vlb, recip_dy);

			xleft = v3d[vlt].x2d;
			uleft = v3d[vlt].u;
			vleft = v3d[vlt].v;
			lleft = v3d[vlt].l;

			du_dy_left = compute_du_dy_lin(t, vlt, vlb, recip_dy);
			dv_dy_left = compute_dv_dy_lin(t, vlt, vlb, recip_dy);

			if (Lighting_enabled) 
			{
				dl_dy_left = compute_dl_dy_lin(t, vlt, vlb, recip_dy);
				lleft = v3d[vlt].l;
			}
		}

		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x.  Not necessary to set new values for u,v.
		if (y == next_break_right) 
		{
			float	recip_dy;

			while (y == (int)v3d[vrb].y2d) 
			{
				vrt = vrb;
				vrb = succmod(vrb, t->nv);
			}

			dy = (int)t->verts[vrb].y2d - (int)t->verts[vrt].y2d;
			if (dy < FIX_RECIP_TABLE_SIZE)
				recip_dy = fix_recip[dy];
			else
				recip_dy = 1.0f / dy;

			next_break_right = v3d[vrb].y2d;
			dx_dy_right = compute_dx_dy(t, vrt, vrb, recip_dy);

			xright = v3d[vrt].x2d;
			uright = v3d[vrt].u;
			vright = v3d[vrt].v;

			du_dy_right = compute_du_dy_lin(t, vrt, vrb, recip_dy);
			dv_dy_right = compute_dv_dy_lin(t, vrt, vrb, recip_dy);

			if (Lighting_enabled) 
			{
				dl_dy_right = compute_dl_dy_lin(t, vrt, vrb, recip_dy);
				lright = v3d[vrt].l;
			}
		}

		if (Lighting_enabled) 
		{
			ScanlineLinear(srcb, y, xleft, xright, uleft, uright, vleft, vright, lleft, lright);
			lleft += dl_dy_left;
			lright += dl_dy_right;
		}
		else
			ScanlineLinear(srcb, y, xleft, xright, uleft, uright, vleft, vright, lleft, lright);

		uleft += du_dy_left;
		vleft += dv_dy_left;

		uright += du_dy_right;
		vright += dv_dy_right;

		xleft += dx_dy_left;
		xright += dx_dy_right;
	}

	// We can get lleft or lright out of bounds here because we compute dl_dy using fixed point values,
	//	but we plot an integer number of scanlines, therefore doing an integer number of additions of the delta.
	ScanlineLinear(srcb, y, xleft, xright, uleft, uright, vleft, vright, lleft, lright);
}

// -------------------------------------------------------------------------------------
// Interface from Matt's data structures to Mike's texture mapper.
// -------------------------------------------------------------------------------------
void Texmap::DrawTMap(grs_bitmap* bp, int nverts, g3s_point** vertbuf)
{
	int	i;

	int	lighting_on_save = Lighting_on;

	Assert(nverts <= MAX_TMAP_VERTS);
#ifdef USE_MULT_CODE
	if (!divide_table_filled) fill_divide_table();
#endif

	//	If no transparency and seg depth is large, render as flat shaded.
	//Also draw as flat if the darkening level is set. 
	if (((Current_seg_depth > Max_linear_depth) && ((bp->bm_flags & 3) == 0)) || Darkening_level < GR_FADE_LEVELS) 
	{
		DrawFlat(bp, nverts, vertbuf);
		return;
	}

	if (bp->bm_flags & BM_FLAG_RLE)
		bp = local_cache.expand_texture(bp);
		//bp = rle_expand_texture(bp);		// Expand if rle'd

	Transparency_on = bp->bm_flags & BM_FLAG_TRANSPARENT;
	if (bp->bm_flags & BM_FLAG_NO_LIGHTING)
		Lighting_on = 0;

	// Setup texture map in Tmap1
	Tmap1.nv = nverts;						// Initialize number of vertices

	for (i = 0; i < nverts; i++) 
	{
		g3ds_vertex* tvp = &Tmap1.verts[i];
		g3s_point* vp = vertbuf[i];

		tvp->x2d = f2fl(vp->p3_sx);
		tvp->y2d = f2fl(vp->p3_sy);

		//	Check for overflow on fixdiv.  Will overflow on vp->z <= something small.  Allow only as low as 256.
		/*if (vp->p3_z < 256) //[ISB] TODO do I still need this?
		{
			vp->p3_z = 256;
		}*/

		tvp->z = 12.0f / f2fl(vp->p3_z);
		tvp->u = f2fl(vp->p3_u << 6); //* bp->bm_w;
		tvp->v = f2fl(vp->p3_v << 6); //* bp->bm_h;

		Assert(Lighting_on < 3);

		if (Lighting_on)
			tvp->l = f2fl(vp->p3_l) * NUM_LIGHTING_LEVELS;
	}

	Lighting_enabled = Lighting_on;

	// Now, call my texture mapper.
	if (Lighting_on) 
	{
		switch (Interpolation_method) // 0 = choose, 1 = linear, 2 = /8 perspective, 3 = full perspective
		{
		case 0:								// choose best interpolation
			if (Current_seg_depth > Max_perspective_depth)
				TMapLinear(bp, &Tmap1);
			else
				TMapPerspective(bp, &Tmap1);
			break;
		case 1:								// linear interpolation
			TMapLinear(bp, &Tmap1);
			break;
		case 2:								// perspective every 8th pixel interpolation
			TMapPerspective(bp, &Tmap1);
			break;
		case 3:								// perspective every pixel interpolation
			TMapPerspective(bp, &Tmap1);
			break;
		default:
			Assert(0);				// Illegal value for Interpolation_method, must be 0,1,2,3
		}
	}
	else 
	{
		switch (Interpolation_method) // 0 = choose, 1 = linear, 2 = /8 perspective, 3 = full perspective
		{	
		case 0:								// choose best interpolation
			if (Current_seg_depth > Max_perspective_depth)
				TMapLinear(bp, &Tmap1);
			else
				TMapPerspective(bp, &Tmap1);
			break;
		case 1:								// linear interpolation
			TMapLinear(bp, &Tmap1);
			break;
		case 2:								// perspective every 8th pixel interpolation
			TMapPerspective(bp, &Tmap1);
			break;
		case 3:								// perspective every pixel interpolation
			TMapPerspective(bp, &Tmap1);
			break;
		default:
			Assert(0);				// Illegal value for Interpolation_method, must be 0,1,2,3
		}
	}

	Lighting_on = lighting_on_save;
}


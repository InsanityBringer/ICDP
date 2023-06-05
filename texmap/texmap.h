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

#include "fix/fix.h"
#include "2d/gr.h"
#include "2d/rle.h"

//[ISB] circular dependency nonsense
#ifndef THREED_H
#error "texmap.h must be included through 3d.h"
#endif

#define	NUM_LIGHTING_LEVELS 32
#define MAX_TMAP_VERTS 25
#define MAX_LIGHTING_VALUE	((NUM_LIGHTING_LEVELS-1)*F1_0/NUM_LIGHTING_LEVELS)
#define MIN_LIGHTING_VALUE	(F1_0/NUM_LIGHTING_LEVELS)

#define MAX_Y_POINTERS 1440

#ifdef BUILD_DESCENT2
//variables for clipping the texture-mapper to screen region
extern int Window_clip_left, Window_clip_bot, Window_clip_right, Window_clip_top;
#endif

// -------------------------------------------------------------------------------------------------------
// This is the main texture mapper call.
//	tmap_num references a texture map defined in Texmap_ptrs.
//	nverts = number of vertices
//	vertbuf is a pointer to an array of vertex pointers
extern void draw_tmap(grs_bitmap* bp, int nverts, g3s_point** vertbuf);

// -------------------------------------------------------------------------------------------------------
// Texture map vertex.
//	The fields r,g,b and l are mutually exclusive.  r,g,b are used for rgb lighting.
//	l is used for intensity based lighting.
typedef struct g3ds_vertex {
	fix	x, y, z;
	fix	u, v;
	fix	x2d, y2d;
	fix	l;
	fix	r, g, b;
} g3ds_vertex;

// A texture map is defined as a polygon with u,v coordinates associated with
// one point in the polygon, and a pair of vectors describing the orientation
// of the texture map in the world, from which the deltas Du_dx, Dv_dy, etc.
// are computed.
typedef struct g3ds_tmap {
	int	nv;			// number of vertices
	g3ds_vertex	verts[MAX_TMAP_VERTS];	// up to 8 vertices, this is inefficient, change
} g3ds_tmap;

//A texture mapper. Each 3D library instance has one texture mapper.
//Texmappers are entirely self contained, and can draw to different parts of one canvas on different threads,
//without any resource conflicts. 
class Texmap
{
	g3ds_tmap Tmap1;
	int	y_pointers[MAX_Y_POINTERS];
	int	bytes_per_row = -1;
	uint8_t* write_buffer;
	int  	window_left;
	int	window_right;
	int	window_top;
	int	window_bottom;
	int  	window_width;
	int  	window_height;
	int	Lighting_enabled;
	fix fx_l, fx_u, fx_v, fx_z, fx_du_dx, fx_dv_dx, fx_dz_dx, fx_dl_dx;
	int fx_xleft, fx_xright, fx_y;
	unsigned char* pixptr;
	int Transparency_on = 0;
	int Current_seg_depth;
	int Window_clip_left, Window_clip_bot, Window_clip_right, Window_clip_top;
	int Lighting_on = 0;
	int Interpolation_method = 0;
	int Darkening_level; //Needed for cloaking

	uint8_t tmap_flat_color;
	uint8_t tmap_flat_shade_value;
	RLECache local_cache; //Local RLE cache, since the game can do late decompression on occasion. This is a bit memory-hungry. 

	//Formerly global state for the scanline drawers. 
	//no idea why these are globals TBH, I may make them local to the scanline functions
	int loop_count, num_left_over;
	dbool new_end;

	unsigned int ut, vt;
	unsigned int ui, vi;
	int uvt, uvi;
	fix U0, V0, Z0, U1, V1;

	//Scanline drawing functions. These use the state of the current Texmap to draw a scanline. 

	void DrawScanlinePerspective();
	void DrawScanlinePerspectiveNoLight();
	void DrawScanlinePerspectivePer16();
	void DrawScanlinePerspectivePer16NoLight();
	void DrawScanlinePerspectivePer16Trans();
	void DrawScanlinePerspectivePer16TransNoLight();
	void DrawScanlineLinear();
	void DrawScanlineLinearNoLight();
	void DrawScanlineFlat();
	void DrawScanlineShaded();
	void DrawScanlinePerspectiveTrans();
	void DrawScanlinePerspectiveTransNoLight();
	void DrawScanlineLinearTrans();
	void DrawScanlineLinearTransNoLight();
	void DrawScanlineEditor();

	//Functions for starting scanlines. These call the scanline funcs above.

	void ScanlinePerspective(grs_bitmap* srcb, int y, fix xleft, fix xright, fix uleft, fix uright, fix vleft, fix vright, fix zleft, fix zright, fix lleft, fix lright);
	void ScanlinePerspectiveNoLight(grs_bitmap* srcb, int y, fix xleft, fix xright, fix uleft, fix uright, fix vleft, fix vright, fix zleft, fix zright, fix lleft, fix lright);
	void ScanlineLinear(grs_bitmap* srcb, int y, fix xleft, fix xright, fix uleft, fix uright, fix vleft, fix vright, fix lleft, fix lright);
	void ScanlineLinearNoLight(grs_bitmap* srcb, int y, fix xleft, fix xright, fix uleft, fix uright, fix vleft, fix vright, fix zleft, fix zright, fix lleft, fix lright);
	void ScanlineFlat(int y, fix xleft, fix xright);

	//Internal texture map drawing functions.

	void TMapPerspective(grs_bitmap* srcb, g3ds_tmap* t);
	void TMapLinear(grs_bitmap* srcb, g3ds_tmap* t);
	void TMapFlat(g3ds_tmap* t, int color);

public:
	Texmap();

	//Sets the canvas this Texmapper will draw to. Computes pointer table, window size variables, and so on.
	void SetCanvas(grs_canvas* canv);

	//Sets a clipping window separate from the window calculated in SetCanvas.
	// This is used to contain each Texmap instance to a subset of the buffer it's drawing to.
	void SetClipWindow(int left, int top, int right, int bottom)
	{
		Window_clip_left = left;
		Window_clip_bot = bottom;
		Window_clip_right = right;
		Window_clip_top = top;
	}
	//Sets the current segment depth. Hack from the vanilla code to determine texture mapper quality
	void SetSegmentDepth(int depth)
	{
		Current_seg_depth = depth;
	}

	//	Note:	Not all interpolation method and lighting combinations are supported.
	// Set Lighting_on to 0/1/2 for no lighting/intensity lighting/rgb lighting
	void SetLightingState(int useLighting)
	{
		Lighting_on = useLighting;
	}

	int GetLightingState() const
	{
		return Lighting_on;
	}

	//	Set Interpolation_method to 0/1/2 for linear/linear, perspective/linear, perspective/perspective
	void SetInterpolation(int newInterpolation)
	{
		Interpolation_method = newInterpolation;
	}

	int GetInterpolation() const
	{
		return Interpolation_method;
	}

	//Sets whether or not color #255 should be treated as transparent
	void SetTransparency(bool useTransparency)
	{
		Transparency_on = useTransparency;
	}

	//Draws a texture map, with nverts vertices as contained in vertbuf,
	//and using texutre bp. Will use the current lighting, interpolation, and transparency states.
	void DrawTMap(grs_bitmap* bp, int nverts, g3s_point** vertbuf);
	//Draws a solid polygon of the palette index color.
	void DrawFlat(int color, int nverts, int* vert); //Color moved directly to the call to avoid modifying gr state. 
	//Draws a solid polygon with TMap compatible parameters
	void DrawFlat(grs_bitmap* bp, int nverts, g3s_point** vertbuf);

	void SetDarkening(int level)
	{
		Darkening_level = level;
	}
};

//[ISB] these remain global state, since all texmapper instances will be using the same values. 
extern	int	Max_perspective_depth;		//	Deepest segment at which perspective interpolation will be used.
extern	int	Max_linear_depth;				//	Deepest segment at which linear interpolation will be used.
extern	int	Max_flat_depth;				//	Deepest segment at which flat shading will be used. (If not flat shading, then what?)

//	Set to !0 to enable Sim City 2000 (or Eric's Drive Through, or Eric's Game) specific code.
extern	int	SC2000;

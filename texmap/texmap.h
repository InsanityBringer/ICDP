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

#define	NUM_LIGHTING_LEVELS 32
#define MAX_TMAP_VERTS 25
#define MAX_LIGHTING_VALUE	((NUM_LIGHTING_LEVELS-1)*F1_0/NUM_LIGHTING_LEVELS)
#define MIN_LIGHTING_VALUE	(F1_0/NUM_LIGHTING_LEVELS)

#define MAX_Y_POINTERS 1280

#ifdef BUILD_DESCENT2
//variables for clipping the texture-mapper to screen region
extern int Window_clip_left, Window_clip_bot, Window_clip_right, Window_clip_top;
#endif

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
	int dither_intensity_lighting = 0;

	uint8_t tmap_flat_color;
	uint8_t tmap_flat_shade_value;

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

	//Functions for starting scanlines. These call the scanline funcs above.

	void ScanlinePerspective(grs_bitmap* srcb, int y, fix xleft, fix xright, fix uleft, fix uright, fix vleft, fix vright, fix zleft, fix zright, fix lleft, fix lright);
	void ScanlinePerspectiveNoLight(grs_bitmap* srcb, int y, fix xleft, fix xright, fix uleft, fix uright, fix vleft, fix vright, fix zleft, fix zright, fix lleft, fix lright);
	void ScanlineLinear(grs_bitmap* srcb, int y, fix xleft, fix xright, fix uleft, fix uright, fix vleft, fix vright, fix zleft, fix zright, fix lleft, fix lright);
	void ScanlineLinearNoLight(grs_bitmap* srcb, int y, fix xleft, fix xright, fix uleft, fix uright, fix vleft, fix vright, fix zleft, fix zright, fix lleft, fix lright);

public:
	Texmap();

	//Sets the canvas this Texmapper will draw to. Computes pointer table, window size variables, and so on.
	void SetCanvas(grs_canvas& canv);

	//Sets a clipping window separate from the window calculated in SetCanvas.
	// This is used to contain each Texmap instance to a subset of the buffer it's drawing to.
	void SetClipWindow(int left, int bottom, int right, int top);
	//Sets the current thresholds for texture mapper detail levels, in terms of segments deep.
	// persp is how far tmaps will be rendered with perspective correct interpolation.
	// linear is how far tmaps will be rendered with linear interpolation.
	// flat is how far tmaps will be rendered as flat polygons. Deeper tmaps will be rejected.
	void SetDetailLevelThresholds(int persp, int linear, int flat);
	//Sets the current segment depth. Hack from the vanilla code to determine texture mapper quality
	void SetSegmentDepth(int depth);
	//Sets the lighting state for drawing tmaps.
	void SetLightingState(bool useLighting);
	//Sets the current interpolation method.
	void SetInterpolation(int newInterpolation);
	//Sets whether or not color #255 should be treated as transparent
	void SetTransparency(bool useTransparency);

	//Draws a texture map, with nverts vertices as contained in vertbuf,
	//and using texutre bp. Will use the current lighting, interpolation, and transparency states.
	void DrawTMap(grs_bitmap* bp, int nverts, g3s_point** vertbuf);
	//Draws a solid polygon of the palette index color.
	void DrawFlat(int color, int nverts, int* vert); //Color moved directly to the call to avoid modifying gr state. 
};

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

// -------------------------------------------------------------------------------------------------------

//	Note:	Not all interpolation method and lighting combinations are supported.
//	Set Interpolation_method to 0/1/2 for linear/linear, perspective/linear, perspective/perspective
extern	int	Interpolation_method;

// Set Lighting_on to 0/1/2 for no lighting/intensity lighting/rgb lighting
extern	int	Lighting_on;

// HACK INTERFACE: how far away the current segment (& thus texture) is
extern	int	Current_seg_depth;
extern	int	Max_perspective_depth;		//	Deepest segment at which perspective interpolation will be used.
extern	int	Max_linear_depth;				//	Deepest segment at which linear interpolation will be used.
extern	int	Max_flat_depth;				//	Deepest segment at which flat shading will be used. (If not flat shading, then what?)

extern void ntexture_map_lighted_linear(grs_bitmap* srcb, g3ds_tmap* t);

//	This is the gr_upoly-like interface to the texture mapper which uses texture-mapper compatible
//	(ie, avoids cracking) edge/delta computation.
void gr_upoly_tmap(int nverts, int* vert);

//This is like gr_upoly_tmap() but instead of drawing, it calls the specified
//function with ylr values
void gr_upoly_tmap_ylr(int nverts, int* vert, void(*ylr_func)(int, fix, fix));

extern int Transparency_on;

//	Set to !0 to enable Sim City 2000 (or Eric's Drive Through, or Eric's Game) specific code.
extern	int	SC2000;

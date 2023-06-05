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

//[ISB] disgusting hack, texmap.h relies on some bits of 3d.h. 
//3d.h needs to be able to put a texmap instance in a 3d lib instance. 
//Including texmap on its own will invariably mess it up, so force it to be
//included through 3d.h
#define THREED_H

#include "fix/fix.h"
#include "vecmat/vecmat.h"
#include "2d/gr.h"
#include "2d/scale.h"

extern int g3d_interp_outline;		//if on, polygon models outlined in white

//Structure for storing u,v,light values.  This structure doesn't have a
//prefix because it was defined somewhere else before it was moved here
typedef struct g3s_uvl 
{
	fix u, v, l;
} g3s_uvl;

//Stucture to store clipping codes in a word
typedef struct g3s_codes 
{
	uint8_t low, high;	//or is low byte, and is high byte
} g3s_codes;

//flags for point structure
#define PF_PROJECTED 	1	//has been projected, so sx,sy valid
#define PF_OVERFLOW		2	//can't project
#define PF_TEMP_POINT	4	//created during clip
#define PF_UVS				8	//has uv values set
#define PF_LS				16	//has lighting values set

//clipping codes flags

#define CC_OFF_LEFT	1
#define CC_OFF_RIGHT	2
#define CC_OFF_BOT	4
#define CC_OFF_TOP	8
#define CC_BEHIND		0x80

//Used to store rotated points for mines.  Has frame count to indictate
//if rotated, and flag to indicate if projected.
typedef struct g3s_point 
{
	vms_vector p3_vec;         //reference as vector...
	fix p3_u, p3_v, p3_l;
	fix p3_sx, p3_sy;		//screen x&y
	uint8_t p3_codes;		//clipping codes
	uint8_t p3_flags;		//projected?
	short p3_pad;			//keep structure longwork aligned
} g3s_point;

#include "texmap/texmap.h"

//macros to reference x,y,z elements of a 3d point
#define p3_x p3_vec.x
#define p3_y p3_vec.y
#define p3_z p3_vec.z

#define MAX_INSTANCE_DEPTH	10

struct instance_context
{
	vms_matrix m;
	vms_vector p;
};

#define G3_MAX_INTERP_POINTS 1000
#define G3_MAX_POINTS_PER_POLY 25
#define G3_MAX_POINTS_IN_POLY 100

//A G3Drawer, unlike a G3Instance, stores no state for anything unrelated to simple drawing. 
//All state relating to the camera, projection matrix, and so on will have been applied to the commands executed here. 
class G3Drawer
{
	fix	clip_ratios[4]; //clipping is performed during this stage, and clip ratios are stored for each tile. 
	//Local clipping window. Used to clamp the window given to the texture mapper. 
	int window_left, window_top, window_right, window_bottom;

	//vertex buffers for polygon drawing and clipping
	g3s_point* Vbuf0[G3_MAX_POINTS_IN_POLY];
	g3s_point* Vbuf1[G3_MAX_POINTS_IN_POLY];

	//list of 2d coords
	fix Vertex_list[G3_MAX_POINTS_IN_POLY * 2];

	//Clipper variables
	int free_point_num = 0;
	g3s_point temp_points[G3_MAX_POINTS_IN_POLY];
	g3s_point* free_points[G3_MAX_POINTS_IN_POLY];
	
	//OPTIMIZE: Currently I copy all points into this temporary buffer
	//because they need to be coded specifically for each cell
	g3s_point temp_point_buffer[G3_MAX_POINTS_IN_POLY];

	grs_point blob_vertices[4];

	fix Canv_w2, Canv_h2;

	//A texture mapper. Each G3 drawer has one, for multithreading purposes.
	//Can only be accessed from the outside world via draw_poly and draw_tmap.
	Texmap texmap_instance;
	//A bitmap scalar, since the scalar was also dependent on global state.
	//I wonder if I should make bitmap scaling use the texmapper instead?
	GrScalar scalar_instance;

	grs_canvas* canv;

	//Debugging: Amount of commands decoded
	int num_commands_decoded = -1;

	//Private drawing functions
	bool must_clip_line(g3s_point* p0, g3s_point* p1, uint8_t codes_or, int color);
	bool must_clip_flat_face(int nv, g3s_codes cc, int color);
	bool must_clip_tmap_face(int nv, g3s_codes cc, grs_bitmap* bm);

	//Private clipping functions
	void free_temp_point(g3s_point* p);
	g3s_point* get_temp_point();
	g3s_point* clip_edge(int plane_flag, g3s_point* on_pnt, g3s_point* off_pnt);
	int clip_plane(int plane_flag, g3s_point** src, g3s_point** dest, int* nv, g3s_codes* cc);
	g3s_point** clip_polygon(g3s_point** src, g3s_point** dest, int* nv, g3s_codes* cc);
	void init_free_points(void);
	void clip_line(g3s_point** p0, g3s_point** p1, uint8_t codes_or);

public:
	G3Drawer();

	void start_frame(fix canv_w2, fix canv_h2);
	void end_frame();

	//Begins decoding the command buffer. Decoding happens while recording may still be occurring. 
	//This function will continue until all commands are decoded and recording has finished.
	void decode_command_buffer();

	//Needs access to thread local clip planes
	uint8_t code_point(g3s_point* p);

	//projects a point
	void project_point(g3s_point* point);

	//draw a flat-shaded face.
	void draw_poly(int nv, g3s_point** pointlist, int color);
	void draw_poly_direct(int nv, g3s_point* pointlist, int color);

	//draw a texture-mapped face.
	void draw_tmap(int nv, g3s_point** pointlist, g3s_uvl* uvl_list, grs_bitmap* bm);
	void draw_tmap_direct(int nv, g3s_point* pointlist, g3s_uvl* uvl_list, grs_bitmap* bm);

	//draw a sortof sphere - i.e., the 2d radius is proportional to the 3d
	//radius, but not to the distance from the eye
	void draw_sphere(g3s_point* pnt, fix rad);

	bool draw_line(g3s_point* p0, g3s_point* p1, int color);

	void draw_bitmap(g3s_point* pnt, fix w, fix h, grs_bitmap* bm, int orientation);

	//Set clip ratios. All should be in the range [-1.0, 1.0]
	void set_clip_ratios(fix left, fix top, fix right, fix bottom)
	{
		clip_ratios[0] = right;
		clip_ratios[1] = top;
		clip_ratios[2] = left;
		clip_ratios[3] = bottom;
	}

	void set_clip_bounds(int left, int top, int right, int bottom)
	{
		window_left = left; window_top = top; window_right = right; window_bottom = bottom;

		scalar_instance.set_clip_bounds(left, top, right, bottom);
	}

	//This sets the Texmap clip window, but clamped to ensure it remains in the 
	void set_tmap_clip_window(int left, int top, int right, int bottom)
	{
		if (left < window_left)
			left = window_left;
		else if (left > window_right)
			left = window_right;

		if (top < window_top)
			top = window_top;
		else if (top > window_bottom)
			top = window_bottom;

		if (right < window_left)
			right = window_left;
		else if (right > window_right)
			right = window_right;

		if (bottom < window_top)
			bottom = window_top;
		else if (bottom > window_bottom)
			bottom = window_bottom;

		texmap_instance.SetClipWindow(left, top, right, bottom);
	}

	//Sets the canvas. ATM this just passes the canvas down to the various drawers
	void set_canvas(grs_canvas* canvas)
	{
		canv = canvas;
		texmap_instance.SetCanvas(canvas);
		scalar_instance.set_canvas(canvas);
	}

	Texmap& get_texmap_instance()
	{
		return texmap_instance;
	}

	int get_num_commands_decoded()
	{
		return num_commands_decoded;
	}

	void reset_debug_counters()
	{
		num_commands_decoded = -1;
	}

	grs_canvas* get_canvas()
	{
		return canv;
	}
};

//3D library class, encapsulates most of the functionality of the 3D library for multithreading
//The G3Instance will record to the command buffer when multithreading is enabled, or it will
//draw directly when multithreading is disabled. 
class G3Instance
{
	vms_vector	View_position;
	fix			View_zoom;

	vms_matrix	Unscaled_matrix;	//before scaling
	vms_matrix	View_matrix;

	vms_vector	Window_scale;		//scaling for window aspect
	vms_vector	Matrix_scale;		//how the matrix is scaled, window_scale * zoom

	int			Canvas_width;		//the actual width
	int			Canvas_height;		//the actual height

	fix			Canv_w2;				//fixed-point width/2
	fix			Canv_h2;				//fixed-point height/2

	fix			clip_ratios[4];

	//performs aspect scaling on global view matrix
	void scale_matrix(void);

	//vertex buffers for polygon drawing and clipping
	g3s_point* Vbuf0[G3_MAX_POINTS_IN_POLY];
	g3s_point* Vbuf1[G3_MAX_POINTS_IN_POLY];

	//list of 2d coords
	fix Vertex_list[G3_MAX_POINTS_IN_POLY * 2];

	G3Drawer drawer;
	//Instance variables
	instance_context instance_stack[MAX_INSTANCE_DEPTH];
	int instance_depth = 0;

	//Clipper variables
	int free_point_num = 0;
	g3s_point temp_points[G3_MAX_POINTS_IN_POLY];
	g3s_point* free_points[G3_MAX_POINTS_IN_POLY];

	//interp variables
	g3s_point interp_point_list[G3_MAX_INTERP_POINTS];
	int glow_num;
	g3s_point* point_list[G3_MAX_POINTS_PER_POLY];

	//rod variables
	g3s_point rod_points[4];
	g3s_point* rod_point_list[4];

	//Private drawing functions
	dbool do_facing_check(vms_vector* norm, g3s_point** vertlist, vms_vector* p);

	//Private rod-drawing functions
	int calc_rod_corners(g3s_point* bot_point, fix bot_width, g3s_point* top_point, fix top_width);

	//Private interp functions
	void rotate_point_list(g3s_point* dest, vms_vector* src, int n);

	//Private threading functions
	//Dispatches render threads if multithreading is enabled. Determines how many jobs to create and assigns workers to them
	void dispatch_render_threads();

public:
	G3Instance();
	//Frame setup functions

	//start the frame
	void start_frame();
	//start the frame by copying the frame data from another instance (including the view position and matrix)
	//This must be called after start_frame is called for inst, but before any instances are started.
	void start_frame_from_inst(G3Instance& inst);
	//set view from x,y,z & p,b,h, zoom.
	void set_view(vms_vector* view_pos, vms_angvec* view_orient, fix zoom);
	//set view from x,y,z, viewer matrix, and zoom.
	void set_view(vms_vector* view_pos, vms_matrix* view_matrix, fix zoom);
	//end the frame
	void end_frame();

	//Instancing

	//instance at specified point with specified orientation
	void start_instance(vms_vector* pos, vms_matrix* orient);
	//instance at specified point with specified orientation
	void start_instance(vms_vector* pos, vms_angvec* angles);
	//pops the old context
	void done_instance();

	//Misc utility functions:

	//returns true if a plane is facing the viewer. takes the unrotated surface 
	//normal of the plane, and a point on it.  The normal need not be normalized
	dbool check_normal_facing(vms_vector* v, vms_vector* norm);

	//Point projection functions:
	//These need to be part of the class since they use the projection parameters. 

	//rotates a point. returns codes.  does not check if already rotated
	uint8_t rotate_point(g3s_point* dest, vms_vector* src);

	//projects a point
	void project_point(g3s_point* point)
	{
		drawer.project_point(point);
	}

	//Needs access to thread local clip planes
	uint8_t code_point(g3s_point* p)
	{
		return drawer.code_point(p);
	}

	//calculate the depth of a point - returns the z coord of the rotated point
	fix calc_point_depth(vms_vector* pnt);

	void point_2_vec(vms_vector* v, short sx, short sy);

	//delta rotation functions
	vms_vector* rotate_delta_x(vms_vector* dest, fix dx);
	vms_vector* rotate_delta_y(vms_vector* dest, fix dy);
	vms_vector* rotate_delta_z(vms_vector* dest, fix dz);
	vms_vector* rotate_delta_vec(vms_vector* dest, vms_vector* src);
	uint8_t add_delta_vec(g3s_point* dest, g3s_point* src, vms_vector* deltav);

	//Drawing functions:

	//draw a flat-shaded face.
	//returns 1 if off screen, 0 if drew
	dbool draw_poly(int nv, g3s_point** pointlist);

	//draw a texture-mapped face.
	//returns 1 if off screen, 0 if drew
	dbool draw_tmap(int nv, g3s_point** pointlist, g3s_uvl* uvl_list, grs_bitmap* bm);

	//draw a sortof sphere - i.e., the 2d radius is proportional to the 3d
	//radius, but not to the distance from the eye
	int draw_sphere(g3s_point* pnt, fix rad);

	//like g3_draw_poly(), but checks to see if facing.  If surface normal is
	//NULL, this routine must compute it, which will be slow.  It is better to 
	//pre-compute the normal, and pass it to this function.  When the normal
	//is passed, this function works like g3_check_normal_facing() plus
	//g3_draw_poly().
	//returns -1 if not facing, 1 if off screen, 0 if drew
	dbool check_and_draw_poly(int nv, g3s_point** pointlist, vms_vector* norm, vms_vector* pnt);
	dbool check_and_draw_tmap(int nv, g3s_point** pointlist, g3s_uvl* uvl_list, grs_bitmap* bm, vms_vector* norm, vms_vector* pnt);

	//draws a line. takes two points.
	dbool draw_line(g3s_point* p0, g3s_point* p1);

	//draw a polygon that is always facing you
	//returns 1 if off screen, 0 if drew
	dbool draw_rod_flat(g3s_point* bot_point, fix bot_width, g3s_point* top_point, fix top_width);

	//draw a bitmap object that is always facing you
	//returns 1 if off screen, 0 if drew
	dbool draw_rod_tmap(grs_bitmap* bitmap, g3s_point* bot_point, fix bot_width, g3s_point* top_point, fix top_width, fix light);

	//draws a bitmap with the specified 3d width & height 
	//returns 1 if off screen, 0 if drew
	dbool draw_bitmap(vms_vector* pos, fix width, fix height, grs_bitmap* bm, int orientation);

	//Object functions:

	//calls the object interpreter to render an object.  The object renderer
	//is really a seperate pipeline. returns true if drew
	dbool draw_polygon_model(void* model_ptr, grs_bitmap** model_bitmaps, vms_angvec* anim_angles, fix light, fix* glow_values);

	//alternate interpreter for morphing object
	dbool draw_morphing_model(void* model_ptr, grs_bitmap** model_bitmaps, vms_angvec* anim_angles, fix light, vms_vector* new_points);

	//Set clip ratios. All should be in the range [-1.0, 1.0]
	void set_clip_ratios(fix left, fix top, fix right, fix bottom)
	{
		clip_ratios[0] = right;
		clip_ratios[1] = top;
		clip_ratios[2] = left;
		clip_ratios[3] = bottom;
		drawer.set_clip_ratios(left, top, right, bottom);
	}

	//Expose the texmapper's state function
	void set_lighting_mode(int new_mode);
	int get_lighting_mode()
	{
		return drawer.get_texmap_instance().GetLightingState();
	}
	void set_interpolation_mode(int new_mode);
	int get_interpolation_mode()
	{
		return drawer.get_texmap_instance().GetInterpolation();
	}
	void set_segment_depth(int depth);
	void set_clip_window(int left, int top, int right, int bottom);

	void set_darkening_level(int level);

	//debugging
	void debug_decode_command_buffer()
	{
		drawer.decode_command_buffer();
	}
};

extern G3Instance g3_global_inst;

//Functions in library

//3d system startup and shutdown:

//initialize the 3d system
void g3_init(void);

//close down the 3d system
void g3_close(void);


//Frame setup functions:

//start the frame
void g3_start_frame(void);

//set view from x,y,z & p,b,h, zoom.  Must call one of g3_set_view_*() 
void g3_set_view_angles(vms_vector* view_pos, vms_angvec* view_orient, fix zoom);

//set view from x,y,z, viewer matrix, and zoom.  Must call one of g3_set_view_*() 
void g3_set_view_matrix(vms_vector* view_pos, vms_matrix* view_matrix, fix zoom);

//end the frame
void g3_end_frame(void);

//draw a horizon
void g3_draw_horizon(int sky_color, int ground_color);

//get vectors that are edge of horizon
int g3_compute_sky_polygon(fix* points_2d, vms_vector* vecs);

//Instancing

//instance at specified point with specified orientation
void g3_start_instance_matrix(vms_vector* pos, vms_matrix* orient);

//instance at specified point with specified orientation
void g3_start_instance_angles(vms_vector* pos, vms_angvec* angles);

//pops the old context
void g3_done_instance();

//Misc utility functions:

//returns true if a plane is facing the viewer. takes the unrotated surface 
//normal of the plane, and a point on it.  The normal need not be normalized
dbool g3_check_normal_facing(vms_vector* v, vms_vector* norm);

//Point definition and rotation functions:

//rotates a point. returns codes.  does not check if already rotated
uint8_t g3_rotate_point(g3s_point* dest, vms_vector* src);

//projects a point
void g3_project_point(g3s_point* point);

//calculate the depth of a point - returns the z coord of the rotated point
fix g3_calc_point_depth(vms_vector* pnt);

//from a 2d point, compute the vector through that point
void g3_point_2_vec(vms_vector* v, short sx, short sy);

//code a point.  fills in the p3_codes field of the point, and returns the codes
uint8_t g3_code_point(g3s_point* point);

//delta rotation functions
vms_vector* g3_rotate_delta_x(vms_vector* dest, fix dx);
vms_vector* g3_rotate_delta_y(vms_vector* dest, fix dy);
vms_vector* g3_rotate_delta_z(vms_vector* dest, fix dz);
vms_vector* g3_rotate_delta_vec(vms_vector* dest, vms_vector* src);
uint8_t g3_add_delta_vec(g3s_point* dest, g3s_point* src, vms_vector* deltav);

//Drawing functions:

//draw a flat-shaded face.
//returns 1 if off screen, 0 if drew
dbool g3_draw_poly(int nv, g3s_point** pointlist);

//draw a texture-mapped face.
//returns 1 if off screen, 0 if drew
dbool g3_draw_tmap(int nv, g3s_point** pointlist, g3s_uvl* uvl_list, grs_bitmap* bm);

//draw a sortof sphere - i.e., the 2d radius is proportional to the 3d
//radius, but not to the distance from the eye
int g3_draw_sphere(g3s_point* pnt, fix rad);

//like g3_draw_poly(), but checks to see if facing.  If surface normal is
//NULL, this routine must compute it, which will be slow.  It is better to 
//pre-compute the normal, and pass it to this function.  When the normal
//is passed, this function works like g3_check_normal_facing() plus
//g3_draw_poly().
//returns -1 if not facing, 1 if off screen, 0 if drew
dbool g3_check_and_draw_poly(int nv, g3s_point** pointlist, vms_vector* norm, vms_vector* pnt);
dbool g3_check_and_draw_tmap(int nv, g3s_point** pointlist, g3s_uvl* uvl_list, grs_bitmap* bm, vms_vector* norm, vms_vector* pnt);

//draws a line. takes two points.
dbool g3_draw_line(g3s_point* p0, g3s_point* p1);

//draw a polygon that is always facing you
//returns 1 if off screen, 0 if drew
dbool g3_draw_rod_flat(g3s_point* bot_point, fix bot_width, g3s_point* top_point, fix top_width);

//draw a bitmap object that is always facing you
//returns 1 if off screen, 0 if drew
dbool g3_draw_rod_tmap(grs_bitmap* bitmap, g3s_point* bot_point, fix bot_width, g3s_point* top_point, fix top_width, fix light);

//draws a bitmap with the specified 3d width & height 
//returns 1 if off screen, 0 if drew
#ifdef BUILD_DESCENT2
dbool g3_draw_bitmap(vms_vector* pos, fix width, fix height, grs_bitmap* bm, int orientation);
#else
dbool g3_draw_bitmap(vms_vector* pos, fix width, fix height, grs_bitmap* bm);
#endif

//specifies 2d drawing routines to use instead of defaults.  Passing
//NULL for either or both restores defaults
void g3_set_special_render(void (*tmap_drawer)(grs_bitmap* bm, int nv, g3s_point** vertlist), void (*flat_drawer)(int nv, int* vertlist), int (*line_drawer)(fix x0, fix y0, fix x1, fix y1));

//Object functions:

//gives the interpreter an array of points to use
void g3_set_interp_points(g3s_point* pointlist);

//calls the object interpreter to render an object.  The object renderer
//is really a seperate pipeline. returns true if drew
dbool g3_draw_polygon_model(void* model_ptr, grs_bitmap** model_bitmaps, vms_angvec* anim_angles, fix light, fix* glow_values);

//init code for bitmap models
void g3_init_polygon_model(void* model_ptr);

//un-initialize, i.e., convert color entries back to RGB15
void g3_uninit_polygon_model(void* model_ptr);

//alternate interpreter for morphing object
dbool g3_draw_morphing_model(void* model_ptr, grs_bitmap** model_bitmaps, vms_angvec* anim_angles, fix light, vms_vector* new_points);

//this remaps the 15bpp colors for the models into a new palette.  It should
//be called whenever the palette changes
void g3_remap_interp_colors(void);

//Sets the darkening level for any future draws.
//Set level to GR_NUM_FADES to disable
void g3_set_darkening_level(int level);

// routine to convert little to big endian in polygon model data
void swap_polygon_model_data(uint8_t* data);

//[ISB] These are needed to get to the texmapper state of the implicit 3D state
void g3_set_lighting_mode(int new_mode);
int g3_get_lighting_mode();
void g3_set_interpolation_mode(int new_mode);
int g3_get_interpolation_mode();

inline G3Instance& g3_global_instance()
{
	return g3_global_inst;
}

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

#include <string.h>
#include "3d/3d.h"
#include "globvars.h"

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

//vertex buffers for polygon drawing and clipping
g3s_point* Vbuf0[MAX_POINTS_IN_POLY];
g3s_point* Vbuf1[MAX_POINTS_IN_POLY];

//list of 2d coords
fix Vertex_list[MAX_POINTS_IN_POLY * 2];

G3Instance g3_global_inst;

G3Instance::G3Instance()
{
	View_position = {};
	View_zoom = 0;

	Unscaled_matrix = {};
	View_matrix = {};

	Window_scale = {};
	Matrix_scale = {};

	Canvas_width = Canvas_height = 0;

	Canv_w2 = Canv_h2 = 0;

	memset(Vbuf0, 0, sizeof(Vbuf0));
	memset(Vbuf1, 0, sizeof(Vbuf1));

	memset(Vertex_list, 0, sizeof(Vertex_list));

	memset(instance_stack, 0, sizeof(instance_stack));
	instance_depth = 0;

	free_point_num = 0;
	memset(temp_points, 0, sizeof(temp_points));
	memset(free_points, 0, sizeof(free_points));

	memset(interp_point_list, 0, sizeof(interp_point_list));
	glow_num = 0;
	memset(point_list, 0, sizeof(point_list));

	memset(blob_vertices, 0, sizeof(blob_vertices));
	memset(rod_points, 0, sizeof(rod_points));
	rod_point_list[0] = &rod_points[0];
	rod_point_list[1] = &rod_points[1];
	rod_point_list[2] = &rod_points[2];
	rod_point_list[3] = &rod_points[3];

	set_clip_ratios(-F1_0, F1_0, F1_0, -F1_0);
}

G3Drawer::G3Drawer()
{
	Canv_w2 = Canv_h2 = 0;
	memset(Vbuf0, 0, sizeof(Vbuf0));
	memset(Vbuf1, 0, sizeof(Vbuf1));
	memset(Vertex_list, 0, sizeof(Vertex_list));

	free_point_num = 0;
	memset(temp_points, 0, sizeof(temp_points));
	memset(free_points, 0, sizeof(free_points));

	set_clip_ratios(-F1_0, F1_0, F1_0, -F1_0);
}

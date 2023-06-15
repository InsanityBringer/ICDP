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

#include "misc/error.h"
#include "3d/3d.h"
#include "texmap/texmap.h"
#include "misc/types.h"
#include <string.h>

int (*line_drawer_ptr)(fix x0, fix y0, fix x1, fix y1) = gr_line;

//specifies 2d drawing routines to use instead of defaults.  Passing
//NULL for either or both restores defaults
void g3_set_special_render(void (*tmap_drawer)(grs_bitmap* bm, int nv, g3s_point** vertlist), void (*flat_drawer)(int nv, int* vertlist), int (*line_drawer)(fix x0, fix y0, fix x1, fix y1))
{
	//tmap_drawer_ptr = (tmap_drawer) ? tmap_drawer : draw_tmap;
	//flat_drawer_ptr = (flat_drawer) ? flat_drawer : gr_upoly_tmap;
	//line_drawer_ptr = (line_drawer) ? line_drawer : gr_line;
}

void g3_set_lighting_mode(int new_mode)
{
	g3_global_inst.set_lighting_mode(new_mode);
}

int g3_get_lighting_mode()
{
	return g3_global_inst.get_lighting_mode();
}

void g3_set_interpolation_mode(int new_mode)
{
	g3_global_inst.set_interpolation_mode(new_mode);
}

int g3_get_interpolation_mode()
{
	return g3_global_inst.get_interpolation_mode();
}

void g3_set_darkening_level(int level)
{
	g3_global_inst.set_darkening_level(level);
}

int gr_line_explicit_clip(grs_canvas* canvas, int color, fix a1, fix b1, fix a2, fix b2, int minx, int miny, int maxx, int maxy);

bool G3Drawer::must_clip_flat_face(int nv, g3s_codes cc, int color)
{
	int i;
	bool ret = false; //[ISB] initalize
	g3s_point** bufptr;

	bufptr = clip_polygon(Vbuf0, Vbuf1, &nv, &cc);

	if (nv > 0 && !(cc.low & CC_BEHIND) && !cc.high)
	{
		for (i = 0; i < nv; i++)
		{
			g3s_point* p = bufptr[i];

			if (!(p->p3_flags & PF_PROJECTED))
				project_point(p);

			if (p->p3_flags & PF_OVERFLOW)
			{
				ret = true;
				goto free_points;
			}

			Vertex_list[i * 2] = p->p3_sx;
			Vertex_list[i * 2 + 1] = p->p3_sy;
		}

		texmap_instance.DrawFlat(color, nv, (int*)Vertex_list);
	}
	else
		ret = true;

	//free temp points
free_points:
	;

	for (i = 0; i < nv; i++)
		if (Vbuf1[i]->p3_flags & PF_TEMP_POINT)
			free_temp_point(Vbuf1[i]);

	//	Assert(free_point_num==0);

	return ret;
}

bool G3Drawer::must_clip_tmap_face(int nv, g3s_codes cc, grs_bitmap* bm)
{
	g3s_point** bufptr;
	int i;

	bufptr = clip_polygon(Vbuf0, Vbuf1, &nv, &cc);

	if (nv && !(cc.low & CC_BEHIND) && !cc.high)
	{
		for (i = 0; i < nv; i++)
		{
			g3s_point* p = bufptr[i];

			if (!(p->p3_flags & PF_PROJECTED))
				project_point(p);

			if (p->p3_flags & PF_OVERFLOW)
			{
				//Int3();		//should not overflow after clip
				goto free_points;
			}
		}

		texmap_instance.DrawTMap(bm, nv, bufptr);
	}

free_points:
	;

	for (i = 0; i < nv; i++)
		if (bufptr[i]->p3_flags & PF_TEMP_POINT)
			free_temp_point(bufptr[i]);

	//	Assert(free_point_num==0);

	return false;
}

dbool G3Instance::do_facing_check(vms_vector* norm, g3s_point** vertlist, vms_vector* p)
{
	if (norm) //have normal
	{
		Assert(norm->x || norm->y || norm->z);

		return check_normal_facing(p, norm);
	}
	else //normal not specified, so must compute
	{
		vms_vector tempv;

		//get three points (rotated) and compute normal
		vm_vec_perp(&tempv, &vertlist[0]->p3_vec, &vertlist[1]->p3_vec, &vertlist[2]->p3_vec);

		return (vm_vec_dot(&tempv, &vertlist[1]->p3_vec) < 0);
	}
}

//returns true if a plane is facing the viewer. takes the unrotated surface 
//normal of the plane, and a point on it.  The normal need not be normalized
dbool G3Instance::check_normal_facing(vms_vector* v, vms_vector* norm)
{
	vms_vector tempv;

	vm_vec_sub(&tempv, &View_position, v);
	return (vm_vec_dot(&tempv, norm) > 0);
}

//draw a flat-shaded face.
//returns 1 if off screen, 0 if drew
void G3Drawer::draw_poly(int nv, g3s_point** pointlist, int color)
{
	int i;
	g3s_point** bufptr;
	g3s_codes cc;

	cc.low = 0; cc.high = 0xff;

	bufptr = Vbuf0;

	for (i = 0; i < nv; i++)
	{
		bufptr[i] = pointlist[i];

		cc.high &= bufptr[i]->p3_codes;
		cc.low |= bufptr[i]->p3_codes;
	}

	if (cc.high)
		return;// 1;	//all points off screen

	if (cc.low)
	{
		must_clip_flat_face(nv, cc, color);
		return;
	}

	//now make list of 2d coords (& check for overflow)

	for (i = 0; i < nv; i++)
	{
		g3s_point* p = bufptr[i];

		if (!(p->p3_flags & PF_PROJECTED))
			project_point(p);

		if (p->p3_flags & PF_OVERFLOW)
		{
			must_clip_flat_face(nv, cc, color);
			return;
		}

		Vertex_list[i * 2] = p->p3_sx;
		Vertex_list[i * 2 + 1] = p->p3_sy;
	}

	texmap_instance.DrawFlat(color, nv, (int*)Vertex_list);
}

//draw a flat-shaded face.
void G3Drawer::draw_poly_direct(int nv, g3s_point* pointlist, int color)
{
	int i;
	g3s_point** bufptr;
	g3s_codes cc;

	cc.low = 0; cc.high = 0xff;

	bufptr = Vbuf0;

	//this is far from elegant, but I need a copy of the points for each thread. Otherwise the coding will leak into the command buffer
	memcpy(temp_point_buffer, pointlist, nv * sizeof(g3s_point));

	for (i = 0; i < nv; i++)
	{
		bufptr[i] = &temp_point_buffer[i];

		//Well I totally messed up here, all points need to be recoded with the new clip planes for this region
		code_point(bufptr[i]);

		cc.high &= bufptr[i]->p3_codes;
		cc.low |= bufptr[i]->p3_codes;
	}

	if (cc.high)
		return;// 1;	//all points off screen

	if (cc.low)
	{
		must_clip_flat_face(nv, cc, color);
		return;
	}

	//now make list of 2d coords (& check for overflow)

	for (i = 0; i < nv; i++)
	{
		g3s_point* p = bufptr[i];

		if (!(p->p3_flags & PF_PROJECTED))
			project_point(p);

		if (p->p3_flags & PF_OVERFLOW)
		{
			must_clip_flat_face(nv, cc, color);
			return;
		}

		Vertex_list[i * 2] = p->p3_sx;
		Vertex_list[i * 2 + 1] = p->p3_sy;
	}

	texmap_instance.DrawFlat(color, nv, (int*)Vertex_list);
}

//draw a texture-mapped face.
//returns 1 if off screen, 0 if drew
void G3Drawer::draw_tmap(int nv, g3s_point** pointlist, g3s_uvl* uvl_list, grs_bitmap* bm)
{
	int i;
	g3s_point** bufptr;
	g3s_codes cc;

	cc.low = 0; cc.high = 0xff;

	bufptr = Vbuf0;

	for (i = 0; i < nv; i++)
	{
		g3s_point* p;

		p = bufptr[i] = pointlist[i];

		cc.high &= p->p3_codes;
		cc.low |= p->p3_codes;

		p->p3_u = uvl_list[i].u;
		p->p3_v = uvl_list[i].v;
		p->p3_l = uvl_list[i].l;

		p->p3_flags |= PF_UVS + PF_LS;
	}

	if (cc.high)
		return;// 1;	//all points off screen

	if (cc.low)
	{
		must_clip_tmap_face(nv, cc, bm);
		return;
	}

	//now make list of 2d coords (& check for overflow)

	for (i = 0; i < nv; i++)
	{
		g3s_point* p = bufptr[i];

		if (!(p->p3_flags & PF_PROJECTED))
			project_point(p);

		if (p->p3_flags & PF_OVERFLOW)
		{
			Int3();		//should not overflow after clip
			return;// 255;
		}
	}

	texmap_instance.DrawTMap(bm, nv, bufptr);

	//return 0;	//say it drew
}

//draw a texture-mapped face.
void G3Drawer::draw_tmap_direct(int nv, g3s_point* pointlist, g3s_uvl* uvl_list, grs_bitmap* bm)
{
	int i;
	g3s_point** bufptr;
	g3s_codes cc;

	cc.low = 0; cc.high = 0xff;

	bufptr = Vbuf0;
	
	//this is far from elegant, but I need a copy of the points for each thread. Otherwise the coding will leak into the command buffer
	memcpy(temp_point_buffer, pointlist, nv * sizeof(g3s_point));

	for (i = 0; i < nv; i++)
	{
		g3s_point* p;

		p = bufptr[i] = &temp_point_buffer[i];

		//Well I totally messed up here, all points need to be recoded with the new clip planes for this region
		code_point(bufptr[i]);

		cc.high &= p->p3_codes;
		cc.low |= p->p3_codes;

		p->p3_u = uvl_list[i].u;
		p->p3_v = uvl_list[i].v;
		p->p3_l = uvl_list[i].l;

		p->p3_flags |= PF_UVS + PF_LS;
	}

	if (cc.high)
		return;// 1;	//all points off screen

	if (cc.low)
	{
		must_clip_tmap_face(nv, cc, bm);
		return;
	}

	//now make list of 2d coords (& check for overflow)

	for (i = 0; i < nv; i++)
	{
		g3s_point* p = bufptr[i];

		if (!(p->p3_flags & PF_PROJECTED))
			project_point(p);

		if (p->p3_flags & PF_OVERFLOW)
		{
			Int3();		//should not overflow after clip
			return;// 255;
		}
	}

	texmap_instance.DrawTMap(bm, nv, bufptr);

	//return 0;	//say it drew*/
}

int checkmuldiv(fix* r, fix a, fix b, fix c);
//draw a sortof sphere - i.e., the 2d radius is proportional to the 3d
//radius, but not to the distance from the eye
void G3Drawer::draw_sphere(g3s_point* pnt, fix rad)
{
	/*if (!(pnt->p3_codes & CC_BEHIND))
	{
		if (!(pnt->p3_flags & PF_PROJECTED))
			project_point(pnt);

		if (!(pnt->p3_codes & PF_OVERFLOW))
		{
			fix r2, t;

			r2 = fixmul(rad, Matrix_scale.x);
			if (checkmuldiv(&t, r2, Canv_w2, pnt->p3_z))
				return gr_disk(pnt->p3_sx, pnt->p3_sy, t);
		}
	}*/
}

//like g3_draw_poly(), but checks to see if facing.  If surface normal is
//NULL, this routine must compute it, which will be slow.  It is better to 
//pre-compute the normal, and pass it to this function.  When the normal
//is passed, this function works like g3_check_normal_facing() plus
//g3_draw_poly().
//returns -1 if not facing, 1 if off screen, 0 if drew
dbool G3Instance::check_and_draw_poly(int nv, g3s_point** pointlist, vms_vector* norm, vms_vector* pnt)
{
	if (do_facing_check(norm, pointlist, pnt))
		return draw_poly(nv, pointlist);
	else
		return 255;
}

dbool G3Instance::check_and_draw_tmap(int nv, g3s_point** pointlist, g3s_uvl* uvl_list, grs_bitmap* bm, vms_vector* norm, vms_vector* pnt)
{
	if (do_facing_check(norm, pointlist, pnt))
		return draw_tmap(nv, pointlist, uvl_list, bm);
	else
		return 255;
}

bool G3Drawer::must_clip_line(g3s_point* p0, g3s_point* p1, uint8_t codes_or, int color)
{
	bool ret;

	if ((p0->p3_flags & PF_TEMP_POINT) || (p1->p3_flags & PF_TEMP_POINT))
		ret = 0;		//line has already been clipped, so give up

	else
	{
		clip_line(&p0, &p1, codes_or);

		ret = draw_line_direct(p0, p1, color);
		//don't recursively call the line drawer, this will cause recursive clipping. 
		//ret = (bool)gr_line_explicit_clip(canv, color, p0->p3_sx, p0->p3_sy, p1->p3_sx, p1->p3_sy, window_left, window_top, window_right - 1, window_bottom - 1);
	}

	//free temp points

	if (p0->p3_flags & PF_TEMP_POINT)
		free_temp_point(p0);

	if (p1->p3_flags & PF_TEMP_POINT)
		free_temp_point(p1);

	return ret;
}

//draws a line. takes two points.
bool G3Drawer::draw_line(g3s_point* p0, g3s_point* p1, int color)
{
	g3s_point p0l = *p0; //need to copy points too because of leaky state...
	g3s_point p1l = *p1;

	code_point(&p0l); code_point(&p1l);

	if (p0l.p3_codes & p1l.p3_codes)
		return 0;

	uint8_t codes_or = p0l.p3_codes | p1l.p3_codes;

	if (codes_or & CC_BEHIND)
		return must_clip_line(&p0l, &p1l, codes_or, color);

	if (!(p0l.p3_flags & PF_PROJECTED))
		project_point(&p0l);

	if (p0l.p3_flags & PF_OVERFLOW)
		return must_clip_line(&p0l, &p1l , codes_or, color);

	if (!(p1l.p3_flags & PF_PROJECTED))
		project_point(&p1l);

	if (p1l.p3_flags & PF_OVERFLOW)
		return must_clip_line(&p0l, &p1l, codes_or, color);

	return (bool)gr_line_explicit_clip(canv, color, p0l.p3_sx, p0l.p3_sy, p1l.p3_sx, p1l.p3_sy, window_left, window_top, window_right, window_bottom);
}

bool G3Drawer::draw_line_direct(g3s_point* p0, g3s_point* p1, int color)
{
	if (p0->p3_codes & p1->p3_codes)
		return 0;

	uint8_t codes_or = p0->p3_codes | p1->p3_codes;

	if (codes_or & CC_BEHIND)
		//return false; //temporary hack because the clipper doesn't handle clip against near.
		return must_clip_line(p0, p1, codes_or, color);

	if (!(p0->p3_flags & PF_PROJECTED))
		project_point(p0);

	if (p0->p3_flags & PF_OVERFLOW)
		return must_clip_line(p0, p1, codes_or, color);

	if (!(p1->p3_flags & PF_PROJECTED))
		project_point(p1);

	if (p1->p3_flags & PF_OVERFLOW)
		return must_clip_line(p0, p1, codes_or, color);

	return (bool)gr_line_explicit_clip(canv, color, p0->p3_sx, p0->p3_sy, p1->p3_sx, p1->p3_sy, window_left, window_top, window_right, window_bottom);
}


//draws a line. takes two points.  returns true if drew
dbool g3_draw_line(g3s_point* p0, g3s_point* p1)
{
	return g3_global_inst.draw_line(p0, p1);
}

//returns true if a plane is facing the viewer. takes the unrotated surface 
//normal of the plane, and a point on it.  The normal need not be normalized
dbool g3_check_normal_facing(vms_vector* v, vms_vector* norm)
{
	return g3_global_inst.check_normal_facing(v, norm);
}

//like g3_draw_poly(), but checks to see if facing.  If surface normal is
//NULL, this routine must compute it, which will be slow.  It is better to 
//pre-compute the normal, and pass it to this function.  When the normal
//is passed, this function works like g3_check_normal_facing() plus
//g3_draw_poly().
//returns -1 if not facing, 1 if off screen, 0 if drew
dbool g3_check_and_draw_poly(int nv, g3s_point** pointlist, vms_vector* norm, vms_vector* pnt)
{
	return g3_global_inst.check_and_draw_poly(nv, pointlist, norm, pnt);
}

dbool g3_check_and_draw_tmap(int nv, g3s_point** pointlist, g3s_uvl* uvl_list, grs_bitmap* bm, vms_vector* norm, vms_vector* pnt)
{
	return g3_global_inst.check_and_draw_tmap(nv, pointlist, uvl_list, bm, norm, pnt);
}

//draw a flat-shaded face.
//returns 1 if off screen, 0 if drew
dbool g3_draw_poly(int nv, g3s_point** pointlist)
{
	return g3_global_inst.draw_poly(nv, pointlist);
}

//draw a texture-mapped face.
//returns 1 if off screen, 0 if drew
dbool g3_draw_tmap(int nv, g3s_point** pointlist, g3s_uvl* uvl_list, grs_bitmap* bm)
{
	return g3_global_inst.draw_tmap(nv, pointlist, uvl_list, bm);
}

//draw a sortof sphere - i.e., the 2d radius is proportional to the 3d
//radius, but not to the distance from the eye
int g3_draw_sphere(g3s_point* pnt, fix rad)
{
	return g3_global_inst.draw_sphere(pnt, rad);
}

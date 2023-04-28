/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/

#include <stdlib.h>
#include "commandbuffer.h"
#include "misc/error.h"
#include "threading.h"

G3CommandBuffer g3_command_buffer;

void* G3CommandBuffer::reserve_space(size_t size)
{
	if (SIZE_MAX - size < used_size)
	{
		Error("G3CommandBuffer::reserve_space would wrap used_size, which means something seriously bad is happening");
	}

	if ((used_size + size) > max_size)
	{
		//not actually thread safe when both consumption and production happen at the same time. This should probably chain multiple allocations together instead, then. 
		/*max_size *= 2;
		void* newbuffer = realloc(buffer, max_size);
		if (!newbuffer)
			Error("G3CommandBuffer::reserve_space failed to reallocate buffer");

		buffer = newbuffer;
		current = ((uint8_t*)buffer) + used_size;*/
		Error("G3CommandBuffer::reserve_space buffer is not large enough");
	}

	void* rtr = current;
	used_size += size;

	current = ((uint8_t*)current) + size;

	return rtr;
}

G3CommandBuffer::G3CommandBuffer()
{
	used_size = 0; max_size = 1024ull * 1024 * 8;
	buffer = malloc(max_size);
	current = buffer;
	finalized_size = 0;
	is_recording = false;
}

void G3CommandBuffer::start_recording()
{
	current = buffer;
	used_size = 0;
	finalized_size = 0;
	is_recording = true;
}

void G3CommandBuffer::end_recording()
{
	is_recording = false;
}

void G3CommandBuffer::cmd_draw_poly(int nv, g3s_point** pointlist)
{
	size_t cmd_size = sizeof(G3CmdDrawFlatPolygon) + nv * sizeof(g3s_point);

	G3CmdDrawFlatPolygon* cmd = (G3CmdDrawFlatPolygon*)reserve_space(cmd_size);
	cmd->base.type = G3CmdType::draw_poly;
	cmd->base.length = cmd_size;
	cmd->nv = nv;
	cmd->color = grd_curcanv->cv_color;

	g3s_point* points = (g3s_point*)(((uint8_t*)cmd) + sizeof(G3CmdDrawFlatPolygon));
	cmd->pointlist = points;

	for (int i = 0; i < nv; i++)
	{
		points[i] = *pointlist[i];
	}

	commit_space();
}

void G3CommandBuffer::cmd_draw_tmap(int nv, g3s_point** pointlist, g3s_uvl* uvl_list, grs_bitmap* bm)
{
	size_t cmd_size = sizeof(G3CmdDrawTmapPolygon) + nv * sizeof(g3s_point) + nv * sizeof(g3s_uvl);

	G3CmdDrawTmapPolygon* cmd = (G3CmdDrawTmapPolygon*)reserve_space(cmd_size);
	cmd->base.type = G3CmdType::draw_tmap;
	cmd->base.length = cmd_size;
	cmd->nv = nv;
	cmd->bm = bm;

	g3s_point* points = (g3s_point*)(((uint8_t*)cmd) + sizeof(G3CmdDrawTmapPolygon));
	g3s_uvl* uvs = (g3s_uvl*)(((uint8_t*)cmd) + sizeof(G3CmdDrawTmapPolygon) + nv * sizeof(g3s_point));
	cmd->pointlist = points; cmd->uvllist = uvs;

	for (int i = 0; i < nv; i++)
	{
		points[i] = *pointlist[i];
		uvs[i] = uvl_list[i];
	}

	commit_space();
}

void G3CommandBuffer::cmd_draw_sphere(g3s_point* pnt, fix rad)
{
	size_t cmd_size = sizeof(G3CmdDrawSphere);

	G3CmdDrawSphere* cmd = (G3CmdDrawSphere*)reserve_space(cmd_size);
	cmd->base.type = G3CmdType::draw_sphere;
	cmd->base.length = cmd_size;
	cmd->point = *pnt;
	cmd->radius = rad;

	commit_space();
}

void G3CommandBuffer::cmd_draw_line(g3s_point* p0, g3s_point* p1)
{
}

void G3CommandBuffer::cmd_draw_bitmap(g3s_point* pnt, fix width, fix height, grs_bitmap* bm, int orientation)
{
	size_t cmd_size = sizeof(G3CmdDrawBitmap);

	G3CmdDrawBitmap* cmd = (G3CmdDrawBitmap*)reserve_space(cmd_size);
	cmd->base.type = G3CmdType::draw_bitmap;
	cmd->base.length = cmd_size;
	cmd->point = *pnt;
	cmd->w = width; cmd->h = height;
	cmd->bm = bm;
	cmd->orientation = orientation;

	commit_space();
}

void G3CommandBuffer::cmd_set_lighting(int lighting)
{
	size_t cmd_size = sizeof(G3CmdVarSet);

	G3CmdVarSet* cmd = (G3CmdVarSet*)reserve_space(cmd_size);
	cmd->base.type = G3CmdType::set_lighting;
	cmd->base.length = cmd_size;
	cmd->value = lighting;

	commit_space();
}

void G3CommandBuffer::cmd_set_interpolation(int mode)
{
	size_t cmd_size = sizeof(G3CmdVarSet);

	G3CmdVarSet* cmd = (G3CmdVarSet*)reserve_space(cmd_size);
	cmd->base.type = G3CmdType::set_interpolation;
	cmd->base.length = cmd_size;
	cmd->value = mode;

	commit_space();
}

void G3CommandBuffer::cmd_set_transparency(int mode)
{
	size_t cmd_size = sizeof(G3CmdVarSet);

	G3CmdVarSet* cmd = (G3CmdVarSet*)reserve_space(cmd_size);
	cmd->base.type = G3CmdType::set_transparency;
	cmd->base.length = cmd_size;
	cmd->value = mode;

	commit_space();
}

void G3CommandBuffer::cmd_set_seg_depth(int depth)
{
	size_t cmd_size = sizeof(G3CmdVarSet);

	G3CmdVarSet* cmd = (G3CmdVarSet*)reserve_space(cmd_size);
	cmd->base.type = G3CmdType::set_tmap_seg_depth;
	cmd->base.length = cmd_size;
	cmd->value = depth;

	commit_space();
}

void G3CommandBuffer::cmd_set_window(int left, int top, int right, int bottom)
{
	size_t cmd_size = sizeof(G3CmdWindowSet);

	G3CmdWindowSet* cmd = (G3CmdWindowSet*)reserve_space(cmd_size);
	cmd->base.type = G3CmdType::set_tmap_clip_window;
	cmd->base.length = cmd_size;
	cmd->left = left;
	cmd->top = top;
	cmd->right = right;
	cmd->bottom = bottom;

	commit_space();
}

void G3Instance::set_lighting_mode(int new_mode)
{
	if (Use_multithread) 
		g3_command_buffer.cmd_set_lighting(new_mode);
	//These commands need to also set the local state even when threading is on, since outside forces may read it back
	drawer.get_texmap_instance().SetLightingState(new_mode);
}

void G3Instance::set_interpolation_mode(int new_mode)
{
	if (Use_multithread)
		g3_command_buffer.cmd_set_interpolation(new_mode);
	
	drawer.get_texmap_instance().SetInterpolation(new_mode);
}

void G3Instance::set_segment_depth(int depth)
{
	if (Use_multithread)
		g3_command_buffer.cmd_set_seg_depth(depth);

	drawer.get_texmap_instance().SetSegmentDepth(depth);
}

void G3Instance::set_clip_window(int left, int top, int right, int bottom)
{
	if (Use_multithread)
		g3_command_buffer.cmd_set_window(left, top, right, bottom);

	drawer.get_texmap_instance().SetClipWindow(left, top, right, bottom);
}

dbool G3Instance::draw_poly(int nv, g3s_point** pointlist)
{
	if (Use_multithread)
		g3_command_buffer.cmd_draw_poly(nv, pointlist);
	else
		drawer.draw_poly(nv, pointlist, grd_curcanv->cv_color);
	return 0;
}

dbool G3Instance::draw_tmap(int nv, g3s_point** pointlist, g3s_uvl* uvl_list, grs_bitmap* bm)
{
	if (Use_multithread)
		g3_command_buffer.cmd_draw_tmap(nv, pointlist, uvl_list, bm);
	else
		drawer.draw_tmap(nv, pointlist, uvl_list, bm);
	return 0;
}

dbool G3Instance::draw_line(g3s_point* p0, g3s_point* p1)
{
	if (Use_multithread)
		g3_command_buffer.cmd_draw_line(p0, p1);
	else
		drawer.draw_line(p0, p1);
	return 0;
}

int G3Instance::draw_sphere(g3s_point* pnt, fix rad)
{
	if (Use_multithread)
		g3_command_buffer.cmd_draw_sphere(pnt, rad);
	else
		drawer.draw_sphere(pnt, rad);
	return 0;
}

int checkmuldiv(fix* r, fix a, fix b, fix c);
dbool G3Instance::draw_bitmap(vms_vector* pos, fix width, fix height, grs_bitmap* bm, int orientation)
{
	g3s_point pnt;
	fix t, w, h;

	if (rotate_point(&pnt, pos) & CC_BEHIND)
		return 1;

	project_point(&pnt);

	if (pnt.p3_flags & PF_OVERFLOW)
		return 1;

	if (checkmuldiv(&t, width, Canv_w2, pnt.p3_z))
		w = fixmul(t, Matrix_scale.x);
	else
		return 1;

	if (checkmuldiv(&t, height, Canv_h2, pnt.p3_z))
		h = fixmul(t, Matrix_scale.y);
	else
		return 1;

	if (Use_multithread)
		g3_command_buffer.cmd_draw_bitmap(&pnt, w, h, bm, orientation);
	else
		drawer.draw_bitmap(&pnt, w, h, bm, orientation);

	return 0;
}

void G3Drawer::decode_command_buffer()
{
	size_t decode_cursor = 0;
	size_t buffer_size = g3_command_buffer.get_finalized_size();

	while (g3_command_buffer.recording() || decode_cursor < buffer_size)
	{
		buffer_size = g3_command_buffer.get_finalized_size();
		while (decode_cursor < buffer_size)
		{
			G3CmdBase* cmd_p = (G3CmdBase*)(((uint8_t*)g3_command_buffer.get_buffer()) + decode_cursor);

			switch (cmd_p->type)
			{
			case G3CmdType::draw_poly:
			{
				G3CmdDrawFlatPolygon* draw_p = (G3CmdDrawFlatPolygon*)cmd_p;
				draw_poly_direct(draw_p->nv, draw_p->pointlist, draw_p->color);
			}
			break;
			case G3CmdType::draw_tmap:
			{
				G3CmdDrawTmapPolygon* draw_p = (G3CmdDrawTmapPolygon*)cmd_p;
				draw_tmap_direct(draw_p->nv, draw_p->pointlist, draw_p->uvllist, draw_p->bm);
			}
			break;
			case G3CmdType::draw_bitmap:
			{
				G3CmdDrawBitmap* draw_p = (G3CmdDrawBitmap*)cmd_p;
				draw_bitmap(&draw_p->point, draw_p->w, draw_p->h, draw_p->bm, draw_p->orientation);
			}
			break;
			case G3CmdType::draw_line:
				//TODO
				break;
			case G3CmdType::draw_sphere:
				//TODO
				break;
			case G3CmdType::set_lighting:
			{
				G3CmdVarSet* var_p = (G3CmdVarSet*)cmd_p;
				texmap_instance.SetLightingState(var_p->value);
			}
			break;
			case G3CmdType::set_interpolation:
			{
				G3CmdVarSet* var_p = (G3CmdVarSet*)cmd_p;
				texmap_instance.SetInterpolation(var_p->value);
			}
			break;
			case G3CmdType::set_transparency:
			{
				G3CmdVarSet* var_p = (G3CmdVarSet*)cmd_p;
				texmap_instance.SetTransparency(var_p->value);
			}
			break;
			case G3CmdType::set_tmap_seg_depth:
			{
				G3CmdVarSet* var_p = (G3CmdVarSet*)cmd_p;
				texmap_instance.SetSegmentDepth(var_p->value);
			}
			break;
			case G3CmdType::set_tmap_clip_window:
			{
				G3CmdWindowSet* window_p = (G3CmdWindowSet*)cmd_p;
				set_tmap_clip_window(window_p->left, window_p->top, window_p->right, window_p->bottom);
			}
			break;
			}

			decode_cursor += cmd_p->length;
		}
		buffer_size = g3_command_buffer.get_finalized_size();
	}
	
}

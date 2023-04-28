/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/

#pragma once

#include <atomic>

#include "3d/3d.h"

enum class G3CmdType
{
	draw_poly,
	draw_tmap,
	draw_sphere,
	draw_bitmap,
	draw_line,
	draw_polymodel,
	//the following three all generate a G3CmdVarSet
	set_lighting,
	set_interpolation,
	set_transparency,
	set_tmap_seg_depth,
	set_tmap_clip_window,
};

struct G3CmdBase
{
	G3CmdType type;
	size_t length;
};

struct G3CmdDrawFlatPolygon
{
	G3CmdBase base;
	int nv;
	//The following pointer will point to data following this command. The size of this command plus the size of this buffer is covered by base.length;
	g3s_point* pointlist;
	uint16_t color;
};

struct G3CmdDrawTmapPolygon
{
	G3CmdBase base;
	int nv;
	//The following two pointers will point to data following this command. The size of this command plus the size of these buffers is covered by base.length;
	g3s_point* pointlist;
	g3s_uvl* uvllist;
	grs_bitmap* bm;
};

struct G3CmdDrawSphere
{
	G3CmdBase base;
	g3s_point point;
	fix radius;
};

struct G3CmdDrawBitmap
{
	G3CmdBase base;
	g3s_point point;
	fix w, h;
	grs_bitmap* bm;
	int orientation;
};

struct G3CmdVarSet
{
	G3CmdBase base;
	int value;
};

struct G3CmdWindowSet
{
	G3CmdBase base;
	int left, top, right, bottom;
};

class G3CommandBuffer
{
	void* buffer, * current;
	size_t used_size, max_size;
	std::atomic_bool is_recording;
	std::atomic_size_t finalized_size;

	void* reserve_space(size_t size);
	void commit_space()
	{
		finalized_size = used_size;
	}
public:
	G3CommandBuffer();

	void start_recording();

	//Records a solid color polygon draw command
	void cmd_draw_poly(int nv, g3s_point** pointlist);
	//Records a texture mapped polygon draw command
	void cmd_draw_tmap(int nv, g3s_point** pointlist, g3s_uvl* uvl_list, grs_bitmap* bm);
	//Records a sphere draw command
	void cmd_draw_sphere(g3s_point* pnt, fix rad);
	//Records a line draw command
	void cmd_draw_line(g3s_point* p0, g3s_point* p1);
	//Records a bitmap draw command
	void cmd_draw_bitmap(g3s_point* pnt, fix width, fix height, grs_bitmap* bm, int orientation);

	//Variable setting commands. These will change the state of the texmapper when decoded
	//Records a texture mapper lighting change. 0 is no lighting, 1 is lighting, and 2 is "editor" lighting, for pick tests.
	void cmd_set_lighting(int lighting);
	//Records a texture mapper interpolation change.
	void cmd_set_interpolation(int mode);
	//Records a transparency state change.
	void cmd_set_transparency(int mode);
	//Records a segment depth change. Segment depth is used to transition the texmapper from perspective-correct to affine interpolation, to solid color rendering.
	void cmd_set_seg_depth(int depth);
	//Records a change to the clipping window for the texture mapper
	void cmd_set_window(int left, int top, int right, int bottom);

	void end_recording();

	//Threads will be executing the command buffer while it's being recorded, so drawing can proceed while other operations like rotation are occurring.
	//This is using an atomic to hopefully ensure proper memory ordering? I'm not entirely sure here. Ordering may need to be explicitly specified for ARM systems.
	std::atomic_size_t& get_finalized_size()
	{
		return finalized_size;
	}

	bool recording() const
	{
		return is_recording;
	}

	void* get_buffer()
	{
		return buffer;
	}
};

extern G3CommandBuffer g3_command_buffer;

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
#include <string>
#include <format>
#include <stdio.h>
#include <algorithm>
#include "misc/rand.h"
#include "cfile/cfile.h"
#include "inferno.h"
#include "game.h"
#include "2d/gr.h"
#include "stdlib.h"
#include "bm.h"
#include "platform/mono.h"
#include "3d/3d.h"
#include "segment.h"
#include "laser.h"
#include "platform/key.h"
#include "gameseg.h"
#include "textures.h"
#include "texmap/texmap.h"
#include "object.h"
#include "physics.h"
#include "slew.h"		
#include "render.h"
#include "wall.h"
#include "vclip.h"
#include "polyobj.h"
#include "fireball.h"
#include "laser.h"
#include "misc/error.h"
#include "ai.h"
#include "hostage.h"
#include "morph.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "fuelcen.h"
#include "sounds.h"
#include "collide.h"
#include "lighting.h"
#include "newdemo.h"
#include "player.h"
#include "weapon.h"
#include "network.h"
#include "newmenu.h"
#include "gauges.h"
#ifdef ARCADE
#include "arcade.h"
#endif
#include "multi.h"
#include "menu.h"
#include "misc/args.h"
#include "stringtable.h"
#include "main_shared/piggy.h"
#include "gameseq.h"

#ifdef EDITOR
#include "editor\editor.h"
#endif

/*
 *  Global variables
 */

uint8_t CollisionResult[MAX_OBJECT_TYPES][MAX_OBJECT_TYPES];

object* ConsoleObject;					//the object that is the player

static short free_obj_list[MAX_OBJECTS];

//Data for objects

// -- Object stuff

//info on the various types of objects
#ifndef NDEBUG
object	Object_minus_one;
#endif

object Objects[MAX_OBJECTS];
int num_objects = 0;
int Highest_object_index = 0;
int Highest_ever_object_index = 0;

int print_object_info = 0;

//	List of objects rendered last frame in order.  Created at render time, used by homing missiles in laser.c
short Ordered_rendered_object_list[MAX_RENDERED_OBJECTS];
int	Num_rendered_objects = 0;

int Debug_objnum = -1;

#ifndef NDEBUG
char	Object_type_names[MAX_OBJECT_TYPES][9] = 
{
	"WALL    ",
	"FIREBALL",
	"ROBOT   ",
	"HOSTAGE ",
	"PLAYER  ",
	"WEAPON  ",
	"CAMERA  ",
	"POWERUP ",
	"DEBRIS  ",
	"CNTRLCEN",
	"FLARE   ",
	"CLUTTER ",
	"GHOST   ",
	"LIGHT   ",
	"COOP    ",
};
#endif

#ifndef RELEASE
//set viewer object to next object in array
void object_goto_next_viewer()
{
	int start_obj = Viewer - Objects;		//get viewer object number

	for (int i = 0; i <= Highest_object_index; i++) 
	{
		start_obj++;
		if (start_obj > Highest_object_index) start_obj = 0;

		if (Objects[start_obj].type != OBJ_NONE) 
		{
			Viewer = &Objects[start_obj];
			return;
		}
	}

	Error("Couldn't find a viewer object!");
}
#endif

//draw an object that has one bitmap & doesn't rotate
void draw_object_blob(object* obj, bitmap_index bmi)
{
	grs_bitmap* bm = &GameBitmaps[bmi.index];
	PIGGY_PAGE_IN(bmi);

	if (bm->bm_w > bm->bm_h)
		g3_draw_bitmap(&obj->pos, obj->size, fixmuldiv(obj->size, bm->bm_h, bm->bm_w), bm);
	else
		g3_draw_bitmap(&obj->pos, fixmuldiv(obj->size, bm->bm_w, bm->bm_h), obj->size, bm);
}

//draw an object that is a texture-mapped rod
void draw_object_tmap_rod(object* obj, bitmap_index bitmapi, int lighted)
{
	grs_bitmap* bitmap = &GameBitmaps[bitmapi.index];
	fix light;

	vms_vector delta, top_v, bot_v;
	g3s_point top_p, bot_p;

	PIGGY_PAGE_IN(bitmapi);

	vm_vec_copy_scale(&delta, &obj->orient.uvec, obj->size);

	vm_vec_add(&top_v, &obj->pos, &delta);
	vm_vec_sub(&bot_v, &obj->pos, &delta);

	g3_rotate_point(&top_p, &top_v);
	g3_rotate_point(&bot_p, &bot_v);

	if (lighted)
		light = compute_object_light(obj, &top_p.p3_vec);
	else
		light = f1_0;

	g3_draw_rod_tmap(bitmap, &bot_p, obj->size, &top_p, obj->size, light);
}

int	Linear_tmap_polygon_objects = 1;

extern fix Max_thrust;

//used for robot engine glow
#define MAX_VELOCITY i2f(50)

//what darkening level to use when cloaked
#define CLOAKED_FADE_LEVEL		28

#define	CLOAK_FADEIN_DURATION_PLAYER	F2_0
#define	CLOAK_FADEOUT_DURATION_PLAYER	F2_0

#define	CLOAK_FADEIN_DURATION_ROBOT	F1_0
#define	CLOAK_FADEOUT_DURATION_ROBOT	F1_0

fix	Cloak_fadein_duration;
fix	Cloak_fadeout_duration;

static int cloak_delta = 0, cloak_dir = 1;
static fix cloak_timer = 0;

void object_update_cloak_pulse()
{
	cloak_timer -= FrameTime;
}

//do special cloaked render
void draw_cloaked_object(object* obj, fix light, fix* glow, fix cloak_start_time, fix cloak_end_time, bitmap_index* alt_textures)
{
	fix cloak_delta_time, total_cloaked_time;
	fix light_scale;
	int cloak_value;
	bool fading = false;		//if true, fading, else cloaking

	total_cloaked_time = cloak_end_time - cloak_start_time;

	switch (obj->type) 
	{
	case OBJ_PLAYER:
		Cloak_fadein_duration = CLOAK_FADEIN_DURATION_PLAYER;
		Cloak_fadeout_duration = CLOAK_FADEOUT_DURATION_PLAYER;
		break;
	case OBJ_ROBOT:
		Cloak_fadein_duration = CLOAK_FADEIN_DURATION_ROBOT;
		Cloak_fadeout_duration = CLOAK_FADEOUT_DURATION_ROBOT;
		break;
	default:
		Int3();		//	Contact Mike: Unexpected object type in draw_cloaked_object.
	}

	cloak_delta_time = GameTime - cloak_start_time;

	if (cloak_delta_time < Cloak_fadein_duration / 2)
	{
		light_scale = Cloak_fadein_duration / 2 - cloak_delta_time;
		fading = true;
	}
	else if (cloak_delta_time < Cloak_fadein_duration) 
	{
		cloak_value = f2i((cloak_delta_time - Cloak_fadein_duration / 2) * CLOAKED_FADE_LEVEL);
	}
	else if (GameTime < cloak_end_time - Cloak_fadeout_duration) 
	{
		while (cloak_timer < 0)
		{
			cloak_timer += Cloak_fadeout_duration / 12;

			cloak_delta += cloak_dir;

			if (cloak_delta == 0 || cloak_delta == 4)
				cloak_dir = -cloak_dir;
		}

		cloak_value = CLOAKED_FADE_LEVEL - cloak_delta;
	}
	else if (GameTime < cloak_end_time - Cloak_fadeout_duration / 2) 
	{
		cloak_value = f2i((total_cloaked_time - Cloak_fadeout_duration / 2 - cloak_delta_time) * CLOAKED_FADE_LEVEL);
	}
	else 
	{
		light_scale = Cloak_fadeout_duration / 2 - (total_cloaked_time - cloak_delta_time);
		fading = true;
	}

	if (fading) 
	{
		fix new_light = fixmul(light, light_scale);
		fix new_glow = fixmul(*glow, light_scale);
		draw_polygon_model(&obj->pos, &obj->orient, &obj->rtype.pobj_info.anim_angles[0], obj->rtype.pobj_info.model_num, obj->rtype.pobj_info.subobj_flags, new_light, &new_glow, alt_textures);
	}
	else 
	{
		g3_set_darkening_level(cloak_value);
		draw_polygon_model(&obj->pos, &obj->orient, &obj->rtype.pobj_info.anim_angles[0], obj->rtype.pobj_info.model_num, obj->rtype.pobj_info.subobj_flags, light, glow, alt_textures);
		g3_set_darkening_level(GR_FADE_LEVELS);
	}
}

//draw an object which renders as a polygon model
void draw_polygon_object(object* obj)
{
	fix light = compute_object_light(obj, NULL);

	int imsave = g3_get_interpolation_mode();
	if (Linear_tmap_polygon_objects)
		g3_set_interpolation_mode(1);

	//set engine glow value
	fix engine_glow_value = f1_0 / 5;
	if (obj->movement_type == MT_PHYSICS) 
	{
		if (obj->mtype.phys_info.flags & PF_USES_THRUST && obj->type == OBJ_PLAYER && obj->id == Player_num) 
		{
			fix thrust_mag = vm_vec_mag_quick(&obj->mtype.phys_info.thrust);
			engine_glow_value += (fixdiv(thrust_mag, Player_ship->max_thrust) * 4) / 5;
		}
		else
		{
			fix speed = vm_vec_mag_quick(&obj->mtype.phys_info.velocity);
			engine_glow_value += (fixdiv(speed, MAX_VELOCITY) * 4) / 5;
		}
	}

	if (obj->rtype.pobj_info.tmap_override != -1) 
	{
		polymodel* pm = &Polygon_models[obj->rtype.pobj_info.model_num];
		bitmap_index bm_ptrs[10];

		Assert(pm->n_textures <= 10);

		for (int i = 0; i < pm->n_textures; i++)
			bm_ptrs[i] = Textures[obj->rtype.pobj_info.tmap_override];

		draw_polygon_model(&obj->pos, &obj->orient, &obj->rtype.pobj_info.anim_angles[0], obj->rtype.pobj_info.model_num, obj->rtype.pobj_info.subobj_flags, light, &engine_glow_value, bm_ptrs);
	}
	else 
	{
		bitmap_index* alt_textures = NULL;

#ifdef NETWORK
		if (obj->rtype.pobj_info.alt_textures > 0)
			alt_textures = multi_player_textures[obj->rtype.pobj_info.alt_textures - 1];
#endif

		if (obj->type == OBJ_PLAYER && (Players[obj->id].flags & PLAYER_FLAGS_CLOAKED))
			draw_cloaked_object(obj, light, &engine_glow_value, Players[obj->id].cloak_time, Players[obj->id].cloak_time + CLOAK_TIME_MAX, alt_textures);
		else if ((obj->type == OBJ_ROBOT) && (obj->ctype.ai_info.CLOAKED)) 
		{
			if (Robot_info[obj->id].boss_flag)
				draw_cloaked_object(obj, light, &engine_glow_value, Boss_cloak_start_time, Boss_cloak_end_time, alt_textures);
			else
				draw_cloaked_object(obj, light, &engine_glow_value, GameTime - F1_0 * 10, GameTime + F1_0 * 10, alt_textures);
		}
		else 
		{
			draw_polygon_model(&obj->pos, &obj->orient, &obj->rtype.pobj_info.anim_angles[0], obj->rtype.pobj_info.model_num, obj->rtype.pobj_info.subobj_flags, light, &engine_glow_value, alt_textures);
			if (obj->type == OBJ_WEAPON && (Weapon_info[obj->id].model_num_inner > -1))
			{
				fix dist_to_eye = vm_vec_dist_quick(&Viewer->pos, &obj->pos);
				if (dist_to_eye < Simple_model_threshhold_scale * F1_0 * 2)
					draw_polygon_model(&obj->pos, &obj->orient, &obj->rtype.pobj_info.anim_angles[0], Weapon_info[obj->id].model_num_inner, obj->rtype.pobj_info.subobj_flags, light, &engine_glow_value, alt_textures);
			}
		}
	}

	g3_set_interpolation_mode(imsave);
}

int	Player_fired_laser_this_frame = -1;

// -----------------------------------------------------------------------------
//this routine checks to see if an robot rendered near the middle of
//the screen, and if so and the player had fired, "warns" the robot
void set_robot_location_info(object* objp)
{
	if (Player_fired_laser_this_frame != -1) 
	{
		g3s_point temp;
		g3_rotate_point(&temp, &objp->pos);

		if (temp.p3_codes & CC_BEHIND)		//robot behind the screen
			return;

		//the code below to check for object near the center of the screen
		//completely ignores z, which may not be good
		if ((abs(temp.p3_x) < F1_0 * 4) && (abs(temp.p3_y) < F1_0 * 4)) 
		{
			objp->ctype.ai_info.danger_laser_num = Player_fired_laser_this_frame;
			objp->ctype.ai_info.danger_laser_signature = Objects[Player_fired_laser_this_frame].signature;
		}
	}
}

//	------------------------------------------------------------------------------------------------------------------
void create_small_fireball_on_object(object* objp, fix size_scale, int sound_flag)
{
	fix			size;
	vms_vector	pos, rand_vec;
	int			segnum;

	pos = objp->pos;
	make_random_vector(&rand_vec);

	vm_vec_scale(&rand_vec, objp->size / 2);

	vm_vec_add2(&pos, &rand_vec);

	size = fixmul(size_scale, F1_0 + P_Rand() * 4);

	segnum = find_point_seg(&pos, objp->segnum);
	if (segnum != -1) 
	{
		object* expl_obj;
		expl_obj = object_create_explosion(segnum, &pos, size, VCLIP_SMALL_EXPLOSION);
		if (!expl_obj)
			return;
		obj_attach(objp, expl_obj);
		if (P_Rand() < 8192) 
		{
			fix	vol = F1_0 / 2;
			if (objp->type == OBJ_ROBOT)
				vol *= 2;
			else if (sound_flag)
				digi_link_sound_to_object(SOUND_EXPLODING_WALL, objp - Objects, 0, vol);
		}
	}
}

//	------------------------------------------------------------------------------------------------------------------
void create_vclip_on_object(object* objp, fix size_scale, int vclip_num)
{
	vms_vector	rand_vec;

	vms_vector pos = objp->pos;
	make_random_vector(&rand_vec);

	vm_vec_scale(&rand_vec, objp->size / 2);

	vm_vec_add2(&pos, &rand_vec);

	fix size = fixmul(size_scale, F1_0 + P_Rand() * 4);

	int segnum = find_point_seg(&pos, objp->segnum);
	if (segnum != -1) 
	{
		object* expl_obj;
		expl_obj = object_create_explosion(segnum, &pos, size, vclip_num);
		if (!expl_obj)
			return;

		expl_obj->movement_type = MT_PHYSICS;
		expl_obj->mtype.phys_info.velocity.x = objp->mtype.phys_info.velocity.x / 2;
		expl_obj->mtype.phys_info.velocity.y = objp->mtype.phys_info.velocity.y / 2;
		expl_obj->mtype.phys_info.velocity.z = objp->mtype.phys_info.velocity.z / 2;
	}
}

// -----------------------------------------------------------------------------
//	Render an object.  Calls one of several routines based on type
void render_object(object* obj)
{
	if (obj == Viewer) return;

	if (obj->type == OBJ_NONE) 
	{
#ifndef NDEBUG
		mprintf((1, "ERROR!!!! Bogus obj %d in seg %d is rendering!\n", obj - Objects, obj->segnum));
		Int3();
#endif
		return;
	}

	int mld_save = Max_linear_depth;
	Max_linear_depth = Max_linear_depth_objects;

	switch (obj->render_type) 
	{

	case RT_NONE:	break;		//doesn't render, like the player

	case RT_POLYOBJ:

		draw_polygon_object(obj);

		//"warn" robot if being shot at
		if (obj->type == OBJ_ROBOT)
			set_robot_location_info(obj);

		break;

	case RT_MORPH:	draw_morph_object(obj); break;

	case RT_FIREBALL: draw_fireball(obj); break;

	case RT_WEAPON_VCLIP: draw_weapon_vclip(obj); break;

	case RT_HOSTAGE: draw_hostage(obj); break;

	case RT_POWERUP: draw_powerup(obj); break;

	case RT_LASER: Laser_render(obj); break;

	default: Error("Unknown render_type <%d>", obj->render_type);
	}

	if (obj->render_type != RT_NONE)
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_render_object(obj);

	Max_linear_depth = mld_save;

}

void check_and_fix_matrix(vms_matrix* m);

#define vm_angvec_zero(v) (v)->p=(v)->b=(v)->h=0

void reset_player_object()
{
	//Init physics
	vm_vec_zero(&ConsoleObject->mtype.phys_info.velocity);
	vm_vec_zero(&ConsoleObject->mtype.phys_info.thrust);
	vm_vec_zero(&ConsoleObject->mtype.phys_info.rotvel);
	vm_vec_zero(&ConsoleObject->mtype.phys_info.rotthrust);
	ConsoleObject->mtype.phys_info.brakes = ConsoleObject->mtype.phys_info.turnroll = 0;
	ConsoleObject->mtype.phys_info.mass = Player_ship->mass;
	ConsoleObject->mtype.phys_info.drag = Player_ship->drag;
	ConsoleObject->mtype.phys_info.flags |= PF_TURNROLL | PF_LEVELLING | PF_WIGGLE | PF_USES_THRUST;

	//Init render info
	ConsoleObject->render_type = RT_POLYOBJ;
	ConsoleObject->rtype.pobj_info.model_num = Player_ship->model_num;		//what model is this?
	ConsoleObject->rtype.pobj_info.subobj_flags = 0;		//zero the flags
	ConsoleObject->rtype.pobj_info.tmap_override = -1;		//no tmap override!

	for (int i = 0; i < MAX_SUBMODELS; i++)
		vm_angvec_zero(&ConsoleObject->rtype.pobj_info.anim_angles[i]);

	// Clear misc
	ConsoleObject->flags = 0;
}

//make object0 the player, setting all relevant fields
void init_player_object()
{
	ConsoleObject->type = OBJ_PLAYER;
	ConsoleObject->id = 0;					//no sub-types for player

	ConsoleObject->size = Polygon_models[Player_ship->model_num].rad;

	ConsoleObject->control_type = CT_SLEW;			//default is player slewing
	ConsoleObject->movement_type = MT_PHYSICS;		//change this sometime

	ConsoleObject->lifeleft = IMMORTAL_TIME;

	ConsoleObject->attached_obj = -1;

	reset_player_object();
}

//sets up the free list & init player & whatever else
void init_objects()
{
	collide_init();

	for (int i = 0; i < MAX_OBJECTS; i++) 
	{
		free_obj_list[i] = i;
		Objects[i].type = OBJ_NONE;
		Objects[i].segnum = -1;
	}

	for (int i = 0; i < MAX_SEGMENTS; i++)
		Segments[i].objects = -1;

	ConsoleObject = Viewer = &Objects[0];

	init_player_object();
	obj_link(ConsoleObject - Objects, 0);	//put in the world in segment 0

	num_objects = 1;						//just the player
	Highest_object_index = 0;
}

//after calling init_object(), the network code has grabbed specific
//object slots without allocating them.  Go though the objects & build
//the free list, then set the apporpriate globals
void special_reset_objects(void)
{
	num_objects = MAX_OBJECTS;

	Highest_object_index = 0;
	Assert(Objects[0].type != OBJ_NONE);		//0 should be used

	for (int i = MAX_OBJECTS; i--;)
		if (Objects[i].type == OBJ_NONE)
			free_obj_list[--num_objects] = i;
		else
			if (i > Highest_object_index)
				Highest_object_index = i;
}

#ifndef NDEBUG
int is_object_in_seg(int segnum, int objn)
{
	int count = 0;

	for (int objnum = Segments[segnum].objects; objnum != -1; objnum = Objects[objnum].next) 
	{
		if (count > MAX_OBJECTS)
		{
			Int3();
			return count;
		}
		if (objnum == objn) count++;
	}
	return count;
}

int search_all_segments_for_object(int objnum)
{
	int count = 0;

	for (int i = 0; i <= Highest_segment_index; i++) 
	{
		count += is_object_in_seg(i, objnum);
	}
	return count;
}

void johns_obj_unlink(int segnum, int objnum)
{
	object* obj = &Objects[objnum];
	segment* seg = &Segments[segnum];

	Assert(objnum != -1);

	if (obj->prev == -1)
		seg->objects = obj->next;
	else
		Objects[obj->prev].next = obj->next;

	if (obj->next != -1) Objects[obj->next].prev = obj->prev;
}

void remove_incorrect_objects()
{
	for (int segnum = 0; segnum <= Highest_segment_index; segnum++) 
	{
		int count = 0;
		for (int objnum = Segments[segnum].objects; objnum != -1; objnum = Objects[objnum].next) 
		{
			count++;
#ifndef NDEBUG
			if (count > MAX_OBJECTS)
			{
				mprintf((1, "Object list in segment %d is circular.\n", segnum));
				Int3();
			}
#endif
			if (Objects[objnum].segnum != segnum) 
			{
#ifndef NDEBUG
				mprintf((0, "Removing object %d from segment %d.\n", objnum, segnum));
				Int3();
#endif
				johns_obj_unlink(segnum, objnum);
			}
		}
	}
}

void remove_all_objects_but(int segnum, int objnum)
{
	for (int i = 0; i <= Highest_segment_index; i++) 
	{
		if (segnum != i) 
		{
			if (is_object_in_seg(i, objnum)) 
			{
				johns_obj_unlink(i, objnum);
			}
		}
	}
}

int check_duplicate_objects()
{
	int count = 0;

	for (int i = 0; i <= Highest_object_index; i++) 
	{
		if (Objects[i].type != OBJ_NONE) 
		{
			count = search_all_segments_for_object(i);
			if (count > 1) 
			{
#ifndef NDEBUG
				mprintf((1, "Object %d is in %d segments!\n", i, count));
				Int3();
#endif
				remove_all_objects_but(Objects[i].segnum, i);
				return count;
			}
		}
	}
	return count;
}

void list_seg_objects(int segnum)
{
	int objnum, count = 0;

	for (objnum = Segments[segnum].objects; objnum != -1; objnum = Objects[objnum].next)
	{
		count++;
		if (count > MAX_OBJECTS) 
		{
			Int3();
			return;
		}
	}
	return;
}
#endif

//link the object into the list for its segment
void obj_link(int objnum, int segnum)
{
	object* obj = &Objects[objnum];

	Assert(objnum != -1);

	Assert(obj->segnum == -1);

	Assert(segnum >= 0 && segnum <= Highest_segment_index);

	obj->segnum = segnum;

	obj->next = Segments[segnum].objects;
	obj->prev = -1;

	Segments[segnum].objects = objnum;

	if (obj->next != -1) Objects[obj->next].prev = objnum;

	//list_seg_objects( segnum );
	//check_duplicate_objects();

	Assert(Objects[0].next != 0);
	if (Objects[0].next == 0)
		Objects[0].next = -1;

	Assert(Objects[0].prev != 0);
	if (Objects[0].prev == 0)
		Objects[0].prev = -1;
}

void obj_unlink(int objnum)
{
	object* obj = &Objects[objnum];
	segment* seg = &Segments[obj->segnum];

	Assert(objnum != -1);

	if (obj->prev == -1)
		seg->objects = obj->next;
	else
		Objects[obj->prev].next = obj->next;

	if (obj->next != -1) Objects[obj->next].prev = obj->prev;

	obj->segnum = -1;

	Assert(Objects[0].next != 0);
	Assert(Objects[0].prev != 0);
}

int Object_next_signature = 0;

int Debris_object_count = 0;

int	Unused_object_slots;

//returns the number of a free object, updating Highest_object_index.
//Generally, obj_create() should be called to get an object, since it
//fills in important fields and does the linking.
//returns -1 if no free objects
int obj_allocate(void)
{
	int objnum;

	if (num_objects >= MAX_OBJECTS)
	{
#ifndef NDEBUG
		mprintf((1, "Object creation failed - too many objects!\n"));
#endif
		return -1;
	}

	objnum = free_obj_list[num_objects++];

	if (objnum > Highest_object_index) 
	{
		Highest_object_index = objnum;
		if (Highest_object_index > Highest_ever_object_index)
			Highest_ever_object_index = Highest_object_index;
	}

	Unused_object_slots = 0;
	for (int i = 0; i <= Highest_object_index; i++)
		if (Objects[i].type == OBJ_NONE)
			Unused_object_slots++;

	return objnum;
}

//frees up an object.  Generally, obj_delete() should be called to get
//rid of an object.  This function deallocates the object entry after
//the object has been unlinked
void obj_free(int objnum)
{
	free_obj_list[--num_objects] = objnum;
	Assert(num_objects >= 0);

	if (objnum == Highest_object_index)
		while (Objects[--Highest_object_index].type == OBJ_NONE);
}

//-----------------------------------------------------------------------------
//	Scan the object list, freeing down to num_used objects
void free_object_slots(int num_used)
{
	int	obj_list[MAX_OBJECTS];

	int olind = 0;
	int num_already_free = MAX_OBJECTS - Highest_object_index - 1;

	if (MAX_OBJECTS - num_already_free < num_used)
		return;

	for (int i = 0; i <= Highest_object_index; i++) 
	{
		if (Objects[i].flags & OF_SHOULD_BE_DEAD)
			num_already_free++;
		else
			switch (Objects[i].type) 
			{
			case OBJ_NONE:
				num_already_free++;
				if (MAX_OBJECTS - num_already_free < num_used)
					return;
				break;
			case OBJ_WALL:
			case OBJ_FLARE:
				Int3();		//	This is curious.  What is an object that is a wall?
				break;
			case OBJ_FIREBALL:
			case OBJ_WEAPON:
			case OBJ_DEBRIS:
				obj_list[olind++] = i;
				break;
			case OBJ_ROBOT:
			case OBJ_HOSTAGE:
			case OBJ_PLAYER:
			case OBJ_CNTRLCEN:
			case OBJ_CLUTTER:
			case OBJ_GHOST:
			case OBJ_LIGHT:
			case OBJ_CAMERA:
			case OBJ_POWERUP:
				break;
			}

	}

	int num_to_free = MAX_OBJECTS - num_used - num_already_free;

	if (num_to_free > olind) 
	{
		mprintf((1, "Warning: Asked to free %i objects, but can only free %i.\n", num_to_free, olind));
		num_to_free = olind;
	}

	for (int i = 0; i < num_to_free; i++)
		if (Objects[obj_list[i]].type == OBJ_DEBRIS) 
		{
			num_to_free--;
			mprintf((0, "Freeing   DEBRIS object %3i\n", obj_list[i]));
			Objects[obj_list[i]].flags |= OF_SHOULD_BE_DEAD;
		}

	if (!num_to_free)
		return;

	for (int i = 0; i < num_to_free; i++)
		if (Objects[obj_list[i]].type == OBJ_FIREBALL && Objects[obj_list[i]].ctype.expl_info.delete_objnum == -1) {
			num_to_free--;
			mprintf((0, "Freeing FIREBALL object %3i\n", obj_list[i]));
			Objects[obj_list[i]].flags |= OF_SHOULD_BE_DEAD;
		}

	if (!num_to_free)
		return;

	for (int i = 0; i < num_to_free; i++)
		if ((Objects[obj_list[i]].type == OBJ_WEAPON) && (Objects[obj_list[i]].id == FLARE_ID)) {
			num_to_free--;
			mprintf((0, "Freeing    FLARE object %3i\n", obj_list[i]));
			Objects[obj_list[i]].flags |= OF_SHOULD_BE_DEAD;
		}

	if (!num_to_free)
		return;

	for (int i = 0; i < num_to_free; i++)
		if ((Objects[obj_list[i]].type == OBJ_WEAPON) && (Objects[obj_list[i]].id != FLARE_ID)) {
			num_to_free--;
			mprintf((0, "Freeing   WEAPON object %3i\n", obj_list[i]));
			Objects[obj_list[i]].flags |= OF_SHOULD_BE_DEAD;
		}

}

//-----------------------------------------------------------------------------
//initialize a new object.  adds to the list for the given segment
//note that segnum is really just a suggestion, since this routine actually
//searches for the correct segment
//returns the object number
int obj_create(uint8_t type, uint8_t id, int segnum, vms_vector* pos,
	vms_matrix* orient, fix size, uint8_t ctype, uint8_t mtype, uint8_t rtype)
{
	Assert((segnum <= Highest_segment_index) && (segnum >= 0));
	Assert(ctype <= CT_CNTRLCEN);

	if (type == OBJ_DEBRIS && Debris_object_count >= Max_debris_objects)
		return -1;

	if (get_seg_masks(pos, segnum, 0).centermask != 0)
		if ((segnum = find_point_seg(pos, segnum)) == -1) 
		{
#ifndef NDEBUG
			mprintf((0, "Bad segnum in obj_create (type=%d)\n", type));
#endif
			return -1;		//don't create this object
		}

	// Find next free object
	int objnum = obj_allocate();

	if (objnum == -1)		//no free objects
		return -1;

	Assert(Objects[objnum].type == OBJ_NONE);		//make sure unused 

	object* obj = &Objects[objnum];

	Assert(obj->segnum == -1);

	// Zero out object structure to keep weird bugs from happening
	// in uninitialized fields.
	memset(obj, 0, sizeof(object));

	obj->signature = Object_next_signature++;
	obj->type = type;
	obj->id = id;
	obj->last_pos = *pos;
	obj->pos = *pos;
	obj->size = size;
	obj->flags = 0;
	if (orient != NULL)
		obj->orient = *orient;

	obj->control_type = ctype;
	obj->movement_type = mtype;
	obj->render_type = rtype;
	obj->contains_type = -1;

	obj->lifeleft = IMMORTAL_TIME;		//assume immortal
	obj->attached_obj = -1;

	if (obj->control_type == CT_POWERUP)
		obj->ctype.powerup_info.count = 1;

	// Init physics info for this object
	if (obj->movement_type == MT_PHYSICS) 
	{
		vm_vec_zero(&obj->mtype.phys_info.velocity);
		vm_vec_zero(&obj->mtype.phys_info.thrust);
		vm_vec_zero(&obj->mtype.phys_info.rotvel);
		vm_vec_zero(&obj->mtype.phys_info.rotthrust);

		obj->mtype.phys_info.mass = 0;
		obj->mtype.phys_info.drag = 0;
		obj->mtype.phys_info.brakes = 0;
		obj->mtype.phys_info.turnroll = 0;
		obj->mtype.phys_info.flags = 0;
	}

	if (obj->render_type == RT_POLYOBJ)
		obj->rtype.pobj_info.tmap_override = -1;

	obj->shields = 20 * F1_0;

	segnum = find_point_seg(pos, segnum);		//find correct segment

	Assert(segnum != -1);

	obj->segnum = -1;					//set to zero by memset, above
	obj_link(objnum, segnum);

	//	Set (or not) persistent bit in phys_info.
	if (obj->type == OBJ_WEAPON) 
	{
		obj->mtype.phys_info.flags |= (Weapon_info[obj->id].persistent * PF_PERSISTENT);
		obj->ctype.laser_info.creation_time = GameTime;
		obj->ctype.laser_info.last_hitobj = -1;
		obj->ctype.laser_info.multiplier = F1_0;
	}

	if (obj->control_type == CT_EXPLOSION)
		obj->ctype.expl_info.next_attach = obj->ctype.expl_info.prev_attach = obj->ctype.expl_info.attach_parent = -1;

#ifndef NDEBUG
	if (print_object_info)
		mprintf((0, "Created object %d of type %d\n", objnum, obj->type));
#endif

	if (obj->type == OBJ_DEBRIS)
		Debris_object_count++;

	return objnum;
}

#ifdef EDITOR
//create a copy of an object. returns new object number
int obj_create_copy(int objnum, vms_vector* new_pos, int newsegnum)
{
	// Find next free object
	int newobjnum = obj_allocate();

	if (newobjnum == -1)
		return -1;

	object* obj = &Objects[newobjnum];

	*obj = Objects[objnum];

	obj->pos = obj->last_pos = *new_pos;

	obj->next = obj->prev = obj->segnum = -1;

	obj_link(newobjnum, newsegnum);

	obj->signature = Object_next_signature++;

	//we probably should initialize sub-structures here
	return newobjnum;
}
#endif

//remove object from the world
void obj_delete(int objnum)
{
	object* obj = &Objects[objnum];

	Assert(objnum != -1);
	Assert(objnum != 0);
	Assert(obj->type != OBJ_NONE);
	Assert(obj != ConsoleObject);

	if (obj == Viewer)		//deleting the viewer?
		Viewer = ConsoleObject;						//..make the player the viewer

	if (obj->flags & OF_ATTACHED)		//detach this from object
		obj_detach_one(obj);

	if (obj->attached_obj != -1)		//detach all objects from this
		obj_detach_all(obj);

#if !defined(NDEBUG) && !defined(NMONO)
	if (print_object_info) mprintf((0, "Deleting object %d of type %d\n", objnum, Objects[objnum].type));
#endif

	if (obj->type == OBJ_DEBRIS)
		Debris_object_count--;

	obj_unlink(objnum);

	Assert(Objects[0].next != 0);

	obj->type = OBJ_NONE;		//unused!
	obj->signature = -1;

	obj_free(objnum);
}

#define	DEATH_SEQUENCE_LENGTH			(F1_0*5)
#define	DEATH_SEQUENCE_EXPLODE_TIME	(F1_0*2)

int		Player_is_dead = 0;			//	If !0, then player is dead, but game continues so he can watch.
object* Dead_player_camera = NULL;	//	Object index of object watching deader.
fix		Player_time_of_death;		//	Time at which player died.
object* Viewer_save;
int		Player_flags_save;
int		Player_exploded = 0;
int		Death_sequence_aborted = 0;
int		Player_eggs_dropped = 0;
fix		Camera_to_player_dist_goal = F1_0 * 4;

uint8_t		Control_type_save, Render_type_save, cockpit_mode_save;

//	------------------------------------------------------------------------------------------------------------------
void dead_player_end(void)
{
	if (!Player_is_dead)
		return;

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_restore_cockpit();

	Player_is_dead = 0;
	Player_exploded = 0;
	obj_delete(Dead_player_camera - Objects);
	Dead_player_camera = NULL;
	select_cockpit(cockpit_mode_save);
	Viewer = Viewer_save;
	ConsoleObject->type = OBJ_PLAYER;
	ConsoleObject->flags = Player_flags_save;

	Assert((Control_type_save == CT_FLYING) || (Control_type_save == CT_SLEW));

	ConsoleObject->control_type = Control_type_save;
	ConsoleObject->render_type = Render_type_save;
	Players[Player_num].flags &= ~PLAYER_FLAGS_INVULNERABLE;
	Player_eggs_dropped = 0;
}

//	------------------------------------------------------------------------------------------------------------------
//	Camera is less than size of player away from 
void set_camera_pos(vms_vector* camera_pos, object* objp)
{
	int	count = 0;

	fix camera_player_dist = vm_vec_dist_quick(camera_pos, &objp->pos);

	if (camera_player_dist < Camera_to_player_dist_goal)
	{
		//	Camera is too close to player object, so move it away.
		vms_vector	player_camera_vec;
		fvi_query	fq;
		fvi_info	hit_data;
		vms_vector	local_p1;

		vm_vec_sub(&player_camera_vec, camera_pos, &objp->pos);
		if ((player_camera_vec.x == 0) && (player_camera_vec.y == 0) && (player_camera_vec.z == 0))
			player_camera_vec.x += F1_0 / 16;

		hit_data.hit_type = HIT_WALL;
		fix far_scale = F1_0;

		while ((hit_data.hit_type != HIT_NONE) && (count++ < 6)) 
		{
			vms_vector closer_p1;
			vm_vec_normalize_quick(&player_camera_vec);
			vm_vec_scale(&player_camera_vec, Camera_to_player_dist_goal);

			fq.p0 = &objp->pos;
			vm_vec_add(&closer_p1, &objp->pos, &player_camera_vec);		//	This is the actual point we want to put the camera at.
			vm_vec_scale(&player_camera_vec, far_scale);						//	...but find a point 50% further away...
			vm_vec_add(&local_p1, &objp->pos, &player_camera_vec);		//	...so we won't have to do as many cuts.

			fq.p1 = &local_p1;
			fq.startseg = objp->segnum;
			fq.rad = 0;
			fq.thisobjnum = objp - Objects;
			fq.ignore_obj_list = NULL;
			fq.flags = 0;
			find_vector_intersection(&fq, &hit_data);

			if (hit_data.hit_type == HIT_NONE) 
			{
				*camera_pos = closer_p1;
			}
			else 
			{
				make_random_vector(&player_camera_vec);
				far_scale = 3 * F1_0 / 2;
			}
		}
	}
}

extern void drop_player_eggs(object* objp);
extern int get_explosion_vclip(object* obj, int stage);

//	------------------------------------------------------------------------------------------------------------------
void dead_player_frame(void)
{
	fix	time_dead;
	vms_vector	fvec;

	if (Player_is_dead) 
	{
		time_dead = GameTime - Player_time_of_death;

		//	If unable to create camera at time of death, create now.
		if (Dead_player_camera == Viewer_save) 
		{
			int		objnum;
			object* player = &Objects[Players[Player_num].objnum];

			objnum = obj_create(OBJ_CAMERA, 0, player->segnum, &player->pos, &player->orient, 0, CT_NONE, MT_NONE, RT_NONE);

			mprintf((0, "Creating new dead player camera.\n"));
			if (objnum != -1)
				Viewer = Dead_player_camera = &Objects[objnum];
			else 
			{
				mprintf((1, "Can't create dead player camera.\n"));
				Int3();
			}
		}

		ConsoleObject->mtype.phys_info.rotvel.x = std::max<fix>(0, DEATH_SEQUENCE_EXPLODE_TIME - time_dead) / 4;
		ConsoleObject->mtype.phys_info.rotvel.y = std::max<fix>(0, DEATH_SEQUENCE_EXPLODE_TIME - time_dead) / 2;
		ConsoleObject->mtype.phys_info.rotvel.z = std::max<fix>(0, DEATH_SEQUENCE_EXPLODE_TIME - time_dead) / 3;

		Camera_to_player_dist_goal = std::min<fix>(time_dead * 8, F1_0 * 20) + ConsoleObject->size;

		set_camera_pos(&Dead_player_camera->pos, ConsoleObject);

		vm_vec_sub(&fvec, &ConsoleObject->pos, &Dead_player_camera->pos);
		vm_vector_2_matrix(&Dead_player_camera->orient, &fvec, NULL, NULL);

		if (time_dead > DEATH_SEQUENCE_EXPLODE_TIME) 
		{
			if (!Player_exploded) 
			{
				if (Players[Player_num].hostages_on_board > 1)
					HUD_init_message(TXT_SHIP_DESTROYED_2, Players[Player_num].hostages_on_board);
				else if (Players[Player_num].hostages_on_board == 1)
					HUD_init_message(TXT_SHIP_DESTROYED_1);
				else
					HUD_init_message(TXT_SHIP_DESTROYED_0);

				Player_exploded = 1;

				drop_player_eggs(ConsoleObject);
				Player_eggs_dropped = 1;
#ifdef NETWORK
				if (Game_mode & GM_MULTI)
				{
					multi_send_position(Players[Player_num].objnum);
					multi_send_player_explode(MULTI_PLAYER_EXPLODE);
				}
#endif

				explode_badass_player(ConsoleObject);

				//is this next line needed, given the badass call above?
				explode_object(ConsoleObject, 0);
				ConsoleObject->flags &= ~OF_SHOULD_BE_DEAD;		//don't really kill player
				ConsoleObject->render_type = RT_NONE;				//..just make him disappear
				ConsoleObject->type = OBJ_GHOST;						//..and kill intersections
			}
		}
		else 
		{
			if (P_Rand() < FrameTime * 4)
			{
#ifdef NETWORK
				if (Game_mode & GM_MULTI)
					multi_send_create_explosion(Player_num);
#endif
				create_small_fireball_on_object(ConsoleObject, F1_0, 1);
			}
		}

		if (Death_sequence_aborted) 
		{ 
			if (!Player_eggs_dropped) 
			{
				drop_player_eggs(ConsoleObject);
				Player_eggs_dropped = 1;
#ifdef NETWORK
				if (Game_mode & GM_MULTI)
				{
					multi_send_position(Players[Player_num].objnum);
					multi_send_player_explode(MULTI_PLAYER_EXPLODE);
				}
#endif
			}

			DoPlayerDead();
		}
	}
}

int Killed_in_frame = -1;
short Killed_objnum = -1;

//	------------------------------------------------------------------------------------------------------------------
void start_player_death_sequence(object* player)
{
	Assert(player == ConsoleObject);
	if ((Player_is_dead != 0) || (Dead_player_camera != NULL))
		return;

	//Assert(Player_is_dead == 0);
	//Assert(Dead_player_camera == NULL);

	reset_rear_view();

	if (!(Game_mode & GM_MULTI))
		HUD_clear_messages();

	Killed_in_frame = FrameCount;
	Killed_objnum = player - Objects;
	Death_sequence_aborted = 0;

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		multi_send_kill(Players[Player_num].objnum);
	}
#endif

	PaletteRedAdd = 40;
	Player_is_dead = 1;
	Players[Player_num].flags &= ~(PLAYER_FLAGS_AFTERBURNER);

	vm_vec_zero(&player->mtype.phys_info.rotthrust);
	vm_vec_zero(&player->mtype.phys_info.thrust);

	Player_time_of_death = GameTime;

	int objnum = obj_create(OBJ_CAMERA, 0, player->segnum, &player->pos, &player->orient, 0, CT_NONE, MT_NONE, RT_NONE);
	Viewer_save = Viewer;
	if (objnum != -1)
		Viewer = Dead_player_camera = &Objects[objnum];
	else 
	{
		mprintf((1, "Can't create dead player camera.\n"));
		Int3();
		Dead_player_camera = Viewer;
	}

	cockpit_mode_save = Cockpit_mode;
	select_cockpit(CM_LETTERBOX);
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_letterbox();

	Player_flags_save = player->flags;
	Control_type_save = player->control_type;
	Render_type_save = player->render_type;

	player->flags &= ~OF_SHOULD_BE_DEAD;
	//	Players[Player_num].flags |= PLAYER_FLAGS_INVULNERABLE;
	player->control_type = CT_NONE;
	player->shields = F1_0 * 1000;

	PALETTE_FLASH_SET(0, 0, 0);
}

//	------------------------------------------------------------------------------------------------------------------
void obj_delete_all_that_should_be_dead()
{
	int	local_dead_player_object = -1;

	// Move all objects
	object* objp = Objects;

	for (int i = 0; i <= Highest_object_index; i++) 
	{
		if ((objp->type != OBJ_NONE) && (objp->flags & OF_SHOULD_BE_DEAD)) 
		{
			Assert(!(objp->type == OBJ_FIREBALL && objp->ctype.expl_info.delete_time != -1));
			if (objp->type == OBJ_PLAYER) 
			{
				if (objp->id == Player_num) 
				{
					if (local_dead_player_object == -1) 
					{
						start_player_death_sequence(objp);
						local_dead_player_object = objp - Objects;
					}
					else
						Int3();	//	Contact Mike: Illegal, killed player twice in this frame!
									// Ok to continue, won't start death sequence again!
					// kill_player();
				}
			}
			else 
			{
				obj_delete(i);
			}
		}
		objp++;
	}
}

//when an object has moved into a new segment, this function unlinks it
//from its old segment, and links it into the new segment
void obj_relink(int objnum, int newsegnum)
{
	Assert((objnum >= 0) && (objnum <= Highest_object_index));
	Assert((newsegnum <= Highest_segment_index) && (newsegnum >= 0));

	obj_unlink(objnum);

	obj_link(objnum, newsegnum);

#ifndef NDEBUG
	if (get_seg_masks(&Objects[objnum].pos, Objects[objnum].segnum, 0).centermask != 0)
		mprintf((1, "obj_relink violates seg masks.\n"));
#endif
}

//process a continuously-spinning object
void spin_object(object* obj)
{
	vms_angvec rotangs;
	vms_matrix rotmat, new_pm;

	Assert(obj->movement_type == MT_SPINNING);

	rotangs.p = (fixang)fixmul(obj->mtype.spin_rate.x, FrameTime);
	rotangs.h = (fixang)fixmul(obj->mtype.spin_rate.y, FrameTime);
	rotangs.b = (fixang)fixmul(obj->mtype.spin_rate.z, FrameTime);

	vm_angles_2_matrix(&rotmat, &rotangs);

	vm_matrix_x_matrix(&new_pm, &obj->orient, &rotmat);
	obj->orient = new_pm;

	check_and_fix_matrix(&obj->orient);
}

//--------------------------------------------------------------------
//move an object for the current frame
void object_move_one(object* obj)
{
	int	previous_segment = obj->segnum;

	obj->last_pos = obj->pos;			// Save the current position

	if ((obj->type == OBJ_PLAYER) && (Player_num == obj->id)) 
	{
		fix fuel;
		fuel = fuelcen_give_fuel(&Segments[obj->segnum], i2f(100) - Players[Player_num].energy);
		if (fuel > 0) 
		{
			Players[Player_num].energy += fuel;
		}
#ifdef RESTORE_REPAIRCENTER
		check_start_repair_center(obj);
#endif
	}

	if (obj->lifeleft != IMMORTAL_TIME)	//if not immortal...
		obj->lifeleft -= FrameTime;		//...inevitable countdown towards death

	switch (obj->control_type) 
	{

	case CT_NONE: break;

	case CT_FLYING:

#if !defined(NDEBUG) && !defined(NMONO)
		if (print_object_info > 1) mprintf((0, "Moving player object #%d\n", obj - Objects));
#endif

		read_flying_controls(obj);

		break;

#ifdef RESTORE_REPAIRCENTER
	case CT_REPAIRCEN: do_repair_sequence(obj); break;
#else
	case CT_REPAIRCEN: Int3();	// -- hey! these are no longer supported!!
#endif

	case CT_POWERUP: do_powerup_frame(obj); break;

	case CT_MORPH:			//morph implies AI
		do_morph_frame(obj);
		//NOTE: FALLS INTO AI HERE!!!!

	case CT_AI:
		//NOTE LINK TO CT_MORPH ABOVE!!!
		if (Game_suspended & SUSP_ROBOTS) return;
#if !defined(NDEBUG) && !defined(NMONO)
		if (print_object_info > 1) mprintf((0, "AI: Moving robot object #%d\n", obj - Objects));
#endif
		do_ai_frame(obj);
		break;

	case CT_WEAPON:		Laser_do_weapon_sequence(obj); break;
	case CT_EXPLOSION:	do_explosion_sequence(obj); break;

#ifndef RELEASE
	case CT_SLEW:
		if (keyd_pressed[KEY_PAD5]) slew_stop();
		if (keyd_pressed[KEY_NUMLOCK]) 
		{
			slew_reset_orient();
		}
		slew_frame(0);		// Does velocity addition for us.
		break;
#endif

		//		case CT_FLYTHROUGH:
		//			do_flythrough(obj,0);			// HACK:do_flythrough should operate on an object!!!!
		//			//check_object_seg(obj);
		//			return;	// DON'T DO THE REST OF OBJECT STUFF SINCE THIS IS A SPECIAL CASE!!!
		//			break;

	case CT_DEBRIS: do_debris_frame(obj); break;

	case CT_LIGHT: break;		//doesn't do anything

	case CT_REMOTE: break;     //movement is handled in com_process_input

	case CT_CNTRLCEN: do_controlcen_frame(obj); break;

	default:

		Error("Unknown control type %d in object %i, sig/type/id = %i/%i/%i", obj->control_type, obj - Objects, obj->signature, obj->type, obj->id);

		break;

	}

	if (obj->lifeleft < 0) // We died of old age
	{
		obj->flags |= OF_SHOULD_BE_DEAD;
		if (Weapon_info[obj->id].damage_radius)
			explode_badass_weapon(obj);
	}

	if (obj->type == OBJ_NONE || obj->flags & OF_SHOULD_BE_DEAD)
		return;			//object has been deleted

	switch (obj->movement_type) 
	{

	case MT_NONE:			break;								//this doesn't move

	case MT_PHYSICS:		do_physics_sim(obj);	break;	//move by physics

	case MT_SPINNING:		spin_object(obj); break;

	}

	//	If player and moved to another segment, see if hit any triggers.
	if (obj->type == OBJ_PLAYER && obj->movement_type == MT_PHYSICS) 
	{
		if (previous_segment != obj->segnum) 
		{
			int	connect_side, i;
			for (i = 0; i < n_phys_segs - 1; i++) 
			{
				connect_side = find_connect_side(&Segments[phys_seglist[i + 1]], &Segments[phys_seglist[i]]);
				if (connect_side != -1)
					check_trigger(&Segments[phys_seglist[i]], connect_side, obj - Objects);
				//check_trigger(&Segments[previous_segment], connect_side, obj-Objects);
#ifndef NDEBUG
				else // segments are not directly connected, so do binary subdivision until you find connected segments.
				{	
					mprintf((1, "UNCONNECTED SEGMENTS %d,%d\n", phys_seglist[i + 1], phys_seglist[i]));
				}
#endif
			}
		}
	}
}

int	Max_used_objects = MAX_OBJECTS - 20;

//--------------------------------------------------------------------
//move all objects for the current frame
void object_move_all()
{
	if (Highest_object_index > Max_used_objects)
		free_object_slots(Max_used_objects);		//	Free all possible object slots.

	obj_delete_all_that_should_be_dead();

	if (Auto_leveling_on)
		ConsoleObject->mtype.phys_info.flags |= PF_LEVELLING;
	else
		ConsoleObject->mtype.phys_info.flags &= ~PF_LEVELLING;

	// Move all objects
	object* objp = Objects;

	for (int i = 0; i <= Highest_object_index; i++) 
	{
		if ((objp->type != OBJ_NONE) && (!(objp->flags & OF_SHOULD_BE_DEAD))) 
		{
			object_move_one(objp);
		}
		objp++;
	}
}

//make object array non-sparse
void compress_objects(void)
{
	//	Note: It's proper to do < (rather than <=) Highest_object_index here because we
	//	are just removing gaps, and the last object can't be a gap.
	for (int start_i = 0; start_i < Highest_object_index; start_i++)
		if (Objects[start_i].type == OBJ_NONE) 
		{

			int	segnum_copy;

			segnum_copy = Objects[Highest_object_index].segnum;

			obj_unlink(Highest_object_index);

			Objects[start_i] = Objects[Highest_object_index];

#ifdef EDITOR
			if (Cur_object_index == Highest_object_index)
				Cur_object_index = start_i;
#endif

			Objects[Highest_object_index].type = OBJ_NONE;

			obj_link(start_i, segnum_copy);

			while (Objects[--Highest_object_index].type == OBJ_NONE);
		}

	reset_objects(num_objects);

}

//called after load.  Takes number of objects,  and objects should be 
//compressed.  resets free list, marks unused objects as unused
void reset_objects(int n_objs)
{
	num_objects = n_objs;

	Assert(num_objects > 0);

	for (int i = num_objects; i < MAX_OBJECTS; i++) 
	{
		free_obj_list[i] = i;
		Objects[i].type = OBJ_NONE;
		Objects[i].segnum = -1;
	}

	Highest_object_index = num_objects - 1;

	Debris_object_count = 0;
}

//Tries to find a segment for an object, using find_point_seg()
int find_object_seg(object* obj)
{
	return find_point_seg(&obj->pos, obj->segnum);
}


//If an object is in a segment, set its segnum field and make sure it's
//properly linked.  If not in any segment, returns 0, else 1.
//callers should generally use find_vector_intersection()  
int update_object_seg(object* obj)
{
	int newseg = find_object_seg(obj);

	if (newseg == -1)
		return 0;

	if (newseg != obj->segnum)
		obj_relink(obj - Objects, newseg);

	return 1;
}


//go through all objects and make sure they have the correct segment numbers
void fix_object_segs()
{
	for (int i = 0; i <= Highest_object_index; i++)
		if (Objects[i].type != OBJ_NONE)
			if (update_object_seg(&Objects[i]) == 0) 
			{
				mprintf((1, "Cannot find segment for object %d in fix_object_segs()\n"));
				Int3();
				compute_segment_center(&Objects[i].pos, &Segments[Objects[i].segnum]);
			}
}

//delete objects, such as weapons & explosions, that shouldn't stay between levels
//	Changed by MK on 10/15/94, don't remove proximity bombs.
//if clear_all is set, clear even proximity bombs
void clear_transient_objects(int clear_all)
{
	int objnum;
	object* obj;

	for (objnum = 0, obj = &Objects[0]; objnum <= Highest_object_index; objnum++, obj++)
		if (((obj->type == OBJ_WEAPON) && (clear_all || obj->id != PROXIMITY_ID)) ||
			obj->type == OBJ_FIREBALL ||
			obj->type == OBJ_DEBRIS ||
			(obj->type != OBJ_NONE && obj->flags & OF_EXPLODING)) {

#ifndef NDEBUG
			if (Objects[objnum].lifeleft > i2f(2))
				mprintf((0, "Note: Clearing object %d (type=%d, id=%d) with lifeleft=%x\n", objnum, Objects[objnum].type, Objects[objnum].id, Objects[objnum].lifeleft));
#endif
			obj_delete(objnum);
		}
#ifndef NDEBUG
		else if (Objects[objnum].type != OBJ_NONE && Objects[objnum].lifeleft < i2f(2))
			mprintf((0, "Note: NOT clearing object %d (type=%d, id=%d) with lifeleft=%x\n", objnum, Objects[objnum].type, Objects[objnum].id, Objects[objnum].lifeleft));
#endif
}

//attaches an object, such as a fireball, to another object, such as a robot
void obj_attach(object* parent, object* sub)
{
	Assert(sub->type == OBJ_FIREBALL);
	Assert(sub->control_type == CT_EXPLOSION);

	Assert(sub->ctype.expl_info.next_attach == -1);
	Assert(sub->ctype.expl_info.prev_attach == -1);

	Assert(parent->attached_obj == -1 || Objects[parent->attached_obj].ctype.expl_info.prev_attach == -1);

	sub->ctype.expl_info.next_attach = parent->attached_obj;

	if (sub->ctype.expl_info.next_attach != -1)
		Objects[sub->ctype.expl_info.next_attach].ctype.expl_info.prev_attach = sub - Objects;

	parent->attached_obj = sub - Objects;

	sub->ctype.expl_info.attach_parent = parent - Objects;
	sub->flags |= OF_ATTACHED;

	Assert(sub->ctype.expl_info.next_attach != sub - Objects);
	Assert(sub->ctype.expl_info.prev_attach != sub - Objects);
}

//dettaches one object
void obj_detach_one(object* sub)
{
	Assert(sub->flags & OF_ATTACHED);
	Assert(sub->ctype.expl_info.attach_parent != -1);

	if ((Objects[sub->ctype.expl_info.attach_parent].type == OBJ_NONE) || (Objects[sub->ctype.expl_info.attach_parent].attached_obj == -1))
	{
		sub->flags &= ~OF_ATTACHED;
		return;
	}

	if (sub->ctype.expl_info.next_attach != -1) {
		Assert(Objects[sub->ctype.expl_info.next_attach].ctype.expl_info.prev_attach = sub - Objects);
		Objects[sub->ctype.expl_info.next_attach].ctype.expl_info.prev_attach = sub->ctype.expl_info.prev_attach;
	}

	if (sub->ctype.expl_info.prev_attach != -1) {
		Assert(Objects[sub->ctype.expl_info.prev_attach].ctype.expl_info.next_attach = sub - Objects);
		Objects[sub->ctype.expl_info.prev_attach].ctype.expl_info.next_attach = sub->ctype.expl_info.next_attach;
	}
	else {
		Assert(Objects[sub->ctype.expl_info.attach_parent].attached_obj = sub - Objects);
		Objects[sub->ctype.expl_info.attach_parent].attached_obj = sub->ctype.expl_info.next_attach;
	}

	sub->ctype.expl_info.next_attach = sub->ctype.expl_info.prev_attach = -1;
	sub->flags &= ~OF_ATTACHED;

}

//dettaches all objects from this object
void obj_detach_all(object* parent)
{
	while (parent->attached_obj != -1)
		obj_detach_one(&Objects[parent->attached_obj]);
}

static void read_vector(vms_vector* v, FILE* file)
{
	v->x = file_read_int(file);
	v->y = file_read_int(file);
	v->z = file_read_int(file);
}

static void read_matrix(vms_matrix* m, FILE* file)
{
	read_vector(&m->rvec, file);
	read_vector(&m->uvec, file);
	read_vector(&m->fvec, file);
}

static void read_angvec(vms_angvec* v, FILE* file)
{
	v->p = file_read_short(file);
	v->b = file_read_short(file);
	v->h = file_read_short(file);
}

static void write_vector(vms_vector* v, FILE* file)
{
	file_write_int(file, v->x);
	file_write_int(file, v->y);
	file_write_int(file, v->z);
}

static void write_matrix(vms_matrix* m, FILE* file)
{
	write_vector(&m->rvec, file);
	write_vector(&m->uvec, file);
	write_vector(&m->fvec, file);
}

static void write_angvec(vms_angvec* v, FILE* file)
{
	file_write_short(file, v->p);
	file_write_short(file, v->b);
	file_write_short(file, v->h);
}

//Read an object from disk. These unions are going to be the end of me.....
void read_obj_instance(object* obj, FILE* f)
{
	obj->signature = file_read_int(f);
	obj->type = file_read_byte(f);
	obj->id = file_read_byte(f);

	obj->next = file_read_short(f); obj->prev = file_read_short(f);

	obj->control_type = file_read_byte(f);
	obj->movement_type = file_read_byte(f);
	obj->render_type = file_read_byte(f);
	obj->flags = file_read_byte(f);

	obj->segnum = file_read_short(f);
	obj->attached_obj = file_read_short(f);

	read_vector(&obj->pos, f);
	read_matrix(&obj->orient, f);

	obj->size = file_read_int(f);
	obj->shields = file_read_int(f);

	read_vector(&obj->last_pos, f);

	obj->contains_type = file_read_byte(f);
	obj->contains_id = file_read_byte(f);
	obj->contains_count = file_read_byte(f);
	obj->matcen_creator = file_read_byte(f);

	obj->lifeleft = file_read_int(f);

	//Pad shorter structures by this much
	int bytesLeft = 64;

	switch (obj->movement_type)
	{
	case MT_PHYSICS:
		read_vector(&obj->mtype.phys_info.velocity, f);
		read_vector(&obj->mtype.phys_info.thrust, f);
		obj->mtype.phys_info.mass = file_read_int(f);
		obj->mtype.phys_info.drag = file_read_int(f);
		obj->mtype.phys_info.brakes = file_read_int(f);
		read_vector(&obj->mtype.phys_info.rotvel, f);
		read_vector(&obj->mtype.phys_info.rotthrust, f);
		obj->mtype.phys_info.turnroll = file_read_short(f);
		obj->mtype.phys_info.flags = file_read_short(f);
		bytesLeft = 0;
		break;
	case MT_SPINNING:
		read_vector(&obj->mtype.spin_rate, f);
		bytesLeft -= 12;
		break;
	}
	fseek(f, bytesLeft, SEEK_CUR);

	bytesLeft = 30;
	switch (obj->control_type)
	{

	case CT_AI:
	{
		//30
		obj->ctype.ai_info.behavior = file_read_byte(f);

		for (int i = 0; i < MAX_AI_FLAGS; i++)
			obj->ctype.ai_info.flags[i] = file_read_byte(f);

		obj->ctype.ai_info.hide_segment = file_read_short(f);
		obj->ctype.ai_info.hide_index = file_read_short(f);
		obj->ctype.ai_info.path_length = file_read_short(f);
		obj->ctype.ai_info.cur_path_index = file_read_short(f);

		obj->ctype.ai_info.follow_path_start_seg = file_read_short(f);
		obj->ctype.ai_info.follow_path_end_seg = file_read_short(f);

		obj->ctype.ai_info.danger_laser_signature = file_read_int(f);
		obj->ctype.ai_info.danger_laser_num = file_read_short(f);
		bytesLeft = 0;
		break;
	}
	case CT_EXPLOSION:
	case CT_DEBRIS:
		//16
		obj->ctype.expl_info.spawn_time = file_read_int(f);
		obj->ctype.expl_info.delete_time = file_read_int(f);
		obj->ctype.expl_info.delete_objnum = file_read_short(f);
		obj->ctype.expl_info.attach_parent = file_read_short(f);
		obj->ctype.expl_info.next_attach = file_read_short(f);
		obj->ctype.expl_info.prev_attach = file_read_short(f);
		bytesLeft -= 16;
		break;
	case CT_WEAPON:
		//20
		obj->ctype.laser_info.parent_type = file_read_short(f);
		obj->ctype.laser_info.parent_num = file_read_short(f);
		obj->ctype.laser_info.parent_signature = file_read_int(f);
		obj->ctype.laser_info.creation_time = file_read_int(f);
		obj->ctype.laser_info.last_hitobj = file_read_short(f);
		obj->ctype.laser_info.track_goal = file_read_short(f);
		obj->ctype.laser_info.multiplier = file_read_int(f);
		bytesLeft -= 20;
		break;
	case CT_LIGHT:
		//4
		obj->ctype.light_info.intensity = file_read_int(f);
		bytesLeft -= 4;
		break;
	case CT_POWERUP:
		//4
		obj->ctype.powerup_info.count = file_read_int(f);
		bytesLeft -= 4;
		break;
	}
	fseek(f, bytesLeft, SEEK_CUR);

	bytesLeft = 76;
	switch (obj->render_type)
	{
	case RT_NONE:
		break;
	case RT_MORPH:
	case RT_POLYOBJ:
	{
		//76
		obj->rtype.pobj_info.model_num = file_read_int(f);
		for (int i = 0; i < MAX_SUBMODELS; i++)
			read_angvec(&obj->rtype.pobj_info.anim_angles[i], f);
		obj->rtype.pobj_info.subobj_flags = file_read_int(f);
		obj->rtype.pobj_info.tmap_override = file_read_int(f);
		obj->rtype.pobj_info.alt_textures = file_read_int(f); 
		bytesLeft = 0;
		break;
	}
	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:
		//9
		obj->rtype.vclip_info.vclip_num = file_read_int(f);
		obj->rtype.vclip_info.frametime = file_read_int(f);
		obj->rtype.vclip_info.framenum = file_read_byte(f);
		bytesLeft -= 9;
		break;
	case RT_LASER:
		break;
	}
	fseek(f, bytesLeft, SEEK_CUR);
}

void write_obj_instance(object* obj, FILE* f)
{
	uint8_t hack[76];
	memset(&hack[0], 0, 76 * sizeof(uint8_t));
	file_write_int(f, obj->signature);
	file_write_byte(f, obj->type);
	file_write_byte(f, obj->id);

	file_write_short(f, obj->next); 
	file_write_short(f, obj->prev);

	file_write_byte(f, obj->control_type);
	file_write_byte(f, obj->movement_type);
	file_write_byte(f, obj->render_type);
	file_write_byte(f, obj->flags);

	file_write_short(f, obj->segnum);
	file_write_short(f, obj->attached_obj);

	write_vector(&obj->pos, f);
	write_matrix(&obj->orient, f);

	file_write_int(f, obj->size);
	file_write_int(f, obj->shields);

	write_vector(&obj->last_pos, f);

	file_write_byte(f, obj->contains_type);
	file_write_byte(f, obj->contains_id);
	file_write_byte(f, obj->contains_count);
	file_write_byte(f, obj->matcen_creator);

	file_write_int(f, obj->lifeleft);

	//Pad shorter structures by this much
	int bytesLeft = 64;

	switch (obj->movement_type)
	{
	case MT_PHYSICS:
		write_vector(&obj->mtype.phys_info.velocity, f);
		write_vector(&obj->mtype.phys_info.thrust, f);
		file_write_int(f, obj->mtype.phys_info.mass);
		file_write_int(f, obj->mtype.phys_info.drag);
		file_write_int(f, obj->mtype.phys_info.brakes);
		write_vector(&obj->mtype.phys_info.rotvel, f);
		write_vector(&obj->mtype.phys_info.rotthrust, f);
		file_write_short(f, obj->mtype.phys_info.turnroll);
		file_write_short(f, obj->mtype.phys_info.flags);
		bytesLeft = 0;
		break;
	case MT_SPINNING:
		write_vector(&obj->mtype.spin_rate, f);
		bytesLeft -= 12;
		break;
	}
	//fseek(f, bytesLeft, SEEK_CUR);
	fwrite(&hack[0], 1, bytesLeft, f);

	bytesLeft = 30;
	switch (obj->control_type)
	{

	case CT_AI:
	{
		//30
		file_write_byte(f, obj->ctype.ai_info.behavior);

		for (int i = 0; i < MAX_AI_FLAGS; i++)
			file_write_byte(f, obj->ctype.ai_info.flags[i]);

		file_write_short(f, obj->ctype.ai_info.hide_segment);
		file_write_short(f, obj->ctype.ai_info.hide_index);
		file_write_short(f, obj->ctype.ai_info.path_length);
		file_write_short(f, obj->ctype.ai_info.cur_path_index);

		file_write_short(f, obj->ctype.ai_info.follow_path_start_seg);
		file_write_short(f, obj->ctype.ai_info.follow_path_end_seg);

		file_write_int(f, obj->ctype.ai_info.danger_laser_signature);
		file_write_short(f, obj->ctype.ai_info.danger_laser_num);
		bytesLeft = 0;
		break;
	}
	case CT_EXPLOSION:
	case CT_DEBRIS:
		//16
		file_write_int(f, obj->ctype.expl_info.spawn_time);
		file_write_int(f, obj->ctype.expl_info.delete_time);
		file_write_short(f, obj->ctype.expl_info.delete_objnum);
		file_write_short(f, obj->ctype.expl_info.attach_parent);
		file_write_short(f, obj->ctype.expl_info.next_attach);
		file_write_short(f, obj->ctype.expl_info.prev_attach);
		bytesLeft -= 16;
		break;
	case CT_WEAPON:
		//20
		file_write_short(f, obj->ctype.laser_info.parent_type);
		file_write_short(f, obj->ctype.laser_info.parent_num);
		file_write_int(f, obj->ctype.laser_info.parent_signature);
		file_write_int(f, obj->ctype.laser_info.creation_time);
		file_write_short(f, obj->ctype.laser_info.last_hitobj);
		file_write_short(f, obj->ctype.laser_info.track_goal);
		file_write_int(f, obj->ctype.laser_info.multiplier);
		bytesLeft -= 20;
		break;
	case CT_LIGHT:
		//4
		file_write_int(f, obj->ctype.light_info.intensity);
		bytesLeft -= 4;
		break;
	case CT_POWERUP:
		//4
		file_write_int(f, obj->ctype.powerup_info.count);
		bytesLeft -= 4;
		break;
	}
	//fseek(f, bytesLeft, SEEK_CUR);
	fwrite(&hack[0], 1, bytesLeft, f);

	bytesLeft = 76;
	switch (obj->render_type)
	{
	case RT_NONE:
		break;
	case RT_MORPH:
	case RT_POLYOBJ:
	{
		//76
		file_write_int(f, obj->rtype.pobj_info.model_num);
		for (int i = 0; i < MAX_SUBMODELS; i++)
			write_angvec(&obj->rtype.pobj_info.anim_angles[i], f);
		file_write_int(f, obj->rtype.pobj_info.subobj_flags);
		file_write_int(f, obj->rtype.pobj_info.tmap_override);
		file_write_int(f, obj->rtype.pobj_info.alt_textures);
		bytesLeft = 0;
		break;
	}
	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:
		//9
		file_write_int(f, obj->rtype.vclip_info.vclip_num);
		file_write_int(f, obj->rtype.vclip_info.frametime);
		file_write_byte(f, obj->rtype.vclip_info.framenum);
		bytesLeft -= 9;
		break;
	case RT_LASER:
		break;
	}
	//fseek(f, bytesLeft, SEEK_CUR);
	fwrite(&hack[0], 1, bytesLeft, f);
}

void obj_get_debug_str(object* objp, std::string& str)
{
	int objnum = objp - Objects;
	std::string objstring = std::format("Object #{}\nIn segment #{}\n", objnum, objp->segnum);
	str += objstring;

	switch (objp->control_type)
	{
	case CT_AI:
	case CT_MORPH:
		ai_get_debug_str(objp, str);
		break;
	}
}

void obj_add_3d_vis(object* objp)
{
	switch (objp->control_type)
	{
	case CT_AI:
	case CT_MORPH:
		ai_obj_add_3d_vis(objp);
		break;
	}
}

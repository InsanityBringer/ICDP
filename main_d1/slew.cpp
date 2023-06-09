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

#include <stdlib.h>
#include "inferno.h"
#include "game.h"
#include "vecmat/vecmat.h"
#include "platform/key.h"
#include "platform/joy.h"
#include "object.h"
#include "misc/error.h"
#include "physics.h"
#include "joydefs.h"
#include "kconfig.h"

//variables for slew system

object* slew_obj = NULL;	//what object is slewing, or NULL if none

#define JOY_NULL 15
#define ROT_SPEED 8		//rate of rotation while key held down
#define VEL_SPEED (2*55)	//rate of acceleration while key held down

short old_joy_x, old_joy_y;	//position last time around

int slew_stop() //has to return int because it's part of the editor function table. 
{
	if (!slew_obj || slew_obj->control_type != CT_SLEW) 
		return 0;

	vm_vec_zero(&slew_obj->mtype.phys_info.velocity);
	return 0;
}

//say start slewing with this object
void slew_init(object* obj)
{
	slew_obj = obj;

	slew_obj->control_type = CT_SLEW;
	slew_obj->movement_type = MT_NONE;

	slew_stop();		//make sure not moving
}

void slew_reset_orient()
{
	if (!slew_obj || slew_obj->control_type != CT_SLEW) return;

	slew_obj->orient.rvec.x = slew_obj->orient.uvec.y = slew_obj->orient.fvec.z = f1_0;

	slew_obj->orient.rvec.y = slew_obj->orient.rvec.z = slew_obj->orient.uvec.x =
		slew_obj->orient.uvec.z = slew_obj->orient.fvec.x = slew_obj->orient.fvec.y = 0;
}

bool do_slew_movement(object* obj, bool check_keys, bool check_joy)
{
	if (!slew_obj || slew_obj->control_type != CT_SLEW) return 0;

	vms_angvec rotang;
	if (check_keys) 
	{
		if (Function_mode == FMODE_EDITOR) 
		{
			obj->mtype.phys_info.velocity.x += VEL_SPEED * (key_down_time(KEY_PAD9) - key_down_time(KEY_PAD7));
			obj->mtype.phys_info.velocity.y += VEL_SPEED * (key_down_time(KEY_PADMINUS) - key_down_time(KEY_PADPLUS));
			obj->mtype.phys_info.velocity.z += VEL_SPEED * (key_down_time(KEY_PAD8) - key_down_time(KEY_PAD2));

			rotang.p = (fixang)((key_down_time(KEY_LBRACKET) - key_down_time(KEY_RBRACKET)) / ROT_SPEED);
			rotang.b = (fixang)((key_down_time(KEY_PAD1) - key_down_time(KEY_PAD3)) / ROT_SPEED);
			rotang.h = (fixang)((key_down_time(KEY_PAD6) - key_down_time(KEY_PAD4)) / ROT_SPEED);
		}
		else 
		{
			obj->mtype.phys_info.velocity.x += VEL_SPEED * Controls.sideways_thrust_time;
			obj->mtype.phys_info.velocity.y += VEL_SPEED * Controls.vertical_thrust_time;
			obj->mtype.phys_info.velocity.z += VEL_SPEED * Controls.forward_thrust_time;

			rotang.p = (fixang)(Controls.pitch_time / ROT_SPEED);
			rotang.b = (fixang)(Controls.bank_time / ROT_SPEED);
			rotang.h = (fixang)(Controls.heading_time / ROT_SPEED);
		}
	}
	else
		rotang.p = rotang.b = rotang.h = 0;

	//check for joystick movement

	if (check_joy && joy_present && (Function_mode == FMODE_EDITOR)) 
	{
		int joy_x, joy_y;
		joy_get_pos(&joy_x, &joy_y);
		int btns = joy_get_btns();

		int joyx_moved = (abs(joy_x - old_joy_x) > JOY_NULL);
		int joyy_moved = (abs(joy_y - old_joy_y) > JOY_NULL);

		if (abs(joy_x) < JOY_NULL) joy_x = 0;
		if (abs(joy_y) < JOY_NULL) joy_y = 0;

		if (btns)
			if (!rotang.p) rotang.p = (uint16_t)fixmul(-joy_y * 512, FrameTime); else;
		else
			if (joyy_moved) obj->mtype.phys_info.velocity.z = -joy_y * 8192;

		if (!rotang.h) rotang.h = (uint16_t)fixmul(joy_x * 512, FrameTime);

		if (joyx_moved) old_joy_x = joy_x;
		if (joyy_moved) old_joy_y = joy_y;
	}

	bool moved = (rotang.p | rotang.b | rotang.h) != 0;

	vms_matrix rotmat, new_pm;
	vm_angles_2_matrix(&rotmat, &rotang);
	vm_matrix_x_matrix(&new_pm, &obj->orient, &rotmat);
	obj->orient = new_pm;
	vm_transpose_matrix(&new_pm);		//make those columns rows

	moved |= (obj->mtype.phys_info.velocity.x | obj->mtype.phys_info.velocity.y | obj->mtype.phys_info.velocity.z) != 0;

	vms_vector svel = obj->mtype.phys_info.velocity;
	vms_vector movement; //scaled velocity (per this frame)
	vm_vec_scale(&svel, FrameTime);		//movement in this frame
	vm_vec_rotate(&movement, &svel, &new_pm);

	//	obj->last_pos = obj->pos;
	vm_vec_add2(&obj->pos, &movement);

	moved |= (movement.x | movement.y | movement.z) != 0;

	if (moved)
		update_object_seg(obj);	//update segment id

	return moved;
}

//do slew for this frame
bool slew_frame(bool check_keys)
{
	return do_slew_movement(slew_obj, !check_keys, true);
}

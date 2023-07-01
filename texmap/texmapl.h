
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

 //	Local include file for texture map library.

extern	int prevmod(int val, int modulus);
extern	int succmod(int val, int modulus);

extern float compute_dx_dy(g3ds_tmap* t, int top_vertex, int bottom_vertex, float recip_dy);
extern void compute_y_bounds(g3ds_tmap* t, int* vlt, int* vlb, int* vrt, int* vrb, int* bottom_y_ind);

extern float compute_du_dy_lin(g3ds_tmap* t, int vlt, int vlb, float recip_dy);
extern float compute_du_dy_lin(g3ds_tmap* t, int vrt, int vrb, float recip_dy);
extern float compute_dv_dy_lin(g3ds_tmap* t, int vlt, int vlb, float recip_dy);
extern float compute_dv_dy_lin(g3ds_tmap* t, int vrt, int vrb, float recip_dy);

extern float fix_recip[];

extern void init_interface_vars_to_assembler(void);

#define FIX_RECIP_TABLE_SIZE	321


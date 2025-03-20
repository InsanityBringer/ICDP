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

#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "fix/fix.h"
#include "platform/mono.h"
#include "misc/error.h"
#include "2d/gr.h"
#include "2d/grdef.h"
#include "3d/3d.h"
#include "texmapl.h"
#include "scanline.h"

//TODO: Platform detection
#ifdef _M_X64
#include <smmintrin.h>
#endif

extern int	y_pointers[];

void Texmap::DrawScanlineFlat()
{
	uint8_t* dest;
	int x;

	//[ISB] godawful hack from the ASM
	if (fx_y > window_bottom)
		return;

	dest = (uint8_t*)(write_buffer + fx_xleft + (bytes_per_row * fx_y));

	for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
	{
		*dest = tmap_flat_color;
		dest++;
	}
}

void Texmap::DrawScanlineShaded()
{
	int fade;
	uint8_t* dest;
	int x;

	//[ISB] godawful hack from the ASM
	if (fx_y > window_bottom)
		return;

	dest = (uint8_t*)(write_buffer + fx_xleft + (bytes_per_row * fx_y));

	fade = tmap_flat_shade_value << 8;
	for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
	{
		*dest = gr_fade_table[fade | (*dest)];
		dest++;
	}
}

void Texmap::DrawScanlineLinearNoLight()
{
	uint8_t* dest;
	uint32_t c;
	int x;
	float u, v, dudx, dvdx;

	//godawful hack
	if (fx_xleft < 0) fx_xleft = 0;

	u = fx_u;
	v = fx_v * 64;
	dudx = fx_du_dx;
	dvdx = fx_dv_dx * 64;

	dest = (uint8_t*)(write_buffer + y_pointers[fx_y] + fx_xleft);

	if (!Transparency_on) 
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
		{
			*dest = (uint32_t)pixptr[((int)v & (64 * 63)) + ((int)u & 63)];
			dest++;
			u += dudx;
			v += dvdx;
		}
	}
	else 
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
		{
			c = (uint32_t)pixptr[((int)v & (64 * 63)) + ((int)u & 63)];
			if (c != 255)
				* dest = c;
			dest++;
			u += dudx;
			v += dvdx;
		}
	}
}


void Texmap::DrawScanlineLinear()
{
	uint8_t* dest;
	uint32_t c;
	int x;
	float u, v, l, dudx, dvdx, dldx;

	//godawful hack
	if (fx_xleft < 0) fx_xleft = 0;

	u = fx_u;
	v = fx_v;
	dudx = fx_du_dx;
	dvdx = fx_dv_dx;

	l = fx_l * 256;
	dldx = fx_dl_dx * 256;
	if (dldx < 0)
		dldx += f2fl(1); //round towards 0 for negative deltas

	dest = write_buffer + y_pointers[fx_y] + fx_xleft;

	if (!Transparency_on) 
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
		{
			*dest = gr_fade_table[((int)l & 0xff00) + (uint32_t)pixptr[(((int)v & 63) << 6) + ((int)u & 63)]];
			dest++;
			l += dldx;
			u += dudx;
			v += dvdx;
		}
	}
	else 
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
		{
			c = (uint32_t)pixptr[(((int)v & 63) << 6) + ((int)u & 63)];
			if (c != 255)
				* dest = gr_fade_table[((int)l & 0xff00) + c];
			dest++;
			l += dldx;
			u += dudx;
			v += dvdx;
		}
	}
}

void Texmap::DrawScanlinePerspectiveNoLight()
{
	uint8_t* dest;
	uint32_t c;
	int x;
	float u, v, z, dudx, dvdx, dzdx;

	//godawful hack
	if (fx_xleft < 0) fx_xleft = 0;

	u = fx_u;
	v = fx_v;
	z = fx_z;
	dudx = fx_du_dx;
	dvdx = fx_dv_dx;
	dzdx = fx_dz_dx;

	dest = write_buffer + y_pointers[fx_y] + fx_xleft;

	if (!Transparency_on) 
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
		{
			*dest = (uint32_t)pixptr[(((int)(v / z) & 63) << 6) + ((int)(u / z) & 63)];
			dest++;
			u += dudx;
			v += dvdx;
			z += dzdx;
		}
	}
	else 
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
		{
			c = (uint32_t)pixptr[(((int)(v / z) & 63) << 6) + ((int)(u / z) & 63)];

			if (c != 255)
				*dest = c;
			dest++;
			u += dudx;
			v += dvdx;
			z += dzdx;
		}
	}
}

#ifdef _M_X64
#ifdef _MSC_VER
__inline int InlineFloor(float f)
#else
int InlineFloor(float f) //TODO: inline traits for other compilers
#endif
{
	__m128 reg = _mm_set_ss(f);
	reg = _mm_round_ss(reg, reg, (_MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC));
	return _mm_cvt_ss2si(reg);
}
#else //TODO: Add ARM version, for macos and others. 
int InlineFloor(float f)
{
	return (int)floor(f);
}
#endif

void Texmap::DrawScanlinePerspective()
{
	uint8_t* dest;
	uint32_t c;
	int x;
	float u, v, z, l, dudx, dvdx, dzdx, dldx;

	//godawful hack
	if (fx_xleft < 0) fx_xleft = 0;

	u = fx_u;
	v = fx_v;
	z = fx_z;
	dudx = fx_du_dx;
	dvdx = fx_dv_dx;
	dzdx = fx_dz_dx;

	l = fx_l * 256;
	dldx = fx_dl_dx * 256;
	dest = write_buffer + y_pointers[fx_y] + fx_xleft;
	if (dldx < 0)
		dldx += f2fl(1); //round towards 0 for negative deltas

	if (!Transparency_on) 
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
		{
			*dest = gr_fade_table[((int)l & 0xff00) + (uint32_t)pixptr[((InlineFloor(v / z) & 63) * 64) + (InlineFloor(u / z) & 63)]];
			dest++;
			l += dldx;
			u += dudx;
			v += dvdx;
			z += dzdx;
			if (z == 0) return;
		}
	}
	else 
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
		{
			c = (uint32_t)pixptr[((InlineFloor(v / z) & 63) * 64) + (InlineFloor(u / z) & 63)];
			if (c != 255)
				*dest = gr_fade_table[((int)l & 0xff00) + c];
			dest++;
			l += dldx;
			u += dudx;
			v += dvdx;
			z += dzdx;
			if (z == 0) return;
		}
	}
}

#define NBITS 4
#define ZSHIFT 4

fix pdiv(int a, int b)
{
	return (fix)((((int64_t)a << (16 - ZSHIFT)) / b) << (ZSHIFT));
}

constexpr float pdivfl(float a, float b)
{
	return (a / b);
}

//These aren't inlined in debug mode so it runs slow..
/*
constexpr void c_tmap_scanline_pln_loop(uint8_t*& dest, uint8_t* pixptr, float& u, float& v, float& z, float& l, float du, float dv, float dl)
{
	*dest = gr_fade_table[(((int)l * 256) & 0xff00) + (uint32_t)pixptr[(((int)v & 63) * 64) + ((int)u & 63)]];
	dest++;
	u += du; v += dv; l += dl;
}

constexpr void c_tmap_scanline_plt_loop(uint8_t*& dest, uint8_t* pixptr, float& u, float& v, float& z, float& l, float du, float dv, float dl)
{
	uint8_t c = pixptr[(((int)v & 63) * 64) + ((int)u & 63)];
	if (c != 255)
		*dest = gr_fade_table[(((int)l * 256) & (0xff00)) + c];
	dest++;
	u += du; v += dv; l += dl;	
	c = pixptr[(((int)v & 63) * 64) + ((int)u & 63)]; \
	if (c != 255) \
		*dest = gr_fade_table[(((int)l * 256) & (0xff00)) + c]; \
	dest++; \
	u += du; v += dv; l += dl; \
}*/

//This LCG isn't ideal, but atm I want this to always be inlined if at all possible. 
//It also needs to be thread local, for the multithreaded renderer
thread_local int g_light_rand_seed = 1;
#define C_TMAP_RND						(g_light_rand_seed = g_light_rand_seed * 1103515245 + 12345, (g_light_rand_seed >> 16) & 0xFF)

#if 1
#define C_TMAP_SCANLINE_PLN_LOOP        *dest = gr_fade_table[((int)l & 0xff00) + (uint32_t)pixptr[(((int)InlineFloor(vt) & 63) * 64) + ((int)InlineFloor(ut) & 63)]]; \
										dest++; \
										ut += ui; vt += vi; l += dldx;
#else
#define C_TMAP_SCANLINE_PLN_LOOP        samplel = (int)l;\
										if ((samplel & 0xFF) >= C_TMAP_RND && (samplel & 0xFF00) < ((NUM_LIGHTING_LEVELS - 1) << 8))\
											samplel += 256;\
										*dest = gr_fade_table[(samplel & 0xff00) + (uint32_t)pixptr[(((int)InlineFloor(vt) & 63) * 64) + ((int)InlineFloor(ut) & 63)]]; \
										dest++; \
										ut += ui; vt += vi; l += dldx;
#endif

#define C_TMAP_SCANLINE_PLT_LOOP 		c = pixptr[(((int)InlineFloor(vt) & 63) << 6) + ((int)InlineFloor(ut) & 63)]; \
										if (c != 255) \
											*dest = gr_fade_table[((int)l & (0xff00)) + c]; \
										dest++; \
										ut += ui; vt += vi; l += dldx; \

void Texmap::DrawScanlinePerspectivePer16()
{
	uint8_t* dest;
	uint32_t c;
	int x;
	short cl;

	//godawful hack
	if (fx_xleft < 0) fx_xleft = 0;

	float u = fx_u;
	float v = fx_v;
	float z = fx_z;
	float dudx = fx_du_dx;
	float dvdx = fx_dv_dx;
	float dzdx = fx_dz_dx;

	float l = fx_l * 256;
	int samplel;
	float dldx = fx_dl_dx * 256;
	dest = write_buffer + y_pointers[fx_y] + fx_xleft;
	if (dldx < 0)
		dldx += f2fl(1); //round towards 0 for negative deltas

	loop_count = fx_xright - fx_xleft + 1;

	num_left_over = (loop_count & ((1 << NBITS) - 1));
	loop_count >>= NBITS;

	V0 = v / z;
	U0 = u / z;

	dudx = fx_du_dx * 16;
	dvdx = fx_dv_dx * 16;
	dzdx = fx_dz_dx * 16;

	for (x = loop_count; x > 0; x--)
	{
		u += dudx;
		v += dvdx;
		z += dzdx;
		if (z == 0) return;

		V1 = v / z;
		U1 = u / z;

		ut = U0; vt = V0;
		ui = (U1 - U0) * (1.0f/16); vi = (V1 - V0) * (1.0f/16);

		U0 = U1;
		V0 = V1;

		if (!Transparency_on)
		{
#ifdef NDEBUG
			for (int i = 0; i < 16; i++)
			{
				C_TMAP_SCANLINE_PLN_LOOP
			}
#else
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
#endif
		}
		else
		{
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
			C_TMAP_SCANLINE_PLT_LOOP
		}
	}

	if (num_left_over == 0) return;

	
	if (!Transparency_on)
	{
		for (x = num_left_over; x > 0; --x)
		{
			*dest = gr_fade_table[((int)l & 0xff00) + (uint32_t)pixptr[(((int)floor(v / z) & 63) << 6) + ((int)floor(u / z) & 63)]];
			dest++;
			//*dest++ = 15;
			l += dldx;
			u += fx_du_dx;
			v += fx_dv_dx;
			z += fx_dz_dx;
			if (z == 0) return;
		}
	}
	else
	{
		for (x = num_left_over; x > 0; --x)
		{
			c = (uint32_t)pixptr[(((int)InlineFloor(v / z) & 63) << 6) + ((int)InlineFloor(u / z) & 63)];
			//c = 15;
			if (c != 255)
				*dest = gr_fade_table[((int)l & 0xff00) + c];
			dest++;
			l += dldx;
			u += fx_du_dx;
			v += fx_dv_dx;
			z += fx_dz_dx;
			if (z == 0) return;
		}
	}
}

//even and odd
#define C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP 		*dest = (uint32_t)pixptr[(((int)InlineFloor(vt) & 63) * 64) + ((int)InlineFloor(ut) & 63)];\
										dest++;\
										ut += ui;\
										vt += vi;\
										l += dldx;\

#define C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP 	c = (uint32_t)pixptr[(((int)InlineFloor(vt) & 63) * 64) + ((int)InlineFloor(ut) & 63)];\
										if (c != 255)\
										*dest = c;\
										dest++;\
										ut += ui;\
										vt += vi;\
										l += dldx;\

void Texmap::DrawScanlinePerspectivePer16NoLight()
{
	uint32_t c;
	int x;
	short cl;

	//godawful hack
	if (fx_xleft < 0) fx_xleft = 0;

	float u = fx_u;
	float v = fx_v;
	float z = fx_z;
	float dudx = fx_du_dx;
	float dvdx = fx_dv_dx;
	float dzdx = fx_dz_dx;

	float l = fx_l;
	float dldx = fx_dl_dx;
	uint8_t* dest = (uint8_t*)(write_buffer + y_pointers[fx_y] + fx_xleft);
	if (dldx < 0)
		dldx += f2fl(1); //round towards 0 for negative deltas

	loop_count = fx_xright - fx_xleft + 1;

	num_left_over = (loop_count & ((1 << NBITS) - 1));
	loop_count >>= NBITS;

	V0 = v / z;
	U0 = u / z;

	dudx = fx_du_dx * 16;
	dvdx = fx_dv_dx * 16;
	dzdx = fx_dz_dx * 16;

	for (x = loop_count; x > 0; x--)
	{
		u += dudx;
		v += dvdx;
		z += dzdx;
		if (z == 0) return;

		V1 = v / z;
		U1 = u / z;

		ut = U0; vt = V0;
		ui = (U1 - U0) / 16; vi = (V1 - V0) / 16;

		U0 = U1;
		V0 = V1;

		if (!Transparency_on)
		{
			C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP
		}
		else
		{
			C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
			C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP
		}
	}

	if (num_left_over == 0) return;

	if (!Transparency_on)
	{
		for (x = num_left_over; x > 0; --x)
		{
			*dest = pixptr[(((int)InlineFloor(v / z) & 63) << 6) + ((int)InlineFloor(u / z) & 63)];
			dest++;
			//*dest++ = 15;
			l += dldx;
			u += fx_du_dx;
			v += fx_dv_dx;
			z += fx_dz_dx;
			if (z == 0) return;
		}
	}
	else
	{
		for (x = num_left_over; x > 0; --x)
		{
			c = (uint32_t)pixptr[(((int)InlineFloor(v / z) & 63) << 6) + ((int)InlineFloor(u / z) & 63)];
			//c = 15;
			if (c != 255)
				*dest = c;
			dest++;
			l += dldx;
			u += fx_du_dx;
			v += fx_dv_dx;
			z += fx_dz_dx;
			if (z == 0) return;
		}
	}
}

#define zonk 1

#define C_TMAP_SCANLINE_PLN_EDITOR_LOOP 	*dest = zonk;\
										dest++;\
										ut += ui;\
										vt += vi;\
										l += dldx;\

#define C_TMAP_SCANLINE_PLT_EDITOR_LOOP 	c = pixptr[(((int)vt & 63) * 64) + ((int)ut & 63)];\
										if (c != 255)\
										*dest = zonk;\
										dest++;\
										ut += ui;\
										vt += vi;\
										l += dldx;\

void Texmap::DrawScanlineEditor()
{
	uint32_t c;
	int x;

	//godawful hack
	if (fx_xleft < 0) fx_xleft = 0;

	float u = fx_u;
	float v = fx_v;
	float z = fx_z;
	float dudx = fx_du_dx;
	float dvdx = fx_dv_dx;
	float dzdx = fx_dz_dx;

	float l = fx_l;
	float dldx = fx_dl_dx;
	uint8_t* dest = write_buffer + y_pointers[fx_y] + fx_xleft;
	if (dldx < 0)
		dldx += f2fl(1); //round towards 0 for negative deltas

	loop_count = fx_xright - fx_xleft + 1;

	num_left_over = (loop_count & ((1 << NBITS) - 1));
	loop_count >>= NBITS;

	V0 = v / z;
	U0 = u / z;

	dudx = fx_du_dx * 16;
	dvdx = fx_dv_dx * 16;
	dzdx = fx_dz_dx * 16;

	for (x = loop_count; x > 0; x--)
	{
		u += dudx;
		v += dvdx;
		z += dzdx;
		if (z == 0) return;

		V1 = v / z;
		U1 = u / z;

		ut = U0; vt = V0;
		ui = (U1 - U0) / 16; vi = (V1 - V0) / 16;

		U0 = U1;
		V0 = V1;

		if (!Transparency_on)
		{
			C_TMAP_SCANLINE_PLN_EDITOR_LOOP
			C_TMAP_SCANLINE_PLN_EDITOR_LOOP
			C_TMAP_SCANLINE_PLN_EDITOR_LOOP
			C_TMAP_SCANLINE_PLN_EDITOR_LOOP
			C_TMAP_SCANLINE_PLN_EDITOR_LOOP
			C_TMAP_SCANLINE_PLN_EDITOR_LOOP
			C_TMAP_SCANLINE_PLN_EDITOR_LOOP
			C_TMAP_SCANLINE_PLN_EDITOR_LOOP
			C_TMAP_SCANLINE_PLN_EDITOR_LOOP
			C_TMAP_SCANLINE_PLN_EDITOR_LOOP
			C_TMAP_SCANLINE_PLN_EDITOR_LOOP
			C_TMAP_SCANLINE_PLN_EDITOR_LOOP
			C_TMAP_SCANLINE_PLN_EDITOR_LOOP
			C_TMAP_SCANLINE_PLN_EDITOR_LOOP
			C_TMAP_SCANLINE_PLN_EDITOR_LOOP
			C_TMAP_SCANLINE_PLN_EDITOR_LOOP
		}
		else
		{
			C_TMAP_SCANLINE_PLT_EDITOR_LOOP
			C_TMAP_SCANLINE_PLT_EDITOR_LOOP
			C_TMAP_SCANLINE_PLT_EDITOR_LOOP
			C_TMAP_SCANLINE_PLT_EDITOR_LOOP
			C_TMAP_SCANLINE_PLT_EDITOR_LOOP
			C_TMAP_SCANLINE_PLT_EDITOR_LOOP
			C_TMAP_SCANLINE_PLT_EDITOR_LOOP
			C_TMAP_SCANLINE_PLT_EDITOR_LOOP
			C_TMAP_SCANLINE_PLT_EDITOR_LOOP
			C_TMAP_SCANLINE_PLT_EDITOR_LOOP
			C_TMAP_SCANLINE_PLT_EDITOR_LOOP
			C_TMAP_SCANLINE_PLT_EDITOR_LOOP
			C_TMAP_SCANLINE_PLT_EDITOR_LOOP
			C_TMAP_SCANLINE_PLT_EDITOR_LOOP
			C_TMAP_SCANLINE_PLT_EDITOR_LOOP
			C_TMAP_SCANLINE_PLT_EDITOR_LOOP
		}
	}

	if (num_left_over == 0) return;

	if (!Transparency_on)
	{
		for (x = num_left_over; x > 0; --x)
		{
			*dest = zonk;
			dest++;
			l += dldx;
			u += fx_du_dx;
			v += fx_dv_dx;
			z += fx_dz_dx;
			if (z == 0) return;
		}
	}
	else
	{
		for (x = num_left_over; x > 0; --x)
		{
			c = (uint32_t)pixptr[(((int)(v / z) & 63) * 64) + ((int)(u / z) & 63)];
			if (c != 255)
				*dest = zonk;
			dest++;
			l += dldx;
			u += fx_du_dx;
			v += fx_dv_dx;
			z += fx_dz_dx;
			if (z == 0) return;
		}
	}
}

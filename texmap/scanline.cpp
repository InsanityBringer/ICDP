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
	fix u, v, dudx, dvdx;

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
			*dest = (uint32_t)pixptr[(f2i(v) & (64 * 63)) + (f2i(u) & 63)];
			dest++;
			u += dudx;
			v += dvdx;
		}
	}
	else 
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
		{
			c = (uint32_t)pixptr[(f2i(v) & (64 * 63)) + (f2i(u) & 63)];
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
	fix u, v, l, dudx, dvdx, dldx;

	//godawful hack
	if (fx_xleft < 0) fx_xleft = 0;

	u = fx_u;
	v = fx_v;
	dudx = fx_du_dx;
	dvdx = fx_dv_dx;

	l = fx_l >> 8;
	dldx = fx_dl_dx >> 8;
	if (dldx < 0)
		dldx++; //round towards 0 for negative deltas

	dest = (uint8_t*)(write_buffer + y_pointers[fx_y] + fx_xleft);

	if (!Transparency_on) 
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
		{
			*dest = gr_fade_table[(l & (0xff00)) + (uint32_t)pixptr[((f2i(v) & 63) * 64) + (f2i(u) & 63)]];
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
			c = (uint32_t)pixptr[((f2i(v) & 63) * 64) + (f2i(u) & 63)];
			if (c != 255)
				* dest = gr_fade_table[(l & (0xff00)) + c];
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
	fix u, v, z, dudx, dvdx, dzdx;

	//godawful hack
	if (fx_xleft < 0) fx_xleft = 0;

	u = fx_u;
	v = fx_v;
	z = fx_z;
	dudx = fx_du_dx;
	dvdx = fx_dv_dx;
	dzdx = fx_dz_dx;

	dest = (uint8_t*)(write_buffer + y_pointers[fx_y] + fx_xleft);

	if (!Transparency_on) 
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
		{
			*dest = (uint32_t)pixptr[(((v / z) & 63) << 6) + ((u / z) & 63)];
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
			c = (uint32_t)pixptr[(((v / z) & 63) << 6) + ((u / z) & 63)];

			if (c != 255)
				*dest = c;
			dest++;
			u += dudx;
			v += dvdx;
			z += dzdx;
		}
	}
}

void Texmap::DrawScanlinePerspective()
{
	uint8_t* dest;
	uint32_t c;
	int x;
	fix u, v, z, l, dudx, dvdx, dzdx, dldx;

	//godawful hack
	if (fx_xleft < 0) fx_xleft = 0;

	u = fx_u;
	v = fx_v;
	z = fx_z;
	dudx = fx_du_dx;
	dvdx = fx_dv_dx;
	dzdx = fx_dz_dx;

	l = fx_l >> 8;
	dldx = fx_dl_dx >> 8;
	dest = (uint8_t*)(write_buffer + y_pointers[fx_y] + fx_xleft);
	if (dldx < 0)
		dldx++; //round towards 0 for negative deltas

	if (!Transparency_on) 
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x) 
		{
			*dest = gr_fade_table[(l & (0xff00)) + (uint32_t)pixptr[(((v / z) & 63) * 64) + ((u / z) & 63)]];
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
			c = (uint32_t)pixptr[(((v / z) & 63) * 64) + ((u / z) & 63)];
			if (c != 255)
				*dest = gr_fade_table[(l & (0xff00)) + c];
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
uint16_t ut, vt;
uint16_t ui, vi;
int uvt, uvi;
fix U0, V0, Z0, U1, V1;

fix pdiv(int a, int b)
{
	return (fix)((((int64_t)a << ZSHIFT) / b) << (16 - ZSHIFT));
}

//even and odd
#define C_TMAP_SCANLINE_PLN_LOOP        *dest = gr_fade_table[(l & (0xff00)) + (uint32_t)pixptr[(((uvt >> 10) & 63) | ((uvt >> 20) & 4032))]];\
										dest++; \
										uvt += uvi;\
										l += dldx;\
										*dest = gr_fade_table[(l & (0xff00)) + (uint32_t)pixptr[(((uvt >> 10) & 63) | ((uvt >> 20) & 4032))]];\
										dest++; \
										uvt += uvi;\
										l += dldx;

#define C_TMAP_SCANLINE_PLT_LOOP 		c = (uint32_t)pixptr[(((uvt >> 10) & 63) | ((uvt >> 20) & 4032))];\
										uvt += uvi;\
										if (c != 255)\
										*dest = gr_fade_table[(l & (0xff00)) + c];\
										dest++;\
										l += dldx;\
										c = (uint32_t)pixptr[(((uvt >> 10) & 63) | ((uvt >> 20) & 4032))];\
										uvt += uvi;\
										if (c != 255)\
										*dest = gr_fade_table[(l & (0xff00)) + c];\
										dest++;\
										l += dldx;

#define C_TMAP_SCANLINE_PLN_LOOP_F 				*dest = gr_fade_table[(l & (0xff00)) + (uint32_t)pixptr[(((uvt >> 10) & 63) | ((uvt >> 20) & 4032))]];\
												dest++; \
												uvt += uvi;\
												l += dldx;\
												if (--num_left_over == 0) return;\
												*dest = gr_fade_table[(l & (0xff00)) + (uint32_t)pixptr[(((uvt >> 10) & 63) | ((uvt >> 20) & 4032))]];\
												dest++; \
												uvt += uvi;\
												l += dldx;\
												if (--num_left_over == 0) return;

#define C_TMAP_SCANLINE_PLT_LOOP_F 		c = (uint32_t)pixptr[(((uvt >> 10) & 63) | ((uvt >> 20) & 4032))];\
										uvt += uvi;\
										if (c != 255)\
										*dest = gr_fade_table[(l & (0xff00)) + c];\
										dest++;\
										l += dldx;\
										if (--num_left_over == 0) return;\
										c = (uint32_t)pixptr[(((uvt >> 10) & 63) | ((uvt >> 20) & 4032))];\
										uvt += uvi;\
										if (c != 255)\
										*dest = gr_fade_table[(l & (0xff00)) + c];\
										dest++;\
										l += dldx;\
										if (--num_left_over == 0) return;

int loop_count, num_left_over;
dbool new_end;

void Texmap::DrawScanlinePerspectivePer16()
{
	uint8_t* dest;
	uint32_t c;
	int x;
	fix u, v, z, l, dudx, dvdx, dzdx, dldx;
	short cl;

	//godawful hack
	if (fx_xleft < 0) fx_xleft = 0;

	u = fx_u;
	v = fx_v;
	z = fx_z;
	dudx = fx_du_dx;
	dvdx = fx_dv_dx;
	dzdx = fx_dz_dx;

	l = fx_l >> 8;
	dldx = fx_dl_dx >> 8;
	dest = (uint8_t*)(write_buffer + y_pointers[fx_y] + fx_xleft);
	if (dldx < 0)
		dldx++; //round towards 0 for negative deltas

	loop_count = fx_xright - fx_xleft + 1;

	//This isn't acutally important for this code, since I don't write words at a time, but it has visual artificating in Descent.
	//TODO: Should I look into doing words/dword writes? Register caching might make it faster (build register of pixels, write that once), but on 32-bit x86 platforms this seems sketchy.
	while ((uintptr_t)(dest) & 3)
	{
		c = (uint32_t)pixptr[(((v / z) & 63) << 6) + ((u / z) & 63)];
		if (c != 255)
			*dest = gr_fade_table[(l & (0xff00)) + c]; //oh yeah first ~3 pixels don't check transparency, not that it's relevant
		dest++;
		l += dldx;
		u += dudx;
		v += dvdx;
		z += dzdx;
		if (z == 0) return;
		if (--loop_count == 0) return; //none to do anymore
	}

	num_left_over = (loop_count & ((1 << NBITS) - 1));
	loop_count >>= NBITS;

	V0 = pdiv(v, z);
	U0 = pdiv(u, z);

	dudx = fx_du_dx << NBITS;
	dvdx = fx_dv_dx << NBITS;
	dzdx = fx_dz_dx << NBITS;

	for (x = loop_count; x > 0; x--)
	{
		u += dudx;
		v += dvdx;
		z += dzdx;
		if (z == 0) return;

		V1 = pdiv(v, z);
		U1 = pdiv(u, z);

		uvt = ((U0 >> 6) & 0xFFFF) | ((V0 >> 6) << 16);
		uvi = (((U1 - U0) >> (NBITS + 6)) & 0xFFFF) | (((V1 - V0) >> (NBITS + 6)) << 16);

		U0 = U1;
		V0 = V1;

		if (!Transparency_on)
		{
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
			C_TMAP_SCANLINE_PLN_LOOP
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
		}
	}

	if (num_left_over == 0) return;

	int zcmp = z * 2 + z;
	int localz = z + dzdx;

	if (localz >= 0)
	{
		localz <<= 2;
		if (zcmp < localz) //Under certain circumstances, the weirder finishing code can be used. Replicate this.
		{
			u += dudx;
			v += dvdx;
			z += dzdx;
			if (z == 0) return;

			cl = 1;
			//z went negative.
			//this can happen because we added DZ1 to the current z, but dz1 represents dz for perhaps 16 pixels
			//though we might only plot one more pixel.
			while (z < 0 && cl != NBITS)
			{
				u -= (dudx >> cl);
				v -= (dvdx >> cl);
				z -= (dzdx >> cl);

				cl++;
			}
			if (z <= (1 << (ZSHIFT + 1)))
			{
				z = (1 << (ZSHIFT + 1));
			}

			V1 = pdiv(v, z);
			U1 = pdiv(u, z);

			uvt = ((U0 >> 6) & 0xFFFF) | ((V0 >> 6) << 16);
			uvi = (((U1 - U0) >> (NBITS + 6)) & 0xFFFF) | (((V1 - V0) >> (NBITS + 6)) << 16);

			U0 = U1;
			V0 = V1;

			if (!Transparency_on)
			{
				C_TMAP_SCANLINE_PLN_LOOP_F
					C_TMAP_SCANLINE_PLN_LOOP_F
					C_TMAP_SCANLINE_PLN_LOOP_F
					C_TMAP_SCANLINE_PLN_LOOP_F
					C_TMAP_SCANLINE_PLN_LOOP_F
					C_TMAP_SCANLINE_PLN_LOOP_F
					C_TMAP_SCANLINE_PLN_LOOP_F
					C_TMAP_SCANLINE_PLN_LOOP_F
			}
			else
			{
				C_TMAP_SCANLINE_PLT_LOOP_F
					C_TMAP_SCANLINE_PLT_LOOP_F
					C_TMAP_SCANLINE_PLT_LOOP_F
					C_TMAP_SCANLINE_PLT_LOOP_F
					C_TMAP_SCANLINE_PLT_LOOP_F
					C_TMAP_SCANLINE_PLT_LOOP_F
					C_TMAP_SCANLINE_PLT_LOOP_F
					C_TMAP_SCANLINE_PLT_LOOP_F
			}

			Int3();
			return;
		}
	}
	
	
	if (!Transparency_on)
	{
		for (x = num_left_over; x > 0; --x)
		{
			*dest = gr_fade_table[(l & (0xff00)) + (uint32_t)pixptr[(((v / z) & 63) << 6) + ((u / z) & 63)]];
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
			c = (uint32_t)pixptr[(((v / z) & 63) << 6) + ((u / z) & 63)];
			//c = 15;
			if (c != 255)
				*dest = gr_fade_table[(l & (0xff00)) + c];
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
#define C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP 		*dest = (uint32_t)pixptr[(((uvt >> 10) & 63) | ((uvt >> 20) & 4032))];\
										dest++;\
										uvt += uvi;\
										l += dldx;\
										*dest = (uint32_t)pixptr[(((uvt >> 10) & 63) | ((uvt >> 20) & 4032))];\
										dest++; \
										uvt += uvi;\
										l += dldx;

#define C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP 	c = (uint32_t)pixptr[(((uvt >> 10) & 63) | ((uvt >> 20) & 4032))];\
										uvt += uvi;\
										if (c != 255)\
										*dest = c;\
										dest++;\
										l += dldx;\
										c = (uint32_t)pixptr[(((uvt >> 10) & 63) | ((uvt >> 20) & 4032))];\
										uvt += uvi;\
										if (c != 255)\
										*dest = c;\
										dest++;\
										l += dldx;

#define C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP_F		*dest = (uint32_t)pixptr[(((uvt >> 10) & 63) | ((uvt >> 20) & 4032))];\
												dest++; \
												uvt += uvi;\
												l += dldx;\
												if (--num_left_over == 0) return;\
												*dest = (uint32_t)pixptr[(((uvt >> 10) & 63) | ((uvt >> 20) & 4032))];\
												dest++; \
												uvt += uvi;\
												l += dldx;\
												if (--num_left_over == 0) return;

#define C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP_F 		c = pixptr[(((uvt >> 10) & 63) | ((uvt >> 20) & 4032))];\
										uvt += uvi;\
										if (c != 255)\
										*dest = c;\
										dest++;\
										l += dldx;\
										if (--num_left_over == 0) return;\
										c = (uint32_t)pixptr[(((uvt >> 10) & 63) | ((uvt >> 20) & 4032))];\
										uvt += uvi;\
										if (c != 255)\
										*dest = c;\
										dest++;\
										l += dldx;\
										if (--num_left_over == 0) return;

void Texmap::DrawScanlinePerspectivePer16NoLight()
{
	uint8_t* dest;
	uint32_t c;
	int x;
	fix u, v, z, l, dudx, dvdx, dzdx, dldx;
	short cl;

	//godawful hack
	if (fx_xleft < 0) fx_xleft = 0;

	u = fx_u;
	v = fx_v;
	z = fx_z;
	dudx = fx_du_dx;
	dvdx = fx_dv_dx;
	dzdx = fx_dz_dx;

	l = fx_l >> 8;
	dldx = fx_dl_dx >> 8;
	dest = (uint8_t*)(write_buffer + y_pointers[fx_y] + fx_xleft);
	if (dldx < 0)
		dldx++; //round towards 0 for negative deltas

	loop_count = fx_xright - fx_xleft + 1;

	//Painful code to try to replicate the ASM drawer. aaa
	//Wants to be dword aligned.
	while ((uintptr_t)(dest) & 3)
	{
		c = (uint32_t)pixptr[(((v / z) & 63) << 6) + ((u / z) & 63)];
		if (c != 255)
			*dest = c; //oh yeah first ~3 pixels don't check transparency, not that it's relevant
		dest++;
		l += dldx;
		u += dudx;
		v += dvdx;
		z += dzdx;
		if (z == 0) return;
		if (--loop_count == 0) return; //none to do anymore
	}

	num_left_over = (loop_count & ((1 << NBITS) - 1));
	loop_count >>= NBITS;

	V0 = pdiv(v, z);
	U0 = pdiv(u, z);

	dudx = fx_du_dx << NBITS;
	dvdx = fx_dv_dx << NBITS;
	dzdx = fx_dz_dx << NBITS;

	for (x = loop_count; x > 0; x--)
	{
		u += dudx;
		v += dvdx;
		z += dzdx;
		if (z == 0) return;

		V1 = pdiv(v, z);
		U1 = pdiv(u, z);

		uvt = ((U0 >> 6) & 0xFFFF) | ((V0 >> 6) << 16);
		uvi = (((U1 - U0) >> (NBITS + 6)) & 0xFFFF) | (((V1 - V0) >> (NBITS + 6)) << 16);

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
		}
	}

	if (num_left_over == 0) return;

	int zcmp = z * 2 + z;
	int localz = z + dzdx;

	if (localz >= 0)
	{
		localz <<= 2;
		if (zcmp < localz) //Under certain circumstances, the weirder finishing code can be used. Replicate this.
		{
			u += dudx;
			v += dvdx;
			z += dzdx;
			if (z == 0) return;

			cl = 1;
			//z went negative.
			//this can happen because we added DZ1 to the current z, but dz1 represents dz for perhaps 16 pixels
			//though we might only plot one more pixel.
			while (z < 0 && cl != NBITS)
			{
				u -= (dudx >> cl);
				v -= (dvdx >> cl);
				z -= (dzdx >> cl);

				cl++;
			}
			if (z <= (1 << (ZSHIFT + 1)))
			{
				z = (1 << (ZSHIFT + 1));
			}

			V1 = pdiv(v, z);
			U1 = pdiv(u, z);

			uvt = ((U0 >> 6) & 0xFFFF) | ((V0 >> 6) << 16);
			uvi = (((U1 - U0) >> (NBITS + 6)) & 0xFFFF) | (((V1 - V0) >> (NBITS + 6)) << 16);

			U0 = U1;
			V0 = V1;

			if (!Transparency_on)
			{
				C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLN_NOLIGHT_LOOP_F
			}
			else
			{
				C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP_F
					C_TMAP_SCANLINE_PLT_NOLIGHT_LOOP_F
			}

			Int3();
			return;
		}
	}


	if (!Transparency_on)
	{
		for (x = num_left_over; x > 0; --x)
		{
			*dest = pixptr[(((v / z) & 63) << 6) + ((u / z) & 63)];
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
			c = (uint32_t)pixptr[(((v / z) & 63) << 6) + ((u / z) & 63)];
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

//[ISB] TODO: This isn't 100% correct to the original editor, since that based the editor tmap off of the per 16 texture mapper
void Texmap::DrawScanlineEditor()
{
	uint8_t* dest;
	uint32_t c;
	int x;
	fix u, v, z, l, dudx, dvdx, dzdx, dldx;

	//godawful hack
	if (fx_xleft < 0) fx_xleft = 0;

	u = fx_u;
	v = fx_v;
	z = fx_z;
	dudx = fx_du_dx;
	dvdx = fx_dv_dx;
	dzdx = fx_dz_dx;

	l = fx_l >> 8;
	dldx = fx_dl_dx >> 8;
	dest = (uint8_t*)(write_buffer + y_pointers[fx_y] + fx_xleft);
	if (dldx < 0)
		dldx++; //round towards 0 for negative deltas

	if (!Transparency_on)
	{
		for (x = fx_xright - fx_xleft + 1; x > 0; --x)
		{
			*dest = zonk;
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
			c = (uint32_t)pixptr[(((v / z) & 63) * 64) + ((u / z) & 63)];
			if (c != 255)
				*dest = zonk;
			dest++;
			l += dldx;
			u += dudx;
			v += dvdx;
			z += dzdx;
			if (z == 0) return;
		}
	}
}

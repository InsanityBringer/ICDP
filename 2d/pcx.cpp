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
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "2d/gr.h"
#include "mem/mem.h"
#include "2d/pcx.h"
#include "cfile/cfile.h"
#include "misc/error.h"

/* PCX Header data type */
struct PCXHeader
{
	uint8_t		Manufacturer;
	uint8_t		Version;
	uint8_t		Encoding;
	uint8_t		BitsPerPixel;
	short		Xmin;
	short		Ymin;
	short		Xmax;
	short		Ymax;
	short		Hdpi;
	short		Vdpi;
	uint8_t		ColorMap[16][3];
	uint8_t		Reserved;
	uint8_t		Nplanes;
	short		BytesPerLine;
	uint8_t		filler[60];

	void from_cfile(CFILE* fp)
	{
		Manufacturer = cfile_read_byte(fp);
		Version = cfile_read_byte(fp);
		Encoding = cfile_read_byte(fp);
		BitsPerPixel = cfile_read_byte(fp);
		Xmin = cfile_read_short(fp);
		Ymin = cfile_read_short(fp);
		Xmax = cfile_read_short(fp);
		Ymax = cfile_read_short(fp);
		Hdpi = cfile_read_short(fp);
		Vdpi = cfile_read_short(fp);

		for (int i = 0; i < 16; i++)
			cfread(ColorMap[i], 1, sizeof(ColorMap[i]), fp);

		Reserved = cfile_read_byte(fp);
		Nplanes = cfile_read_byte(fp);
		BytesPerLine = cfile_read_short(fp);
		cfread(filler, 1, sizeof(filler), fp);
	}

	void to_file(FILE* fp)
	{
		file_write_byte(fp, Manufacturer);
		file_write_byte(fp, Version);
		file_write_byte(fp, Encoding);
		file_write_byte(fp, BitsPerPixel);
		file_write_short(fp, Xmin);
		file_write_short(fp, Ymin);
		file_write_short(fp, Xmax);
		file_write_short(fp, Ymax);
		file_write_short(fp, Hdpi);
		file_write_short(fp, Vdpi);

		for (int i = 0; i < 16; i++)
			fwrite(ColorMap[i], 1, sizeof(ColorMap[i]), fp);

		file_write_byte(fp, Reserved);
		file_write_byte(fp, Nplanes);
		file_write_short(fp, BytesPerLine);
		fwrite(filler, 1, sizeof(filler), fp);
	}
};

int pcx_read_bitmap(const char* filename, grs_bitmap* bmp, int bitmap_type, uint8_t* palette)
{
	CFILE* PCXfile = cfopen(filename, "rb");
	if (!PCXfile)
		return PCX_ERROR_OPENING;

	// read 128 char PCX header
	PCXHeader header;
	header.from_cfile(PCXfile);

	// Is it a 256 color PCX file?
	if ((header.Manufacturer != 10) || (header.Encoding != 1) || (header.Nplanes != 1) || (header.BitsPerPixel != 8) || (header.Version != 5)) 
	{
		cfclose(PCXfile);
		return PCX_ERROR_WRONG_VERSION;
	}

	// Find the size of the image
	int xsize = header.Xmax - header.Xmin + 1;
	int ysize = header.Ymax - header.Ymin + 1;

	if (bitmap_type == BM_LINEAR) 
	{
		if (bmp->bm_data == NULL) 
		{
			memset(bmp, 0, sizeof(grs_bitmap));
			bmp->bm_data = (unsigned char*)malloc(xsize * ysize);
			if (bmp->bm_data == NULL) 
			{
				cfclose(PCXfile);
				return PCX_ERROR_MEMORY;
			}
			bmp->bm_w = bmp->bm_rowsize = xsize;
			bmp->bm_h = ysize;
			bmp->bm_type = bitmap_type;
		}
	}

	uint8_t data, * pixdata;
	for (int row = 0; row < ysize; row++) 
	{
		pixdata = &bmp->bm_data[bmp->bm_rowsize * row];
		for (int col = 0; col < xsize; ) 
		{
			if (cfread(&data, 1, 1, PCXfile) != 1) 
			{
				cfclose(PCXfile);
				return PCX_ERROR_READING;
			}
			if ((data & 0xC0) == 0xC0) 
			{
				int count = data & 0x3F;
				if (cfread(&data, 1, 1, PCXfile) != 1) 
				{
					cfclose(PCXfile);
					return PCX_ERROR_READING;
				}
				memset(pixdata, data, count);
				pixdata += count;
				col += count;
			}
			else 
			{
				*pixdata++ = data;
				col++;
			}
		}
	}

	// Read the extended palette at the end of PCX file
	if (palette != NULL) 
	{
		// Read in a character which should be 12 to be extended palette file
		if (cfread(&data, 1, 1, PCXfile) == 1) 
		{
			if (data == 12) 
			{
				if (cfread(palette, 768, 1, PCXfile) != 1) 
				{
					cfclose(PCXfile);
					return PCX_ERROR_READING;
				}
				for (int i = 0; i < 768; i++)
					palette[i] >>= 2;
			}
		}
		else 
		{
			cfclose(PCXfile);
			return PCX_ERROR_NO_PALETTE;
		}
	}
	cfclose(PCXfile);
	return PCX_ERROR_NONE;
}

int pcx_encode_line(uint8_t* inBuff, int inLen, FILE* fp);

int pcx_write_bitmap(const char* filename, grs_bitmap* bmp, uint8_t* palette)
{
	PCXHeader header;
	memset(&header, 0, sizeof(PCXHeader));

	header.Manufacturer = 10;
	header.Encoding = 1;
	header.Nplanes = 1;
	header.BitsPerPixel = 8;
	header.Version = 5;
	header.Xmax = bmp->bm_w - 1;
	header.Ymax = bmp->bm_h - 1;
	header.BytesPerLine = bmp->bm_w;

	FILE* PCXfile = fopen(filename, "wb");
	if (!PCXfile)
		return PCX_ERROR_OPENING;

	header.to_file(PCXfile);

	for (int i = 0; i < bmp->bm_h; i++) 
	{
		if (!pcx_encode_line(&bmp->bm_data[bmp->bm_rowsize * i], bmp->bm_w, PCXfile))
		{
			fclose(PCXfile);
			return PCX_ERROR_WRITING;
		}
	}

	// Mark an extended palette	
	uint8_t data = 12;
	if (fwrite(&data, 1, 1, PCXfile) != 1) 
	{
		fclose(PCXfile);
		return PCX_ERROR_WRITING;
	}

	// Write the extended palette
	for (int i = 0; i < 768; i++)
		palette[i] <<= 2;

	int retval = fwrite(palette, 768, 1, PCXfile);

	for (int i = 0; i < 768; i++)
		palette[i] >>= 2;

	if (retval != 1) 
	{
		fclose(PCXfile);
		return PCX_ERROR_WRITING;
	}

	fclose(PCXfile);
	return PCX_ERROR_NONE;

}

int pcx_encode_byte(uint8_t byt, uint8_t cnt, FILE* fid);

// returns number of bytes written into outBuff, 0 if failed 
int pcx_encode_line(uint8_t* inBuff, int inLen, FILE* fp)
{
	int total = 0;
	uint8_t last = *(inBuff);
	uint8_t runCount = 1;

	for (int srcIndex = 1; srcIndex < inLen; srcIndex++) 
	{
		uint8_t cur = *(++inBuff);
		if (cur == last) 
		{
			runCount++;			// it encodes
			if (runCount == 63) 
			{
				int i;
				if (!(i = pcx_encode_byte(last, runCount, fp)))
					return(0);
				total += i;
				runCount = 0;
			}
		}
		else // cur != last
		{
			if (runCount) 
			{
				int i;
				if (!(i = pcx_encode_byte(last, runCount, fp)))
					return(0);
				total += i;
			}
			last = cur;
			runCount = 1;
		}
	}

	if (runCount) // finish up
	{
		int i;
		if (!(i = pcx_encode_byte(last, runCount, fp)))
			return 0;
		return total + i;
	}
	return total;
}

// subroutine for writing an encoded byte pair 
// returns count of bytes written, 0 if error
int pcx_encode_byte(uint8_t byt, uint8_t cnt, FILE* fid)
{
	if (cnt) 
	{
		if ((cnt == 1) && (0xc0 != (0xc0 & byt))) 
		{
			if (EOF == putc((int)byt, fid))
				return 0; 	// disk write error (probably full)
			return 1;
		}
		else 
		{
			if (EOF == putc((int)0xC0 | cnt, fid))
				return 0; 	// disk write error
			if (EOF == putc((int)byt, fid))
				return 0; 	// disk write error
			return 2;
		}
	}
	return 0;
}

//text for error messges
const char* pcx_error_messages[] = 
{
	"No error.",
	"Error opening file.",
	"Couldn't read PCX header.",
	"Unsupported PCX version.",
	"Error reading data.",
	"Couldn't find palette information.",
	"Error writing data."
};

//function to return pointer to error message
const char* pcx_errormsg(int error_number)
{
	if (error_number < 0 || error_number >= 7)
		Error("pcx_errormsg: error_number %d out of range.", error_number);

	return pcx_error_messages[error_number];
}

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

#pragma once

#include "misc/types.h"
#include "fix/fix.h"
#include "vecmat/vecmat.h"

 // Version 1 - Initial version
 // Version 2 - Mike changed some shorts to bytes in segments, so incompatible!

#define	SIDE_IS_QUAD	1			// render side as quadrilateral
#define	SIDE_IS_TRI_02	2			// render side as two triangles, triangulated along edge from 0 to 2
#define	SIDE_IS_TRI_13	3			// render side as two triangles, triangulated along edge from 1 to 3

// Set maximum values for segment and face data structures.
#define	MAX_VERTICES_PER_SEGMENT	8
#define	MAX_SIDES_PER_SEGMENT		6
#define	MAX_VERTICES_PER_POLY		4
#define	WLEFT						0
#define	WTOP						1
#define	WRIGHT						2
#define	WBOTTOM						3
#define	WBACK						4
#define	WFRONT						5

#define	MAX_GAME_SEGMENTS			8000
#define	MAX_GAME_VERTICES			36000

#if defined(SHAREWARE) && !defined(EDITOR)
#define	MAX_SEGMENTS				MAX_GAME_SEGMENTS
#define	MAX_SEGMENT_VERTICES		MAX_GAME_VERTICES
#else
#define	MAX_SEGMENTS				9000
#define	MAX_SEGMENT_VERTICES		(4*MAX_SEGMENTS)
#endif

//normal everyday vertices

#define	DEFAULT_LIGHTING			0 // (F1_0/2)

#ifdef EDITOR	//verts for the new segment
#define	NUM_NEW_SEG_VERTICES		8
#define	NEW_SEGMENT_VERTICES		(MAX_SEGMENT_VERTICES)
#define	MAX_VERTICES				(MAX_SEGMENT_VERTICES+NUM_NEW_SEG_VERTICES)
#else			//No editor
#define	MAX_VERTICES				(MAX_SEGMENT_VERTICES)
#endif

//	Returns true if segnum references a child, else returns false.
//	Note that -1 means no connection, -2 means a connection to the outside world.
#define	IS_CHILD(segnum) (segnum > -1)

//Structure for storing u,v,light values. 
//NOTE: this structure should be the same as the one in 3d.h
struct uvl
{
	fix u, v, l;
};

struct side
{
	int8_t		type;									// replaces num_faces and tri_edge, 1 = quad, 2 = 0:2 triangulation, 3 = 1:3 triangulation
	uint8_t		pad;									//keep us longword alligned
	short		wall_num;
	short		tmap_num;
	short		tmap_num2;
	uvl			uvls[4];
	vms_vector	normals[2];						// 2 normals, if quadrilateral, both the same.
};

#ifdef EDITOR
#define SEGMENT_SIZEOF 522
#else
#define SEGMENT_SIZEOF sizeof(segment)
#endif

struct segment
{
#ifdef	EDITOR
	short			segnum;								// segment number, not sure what it means
#endif
	side			sides[MAX_SIDES_PER_SEGMENT];		// 6 sides
	short			children[MAX_SIDES_PER_SEGMENT];	// indices of 6 children segments, front, left, top, right, bottom, back
	unsigned short	verts[MAX_VERTICES_PER_SEGMENT];	// vertex ids of 4 front and 4 back vertices
#ifdef	EDITOR
	short			group;								// group number to which the segment belongs.
#endif
	short			objects;							// pointer to objects in this segment
	uint8_t			special;							// special property of a segment (such as damaging, trigger, etc.)
	int8_t			matcen_num;							//	which center segment is associated with.
	short			value;
	fix				static_light;						//average static light in segment
#ifndef	EDITOR
	short			pad;								//make structure longword aligned
#endif
};

//	Local segment data.
//	This is stuff specific to a segment that does not need to get written to disk.
//	This is a handy separation because we can add to this structure without obsoleting
//	existing data on disk.
#define	SS_REPAIR_CENTER	0x01				//	Bitmask for this segment being part of repair center.

#ifdef RESTORE_REPAIRCENTER
struct lsegment
{
	int	special_type;
	short	special_segment; // if special_type indicates repair center, this is the base of the repair center
};
#endif

struct group
{
	int		num_segments;
	int		num_vertices;
	short	segments[MAX_SEGMENTS];
	short	vertices[MAX_VERTICES];
};

// Globals from mglobal.c
extern	vms_vector	Vertices[];
extern	segment		Segments[];
#ifdef RESTORE_REPAIRCENTER
extern	lsegment	Lsegments[];
#endif
extern	int			Num_segments;
extern	int			Num_vertices;

extern	int8_t	Side_to_verts[MAX_SIDES_PER_SEGMENT][4];		// Side_to_verts[my_side] is list of vertices forming side my_side.
extern	int		Side_to_verts_int[MAX_SIDES_PER_SEGMENT][4];	// Side_to_verts[my_side] is list of vertices forming side my_side.
extern	char	Side_opposite[];								// Side_opposite[my_side] returns side opposite cube from my_side.

#define SEG_PTR_2_NUM(segptr) (Assert((unsigned) (segptr-Segments)<MAX_SEGMENTS),(segptr)-Segments)

// ----------------------------------------------------------------------------------------------------------
// --------------------------  Segment interrogation functions ------------------------
//       Do NOT read the segment data structure directly.  Use these functions instead.
//			The segment data structure is GUARANTEED to change MANY TIMES.  If you read the
//			segment data structure directly, your code will break, I PROMISE IT!
//	Return a pointer to the list of vertex indices for the current segment in vp and
//	the number of vertices in *nv.
#ifdef EDITOR
extern void med_get_vertex_list(segment* s, int* nv, unsigned short** vp);

// Delete segment function added for curves.c
extern int med_delete_segment(segment* sp);

//	Delete segment from group
extern void delete_segment_from_group(int segment_num, int group_num);

//	Add segment to group
extern void add_segment_to_group(int segment_num, int group_num);

// Verify that all vertices are legal.
extern void med_check_all_vertices();
#endif

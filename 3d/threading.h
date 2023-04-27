#pragma once

#include "3d.h"
#include "commandbuffer.h"

extern bool Use_multithread;

//This is a job for drawing. One thread will take this job and draw pixels only for this specified region of the screen. 
struct G3DrawJob
{
	//Screen region the job is supposed to cover
	int left, top, right, bottom;
	//Clipping planes for the screen region this job will cover, for polygons.
	fix clip_left, clip_top, clip_right, clip_bottom;
};

struct G3ThreadData
{
	G3Drawer drawer;
	G3DrawJob current_planes;
};

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
#include <mutex>
#include <condition_variable>
#include <vector>
#include <algorithm>
#include <atomic>

#include "misc/error.h"

#include "3d/3d.h"
#include "clipper.h"
#include "threading.h"
#include "platform/platform.h"
#include "platform/mono.h"

bool Use_multithread = false;
int Thread_count = 0;

std::condition_variable Render_start_signal;
std::condition_variable Render_complete_signal;
std::condition_variable Render_thread_obtain_signal;
std::mutex Render_start_mutex;
std::mutex Render_thread_obtain_mutex;
std::mutex Render_complete_mutex;
//Protected by the start CV, A thread will read this and start working with Render_thread_data[Render_thread_start_num]
std::atomic_int Render_thread_start_num;
//Protected by the completion CV, waiting is done when this equals Thread_count
int Num_render_thread_completed;
//Number of threads that were dispatched when calling start_frame. end_frame will wait until Num_threads_dispatched == Num_render_thread_completed
int Num_threads_dispatched;

int Num_cells_x, Num_cells_y;

std::vector<std::thread> Render_threads;
std::vector<G3ThreadData> Render_thread_data;

void g3_worker_thread(int thread_num);

//initialize the 3d system
void g3_init(void)
{
	int num_cpus = plat_get_num_cpus();
	mprintf((0, "ICDP: %d CPUs available\n", num_cpus));

	//Pick an even amount of CPUs only, to allow for a grid pattern. 
	Thread_count = std::max(1, (num_cpus - 2) & ~1);
	if (Thread_count > 1)
	{
		mprintf((0, "ICDP: Enabling multithreaded renderer, %d threads provided\n", Thread_count));
		Use_multithread = true;
		Render_thread_data.reserve(Thread_count); //this is needed until RLECache and Texmerge are movable. 

		//TODO: Determine which geometry is the best here. ATM I'm preferring wide cells to try to limit the amount of places where scanlines are divided, for cache reasons.
		Num_cells_x = Thread_count > 2 ? 2 : 1;
		Num_cells_y = Thread_count / Num_cells_x;

		mprintf((0, "ICDP: Tiles are %d x %d\n", Num_cells_x, Num_cells_y));

		for (int i = 0; i < Thread_count; i++)
		{
			/*int cell_x = i % Num_cells_x;
			int cell_y = i / Num_cells_x;

			fix x_frac_left = fixdiv(i2f(cell_x), i2f(Num_cells_x)) * 2 - F1_0;
			fix x_frac_right = fixdiv(i2f(cell_x + 1), i2f(Num_cells_x)) * 2 - F1_0;

			fix y_frac_top = fixdiv(i2f(cell_y), i2f(Num_cells_y)) * 2 - F1_0;
			fix y_frac_bottom = fixdiv(i2f(cell_y + 1), i2f(Num_cells_y)) * 2 - F1_0;
			mprintf((0, "ICDP: Thread %d, planes left: %f top: %f right: %f bottom: %f\n", i, f2fl(x_frac_left), f2fl(y_frac_top), f2fl(x_frac_right), f2fl(y_frac_bottom)));*/
			Render_thread_data.emplace_back();
			//Render_thread_data[i].set_clipping_planes(x_frac_left, y_frac_top, x_frac_right, y_frac_bottom);
			Render_threads.push_back(std::thread(g3_worker_thread, i));
		}
	}
	atexit(g3_close);
}

//close down the 3d system
void g3_close(void) 
{
	if (Thread_count > 1)
	{
		//Detach all threads so that they can be terminated. 
		auto it = Render_threads.begin();
		while (it < Render_threads.end())
		{
			std::thread& thread = *it;
			thread.detach();
			it++;
		}

		Render_threads.clear();
	}
}

extern void init_interface_vars_to_assembler(void);

void G3Drawer::start_frame(fix canv_w2, fix canv_h2)
{
	Canv_w2 = canv_w2;
	Canv_h2 = canv_h2;

	init_free_points();
	set_canvas(grd_curcanv);
}

void G3Drawer::end_frame()
{
	free_point_num = 0;
}

void G3Instance::dispatch_render_threads()
{
	//Threads probably won't be efficient if they're drawing tiny regions of the screen.
	//Therefore I use 256x256 tiles as a baseline for determining how many threads to use.
	//On high resolution canvases, the amount of tiles will be bounded to Num_cells_x and Num_cells_y,
	//resulting in much larger tiles.

	//TODO: The tiles need some adjustment. At 640x480 the overhead is slightly higher than rendering without threads. 

	Render_thread_start_num = 0;
	int num_threads_x = std::min(Canvas_width / 256 + 1, Num_cells_x);
	int num_threads_y = std::min(Canvas_height / 256 + 1, Num_cells_y);

	Num_threads_dispatched = num_threads_x * num_threads_y;

	for (int i = 0; i < Num_threads_dispatched; i++)
	{
		int cell_x = i % num_threads_x;
		int cell_y = i / num_threads_x;

		//Compute clipping planes and screen regions for this thread
		fix x_frac_left = fixdiv(i2f(cell_x), i2f(num_threads_x));
		fix x_frac_right = fixdiv(i2f(cell_x + 1), i2f(num_threads_x));

		fix y_frac_top = fixdiv(i2f(cell_y), i2f(num_threads_y));
		fix y_frac_bottom = fixdiv(i2f(cell_y + 1), i2f(num_threads_y));

		G3ThreadData& data = Render_thread_data[i];
		data.current_planes.left = f2i(x_frac_left * Canvas_width);
		data.current_planes.right = f2i(x_frac_right * Canvas_width) - 1;
		data.current_planes.top = f2i(y_frac_top * Canvas_height);
		data.current_planes.bottom = f2i(y_frac_bottom * Canvas_height) - 1;

		data.current_planes.clip_left = x_frac_left * 2 - F1_0;
		data.current_planes.clip_right = x_frac_right * 2 - F1_0;
		data.current_planes.clip_top = (F1_0 - y_frac_top) * 2 - F1_0;
		data.current_planes.clip_bottom = (F1_0 - y_frac_bottom) * 2 - F1_0;

		data.current_planes.canv_w2 = Canv_w2; data.current_planes.canv_h2 = Canv_h2;

		/*{
			std::unique_lock<std::mutex> lock(Render_start_mutex);
			lock.unlock();

			Render_start_signal.notify_one();
		}*/
	}

	{
		std::unique_lock<std::mutex> lock(Render_start_mutex);
		Render_start_signal.notify_all();
	}
}

//start the frame
void G3Instance::start_frame()
{
	fix s;

	//set int w,h & fixed-point w,h/2
	Canv_w2 = (Canvas_width = grd_curcanv->cv_bitmap.bm_w) << 15;
	Canv_h2 = (Canvas_height = grd_curcanv->cv_bitmap.bm_h) << 15;

	//compute aspect ratio for this canvas

	s = fixmuldiv(grd_curscreen->sc_aspect, Canvas_height, Canvas_width);

	memset(grd_curcanv->cv_bitmap.bm_data, 0xC0, grd_curcanv->cv_bitmap.bm_rowsize * grd_curcanv->cv_bitmap.bm_h);

	if (s <= F1_0) //scale x
	{
		Window_scale.x = s;
		Window_scale.y = f1_0;
	}
	else
	{
		Window_scale.y = fixdiv(f1_0, s);
		Window_scale.x = f1_0;
	}

	Window_scale.z = f1_0;		//always 1

	init_interface_vars_to_assembler();		//for the texture-mapper
	drawer.get_texmap_instance().SetCanvas(grd_curcanv);
	drawer.start_frame(Canv_w2, Canv_h2);

	g3_command_buffer.start_recording();
	//Use_multithread = false; //debugging
	if (Use_multithread)
	{
		dispatch_render_threads();
	}

	//This is a bit of a legacy of the global state, but ensure all drawers have the right interpolation and lighting
	g3_command_buffer.cmd_set_lighting(get_lighting_mode());
	g3_command_buffer.cmd_set_interpolation(get_interpolation_mode());
}

void G3Instance::start_frame_from_inst(G3Instance& inst)
{
	fix s;

	//set int w,h & fixed-point w,h/2
	Canv_w2 = inst.Canv_w2;
	Canv_h2 = inst.Canv_h2;

	Window_scale = inst.Window_scale;

	init_interface_vars_to_assembler();		//for the texture-mapper
	drawer.get_texmap_instance().SetCanvas(grd_curcanv);
	drawer.start_frame(Canv_w2, Canv_h2);

	View_zoom = inst.View_zoom;
	View_position = inst.View_position;
	View_matrix = inst.View_matrix;
}

//end the frame
void G3Instance::end_frame()
{
	drawer.end_frame();
	g3_command_buffer.end_recording();
	free_point_num = 0;

	if (Num_threads_dispatched > 0)
	{
		{
			std::unique_lock<std::mutex> lock(Render_complete_mutex);
			Render_complete_signal.wait(lock, [] {return Num_render_thread_completed == Thread_count; });
		}

		Num_render_thread_completed = 0;
	}

	//debugging the command decoder..
	//debug_decode_command_buffer();
}

//start the frame
void g3_start_frame(void)
{
	g3_global_inst.start_frame();
}

//this doesn't do anything, but is here for completeness
void g3_end_frame(void)
{
	g3_global_inst.end_frame();
}

void g3_worker_thread(int thread_num)
{
	while (true)
	{
		{
			std::unique_lock<std::mutex> lock(Render_start_mutex);
			Render_start_signal.wait(lock);
		}

		//emit pixels
		if (thread_num < Num_threads_dispatched) //Notifying all threads to check if they have work to do isn't the most elegant, but it'll work for now. 
		//if (thread_num == 0)
		{
			G3ThreadData& my_job = Render_thread_data[thread_num];
			my_job.drawer.start_frame(my_job.current_planes.canv_w2, my_job.current_planes.canv_h2);
			my_job.drawer.set_clip_ratios(my_job.current_planes.clip_left, my_job.current_planes.clip_top, my_job.current_planes.clip_right, my_job.current_planes.clip_bottom);
			my_job.drawer.set_clip_bounds(my_job.current_planes.left, my_job.current_planes.top, my_job.current_planes.right, my_job.current_planes.bottom);
			my_job.drawer.decode_command_buffer();
			my_job.drawer.end_frame();
		}

		{
			std::unique_lock<std::mutex> lock(Render_complete_mutex);
			Num_render_thread_completed++;
			lock.unlock();

			Render_complete_signal.notify_one();
		}
	}
}

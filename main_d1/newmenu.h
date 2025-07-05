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
#include "2d/gr.h"
#include <vector>
#include <string>
#include <memory>

struct bkg
{
	grs_canvas* menu_canvas;
	grs_bitmap* saved;			// The background under the menu.
	grs_bitmap* background;
};

#define NM_TYPE_MENU  		0		// A menu item... when enter is hit on this, newmenu_do returns this item number
#define NM_TYPE_INPUT 		1		// An input box... fills the text field in, and you need to fill in text_len field.
#define NM_TYPE_CHECK 		2		// A check box. Set and get its status by looking at flags field (1=on, 0=off)
#define NM_TYPE_RADIO 		3		// Same as check box, but only 1 in a group can be set at a time. Set group fields.
#define NM_TYPE_TEXT	 	4		// A line of text that does nothing.
#define NM_TYPE_NUMBER		5		// A numeric entry counter.  Changes value from min_value to max_value;
#define NM_TYPE_INPUT_MENU	6		// A inputbox that you hit Enter to edit, when done, hit enter and menu leaves.
#define NM_TYPE_SLIDER		7		// A slider from min_value to max_value. Draws with text_len chars.

#define NM_MAX_TEXT_LEN	50

struct newmenu_item
{
	int 	type;				// What kind of item this is, see NM_TYPE_????? defines
	int 	value;			// For checkboxes and radio buttons, this is 1 if marked initially, else 0
	int 	min_value, max_value;	// For sliders and number bars.
	int 	group;			// What group this belongs to for radio buttons.
	int	text_len;		// The maximum length of characters that can be entered by this inputboxes
	char* text;			// The text associated with this item.
	// The rest of these are used internally by by the menu system, so don't set 'em!!
	short	x, y;
	short w, h;
	short right_offset;
	uint8_t redraw;
	char	saved_text[NM_MAX_TEXT_LEN + 1];
};

//Honestly, I should have a private header for newmenu. 
extern grs_canvas* nm_canvas;
extern grs_bitmap nm_background;

class nm_window
{
	bool drawn = false;
	bool closing = false;
public:
	virtual ~nm_window()
	{
	}

	//Closes the window, and cleans it up from the newmenu canvas. 
	void close()
	{
		closing = true;
	}

	//Returns true if the window should be cleaned up and closed, false otherwise.
	bool is_closing() const
	{
		return closing;
	}

	bool must_draw() const
	{
		return !drawn;
	}

	//Draws the window, and marks it as drawn
	virtual void draw()
	{
		gr_set_current_canvas(nm_canvas);
		drawn = true;
	}

	//Runs a single frame for this window.
	virtual void frame() = 0;
	//Cleans up this window from the newmenu canvas. 
	virtual void cleanup() = 0;
};

void newmenu_init();

//Does a frame of the menu system
void newmenu_frame();

//Draws or redraws the currently open window, if needed.
//This is done as a separate stage because the game code may have created a window after running the newmenu frame. 
void newmenu_draw();

//Presents the currently drawn menu to the screen
void newmenu_present();

//Checks if there are no windows open
bool newmenu_empty();

//Closes all open windows. 
void newmenu_close_all();

// Pass an array of newmenu_items and it processes the menu. It will
// return a -1 if Esc is pressed, otherwise, it returns the index of 
// the item that was current when Enter was was selected.
// The subfunction function gets called constantly, so you can dynamically
// change the text of an item.  Just pass NULL if you don't want this.
// Title draws big, Subtitle draw medium sized.  You can pass NULL for
// either/both of these if you don't want them.
extern int newmenu_do(const char* title, const char* subtitle, int nitems, newmenu_item* item, void (*subfunction)(int nitems, newmenu_item* items, int* last_key, int citem));

// Same as above, only you can pass through what item is initially selected.
extern int newmenu_do1(const char* title, const char* subtitle, int nitems, newmenu_item* item, void (*subfunction)(int nitems, newmenu_item* items, int* last_key, int citem), int citem);

// Same as above, only you can pass through what background bitmap to use.
extern int newmenu_do2(const char* title, const char* subtitle, int nitems, newmenu_item* item, void (*subfunction)(int nitems, newmenu_item* items, int* last_key, int citem), int citem, const char* filename);

// Same as above, only you can pass through the width & height
extern int newmenu_do3(const char* title, const char* subtitle, int nitems, newmenu_item* item, void (*subfunction)(int nitems, newmenu_item* items, int* last_key, int citem), int citem, const char* filename, int width, int height, bool tiny_mode = false);

inline int newmenu_dotiny(const char* title, const char* subtitle, int nitems, newmenu_item* item, void (*subfunction)(int nitems, newmenu_item* items, int* last_key, int citem), int citem, int width)
{
	return newmenu_do3(title, subtitle, nitems, item, subfunction, citem, nullptr, width, -1, true);
}

extern void newmenu_open(const char* title, const char* subtitle, std::vector<newmenu_item>& items, 
	void (*subfunction)(int nitems, newmenu_item* items, int* last_key, int citem), bool (*choicefunc)(int choice, int nitems, newmenu_item* items), 
	int citem = 0, const char* filename = nullptr, int width = -1, int height = -1, bool tiny_mode = false);

extern void newmenu_open(const char* title, const char* subtitle, int nitems, newmenu_item* items,
	void (*subfunction)(int nitems, newmenu_item* items, int* last_key, int citem), bool (*choicefunc)(int choice, int nitems, newmenu_item* items),
	int citem = 0, const char* filename = nullptr, int width = -1, int height = -1, bool tiny_mode = false);

//Simple callback for purely informative message boxes.
//This will be the default if you don't specify a callback. 
bool newmenu_messagebox_informative_callback(int choice, int nitems, newmenu_item* item);

//Opens a messagebox
void nm_open_messagebox(const char* title, bool (*callback)(int choice, int nitems, newmenu_item* item), int nchoices, ...);

//Opens an arbitary window
void newmenu_open_window(std::unique_ptr<nm_window>&& window);

// Sample Code:
/*
			{
			int mmn;
			newmenu_item mm[8];
			char xtext[21];

			strcpy( xtext, "John" );

			mm[0].type=NM_TYPE_MENU; mm[0].text="Play game";
			mm[1].type=NM_TYPE_INPUT; mm[1].text=xtext; mm[1].text_len=20;
			mm[2].type=NM_TYPE_CHECK; mm[2].value=0; mm[2].text="check box";
			mm[3].type=NM_TYPE_TEXT; mm[3].text="-pickone-";
			mm[4].type=NM_TYPE_RADIO; mm[4].value=1; mm[4].group=0; mm[4].text="Radio #1";
			mm[5].type=NM_TYPE_RADIO; mm[5].value=1; mm[5].group=0; mm[5].text="Radio #2";
			mm[6].type=NM_TYPE_RADIO; mm[6].value=1; mm[6].group=0; mm[6].text="Radio #3";
			mm[7].type=NM_TYPE_PERCENT; mm[7].value=50; mm[7].text="Volume";

			mmn = newmenu_do("Descent", "Sample Menu", 8, mm, NULL );
			mprintf( 0, "Menu returned: %d\n", mmn );
			}

*/

// This function pops up a messagebox and returns which choice was selected...
// Example:
// nm_messagebox( "Title", "Subtitle", 2, "Ok", "Cancel", "There are %d objects", nobjects );
// Returns 0 through nchoices-1.
int nm_messagebox(const char* title, int nchoices, ...);
// Same as above, but you can pass a function
int nm_messagebox1(const char* title, void (*subfunction)(int nitems, newmenu_item* items, int* last_key, int citem), int nchoices, ...);

void nm_draw_background1(const char* filename);
void nm_draw_background(int x1, int y1, int x2, int y2);
void nm_restore_background(int x, int y, int w, int h);

// Returns 0 if no file selected, else filename is filled with selected file.
int newmenu_get_filename(const char* title, const char* filespec, char* filename, int allow_abort_flag);

void newmenu_open_filepicker(const char* title, const char* filespec, int allow_abort_flag, void (*callback)(std::string& str, int num));

//	in menu.c
extern int Max_linear_depth_objects;

extern char* Newmenu_allowed_chars;

// Example listbox callback function...
// int lb_callback( int * citem, int *nitems, char * items[], int *keypress )
// {
// 	int i;
// 
// 	if ( *keypress = KEY_CTRLED+KEY_D )	{
// 		if ( *nitems > 1 )	{
// 			unlink( items[*citem] );		// Delete the file
// 			for (i=*citem; i<*nitems-1; i++ )	{
// 				items[i] = items[i+1];
// 			}
// 			*nitems = *nitems - 1;
// 			free( items[*nitems] );
// 			items[*nitems] = NULL;
// 			return 1;	// redraw;
// 		}
//			*keypress = 0;
// 	}			
// 	return 0;
// }

extern int newmenu_listbox(const char* title, int nitems, char* items[], int allow_abort_flag, int (*listbox_callback)(int* citem, int* nitems, char* items[], int* keypress));
extern int newmenu_listbox1(const char* title, int nitems, char* items[], int allow_abort_flag, int default_item, int (*listbox_callback)(int* citem, int* nitems, char* items[], int* keypress));

void newmenu_open_listbox(const char* title, int nitems, char* items[], bool allow_abort_flag, void (*callback)(int choice), int default_item = 0);
void newmenu_open_listbox(const char* title, std::vector<char*>& items, bool allow_abort_flag, void (*callback)(int choice), int default_item = 0);

//Gets the currently visible canvas for the menu system, or nullptr if no menu is active. 
extern grs_canvas* nm_get_top_canvas();

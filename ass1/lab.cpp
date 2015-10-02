#include <iostream>
#include <cfloat> // for FLT_MAX
#include <chrono>
#include <thread>
#include <cstdlib>
#include <vector>
#include <list>
#include "graphics.h"
#include "lab.h"

using namespace std;

/* Constants
 */
Circuit cct; // constant cct file parsed only once
Track *all_tracks = NULL; //array to house all tracks
list<Track*> expansion_list;

/* end Constants
 */

// Callbacks for event-driven window handling.
void drawscreen (void);
void act_on_new_button_func (void (*drawscreen_ptr) (void));
void act_on_button_press (float x, float y, t_event_buttonPressed event);
void act_on_mouse_move (float x, float y);
void act_on_key_press (char c);

// A handy delay function for the animation example
void delay (long milliseconds);

// State variables for the example showing entering lines and rubber banding
// and the new button example.
static bool line_entering_demo = false;
static bool have_rubber_line = false;
static t_point rubber_pt;         // Last point to which we rubber-banded.
static std::vector<t_point> line_pts;  // Stores the points entered by user clicks.
static int num_new_button_clicks = 0;


// You can use any coordinate system you want.
// The coordinate system below matches what you have probably seen in math 
// (ie. with origin in bottom left).
// Note, that text orientation is not affected. Zero degrees will always be the normal, readable, orientation.
const t_bound_box initial_coords = t_bound_box(0,0,1000,1000); 


// Use this coordinate system for a conventional 
// graphics coordinate system (ie. with inverted y; origin in top left)
// const t_bound_box initial_coords = t_bound_box(0,1000,1000,0); 

Circuit parse_circuit_file(string filename){

  Circuit cct;

  std::ifstream cctfile(filename);

  int grid_size, tracks_per_channel, x1, y1, p1, x2, y2, p2; 

  if(!(cctfile >> grid_size)) {
    //error, could not read first line, abort!
  } else {
    cct.grid_size = grid_size;
  }
  cctfile >> tracks_per_channel;
  cct.tracks_per_channel = tracks_per_channel;

  SourceSink ** ptr = &cct.source_sink_head;
  while((*ptr) != NULL){
    cctfile >> x1 >> y1 >> p1 >> x2 >> y2 >> p2;

    if(x1 == -1){
      //end of file
      *ptr = NULL;
    } else {

      (*ptr)->X1 = x1;
      (*ptr)->Y1 = y1;
      (*ptr)->P1 = p1;

      (*ptr)->X2 = x2;
      (*ptr)->Y2 = y2;
      (*ptr)->P2 = p2;

      (*ptr)->next = new SourceSink();
      ptr = &(*ptr)->next;

    }
   
  } //end while

  cctfile.close();

  return cct;
}

void print_circuit(Circuit cct){

  cout << "\n--- Reading cct file ---\n";

  cout << cct.grid_size << endl;
  cout << cct.tracks_per_channel << endl;

  for(SourceSink * ptr = cct.source_sink_head; ptr != NULL; ptr = ptr->next){
    cout << ptr->X1 << ptr->Y1 << ptr->P1 << ptr->X2 << ptr->Y2 << ptr->P2 << endl;
  
  }
}

int initialize_tracks(float rectangle_height, float rectangle_width, float wire_space){

  t_point line_start;
  t_point line_end;

  int w = cct.tracks_per_channel;

  int number_of_tracks = 2*cct.grid_size*(cct.grid_size + 1)*cct.tracks_per_channel;
  all_tracks = new Track[number_of_tracks];

  int track_index = 0;

  for(int z = 0; z < 2; z++){ //0 - horizontal // 1 - vertical
    //cout << endl;
    if(z==0){
      line_start = t_point(120, 120);
      line_end = t_point(120+60, 120);
    }else if (z==1){
      line_start = t_point(120, 120);
      line_end = t_point(120, 120+60);
    }
    for(int i = 0; i < cct.grid_size + 1; i++){
      for(int j = 0; j < cct.grid_size; j++){
        for(int w = 0; w < cct.tracks_per_channel; w++){
          if(z == 0){
            all_tracks[track_index].x = j;
            all_tracks[track_index].y = i;
            all_tracks[track_index].z = z;
            all_tracks[track_index].wire = w;

            line_start += t_point(0, -wire_space);
            line_end += t_point(0, -wire_space);
            drawline (line_start, line_end);
 
            all_tracks[track_index].s_pt = line_start;
            all_tracks[track_index].e_pt = line_end;
       
            //cout << j << i << z << endl;
            
          } else if (z == 1 ){
            all_tracks[track_index].x = i;
            all_tracks[track_index].y = j;
            all_tracks[track_index].z = z;
            all_tracks[track_index].wire = w;

            line_start += t_point(-wire_space, 0);
            line_end += t_point(-wire_space, 0);
            drawline (line_start, line_end);
            
            all_tracks[track_index].s_pt = line_start;
            all_tracks[track_index].e_pt = line_end;

            //cout << i << j << z << endl;
          } else {
            //nothing
          }
          track_index += 1;
        }
        if(z==0){
          line_start += t_point(rectangle_width*2, w*wire_space);
          line_end += t_point(rectangle_width*2, w*wire_space);
        }else if (z==1){
          line_start += t_point(w*wire_space, rectangle_height*2);
          line_end += t_point(w*wire_space, rectangle_height*2);            
        }
      }
      if(z==0){
        line_start = t_point(120, rectangle_height*2*(i+2)+wire_space/2);
        line_end = t_point(120+60, rectangle_height*2*(i+2)+wire_space/2);
      }else if(z==1){
        line_start = t_point(rectangle_width*2*(i+2)+wire_space/2, 120);
        line_end = t_point(rectangle_width*2*(i+2)+wire_space/2, 120+60);        
      }
    }
  }//end of loops
  //cout << track_index << endl;
  return track_index; //size of array

}

Track * get_connected_track(int lb_x, int lb_y, int lb_p, int wire, int number_of_tracks){
  
  int x;
  int y;
  int z;

   switch(lb_p){
     case 1: //south
       x = lb_x;
       y = lb_y;
       z = 0;

       break;
     case 2: //east
       x = lb_x+1;
       y = lb_y;
       z = 1;

       break;
     case 3: //north
       x = lb_x;
       y = lb_y+1;
       z = 0;

       break;
     case 4: //west
       x = lb_x;
       y = lb_y;
       z = 1;

       break;
     default:
       printf("Error: not valid pin number\n");
  }
  
   Track * track = NULL;
   for(int i = 0 ; i < number_of_tracks; i++){
     if(all_tracks[i].x == x &&
         all_tracks[i].y == y &&
         all_tracks[i].z == z &&
         all_tracks[i].wire == wire){
       //found our track
       track = &all_tracks[i];
     }
   }

   if(track == NULL){cout << "ERROR! No tracks connected to this block on this pin\n" << endl;}

   return track;  
}


int main() {

	std::cout << "Parsing cct File\n";
  cct = parse_circuit_file("cct0");

  //print_circuit(cct);

	/**************** initialize display **********************/
	
	// Set the name of the window (in UTF-8), with white background.
	init_graphics("ECE1387", WHITE);
	// you could pass a t_color RGB triplet instead

	// Set-up coordinates. The coordinates passed in define what coordinate
        // limits you want to use; this coordinate system is mapped to fill 
        // the screen.
	set_visible_world(initial_coords);

	// Set the message at the bottom (also UTF-8)
	update_message("Assignment 1 - 2015");

	// Pass control to the window handling routine.  It will watch for user input
	// and redraw the screen / pan / zoom / etc. the graphics in response to user
	// input or windows being moved around the screen.  We have to pass in 
        // at least one callback -- the one to redraw the graphics.
        // Three other callbacks can be provided to handle mouse button presses,
        // mouse movement and keyboard button presses in the graphics area, 
        // respectively. Those 3 callbacks are optional, so we can pass NULL if
        // we don't need to take any action on those events, and we do that
        // below.
        // This function will return if and when
	// the user presses the proceed button.

	event_loop(NULL, NULL, NULL, drawscreen);   
	t_bound_box old_coords = get_visible_world(); // save the current view for later;

	/******************* animation section *******************************/
        // User might have panned and zoomed, which changes what world 
        // coordinates are visible on the screen. Get back to a known view.
	set_visible_world(initial_coords);
	clearscreen();
	update_message("Non-interactive (animation) graphics example.");
	setcolor (RED);
	setlinewidth(1);
	setlinestyle (DASHED);

	t_point start_point = t_point(100,0);
	for (int i = 0; i < 50; i++) {
		drawline (start_point, start_point + t_point(500, 10));
		flushinput();
		delay(50);
		start_point += t_point(5,10);
	}

	/**** Draw an interactive still picture again.  I'm also creating one new button. ****/

	update_message("Interactive graphics #2 Click in graphics area to rubber band line.");
	create_button ("Window", "0 Clicks", act_on_new_button_func); // name is UTF-8

	// Enable mouse movement (not just button presses) and key board input.
	// The appropriate callbacks will be called by event_loop.
	set_keypress_input (true);
	set_mouse_move_input (true);
	line_entering_demo = true;

	// draw the screen once before calling event loop, so the picture is correct 
	// before we get user input.
	set_visible_world(old_coords); // restore saved coords -- this takes us back to where the user panned/zoomed.
	drawscreen();
        
        // Call event_loop again so we get interactive graphics again.
        // This time pass in all the optional callbacks so we can take
        // action on mouse buttons presses, mouse movement, and keyboard
        // key presses.
	event_loop(act_on_button_press, act_on_mouse_move, act_on_key_press, drawscreen);

	close_graphics ();
	std::cout << "Graphics closed down.\n";

	return (0);
}


void drawscreen (void) {

	/* The redrawing routine for still pictures.  The graphics package calls  
	 * this routine to do redrawing after the user changes the window 
	 * in any way.                                                    
	*/
 
	set_draw_mode (DRAW_NORMAL);  // Should set this if your program does any XOR drawing in callbacks.
	clearscreen();  /* Should precede drawing for all drawscreens */

	setfontsize (10);
	setlinestyle (SOLID);
	setlinewidth (1);
	setcolor (BLACK);

  int n = cct.grid_size;
  int w = cct.tracks_per_channel;

  const float rectangle_width = 60;
  const float rectangle_height = rectangle_width;
  const float wire_space = rectangle_width/w;
  const t_point start_point = t_point(120,120);



	{
		/**************
		 * Draw logic blocks 
		 **************/

		t_bound_box logic_block = t_bound_box(start_point, rectangle_width, rectangle_height);

		for(int i = 0; i < n; i++) {
      for(int j = 0; j < n; j++){
        setcolor(LIGHTGREY);
        fillrect(logic_block);
        logic_block += t_point(rectangle_width*2,0);
      }
      logic_block += t_point(-rectangle_width*n*2,rectangle_height*2);
    }

	}

	/********
	 * Draw wires 
	 ********/
	{
    setcolor(BLACK);
		setlinestyle (SOLID);
		setlinewidth (1);

    int number_of_tracks = 0;
    number_of_tracks = initialize_tracks(rectangle_width, rectangle_height, wire_space);

    //TODO: implement return track channel given logic block coord and pin number
    for(SourceSink * ptr = cct.source_sink_head; ptr != NULL; ptr = ptr->next){
      for(int i = 0; i < w; i++){ //find track of wire 0 

        // mark sources
        Track * t_source = get_connected_track(ptr->X1, ptr->Y1, ptr->P1, i, number_of_tracks);
        t_source->is_labeled = true;
        expansion_list.push_back(t_source);
        
        // mark targets
        Track * t_sink = get_connected_track(ptr->X2, ptr->Y2, ptr->P2, i, number_of_tracks);
        t_sink->is_target = true;

        //printing out stuff
        cout << endl;
        cout << ptr->X1 << ptr->Y1 << ptr->P1 << endl;
        cout << t_source->x << t_source->y << t_source->z << " wire:" << t_source->wire << endl;
        cout << ptr->X2 << ptr->Y2 << ptr->P2 << endl;
        cout << t_sink->x << t_sink->y << t_sink->z << " wire:" << t_sink->wire << endl;
        
        
      }
    }//end for loops

    cout << "expansion list total: " << expansion_list.size() << endl;

    while(!expansion_list.empty()){
      expansion_list.front();

      expansion_list.pop_front();
    
    }

    if(expansion_list.empty()){ cout <<"empy"<<endl;}


    
    //TODO: implement event callbacks for click actions - step by step debug

    //color all tracks green
   // setcolor(GREEN);
   // for(int i = 0; i < number_of_tracks; i++){
   //   drawline(all_tracks[i].s_pt, all_tracks[i].e_pt);
   // }


  }

}
void delay (long milliseconds) {
	//if you would like to use this function in your
	// own code you will need to #include <chrono> and
	// <thread>
	std::chrono::milliseconds duration(milliseconds);
	std::this_thread::sleep_for(duration);
}


void act_on_new_button_func (void (*drawscreen_ptr) (void)) {

	char old_button_name[200], new_button_name[200];
	std::cout << "You pressed the new button!\n";
	setcolor (MAGENTA);
	setfontsize (12);
	drawtext (500, 500, "You pressed the new button!", 10000.0, FLT_MAX);
	sprintf (old_button_name, "%d Clicks", num_new_button_clicks);
	num_new_button_clicks++;
	sprintf (new_button_name, "%d Clicks", num_new_button_clicks);
	change_button_text (old_button_name, new_button_name);

        // Re-draw the screen (a few squares are changing colour with time)
        drawscreen_ptr ();  
}


void act_on_button_press (float x, float y, t_event_buttonPressed event) {

/* Called whenever event_loop gets a button press in the graphics *
 * area.  Allows the user to do whatever he/she wants with button *
 * clicks.                                                        */

    std::cout << "User clicked a mouse button at coordinates (" 
             << x << "," << y << ")";
    if (event.shift_pressed || event.ctrl_pressed) {
            std::cout << " with ";
            if (event.shift_pressed) {
                    std::cout << "shift ";
                    if (event.ctrl_pressed)
                            std::cout << "and ";
            }
            if (event.ctrl_pressed) 
                    std::cout << "control ";
            std::cout << "pressed.";
    }
    std::cout << std::endl;

    if (line_entering_demo) {
        line_pts.push_back(t_point(x,y));
        have_rubber_line = false;

        // Redraw screen to show the new line.  Could do incrementally, but this is easier.
        drawscreen ();  
    }
}


void act_on_mouse_move (float x, float y) {
	// function to handle mouse move event, the current mouse position in the current world coordinate
	// system (as defined in your call to init_world) is returned

	std::cout << "Mouse move at " << x << "," << y << ")\n";
	if (line_pts.size() > 0 ) {
                // Rubber banding to a previously entered point.
		// Go into XOR mode.  Make sure we set the linestyle etc. for xor mode, since it is 
		// stored in different state than normal mode.
		set_draw_mode (DRAW_XOR); 
		setlinestyle (SOLID);
		setcolor (WHITE);
		setlinewidth (1);
                int ipt = line_pts.size()-1;

		if (have_rubber_line) {
			// Erase old line.  
			drawline (line_pts[ipt], rubber_pt); 
		}
		have_rubber_line = true;
		rubber_pt.x = x;
		rubber_pt.y = y;
		drawline (line_pts[ipt], rubber_pt);   // Draw new line
	}
}


void act_on_key_press (char c) {
	// function to handle keyboard press event, the ASCII character is returned
	std::cout << "Key press: " << c << std::endl;
}


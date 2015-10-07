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
list< list<Track*>> routes_list;
int number_of_tracks = 0;
bool debug_mode = false;
float total_routes = 0;
float routes_not_found = 0;
bool is_routed = false;
SourceSink * expanding_route = NULL;
bool unidirect_mode = false; //bidirect by default

//drawing my grid
float rectangle_width = 80;
float rectangle_height = rectangle_width;
float wire_length = rectangle_width;
float wire_space = 0.0f;
t_point start_point = t_point(160,160);

/* end Constants
 */

// Callbacks for event-driven window handling.
void drawscreen (void);
void act_on_new_button_func (void (*drawscreen_ptr) (void));
void route_all_button_func (void (*drawscreen_ptr) (void));
void expand_button_func(void (*drawscreen_ptr) (void));
void traceback_button_func(void (*drawscreen_ptr) (void));
//void act_on_button_press (float x, float y, t_event_buttonPressed event);
//void act_on_mouse_move (float x, float y);
//void act_on_key_press (char c);

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


/* Parses the circuit file <filename>
 * saves into struct Circuit
 */
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
    
    total_routes ++;

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

  total_routes -=1; //account for -1 -1 -1 line

  cctfile.close();

  return cct;
}

void draw_cross(float x, float y){
  
  drawline(x-5, y-5, x+5, y+5);
  drawline(x+5, y-5, x-5, y+5);
  
}

void draw_pin(int pin_num, Track* track){
  
  float track_x = (track->s_pt.x + track->e_pt.x)/2;
  float track_y = (track->s_pt.y + track->e_pt.y)/2;
  float wire = track->wire+1;

  draw_cross(track_x, track_y);

  if(pin_num == 1){
    drawline(track_x, track_y,
             track_x, track_y + (cct.tracks_per_channel - wire +1) * wire_space);
  }else if(pin_num == 2){
    drawline(track_x, track_y, track_x - wire * wire_space, track_y);
  }else if(pin_num == 3){
    drawline(track_x, track_y, track_x, track_y - wire * wire_space);
  }else if(pin_num == 4){
    drawline(track_x, track_y,
             track_x + (cct.tracks_per_channel - wire +1) * wire_space, track_y);
  }

}
/* Prints the Circuit object
 */
void print_circuit(Circuit cct){

  cout << "\n--- Reading cct file ---\n";

  cout << cct.grid_size << endl;
  cout << cct.tracks_per_channel << endl;

  for(SourceSink * ptr = cct.source_sink_head; ptr != NULL; ptr = ptr->next){
    cout << ptr->X1 << ptr->Y1 << ptr->P1 << ptr->X2 << ptr->Y2 << ptr->P2 << endl;
  
  }
}

/* Draw all tracks
 * save all tracks into all_tracks array
 * save all wire start and end points
 */
int initialize_tracks(float rectangle_height, float rectangle_width, float wire_space){

  t_point line_start;
  t_point line_end;

  int w = cct.tracks_per_channel;

  number_of_tracks = 2*cct.grid_size*(cct.grid_size + 1)*cct.tracks_per_channel;
  all_tracks = new Track[number_of_tracks];

  int track_index = 0;

  for(int z = 0; z < 2; z++){ //0 - horizontal // 1 - vertical
    //cout << endl;
    if(z==0){
      line_start = start_point;
      line_end = start_point + t_point(wire_length, 0);
    }else if (z==1){
      line_start = start_point;
      line_end = start_point + t_point(0, wire_length);
    }
    for(int i = 0; i < cct.grid_size + 1; i++){
      for(int j = 0; j < cct.grid_size; j++){
        for(int w = cct.tracks_per_channel-1; w >= 0; w--){
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
        line_start = t_point(start_point.x, rectangle_height*2*(i+2)+wire_space/2);
        line_end = t_point(start_point.x+wire_length, rectangle_height*2*(i+2)+wire_space/2);
      }else if(z==1){
        line_start = t_point(rectangle_width*2*(i+2)+wire_space/2, start_point.y);
        line_end = t_point(rectangle_width*2*(i+2)+wire_space/2, start_point.y+wire_length);        
      }
    }
  }//end of loops
  //cout << track_index << endl;
  return track_index; //size of array

}

/* Given the Logic Block coordinate, pin, wire number, and total number of tracks
 * returns a pointer to the Track
 */
Track * get_connected_track(int lb_x, int lb_y, int lb_p, int wire){
  
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

void clear_labels(){
  for(int i = 0; i < number_of_tracks; i++){
    all_tracks[i].label = 0;
    all_tracks[i].is_labeled = false;
  }
}

Track * get_track(int x, int y, int z, int wire) {

  

  for(int i = 0; i < number_of_tracks; i++){
    if(all_tracks[i].x == x &&
        all_tracks[i].y == y &&
        all_tracks[i].z == z &&
        all_tracks[i].wire == wire){
      return &all_tracks[i];
    }
  }
  return NULL; //no track found
}

Track * get_connected_track(Track * origin_track){
  Track * connected_track = NULL;

  //get current track parameters
  int x, y, z, wire;
  x = origin_track->x;
  y = origin_track->y;
  z = origin_track->z;
  wire = origin_track->wire;

  //clockwise track exploration
  int hor_x[6] = {x+1, x+1, x+1, x,   x-1, x  };
  int hor_y[6] = {y,   y,   y-1, y-1, y,   y  };
  int hor_z[6] = {z+1, z,   z+1, z+1, z,   z+1};

  int ver_x[6] = {x-1, x,   x,   x,   x,   x-1};
  int ver_y[6] = {y+1, y+1, y+1, y,   y-1, y  };
  int ver_z[6] = {z-1, z,   z-1, z-1, z,   z-1};

  //horizontal track
  if(z == 0){
   
    if(unidirect_mode){
      if(wire%2 == 0){ //even
        for(int i = 0; i < 3; i++ ) {
          int uni_wire = wire;
          switch(i){
            case 0: uni_wire = wire+1; break;
            case 1: uni_wire = wire; break;
            case 2: uni_wire = wire; break;
            default: cout << "something went wrong" << endl;
          }
          Track * track = get_track(hor_x[i], hor_y[i], hor_z[i], uni_wire);
          if(track != NULL){
            if(track->is_labeled || track->is_unavailable ){
              //skip
            } else {
               return track; //return first found
            }
          }
        }//end for
      }else if(wire%2 != 0){ //odd
        for(int i = 3; i < 6; i++ ) {
          int uni_wire = wire;
          switch(i){
            case 3: uni_wire = wire-1; break;
            case 4: uni_wire = wire; break;
            case 5: uni_wire = wire; break;
            default: cout << "something went wrong" << endl;
          }
          Track * track = get_track(hor_x[i], hor_y[i], hor_z[i], uni_wire);
          if(track != NULL){
            if(track->is_labeled || track->is_unavailable ){
              //skip
            } else {
               return track; //return first found
            }
          }
        }
      }
    } else { //bi-directional
      for(int i = 0; i < 6; i++ ) {
        Track * track = get_track(hor_x[i], hor_y[i], hor_z[i], wire);
        if(track != NULL){
           if(track->is_labeled || track->is_unavailable ){
             //skip
           } else {
              return track; //return first found
           }
        }
      }
    } // end else

  // vertical track
  } else if(z == 1){
 
    if(unidirect_mode){
      if(wire%2 == 0){ //even
        for(int i = 3; i < 6; i++ ) {
          int uni_wire = wire;
          switch(i){
            case 3: uni_wire = wire; break;
            case 4: uni_wire = wire; break;
            case 5: uni_wire = wire+1; break;
            default: cout << "something went wrong" << endl;
          }
          Track * track = get_track(ver_x[i], ver_y[i], ver_z[i], uni_wire);
          if(track != NULL){
            if(track->is_labeled || track->is_unavailable ){
              //skip
            } else {
               return track; //return first found
            }
          }
        }//end for
      }else if(wire%2 != 0){ //odd
        for(int i = 0; i < 3; i++ ) {
          int uni_wire = wire;
          switch(i){
            case 0: uni_wire = wire; break;
            case 1: uni_wire = wire; break;
            case 2: uni_wire = wire-1; break;
            default: cout << "something went wrong" << endl;
          }
          Track * track = get_track(ver_x[i], ver_y[i], ver_z[i], uni_wire);
          if(track != NULL){
            if(track->is_labeled || track->is_unavailable ){
              //skip
            } else {
               return track; //return first found
            }
          }
        } // end for
      }
    }else{//bi directional
      for(int i = 0; i < 6; i++ ) {
        Track * track = get_track(ver_x[i], ver_y[i], ver_z[i], wire);
        if(track != NULL){
           if(track->is_labeled || track->is_unavailable ){
             //skip
           } else {
              return track;
           }
        }
      } // end for
    }
  } else {}

 // if(connected_track == NULL)
    //cout << "No available tracks were found!" << endl;
  
  return connected_track;
}

/* expands to find target
 * returns target track if found
 */
Track * expand(){

  int label = 0;
  setfontsize(10);
  setcolor (RED);

  Track * focus_track = NULL;
    while(!expansion_list.empty()){
      
      if(debug_mode) cout << "expansion list total: " << expansion_list.size() << endl;
      
      focus_track = expansion_list.front();
      label = focus_track->label +1;
      
      if(debug_mode) cout << "focus_track: " << *focus_track << endl;

      if(focus_track->is_target){
        return focus_track;
      }

      for(Track * connected_track = get_connected_track(focus_track); 
          connected_track != NULL;
          connected_track = get_connected_track(focus_track)) { 

          if(connected_track->is_target){ //is target or not?
            connected_track->label = label;
            connected_track->is_labeled = true;
            if(debug_mode) drawtext(connected_track->s_pt, to_string(label), FLT_MAX, FLT_MAX);
            return connected_track; //return target if found
          } else {
            connected_track->is_labeled = true;
            connected_track->label = label;
            if(debug_mode) drawtext(connected_track->s_pt, to_string(label), FLT_MAX, FLT_MAX);
            expansion_list.push_back(connected_track);

          }
      }
      expansion_list.pop_front();
    }//end while expand
    
    //target not found, return null
    cout << "target not found :(" << endl;
    return NULL;

}

Track * traceback_route(Track * target){
  
  int x, y, z, wire;
  x = target->x;
  y = target->y;
  z = target->z;
  wire = target->wire;

  int hor_x[6] = {x, x-1,   x,   x+1, x+1, x+1};
  int hor_y[6] = {y,   y,   y-1, y-1, y,   y  };
  int hor_z[6] = {z+1, z,   z+1, z+1, z,   z+1};

  int ver_x[6] = {x-1, x,   x,   x,   x,   x-1};
  int ver_y[6] = {y,   y-1, y, y+1, y+1,   y+1};
  int ver_z[6] = {z-1, z,   z-1, z-1, z,   z-1};

  int label = target->label -1;

  //horizontal track
  if(z == 0){
   
    for(int i = 0; i < 6; i++ ) {
      Track * track = get_track(hor_x[i], hor_y[i], hor_z[i], wire);
      if(track != NULL){
         if(track->label == label && track->is_labeled ){
           return track;
         }
      }
    } // end for

  // vertical track
  } else if(z == 1){
 
    for(int i = 0; i < 6; i++ ) {
      Track * track = get_track(ver_x[i], ver_y[i], ver_z[i], wire);
      if(track != NULL){
         if(track->label == label && track->is_labeled ){
           return track;
         }
      }
    } // end for
  } else {}

  cout <<"ERROR!! Traceback not found!" <<endl;
  return NULL; //should always find traceback
  
}

void draw_traceback_routes(void){

  cout << "drawing tracebacks" <<endl;
  //color go through traced back routes

  for(list<list <Track *>>::iterator it = routes_list.begin();
      it != routes_list.end();
      ++it){
      t_point tmp_s = t_point(0,0);
      t_point tmp_e = t_point(0,0);
      int tmp_or = 0;

    for(list<Track*>::iterator itt = (*it).begin();
        itt != (*it).end();
        ++itt){

        t_point s = (*itt)->s_pt;
        t_point e = (*itt)->e_pt;
        int ori = (*itt)->z;
        int pin = (*itt)->lb_p;
        int pin2 = (*itt)->lb_p2;
        if(pin != 0 || pin2 != 0){
          //connected to a logic block. draw the conn
          if( next(itt,1) == (*it).end() ){
            setcolor(PURPLE);
          }else{
            setcolor(GREEN);
          }
          if(pin2 != 0){
            draw_pin(pin, (*itt));
            draw_pin(pin2, (*itt));
          } else {
            draw_pin(pin, (*itt));
          }
        }

        setcolor(GREEN);
        if(tmp_s.x == 0){
          //first iteration, skip
        } else {
          
          if(ori == 0){ //horizontal
            if(s.x > tmp_s.x){ //to left
              if(tmp_e.y == tmp_s.y){ //hor
                drawline(s, tmp_e);
              } else if(tmp_s.y > s.y) { //up
                drawline(s, tmp_s);
              } else if(tmp_e.y < s.y){ //down
                drawline(s, tmp_e);
              }
            } else if (e.x < tmp_s.x){ //to right
              if(tmp_s.y == tmp_e.y){ //hor
                drawline(e, tmp_s);
              } else if (e.y < tmp_s.y) { //above
                drawline(e, tmp_s);
              } else if (e.y > tmp_e.y) { //below
                drawline(e, tmp_e);
              }
            }
          } else if (ori == 1) { //vertical
            if(e.y < tmp_s.y) { //above
              if(tmp_s.x == tmp_e.x){ //vertical
                drawline(e, tmp_s);
              } else if (tmp_s.x > e.x){ //right
                drawline(e, tmp_s);
              } else if (tmp_s.x < e.x){ //left
                drawline(e, tmp_e);
              }
            } else if (s.y > tmp_s.y){ //below
              if (tmp_s.x == tmp_e.x) {
                drawline(tmp_e, s);
              } else if (tmp_s.x > s.x){ //right
                drawline(s, tmp_s);
              } else if (tmp_e.x < s.x) { //left
                drawline(tmp_e, s);
              }
            }
          }
        
        }// end else

        drawline(s, e);
        tmp_s = s;
        tmp_e = e;
        tmp_or = ori;

    }
  }
}

int main(int argc, char* argv[]) {

  if(argv[1] == NULL){
    cout << "please provide cct file" << endl;
    return 0;
  }
  if(argv[2] != NULL){
    unidirect_mode = true;
    cout << "Running Uni-directional case" << endl;
  }
  if(argv[3] != NULL){
    debug_mode = true;
  }

	std::cout << "Parsing cct File" << argv[1] << endl;
  cct = parse_circuit_file(argv[1]);

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

	create_button ("Window", "I'm nobody", act_on_new_button_func); // name is UTF-8
	create_button ("Window", "Traceback Routes", traceback_button_func); // name is UTF-8
	create_button ("Window", "Route All", route_all_button_func); // name is UTF-8
	create_button ("Window", "Expand Route", expand_button_func); // name is UTF-8

	event_loop(NULL, NULL, NULL, drawscreen);   

	t_bound_box old_coords = get_visible_world(); // save the current view for later;


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


  int n = cct.grid_size;
  int w = cct.tracks_per_channel;

  wire_space = rectangle_width/w;

	{
		/**************
		 * Draw logic blocks 
		 **************/

    setfontsize (12);

		t_bound_box logic_block = 
      t_bound_box(start_point, rectangle_width, rectangle_height);

		for(int i = 0; i < n; i++) {
      for(int j = 0; j < n; j++){

        setcolor(LIGHTGREY);
        fillrect(logic_block);

        setcolor(BLUE);
        drawtext(logic_block.get_center(),
            to_string(j) + ", " + to_string(i),
            logic_block);

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

    number_of_tracks = 
      initialize_tracks(rectangle_width, rectangle_height, wire_space);

    //TODO: implement event callbacks for click actions - step by step debug
    
    //always color in traceback
    draw_traceback_routes();

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
	std::cout << "Next Pressed\n";
	sprintf (old_button_name, "Next %d", num_new_button_clicks);
	num_new_button_clicks++;
	sprintf (new_button_name, "Next %d", num_new_button_clicks);
	change_button_text (old_button_name, new_button_name);

        // Re-draw the screen (a few squares are changing colour with time)
        drawscreen_ptr ();  
}

void route_one(SourceSink * route_ptr){

  Track * t_source = NULL;
  Track * t_sink = NULL;

  //print out which route is being processed
  cout << "Processing route: " << 
    route_ptr->X1 << route_ptr->Y1 << route_ptr->P1 << " " <<
    route_ptr->X2 << route_ptr->Y2 << route_ptr->P2 << endl;

  for(int i = 0; i < cct.tracks_per_channel; i++){ //find track of wire 0 

    // mark sources
    t_source = get_connected_track(route_ptr->X1, route_ptr->Y1, route_ptr->P1, i);
    t_source->is_labeled = true;
    t_source->label = 0;
    t_source->lb_p = route_ptr->P1;
    if(debug_mode) drawtext(t_source->s_pt, to_string(0), FLT_MAX, FLT_MAX);

    expansion_list.push_back(t_source);
    if(debug_mode) cout << "Source ===== " << *t_source <<  endl;
      
    // mark targets
    t_sink = get_connected_track(route_ptr->X2, route_ptr->Y2, route_ptr->P2, i);
    t_sink->is_target = true;
    if(t_source->x == t_sink->x && t_source->y == t_sink->y){
      t_sink->lb_p2 = route_ptr->P2; //if source == target, target uses p2
    } else {
      t_sink->lb_p = route_ptr->P2;
    }
    if(debug_mode) drawtext(t_sink->s_pt, "T", FLT_MAX, FLT_MAX);
    if(debug_mode) cout << "Sink ======= " << *t_sink << endl;

  }// end for marking source and targets

  // expand -> returns target if found
  // goes through the entire expansion_list
  Track * found_target = expand();

  if(found_target != NULL) {// target found!
    if(debug_mode) cout << "found target! " << *found_target <<endl;
    //traceback to source
    //traceback
    list<Track*> route;
    Track * trace = found_target;

    route.push_front(trace);    
    trace->is_unavailable = true;
    while(trace->label >= 0){ //if its 0, we don't want to trace anymore
      if(trace->label != 0){
        trace = traceback_route(trace);
        trace->is_unavailable = true;
        route.push_front(trace);
      } else {
        trace->is_unavailable = true;  //get source as well
        route.push_front(trace);
        break;
      }
    }

    //add traced back route into list
    routes_list.push_back(route); 

    //lets iterate through them?
    if(debug_mode) {
      cout << "\n" ;
      for(list<Track *>::iterator it = route.begin();
          it != route.end();
          ++it) {
        /* std::cout << *it; ... */
        cout << "traceback -->> "<< **it << endl;
      }
    }

  } else {
    cout << "found target NOT" << endl;
    routes_not_found ++;
  }

  expansion_list.clear(); // we already found the target. clear it up
  //clear up targets
  for(int i = 0; i < cct.tracks_per_channel; i++){
    t_sink = get_connected_track(route_ptr->X2, route_ptr->Y2, route_ptr->P2, i);
    t_sink->is_target=false;
    if(debug_mode) cout << "clear targets --> " << *t_sink << endl;
  }
  //gotta clear all labels
  clear_labels();

}

void expand_button_func(void (*drawscreen_ptr) (void)){

  setfontsize(10);
  setcolor (RED);

  // Mark source tracks and put into expansion list
  // Mark target tracks
  if(is_routed){
    return; //don't step through if we're already routed
  }
  if(expanding_route == NULL){
     expanding_route = cct.source_sink_head;
  }

  route_one(expanding_route);

  if(expanding_route->next == NULL){ // this is the last route
    is_routed = true;

    cout << "Finished Processing "<< total_routes <<" Routes" << endl;
    if(routes_not_found > 0) {
      cout << routes_not_found << " routes were not found!" << endl;
    }
    cout << "Percentage Routed: " 
         << (total_routes - routes_not_found) / total_routes * 100 
         << "%" << endl;

  } else {
    expanding_route  = expanding_route->next;
  }


}


void route_all_button_func(void (*drawscreen_ptr) (void)) {

  setcolor(RED);
  if(is_routed){ // we are routing only once
    return;
  }
  is_routed = true;

  // Mark source tracks and put into expansion list
  // Mark target tracks
  for(SourceSink * ptr = cct.source_sink_head; ptr != NULL; ptr = ptr->next){
    
    route_one(ptr);   

  }//end for each source/sink

  cout << "Finished Processing "<< total_routes <<" Routes" << endl;
  if(routes_not_found > 0) {
    cout << routes_not_found << " routes were not found!" << endl;
  }
  cout << "Percentage Routed: " 
       << (total_routes - routes_not_found) / total_routes * 100 
       << "%" << endl;

}



void traceback_button_func(void (*drawscreen_ptr) (void) ){
  draw_traceback_routes();
}

//void act_on_button_press (float x, float y, t_event_buttonPressed event) {
//
///* Called whenever event_loop gets a button press in the graphics *
// * area.  Allows the user to do whatever he/she wants with button *
// * clicks.                                                        */
//
//    std::cout << "User clicked a mouse button at coordinates (" 
//             << x << "," << y << ")";
//    if (event.shift_pressed || event.ctrl_pressed) {
//            std::cout << " with ";
//            if (event.shift_pressed) {
//                    std::cout << "shift ";
//                    if (event.ctrl_pressed)
//                            std::cout << "and ";
//            }
//            if (event.ctrl_pressed) 
//                    std::cout << "control ";
//            std::cout << "pressed.";
//    }
//    std::cout << std::endl;
//
//    if (line_entering_demo) {
//        line_pts.push_back(t_point(x,y));
//        have_rubber_line = false;
//
//        // Redraw screen to show the new line.  Could do incrementally, but this is easier.
//        drawscreen ();  
//    }
//}

/*
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
*/
/*
void act_on_key_press (char c) {
	// function to handle keyboard press event, the ASCII character is returned
	std::cout << "Key press: " << c << std::endl;
}
*/

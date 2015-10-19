#include <iostream>
#include <cfloat> // for FLT_MAX
#include <chrono>
#include <thread>
#include <cstdlib>
#include <vector>
#include <list>
#include <boost/tokenizer.hpp>
#include "graphics.h"
#include "lab.h"
#include "helpers.h"

extern "C"{
  #include "umfpack_matrix_solver.h"
}


using namespace std;
using namespace boost;

/*
 ******************* Constants
 */
bool debug_mode = true;
list<Block> block_list;
list<Net> net_list;
list<Weight> weight_list;

void drawscreen (void);
const t_bound_box initial_coords = t_bound_box(0,0,1000,1000); 
/* 
 ******************* END Constants
 */

/* 
 ******************* Functions
 */
Block* get_block(list<Block>* block_list, int n);
void print_net_list(list<Net> net_list);
void print_block_list(list<Block> block_list);
void print_weight_list(list<Weight> weight_list);
void print_int_array(int arr[], int size);
void print_double_array(double arr[], int size);

double sum_weights_connected_to_block(list<Weight> weight_list, int block_num);

double get_weight_between(list<Weight> weight_list, int b1, int b2);

int count_non_fixed_blocks(list<Block> block_list);
double sum_weight_to_fixed(list<Weight> weight_list, int block_num, char c);

void model_clique(const Net net, list<Weight> * weight_list);

//pamper argc and argv[]
void print_instructions(void);

//parse inputed file
void parse_circuit_file(string filename);

//// Callbacks for event-driven window handling.
//void find_minimum_w_button_func(void (*drawscreen_ptr) (void));
//void count_used_wires_button_func(void (*drawscreen_ptr) (void));
//void route_all_button_func (void (*drawscreen_ptr) (void));
//void route_one_source_sink(void (*drawscreen_ptr) (void));
//void traceback_button_func(void (*drawscreen_ptr) (void));
//void find_minimum_w_button_func( void (*drawscreen_ptr) (void));
//
////drawing and printing functions
//void drawscreen (void);
//void draw_pin(int pin_num, Track* track);
//void draw_cross(float x, float y);
//void draw_traceback_routes(void);
//void print_circuit(Circuit cct);
//
////draw dracks onto screen and initialize all_tracks global variable
//int initialize_tracks(float rectangle_height, float rectangle_width, float wire_space);
//
////routing functions
//void route_all(void);
//void route_one(SourceSink * route_ptr);
//Track * expand();
//Track * traceback_route(Track * target);
//
////helper to routing functions
//Track * get_track(int x, int y, int z, int wire);
//Track * get_connected_track(Track * origin_track);
//Track * get_connected_track_from_logic_block(int lb_x, int lb_y, int lb_p, int wire);
//
////cleaning lady
//void clean_routing_memory();
//void clear_labels();

/* 
 ******************* End Functions
 */

/* 
 ******************* MAIN
 */
int main(int argc, char* const argv[]) {

  string cct_file_name = argv[1];
	
  std::cout << "Parsing cct File " << cct_file_name << endl;
  parse_circuit_file(cct_file_name);

  if(debug_mode) print_net_list(net_list);
  if(debug_mode) print_block_list(block_list);

  //for every element of netlist, generate clique weights
  list<Net>::iterator iterator;
  for (iterator = net_list.begin();
      iterator != net_list.end();
      ++iterator) {
    // pass in this net element for read
    // will be writting to the weight list
    model_clique(*iterator, &weight_list);
  }
  
  //add weights
  list<Weight>::iterator i;
  list<Weight>::iterator j;
  for (i = weight_list.begin(); i != weight_list.end(); ++i) {
    for (j = weight_list.begin(); j != weight_list.end(); ++j) {
      if(j == i) continue;
      if(same_connection(*j, *i)){
        //add weights, save into *i
        (*i).value += (*j).value;
        //remove *j
        weight_list.erase(j);
        --j;
      }
    }
  }

  if(debug_mode) print_weight_list(weight_list);

  //tot movable blocks - find 2D array size
  int num_movable_blocks = count_non_fixed_blocks(block_list);

  if(debug_mode) 
    cout << "Num non-fixed blocks " << num_movable_blocks << endl;

  double q_matrix[num_movable_blocks][num_movable_blocks];
  int q_map[num_movable_blocks];
  {
    //gotta map that shit
    list<Block>::iterator b_it;
    int i = 0;
    for(b_it = block_list.begin();
        b_it != block_list.end();
        ++b_it){
      if((*b_it).is_fixed == false){
        if(i >= num_movable_blocks) cout <<"Error"<<endl;
        q_map[i] = (*b_it).number;
        i++;
      }
    }
  }

  //fill in matrix
  for(int i = 0; i < num_movable_blocks; i++){
    for(int j = i; j < num_movable_blocks; j++){
      if(i == j){
        q_matrix[i][j] = 
          sum_weights_connected_to_block(
              weight_list,
              q_map[i]
              );
      } else {
        double w = 
          get_weight_between(
              weight_list,
              q_map[i],
              q_map[j]
              );
        q_matrix[i][j] = -1*w;
        q_matrix[j][i] = -1*w;
      }
    }
  }

  //print q_matrix
  cout << "print q_matrix" << endl;
  for(int i = 0; i < num_movable_blocks; i++){
    for(int j = 0; j < num_movable_blocks; j++){
      cout << q_matrix[i][j] << " " ;
    }
    cout << endl;
  }
  cout << "end print q_matrix" << endl;

  //fill in fixed obj connections
  double bx[num_movable_blocks];
  double by[num_movable_blocks];
  for(int i = 0; i < num_movable_blocks; i++){
    bx[i] = sum_weight_to_fixed(weight_list, q_map[i], 'x');
    by[i] = sum_weight_to_fixed(weight_list, q_map[i], 'y');
  }

  //count number of non-zero objects
  int non_zero_entries = 0;
  for(int i = 0; i < num_movable_blocks; i++){
    for(int j = 0; j < num_movable_blocks; j++){
      if(q_matrix[i][j] != 0.){
        non_zero_entries ++;
      }
    }
  } //end for

  // generate and solve matrix for positions
  int Ap [num_movable_blocks + 1];
  int Ai [non_zero_entries];
  double Ax [non_zero_entries];
  //sum number terms in matrix per column
  Ap[0] = 0; //first term always 0
  int Ap_i = 1;
  int col_item_counter = 0;
  for(int j = 0; j < num_movable_blocks; j++){//column
    for(int i = 0; i < num_movable_blocks; i++){//row
      if(q_matrix[i][j] != 0){
        Ai[col_item_counter] = i;
        Ax[col_item_counter] = q_matrix[i][j];
        col_item_counter ++;
      }
    }//finish traversing all rows in column
    Ap[Ap_i] = col_item_counter;
    Ap_i ++;
  }//end for

  double x [num_movable_blocks] ;
  double y [num_movable_blocks] ;

  print_int_array(Ap, num_movable_blocks+1);
  print_int_array(Ai, non_zero_entries);
  print_double_array(Ax, non_zero_entries);
  print_double_array(bx, num_movable_blocks);
  print_double_array(x, num_movable_blocks);

  umfpack_matrix_solver (
      num_movable_blocks,
      Ap,
      Ai,
      Ax,
      bx,
      x);
  for (int i = 0 ; i < num_movable_blocks ; i++) 
    cout << "x ["<< i <<"] = "<< x[i] << " | map_to_block " << q_map[i] << endl;

  umfpack_matrix_solver (
      num_movable_blocks,
      Ap,
      Ai,
      Ax,
      by,
      y); 
  for (int i = 0 ; i < num_movable_blocks ; i++) 
    cout << "y ["<< i <<"] = "<< y[i] << " | map_to_block " << q_map[i] << endl;


  for (int i = 0 ; i < num_movable_blocks ; i++) 
    (*get_block(&block_list, q_map[i])).write_coords(x[i],y[i]);

  print_block_list(block_list);

	/**************** initialize display **********************/
	init_graphics("ECE1387", WHITE); // window title
	update_message("Assignment 2 - 2015"); //bottom message
	set_visible_world(initial_coords);

	//create_button ("Window", "minW: 0", find_minimum_w_button_func); // name is UTF-8
	//create_button ("Window", "Wires: 0", count_used_wires_button_func); // name is UTF-8
	//create_button ("Window", "Traceback Routes", traceback_button_func); // name is UTF-8
	//create_button ("Window", "Route All", route_all_button_func); // name is UTF-8
	//create_button ("Window", "Expand Route", route_one_source_sink); // name is UTF-8

	event_loop(NULL, NULL, NULL, drawscreen);   

	close_graphics ();
	std::cout << "Graphics closed down.\n";

	return (0);
}

void drawscreen (void) {

	set_draw_mode (DRAW_NORMAL);  // Should set this if your program does any XOR drawing in callbacks.
	clearscreen();  /* Should precede drawing for all drawscreens */



  return;
}

/* Parses the circuit file <filename>
 * saves into class Circuit
 */
void parse_circuit_file(string filename){

  ifstream cctfile(filename);

  string s;

  while (getline(cctfile, s)){
    cout << s << endl; //debug

    if(s == "-1"){
      cout << "stop parse movable" << endl;
      break;
    }

    Block * block;
    typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
    boost::char_separator<char> sep{" "};
    tokenizer tok{s, sep};

    for(tokenizer::iterator tok_iter = tok.begin();
        tok_iter != tok.end();
        ++tok_iter){

      int n = atoi((*tok_iter).c_str());

      if(n == -1){ //end of line
        break;
      }
      if(tok_iter == tok.begin()){ //create block
        block = new Block(n, false, false, 0, 0);
        block_list.push_back(*block);
        delete(block);
        block = &block_list.back();
      } else { //create net
        if(net_number_exists(net_list, n)){
          get_net(&net_list, n)->add_block(block);
        } else {
          net_list.push_back(*(new Net(n)));
          get_net(&net_list, n)->add_block(block);
        }
        //block->add_net(get_net(&net_list, n));
        block_list.back().add_net(get_net(&net_list, n));
      }

    } //end for

  }// end while
  //broke when parsed movable objects
  //parsing fixed objects
  int block_num, bx, by;
  while(cctfile >> block_num >> bx >>by){
    cout << block_num << bx << by << endl;
    get_block(&block_list, block_num)->is_fixed = true;
    get_block(&block_list, block_num)->x_coord = bx;
    get_block(&block_list, block_num)->y_coord = by;
  }

  //fill out total blocks for Net objects
  for(list<Net>::iterator it=net_list.begin(); it !=net_list.end(); ++it){
    (*it).set_tot_blocks();
  }
  
  cctfile.close();
  return;
}

/* 
 * user friendlyness
 */
void print_instructions(void){
    //no arguments provided print instructions
//    cout <<endl;
//    cout << "Usage:" <<endl;
//    cout << "./lab <circuit_file> [-v] [-b | -u]" <<endl;
//    cout <<endl;
//    cout << "    <circuit_file> : file containing grid and routing definition"<< endl;
//    cout << "    -v : verbose"<< endl;
//    cout << "    -b : bi-directional routing option (default)"<< endl;
//    cout << "    -u : uni-directional routing option"<< endl;
//    cout <<endl;
}

bool net_number_exists(list<Net> net_list, int n){

  list<Net>::iterator iterator;
  for (iterator = net_list.begin(); iterator != net_list.end(); ++iterator) {
    if ((*iterator).number == n){
      //found it
      return true;
    }
  }

  return false;
}

Net* get_net(list<Net>* net_list, int n){

  list<Net>::iterator iterator;
  for (iterator = net_list->begin(); iterator != net_list->end(); ++iterator) {
    if ((*iterator).number == n){
      //found it
      return &(*iterator);
    }
  }

  return NULL;
}

Block* get_block(list<Block>* block_list, int n){

  list<Block>::iterator iterator;
  for (iterator = block_list->begin(); iterator != block_list->end(); ++iterator) {
    if ((*iterator).number == n){
      //found it
      return &(*iterator);
    }
  }

  return NULL;
}

void print_net_list(list<Net> net_list){
  list<Net>::iterator iterator;
  for (iterator = net_list.begin(); iterator != net_list.end(); ++iterator) {
    cout << *iterator << endl;
  }
}
void print_block_list(list<Block> block_list){
  list<Block>::iterator iterator;
  for (iterator = block_list.begin(); iterator != block_list.end(); ++iterator) {
    cout << *iterator << endl;
  }
}
void print_weight_list(list<Weight> weight_list){
  list<Weight>::iterator iterator;
  for (iterator = weight_list.begin(); iterator != weight_list.end(); ++iterator) {
    cout << *iterator << endl;
  }
}


void model_clique(const Net net, list<Weight> * weight_list){

  list<Block*> blocks = net.block_list;

  if(net.tot_blocks == 2){
    double w_val = 1.0;
    (*weight_list).push_back(
        *(new Weight(blocks.front(), blocks.back(), w_val))
        );

  } else if (net.tot_blocks > 2){
    cout << "model_clique " << net <<endl;

    double w_val = 2.0/(double)net.tot_blocks;

    list<Block*>::iterator i;
    list<Block*>::iterator j;
    for(i = blocks.begin(); i != blocks.end(); ++i){
      for(j = i; j != blocks.end(); ++j){
        if(j == i) continue;
        (*weight_list).push_back(
            *(new Weight(*i, *j, w_val))
            );
      }
    } // end for loops
  
  }

}

int count_non_fixed_blocks(list<Block> block_list){
  int counter = 0;
  list<Block>::iterator iterator;
  for (iterator = block_list.begin(); iterator != block_list.end(); ++iterator) {
    if((*iterator).is_fixed == false){
      counter++;
    }
  }
  return counter;
}

double sum_weights_connected_to_block(list<Weight> weight_list, int block_num){
  double val = 0.0;
  list<Weight>::iterator iterator;
  for (iterator = weight_list.begin(); iterator != weight_list.end(); ++iterator) {
    if( ((*iterator).b_1->number == block_num ) ||
        ((*iterator).b_2->number == block_num )){
        val += (*iterator).value;
       
    }
  }
  return val;
}

double get_weight_between(list<Weight> weight_list, int b1, int b2){

  double weight_val = 0;
  list<Weight>::iterator iterator;
  for (iterator = weight_list.begin(); iterator != weight_list.end(); ++iterator) {
    Weight weight = (*iterator);
    if( ((weight.b_1->number == b1)&&(weight.b_2->number == b2)) ||
        ((weight.b_1->number == b2)&&(weight.b_2->number == b1)) ) {

        weight_val = (*iterator).value;
       
    }
  }
  return weight_val; //if not found connection, weight == 0
}

double sum_weight_to_fixed(list<Weight> weight_list, int block_num, char c){
  
  double tot_weight = 0;
  list<Weight>::iterator iterator;
  for (iterator = weight_list.begin(); iterator != weight_list.end(); ++iterator) {
    Weight weight = (*iterator);

    if((weight.b_1->number == block_num)&&(weight.b_2->is_fixed)){
     if(c == 'x'){
        tot_weight += weight.value * weight.b_2->x_coord;
      }else if(c == 'y'){
        tot_weight += weight.value * weight.b_2->y_coord;
      } 
    }
    if((weight.b_2->number == block_num)&&(weight.b_1->is_fixed) ){
      if(c == 'x'){
        tot_weight += weight.value * weight.b_1->x_coord;
      }else if(c == 'y'){
        tot_weight += weight.value * weight.b_1->y_coord;
      }
    }
  }//end for
  return tot_weight;
}

void print_int_array(int arr[], int size){
  for(int i = 0; i< size; i++){
    cout << arr[i] << ", ";
  }
  cout << endl;
}
void print_double_array(double arr[], int size){
  for(int i = 0; i< size; i++){
    cout << arr[i] << ", ";
  }
  cout << endl;
}

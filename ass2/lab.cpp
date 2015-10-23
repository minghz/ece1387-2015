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
list<BlockSet> block_set_list;

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

void generate_weights(void);
int count_non_fixed_blocks(list<Block> block_list);
double sum_weight_to_fixed(list<Weight> weight_list, int block_num, char c);

void model_clique(const Net net, list<Weight> * weight_list);
void calculate_total_hpwl(void);
void calculate_placement(void);

//pamper argc and argv[]
void print_instructions(void);

//parse inputed file
void parse_circuit_file(string filename);

//// Callbacks for event-driven window handling.
void randomize_fixed_blocks(void (*drawscreen_ptr) (void));
void spread_blocks(void (*drawscreen_ptr) (void));

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

  //if(debug_mode) print_net_list(net_list);
  //if(debug_mode) print_block_list(block_list);

  generate_weights();

  //if(debug_mode) print_weight_list(weight_list);

  calculate_placement();

  calculate_total_hpwl();

	/**************** initialize display **********************/
	init_graphics("ECE1387", WHITE); // window title
	update_message("Assignment 2 - 2015"); //bottom message
	set_visible_world(initial_coords);

	create_button ("Window", "Randomize FB", randomize_fixed_blocks); // name is UTF-8
	create_button ("Window", "Spread", spread_blocks); // name is UTF-8

	event_loop(NULL, NULL, NULL, drawscreen);   

	close_graphics ();
	std::cout << "Graphics closed down.\n";

	return (0);
}

void drawscreen (void) {

	set_draw_mode (DRAW_NORMAL);  // Should set this if your program does any XOR drawing in callbacks.
	clearscreen();  /* Should precede drawing for all drawscreens */

  //the draw the weights - represent the "connections"
  {
    setlinestyle (SOLID);
    setlinewidth (1);
    setcolor(BLUE);
    list<Weight>::iterator it;
    for(it = weight_list.begin(); it != weight_list.end(); ++it){
      t_point p1 = t_point((*it).b_1->x_coord*10, (*it).b_1->y_coord*10);
      t_point p2 = t_point((*it).b_2->x_coord*10, (*it).b_2->y_coord*10);
      drawline(p1, p2);
    } 
  }

  //draw dots representing the block positions
  //draw numbers representing block number
  {
    setlinestyle (SOLID);
    setlinewidth (1);
    list<Block>::iterator it;
    for(it = block_list.begin(); it != block_list.end(); ++it){
      if((*it).is_fake){
        setcolor(GREEN);
        setfontsize(10);
      } else {
        setcolor(RED);
        setfontsize(15);
      }
      t_point p0 = t_point(((*it).x_coord*10), ((*it).y_coord*10));
      t_point p1 = t_point(((*it).x_coord*10)-5, ((*it).y_coord*10)-5);
      t_point p2 = t_point(((*it).x_coord*10)+5, ((*it).y_coord*10)+5);
      drawrect(p1, p2);
      drawtext(p0, to_string((*it).number), 50, 50);
      
    }
  }

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


void model_clique(Net net, list<Weight> * weight_list){

  list<Block*> blocks = net.block_list;

  if(net.tot_blocks == 2){
    double w_val = 1.0;
    if(net.has_fake_block()) w_val = 10.0;
    (*weight_list).push_back(
        *(new Weight(blocks.front(), blocks.back(), w_val))
        );

  } else if (net.tot_blocks > 2){
    //if(debug_mode) cout << "model_clique " << net <<endl;

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

int count_non_fixed_blocks(BlockSet block_set){
  int counter = 0;
  list<Block*> block_list = block_set.block_set;
  list<Block*>::iterator iterator;
  for (iterator = block_list.begin(); iterator != block_list.end(); ++iterator) {
    if((*iterator)->is_fixed == false){
      counter++;
    }
  }
  return counter;
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

void calculate_placement(void){

  //tot movable blocks - find 2D array size
  int num_movable_blocks = count_non_fixed_blocks(block_list);

  if(debug_mode) 
    cout << "Num non-fixed blocks " << num_movable_blocks << endl;

  //processing matrix calculations
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

//  if(debug_mode){
//    //print q_matrix
//    cout << "print q_matrix" << endl;
//    for(int i = 0; i < num_movable_blocks; i++){
//      for(int j = 0; j < num_movable_blocks; j++){
//        cout << q_matrix[i][j] << " " ;
//      }
//      cout << endl;
//    }
//    cout << "end print q_matrix" << endl;
//  }

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

//  print_int_array(Ap, num_movable_blocks+1);
//  print_int_array(Ai, non_zero_entries);
//  print_double_array(Ax, non_zero_entries);
//  if(debug_mode){
//  cout << "bx\n" ;
//  print_double_array(bx, num_movable_blocks);
//  cout << "by\n" ;
//  print_double_array(by, num_movable_blocks);
//}
//  print_double_array(x, num_movable_blocks);

  umfpack_matrix_solver (
      num_movable_blocks,
      Ap,
      Ai,
      Ax,
      bx,
      x);
 // if(debug_mode){
 //   for (int i = 0 ; i < num_movable_blocks ; i++) 
 //     cout << "x ["<< i <<"] = "<< x[i] << " | map_to_block " << q_map[i] << endl;
 // }

  umfpack_matrix_solver (
      num_movable_blocks,
      Ap,
      Ai,
      Ax,
      by,
      y); 
 // if(debug_mode){
 //   for (int i = 0 ; i < num_movable_blocks ; i++) 
 //     cout << "y ["<< i <<"] = "<< y[i] << " | map_to_block " << q_map[i] << endl;
 // }

   for (int i = 0 ; i < num_movable_blocks ; i++) 
     (*get_block(&block_list, q_map[i])).write_coords(x[i],y[i]);

}

void calculate_total_hpwl(void){

   //calculate HPWL for all nets
    double tot_hpwl = 0;
    list<Net>::iterator it;
    for(it = net_list.begin(); it != net_list.end(); ++it){
      if((*it).has_fake_block()) continue; //lets not calculate for fake
      double max_x = 0;
      double min_x = 100;
      double max_y = 0;
      double min_y = 100;
      list<Block*> block_list = (*it).block_list;
      list<Block*>::iterator bp_it;
      for(bp_it = block_list.begin(); bp_it != block_list.end(); ++bp_it){
        double x = (*bp_it)->x_coord;
        double y = (*bp_it)->y_coord;
        if(x > max_x){ max_x = x; }
        if(x < min_x){ min_x = x; }
        if(y > max_y){ max_y = y; }
        if(y < min_y){ min_y = y; }
      }//end for loop - iterate through net's blocks - found BB for net
      double hpwl = max_x - min_x + max_y - min_y;
      tot_hpwl += hpwl;
    }//end for loop - iterate through another net
  
    cout << "Total HPWL: "<< tot_hpwl << endl;

}

void add_fake_blocks(double x, double y, list<Block*> block_set){

  int max_net_num = net_list.size();
  int max_block_num = block_list.size();
  
  //add block to block list
  Block * fake_block = new Block(max_block_num+1, true, true, x, y);
  block_list.push_back(*fake_block);
  delete(fake_block);
  fake_block = &block_list.back();

  //add nets connecting every block to the fake block
  {
    int counter = max_net_num;
    list<Block*>::iterator it;
    for(it = block_set.begin(); it != block_set.end(); ++it){
      //dont' add itself
      if((*it)->number == fake_block->number) continue;

      //create new net with fake<-->this block
      Net * net = new Net(counter + 1);
      net->tot_blocks = 2;
      net->block_list.push_back(*it);
      net->block_list.push_back(fake_block);
      //add to net_list and get back reference
      net_list.push_back(*net);
      delete(net);
      net = &net_list.back();
      //add the net to the newly created fake block
      fake_block->add_net(net);

      counter ++;
    }//end for - iterating all blocks
  
  }


}

void generate_weights(void){

  weight_list.clear();

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
}


/* Button Handlers */

void randomize_fixed_blocks(void (*drawscreen_ptr) (void)){
  
  //count number of fixed blocks
  int fix_block_num = 0;
  list<Block>::iterator it;
  for(it = block_list.begin(); it != block_list.end(); ++it){
    if((*it).is_fixed){
      fix_block_num ++;
    }
  }

  //get an array of pointers to fixed blocks
  Block * fix_block_pt[fix_block_num];

  //fill up array
  int i = 0;
  for(it = block_list.begin(); it != block_list.end(); ++it){
    if((*it).is_fixed){
       fix_block_pt[i] = &(*it);
       i++;
    }
  }

  //swap block number with random fixed block
  for(int j = 0; j < fix_block_num; j++){
    int from = j;
    int to = rand() % fix_block_num;
    
    int from_valx = fix_block_pt[from]->x_coord;
    int to_valx = fix_block_pt[to]->x_coord;
    int from_valy = fix_block_pt[from]->y_coord;
    int to_valy = fix_block_pt[to]->y_coord;

    fix_block_pt[from]->x_coord = to_valx;
    fix_block_pt[from]->y_coord = to_valy;
    fix_block_pt[to]->x_coord = from_valx;
    fix_block_pt[to]->y_coord = from_valy;
  }

  calculate_placement();

  calculate_total_hpwl();

  drawscreen_ptr ();  

}

void spread(void){

  BlockSet block_set = block_set_list.front();
  block_set_list.pop_front();

  cout << "spreading this " << block_set << endl;
  //tot number of non-fixed
  int num_movable_blocks = count_non_fixed_blocks(block_set);

  //calculate mid point of all blocks
  double total_x = 0;
  double total_y = 0;
  
  list<Block*>::iterator it;
  for(it = block_set.block_set.begin();
      it != block_set.block_set.end();
      ++it){
    if((*it)->is_fixed || (*it)->is_fake) continue;
    total_x += (*it)->x_coord;
    total_y += (*it)->y_coord;
  }
  
  double ave_x = total_x /(double) num_movable_blocks;
  double ave_y = total_y /(double) num_movable_blocks;
  
  //separate into lists per quadrant
  //
  // q2 | q1
  // -------
  // q3 | q4
  //
  BlockSet block_set_q1;
  BlockSet block_set_q2;
  BlockSet block_set_q3;
  BlockSet block_set_q4;

  for(it = block_set.block_set.begin();
      it != block_set.block_set.end();
      ++it){
    if((*it)->is_fixed || (*it)->is_fake) continue;
    double x = (*it)->x_coord;
    double y = (*it)->y_coord;
    if        (x > ave_x && y > ave_y){ //block_set_q1
      block_set_q1.block_set.push_back((*it));
    } else if (x < ave_x && y > ave_y){ //q2
      block_set_q2.block_set.push_back((*it));
    } else if (x <= ave_x && y <= ave_y){ //q3
      block_set_q3.block_set.push_back((*it));
    } else if (x >= ave_x && y <= ave_y){ //q4
      block_set_q4.block_set.push_back((*it));
    } else {
      cout << "Error: some blocks can't find a quadrant" <<endl;
    }
  }// end sorting into quadrants
  
  //find positioning of fake blocks
  double lower_bound_x = block_set.low_left.x;
  double lower_bound_y = block_set.low_left.y;
  double upper_bound_x = block_set.up_right.x;
  double upper_bound_y = block_set.up_right.y;

  double quarter_distance = (upper_bound_x - lower_bound_x)/4;
  double half_distance = (upper_bound_x - lower_bound_x)/2;

  double middle_bound_x = (upper_bound_x - lower_bound_x)/2;
  double middle_bound_y = (upper_bound_y - lower_bound_y)/2;

  cout << "lowerx: "<<lower_bound_x<<" lowery: "<<lower_bound_y
       << " upperx: "<<upper_bound_x<<" uppery: "<<upper_bound_y
       << " middlex: "<<middle_bound_x<<" middley: "<<middle_bound_y
       << endl;

  block_set_q4.low_left.x = lower_bound_x + half_distance;
  block_set_q4.low_left.y = lower_bound_y;
  block_set_q4.up_right.x = upper_bound_x;
  block_set_q4.up_right.y = lower_bound_y + half_distance;
  
  block_set_q3.low_left.x = lower_bound_x;
  block_set_q3.low_left.y = lower_bound_y;
  block_set_q3.up_right.x = lower_bound_x + half_distance;
  block_set_q3.up_right.y = lower_bound_y + half_distance;
  
  block_set_q2.low_left.x = lower_bound_x;
  block_set_q2.low_left.y = lower_bound_y + half_distance;
  block_set_q2.up_right.x = lower_bound_x + half_distance;
  block_set_q2.up_right.y = upper_bound_y;
  
  block_set_q1.low_left.x = lower_bound_x + half_distance;
  block_set_q1.low_left.y = lower_bound_y + half_distance;
  block_set_q1.up_right.x = upper_bound_x;
  block_set_q1.up_right.y = upper_bound_y;

  cout << block_set_q1 << endl;
  cout << block_set_q2 << endl;
  cout << block_set_q3 << endl;
  cout << block_set_q4 << endl;

  double area_q1_x = lower_bound_x + half_distance + quarter_distance;
  double area_q1_y = lower_bound_y + half_distance + quarter_distance;

  double area_q2_x = lower_bound_x + half_distance - quarter_distance;
  double area_q2_y = lower_bound_y + half_distance + quarter_distance;
  
  double area_q3_x = lower_bound_x + half_distance - quarter_distance;
  double area_q3_y = lower_bound_y + half_distance - quarter_distance;
  
  double area_q4_x = lower_bound_x + half_distance + quarter_distance;
  double area_q4_y = lower_bound_y + half_distance - quarter_distance;

  //add fake blocks
  if(block_set_q1.block_set.size() != 0){
    add_fake_blocks(area_q1_x, area_q1_y, block_set_q1.block_set);
    block_set_list.push_back(block_set_q1);
  }
  if(block_set_q2.block_set.size() != 0){
    add_fake_blocks(area_q2_x, area_q2_y, block_set_q2.block_set);
    block_set_list.push_back(block_set_q2);
  }
  if(block_set_q3.block_set.size() != 0){
    add_fake_blocks(area_q3_x, area_q3_y, block_set_q3.block_set);
    block_set_list.push_back(block_set_q3);
  }
  if(block_set_q4.block_set.size() != 0){
    add_fake_blocks(area_q4_x, area_q4_y, block_set_q4.block_set);
    block_set_list.push_back(block_set_q4);
  }

}


void spread_blocks(void (*drawscreen_ptr) (void)){

  if(block_set_list.size() == 0){
    //first time clicked
    BlockSet all_blocks;

    list<Block>::iterator it;
    for(it = block_list.begin(); it != block_list.end(); ++it){
      all_blocks.block_set.push_back(&(*it));
    }

    all_blocks.low_left.x = 0;
    all_blocks.low_left.y = 0;
    all_blocks.up_right.x = 100;
    all_blocks.up_right.y = 100;

    block_set_list.push_back(all_blocks);
  }

  spread();

  //need to re-generate weights for everybody
  generate_weights();

  calculate_placement();

  calculate_total_hpwl();
  
  drawscreen_ptr();
}
/* End Button Handlers */

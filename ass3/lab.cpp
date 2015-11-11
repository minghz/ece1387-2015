#include <iostream>
#include <cfloat> // for FLT_MAX
#include <chrono>
#include <thread>
#include <future>
#include <cstdlib>
#include <vector>
#include <list>
#include <boost/tokenizer.hpp>
#include <ctime>
#include <math.h>
#include "graphics.h"
#include "lab.h"
#include "helpers.h"


using namespace std;
using namespace boost;

/*
 ******************* Constants
 */
bool debug_mode = false;
int balance; //tot ntot num blocks / 2
int best_cost;
Node* best_leaf = NULL;
int nodes_visited = 0;
list<Block> block_list;
list<Net> net_list;
Node* root  = NULL;//root of our tree
list<thread> all_threads;
mutex best_cost_lock; 
mutex thread_size_lock;

bool thread_mode = false;

void drawscreen (void);
const t_bound_box initial_coords = t_bound_box(-500,1000,500,0); 
/* 
 ******************* END Constants
 */

/* 
 ******************* Functions
 */
Block* get_block(list<Block>* block_list, int n);
void print_net_list(list<Net> net_list);
void print_block_list(list<Block> block_list);
void print_int_array(int arr[], int size);
void print_double_array(double arr[], int size);
void print_best_result(Node* best_leaf);
string parse_input(int ac, char* const av[]);

bool has_block(list<Block*> bl, Block* b);
bool has_cross_block(list<Block*> bl, list<Block*> rB, list<Block*> lB);
bool compare_fanout(const Block& first, const Block& second);
bool block_connects(Node * n1, Node* n2);
bool has_net(list <Net*> nlist, Net* n);

int count_non_fixed_blocks(list<Block> block_list);
int get_balance(Node* node);
int get_lower_bound(Node * node);

bool traverse_tree(Node* node, list<Block>::iterator b_it);
void draw_tree(Node * node, t_point position, int curr_block, int num_blocks);

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


  

  string cct_file_name = parse_input (argc, argv);
	
  std::cout << "Parsing cct File " << cct_file_name << endl;
  parse_circuit_file(cct_file_name);

  if(debug_mode) print_net_list(net_list);
  if(debug_mode) print_block_list(block_list);

  //step1: decision tree
  
  //step2: construct initial solution to entire problem
  //balance of blocks
  balance = block_list.size() / 2;
  cout << "balance " << balance << endl;
  list<Block*> rB;
  list<Block*> lB;
  
  list<Net>::iterator it;
  for (it = net_list.begin(); it != net_list.end(); ++it){
    //iterate through each Net
    Net net = *it;
    list<Block*>::iterator b_list;
    
    for(b_list = net.block_list.begin();
        b_list != net.block_list.end();
        ++b_list){
        // put all that net's blocks into a set until its full
      if(rB.size() < balance && !has_block(rB, *b_list)){
        if (debug_mode) cout << "right side "<< (*b_list)->number << endl;
        rB.push_back(*b_list);
      }else if (rB.size() >= balance && !has_block(lB, *b_list) && !has_block(rB, *b_list)){
        if (debug_mode) cout << "left side "<< (*b_list)->number << endl;
        lB.push_back(*b_list);
      } else {
        //cout << "repeat\n";
      }
    }// end for
  }// end for
  
  //now we have cut into rB and lB block groups. lets find the cut cost
  //go though each Net. If the net has blocks in both sets, cut++
  int cut_count = 0;
  //list<Net>::iterator it;
  for(it = net_list.begin(); it != net_list.end(); ++it){
    Net net = *it;
    if(has_cross_block(net.block_list, rB, lB)){
      cut_count ++;
    }
  }

  //best_cost = cut_count;
  //SABOTAGE!!!
  best_cost = 1000;
  if (debug_mode) cout << "initial cost " << best_cost << endl;

  //step3: create bounding function that can be applied to any node
  // if current_node == right_side
  //  go up parent untill reach root, if node == left_side, save into list
  //  for each node in left_side list
  //    if node connects with current_node
  //      lower bound ++
  // if current_node == left_side
  //  do same thing as right_side, but other way around

  //step4: traverse tree
  // sort block_list into max fanout first
  
  block_list.sort(compare_fanout);
  if(debug_mode) print_block_list(block_list);

  // traverse/create depth-first tree, pruning as we go
  // pre-order traversal for depth-first
  list<Block>::iterator b_it = block_list.begin();
  root = new Node(&block_list.front(), false); //put root on left
  root->parent = NULL;
  root->is_root = true;

  clock_t begin = clock();
  if(thread_mode){
    std::future<bool> main_finished = std::async(traverse_tree, root, b_it);
    //main_finished.get();
    //all_threads.push_back(thread(traverse_tree, root, b_it));

   // list<thread>::iterator ti;
   // for(ti = all_threads.begin(); ti !=all_threads.end(); ++ti){
   //   (*ti).join();
   // }
  } else {
    traverse_tree(root, b_it);
  }

  clock_t end = clock();
  double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;


  cout << endl;
  cout << "Best Cost: " << best_cost << endl;
  print_best_result(best_leaf);
  cout << "Seconds Elapsed: " << elapsed_secs << endl;
  cout << "Nodes Visited: " << nodes_visited << endl;

	/**************** initialize display **********************/
	init_graphics("ECE1387", WHITE); // window title
	update_message("Assignment 3 - 2015"); //bottom message
	set_visible_world(initial_coords);

	//create_button ("Window", "Randomize FB", randomize_fixed_blocks); // name is UTF-8
	//create_button ("Window", "Spread", spread_blocks); // name is UTF-8

	event_loop(NULL, NULL, NULL, drawscreen);   

	close_graphics ();
	std::cout << "Graphics closed down.\n";

	return (0);
}

void drawscreen (void) {

	set_draw_mode (DRAW_NORMAL);  // Should set this if your program does any XOR drawing in callbacks.
	clearscreen();  /* Should precede drawing for all drawscreens */

  //draw tree something here
  //traverse through generated tree, and draw it 
  int vertical_interval = 1000 / block_list.size();
  int num_blocks = block_list.size();
  t_point root_pos = t_point(0, 50); //starts on top

  setcolor(BLACK);

  setfontsize(15);
      //drawline(p1, p2);
  draw_tree(root, root_pos, 1, num_blocks);
  
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
      cout << "Finished parsing" << endl;
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
        block = new Block(n);
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
        block_list.back().fanout ++;
      }

    } //end for

  }// end while

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

string parse_input(int ac, char* const av[]){

  string cct_file_name = "";
  if(ac > 4 || ac == 1){
    print_instructions();
    return 0;
  } else {
    for(int i = 1; i < ac; i++){
      if(av[i] == string("-t")){
        thread_mode = true;
      }else if(av[i] == string("-b")){
        thread_mode = false;
      }else if(av[i] == string("-v")){
        debug_mode = true;
      }else{
        cct_file_name = av[i];
      }
    }// end for
  }
  if(cct_file_name == "") {
    cout << "Error: <circuit_file> was not provided" << endl;
    print_instructions();
    return 0;
  }
  if(thread_mode){
    cout << "Threaded Mode" << endl;
  }else{
    cout << "Non-threaded Mode" << endl;
  }
  if(debug_mode){
    cout << "Verbose Mode On" << endl;
  }
  cout << endl;

  return cct_file_name;
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

bool has_block(list<Block*> bl, Block* b){
 
  bool has = false; 
  list<Block*>::iterator it;
  for(it = bl.begin(); it != bl.end(); ++it){
    if((*it)->number == b->number)
      has = true;
  }

  return has;
}

bool has_cross_block(list<Block*> bl, list<Block*> rB, list<Block*> lB){
  //returns true if 'bl' has blocks on both rB and lB
  bool rB_has = false;
  bool lB_has = false;

  list<Block*>::iterator it;
  for(it = bl.begin(); it != bl.end(); ++it){
    if(has_block(rB, *it)){
      rB_has = true;
    }
    if(has_block(lB, *it)){
      lB_has = true;
    }
    if(rB_has && lB_has){
      return true;
    }
  }//end for

  return false;
}

bool compare_fanout(const Block& first, const Block& second){
  //used for sorting list<Block> for highest fanout first
  return(first.fanout > second.fanout);
}


bool traverse_tree(Node* node, list<Block>::iterator b_it){

  //if(thread_mode){
  //  thread_size_lock.lock();
  //  cout << "num_threads " << all_threads.size() << endl;
  //  thread_size_lock.unlock();
  //}
  nodes_visited ++;
  //if(debug_mode) cout << "node-num: "<< node->block->number << endl;
  node->lb = get_lower_bound(node);
  node->balance = get_balance(node);
  
  list<Block>::iterator check_next = next(b_it, 1);
  if(check_next == block_list.end()) node->is_leaf = true;

  //is balance violated?
  if(node->balance > balance){
    node->is_pruned = true;
    //if(debug_mode) cout << "broke-balance!"<<endl;
  }
  //is higher than lower bound
  if(node->lb >= best_cost){
    node->is_pruned = true;
    //if(debug_mode) cout << "broke-LB!"<<endl;
  }
  //is a leaf and cost is better
  if(node->is_leaf){
    if(thread_mode) best_cost_lock.lock();
    if(node->lb < best_cost){
      best_cost = node->lb;
      best_leaf = node;
    }
    node->is_leaf = true;
    if(thread_mode) best_cost_lock.unlock();    
    //if(debug_mode) cout << "better-leaf-cost!"<<endl;
  }

  //testinggggggg
  std::thread::id this_id = std::this_thread::get_id();
  thread_size_lock.lock();
  cout << *node << " thread ID: " << this_id << endl;
  thread_size_lock.unlock();

  if(node->is_pruned || node->is_leaf)
    return true;


  //is not a leaf, and we still havn't found our answer
  ++b_it;
  Node* lnode = new Node(&(*b_it), false); //left node
  lnode->parent = node;
  node->left = lnode;

  Node* rnode = new Node(&(*b_it), true); //right node
  rnode->parent = node;
  node->right = rnode;

  if(thread_mode){
    std::future<bool> left_finished = std::async(traverse_tree, node->left, b_it);
    //all_threads.push_back(thread(traverse_tree, node->left, b_it));

    std::future<bool> right_finished = std::async(traverse_tree, node->right, b_it);
    //all_threads.push_back(thread(traverse_tree, node->right, b_it));
  
    //left_finished.get();
    //right_finished.get();

  } else {
    traverse_tree(node->left, b_it);
    traverse_tree(node->right, b_it);
  }

  return true;
}

int get_balance(Node* node){

  bool at_right = node->tr_fl;
  int num_this_side = 1; //count current node into side
  Node* parent = node->parent;
  while(parent != NULL){ // not reached root yet
    if(parent->tr_fl == at_right){ // same side as parent
      num_this_side ++;
    }
    parent = parent->parent;
  }

  return num_this_side;
}

int get_lower_bound(Node * node){
  //keep going up the parent
  //grab all nodes not on my side
  //see if I connect with those nodes
  //they can't be on the same NET as me, because only counted once
  //if I do, lb++
  bool at_right = node->tr_fl;  
  list<Node*> other_side_list;
  int lb = 0;

  if(! node->is_root){
    node->counted_nets = node->parent->counted_nets;
  }

  Node * parent = node->parent;
  while(parent != NULL){ // not reached root yet
    if(parent->tr_fl != at_right){ // not node's side
      other_side_list.push_back(parent);
    }
    parent = parent->parent;
  }

  //does node connecte with folks in other_side_list?
  list<Node*>::iterator it;
  for(it = other_side_list.begin(); it != other_side_list.end(); ++it){
    if(block_connects(node, *it)){
      lb ++;
    }
  }//end for

  //all my lb to my direct parent's lb
  if(node->parent != NULL)
    lb = lb + node->parent->lb;

  //return that
  return lb;
}

bool block_connects(Node * node1, Node* node2){
  //see if the blocks of these two nodes connect
  Block * b1 = node1->block;
  Block * b2 = node2->block;

  //see if these two have matching net numbers
  list<Net*> netL1 = b1->net_list;
  list<Net*> netL2 = b2->net_list;
  list<Net*>::iterator netL1i;
  list<Net*>::iterator netL2i;

  for(netL1i = netL1.begin(); netL1i != netL1.end(); ++netL1i){
    int n1 = (*netL1i)->number;
    for(netL2i = netL2.begin(); netL2i != netL2.end(); ++netL2i){
      int n2 = (*netL2i)->number;
      if(n1 == n2 && !has_net(node1->counted_nets, *netL1i)){ //same netlist
        node1->counted_nets.push_back(*netL1i); //this net as already counted
        return true;
      }
    }
  }//end for

  return false;
}

bool has_net(list <Net*> nlist, Net* n){

  list<Net*>::iterator it;
  for(it = nlist.begin(); it != nlist.end(); ++it){
    if((*it)->number == n->number){
      return true;
    }
  }

  return false;
}

void draw_tree(Node * node, t_point position, int curr_block, int num_blocks){

  curr_block ++;
  float factor = pow(2, curr_block);
  float hspacer = 1000 / factor;
  float vspacer = 1000 / factor;
  //float vspacer = 1000 / num_blocks;
  //num_blocks;
  

  setfontsize(10);
  //draw lower bound at node
  setcolor(GREEN);
  drawtext(position + t_point(0, 10/factor), "LB:"+ to_string(node->lb), 20, 20);

  //draw if its left or right
  if(node->tr_fl == true) //right node
    drawtext(position + t_point(0, 20/factor), "R:"+to_string(node->balance), 20, 20);
  else
    drawtext(position + t_point(0, 20/factor), "L:"+to_string(node->balance), 20, 20);


  setfontsize(12);
  setcolor(BLACK);
  //draw current node
  if(node->is_pruned){
    setcolor(RED);
  }
  drawtext(position, to_string(node->block->number), 50, 50);
  

  //resume
  setfontsize(12);
  setcolor(BLACK);
  
  if(node->left != NULL){
    //draw left node
    draw_tree(node->left, position + t_point(-hspacer, vspacer), curr_block, num_blocks);
    //draw connection to left node
    setcolor(BLUE);
    drawline(position, position + t_point(-hspacer, vspacer));
    setcolor(GREEN);

  }

  if(node->right != NULL){
    //draw right node
    draw_tree(node->right, position + t_point(hspacer, vspacer), curr_block, num_blocks);
    //draw connection to right node
    setcolor(BLUE);
    drawline(position, position + t_point(hspacer, vspacer));
    setcolor(RED);
  }

}

void print_best_result(Node* best_leaf){

  cout << "\t" << "LEFT" << "\t" << "RIGHT"<< endl;

  for(Node* n = best_leaf; n != NULL; n = n->parent){
    if(n->tr_fl)
      cout << "\t\t" << n->block->number <<endl;
    else
      cout << "\t" << n->block->number <<endl;
  }
}

/* Button Handlers */

/* End Button Handlers */

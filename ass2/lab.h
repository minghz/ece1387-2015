#ifndef LAB_H
#define LAB_H
#include <iostream>
#include <string>
#include <cstdint>
#include <fstream>
#include <list>
#include "easygl_constants.h"
#include "graphics.h"

using namespace std; 

class Net;
class Block;
class Weight;

class Block{
  public:
    int number;
    bool is_fixed;
    bool is_fake; // virtual fixed blocks for positioning  
    double x_coord;
    double y_coord;
    list<Net*> net_list; //list of pointers to Net objects it belongs to

    //constructor
    Block(int number, bool is_fixed, bool is_fake, double x_coord, double y_coord);

    void add_net(Net * net);
    void write_coords(double x, double y);
};

void Block::write_coords(double x, double y){
  x_coord = x;
  y_coord = y;
}

Block::Block(int n, bool fix, bool fake, double x, double y){
  number = n;
  is_fixed = fix;
  is_fake = fake;
  x_coord = x;
  y_coord = y;
}
void Block::add_net(Net * net){
  net_list.push_back(net);
}

class Weight{
  public:
    double value; //weight of the 2 pin connection
    Block* b_1;
    Block* b_2;
    //constructor
    Weight(Block * b1, Block * b2, double val);
    //comparison operator
    friend bool operator== (Weight &w1, Weight &w2);
    friend bool same_connection(Weight &w1, Weight &w2);
};

Weight::Weight(Block * b1, Block * b2, double val){
  value = val;
  b_1 = b1;
  b_2 = b2;
}
//easy compare weights
bool operator== (Weight &w1, Weight &w2){
  return ((w1.b_1 == w2.b_1 && w1.b_2 == w2.b_2) ||
          (w1.b_1 == w2.b_2 && w1.b_2 == w2.b_1)
          );
}
bool same_connection(Weight &w1, Weight &w2){
  return ((w1.b_1 == w2.b_1 && w1.b_2 == w2.b_2) ||
          (w1.b_1 == w2.b_2 && w1.b_2 == w2.b_1)
          );
}

class Net{
  public:
    int number;
    int tot_blocks;
    list<Block*> block_list;

    //constructor
    Net(int n);

    void add_block(Block * block);
    void set_tot_blocks();
    bool has_fake_block();
};

Net::Net(int n){
  number = n;
}
bool Net::has_fake_block(){
  list<Block*>::iterator it;
  for(it = block_list.begin(); it != block_list.end(); ++it){
    if((*it)->is_fake) return true;
  }
  return false;
}
void Net::add_block(Block * block){
  block_list.push_back(block);
}
void Net::set_tot_blocks(){
  int counter = 0;
  for (list<Block*>::iterator it=block_list.begin(); it != block_list.end(); ++it) {
    counter ++;
  }
  tot_blocks = counter;
}

class Coordinates {
  public:
    double x;
    double y;

    //constructor
    Coordinates();
    Coordinates(double x_val, double y_val);
};
Coordinates::Coordinates(){}

Coordinates::Coordinates(double x_val, double y_val){
  x = x_val;
  y = y_val;
}

class BlockSet{
  public:
  list<Block*> block_set;
  Coordinates low_left;
  Coordinates up_right;

  BlockSet();
};

BlockSet::BlockSet(){}

ostream& operator<< (ostream & out, BlockSet const& data){
  out << "BlockSet= low_left x:"<< data.low_left.x 
      << " | y:"<< data.low_left.y << " | ";
  out << "up_right x:"<< data.up_right.x
      <<" | y:"<< data.up_right.y<< " | ";
  list<Block*>::const_iterator iterator;
  for (iterator = data.block_set.begin();
      iterator != data.block_set.end();
      ++iterator) {
    out << (*iterator)->number << ",";
  }
  return out;
}

//printing a Weight object on cout
ostream& operator<< (ostream & out, Weight const& data) {

    out << "Weight= value: " << data.value << " | " ;
    out << "b_1: " << data.b_1->number << " | " ;
    out << "\t" << data.b_1 << " | " ;
    out << "b_2: " << data.b_2->number << " | " ;
    out << "\t" << data.b_2 << " | " ;
    return out ;
}

//printing a Net object on cout
ostream& operator<< (ostream & out, Net const& data) {
    out << "Net= number: " << data.number << "|" ;
    out << "tot_blocks: " << data.tot_blocks << "|" ;
    out << "block_list_num: ";
    list<Block*>::const_iterator iterator;
    for (iterator = data.block_list.begin();
        iterator != data.block_list.end();
        ++iterator) {
      out << (*iterator)->number << ",";
    }
    out << " | ";
    return out ;
}

//printing a Block object on cout
ostream& operator<< (ostream & out, Block const& data) {
    out << "Block= number: " << data.number << "|" ;
    out << "is_fixed: " << data.is_fixed << "|" ;
    out << "x_coord: " << data.x_coord << "|" ;
    out << "y_coord: " << data.y_coord << "|" ;
    out << "is_fake: " << data.is_fake << "|" ;

    out << "net_list_num: ";
    list<Net*>::const_iterator iterator;
    for (iterator = data.net_list.begin();
        iterator != data.net_list.end();
        ++iterator) {
      out << (*iterator)->number << ",";
    }
    out << " | ";

    return out ;
}

#endif // LAB_H

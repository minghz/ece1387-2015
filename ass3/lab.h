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
class Node;

class Block{
  public:
    int number;
    int fanout; //number of nets it is connected to
    list<Net*> net_list; //list of pointers to Net objects it belongs to

    //constructor
    Block(int number);

    void add_net(Net * net);
};

  
Block::Block(int n){
  number = n;
  fanout = 0;
}
void Block::add_net(Net * net){
  net_list.push_back(net);
}

class Node {
  public:
    
    int lb; //lower bound
    int balance; //how many on this side?

    Block* block;

    bool is_root;
    bool is_leaf;
    bool is_pruned;
    bool tr_fl; // true if its Right. false if its Left side

    list<Net*> counted_nets; //list of nets already counted in the LB

    Node* parent;
    Node* right;
    Node* left;

    //constructor
    Node(Block* block_ptr, bool tr_fl_side);
};

Node::Node(Block* block_ptr, bool tr_fl_side){
  block = block_ptr;
  right = NULL;
  left = NULL;
  tr_fl = tr_fl_side;

  parent = NULL;
  is_root = false;
  is_leaf = false;

  lb = 0;
  is_pruned = false;

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

//printing a Node object on cout
ostream& operator<< (ostream & out, Node const& data) {

    out << "Node= block#: " << data.block->number << " | " ;
    out << "lb: " << data.lb << " | " ;
    out << "is_root" << data.is_root << " | " ;
    out << "is_leaf: " << data.is_leaf << " | " ;
    out << "is_pruned" << data.is_pruned << " | " ;
    out << "tr_fl" << data.tr_fl << " | " ;
    if (data.is_root == false)
      out << "parent->block#" << data.parent->block->number << " | " ;
   // if (data.is_leaf == false){
   //   out << "right->block#" << data.right->block->number << " | " ;
   //   out << "left->block#" << data.left->block->number << " | " ;
   // }

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
    out << "fanout: " << data.fanout << "|" ;

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

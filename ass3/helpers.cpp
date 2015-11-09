#include <iostream>
#include <cstdlib>
#include <vector>
#include <list>
#include <boost/tokenizer.hpp>
#include "lab.h"

using namespace std;

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



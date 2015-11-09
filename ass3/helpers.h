#ifndef HELPERS_H
#define HELPERS_H

#include <iostream>
#include <cstdlib>
#include <list>
#include "lab.h"

//helper functions
bool net_number_exists(list<Net> net_list, int n);
Net* get_net(list<Net>* net_list, int n);
bool has_block(list<Block*> bl, Block* b);
#endif // HELPERS_H


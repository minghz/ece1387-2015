#ifndef LAB_H
#define LAB_H
#include <iostream>
#include <string>
#include <cstdint>
#include <fstream>
#include "easygl_constants.h"
#include "graphics.h"

class Track {
  public:
    int x; //x - coordinate
    int y; //y - coordinate
    int z; //vertical == 1, horizontal == 0
    int wire; // 0 to W-1
    int lb_x; // logic block x coord
    int lb_y; // logic block y coord
    int lb_p; // logic block pin number

    t_point s_pt; //start point to draw
    t_point e_pt; //end point to draw

    //constructor
    Track(){
      x = -1;
      y = -1;
      z = -1;
      wire = -1;
      lb_x = -1;
      lb_y = -1;
      lb_p = -1;
    }

};

class SourceSink{
  public:

    //source is 1, sink is 2
    int X1;
    int Y1;
    int P1;
    
    int X2;
    int Y2;
    int P2;

    SourceSink * next;
};

class Circuit {
  public:
    int grid_size;
    int tracks_per_channel;
    SourceSink * source_sink_head = new SourceSink();
};


#endif // LAB_H

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
    int x = -1; //x - coordinate
    int y = -1; //y - coordinate
    int z = -1; //vertical == 1, horizontal == 0
    int wire = -1; // 0 to W-1
    int lb_p = 0; // logic block pin number
    int lb_p2 = 0; // reserve in case of source=target
    
    int label = -1; //== cost
    bool is_labeled = false;
    bool is_unavailable = false;
    bool is_target = false;

    t_point s_pt; //start point to draw
    t_point e_pt; //end point to draw

    //comparison operator
    friend bool operator== (Track &t1, Track &t2);

};

//eazy compare Tracks
bool operator== (Track &t1, Track &t2){
  return (t1.x == t2.x &&
          t1.y == t2.y &&
          t1.z == t2.z &&
          t1.wire == t2.wire);
}

//printing a Track object on std::cout
std::ostream& operator<< (std::ostream & out, Track const& data) {
    out << "x: " << data.x << "|" ;
    out << "y: " << data.y << "|" ;
    out << "z: " << data.z << "|" ;
    out << "wire: " << data.wire << "|" ;
    out << "label: " << data.label << "|" ;
    out << "is_labeled: " << data.is_labeled << "|" ;
    out << "is_unavailable: " << data.is_unavailable << "|" ;
    out << "is_target: " << data.is_target << "|";
    out << "lb_p: " << data.lb_p << "|";
    out << "lb_p2: " << data.lb_p2 << "|";
    out << "s_pt:(" << data.s_pt.x << ", " << data.s_pt.y << ")"; //start point to draw
    out << " | e_pt:(" << data.e_pt.x << ", " << data.e_pt.y << ")"; //start point to draw
    return out ;
}

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

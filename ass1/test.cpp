#include <iostream>
#include <cstdlib>


int print_possible_conn(int x, int y, int z, int wire){

  int n, s, w, e;
  n = s = w = e = 0; 
  
  //horizontal track
  if(z == 0){
    //switch block connection in from west
    printf("n%d%d%dw%d, e%d%d%dw%d, s%d%d%dw%d\n",
          x+1,y,z+1,wire,
          x+1,y,z,wire,
          x+1,y-1,z+1,wire);

    //switch block connection in from east
    printf("s%d%d%dw%d, w%d%d%dw%d, n%d%d%dw%d\n",
        x,y-1,z+1,wire,
        x-1,y,z,wire,
        x,y,z+1,wire);

  // vertical track
  } else {
    //switch block connection from south
    printf("w%d%d%dw%d, n%d%d%dw%d, e%d%d%dw%d\n",
       x-1,y+1,z-1,wire,
       x,y+1,z,wire,
       x,y+1,z-1,wire); 

    //switch block connection from north
    printf("e%d%d%dw%d, s%d%d%dw%d, w%d%d%dw%d\n",
      x,y,z-1,wire,
      x,y-1,z,wire,
      x-1,y,z-1,wire); 
 
  }

  return 0;
}


int main(){
  
  printf("test123\n----\n");

  int x = 4;
  int y = 2;
  int z = 1; //vertical = 1, horizontal = 0
  int wire = 0;

  //1->south, 2->east, 3->north, 4->west

  printf("printing possible connections for: %d%d%dw%d\n", x,y,z,wire);
  print_possible_conn(x, y, z, wire);

  return 0;
}

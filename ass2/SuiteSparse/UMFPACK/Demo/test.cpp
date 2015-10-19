
#include <iostream>

extern "C" {
  #include "umfpack.h"
#include "test.h"
}

int main (){

//  int    n = 5 ;
//  int    Ap [ ] = {0, 2, 5, 9, 10, 12} ;
//  int    Ai [ ] = { 0,  1,  0,   2,  4,  1,  2,  3,   4,  2,  1,  4} ;
//  double Ax [ ] = {2., 3., 3., -1., 4., 4., -3., 1., 2., 2., 6., 1.} ;
//  double b [ ] = {8., 45., -3., 3., 19.} ;
//  double x [5] ;

  int    n = 5 ;
  int    Ap [ ] = {0, 2, 5, 9, 10, 12} ;
  int    Ai [ ] = { 0,  1,  0,   2,  4,  1,  2,  3,   4,  2,  1,  4} ;
  double Ax [ ] = {2., 3., 3., -1., 4., 4., -3., 1., 2., 2., 6., 1.} ;
  double b [ ] = {8., 45., -3., 3., 19.} ;
  double x [n] ;

  foo(n,
      Ap,
      Ai,
      Ax,
      b,
      x);

  for (int i = 0 ; i < n ; i++) 
    std::cout << "x ["<< i <<"] = "<< x[i] << std::endl;


  return 0;
}

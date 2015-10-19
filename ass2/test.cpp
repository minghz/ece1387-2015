#include <iostream>
#include <cstdint>
#include <list>
#include <cstdlib>

using namespace std;

int main (){

  //list by itself is pass by value
  //need to make a list of pointers to pass by reference
  list<int*> foo;
  list<int*> noo;

  int a = 2;
  int b = 3;
  int c = 4;
  int d = 5;

  foo.push_back(&a);
  foo.push_back(&b);
  foo.push_back(&c);
  foo.push_back(&d);

  noo.push_back(&a);

  *(noo.front()) = 100;

  cout << *(foo.front()) << endl;
  cout << *(noo.front()) << endl;

  return 0;
}

#ifndef PALM_H_INCLUDED
#define PALM_H_INCLUDED
#include "basetype.h"
#include "avl.h"
#include "btree.h"
#endif
//typedef int key_t;

typedef enum {LT,GT,NE,LE,GE,EQ} relop;
typedef enum {AND,OR} boolop;
struct condition {
      relop op;
      key_t value;
};
struct condition_set {
      int num;
      struct condition * c;
      boolop * bset;
};
#endif // PALM_H_INCLUDED

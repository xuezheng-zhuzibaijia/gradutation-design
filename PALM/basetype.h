#ifndef BASETYPE_H_INCLUDED
#define BASETYPE_H_INCLUDED
typedef int key_t;
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
#define TRUE 1
#define FALSE 0

#endif // BASETYPE_H_INCLUDED

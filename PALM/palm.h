#ifndef PALM_H_INCLUDED
#define PALM_H_INCLUDED
#include "basetype.h"
#include "avl.h"
#include "btree.h"
#ifndef MAX_QUERY_SIZE
#define MAX_QUERY_SIZE (MAX_LIST_SIZE/2)
#endif // MAX_QUERY_SIZE
#define THREADS 4
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
struct operation{
    optype op;
    key_t key;
    void * value;
    size_t vsize;
    void (*update)(void *data);
    struct condition_set * range_condition;
    struct data_list * preread_list;
    int id;
};
struct operation operation_list[MAX_QUERY_SIZE];
int operation_list_count;

struct keyop keyop_list[MAX_LIST_SIZE];
int keyop_list_count;
node_pointer targets[MAX_LIST_SIZE];
struct rangeop{
    int id;
    struct condition_set * range_condition;
    int read_id;
    int id_num;
    struct data_list * result;
};
struct rangeop rangeop_list[MAX_QUERY_SIZE];
int rangeop_list_count;


struct nodeop modified_list[MAX_LIST_SIZE];
int modified_list_count;

struct noppair{
    node_pointer n;
    keyop p;
};
#endif // PALM_H_INCLUDED

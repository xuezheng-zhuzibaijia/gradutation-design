#ifndef BTREE_H_INCLUDED
#define BTREE_H_INCLUDED
#include <libpmemobj.h>
#include "basetype.h"
#define BTREE_ORDER 8
#define BTREE_MIN (BTREE_ORDER / 2)
#ifndef BTREE_TYPE_OFFSET
#define BTREE_TYPE_OFFSET 1012
TOID_DECLARE(struct tree_node,BTREE_TYPE_OFFSET);
TOID_DECLARE(struct tree,BTREE_TYPE_OFFSET+1);
TOID_DECLARE(void,BTREE_TYPE_OFFSET+2);
TOID_DECLARE(struct leaf_arg,BTREE_TYPE_OFFSET+3);
typedef int key_t;
typedef (struct tree_node) node_pointer;
struct leaf_arg{
     int is_deleted;
     size_t  vsizes[BTREE_ORDER];
};
struct tree_node
{
    key_t keys[BTREE_ORDER];
    PMEMoid pointers[BTREE_ORDER+1];
    int num_keys;
    int is_leaf;
    node_pointer parent;
    TOID(struct leaf_arg) arg;
};
struct tree
{
    node_pointer root,leaf_head;
};
//operation:read,update,delete
#ifndef MAX_LIST_SIZE
#define MAX_LIST_SIZE 100
#endif // MAX_LIST_SIZE
typedef struct{ key_t key;void *value;size_t vsize;int id;} kvpair;
typedef enum {INSERT,DELETE,READ,UPDATE,RANGE} optype;
struct keyop{
        optype op;
        key_t key;
        void * value;
        size_t vsize;
        void (*update)(void *value);
        int id;
        PMEMoid child;
};
struct nodeop{
        int num;
        struct keyop * op_list;
        node_pointer n;
};
struct record{
        int id;
        key_t key;
};
struct record record_list[MAX_LIST_SIZE];

int record_list_count;
kvpair read_record[MAX_LIST_SIZE];
int read_record_count;

node_pointer make_node();
node_pointer make_leaf();
void btree_init(TOID(struct tree) t);
node_pointer get_leaf(node_pointer root,key_t key);
node_pointer get_leftest_leaf(node_pointer root);
void common_operation(struct nodeop *p,node_pointer * root);
#endif // BTREE_H_INCLUDED



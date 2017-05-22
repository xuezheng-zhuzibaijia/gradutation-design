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
typedef TOID(struct tree_node) node_pointer;
struct leaf_arg{
     int is_deleted;
     size_t  vsize[BTREE_ORDER];
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
typedef enum {INSERT,DELETE,READ,UPDATE} optype;
struct keyop{
        optype op;
        key_t key;
        void * value;
        size_t vsize;
        void (*update)(void *value);
        int id;
};
struct leafop{
        int num;
        struct keyop * op_list;
        node_pointer n;
};
struct parentop{
        optype op;
        key_t key;
        PMEMoid child;
};
node_pointer targets[MAX_LIST_SIZE];

struct parentop modified_list[MAX_LIST_SIZE];
int modified_list_count;

struct nodeop{
        int num;
        struct parentop * op_list;
        node_pointer n;
};
struct {
        int id;
        key_t key;
}change_record[MAX_LIST_SIZE];
int change_record_count;
kvpair * read_record[MAX_LIST_SIZE];
int read_record_count;

node_pointer make_node();
node_pointer make_leaf();
void btree_init(TOID(struct tree) t);

#endif // BTREE_H_INCLUDED

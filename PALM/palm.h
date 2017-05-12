#ifndef PALM_H_INCLUDED
#define PALM_H_INCLUDED

#define BTREE_ORDER 8
#define BTREE_MIN (BTREE_ORDER / 2)

#define TRUE  1
#define FALSE 0
#include <libpmemobj.h>  //compile command would be 'cc -std=gnu99　... -lpmemobj -lpmem'
#ifndef BTREE_TYPE_OFFSET
#define BTREE_TYPE_OFFSET 1012
#endif

/******************************avl tree**********************************************/
typedef struct avl_node avl_pointer;
struct avl_node
{
    int key;
    short int bf;
    avl_pointer left_child,right_child;
};
struct key_set
{
    int * key_list;
    int num;
};
int unbalanced;
avl_pointer avl_tree_root;
//function declaration
void avl_init();
void avl_destroy(avl_pointer * parent);
void avl_insert(avl_pointer *parent,int key,int *unbalanced);
struct key_set * get_interval_keyset(int i,int j);



/**********************tree declaration*******************************/
TOID_DECLARE(struct tree_node,BTREE_TYPE_OFFSET);
TOID_DECLARE(struct tree,BTREE_TYPE_OFFSET+1);
TOID_DECLARE(void,BTREE_TYPE_OFFSET+2);
enum dmark {NORMAL,DELETED,HEAD,TAIL};
struct leaf_arg
{
    int id[BTREE_ORDER];
    enum dmark mask;

};
struct tree_node
{
    int keys[BTREE_ORDER];
    PMEMoid pointers[BTREE_ORDER+1];
    int num_keys;
    int is_leaf;
    struct leaf_arg * arg; //This part　is volitile,and it‘s only used in leaf node
    TOID(struct tree_node) parent;
};
struct tree
{
    TOID(struct tree_node) root;
};
typedef TOID(struct tree_node) node_pointer;
#ifndef MAX_LIST_SIZE
#define MAX_LIST_SIZE 100
#endif // MAX_LIST_SIZE


typedef enum OP {IN,DEL,RD,UP,RANGE} optype;
typedef struct {int i,j;} int_pair;
typedef struct {int key;void * value;size_t length;} kv_pair;
/**************input structure declaration**************/
struct operation
{
    int id;
    optype op;
    union {
        int_pair interval;
        kv_pair in_up;
        int key;
    }arg;
};
struct operation operation_lst[MAX_LIST_SIZE];
int operation_lst_count;

struct range_op{
    int_pair interval;
    struct key_set * kset;
    int id_from;
};
struct range_op  range_set[MAX_LIST_SIZE];
int range_set_count;


/********************task declartion********************/
struct key_op{
        optype op;
        int key;
        union{
              PMEMoid value;
              void (*update)(PMEMoid value);
        }
};
struct node_opset{
       node_pointer node;
       struct key_op * key_op_set;
       int opcount;
};
struct task{
    struct node_opset *  node_set;
    int nodecount;
};


/********************modified list declaration***********/
struct modified_op{
     optype op;
     int key;
     PMEMoid value;
     void (*update)(PMEMoid value);
     node_pointer n;
};
struct modified_op modified_list[MAX_LIST_SIZE];
int modified_count;


/********************result declaration******************/
struct {
    int id;
    int key;
    void * value;
}read_result[MAX_LIST_SIZE*10];
int read_result_count;

struct op_result
{
    optype op;
    union
    {
        kv_pair read_result;
        struct
        {
            int i,j;
            int * key;
            void * value;
            int num;
        } range_result;
    }
};
struct op_result[MAX_LIST_SIZE];
int op_result_count;


#endif // PALM_H_INCLUDED

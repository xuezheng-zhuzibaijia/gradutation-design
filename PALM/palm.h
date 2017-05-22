#ifndef PALM_H_INCLUDED
#define PALM_H_INCLUDED

#include <libpmemobj.h>  //compile command would be 'cc -std=gnu99　... -lpmemobj -lpmem'
#include "basetype.h"
#include "avl.h"
#define BTREE_ORDER 8
#define BTREE_MIN (BTREE_ORDER / 2)
#ifndef BTREE_TYPE_OFFSET
#define BTREE_TYPE_OFFSET 1012
#endif
//typedef int key_t;
/******************************avl tree**********************************************/
//function declaration
void avl_init();
void avl_destroy(avl_pointer * parent);
void avl_insert(avl_pointer *parent,void * newdata,size_t data_size,int *unbalanced,int(*compare)(void * data,void * newdata),void (*update)(void *data,void *newdata));

struct data_set * get_avl_dataset(int(*contain)(void *data));
TOID_DECLARE(struct tree_node,BTREE_TYPE_OFFSET);
TOID_DECLARE(struct tree,BTREE_TYPE_OFFSET+1);
TOID_DECLARE(void,BTREE_TYPE_OFFSET+2);
enum dmark {NORMAL,DELETED};
struct tree_node
{
    key_t keys[BTREE_ORDER];
    PMEMoid pointers[BTREE_ORDER+1];
    int num_keys;
    int is_leaf;
    enum dmark mask;
    size_t value_size[BTREE_ORDER];
    TOID(struct tree_node) parent;
};
struct tree
{
    TOID(struct tree_node) root;
};
typedef TOID(struct tree_node) node_pointer;
node_pointer leftest_leaf;


/*
  *PALM related
  */
#ifndef MAX_LIST_SIZE
#define MAX_LIST_SIZE 100
#endif // MAX_LIST_SIZE


typedef enum OP {INSERT,DELETE,READ,UPSERT,RANGE} optype;
typedef struct {key_t i,j;} key_pair;
typedef struct {key_t key;void * value;size_t value_size;} kv_pair;
/**************input structure declaration**************/
struct operation
{
     node_pointer n;
    int id;
    optype op;
    key_t key; //delete read
    union{
        PMEMoid child; //delete child from parent
        kv_pair interval; //insert upate
       void (*update)(void *);
    }arg;
}operation_lst[MAX_LIST_SIZE];
int operation_lst_count;

struct range_op{
    key_pair interval;
    int read_num;
    int id_from;
};
struct range_op  range_set[MAX_LIST_SIZE];
int range_set_count;

struct new_key_record{
      key_t key;
      int id;
}record_list[MAX_LIST_SIZE];
int record_list_count;
/********************task declartion********************/
struct key_op{
        int id;
        optype op;
        key_t key; //delete/read
        PMEMoid child;
        size_t value_size;
        void * value;
        void (*update)(void *);
};
struct node_opset{
       node_pointer n;
       struct key_op * key_op_set;
       int opcount;
};
struct task{
    struct node_opset *  node_set;
    int nodecount;
};

struct task task_list[MAX_LIST_SIZE];
int task_list_count;


/********************result declaration******************/
struct {
    int id;
    key_t key;
    size_t value_size;
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
            key_t * key;
            void * value;
            int num;
        } range_result;
    }
};
struct op_result  result_list[MAX_LIST_SIZE];
int result_list_count;


#endif // PALM_H_INCLUDED

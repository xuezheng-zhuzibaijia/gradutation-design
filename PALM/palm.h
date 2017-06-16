#ifndef PALM_H_INCLUDED
#define PALM_H_INCLUDED
typedef int key_t;
#include <libpmemobj.h>
#include "avl.h"
POBJ_LAYOUT_BEGIN(test);
POBJ_LAYOUT_ROOT(test,struct tree);
POBJ_LAYOUT_TOID(test,struct tree_node);
POBJ_LAYOUT_TOID(test,void);
POBJ_LAYOUT_TOID(test,struct leaf_arg);
POBJ_LAYOUT_END(test);
typedef TOID(struct tree_node) node_pointer;
struct tree {
    node_pointer root;
};
#define BTREE_ORDER 8
#define INCR(ADDR, INCVAL) __sync_fetch_and_add((ADDR), (INCVAL))
#define CAS(ADDR, OLD, NEW) __bool_sync_compare_and_swap((ADDR),(OLD), (NEW))
struct leaf_arg{
    size_t vsize[BTREE_ORDER];
};
struct tree_node {
    node_pointer parent;
    int num_keys;
    char is_leaf,is_mask;
    key_t keys[BTREE_ORDER];
    PMEMoid pointers[BTREE_ORDER+1];
    TOID(struct leaf_arg) arg;
};
#define MAX_QUERY_SIZE 80
#define MAX_LIST_SIZE (MAX_QUERY_SIZE*MAX_QUERY_SIZE/4)
#define THREAD_NUM 4
enum optype {READ,UPDATE,DELETE,INSERT,RANGE};
typedef struct _singleop {
    enum optype op;
    key_t key;
    int id;
    size_t vsize;
    PMEMoid child;
    struct _singleop * next;
    node_pointer parent;
} singleop;
typedef struct {
    singleop * head,*tail;
} nodeop;
struct _read_result{
    int id;
    key_t key;
    void * value;
    size_t data_size;
};
struct insert_log{
    key_t key;
    int id;
};
struct read_pair{
    key_t key;
    void *v;
    size_t vsize;
    struct read_pair *next;
};
struct origin_op{
    int read_id,range_id,range_num;
    int is_useless;
    enum optype op;
    key_t key;
    void * v; //the pointer must be heap pointer
    size_t vsize;
    key_t m,n;
    struct read_pair * head;
    struct read_pair *tail;
};
struct assign_arg{
    node_pointer root;
    int start_index,step;
};
struct operation_arg{
    node_pointer * root;
    int REBALANCE;
    int start_index;
    int step;
};
struct origin_opavl{
    enum optype op;
    key_t key;
};
struct range_operation_arg{
    int start_index;
    int step;
    avl_pointer avl_root;
    node_pointer leaf;
};
extern int origin_op_count;
extern struct origin_op origin_op_tasks[];
extern int singleop_count,modified_list_count;
extern singleop single_tasks[],modified_list[];
extern nodeop tasks[];
extern int read_results_count;
extern struct _read_result read_results[];
extern singleop *nowtasks,*nexttasks;
extern int *nowtasks_count,*nexttasks_count;
extern int nodeop_count;
extern int free_count;
extern PMEMoid free_records[];
extern int node_count;
extern node_pointer internal_nodes[];
extern int leaf_alloc_count;
extern node_pointer leafs[];
node_pointer make_node();
node_pointer make_leaf();
PMEMoid make_record(void * data,size_t data_size);
void palm_init(node_pointer * root);
void* assigntask(void *arg);
void redistribute_singleop();
void operation(node_pointer *root,nodeop np,int REBALANCE);
void print_btree(node_pointer root);
void circle();
void set_now_single();
void set_now_modified_list();
void palm_process(node_pointer *root);
void preprocess();
void preprocess_originop();
void print_readresult();
#endif // PALM_H_INCLUDED

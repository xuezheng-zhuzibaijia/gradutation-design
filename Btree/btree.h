/*
 *btree.h -- Tree sorted collection implementation
 */
#ifndef BTREE_H_INCLUDED
#define BTREE_H_INCLUDED
#include <libpmemobj.h>  //compile command would be 'cc -std=gnu99　... -lpmemobj -lpmem'
#ifndef BTREE_TYPE_OFFSET
#define BTREE_TYPE_OFFSET 1012
#endif
#define BTREE_ORDER 8
#define BTREE_MIN (BTREE_ORDER / 2)
#define TRUE  1
#define FALSE 0
TOID_DECLARE(struct tree_node,BTREE_TYPE_OFFSET);
TOID_DECLARE(struct tree,BTREE_TYPE_OFFSET+1);
TOID_DECLARE(void,BTREE_TYPE_OFFSET+2);
/*
 *tree_node--struct of B+tree interval node or leaf
 *pointers[i] point to the ist child of node if is_leaf is 'TRUE'
 *else,point to the value which key is the keys[i]
 */
struct tree_node{
    int keys[BTREE_ORDER];
    PMEMoid pointers[BTREE_ORDER+1];
    int num_keys;
    int is_leaf;
    TOID(struct tree_node) parent;
};
struct tree{
    TOID(struct tree_node) root;
};
typedef TOID(struct tree_node) node_pointer;
void tree_init(PMEMobjpool*pop);
PMEMoid find_value(PMEMobjpool *pop,int key);
//Output
PMEMoid make_record(void *value,size_t len);
void tree_insert(char *popname,int key,void *value,size_t len);
void tree_delete(PMEMobjpool *pop,int key);
void destroy_tree(PMEMobjpool *pop);
void display(char *popname);
//Insertion
node_pointer find_leaf(node_pointer root,int key);
node_pointer make_node();
node_pointer make_leaf();
int get_left_index(node_pointer parent,node_pointer left);
node_pointer insert_into_leaf(node_pointer leaf,int key,PMEMoid value);
node_pointer insert_into_leaf_after_splitting(node_pointer root,node_pointer leaf,int key,PMEMoid value);
node_pointer insert_into_node(node_pointer root,node_pointer n,int key,node_pointer child);
node_pointer insert_into_node_after_splitting(node_pointer root,node_pointer n,int key,node_pointer child);
node_pointer insert_into_parent(node_pointer root,node_pointer left,int key,node_pointer right);
node_pointer insert_into_new_root(node_pointer left,int key,node_pointer right);
node_pointer start_new_tree(int key,PMEMoid value);

//Deletion
int get_neighbor_index(node_pointer n);
node_pointer adjust_root(node_pointer root);
node_pointer remove_entry_from_node(node_pointer n,int key);
node_pointer coalesce_nodes(node_pointer root,node_pointer n,node_pointer neighbor,int neighbor_index,int k_prime);
node_pointer redistribute_nodes(node_pointer root,node_pointer n,node_pointer neighbor,int neighbor_index,int k_prime,int k_prime_index);
node_pointer delete_entry(node_pointer root,node_pointer n,int key);
#endif // BTREE_H_INCLUDED

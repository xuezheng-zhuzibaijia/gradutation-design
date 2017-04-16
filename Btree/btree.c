#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpmemobj.h>
#include "btree.h"
//Output
node_pointer find_leaf(node_pointer root,uint64_t key){
   node_pointer c = root;
   if(TOID_IS_NULL(c)){
       return c;
   }while(!D_RO(c)->is_leaf){
        int i = 0;
        while(i < D_RO(c)->num_keys){
            if(key >= )
        }
   }
}
//Look up
static int binary_search_exact(){
   return 0;
}
PMEMoid find(node_pointer root,uint64_t key);
//Insertion
PMEMoid make_node();
PMEMoid make_leaf();
int get_left_index(node_pointer parent,node_pointer left);
node_pointer insert_into_leaf(node_pointer leaf,int key,PMEMoid value);
node_pointer insert_into_leaf_after_splitting(node_pointer root,node_pointer parent,int left_index,node_pointer right);
node_pointer insert_into_node(node_pointer root,node_pointer parent,int left_index,int key,node_pointer right);
node_pointer insert_into_node_after_splitting(node_pointer root,node_pointer parent,int left,int key,node_pointer right);
node_pointer insert_into_parent(node_pointer root,node_pointer left,int key,node_pointer right);
node_pointer insert_into_new_root(node_pointer left,int key,node_pointer right);
node_pointer start_new_tree(int key,PMEMoid value);
node_pointer tree_insert(node_pointer root,int key,PMEMoid value);
//Deletion
int get_neighbor_index(node_pointer n);
node_pointer adjust_root(node_pointer root);
node_pointer coalesce_nodes(node_pointer root,node_pointer n,node_pointer neighbor,int neighbor_index,int k_prime);
node_pointer redistribute_nodes(node_pointer root,node_pointer n,node_pointer neighbot,int neighbor_index,int k_prime,int k_prime_index);
node_pointer delete_entry(node_pointer root,node_pointer n,int key);
node_pointer tree_delete(node_pointer root,int key);

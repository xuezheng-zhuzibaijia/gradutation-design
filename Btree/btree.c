#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpmemobj.h>
#include "btree.h"
/*
 *binary_search_exact--(interval)return index of key in the keys,if not exists,return -1
 */
static int binary_search_exact(node_pointer n,int key){
    int left = 0,right = D_RO(n)->num_keys - 1;
    while(left<=right)
    {
        int mid = (left+right)/2;
        if(D_RO(n)->keys[mid]==key)
        {
            return mid;
        }
        if(D_RO(n)->keys[mid]>key)
        {
            right = mid-1;
        }
        else
        {
            left = mid+1;
        }
    }return -1;
}
/*
 *binary_search--(interval) return the index of pointers,which may include the key 
 */
static int binary_search(node_pointer n,int key){
    
    int left = 0,right = D_RO(n)->num_keys - 1;
    while(left <= right){
        int mid = (left + right)/2;
        if(D_RO(n)->num_keys[mid] > key){
            right =  mid - 1;
        }else{
            left = mid + 1;
        }
    }return left;
}
//Output
/*
 *find_leaf--return the leaf which may contain the key
 */
node_pointer find_leaf(node_pointer root,int key){
   node_pointer c = root;
   if(TOID_IS_NULL(c)){
       return c;
   }while(!D_RO(c)->is_leaf){
       int index =  binary_search(c,key);
       TOID_ASSIGN(c,D_RO(c)->pointers[index]);
   }return c;
}
/*
 *find--return the value of key if exists,else return OID_NULL
 */
PMEMoid find(node_pointer root,int key){
    node_pointer leaf = find_leaf(root,key);
    int index = binary_search_exact(leaf,key);
    return (index==-1)?OID_NULL:leaf->pointers[index];
}
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

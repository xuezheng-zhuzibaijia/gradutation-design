#include <stdio.h>
#include "btree.h"
TOID_DECLARE(struct tree_node,BTREE_TYPE_OFFSET+1);
#define BTREE_ORDER 8 /* can't be odd */
#define BTREE_MIN ((BTREE_ORDER / 2) - 1) /* min number of keys per node */
struct tree_node{
   int n; /* number of occupied keys*/
   int is_leaf;
   uint64_t keys[BTREE_ORDER];
   PMEMoid  values[BTREE_ORDER];
   TOID(struct tree_node)*  slots; 
  /*
   *leaf can use the slots[BTREE_ORDER].value to point the right neighbour 
   *except it's the rightest one then it refers to NULL;
   */
};
struct btree{
    TOID(struct tree_node) root;
};


/*
 *bsearch_in_node -- (internal) get the index of the key if exists
 */
int bsearch_in_node(TOID(struct tree_node) node,uint64_t key){
    if(D_R0(node)->keys[0]>key){         
        return -1;
    }
    if(D_RO(node)->keys[D_RO(node)->n-1]<=key){
        return D_RO(node)->n - 1;
    }
    int left = 0,right = D_RO(node)->n-1;
    while(left <= right){
        int mid = (right+left)/2;
        if(D_RO(node)->keys[mid]==key){
            return mid;
        }else{
            if(D_RO(node)->keys[mid]<key){
                left = mid + 1;
            }else{
                right = mid - 1;
            }
        }
    }return mid;
}



/*
 *btree_get_in_node --(internal) search for a value of the key in a node
 */
static PMEMoid btree_get_in_node(TOID(struct tree_node) node,uint64_t key){
    int index = bsearch_in_node(node,key);
    if(index==-1){
        return OID_NULL;
    }if(D_RO(node)->is_leaf){
        return (D_RO(node)->keys[index]==key)?D_RO(node)->values[index]:OID_NULL;
    }return (D_RO(node)->slots[index]==OID_NULL)?OID_NULL:btree_get_in_node(D_RO(node)->slots[index],key);
}

/*
 *btree_get --search for a value of the key in a tree;
 */
PMEMoid btree_get(TOID(struct btree) tree,uint64_t key){
   return (TOID_IS_NULL(D_RO(tree)->root))?OID_NULL:btree_get_in_node(D_RO(tree)->root);
}

/*
 *btree_lookup_in_node --(internal) search for key in node if exists
 */
static int btree_lookup_in_node(TOID(struct tree_node) node,uint64_t key){
    int index = bsearch_in_node(node,key);
    if(index==-1){
        return 0;
    }if(D_RO(node)->is_leaf){
        return (D_RO(node)->keys[index]==key)?1:0;
    }return (D_RO(node)->slots[index]==OID_NULL)?0:btree_get_in_node(D_RO(node)->slots[index],key);
}

/*
 *btree_lookup -- search for key if exists
 */
int btree_lookup(PMEMobjpool *pop, TOID(struct btree) tree,
		uint64_t key){
    return (TOID_IS_NULL(D_RO(tree)->root))?0:btree_get_in_node(D_RO(tree)->root);
}

int btree_create(PMEMobjpool *pop, TOID(struct btree) * tree, void *arg);
int btree_destroy(PMEMobjpool *pop, TOID(struct btree) * tree);
int btree_insert(PMEMobjpool *pop, TOID(struct btree) tree,
    uint64_t key, PMEMoid value);
int btree_insert_new(PMEMobjpool *pop, TOID(struct btree) tree,
    uint64_t key, size_t size, unsigned type_num,
    void (*constructor)(PMEMobjpool *pop, void *ptr, void *arg),
		void *arg);
PMEMoid btree_remove(PMEMobjpool *pop, TOID(struct btree) tree,
		uint64_t key);
int btree_remove_free(PMEMobjpool *pop, TOID(struct btree) tree,
		uint64_t key);
int btree_clear(PMEMobjpool *pop, TOID(struct btree) tree);
int btree_foreach(PMEMobjpool *pop, TOID(struct btree) tree,
	int (*cb)(uint64_t key, PMEMoid value, void *arg), void *arg);
int btree_is_empty(PMEMobjpool *pop, TOID(struct btree) tree);
int btree_check(PMEMobjpool *pop, TOID(struct btree) tree);

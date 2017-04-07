#include <stdio.h>
#include "btree.h"
TOID_DECLARE(struct tree_node,BTREE_TYPE_OFFSET+1);
#define BTREE_ORDER 8 /* can't be odd */
#define BTREE_MIN ((BTREE_ORDER / 2) - 1) /* min number of keys per node */
struct tree_node_item{
    uint_64_t key;
    PMEMoid value;
};

struct tree_node{
   int n; /* number of occupied slots*/
   struct tree_node_item[BTREE_ORDER];  
   TOID(struct tree_node) slots[BTREE_ORDER+1];
};
struct tree{
    TOID(struct tree_node) root;
};
int btree_check(PMEMobjpool *pop, TOID(struct btree) tree);
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
PMEMoid btree_get(PMEMobjpool *pop, TOID(struct btree) tree,
		uint64_t key);
int btree_lookup(PMEMobjpool *pop, TOID(struct btree) tree,
		uint64_t key);
int btree_foreach(PMEMobjpool *pop, TOID(struct btree) tree,
	int (*cb)(uint64_t key, PMEMoid value, void *arg), void *arg);
int btree_is_empty(PMEMobjpool *pop, TOID(struct btree) tree);

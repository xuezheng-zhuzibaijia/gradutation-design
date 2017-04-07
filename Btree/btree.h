/*
 *btree.h -- Tree sorted collection implementation
 */
#ifndef BTREE_H_INCLUDED
#define BTREE_H_INCLUDED
#include <libmemobj.h>  //compile command would be 'cc -std=gnu99　... -lpmemobj -lpmem'
#ifndef BTREE_TYPE_OFFSET
#define BTREE_TYPE_OFFSET 1012
#endif
struct btree;
TOID_DECLARE(struct btree, BTREE_TYPE_OFFSET + 0);
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
PMEMoid btree_get(TOID(struct btree) tree,
		uint64_t key);
int btree_lookup(PMEMobjpool *pop, TOID(struct btree) tree,
		uint64_t key);
int btree_foreach(PMEMobjpool *pop, TOID(struct btree) tree,
	int (*cb)(uint64_t key, PMEMoid value, void *arg), void *arg);
int btree_is_empty(PMEMobjpool *pop, TOID(struct btree) tree);
#endif // BTREE_H_INCLUDED

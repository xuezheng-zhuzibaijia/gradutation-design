#ifndef SKIP_LIST_H_INCLUDED
#define SKIP_LIST_H_INCLUDED

#include <time.h>
#include <libpmemobj.h>

#ifndef SKIP_LIST_TYPE_OFFSET
#define SKIP_LIST_TYPE_OFFSET 1050
#endif

#define SKIP_MAX_HEIGHT 20

TOID_DECLARE(struct snode,SKIP_LIST_TYPE_OFFSET);
TOID_DECLARE(struct sroot,SKIP_LIST_TYPE_OFFSET+1);
TOID_DECLARE(void,SKIP_LIST_TYPE_OFFSET+2);
struct snode{
    TOID(struct snode) next[SKIP_MAX_HEIGHT];
    int key;
    int level;
    PMEMoid value;
};
struct sroot{
    struct snode head;
};
typedef TOID(struct snode) snode_pointer;
typedef TOID(struct sroot) sroot_pointer;
void skiplist_init(PMEMobjpool *pop);
int skiplist_is_in(PMEMobjpool *pop,int key);
PMEMoid skiplist_find_value(PMEMobjpool *pop,int key);
void skiplist_remove(PMEMobjpool *pop,int key);
void skiplist_insert(PMEMobjpool *pop,int key,void *value,size_t len);
void skiplist_destroy(PMEMobjpool *pop);
void printf_skiplist(PMEMobjpool *pop);
void display(PMEMobjpool *pop);
#endif // SKIP_LIST_H_INCLUDED

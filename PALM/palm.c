#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "palm.h"
#define DEBUG(s,...) //printf(s,##__VA_ARGS__)
int singleop_count;
int nodeop_count;
int modified_list_count;
singleop single_tasks[MAX_LIST_SIZE];
singleop modified_list[MAX_LIST_SIZE];
nodeop tasks[MAX_LIST_SIZE];
singleop *nowtasks,*nexttasks;
int *nowtasks_count,*nexttasks_count;
struct _read_result read_results[MAX_LIST_SIZE];
int read_results_count;
int DELETE_FLAG = FALSE;
int insert_results_count;
struct insert_log insert_results[MAX_LIST_SIZE];

int free_count;
PMEMoid free_records[MAX_LIST_SIZE*4];
int node_alloc_count;
node_pointer internal_nodes[MAX_QUERY_SIZE*4];
int leaf_alloc_count;
node_pointer leafs[MAX_QUERY_SIZE];
int node_alloc_max;
int leaf_alloc_max;

int origin_op_count;
struct origin_op origin_op_tasks[MAX_QUERY_SIZE];
node_pointer make_node()
{
    node_pointer t = TX_ZNEW(struct tree_node);
    D_RW(t)->is_leaf = FALSE;
    D_RW(t)->is_mask = FALSE;
    D_RW(t)->num_keys = 0;
    D_RW(t)->parent = TOID_NULL(struct tree_node);
    for(int i = 0; i < BTREE_ORDER + 1; i++) {
        D_RW(t)->pointers[i] = OID_NULL;
    }
    D_RW(t)->arg = TOID_NULL(struct leaf_arg);
    return t;
}
node_pointer make_leaf()
{
    node_pointer t = make_node();
    D_RW(t)->arg = TX_NEW(struct leaf_arg);
    D_RW(t)->is_leaf = TRUE;
    return t;
}
PMEMoid make_record(void *data,size_t data_size)
{
    PMEMoid t = pmemobj_tx_alloc(data_size,TOID_TYPE_NUM(void));
    memcpy(pmemobj_direct(t),data,data_size);
    free(data);
    return t;
}
node_pointer alloc_leaf()
{
    int index = INCR(&leaf_alloc_count,1);
    if(index < leaf_alloc_max) {
        return leafs[index];
    }
    perror("alloc leaf is too few");
    exit(-1);
}
node_pointer alloc_node()
{
    int index = INCR(&node_alloc_count,1);
    if(index < node_alloc_max) {
        return internal_nodes[index];
    }
    perror("alloc node is too few");
    exit(-1);
}
void psefree(PMEMoid t)
{
    int index = INCR(&free_count,1);
    if(index < MAX_LIST_SIZE * 4) {
        free_records[index] = t;
    } else {
        perror("psefree too few");
        exit(-1);
    }
}
void * get_record(PMEMoid value,size_t data_size)
{
    void * result;
    if((result=(void *)malloc(data_size))==NULL) {
        perror("get record malloc failed");
        exit(-1);
    }
    memcpy(result,pmemobj_direct(value),data_size);
    return result;
}
void palm_init(node_pointer *root)
{
    read_results_count = 0;
    if(TOID_IS_NULL(*root)) {
        *root = make_leaf();
    }
    insert_results_count = 0;
    DELETE_FLAG = FALSE;
    for(int i = 0; i < leaf_alloc_max; i++) {
        leafs[i] = make_leaf();
    }
    for(int i = 0; i < node_alloc_max; i++) {
        internal_nodes[i] = make_node();
    }
    leaf_alloc_count = 0;
    node_alloc_count = 0;
}
static int compare_key_to_key(void * a,void * b)
{
    //DEBUG("\ncmp < %d %d>\n",*(int *)a,*(int *)b);
    if(*((key_t *)a) == *((key_t*)b)) return 0;
    if(*((key_t *)a) < *((key_t *)b)) return -1;
    return 1;
}
static int search_exact(void * keys,size_t key_size,int length,void *key,cmp cp)
{
    int left = 0,right = length - 1;
    while(left <= right) {
        int medium = (left+right)/2;
        switch(cp(keys+medium*key_size,key)) {
        case 0:
            return medium;
        case -1:
            left = medium + 1;
            break;
        case 1:
            right = medium - 1;
            break;
        default:
            break;
        }
    }
    return -1;
}
static int search_fuzzy(void * keys,size_t key_size,int length,void *key,cmp cp)
{
    int left = 0,right = length - 1;
    while(left <= right) {
        int medium = (left+right)/2;
        switch(cp(keys+medium*key_size,key)) {
        case 0:
            left = medium + 1;
            break;
        case -1:
            left = medium + 1;
            break;
        case 1:
            right = medium - 1;
            break;
        default:
            break;
        }
    }
    return left;
}
static node_pointer get_target(node_pointer root,key_t key)
{
    if(TOID_IS_NULL(root)) return root;
    node_pointer c = root;
    while(!D_RO(c)->is_leaf) {
        TOID_ASSIGN(c,D_RO(c)->pointers[search_fuzzy((void *)D_RW(c)->keys,sizeof(key_t),D_RW(c)->num_keys,&key,compare_key_to_key)]);
    }
    return c;
}
struct assign_avl {
    int nodeop_index;
    node_pointer p;
};
static int compare_assignavl_singleop(void *a,void *b)
{
    PMEMoid t = ((struct assign_avl *)a)->p.oid,m = ((singleop *)b)->parent.oid;
    if(t.pool_uuid_lo==m.pool_uuid_lo && t.off == m.off) return 0;
    if(t.pool_uuid_lo > m.pool_uuid_lo || (t.pool_uuid_lo==m.pool_uuid_lo && t.off > m.off)) return 1;
    return -1;
}
static void * construct_assignavl(void *data,size_t data_size)
{
    struct assign_avl * t;
    singleop * m = (singleop *)data;
    if((t=(struct assign_avl *)malloc(sizeof(struct assign_avl)))==NULL) {
        perror("construct assignavl malloc failed");
        exit(-1);
    }
    t->nodeop_index = nodeop_count;
    nodeop_count++;
    t->p = m->parent;
    m->next = NULL;
    tasks[t->nodeop_index].head = tasks[t->nodeop_index].tail = m;
    return t;
}
static void * update_assignavl(void *data,void *newdata)
{
    struct assign_avl * t = (struct assign_avl *)data;
    singleop * m = (singleop *)newdata;
    tasks[t->nodeop_index].tail->next = m;
    tasks[t->nodeop_index].tail = m;
    m->next = NULL;
    return t;
}

void* assigntask(void *arg)
{
    struct assign_arg * t =(struct assign_arg *)arg;
    node_pointer root = t->root;
    int start_index = t->start_index;
    int step = t->step;
    for(int i = start_index; i < *nowtasks_count; i+=step) {
        nowtasks[i].parent = get_target(root,nowtasks[i].key);
    }
    return NULL;
}
void preadd()
{
    for(int i = 0; i < nodeop_count; i++) {
        if(!TOID_IS_NULL(tasks[i].head->parent)) {
            TX_ADD(tasks[i].head->parent);
        }
    }
}
void redistribute_singleop()
{
    DEBUG("now has %d operations\n",*nowtasks_count);
    nodeop_count = 0;
    avl_pointer avl_root = NULL;
    int unbalanced = FALSE;
    for(int i = 0; i < *nowtasks_count; i++) {
        avl_insert(&avl_root,&unbalanced,&nowtasks[i],sizeof(nowtasks[i]),compare_assignavl_singleop,construct_assignavl,update_assignavl);
    }
    avl_destory(&avl_root,free);
    preadd();
}
void btree_insert(key_t * keys,PMEMoid * values,size_t * vsize,int *length,singleop op,int REBALANCE)
{
    //DEBUG("in btree insert\n");
    if(search_exact((void *)keys,sizeof(key_t),*length,&(op.key),compare_key_to_key)!=-1) {
        if(!REBALANCE) {
            psefree(op.child);
        }
        return;
    }
    int index = search_fuzzy((void *)keys,sizeof(key_t),*length,&(op.key),compare_key_to_key);
    //DEBUG("insert %d index %d\n length = %d\n",op.key,index,*length);
    if(D_RO(op.parent)->is_leaf) { //node is leaf
        for(int i = *length - 1; i >= index; --i) {
            keys[i+1] = keys[i];
            values[i+1] = values[i];
            vsize[i+1] = vsize[i];
        }
        keys[index] = op.key;
        values[index] = op.child;
        vsize[index] = op.vsize;
        (*length)++;
        if(!REBALANCE) {
            int insert_results_index = INCR(&insert_results_count,1);
            insert_results[insert_results_index].key = op.key;
            insert_results[insert_results_index].id = op.id;
        }
    } else { //internal node
        for(int i = *length - 1; i >= index; --i) {
            keys[i+1] = keys[i];
            values[i+2] = values[i+1];
        }
        keys[index] = op.key;
        values[index+1] = op.child;
        (*length)++;
    }
    /*DEBUG("after insert %d keys :",op.key);
    for(int i = 0; i < *length; i++) {
        DEBUG("%d ",keys[i]);
    }*/
}

void btree_update(key_t * keys,PMEMoid * values,size_t * vsize,int length,singleop op)
{
    if(!D_RO(op.parent)->is_leaf) return; //internal node has no update operation
    int index = search_exact((void *)keys,sizeof(key_t),length,&(op.key),compare_key_to_key);
    if(index==-1) {
        psefree(op.child);
        return;
    }
    psefree(values[index]);
    vsize[index] = op.vsize;
    values[index] = op.child;
}
void  btree_read(key_t * keys,PMEMoid * values,size_t * vsize,int length,singleop op)
{
    //DEBUG("in btreeread\n");
    void * result = NULL;
    int index = search_exact((void *)keys,sizeof(key_t),length,&(op.key),compare_key_to_key);
    if(index!=-1) {
        result = get_record(values[index],vsize[index]);
    }
    int read_results_index = INCR(&read_results_count,1);
    read_results[read_results_index].key = keys[index];
    read_results[read_results_index].value = result;
    read_results[read_results_index].data_size = vsize[index];
    read_results[read_results_index].id = op.id;
}
void btree_del(key_t * keys,PMEMoid * values,size_t * vsize,int *length,singleop op)
{
    DEBUG("thread %lu btree_del %d \n",pthread_self(),op.key);
    if(D_RO(op.parent)->is_leaf) {
        int index = search_exact((void *)keys,sizeof(key_t),*length,&(op.key),compare_key_to_key);
        if(index==-1) return; //not found
        psefree(values[index]);
        for(int i = index; i < *length - 1; i++) {
            keys[i] = keys[i+1];
            values[i] = values[i+1];
            vsize[i] = vsize[i+1];
        }(*length)--;
    } else {
        DEBUG("btree node\n");
        int index = -1;
        for(int i = 0; i < (*length+1); i++) {
            if(OID_EQUALS(op.child,values[i])) {
                index = i;
                break;
            }
        }
        if(index == -1) return; //not found
        for(int i = index; i < *length - 1; i++) {
            keys[i] = keys[i+1];
            values[i] = values[i+1];
        }
        if(*length > 0) {
            values[*length - 1] = values[*length];
        }(*length)--;
    }
    DEBUG("after btree del %d\n",op.key);
    fflush(stdout);
}
void destroy_subtree(node_pointer p)
{
    if(!D_RO(p)->is_leaf) {
        node_pointer c;
        for(int i = 0; i < D_RO(p)->num_keys+1; i++) {
            TOID_ASSIGN(c,D_RO(p)->pointers[i]);
            destroy_subtree(c);
        }
        psefree(p.oid);
    } else {
        D_RW(p)->is_mask=TRUE;
    }
}
node_pointer node_split(node_pointer parent,key_t key,key_t * keys,PMEMoid * values,size_t *vsize,int length,int is_leaf,node_pointer pre)
{
    node_pointer temp = (TOID_IS_NULL(pre))?alloc_node():alloc_leaf();
    int modified_index = INCR(nexttasks_count,1);
    nexttasks[modified_index].child = temp.oid;
    nexttasks[modified_index].parent = parent;
    nexttasks[modified_index].op = INSERT;
    nexttasks[modified_index].key = key;
    nexttasks[modified_index].next = NULL;
    D_RW(temp)->num_keys = length;
    memcpy(D_RW(temp)->keys,keys,sizeof(key_t)*(length));
    memcpy(D_RW(temp)->pointers,values,sizeof(PMEMoid)*(length+1-is_leaf));
    if(is_leaf) {
        memcpy(D_RW(D_RW(temp)->arg)->vsize,vsize,sizeof(size_t)*length);
        if(!TOID_IS_NULL(pre)) {
            D_RW(pre)->pointers[BTREE_ORDER] = temp.oid;
        }
    } else {
        for(int i = 0; i < length + 1; i++) {
            node_pointer c;
            TOID_ASSIGN(c,values[i]);
            D_RW(c)->parent = temp;
        }
    }
    return temp;
}
void print_singleop()
{
    for(int i = 0; i < *nexttasks_count; i++) {
        switch(nexttasks[i].op) {
        case READ:
            DEBUG("r ");
            break;
        case UPDATE:
            DEBUG("u ");
            break;
        case DELETE:
            DEBUG("d ");
            break;
        case INSERT:
            DEBUG("i ");
            break;
        default:
            break;
        }
        DEBUG("%d ",nexttasks[i].key);
    }
    DEBUG("\n");
}

void node_process(node_pointer *root,node_pointer n,key_t * keys,PMEMoid * values,size_t * vsize,int length)
{
    DEBUG("in node_process length %d thread %lu\n",length,pthread_self());
    int is_leaf = D_RO(n)->is_leaf;
    if(length <= BTREE_ORDER) {
        D_RW(n)->num_keys=length;//TX_SET(n,num_keys,length);
        memcpy(D_RW(n)->keys,keys,sizeof(key_t)*((length>0)?length:0));
        memcpy(D_RW(n)->pointers,values,sizeof(PMEMoid)*(length+1-is_leaf));
        if(!is_leaf) {
            for(int i = 0; i < length+1; i++) {
                node_pointer c;
                TOID_ASSIGN(c,values[i]);
                D_RW(c)->parent = n;
            }
        } else {
            memcpy(D_RW(D_RW(n)->arg)->vsize,vsize,sizeof(size_t)*length);
        }
        if(length < BTREE_ORDER / 2) { //the node has to be deleted
            //DEBUG("deleting\n");
            if(TOID_EQUALS(*root,n)) {
                if((!D_RO(n)->is_leaf)) {
                    if(D_RO(n)->num_keys==0) {
                        DEBUG("root is node\n");
                        TOID_ASSIGN(*root,D_RO(n)->pointers[0]);
                        D_RW(*root)->parent=TOID_NULL(struct tree_node);
                        psefree(n.oid);
                    } else if(D_RO(n)->num_keys < 0) {
                        DEBUG("root is node and it doesn't have any child\n");
                        psefree(n.oid);
                        *root = alloc_leaf();
                    }
                }
            } else {
                if(!DELETE_FLAG) {
                    DELETE_FLAG = TRUE;
                }
                DEBUG("set delete flag\n");
                int modified_index = INCR(nexttasks_count,1);
                DEBUG("set delete flag max_list_size %d modified_index %d\n",MAX_LIST_SIZE,modified_index);
                assert(nexttasks!=NULL);
                nexttasks[modified_index].child = n.oid;
                nexttasks[modified_index].parent = D_RO(n)->parent;
                nexttasks[modified_index].op = DELETE;
                nexttasks[modified_index].next = NULL;
                //assert(!TOID_IS_NULL(D_RO(n)->parent));
                DEBUG("is_leaf %d-------------------\n",is_leaf);
                if(is_leaf) {
                    DEBUG("mask node\n");
                    D_RW(n)->is_mask = TRUE;

                } else {
                    destroy_subtree(n);
                }
            }
        }DEBUG("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    } else {
        //DEBUG("splitting\n");
        D_RW(n)->num_keys = BTREE_ORDER/2;
        memcpy(D_RW(n)->keys,keys,sizeof(key_t)*(BTREE_ORDER/2));
        memcpy(D_RW(n)->pointers,values,sizeof(PMEMoid)*(BTREE_ORDER/2+1-is_leaf));
        if(is_leaf) {
            memcpy(D_RW(D_RW(n)->arg)->vsize,vsize,sizeof(size_t)*(BTREE_ORDER/2));
        } else {
            for(int i = 0; i < BTREE_ORDER/2+1; i++) {
                node_pointer c;
                TOID_ASSIGN(c,values[i]);
                D_RW(c)->parent=n;
            }
        }
        if(TOID_IS_NULL(D_RO(n)->parent)) {
            *root = alloc_node();
            D_RW(n)->parent = *root;
            D_RW(*root)->pointers[0]=n.oid;
        }
        int base = BTREE_ORDER/2;
        node_pointer pre = (is_leaf)?n:TOID_NULL(struct tree_node);
        PMEMoid tail = (is_leaf)?D_RO(n)->pointers[BTREE_ORDER]:OID_NULL;
        while(length-base > BTREE_ORDER+1-is_leaf) {
            node_pointer temp = node_split(D_RO(n)->parent,keys[base],keys+base+1-is_leaf,values+base+1-is_leaf,(is_leaf)?(vsize+base):NULL,BTREE_ORDER/2,is_leaf,pre);
            if(is_leaf) {
                pre = temp;
            }
            base += (BTREE_ORDER/2 + 1-is_leaf);
        }
        //DEBUG("rest length %d\n",length - base - 1 + is_leaf);
        node_pointer t = node_split(D_RO(n)->parent,keys[base],keys+base+1-is_leaf,values+base+1-is_leaf,(is_leaf)?(vsize+base):NULL,length-base-1+is_leaf,is_leaf,pre);
        if(is_leaf) {
            D_RW(t)->pointers[BTREE_ORDER] = tail;
        }
    }
    DEBUG("end node process\n");
}
void clear_nodeop()
{
    for(int i = 0; i < nodeop_count; i++) {
        tasks[i].head = tasks[i].tail = NULL;
    }
    nodeop_count = 0;
}

void operation(node_pointer *root,nodeop np,int REBALANCE)
{
    node_pointer p = np.head->parent;
    //struct tree_node * t = D_RW(p);
    key_t keys[MAX_LIST_SIZE+BTREE_ORDER];
    PMEMoid values[MAX_LIST_SIZE+BTREE_ORDER+1];
    size_t vsize[MAX_LIST_SIZE+BTREE_ORDER];
    int length = D_RO(p)->num_keys;
    memcpy(keys,D_RO(p)->keys,sizeof(key_t)*length);
    if(D_RO(p)->is_leaf) {
        memcpy(values,D_RO(p)->pointers,sizeof(PMEMoid)*length);
        memcpy(vsize,D_RO(D_RO(p)->arg)->vsize,sizeof(size_t)*length);
    } else {
        memcpy(values,D_RO(p)->pointers,sizeof(PMEMoid)*(length+1));
    }
    if(TOID_EQUALS(*root,p)){
        DEBUG("this one is root ");
    }else{
        DEBUG("ordinary node ");
    }if(D_RO(p)->is_leaf){
        DEBUG("[leaf]");
    }else{
        DEBUG("[node]");
    }
    DEBUG("thread %lu length %d |",pthread_self(),length);
    for(int i = 0;i < length;i++){
        DEBUG("%d ",keys[i]);
    }DEBUG("\n");
    for(singleop * tmp = np.head; tmp!=NULL; tmp=tmp->next) {
        tmp->parent = p;
        switch(tmp->op) {
        case READ:
            //DEBUG("\noperation: r %d \n",tmp->key);
            btree_read(keys,values,vsize,length,*tmp);
            break;
        case INSERT:
            //DEBUG("\noperation: i %d \n",tmp->key);
            btree_insert(keys,values,vsize,&length,*tmp,REBALANCE);
            break;
        case DELETE:
            //DEBUG("\noperation: d %d \n",tmp->key);
            btree_del(keys,values,vsize,&length,*tmp);
            break;
        case UPDATE:
            //DEBUG("\noperation: u %d \n",tmp->key);
            btree_update(keys,values,vsize,length,*tmp);
            break;
        default:
            break;
        }
    }
    node_process(root,p,keys,values,vsize,length);
}

static void print_node(node_pointer n)
{
    if(TOID_IS_NULL(n)) return;
    if(D_RO(n)->is_leaf) {
        DEBUG("[ keys = %d |",D_RO(n)->num_keys);
        for(int i = 0; i < D_RO(n)->num_keys; i++) {
            DEBUG("%d ",D_RO(n)->keys[i]);
        }
        DEBUG("] ");
        return;
    }else{
        DEBUG("[ children = %d |",D_RO(n)->num_keys);
        for(int i = 0; i < D_RO(n)->num_keys; i++) {
            DEBUG("%d ",D_RO(n)->keys[i]);
        }
        DEBUG("] ");
    }
    for(int i = 0; i < D_RO(n)->num_keys+1; i++) {
        node_pointer c;
        TOID_ASSIGN(c,D_RO(n)->pointers[i]);
        print_node(c);
    }
}
void print_btree(node_pointer root)
{
    if(TOID_IS_NULL(root)) {
        DEBUG("btree is empty!\n");
    } else {
        print_node(root);
    }
}
void circle()
{
    int * temp1;
    singleop * temp2;

    temp1 = nexttasks_count;
    nexttasks_count = nowtasks_count;
    nowtasks_count = temp1;

    temp2 = nowtasks;
    nowtasks = nexttasks;
    nexttasks = temp2;

    *nexttasks_count = 0;
}
node_pointer get_leftest_leaf(node_pointer root)
{
    node_pointer c = root;
    while(!D_RO(c)->is_leaf) {
        TOID_ASSIGN(c,D_RO(c)->pointers[0]);
    }
    return c;
}
void set_now_single()
{
    singleop_count = 0;
    modified_list_count = 0;
    nowtasks = single_tasks;
    nowtasks_count = &singleop_count;
    nexttasks = modified_list;
    nexttasks_count = &modified_list_count;
}
void set_now_modified_list()
{
    singleop_count = 0;
    modified_list_count = 0;
    nowtasks = modified_list;
    nowtasks_count = &modified_list_count;
    nexttasks = single_tasks;
    nexttasks_count = &singleop_count;
}

void clear_mess(node_pointer leaf)
{
    set_now_single();
    node_pointer t = leaf;
    node_pointer pre = TOID_NULL(struct tree_node);
    while(!TOID_IS_NULL(t)) {
        if(D_RO(t)->is_mask) {
            //DEBUG("masked\n");
            int nowtasks_index = INCR(nowtasks_count,D_RO(t)->num_keys);
            for(int i = nowtasks_index,j = 0; j < D_RO(t)->num_keys; i++,j++) {
                nowtasks[i].op = INSERT;
                nowtasks[i].key = D_RO(t)->keys[j];
                nowtasks[i].child = D_RO(t)->pointers[j];
                nowtasks[i].next = NULL;
            }
            node_pointer m = t;
            TOID_ASSIGN(t,D_RO(t)->pointers[BTREE_ORDER]);
            psefree(D_RO(m)->arg.oid);
            psefree(m.oid);
        } else {
            if(!TOID_IS_NULL(pre)) {
                TX_SET(pre,pointers[BTREE_ORDER],t.oid);
            }
            pre = t;
            TOID_ASSIGN(t,D_RO(t)->pointers[BTREE_ORDER]);
        }

    }
    if(!TOID_IS_NULL(pre)) {
        TX_SET(pre,pointers[BTREE_ORDER],OID_NULL);
    }
}

void avl_print(avl_pointer p,key_t m,key_t n,int *range_num)
{
    if(!p) return;
    if(*((key_t*)p->data) <= n) {
        avl_print(p->lchild,m,n,range_num);
    }
    if(*((key_t*)p->data) >= m && *((key_t*)p->data) <= n) {
        single_tasks[singleop_count].op=READ;
        single_tasks[singleop_count].key = *((key_t*)p->data);
        single_tasks[singleop_count].id = singleop_count;
        singleop_count++;
        (*range_num)++;
    }
    if(*((key_t*)p->data) >= m) {
        avl_print(p->rchild,m,n,range_num);
    }
}
void * construct_do_nothing(void *data,size_t data_size)
{
    return data;
}
void *update_preprocess(void *data,void *newdata)
{
    return data;
}
void preprocess()
{
    preprocess_originop();
    avl_pointer avl_root = NULL;
    int unbalance = FALSE;
    singleop_count = 0;
    for(int i = origin_op_count-1; i>=0; i--) {
        if(origin_op_tasks[i].is_useless) {
            continue;
        }
        if(origin_op_tasks[i].op==RANGE) {
            origin_op_tasks[i].range_num = 0;
            origin_op_tasks[i].range_id = singleop_count;
            origin_op_tasks[i].head = origin_op_tasks[i].tail = NULL;
            avl_print(avl_root,origin_op_tasks[i].m,origin_op_tasks[i].n,&origin_op_tasks[i].range_num);
        } else {
            if(origin_op_tasks[i].op==DELETE || origin_op_tasks[i].op==UPDATE) {
                avl_insert(&avl_root,&unbalance,&origin_op_tasks[i].key,sizeof(key_t),compare_key_to_key,construct_do_nothing,update_preprocess);
            } else if(origin_op_tasks[i].op==READ) {
                origin_op_tasks[i].read_id = singleop_count;
            }
            single_tasks[singleop_count].id = singleop_count;
            single_tasks[singleop_count].op = origin_op_tasks[i].op;
            single_tasks[singleop_count].key = origin_op_tasks[i].key;
            if(origin_op_tasks[i].op==INSERT||origin_op_tasks[i].op==UPDATE) {
                single_tasks[singleop_count].child = make_record(origin_op_tasks[i].v,origin_op_tasks[i].vsize);
                single_tasks[singleop_count].vsize = origin_op_tasks[i].vsize;
            }
            singleop_count++;
        }
    }
    avl_destory(&avl_root,NULL);
    for(int i = 0; i < singleop_count/2; i++) {
        singleop temp = single_tasks[i];
        single_tasks[i] = single_tasks[singleop_count-1-i];
        single_tasks[singleop_count-1-i] = temp;
    }
    //DEBUG("there is %d singops :",singleop_count);
    for(int i = 0; i < singleop_count; i++) {
        switch(single_tasks[i].op) {
        case READ:
            //DEBUG("r %d ",single_tasks[i].key);
            break;
        case DELETE:
            //DEBUG("del %d ",single_tasks[i].key);
            break;
        case INSERT:
            //DEBUG("insert %d ",single_tasks[i].key);
            break;
        case UPDATE:
            //DEBUG("update %d ",single_tasks[i].key);
            break;
        default:
            break;
        }
    }
    //DEBUG("\n");
}
int compare_insert_log(void *data,void *newdata)
{
    struct insert_log * a = (struct insert_log *)data,*b = (struct insert_log *)newdata;
    if(a->key < b->key) return -1;
    if(a->key == b->key) return 0;
    return 1;
}
void *update_insert_log(void *data,void *newdata)
{
    struct insert_log * a = (struct insert_log *)data,*b = (struct insert_log *)newdata;
    if(a->key < b->key) return newdata;
    return data;
}
avl_pointer build_avl_insert_log()
{
    avl_pointer avl_root = NULL;
    int unbalance = FALSE;
    for(int i = 0; i < insert_results_count; i++) {
        avl_insert(&avl_root,&unbalance,&insert_results[i],sizeof(struct insert_log),compare_insert_log,construct_do_nothing,update_insert_log);
    }
    return avl_root;
}
int compare_key_insertlog(void *data,void *newdata)
{
    if(*((int*)data) == ((struct insert_log*)newdata)->key) return 0;
    if(*((int*)data) < ((struct insert_log*)newdata)->key) return -1;
    return 1;
}
int compare_id_readresult(void * data,void *newdata)
{
    //DEBUG("compare_id_readresult %d %d\n",*((int*)(data)),((struct _read_result *)newdata)->id);
    if(*((int*)data) == ((struct _read_result*)newdata)->id) return 0;
    if(*((int*)data) < ((struct _read_result*)newdata)->id) return -1;
    return 1;
}
int compare_const_id_readresult(const void * data,const void *newdata)
{
    if(*((const int*)data) == ((const struct _read_result*)newdata)->id) return 0;
    if(*((const int*)data) < ((const struct _read_result*)newdata)->id) return -1;
    return 1;
}
struct read_pair * make_readpair(key_t key,size_t size,void * v)
{
    struct read_pair * tmp;
    if((tmp=(struct read_pair *)malloc(sizeof(struct read_pair)))==NULL) {
        perror("make_readpair malloc failed");
        exit(-1);
    }
    tmp->key = key;
    tmp->next = NULL;
    tmp->v = v;
    tmp->vsize = size;
    return tmp;
}
void print_read_range_op(struct origin_op op)
{
    if(op.op==READ) {
        if(op.v) {
            DEBUG("[read <%d %d>] ",op.key,*((int*)op.v));
            free(op.v);
        } else {
            DEBUG("[read <%d null>]",op.key);
        }
    } else {
        DEBUG("range [%d %d]:",op.m,op.n);
        for(struct read_pair * t = op.head; t; t=t->next) {
            DEBUG("<%d,%d>",t->key,*((int *)t->v));
        }
        DEBUG("\n");
        struct read_pair head;
        head.next = op.head;
        while(head.next) {
            struct read_pair * t = head.next;
            head.next = head.next->next;
            free(t);
        }
    }
    DEBUG("\n\n");
}
void range_operation(struct origin_op *op,node_pointer leaf,avl_pointer avl_root)
{
    //DEBUG("in range_operation\n");
    if(op->op == READ) {
        //struct _read_result * result = (struct _read_result *)bsearch(&op.read_id,read_results,read_results_count,sizeof(struct _read_result),compare_id_readresult);
        //DEBUG("in read\n");
        int index = search_exact(read_results,sizeof(struct _read_result),read_results_count,&op->read_id,compare_id_readresult);
        op->v = read_results[index].value;
        op->vsize = read_results[index].data_size;
        return;
    }
    node_pointer t = leaf;
    struct _read_result * result = (op->range_num > 0)?(struct _read_result *)bsearch(&op->range_id,read_results,read_results_count,sizeof(struct _read_result),compare_const_id_readresult):NULL;
    int result_count = 0;
    for(; (!TOID_IS_NULL(t))&&(D_RO(t)->num_keys==0||D_RO(t)->keys[0]<=op->n); TOID_ASSIGN(t,D_RO(t)->pointers[BTREE_ORDER])) {
        if(D_RO(t)->num_keys==0 || D_RO(t)->keys[D_RO(t)->num_keys-1]<op->m) {
            continue;
        }
        for(int i = 0; i < D_RO(t)->num_keys; i++) {
            struct insert_log * temp = (struct insert_log *)avl_read(avl_root,&D_RW(t)->keys[i],compare_key_insertlog);
            if(temp==NULL||temp->id > op->range_id) {
                //DEBUG("in here\n");
                /**
                because the single_tasks has been built reversely,so the first operation has bigger id
                */
                if(op->range_num) {
                    for(; result_count < op->range_num&& result[result_count].key < D_RO(t)->keys[i]; result_count++) {
                        if(result[result_count].value == NULL) {
                            continue;
                        }
                        struct read_pair * tmp = make_readpair(result[result_count].key,result[result_count].data_size,result[result_count].value);
                        if(op->head==NULL) {
                            op->head =op->tail = tmp;
                        } else {
                            op->tail->next=tmp;
                            op->tail=tmp;
                        }
                    }
                    if(result_count < op->range_num && result[result_count].key == D_RO(t)->keys[i]) {
                        result_count++;
                    }
                }
                //DEBUG("before make pair");
                struct read_pair * tmp = make_readpair(D_RO(t)->keys[i],D_RO(D_RO(t)->arg)->vsize[i],get_record(D_RO(t)->pointers[i],D_RO(D_RO(t)->arg)->vsize[i]));
                if(op->head==NULL) {
                    op->head =op->tail = tmp;
                } else {
                    op->tail->next=tmp;
                    op->tail=tmp;
                }
            }
        }
    }
}
int compare_id(const void *a,const void *b)
{
    if(*((int *)a)==*((int*)b)) return 0;
    if(*((int *)a) < *((int*)b)) return -1;
    return 1;
}
void * parrell_operation(void *arg)
{
    struct operation_arg * a = (struct operation_arg *)arg;
    //DEBUG("start index = %d step = %d\n",a->start_index,a->step);
    for(int i = a->start_index; i < nodeop_count; i+=a->step) {
        operation(a->root,tasks[i],a->REBALANCE);
    }
    return NULL;
}
void post_process()
{
    for(int i = leaf_alloc_count; i < leaf_alloc_max; i++) {
        TX_FREE(leafs[i]);
    }
    for(int i = node_alloc_count; i < node_alloc_max; i++) {
        TX_FREE(internal_nodes[i]);
    }
    for(int i = 0; i < free_count; i++) {
        pmemobj_tx_free(free_records[i]);
    }
}
void *parrell_range_operation(void *arg)
{
    struct range_operation_arg * t = (struct range_operation_arg *)arg;
    for(int i = t->start_index; i < origin_op_count; i+=t->step) {
        if(origin_op_tasks[i].op==READ||origin_op_tasks[i].op==RANGE) {
            range_operation(&origin_op_tasks[i],t->leaf,t->avl_root);
        }
    }
    return NULL;
}
void print_readresult()
{
    for(int i = 0; i < origin_op_count; i+=1) {
        if(origin_op_tasks[i].op==READ||origin_op_tasks[i].op==RANGE) {
            print_read_range_op(origin_op_tasks[i]);
        }
    }
}
void save_leaves_mask(node_pointer leaf)
{
    node_pointer c = leaf;
    while(!TOID_IS_NULL(c)) {
        TX_SET(c,is_mask,FALSE);
        TOID_ASSIGN(c,D_RW(c)->pointers[BTREE_ORDER]);
    }
}
void palm_process(node_pointer *root)
{
    pthread_t threads[THREAD_NUM];
    palm_init(root);
    node_pointer left = get_leftest_leaf(*root);
    save_leaves_mask(left);
    struct assign_arg assignargs[THREAD_NUM];
    struct operation_arg operationargs[THREAD_NUM];
    struct range_operation_arg rangeargs[THREAD_NUM];
    for(int i = 0; i < THREAD_NUM; i++) {
        assignargs[i].root = *root;
        assignargs[i].start_index = i;
        assignargs[i].step = THREAD_NUM;
    }
    for(int i = 0; i < THREAD_NUM; i++) {
        pthread_create(&threads[i],NULL,(void *)assigntask,(void *)&assignargs[i]);
    }
    for(int j = 0; j < THREAD_NUM; j++) {
        pthread_join(threads[j],NULL);
    }
    /*struct assign_arg a = {*root,0,1};
    assigntask(&a);*/
    DEBUG("there********** \n");
    redistribute_singleop();
    preadd();
    //DEBUG("node count %d\n",nodeop_count);
    for(int i = 0; i < THREAD_NUM; i++) {
        operationargs[i].root = root;
        operationargs[i].start_index = i;
        operationargs[i].step = THREAD_NUM;
        operationargs[i].REBALANCE = FALSE;
        pthread_create(&threads[i],NULL,parrell_operation,&operationargs[i]);
    }
    for(int i = 0; i < THREAD_NUM; i++) {
        pthread_join(threads[i],NULL);
    }
    clear_nodeop();
    /**leaf stage is end*/
    qsort(read_results,read_results_count,sizeof(struct _read_result),compare_id);
    DEBUG("start print read_record,has %d logs\n",read_results_count);
    avl_pointer avl_root = build_avl_insert_log();
    for(int i = 0; i < THREAD_NUM; i++) {
        rangeargs[i].start_index = i;
        rangeargs[i].step = THREAD_NUM;
        rangeargs[i].avl_root = avl_root;
        rangeargs[i].leaf = left;
        pthread_create(&threads[i],NULL,parrell_range_operation,&rangeargs[i]);
    }
    for(int i = 0; i < THREAD_NUM; i++) {
        pthread_join(threads[i],NULL);
    }
    avl_destory(&avl_root,NULL);
    DEBUG("after print record\n");
    while(*nexttasks_count) {
        print_singleop();
        assert(nexttasks!=NULL);
        DEBUG("nexttasks %d singleop %d modified_list %d\n",*nexttasks_count,singleop_count,modified_list_count);
        fflush(stdout);
        circle();
        redistribute_singleop();
        DEBUG("after redistribute\n");
        for(int i = 0; i < THREAD_NUM; i++) {
            operationargs[i].root = root;
            operationargs[i].start_index = i;
            operationargs[i].step = THREAD_NUM;
            operationargs[i].REBALANCE = FALSE;
            pthread_create(&threads[i],NULL,parrell_operation,&operationargs[i]);
        }
        for(int i = 0; i < THREAD_NUM; i++) {
            pthread_join(threads[i],NULL);
        }
        clear_nodeop();
    }
    DEBUG("print tree");
    //print_btree(*root);
    DEBUG("\nstart clean mess stage\n");
    if(DELETE_FLAG) {
        clear_mess(left);
        for(int i = 0; i < THREAD_NUM; i++) {
            if(pthread_create(&threads[i],NULL,(void *)assigntask,(void *)&assignargs[i])!=0) {
                break;
            }
        }
        for(int j = 0; j < THREAD_NUM; j++) {
            pthread_join(threads[j],NULL);
        }
        redistribute_singleop();
        DEBUG("\nnode count %d\n",nodeop_count);
        for(int i = 0; i < THREAD_NUM; i++) {
            operationargs[i].root = root;
            operationargs[i].start_index = i;
            operationargs[i].step = THREAD_NUM;
            operationargs[i].REBALANCE = TRUE;
            pthread_create(&threads[i],NULL,parrell_operation,&operationargs[i]);
        }
        for(int i = 0; i < THREAD_NUM; i++) {
            pthread_join(threads[i],NULL);
        }
        clear_nodeop();
        while(*nexttasks_count) {
            circle();
            redistribute_singleop();
            DEBUG("\nnode count %d\n",nodeop_count);
            for(int i = 0; i < THREAD_NUM; i++) {
                operationargs[i].root = root;
                operationargs[i].start_index = i;
                operationargs[i].step = THREAD_NUM;
                operationargs[i].REBALANCE = FALSE;
                pthread_create(&threads[i],NULL,parrell_operation,&operationargs[i]);
            }
            for(int i = 0; i < THREAD_NUM; i++) {
                pthread_join(threads[i],NULL);
            }
            clear_nodeop();
        }
    }
    post_process();
    print_readresult();
    //DEBUG("here!\n");
}
void * construct_opavl(void *data,size_t data_size)
{
    void * temp = (void *)malloc(data_size);
    if(temp==NULL) {
        perror("construct opavl malloc failed");
        exit(-1);
    }
    memcpy(temp,data,data_size);
    return temp;
}
int compare_opavl(void *data,void *newdata)
{
    struct origin_opavl * a = (struct origin_opavl *)data,*b = (struct origin_opavl*)newdata;
    if(a->key==b->key) return 0;
    if(a->key < b->key) return -1;
    return 1;
}
void preprocess_originop()
{
    int insert_op_num = 0;
    avl_pointer avl_root = NULL;
    int unbalanced = FALSE;
    for(int i = 0; i < origin_op_count; i++) {
        origin_op_tasks[i].is_useless = FALSE;
        if(origin_op_tasks[i].op==RANGE) {
            continue;
        }
        struct origin_opavl t;
        t.key = origin_op_tasks[i].key;
        if(origin_op_tasks[i].op==INSERT||origin_op_tasks[i].op==UPDATE) {
            t.op = INSERT;
        } else {
            t.op = origin_op_tasks[i].op;
        }
        struct origin_opavl * temp = (struct origin_opavl *)avl_read(avl_root,&t,compare_opavl);
        if(temp==NULL) {
            if(origin_op_tasks[i].op==INSERT||origin_op_tasks[i].op==UPDATE||origin_op_tasks[i].op==DELETE) {
                avl_insert(&avl_root,&unbalanced,&t,sizeof(t),compare_opavl,construct_opavl,NULL);
                insert_op_num += (origin_op_tasks[i].op==INSERT);
            }
        } else {
            if(temp->op==INSERT) {
                if(origin_op_tasks[i].op==DELETE) {
                    temp->op = DELETE;
                } else if(origin_op_tasks[i].op==INSERT) {
                    origin_op_tasks[i].is_useless = TRUE;
                    free(origin_op_tasks[i].v);
                }
            } else if(temp->op==DELETE) {
                if(origin_op_tasks[i].op==DELETE || origin_op_tasks[i].op==UPDATE) {
                    origin_op_tasks[i].is_useless = TRUE;
                    if(origin_op_tasks[i].op==UPDATE) {
                        free(origin_op_tasks[i].v);
                    }
                } else if(origin_op_tasks[i].op==INSERT) {
                    temp->op = INSERT;
                    insert_op_num++;
                }
            }
        }
    }
    avl_destory(&avl_root,free);
    leaf_alloc_max = MAX_QUERY_SIZE;
    node_alloc_max = MAX_QUERY_SIZE*2;
}

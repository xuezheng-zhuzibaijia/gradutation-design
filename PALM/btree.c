#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "btree.h"
node_pointer make_node()
{
    node_pointer t = TX_NEW(struct tree_node);
    D_RW(t)->num_keys = 0;
    D_RW(t)->is_leaf = FALSE;
    D_RW(t)->parent = TOID_NULL(struct tree_node);
    D_RW(t)->arg = TOID_NULL(struct leaf_arg);
    return t;
}
node_pointer make_leaf()
{
    node_pointer t = make_node();
    D_RW(t)->arg = TX_NEW(struct leaf_arg);
    D_RW(D_RW(t)->arg)->is_deleted = FALSE;
    D_RW(t)->is_leaf = TRUE;
    D_RW(t)->pointers[BTREE_ORDER] = OID_NULL;
    return t;
}

void btree_init(TOID(struct tree) t)
{
    if(TOID_IS_NULL(D_RO(t)->root,struct tree_node))
    {
        TX_SET(t,root,make_leaf());
    }
}
static int search_exact(key_t * keys,key_t key,int length)
{
    int left = 0,right = length - 1;
    while(left <= right)
    {
        int mid = (left+right)/2;
        if(keys[mid]==key)
        {
            return mid;
        }
        if(keys[mid] < key)
        {
            left = mid + 1;
        }
        else
        {
            right = mid - 1;
        }
    }
    return -1;
}
static int search_fuzzy(key_t *keys,key_t key,int length)
{
    int left = 0,right = length - 1;
    while(left <= right)
    {
        int mid = (left+right)/2;
        if(keys[mid]>  key)
        {
            right = mid -1;
        }
        else
        {
            left = mid + 1;
        }
    }
    return left;
}
static void leaf_read(struct keyop  p, key_t * keys,PMEMoid * values,size_t * vsizes,int length)
{
    kvpair tmp;
    tmp.key = p.key;
    tmp.id = p.id;
    int index = search_exact(keys,p.key,length);
    if(index == -1)
    {
        tmp.value = NULL;
        tmp.vsize = 0;
    }
    else
    {
        if((tmp.value=(void*)malloc(p.vsize))==NULL)
        {
            perror("kvpair value malloc failed");
            exit(-1);
        }
        memcpy(tmp.value,pmemobj_direct(values[index]),vsizes[index]);
        tmp.vsize = vsizes[index];
    }int m;
    do{
        m = read_record_count;
    }while(!__sync_compare_and_swap(&read_record_count,m,m+1));
    read_record[m] = tmp;
}
static void leaf_update(struct keyop p,key_t *keys,PMEMoid*values,size_t *vsizes,int length)
{
    int index = search_exact(keys,p.key,length);
    if(index==-1) return;
    if(p.update==NULL)
    {
        PMEMoid tmp = pmemobj_tx_alloc(p.vsize,TOID_TYPE_NUM(void));
        memcpy(pmemobj_direct(tmp),p.value,p.vsize);
        pmemobj_tx_free(values[index]);
        values[index] = tmp;
        vsizes[index] = p.vsize;
    }
    else
    {
        pmemobj_tx_add_range(values[index],0,vsizes[index]);
        p.update(pmemobj_direct(values[index]));
    }
}
static int leaf_insert(struct keyop p,key_t *keys,PMEMoid *values,size_t *vsizes,int* length)
{
    int index = search_exact(keys,p.key,*length);
    if(index!=-1)
    {
        return;
    }
    index = search_fuzzy(keys,p.key,length);
    for(int i = length-1; i>=index; i--)
    {
        keys[i+1] = keys[i];
        values[i+1] = values[i];
        vsizes[i+1] = vsizes[i];
    }
    keys[index] = p.key;
    PMEMoid tmp = pmemobj_tx_alloc(p.vsize,TOID_TYPE_NUM(void));
    memcpy(pmemobj_direct(tmp),p.value,p.vsize);
    values[index] = tmp;
    vsizes[index] = p.vsize;
    (*length)++;
    int record_index;
    do
    {
        record_index = record_list_count;
    }
    while(!__sync_compare_and_swap(&record_list_count,record_index,record_index+1));
    record_list[record_index].id = p.id;
    record_list[record_index].key = p.key;
}
static void leaf_delete(struct keyop p,key_t* keys,PMEMoid *values,size_t*vsizes,int *length)
{
    int index = search_exact(keys,p.key,*length);
    if(index==-1)
    {
        return;
    }
    pmemobj_tx_free(values[index]);
    for(int i = index; i < (*length)-1; i++)
    {
        keys[i] = keys[i+1];
        values[i] = values[i+1];
        vsizes[i] = vsizes[i+1];
    }
    (*length)--;
}
static void node_insert(struct keyop p,key_t * keys,PMEMoid *children,int* length)
{
    int index = search_fuzzy(keys,p.key,*length);
    for(int i = (*length)-1; i>=index; i--)
    {
        keys[i+1] = keys[i];
        children[i+2] = children[i+1];
    }
    keys[i] = p.key;
    children[i] = p.child;
    (*length)++;
}
static void node_delete(struct keyop p,key_t *keys,PMEMoid *children,int *length)
{
    int index = -1;
    for(int i = 0; i < (*length+1); i++)
    {
        if(OID_EQUALS(p.child,children[i]))
        {
            index = i;
            break;
        }
    }
    if(index==-1)
    {
        perror("delete child doesn't exist");
        exit(-1);
    }
    for(int i = index; i < *length; i++)
    {
        if(i+1!=*length)
        {
            keys[i] = keys[i+1];
        }
        children[i] = children[i+1];
    }(*length)--;
}

static void leaf_operation(struct nodeop * p,node_pointer root)
{
    node_pointer leaf = p->n;
    key_t * keys;
    PMEMoid * values;
    size_t * vsizes;
    if((keys=(key_t *)malloc(sizeof(key_t)*(BTREE_ORDER+MAX_LIST_SIZE)))==NULL)
    {
        perror("keys malloc failed!");
        exit(-1);
    }
    if((values=(PMEMoid *)malloc(sizeof(PMEMoid)*(BTREE_ORDER+MAX_LIST_SIZE)))==NULL)
    {
        free(keys);
        perror("values malloc failed");
        exit(-1);
    }
    if((vsizes=(size_t *)malloc(sizeof(size_t)*(BTREE_ORDER+MAX_LIST_SIZE)))==NULL)
    {
        free(keys);
        free(values);
        perror("vsize malloc failed");
        exit(-1);
    }
    TX_ADD(leaf);
    int length = D_RO(leaf)->num_keys;
    memcpy(keys,D_RO(leaf)->keys,sizeof(key_t)*length);
    memcpy(values,D_RO(leaf)->pointers,sizeof(PMEMoid)*length);
    memcpy(vsizes,D_RO(D_RO(leaf)->arg)->vsizes,sizeof(size_t)*length);
    for(int i = 0; i < p->num; i++)
    {
        switch(p->op_list[i].op)
        {
        case DELETE:
            leaf_delete(p->op_list[i],keys,values,vsizes,&length);
            break;
        case INSERT:
            leaf_insert(p->op_list[i],keys,values,vsizes,&length);
            break;
        case READ:
            leaf_read(p->op_list[i],keys,values,vsizes,length);
            break;
        case UPDATE:
            leaf_update(p->op_list[i],keys,values,vsizes,length);
            break;
        default:
            break;

        }
    }
    //split or underflow
    TX_ADD(D_RO(leaf)->arg);
    if(length <= BTREE_ORDER)
    {
        if(length < BTREE_MIN && !TOID_EQUALS(root,leaf))
        {
            D_RW(D_RW(leaf)->arg)->is_deleted = TRUE;
        }
        D_RW(leaf)->num_keys = length;
        memcpy(D_RO(leaf)->keys,keys,sizeof(key_t)*length);
        memcpy(D_RO(leaf)->pointers,values,sizeof(PMEMoid)*length);
        memcpy(D_RO(D_RO(leaf)->arg)->vsize,vsizes,sizeof(size_t)*length);
    }
    else
    {
        PMEMoid tail = D_RO(leaf)->pointers[BTREE_ORDER];
        node_pointer sibling = leaf;
        D_RO(leaf)->num_keys = BTREE_MIN;
        memcpy(D_RO(leaf)->keys,keys,sizeof(key_t)*BTREE_MIN);
        memcpy(D_RO(leaf)->pointers,values,sizeof(PMEMoid)*BTREE_MIN);
        memcpy(D_RO(D_RO(leaf)->arg)->vsize,vsizes,sizeof(size_t)*BTREE_MIN);
        int base = BTREE_ORDER;
        int tmp_count;
        while(length - base > BTREE_ORDER)
        {
            node_pointer tmp = make_leaf();
            D_RW(tmp)->num_keys = BTREE_MIN;
            memcpy(D_RO(tmp)->keys,keys+base,sizeof(key_t)*BTREE_MIN);
            memcpy(D_RO(tmp)->pointers,values+base,sizeof(PMEMoid)*BTREE_MIN);
            memcpy(D_RO(D_RO(tmp)->arg)->vsize,vsizes+base,sizeof(size_t)*BTREE_MIN);
            do
            {
                tmp_count = modified_list_count;
            }
            while(!__sync_compare_and_swap(&modified_list_count,tmp_count,tmp_count+1));
            modified_list[tmp_count].op = INSERT;
            modified_list[tmp_count].child = tmp.oid;
            modified_list[tmp_count].key = D_RO(tmp)->keys[0];
            targets[tmp_count] = tmp;
            D_RW(sibling)->pointers[BTREE_ORDER] = tmp.oid;
            sibling = tmp;
            base += BTREE_MIN;
        }
        node_pointer tmp = make_leaf();
        D_RW(tmp)->num_keys = length-base;
        memcpy(D_RO(tmp)->keys,keys+base,sizeof(key_t)*(length-base));
        memcpy(D_RO(tmp)->pointers,values+base,sizeof(PMEMoid)*(length-base));
        memcpy(D_RO(D_RO(tmp)->arg)->vsizes,vsizes+base,sizeof(size_t)*(length-base));
        do
        {
            tmp_count = modified_list_count;
        }
        while(!__sync_compare_and_swap(&modified_list_count,tmp_count,tmp_count+1));
        modified_list[tmp_count].op = INSERT;
        modified_list[tmp_count].child = tmp.oid;
        modified_list[tmp_count].key = D_RO(tmp)->keys[0];
        targets[tmp_count] = tmp;
        D_RW(sibling)->pointers[BTREE_ORDER] = tmp.oid;
        D_RW(tmp)->pointers[BTREE_ORDER] = tail;
    }
    free(keys);
    free(values);
    free(vsizes);
}
static void destory_subtree(node_pointer n)
{
    if(D_RO(n)->is_leaf)
    {
        TX_SET(D_RW(n)->arg,is_deleted,TRUE);
    }
    else
    {
        node_pointer c;
        for(int i = 0; i < D_RO(n)->num_keys+1; i++)
        {
            TOID_ASSIGN(c,D_RO(n)->pointers[i]);
            destory_subtree(c);
        }
        TX_FREE(n);
    }
}

static void node_operation(struct nodeop *p,node_pointer * root)
{
    node_pointer n = p->n;
    //root has split,need to build a new one
    if(TOID_IS_NULL(n))
    {
        n = make_node();
        *root = n;
    }
    key_t * keys;
    PMEMoid * children;
    if((keys=(key_t *)malloc(sizeof(key_t)*MAX_LIST_SIZE))==NULL)
    {
        perror("keys malloc failed!");
        exit(-1);
    }
    if((children=(PMEMoid *)malloc(sizeof(PMEMoid)*MAX_LIST_SIZE))==NULL)
    {
        free(keys);
        perror("values malloc failed");
        exit(-1);
    }
    TX_ADD(n);
    int length = D_RO(n)->num_keys;
    memcpy(keys,D_RO(n)->keys,sizeof(key_t)*length);
    memcpy(children,D_RO(n)->pointers,sizeof(PMEMoid)*(length+1));
    for(int i = 0; i < p->num; i++)
    {
        switch(p->op_list[i].op)
        {
        case DELETE:
            node_delete(p->op_list[i],keys,children,&length);
            break;
        case INSERT:
            node_insert(p->op_list[i],keys,children,&length);
            break;
        default:
            break;
        }
    }
    int insert_index;
    if(length <= BTREE_ORDER)
    {
        if(length >= 0){
            D_RW(n)->num_keys = length;
            memcpy(D_RO(n)->keys,keys,sizeof(key_t)*length);
            memcpy(D_RO(n)->pointers,children,sizeof(PMEMoid)*(length+1));
        }
        for(int i = 0; i < length+1; i++)
        {
            node_pointer c;
            TOID_ASSIGN(c,D_RO(n)->pointers[i]);
            D_RW(c)->parent = n;
        }
        if(length < BTREE_MIN)
        {
            if(TOID_EQUALS(*root,n))
            {
                    if(length  == 0){ //root is an interval node and it's  only has one child then make the root be the first child
                            TX_FREE(n);
                            TOID_ASSIGN(*root,D_RO(n)->pointers[0]);
                    }else{  //root has no child,so just swap it with a new leaf;
                        if(length < 0){
                            TX_FREE(n);
                            *root = make_leaf();
                        }
                    }
            }
            else
            {
                destory_subtree(n);
                do
                {
                    insert_index = modified_list_count;
                }
                while(!__sync_compare_and_swap(&modified_list_count,insert_index,insert_index + 1));
                modified_list[insert_index].child = n.oid;
                modified_list[insert_index].op = DELETE;
                targets[insert_index] = D_RO(n)->parent;
            }
        }
    }
    else
    {
        D_RW(n)->num_keys = BTREE_MIN;
        memcpy(D_RO(n)->keys,keys,sizeof(key_t)*BTREE_MIN);
        memcpy(D_RO(n)->pointers,children,sizeof(PMEMoid)*(BTREE_MIN+1));
        for(int i = 0; i < BTREE_MIN+1; i++)
        {
            node_pointer c;
            TOID_ASSIGN(c,D_RO(n)->pointers[i]);
            D_RW(c)->parent = n;
        }
        int base = BTREE_MIN;
        node_pointer tmp;
        while(length-base > BTREE_ORDER)
        {
            tmp = make_node();
            memcpy(D_RO(tmp)->keys,keys+base+1,sizeof(key_t)*(BTREE_MIN-1));
            memcpy(D_RO(tmp)->pointers,children+base,sizeof(PMEMoid)*BTREE_MIN);
            for(int i = 0; i < BTREE_MIN; i++)
            {
                node_pointer c;
                TOID_ASSIGN(c,D_RO(tmp)->pointers[i]);
                D_RW(c)->parent = n;
            }
            do
            {
                insert_index = modified_list_count;
            }
            while(!__sync_compare_and_swap(&modified_list_count,insert_index,insert_index + 1));
            modified_list[insert_index].child = tmp.oid;
            modified_list[insert_index].op = INSERT;
            modified_list[insert_index].key = keys[k];
            targets[insert_index] = D_RO(n)->parent;
            base += BTREE_MIN;
        }
        tmp = make_node();
        memcpy(D_RO(tmp)->keys,keys+base+1,sizeof(key_t)*(length-base-1));
        memcpy(D_RO(tmp)->pointers,children+base,sizeof(PMEMoid)*(length-base));
        for(int i = 0; i < length-base; i++)
        {
            node_pointer c;
            TOID_ASSIGN(c,D_RO(tmp)->pointers[i]);
            D_RW(c)->parent = n;
        }
        do
        {
            insert_index = modified_list_count;
        }
        while(!__sync_compare_and_swap(&modified_list_count,insert_index,insert_index + 1));
        modified_list[insert_index].child = tmp.oid;
        modified_list[insert_index].op = INSERT;
        modified_list[insert_index].key = keys[k];
        targets[insert_index] = D_RO(n)->parent;
    }
    free(keys);
    free(children);
}

node_pointer get_leaf(node_pointer root,key_t key){
    node_pointer c = root;
    while(!D_RO(c)->is_leaf){
        TOID_ASSIGN(c,D_RO(c)->pointers[search_fuzzy(D_RO(c)->keys,key,D_RO(c)->num_keys)]);
    }return c;
}
node_pointer get_leftest_leaf(node_pointer root){
    node_pointer c = root;
    while(!D_RO(c)->is_leaf){
        TOID_ASSIGN(c,D_RO(c)->pointers[0]);
    }return c;
}
void common_operation(struct nodeop *p,node_pointer * root){
    if(TOID_IS_NULL(D_RO(p->n)) || (!D_RO(p->n)->is_leaf)){
        node_operation(p,root);
    }else{
        leaf_operation(p,*root);
    }
}

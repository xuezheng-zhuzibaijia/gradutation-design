#include <stdio.h>
#include <stdlib.h>
#include "palm.h"
/***************tree operation********/
static int search(key_t *a,int length,key_t key)
{

    int left = 0,right = length - 1;
    while(left <= right)
    {
        int mid = (left + right)/2;
        if(keys[mid] > key)
        {
            right =  mid - 1;
        }
        else
        {
            left = mid + 1;
        }
    }
    return left;
}
static int search_exact(key_t *keys,int length,key_t key)
{
    int left = 0,right = length - 1;
    while(left<=right)
    {
        int mid = (left+right)/2;
        if(keys[mid]==key)
        {
            return mid;
        }
        if(keys[mid]>key)
        {
            right = mid-1;
        }
        else
        {
            left = mid+1;
        }
    }
    return -1;
}

static node_pointer find_leaf(node_pointer root,key_t key)
{
    node_pointer temp = root;
    while(!D_RO(temp)->is_leaf)
    {
        TOID_ASSIGN(temp,D_RO(temp)->pointers[search(D_RO(temp)->keys,D_RO(temp)->num_keys,key)]);
    }
    return temp;
}
static node_pointer make_node
{
    node_pointer n = TX_NEW(struct tree_node);
    TX_SET(n,is_leaf,FALSE);
    TX_SET(n,num_keys,0);
    TX_SET(n,parent,TOID_NULL(struct tree_node));
    TX_SET(n,mask,NORMAL);
    return n;
}
static node_pointer make_leaf()
{
    node_pointer n = make_node();
    D_RW(n,is_leaf,TRUE);
    return n;
}
/** 这个函数是专门处理对叶子节点的读操作的
  */
static kv_pair * read_key(key_t *keys,PMEMoid * values,size_t * value_size,int length,struct key_op op)
{
    kv_pair * result;
    if((result=(kv_pair *)malloc(sizeof(kv_pair)))==NULL)
    {
        perror("READ KEY MALLOC FAILED");
        exit(-1);
    }
    result->key = op.key;
    int k = search_exact(keys,length,op.key);
    if(k==-1)
    {
        result->value = NULL;
        result->value_size = 0;
    }
    else
    {
        result->value_size =  value_size[k];
        if((result->value=(void *)malloc(sizeof(void)*result->value_size))==NULL)
        {
            perror("READ KEY MALLOC ERROR");
            exit(-1);
        }
        memcpy(result->value,pmemobj_direct(values[k]),result->value_size);
    }
    return result;
}
/**
　　　*处理节点的插入操作
   */
static void insert_node(key_t * keys,PMEMoid * values,int* length,struct key_op op,node_pointer parent)
{
    int k = search(keys,*length,op.key);
    if(k < (*length))
    {
        for(int i = (*length)-1; i >= k; i--)
        {
            keys[i+1] = keys[i];
            values[i+2] = values[i+1];
        }
    }
    keys[k] = op.key;
    values[k+1] = op.child;
    node_pointer n;
    TOID_ASSIGN(n,values[k+1]);
    TX_SET(n,parent,parent);
    (*length)++;
}
static void insert_leaf(key_t *keys,PMEMoid * values,size_t * value_size,int *length,struct key_op op)
{
    int k = search_exact(keys,*length,op.key);
    if(k!=-1) return;
    k = search(keys,*length,op.key);
    if(k < (*length))
    {
        for(int i = (*length)-1; i >= k; i--)
        {
            keys[i+1] = keys[i];
            values[i+1] = values[i];
            value_size[i+1] = value_size[i];
        }
    }
    keys[k] = op.key;
    PMEMoid tmpvalue = pmemobj_tx_alloc(op.value_size,TOID_TYPE_NUM_OF(void));
    values[k] = tmpvalue;
    value_size[k] = op.value_size;
    int m;
    do
    {
        m = record_list_count;
    }
    while(!__sync_bool_compare_and_swap (&record_list_count,m,m+1));
    record_list[m].id = op.id;
    record_list[m].key = op.key;
    (*length)++;
}
/**update叶子节点
   */
static void update_leaf_value(key_t * keys,PMEMoid * values,size_t *value_size,int length,struct key_op op)
{
    int k = search_exact(keys,length,op.key);
    if(k==-1) return;
    PMEMoid tmpvalue = pmemobj_tx_alloc(op.value_size,TOID_TYPE_NUM_OF(void));
    TX_MEMCPY(pmemobj_direct(tmpvalue),op.value,op.value_size);
    pmemobj_tx_free(values[k]);
    values[k] = tmpvalue;
    value_size[k] = op.value_size;
}
static void update_leaf_func(key_t *keys,PMEMoid *values,size_t *value_size,int length,struct key_op op)
{
    int k = search_exact(keys,length,op.key);
    if(k==-1) return;
    void * v = pmemobj_direct(values[k]);
    pmemobj_tx_add_range_direct(v,value_size[k]);
    op.update(v);
}

static void delete_leaf(key_t *keys,PMEMoid * values,size_t *value_size,int *length,struct key_op op)
{
    int k = search_exact(keys,length,op.key);
    if(k==-1) return;
    pmemobj_tx_free(values[k]);
    for(int i = k; i < (*length)-1; i++)
    {
        keys[i] = keys[i+1];
        values[i] = values[i+1];
        value_size[i] = value_size[i+1];
    }(*length)--;
}
static int find_pointer_index(PMEMoid * values,int length,PMEMoid v)
{
    for(int i = 0; i < length; i++)
    {
        if(OID_EQUALS(v,values[i]))
        {
            return i;
        }
    }
    return -1;
}
static void delete_node(key_t *keys,PMEMoid * values,int * length,struct key_op op)
{
    int k = find_pointer_index(values,(*length)+1,op.child);
    for(int i = k-1; i < (*length)-1; i++)
    {
        keys[i] = keys[i+1];
        values[i+1] = values[i+2];
    }(*length)--;
}
static node_pointer get_leftest_leaf(node_pointer n){
        node_pointer c = n;
        while(!D_RO(c)->is_leaf){
                TOID_ASSIGN(c,D_RO(c)->pointers[0]);
        }return c;
}
static node_pointer get_rightest_leaf(node_pointer n){
         node_pointer c = n;
        while(!D_RO(c)->is_leaf){
                TOID_ASSIGN(c,D_RO(c)->pointers[D_RO(c)->num_keys]);
        }return c;
}
static void mark_delete(node_pointer n){
        for(node_pointer c = get_leftest_leaf(n);!TOID_EQUALS(c,get_rightest_leaf(n));
        TOID_ASSIGN(c,D_RO(c)->pointers[BTREE_ORDER])){
                TX_SET(c,mask,DELETED);
        }
}
static void destroy_subtree(node_pointer n)
{
    if(D_RO(n)->is_leaf) return;
    node_pointer temp;
    for(int i = 0; i < D_RO(n)->num_keys+1; i++)
    {
        TOID_ASSIGN(temp,D_RO(n)->pointers[i]);
        destroy_subtree(temp);
    }
    TX_FREE(n);
}
static void proceed_node_task(struct node_opset nop)
{
    key_t * keys;
    PMEMoid * values;
    if((keys=(key_t *)malloc(sizeof(key_t)*MAX_LIST_SIZE))==NULL)
    {
        perror("NODE KEYS MALLOC FAILED");
        exit(-1);
    }
    if((values=(PMEMoid *)malloc(sizeof(PMEMoid)*(MAX_LIST_SIZE+1)))==NULL)
    {
        free(keys);
        perror("NODE VALUES MALLOC FAILED");
        exit(-1);
    }
    node_pointer n = nop.n;
    int length = D_RO(n)->num_keys;
    for(int i = 0; i < length; i++)
    {
        keys[i] = D_RO(n)->keys[i];
        values[i] = D_RO(n)->pointers[i];
    }
    values[length] = D_RO(n)->pointers[length];
    for(int i = 0; i < nop.opcount; i++)
    {
        switch(nop->key_op_set[i].op)
        {
        case DELETE:
            delete_node(keys,values,&length,nop->key_op_set[i]);
            break;
        case INSERT:
            insert_node(keys,values,&length,nop->key_op_set[i],n);
        default:
            break;
        }
    }
    if(length < BTREE_ORDER)
    {
        int m;
        do
        {
            m = operation_lst_count;
        }
        while(!__sync_bool_compare_and_swap (&operation_lst_count,m,m+1));
        operation_lst[m].n = D_RO(n)->parent;
        operation_lst[m].arg.child = n.oid;
        operation_lst[m].op = DELETE;
        TX_SET(n,num_keys,length);
        for(int i = 0; i < length; i++)
        {
            TX_SET(n,keys[i],keys[i]);
            TX_SET(n,pointers[i],values[i]);
        }
        TX_SET(n,pointers[length],values[length]);
        mark_delete(n);
        destroy_subtree(n);
    }
    else if(length > BTREE_ORDER)
    {
       //origin node
        for(int i = 0; i < BTREE_MIN; i++)
        {
            TX_SET(n,keys[i],keys[i]);
            TX_SET(n,pointers[i],values[i]);
        }
        TX_SET(n,pointers[BTREE_MIN],values[BTREE_MIN]);
        TX_SET(n,num_keys,BTREE_MIN);

        int m;
        int base = BTREE_MIN;
        node_pointer temp;
        //create new node
        while(length - base > BTREE_ORDER)
        {
            do
            {
                m = operation_lst_count;
            }
            while(!__sync_bool_compare_and_swap (&operation_lst_count,m,m+1));
            temp = TX_NEW(struct tree_node);
            operation_lst[m].n = D_RO(n)->parent;
            operation_lst[m].arg.child = temp.oid;
            operation_lst[m].op = INSERT;
            operation_lst[m].key = keys[base];
            base++;
            for(int i = 0; i < BTREE_MIN-1; i++)
            {
                TX_SET(temp,keys[i],keys[base+i]);
                TX_SET(temp,pointers[i],values[base+i]);
            }
            TX_SET(temp,pointers[base+BTREE_MIN-1],values[BTREE_MIN-1]);
            TX_SET(temp,num_keys,BTREE_MIN-1);
            base += BTREE_MIN;
        }
        //the last new node
        do
        {
            m = operation_lst_count;
        }
        while(!__sync_bool_compare_and_swap (&operation_lst_count,m,m+1));
        temp = TX_NEW(struct tree_node);
        operation_lst[m].n = D_RO(n)->parent;
        operation_lst[m].arg.child = temp.oid;
        operation_lst[m].op = INSERT;
        operation_lst[m].key = keys[base];
        base++;
        for(int i = 0; i < length-base; i++)
        {
            TX_SET(temp,keys[i],keys[base+i]);
            TX_SET(temp,pointers[i],values[base+i]);
        }
        TX_SET(temp,pointers[length-base],values[length]);
        TX_SET(temp,num_keys,length-base);
    }
    else
    {
        for(int i = 0; i < length; i++)
        {
            TX_SET(n,keys[i],keys[i]);
            TX_SET(n,pointers[i],values[i]);
        }
        TX_SET(n,pointers[length],values[length]);
        TX_SET(n,num_keys,length);
    }
}

void proceed_leaf_task(struct node_opset nop){

}








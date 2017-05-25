#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "palm.h"
void palm_init()
{
    modified_list_count = 0;
    record_list_count = 0;
    read_record_count = 0;
    keyop_list_count = 0;
    rangeop_list_count = 0;
}
static int compare(void * data,void *newdata)
{
    key_t * a,*b;
    a = (key_t *)data;
    b = (key_t *)newdata;
    if(a < b)
    {
        return -1;
    }
    if(a == b)
    {
        return 0;
    }
    return 1;
}
static int contain(void *data,void *c)
{
    return 1;
}
static void * construct(void *newdata,size_t data_size)
{
    void * tmp;
    if((tmp=(void *)malloc(data_size))==NULL)
    {
        perror("construct malloc failed!");
        exit(-1);
    }
    memcpy(tmp,newdata,data_size);
    return tmp;
}
static int is_pass(struct condition c,key_t key)
{
    switch(c.op)
    {
    case EQ:
        return c.value == key;
    case LT:
        return c.value < key;
    case LE:
        return c.value <= key;
    case GT:
        return c.value > key;
    case GE:
        return c.value >= key;
    case NE:
        return c.value != key;
    default:
        perror("unknow relop option");
        exit(-1);
    }
    return 0;
}
static int contain1(void *data,void *c)
{
    struct condition_set * cset = (struct condition_set)c;
    if(cset==NULL)
    {
        return 1;
    }
    key_t * t = (key_t *)data;
    int base = is_pass(cset->c[0],*t);
    for(int i = 1; i < cset->num; i++)
    {
        int tmp = is_pass(cset->c[i],*t);
        switch(cset->bset[i-1])
        {
        case AND:
            base = base && tmp;
            break;
        case OR:
            base = base || tmp;
            break;
        default:
            perror("unknow boolop option");
            exit(-1);
        }
    }
    return base;
}
void operation_init
{
    avl_pointer avl_root;
    int unbalanced;
    avl_init(&avl_root,&unbalanced);
    for(int i = operation_list_count-1; i>=0; i--)
    {
        if(operation_list[i].op==UPDATE || operation_list[i].op==DELETE)
        {
            avl_insert(&avl_root,&operation_list[i].key,sizeof(key_t),&unbalanced,construct,compare,NULL);
        }
        if(operation_list[i].op==RANGE)
        {
            operation_list[i].preread_list = avl_print(avl_root,contain1,operation_list[i].range_condition);
        }
    }
    int id = 0;
    for(int i = 0; i < operation_list_count; i++)
    {
        operation_list[i].id = id;
        if(operation_list[i].op==RANGE)
        {
            rangeop_list[rangeop_list_count].id = id;
            rangeop_list[rangeop_list_count].id_num = operation_list[i].preread_list->num;
            rangeop_list_count++;
            for(int j = 0; j < operation_list[i].preread_list->num)
            {
                keyop_list[keyop_list_count].id = id++;
                keyop_list[keyop_list_count].key = *((key_t *)operation_list[i].preread_list->data[i]);
                keyop_list[keyop_list_count].op = READ;
                keyop_list_count++;
            }
        }
        else
        {
            keyop_list[keyop_list_count].id = id++;
            keyop_list[keyop_list_count].key = operation_list[i].key;
            keyop_list[keyop_list_count].op = operation_list[i].op;
            keyop_list[keyop_list_count].update = operation_list[i].update;
            keyop_list[keyop_list_count].value = operation_list[i].value;
            keyop_list[keyop_list_count].vsize = operation_list[i].vsize;
            keyop_list_count++;
        }
    }
    avl_destroy(&avl_root);
}
/**
 *thread-operation--get the leaf
 */
void get_target(int start_index,int step,node_pointer root)
{
    while(start_index < keyop_list_count)
    {
        targets[start_index] = get_leaf(root,keyop_list[start_index].key);
        start_index += step;
    }
}
static void * construct1(void *newdata,size_t data_size)
{
    node_pointer * n = (node_pointer *)newdata;
    struct nodeop * tmp;
    if((tmp=(struct nodeop *)malloc(sizeof(struct nodeop)))==NULL)
    {
        perror("construct1 malloc failed");
        exit(-1);
    }
    tmp->num = 0;
    tmp->n = *n;
    tmp->op_list = NULL;
    return tmp;
}
static int compare1(void *data,void *newdata)
{
    node_pointer *b = (node_pointer *)newdata;
    struct nodeop * tmp = (struct nodeop *)data;
    node_pointer a = tmp->n;
    if(TOID_EQUALS(a,*b)) return 0;
    if(a.oid.pool_uuid_lo > (*b).oid.pool_uuid_lo || (a.oid.pool_uuid_lo == (*b).oid.pool_uuid_lo &&a.oid.off > (*b).oid.off)) return 1;
    return -1;
}
static void update1(void *data, void *newdata)
{
    struct nodeop * tmp = (struct nodeop *)data;
    tmp->num++;
}
static void update2(struct nodeop * tmp,keyop p)
{
    if(tmp->op_list==NULL)
    {
        if((tmp->op_list=(keyop *)malloc(sizeof(keyop)*tmp->num))==NULL)
        {
            perror("nodeop update malloc failed");
            exit(-1);
        }
        tmp->num = 0;
    }
    tmp->op_list[tmp->num++] = p;
}
void clear_modified_list()
{
    for(int i = 0; i < modified_list_count; i++)
    {
        free(modified_list_count[i].op_list);
    }
    modified_list_count = 0;
}

void redistribute_key()
{
    avl_pointer avl_root;
    int unbalanced = FALSE;
    avl_init(avl_root,&unbalanced);
    for(int i = 0; i < keyop_list_count; i++)
    {
        avl_insert(avl_root,&targets[i],sizeof(node_pointer),&unbalanced,construct1,compare1,update1);
    }
    for(int i = 0; i < keyop_list_count; i++)
    {
        update2(avl_read(*avl_root,&targets[i],compare1),keyop_list[i]);
    }
    struct data_list * result = avl_print(*avl_root,contain,NULL);
    for(int i = 0; i < result->num; i++)
    {
        modified_list[i] = *((struct nodeop*)(result->data[i]));
    }
    modified_list_count = result->num;
    free(result);
    avl_destroy(&avl_root);
}
/**
  *pthread_operation
  */
void process_node(int start_index,int step,node_pointer * root)
{
    while(start_index < modified_list_count)
    {
        common_operation(&modified_list[i],root);
        start_index += step;
    }
}
static void update_record(void *data,void *newdata)
{
    struct record *a,*b;
    a = (struct record *)data;
    b = (struct record *)newdata;
    if(b->id > a->id)
    {
        a->id = b->id;
    }
}
static void* construct_record(void *newdata,size_t data_size)
{
    void * tmp;
    if((tmp=(void *)malloc(data_size))==NULL)
    {
        perror("construct record malloc failed");
        exit(-1);
    }
    memcpy(tmp,newdata,data_size);
    return tmp;
}
static int compare_record(void *data,void *newdata)
{
    struct record * a,*b;
    a = (struct record *)data;
    b = (struct record *)newdata;
    if(a->key==b->key) return 0;
    if(a->key > b->key) return 1;
    return -1;
}
/**
  * build insert record avl tree
  */
static avl_pointer build_record_avl()
{
    avl_pointer avl_root;
    int unbalanced;
    avl_init(&avl_root,&unbalanced);
    for(int i = 0; i < record__list_count; i++)
    {
        avl_insert(&avl_root,&record_list[i],sizeof(struct record),&unbalanced,construct_record,compare_record,update_record);
    }
    return avl_root;
}
static int compare3(const void *data,const void *newdata)
{
    const kvpair *a = (const struct kvpair *)data;
    const kvpair *b = (const struct kvpair *)newdata;
    if(a->id < b->id) return -1;
    if(a->id == b->id) return 0;
    return 1;
}
static void reorder_readrecord()
{
    /** sort read record by id*/
    qsort(read_record,sizeof(kvpair),sizeof(kvpair)*read_record_count,compare3);
}
static int compare4(const void *data,const void *newdata)
{
    const int * id = (const int *)data;
    const kvpair * t = (const kvpair *)newdata;
    if(*id < t->id) return -1;
    if(*id == t->id) return 0;
    return 1;
}
static int compare5(void *data,void *newdata)
{
    struct record * a;
    key_t * b;
    a = (struct record *)data;
    b = (key_t *)b;
    if(a->key==*b) return 0;
    if(a->key < *b) return -1;
    return 1;
}
static void range_result_count(int start_index,avl_pointer avl_root,node_pointer leaf_head)
{
    struct data_list * tmp;
    if((tmp=(struct data_list *)malloc(sizeof(struct data_list)))==NULL)
    {
        perror("range result count malloc failed");
        exit(-1);
    }
    tmp->num = rangeop_list[start_index].id_num;
    node_pointer c = leaf_head;
    while(!TOID_IS_NULL(c))
    {
        struct record * t;
        for(int i = 0; i < D_RO(c)->num_keys; i++)
        {
            t = (struct record *)avl_read(avl_root,&(D_RO(c)->keys[i]),compare5);
            if(t==NULL || t->id < rangeop_list[start_index].id)
            {
                tmp->num++;
            }
        }
    }
    if((tmp->data=(void *)malloc(sizeof(void *)*tmp->num))==NULL)
    {
        perror("range result count tmp_data malloc failed");
        exit(-1);
    }
    tmp->num = 0;
    rangeop_list[start_index].result = tmp;
}
static void range_operation(int start_index,int step,avl_pointer avl_root,node_pointer leaf_head)
{
    while(start_index < rangeop_list_count)
    {
        kvpair * read_buffer = (kvpair *)bsearch(rangeop_list[start_index].id,rangeop_list,sizeof(kvpair),sizeof(kvpair)*rangeop_list_count,compare4);
        node_pointer c = leaf_head;
        int i = 0,j = 0;
        kvpair * tmp;
        struct record * t;
        while(!TOID_IS_NULL(c))
        {
            j = 0;
            while(j < D_RO(c)->num_keys)
            {
                while(i < rangeop_list[i].id_num && read_buffer[i].key < D_RO(c)->keys[j])
                {
                    if((tmp=(kvpair *)malloc(sizeof(kvpair)))==NULL)
                    {
                        perror("range operation tmp malloc failed");
                        exit(-1);
                    }
                    memcpy(tmp,&read_buffer[i],sizeof(kvpair));
                    rangeop_list[start_index].result->data[rangeop_list[start_index].result->num++] = (void *)tmp;
                    i++;
                }
                t =avl_read(avl_root,&(D_RO(c)->keys[j]),compare5);
                if(t == NULL || t->id < rangeop_list[start_index].id)
                {
                    if((tmp=(kvpair *)malloc(sizeof(kvpair)))==NULL)
                    {
                        perror("range operation tmp malloc failed");
                        exit(-1);
                    }
                    tmp->key = D_RO(c)->keys[j];
                    tmp->vsize = D_RO(D_RO(c)->arg)->vsizes[j];
                    if((tmp->value=(void *)malloc(sizeof(void)*tmp->vsize))==NULL)
                    {
                        perror("range operation tmp value malloc failed");
                        exit(-1);
                    }
                    memcpy(tmp->value,pmemobj_direct(D_RO(c)->pointers[j]),tmp->vsize);
                    rangeop_list[start_index].result->data[rangeop_list[start_index].result->num++] = (void *)tmp;

                }
                j++;
            }
            TOID_ASSIGN(c,D_RO(c)->pointers[BTREE_ORDER]);
        }
        start_index += step;
    }
}
static void delete_leaves(node_pointer leaf_head)
{
    node_pointer c = leaf_head;
    node_pointer n;
    node_pointer pre = TOID_NULL(struct tree_node);
    keyop_list_count = 0;
    while(!TOID_IS_NULL(c))
    {
        if(D_RO(D_RO(c)->arg)->is_deleted)
        {
            for(int i = 0; i < D_RO(c)->num_keys; i++)
            {
                keyop_list[keyop_list_count].op = INSERT;
                keyop_list[keyop_list_count].key = D_RO(c)->keys[i];
                keyop_list[keyop_list_count].vsize = D_RO(D_RO(c)->arg)->vsizes[i];
                void * tmp;
                if(tmp==(void *)malloc(keyop_list[keyop_list_count].vsize)==NULL){
                    perror("delete leaves malloc failed");
                    exit(-1);
                }memcpy(tmp,pmemobj_direct(D_RO(c)->pointers[i]),keyop_list[keyop_list_count].vsize);
                keyop_list[keyop_list_count].value = tmp;
                pmemobj_tx_free(D_RO(c)->pointers[i]);
            }n = c;
            TOID_ASSIGN(c,D_RO(c)->pointers[BTREE_ORDER]);
            TX_FREE(n);
        }else{
            if(!TOID_IS_NULL(pre)){
                TX_SET(pre,pointers[BTREE_ORDER],c.oid);
                pre = c;
            }
            TOID_ASSIGN(c,D_RO(c)->pointers[BTREE_ORDER]);
        }
    }if(!TOID_IS_NULL(pre)){
        TX_SET(pre,pointers[BTREE_ORDER],OID_NULL);
    }
}
/**
  */
void palm_process(){

}


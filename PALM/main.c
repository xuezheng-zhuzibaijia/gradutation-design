#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "palm.h"
PMEMobjpool * get_pool()
{
    PMEMobjpool * pop = pmemobj_open("testfile",POBJ_LAYOUT_NAME(test));
    if(pop==NULL) {
        pop = pmemobj_create("testfile",POBJ_LAYOUT_NAME(test),PMEMOBJ_MIN_POOL,0666);
        if(pop==NULL) {
            perror("pop create failed");
            exit(-1);
        }
    }
    return pop;
}

void print_originop()
{
    for(int i=0; i < origin_op_count; i++) {
        if(origin_op_tasks[i].op==RANGE) {
            printf("range_id %d range_num %d|",origin_op_tasks[i].range_id,origin_op_tasks[i].range_num);
        }
    }
    printf("\nsingleop count %d\n",singleop_count);
    for(int i = 0; i < singleop_count; i++) {
        switch(single_tasks[i].op) {
        case READ:
            printf("R:");
            break;
        case INSERT:
            printf("I:");
            break;
        case DELETE:
            printf("D:");
            break;
        case UPDATE:
            printf("U:");
            break;
        default:
            break;
        }
        printf("id-%d key-%d|",single_tasks[i].id,single_tasks[i].key);
    }
}
void preprocess_test(int start,int num)
{
    set_now_single();
    origin_op_count = num;
    for(int i = 0;i < origin_op_count;i++){
        origin_op_tasks[i].op =  INSERT;
        origin_op_tasks[i].key = i+start;//(i % 2 == 0)?(2*i):(i*-1);
        int * t = (int *)malloc(sizeof(int));
        *t = i;
        origin_op_tasks[i].v = (void *)t;
        origin_op_tasks[i].vsize = sizeof(int);
//        origin_op_tasks[i].op =  DELETE;
//        origin_op_tasks[i].key = i;
    }
}
/**
 This function is the palm function,which would execute most of the process
*/
void palm(PMEMobjpool *pop,TOID(struct tree) btree){
    node_pointer root = D_RO(btree)->root;
    TX_BEGIN(pop) {
        preprocess();
        //print_originop();
        palm_process(&root);
        TX_SET(btree,root,root);
    }
    TX_END
}
/**
  This is a function for test.origin_op_tasks is a global array,
  which you should put the operation in,and set origin_op_count the num of operations
*/
void palm_test(){
    set_now_single();
    //read(1)
    origin_op_tasks[0].op = READ;
    origin_op_tasks[0].key = 1;
    //insert(1,1)
    origin_op_tasks[1].op = INSERT;
    origin_op_tasks[1].key = 1;
    int * t;
    t = malloc(sizeof(int));
    *t = 1;
    origin_op_tasks[1].v = (void*)t;
    origin_op_tasks[1].vsize = sizeof(int);
    //read(1)
    origin_op_tasks[2].op = READ;
    origin_op_tasks[2].key = 1;
    //update(1,10)
    origin_op_tasks[3].op = UPDATE;
    origin_op_tasks[3].key = 1;
    t = malloc(sizeof(int));
    *t = 10;
    origin_op_tasks[3].v = (void*)t;
    origin_op_tasks[3].vsize = sizeof(int);
    //read(1)
    origin_op_tasks[4].op = READ;
    origin_op_tasks[4].key = 1;
    //insert 0-10
    for(int i = 5;i <= 15;i++){
        origin_op_tasks[i].op = INSERT;
        origin_op_tasks[i].key = i - 5;
        t = malloc(sizeof(int));
        *t = i;
        origin_op_tasks[i].v = (void*)t;
        origin_op_tasks[i].vsize = sizeof(int);
    }//range(0,10)
    origin_op_tasks[16].op = RANGE;
    origin_op_tasks[16].m = 0;
    origin_op_tasks[16].n = 10;
    //delete 0-10
    for(int i = 17;i <= 27;i++){
        origin_op_tasks[i].op = DELETE;
        origin_op_tasks[i].key = i - 17;
    }//range(0,10)
     origin_op_tasks[28].op = RANGE;
     origin_op_tasks[28].m = 0;
     origin_op_tasks[28].n = 10;
     origin_op_count = 29;
}
int main(int argc,char**argv)
{
    PMEMobjpool * pop = get_pool();
    TOID(struct tree) btree = POBJ_ROOT(pop,struct tree);
    /*int count = atoi(argv[1]);
    int base = 0;
    while(count-base > 80){
        preprocess_test(base,80);
        palm(pop,btree);
        base += 80;
    }if(count > base){
        preprocess_test(base,count-base);
        palm(pop,btree);
    }*/
    palm_test();
    palm(pop,btree);
//    print_btree(D_RO(btree)->root);
    pmemobj_close(pop);
    return 0;
}

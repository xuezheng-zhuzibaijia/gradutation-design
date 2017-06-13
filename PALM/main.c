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
int main(int argc,char**argv)
{
    int count = atoi(argv[1]);
    PMEMobjpool * pop = get_pool();
    TOID(struct tree) btree = POBJ_ROOT(pop,struct tree);
    int base = 0;
    while(count-base > 80){
        preprocess_test(base,80);
        palm(pop,btree);
        base += 80;
    }if(count > base){
        preprocess_test(base,count-base);
        palm(pop,btree);
    }
    pmemobj_close(pop);
    return 0;
}

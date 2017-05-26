#include <stdio.h>
#include <stdlib.h>
#include "palm.h"
key_t test_keys[10] = {10,100,-8,27,6,3,9,73,90,91};
int values[10] = {1,2,3,4,5,6,7,8,9,10};
int main(){
    PMEMobjpool * pop = pmemobj_open("testfile",POBJ_LAYOUT_NAME(test));
    if(pop==NULL){
        PMEMobjpool * pop = pmemobj_create("testfile",POBJ_LAYOUT_NAME(test),PMEMOBJ_MIN_POOL,0666);
        btree_init(pop);
    }for(int i = 0;i < 10;i++){
        operation_list[i].op = INSERT;
        operation_list[i].key = test_keys[i];
        operation_list[i].value = (void*)&values[i];
        operation_list[i].vsize = sizeof(int);
    }operation_list_count = 10;
    palm(pop);
    for(int i = 0;i < 10;i++){
        printf("(%d,%d) ",read_record[i].key,*((int *)read_record[i].value));
        free(read_record[i].value);
    }pmemobj_close(pop);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <libpmemobj.h>
#include "skip_list.h"
void read_value(PMEMobjpool *pop,int key)
{
    PMEMoid value = skiplist_find_value(pop,key);
    if(OID_IS_NULL(value))
    {
        printf("NOT FOUND\n");
        return;
    }
    printf("key = %d,value = %d\n",key,*((int *)pmemobj_direct(value)));
}
int main(int argc,char**argv)
{
    char fname[] = "skipfile";
    PMEMobjpool * pop = pmemobj_open(fname,POBJ_LAYOUT_NAME(example));
    if(pop==NULL){
        perror("OPEN FAILED");
        exit(EXIT_FAILURE);
    }
    int read_nums = atoi(argv[1]);
    clock_t start = clock();
    for(int i = 0;i < read_nums;i++){
        read_value(pop,i);
    }
    clock_t end = clock();
    printf("%d,%f\n",read_nums,(double)(end-start)/CLOCKS_PER_SEC);
    pmemobj_close(pop);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <libpmemobj.h>
#include "btree.h"
void get_value(PMEMobjpool *pop,int key)
{
    PMEMoid value = find_value(pop,key);
    if(OID_IS_NULL(value))
    {
        printf("NOT FOUND");
	return;
    }
    printf("key %d value %d\n",key,*((int *)pmemobj_direct(value)));
}
int main(int argc,char**argv)
{
    if(argc!=2)
    {
        perror("read program needs its key,please input one!\n");
        exit(EXIT_FAILURE);
    }
    PMEMobjpool * pop = pmemobj_open("treefile",POBJ_LAYOUT_NAME(example));
    if(pop==NULL)
    {
	exit(EXIT_FAILURE);
    }
    //print_tree(pop);
    int key = atoi(argv[1]);
    clock_t start = clock();
    for(int i = 0;i < key;i++){
       get_value(pop,i);
    }
    clock_t end = clock();
    printf("%d,%f",key,(double)(end-start)/CLOCKS_PER_SEC);
    pmemobj_close(pop);
    return 0;
}

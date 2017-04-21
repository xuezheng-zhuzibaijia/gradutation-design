#include <stdio.h>
#include <stdlib.h>
#include <libpmemobj.h>
#include "btree.h"
const int * get_value(PMEMobjpool *pop,int key)
{
    PMEMoid value = find_value(pop,key);
    if(OID_IS_NULL(value))
    {
        return NULL;
    }
    return (int *)pmemobj_direct(value);
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
    }
    //print_tree(pop);
    int key = atoi(argv[1]);
    const int * value = get_value(pop,key);
    if(value==NULL)
    {
        printf("NOT found!\n");
    }
    else
    {
        printf("key is %d,value is %d\n",key,*value);
    }
    pmemobj_close(pop);
    return 0;
}

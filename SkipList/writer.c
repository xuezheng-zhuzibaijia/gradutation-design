#include <stdio.h>
#include <stdlib.h>
#include <libpmemobj.h>
#include "skip_list.h"
int main(int argc,char**argv)
{
    char fname[] = "skipfile";
    PMEMobjpool * pop = pmemobj_open(fname,POBJ_LAYOUT_NAME(example));
    if(pop==NULL)
    {
        pop = pmemobj_create(fname,POBJ_LAYOUT_NAME(example),PMEMOBJ_MIN_POOL,0666);
        if(pop==NULL)
        {
            perror("CREATION FAILED");
            exit(EXIT_FAILURE);
        }
        skiplist_init(pop);

    }
    for(int i = 20; i >= 0; i--)
    {
        int key = (int)(10000.0*rand()/(RAND_MAX+1.0));
        tree_insert(pop,key,(void *)&i,sizeof(int));
    }
    display(pop);
    pmemobj_close(pop);
    if(pop!=NULL)
    {
        pmemobj_close(pop);
    }
    return 0;
}

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

    }int write_times = atoi(argv[1]);
    for(int i = 0; i < write_times; i++)
    {
        skiplist_insert(pop,i,(void*)&i,sizeof(int));
    }
    if(pop!=NULL)
    {
        pmemobj_close(pop);
    }
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <libpmemobj.h>
#include "btree.h"
int main()
{
    PMEMobjpool * pop = pmemobj_open("treefile",POBJ_LAYOUT_NAME(example));
    if(pop==NULL)
    {
        pop = pmemobj_create("treefile",POBJ_LAYOUT_NAME(example),PMEMOBJ_MIN_POOL,0666);
        if(pop==NULL)
        {
            perror("CREATION FAILED");
            exit(EXIT_FAILURE);
        }
    }
    tree_init(pop);
    int a[60]={8, 9, 4, 3, 1, 7, 0, 1, 3, 8, 9, 6, 9, 0, 2, 2, 0, 4, 4, 5, 6, 2, 9, 1, 3, 3, 0, 1, 4, 2, 4, 3, 1, 9, 6, 2, 7, 7, 4, 0, 6, 4, 7, 6, 5, 9, 8, 6, 4, 3, 1, 0, 5, 0, 2, 9, 4, 2, 0, 9};
    for(int i = 0; i < 60; i++)
    {
        tree_insert(pop,i,(void *)&a[i],sizeof(int));
    }
    pmemobj_close(pop);
    return 0;
}

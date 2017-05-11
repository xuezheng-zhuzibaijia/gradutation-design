#include <stdio.h>
#include <stdlib.h>
#include <libpmemobj.h>
#include <time.h>
#include "btree.h"
int main(int argc,char**argv)
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
        tree_init(pop);
    }
    clock_t start = clock();
    int writer_times = atoi(argv[1]);
    for(int i = 0; i <= writer_times; i++)
    {
        tree_insert(pop,i,(void *)&i,sizeof(int));
    }clock_t end = clock();
    printf("%d,%f\n",writer_times,(double)(end-start)/CLOCKS_PER_SEC);
    //display(pop);
    pmemobj_close(pop);
    return 0;
}

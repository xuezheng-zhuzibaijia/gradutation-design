#include <stdio.h>
#include <stdlib.h>
#include <libpmemobj.h>
#include "btree.h"
int main(int argc,char **argv)
{
    PMEMobjpool * pop = pmemobj_open("treefile",POBJ_LAYOUT_NAME(example));
    if(pop==NULL)
    {
            perror("OPEN FAILED");
            exit(EXIT_FAILURE);
    }
    if(argc!=2){
        perror("This program require one argument");
        exit(EXIT_FAILURE);
    }
    tree_delete(pop,atoi(argv[1]));
    pmemobj_close(pop);
    return 0;
}


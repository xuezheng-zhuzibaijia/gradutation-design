#include <stdio.h>
#include <stdlib.h>
#include "btree.h"
int main()
{
    PMEMobjpool * pop = pmemobj_open("treefile",POBJ_LAYOUT_NAME(example));
    if(pop==NULL){
        pop = pmemobj_create("testfile",POBJ_LAYOUT_NAME(example),PMEMOBJ_MIN_POOL,0666);
        if(pop==NULL){
            perror("CREATION FAILED");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}

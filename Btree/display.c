#include <stdio.h>
#include <stdlib.h>
#include <libpmemobj.h>
#include "btree.h"
int main()
{
    PMEMobjpool * pop = pmemobj_open("treefile",POBJ_LAYOUT_NAME(example));
    if(pop==NULL)
    {
            perror("OPEN FAILED");
            exit(EXIT_FAILURE);
    }
    display(pop);
    pmemobj_close(pop);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <libpmemobj.h>
#include "skip_list.h"
int main(int argc,char**argv)
{
    char fname[] = "skipfile";
    PMEMobjpool * pop = pmemobj_open(fname,POBJ_LAYOUT_NAME(example));
    if(pop==NULL){
        perror("OPEN FAILED");
        exit(EXIT_FAILURE);
    }
    printf_skiplist(pop);
    pmemobj_close(pop);
    return 0;
}

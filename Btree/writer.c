#include <stdio.h>
#include <stdlib.h>
#include <libpmemobj.h>
#include "btree.h"
int main()
{
    int a[60]={8, 9, 4, 3, 1, 7, 0, 1, 3, 8, 9, 6, 9, 0, 2, 2, 0, 4, 4, 5, 6, 2, 9, 1, 3, 3, 0, 1, 4, 2, 4, 3, 1, 9, 6, 2, 7, 7, 4, 0, 6, 4, 7, 6, 5, 9, 8, 6, 4, 3, 1, 0, 5, 0, 2, 9, 4, 2, 0, 9};
    for(int i = 0; i < 60; i++)
    {
        tree_insert("treefile",i,(void *)&a[i],sizeof(int));
    }
    return 0;
}

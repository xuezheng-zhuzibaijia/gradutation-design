#include <stdio.h>
#include <stdlib.h>
#include <libpmemobj.h>
#include "btree.h"
int print_node(node_pointer n)
{
    node_pointer temp = TOID_NULL(struct tree_node);
    if(!D_RO(n)->is_leaf)
    {
        printf("node has %d childs\n",D_RO(n)->num_keys+1);
        for(int i = 0; i <= D_RO(n)->num_keys; i++)
        {
            TOID_ASSIGN(temp,D_RO(n)->pointers[i]);
            print_node(temp);
        }
    }
    else
    {
        printf("leaf has %d keys\n",D_RO(n)->num_keys);
        for(int i = 0; i < D_RO(n)->num_keys; i++)
        {
            printf("(%d,%d) ",D_RO(n)->keys[i],*((int *)pmemobj_direct(D_RO(n)->pointers[i])));
        }
        printf("\n");
    }
    return 1;
}
void print_tree(PMEMobjpool *pop)
{
    TOID(struct tree) t = POBJ_ROOT(pop,struct tree);
    print_node(D_RO(t)->root);
}
int main()
{
    PMEMobjpool * pop = pmemobj_open("treefile",POBJ_LAYOUT_NAME(example));
    if(pop==NULL)
    {
            perror("CREATION FAILED");
            exit(EXIT_FAILURE);
    }
    print_tree(pop);
    pmemobj_close(pop);
    return 0;
}


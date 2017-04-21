#include <stdio.h>
#include <stdlib.h>
#include <libpmemobj.h>
#include "btree.h"
void print_node(node_pointer n){
    printf("the node has");
    if(D_RO(n)->is_leaf){
        printf(" %d values\n",D_RO(n)->num_keys);
    }else{
        printf(" %d childs\n",D_RO(n)->num_keys+1);
    }
    for(int i = 0;i < D_RO(n)->num_keys;i++){
        printf(" %d",D_RO(n)->keys[i]);
        if(D_RO(n)->is_leaf){
            printf(",%d",*((int *)pmemobj_direct(D_RO(n)->pointers[i])));
        }
    }printf("\n");
    if(!D_RO(n)->is_leaf){
        for(int i = 0;i <= D_RO(n)->num_keys;i++){
            node_pointer temp;
            TOID_ASSIGN(temp,D_RO(n)->pointers[i]);
            print_node(temp);
        }
    }
}
void print_tree(PMEMobjpool *pop){
    TOID(struct tree) t = POBJ_ROOT(pop,struct tree);
    print_node(D_RO(t)->root);
}
const int * get_value(PMEMobjpool *pop,int key){
    PMEMoid value = find_value(pop,key);
    if(OID_IS_NULL(value)){
        return NULL;
    }return (int *)pmemobj_direct(value);
}
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
    //print_tree(pop);
    int key = 1;
    const int * value = get_value(pop,key);
    if(value==NULL){
        printf("NOT found!\n");
    }else{
        printf("key is %d,value is %d\n",key,*value);
    }
    pmemobj_close(pop);
    return 0;
}

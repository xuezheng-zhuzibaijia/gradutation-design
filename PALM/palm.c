#include <stdio.h>
#include <stdlib.h>
#include "palm.h"
/**************preprocess*************/
void palm_init(){
        range_set_count = 0;
        modified_count = 0;
        read_result_count = 0;
        op_result_count = 0;
}
static int btree_search(node_pointer n,int key)
{

    int left = 0,right = D_RO(n)->num_keys - 1;
    while(left <= right)
    {
        int mid = (left + right)/2;
        if(D_RO(n)->keys[mid] > key)
        {
            right =  mid - 1;
        }
        else
        {
            left = mid + 1;
        }
    }
    return left;
}
node_pointer find_leaf(node_pointer root,int key){
        node_pointer temp = root;
        while(!D_RO(temp)->is_leaf){
                TOID_ASSIGN(temp,D_RO(temp)->pointers[btree_search(temp,key)]);
        }return temp;
}
void preprocess(){
        if(operation_lst_count==0){
            perror("OPERATION LIST IS EMPTY");
            exit(-1);
        }
        for(int i = 0 ;i < operation_lst_count;i++){
                if(operation_lst[i].op == RANGE){
                        range_set[range_set_count].interval = operation_lst[i].arg.interval;
                        range_set[range_set_count].kset = NULL;
                        range_set_count++;
                }
        }avl_init();
        for(int i = operation_lst_count-1,j = range_set_count-1;i>=0;i--){
                if(operation_lst[i].op == DEL || operation_lst[i].op == UP){
                        avl_insert(&avl_tree_root,operation_lst[i].arg.key,unbalanced);
                }else if(operation_lst[i].op == RANGE){
                        range_set[j].kset = get_interval_keyset(range_set[j].interval.i,range_set[j].interval.j);
                        j--;
                }
        }for(int i = 0,j = 1,k = 0;i < operation_lst_count;i++){
                if(operation_lst[i].optype==RANGE&&range_set[k].kset){
                        range_set[k].id_from = j;
                        j = j + range_set[k].kset->num;
                        k++;
                }else if(operation_lst[i].optype!=RANGE){
                        operation_lst[i].id = j++;
                }
        }for(int i=0;i < range_set_count;i++){
                if(range_set[i].kset){
                        for(int j = 0;j < range_set[i].kset->num;j++){
                                operation_lst[operation_lst_count].optype = RD;
                                operation_lst[operation_lst_count].arg.key = range_set[i].kset->key_list[j];
                                operation_lst[operation_lst_count++].id = range_set[i].id_from+j;
                        }
                }
        }
}






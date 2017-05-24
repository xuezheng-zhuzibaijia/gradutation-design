#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "palm.h"
void palm_init(){
    modified_list_count = 0;
    change_record_count = 0;
    read_record_count = 0;
    keyop_list_count = 0;
    rangeop_list_count = 0;
}
static int compare(void * data,void *newdata){
    key_t * a,*b;
    a = (key_t *)data;
    b = (key_t *)newdata;
    if(a < b){
        return -1;
    }if(a == b){
        return 0;
    }return 1;
}
static int contain(void *data,void *c){
    return 1;
}
static void * construct(void *newdata,size_t data_size){
    void * tmp;
    if((tmp=(void *)malloc(sizeof(data_size)))==NULL){
            perror("construct malloc failed!");
            exit(-1);
    }memcpy(tmp,newdata,data_size);
    return tmp;
}
void operation_init{
    avl_pointer avl_root;
    int unbalanced;
    avl_init(&avl_root,&unbalanced);
    for(int i = operation_list_count-1;i>=0;i--){
        if(operation_list[i].op==UPDATE || operation_list[i].op==DELETE){
            avl_insert(&avl_root,&operation_list[i].key,sizeof(key_t),&unbalanced,construct,compare,NULL);
        }if(operation_list[i].op==RANGE){
            operation_list[i].preread_list = avl_print(avl_root,contain,NULL);
        }
    }int id = 0;
    for(int i = 0;i < operation_list_count;i++){
        operation_list[i].id = id;
        if(operation_list[i].op==RANGE){
            rangeop_list[rangeop_list_count].id = id;
            rangeop_list[rangeop_list_count].id_num = operation_list[i].preread_list->num;
            rangeop_list_count++;
            for(int j = 0;j < operation_list[i].preread_list->num){
                keyop_list[keyop_list_count].id = id++;
                keyop_list[keyop_list_count].key = *((key_t *)operation_list[i].preread_list->data[i]);
                keyop_list[keyop_list_count].op = READ;
                keyop_list_count++;
            }
        }else{
            keyop_list[keyop_list_count].id = id++;
            keyop_list[keyop_list_count].key = operation_list[i].key;
            keyop_list[keyop_list_count].op = operation_list[i].op;
            keyop_list[keyop_list_count].update = operation_list[i].update;
            keyop_list[keyop_list_count].value = operation_list[i].value;
            keyop_list[keyop_list_count].vsize = operation_list[i].vsize;
            keyop_list_count++;
        }
    }avl_destroy(&avl_root);
}
/**
 *thread-operation--get the leaf
 */
void get_target(int start_index,int step,node_pointer root){
    while(start_index < keyop_list_count){
        targets[start_index] = get_leaf(root,keyop_list[start_index].key);
        start_index += step;
    }
}



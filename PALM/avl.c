#include <stdio.h>
#include <stdlib.h>
#include "avl.h"
avl_pointer make_avlnode(){
    avl_pointer p = (avl_pointer)malloc(sizeof(struct avl_node));
    if(p==NULL){
        perror("avl node malloc failed");
        exit(-1);
    }
    p->bf = 0;
    p->data = 0;
    p->lchild = p->rchild = NULL;
    return p;
}
void left_rotation(avl_pointer *parent,int *unbalanced){
    avl_pointer grand_child, child;
    child = (*parent)->lchild;
    if(child->bf == 1)
    {
        (*parent)->lchild = child->rchild;
        child->rchild = (*parent);
        (*parent)->bf = 0;
        (*parent) = child;
    }
    else
    {
        grand_child = child -> rchild;
        child->rchild = grand_child->lchild;
        grand_child->lchild = child;
        (*parent)->lchild = grand_child->rchild;
        grand_child->rchild = *parent;
        switch(grand_child->bf)
        {
        case 1:
            (*parent)->bf = -1;
            child->bf = 0;
            break;
        case 0:
            (*parent)->bf = child->bf = 0;
            break;
        case -1:
            (*parent)->bf = 0;
            child->bf = 1;
        }*parent = grand_child;
    }(*parent)->bf = 0;
    *unbalanced = 0;
}
void right_rotation(avl_pointer *parent,int *unbalanced){
    avl_pointer grand_child, child;
    child = (*parent)->rchild;
    if(child->bf == -1)
    {
        (*parent)->rchild= child->lchild;
        child->lchild = (*parent);
        (*parent)->bf = 0;
        (*parent) = child;
    }
    else
    {
        grand_child = child -> lchild;
        child->lchild = grand_child->rchild;
        grand_child->rchild = child;
        (*parent)->rchild = grand_child->lchild;
        grand_child->lchild = *parent;
        switch(grand_child->bf)
        {
        case 1:
            (*parent)->bf = 0;
            child->bf = -1;
            break;
        case 0:
            (*parent)->bf = child->bf = 0;
            break;
        case -1:
            (*parent)->bf = 1;
            child->bf = 0;
        }*parent = grand_child;
    }(*parent)->bf = 0;
    *unbalanced = 0;
}
void avl_insert(avl_pointer *parent,int *unbalanced,void *newdata,size_t data_size,cmp cp,construct_func construct,update_func update){
    if(!(*parent)){
        *parent = make_avlnode();
        (*parent)->data = construct(newdata,data_size);
    }else if(cp((*parent)->data,newdata)==1){
        avl_insert(&((*parent)->lchild),unbalanced,newdata,data_size,cp,construct,update);
        if(*unbalanced)
        {
            switch((*parent)->bf)
            {
            case 1:
                left_rotation(parent,unbalanced);
                break;
            case 0:
                (*parent)->bf = 1;
                break;
            case -1:
                (*parent)->bf = 0;
                *unbalanced = FALSE;
                break;
            }
        }
    }else if(cp((*parent)->data,newdata)==-1){
        avl_insert(&((*parent)->rchild),unbalanced,newdata,data_size,cp,construct,update);
        if(*unbalanced)
        {
            switch((*parent)->bf)
            {
            case 1:
                (*parent)->bf = 0;
                *unbalanced = FALSE;
                break;
            case 0:
                (*parent)->bf = -1;
                break;
            case -1:
                right_rotation(parent,unbalanced);
                break;
            }
        }
    }else{
        (*parent)->data = update((*parent)->data,newdata);
        *unbalanced = FALSE;
    }
}
void * avl_read(avl_pointer parent,void *data,cmp cp){
    if(!parent) return NULL;
    if(cp(parent->data,data)==0) return parent->data;
    if(cp(parent->data,data)==1) return avl_read(parent->lchild,data,cp);
    return avl_read(parent->rchild,data,cp);
}
int avl_find(avl_pointer parent,void *data,cmp cp){
    if(!parent) return 0;
    if(cp(parent->data,data)==0) return 1;
    if(cp(parent->data,data)==1) return avl_find(parent->lchild,data,cp);
    return avl_find(parent->rchild,data,cp);
}
void avl_destory(avl_pointer *parent,void (*destroy)(void *data)){
    if(*parent){
        avl_destory(&((*parent)->lchild),destroy);
        avl_destory(&((*parent)->rchild),destroy);
        if(destroy){
            destroy((*parent)->data);
        }
        free(*parent);
        *parent = NULL;

    }
}


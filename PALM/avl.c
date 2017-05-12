#include <stdio.h>
#include <stdlib.h>
#include "palm.h"
/**
 * avl.c是用于支持PALM树预处理的数据结构以及相关操作
 *
 */
void avl_init(){
    unbalanced = FALSE;
    avl_tree_root = NULL;
}
static void left_rotation(avl_pointer * parent,int *unbalanced)
{
    avl_pointer grand_child, child;
    child = (*parent)->left_child;
    if(child->bf == 1)
    {
        /*LL rotation*/
        (*parent)->left_child = child->right_child;
        child->right_child = (*parent);
        (*parent)->bf = 0;
        (*parent) = child;
    }
    else
    {
        /*LR rotation*/
        grand_child = child -> right_child;
        child->right_child = grand_child->left_child;
        grand_child->left_child = child;
        (*parent)->left_child = grand_child->right_child;
        grand_child->right_child = *parent;
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

static void right_rotation(avl_pointer * parent,int *unbalanced)
{
    avl_pointer grand_child, child;
    child = (*parent)->right_child;
    if(child->bf == -1)
    {
        /*RR rotation*/
        (*parent)->right_child= child->left_child;
        child->left_child = (*parent);
        (*parent)->bf = 0;
        (*parent) = child;
    }
    else
    {
        /*RL rotation*/
        grand_child = child -> left_child;
        child->left_child = grand_child->right_child;
        grand_child->right_child = child;
        (*parent)->right_child = grand_child->left_child;
        grand_child->left_child = *parent;
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
void avl_insert(avl_pointer *parent,int key,int *unbalanced){
     if(!(*parent)){
            *unbalanced = TRUE;
            *parent = (avl_pointer)malloc(sizeof(struct avl_node));
            if((*parent)==NULL){
                perror("AVL MALLOC ERROR");
                exit(-1);
            }
            (*parent)->bf = 0;
            (*parent)->left_child = (*parent)->right_child = NULL;
            (*parent)->key = key;
     }else if(key < (*parent)->key){
            avl_insert(&((*parent)->left_child),key,unbalanced);
            if(*unbalanced){
                switch((*parent)->bf){
                    case 1:left_rotation(parent,unbalanced);break;
                    case 0:(*parent)->bf = 1;break;
                    case -1:(*parent)->bf = 0;*unbalanced = FALSE;break;
                }
            }
     }else if(key > (*parent)->key){
          avl_insert(&((*parent)->right_child),key,unbalanced);
          if(*unbalanced){
                    switch((*parent)->bf){
                        case 1:(*parent)->bf = 0;*unbalanced = FALSE;break;
                        case 0:(*parent)->bf = 1;break;
                        case -1:right_rotation(parent,unbalanced);break;
                    }
          }
     }else{
            *unbalanced = FALSE;
     }
}

void avl_destroy(avl_pointer * parent){
    if(!(*parent)) return;
    avl_destroy(&((*parent)->left_child));
    avl_destroy(&((*parent)->right_child));
    free(*parent);
    *parent = NULL;
}
static int get_interval_keysum(avl_pointer parent,int i,int j){
    if(parent==NULL) return 0;
    int tmp = parent->key <= j && parent->key >= i;
    if(i < parent->key){ tmp += get_interval_keysum(parent->left_child,i,j);}
    if(parent->key < j){ tmp += get_interval_keysum(parent->right_child,i,j);}
    return tmp;
}
void static collect_interval(struct key_set * result,avl_pointer parent,int i,int j){
    if(parent==NULL) return;
    if(i < parent->key){ collect_interval(result,parent->left_child,i,j);}
    if(parent->key <= j && parent->key >= i){
        result->key_list[result->num++] = parent->key;
    }if(parent->key < j){ collect_interval(result,parent->right_child,i,j);}
}
struct key_set * get_interval_keyset(int i,int j){
    int sum = get_interval_keysum(avl_tree_root,i,j);
    if(sum == 0){ return NULL;}
    struct key_set * result = (struct key_set *)malloc(sizeof(struct key_set));
    if(result==NULL){
        perror("KEY SET MALLOC ERROR");
        exit(-1);
    }
    int * key_list = (int *)malloc(sizeof(int)*sum);
    if(key_list==NULL){
         perror("KEY LIST MALLOC ERROR");
         exit(-1);
    }
    result -> num = 0;
    result->key_list = key_list;
    collect_interval(result,avl_tree_root,i,j);
    return result;
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "avl.h"
/**
 * avl.c是用于支持PALM树预处理的数据结构以及相关操作
 *
 */

void avl_init(avl_pointer *avl_root,int *unbalanced)
{
        *avl_root = NULL;
        *unbalanced = FALSE;
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
/**
 *                            { -1  a  <  b
 *compare(a,b) = -{   0  a == b
 *                            {   1  a  >  b
 */
void avl_insert(avl_pointer *parent,void *newdata,size_t data_size,int *unbalanced,void* (*construct)(void *newdata,size_t data_size),
int (*compare)(void *data,void *newdata),void (*update)(void *data,void *newdata))
{
    if(!(*parent))
    {
        *unbalanced = TRUE;
        *parent = (avl_pointer)malloc(sizeof(struct avl_node));
        if((*parent)==NULL)
        {
            perror("AVL NODE MALLOC FAILED");
            exit(-1);
        }
        (*parent)->bf = 0;
        (*parent)->left_child = (*parent)->right_child = NULL;
        (*parent)->data = construct(newdata,data_size);
    }
    else if(compare((*parent)->data,newdata)==1) //node value larger than newdata
    {
        avl_insert(&((*parent)->left_child),newdata,data_size,unbalanced,construct,compare,update);
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
    }
    else if(compare((*parent)->data,newdata)==-1)
    {
        avl_insert(&((*parent)->right_child),newdata,data_size,unbalanced,construct,compare,update);
        if(*unbalanced)
        {
            switch((*parent)->bf)
            {
            case 1:
                (*parent)->bf = 0;
                *unbalanced = FALSE;
                break;
            case 0:
                (*parent)->bf = 1;
                break;
            case -1:
                right_rotation(parent,unbalanced);
                break;
            }
        }
    }
    else
    {
        if(update!=NULL)
        {
            update((*parent)->data,newdata);
        }
        *unbalanced = FALSE;
    }
}

void avl_destroy(avl_pointer * parent)
{
    if(!(*parent)) return;
    avl_destroy(&((*parent)->left_child));
    avl_destroy(&((*parent)->right_child));
    free((*parent)->data);
    free(*parent);
    *parent = NULL;
}
/**
   *avl_nodenum(interval)--get the num of nodes data of which satisfied function 'contain'
   */
static int avl_nodenum(avl_pointer parent,contain_func contain,void *c)
{
    if(parent==NULL) return 0;
    return contain(parent->data,c) +avl_nodenum(parent->left_child,contain,c) + avl_nodenum(parent->right_child,contain,c);
}
void static collect_interval(struct data_list * result,avl_pointer parent,contain_func contain,void *c)
{
    if(parent==NULL) return;
    collect_interval(result,parent->left_child,contain,c);
    if(contain(parent->data,c)){
            result->data[result->num++] = parent->data;
    }
    collect_interval(result,parent->right_child,contain,c);
}

struct data_list * avl_print(avl_pointer avl_root,contain_func contain,void *c)
{
    int num = avl_nodenum(avl_root,contain,c);
    if(num == 0)  //no node satisfied the 'contain' function
    {
        return NULL;
    }
    struct data_list * result = (struct data_list *)malloc(sizeof(struct data_list));
    if(result==NULL)
    {
        perror("DATA LIST MALLOC FAILED");
        exit(-1);
    }
    void ** temp = (void **)malloc(sizeof(void*)*num);
    if(temp==NULL)
    {
        perror("DATA MALLOC FAILED");
        exit(-1);
    }
    result -> num = 0;
    result->data = temp;
    collect_interval(result,avl_root,contain,c);
    return result;
}
void * data avl_read(avl_pointer avl_root,void *newdata,int (*compare)(void* data,void *newdata))
{
    if(avl_root==NULL){
        return NULL;
    }void * result = NULL;
    switch(compare(avl_root->data,newdata)){
        case 0:
            result = avl_root->data;
            break;
        case 1:
            result = avl_read(avl_root->left_child,newdata,compare);
            break;
        case -1:
            result = avl_read(avl_root->right_child,newdata,compare);
            break;
        default:
            break;
    }return result;
}

#ifndef AVL_H_INCLUDED
#define AVL_H_INCLUDED
#include "basetype.h"
typedef struct avl_node * avl_pointer;
struct avl_node
{
    void * data;
    short int bf;
    avl_pointer left_child,right_child;
};
struct data_list{
        int num;
        void ** data;
};
typedef int (*contain_func)(void *data,void * c);
void avl_init(avl_pointer * avl_root,int *unbalanced);
void avl_destroy(avl_pointer * avl_root);
void avl_insert(avl_pointer *parent,void *newdata,size_t data_size,int *unbalanced,
    void* (*construct)(void *newdata,size_t data_size),
    int (*compare)(void *data,void *newdata),
    void (*update)(void *data,void *newdata));
struct data_list* avl_print(avl_pointer avl_root,contain_func  contain,void * c);
void * avl_read(avl_pointer avl_root,void *newdata,int (*compare)(void* data,void *newdata));
#endif // AVL_H_INCLUDED

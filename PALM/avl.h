#ifndef AVL_H_INCLUDED
#define AVL_H_INCLUDED

#ifndef TRUE
#define TRUE 1
#endif // TRUE
#ifndef FALSE
#define FALSE 0
#endif // FALSE

typedef struct avl_node * avl_pointer;
typedef int (*cmp)(void *a,void *b);
typedef void * (*construct_func)(void *data,size_t data_size);
typedef void * (*update_func)(void *data,void *newdata);
struct avl_node{
    avl_pointer lchild,rchild;
    short int bf;
    void * data;
};
avl_pointer make_avlnode();
void left_rotation(avl_pointer*parent,int *unbalanced);
void right_rotation(avl_pointer*parent,int *unbalanced);
void avl_insert(avl_pointer *parent,int *unbalanced,void *newdata,size_t data_size,cmp cp,construct_func construct,update_func update);
void * avl_read(avl_pointer parent,void *data,cmp cp);
void avl_destory(avl_pointer *parent,void (*destroy)(void *data));
int avl_find(avl_pointer parent,void *data,cmp cp);
#endif // AVL_H_INCLUDED

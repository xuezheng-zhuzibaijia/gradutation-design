#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "avl.h"
struct test_struct
{
    int key;
    int value;
};
static int contain(void *data,void *c)
{
    return 1;
}
static int compare(void *data,void *newdata)
{
    struct test_struct * a,*b;
    a  = (struct test_struct *)data;
    b = (struct test_struct *)newdata;
    if (a->key < b->key)
    {
        return -1;
    }
    if(a->key == b->key)
    {
        return 0;
    }
    return 1;
}
static void update(void *data,void *newdata)
{
    struct test_struct * a,*b;
    a  = (struct test_struct *)data;
    b = (struct test_struct *)newdata;
    a->value = b->value;
}
static void usage()
{
    printf("This is a test program for avl tree\n");
}
int main()
{
    avl_pointer avl_root;
    int unbalanced;
    avl_init(&avl_root,&unbalanced);
    usage();
    while(1)
    {
        int i,j;
         struct test_struct t;
        printf("please enter command(i:insert  p:print q:quit):");
        char ch = getchar();
        switch(ch)
        {
        case 'i':

            scanf("%d %d",&i,&j);
            t.key = i;
            t.value = j;
            avl_insert(&avl_root,&t,sizeof(t),&unbalanced,compare,update);
            break;
        case 'p':
            printf("tree is : ");
            struct data_list * result = avl_print(avl_root,contain,NULL);
            if(result==NULL)
            {
                printf("empty!\n");
            }
            else
            {
                for(int i = 0; i < result->num; i++)
                {
                    struct test_struct * temp = (struct test_struct *)(result->data[i]);
                    printf("<%d,%d> ",temp->key,temp->value);
                }
                printf("\n");
            }
            break;
        case 'q':
            avl_destroy(&avl_root);
            return 0;
        default:
            break;
        }
    }
    return 0;
}


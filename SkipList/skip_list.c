#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "skip_list.h"
#define MAX_BUF_SIZE 2000
static snode_pointer make_node(int key,void * value,size_t len)
{
    PMEMoid v = pmemobj_tx_alloc(len,TOID_TYPE_NUM(void));
    TX_MEMCPY(pmemobj_direct(v),value,len);
    snode_pointer np = TX_NEW(struct snode);
    TX_SET(np,key,key);
    TX_SET(np,level,-1);
    TX_SET(np,value,v);
    for(int i = 0; i < SKIP_MAX_HEIGHT; i++)
    {
        TX_SET(np,next[i],TOID_NULL(struct snode));
    }
    return np;
}
void skiplist_init(PMEMobjpool *pop)
{
    sroot_pointer root = POBJ_ROOT(pop,struct sroot);
    TX_BEGIN(pop)
    {
        TX_SET(root,head.level,-1);
        for(int i = 0; i < SKIP_MAX_HEIGHT; i++)
        {
            TX_SET(root,head.next[i],TOID_NULL(struct snode));
        }
    }
    TX_END
}

int skiplist_is_in(PMEMobjpool *pop,int key)
{
    sroot_pointer root = POBJ_ROOT(pop,struct sroot);
    int level = D_RO(root)->head.level;
    if(level < 0)
    {
        return 0;
    }
    snode_pointer temp = D_RO(root)->head.next[level];
    while(level>=0 && (!TOID_IS_NULL(temp)))
    {
        int temp_key = D_RO(temp)->key;
        if(temp_key==key)
        {
            return 1;
        }
        if(temp_key > key)
        {
            return 0;
        }
        if(TOID_IS_NULL(D_RO(temp)->next[level]))
        {
            level--;
        }
        else
        {
            temp = D_RO(temp)->next[level];
        }
    }
    return 0;
}

PMEMoid skiplist_find_value(PMEMobjpool *pop,int key)
{
    sroot_pointer root = POBJ_ROOT(pop,struct sroot);
    int level = D_RO(root)->head.level;
    if(level < 0)
    {
        return OID_NULL;
    }
    snode_pointer temp = D_RO(root)->head.next[level];
    while(level>=0 && (!TOID_IS_NULL(temp)))
    {
        int temp_key = D_RO(temp)->key;
        if(temp_key==key)
        {
            return D_RO(temp)->value;
        }
        if(TOID_IS_NULL(D_RO(temp)->next[level]))
        {
            level--;
            continue;
        }
        if(temp_key < key)
        {
            temp = D_RO(temp)->next[level];
        }
        else
        {
            return OID_NULL;
        }
    }
    return OID_NULL;
}
static void skiplist_remove_level(snode_pointer n,int key,int level)
{
    snode_pointer temp = n;
    for(; D_RO(D_RO(temp)->next[level])->key!=key; temp=D_RO(temp)->next[level]);
    snode_pointer dnode = D_RO(temp)->next[level];
    TX_SET(temp,next[level],D_RO(dnode)->next[level]);
    if(level>0)
    {
        skiplist_remove_level(temp,key,level-1);
    }
    TX_FREE(dnode);
}
void skiplist_remove(PMEMobjpool *pop,int key)
{
    if(!skiplist_is_in(pop,key))
    {
        printf("key %d is not in skip list\n",key);
        return;
    }
    sroot_pointer root = POBJ_ROOT(pop,struct sroot);
    TX_BEGIN(pop)
    {

        int level = D_RO(root)->head.level;
        skiplist_remove_level(D_RO(root)->head.next[level],key,level);
        for(int i = level; i >= 0; i--)
        {
            if(TOID_IS_NULL(D_RO(root)->head.next[level]))
            {
                TX_SET(root,head.level,i-1);
            }
            else
            {
                break;
            }
        }
    }
    TX_END
}

static void skiplist_insert_level(snode_pointer n,int key,snode_pointer inode,int level)
{
    snode_pointer temp = n;
    for(; (!TOID_IS_NULL(D_RO(temp)->next[level]))&&D_RO(D_RO(temp)->next[level])->key<key; temp=D_RO(temp)->next[level]);
    TX_SET(inode,next[level],D_RO(temp)->next[level]);
    TX_SET(temp,next[level],inode);
    if(level > 0)
    {
        skiplist_insert_level(temp,key,inode,level-1);
    }
}
void skiplist_insert(PMEMobjpool *pop,int key,void *value,size_t len)
{
    if(skiplist_is_in(pop,key))
    {
        printf("key %d is already in skip list\n",key);
        return;
    }
    sroot_pointer root = POBJ_ROOT(pop,struct sroot);
    TX_BEGIN(pop)
    {
        int level = D_RO(root)->head.level;
        snode_pointer newnode = make_node(key,value,len);
        srand(time(NULL));
        int new_level = (int)(SKIP_MAX_HEIGHT*1.0*rand()/(RAND_MAX+1.0));
        TX_SET(newnode,level,new_level);
        if(new_level > level)
        {
            TX_SET(root,head.level,new_level);
            for(int i = level+1; i < new_level; i++)
            {
                TX_SET(root,head.next[i],TOID_NULL(struct snode));
            }
        }
        snode_pointer head = TOID_NULL(struct snode);
        TOID_ASSIGN(head,pmemobj_oid(&(D_RO(root)->head)));
        if(TOID_IS_NULL(head))
        {
            perror("oid transform failed\n");
            exit(EXIT_FAILURE);
        }
        skiplist_insert_level(head,key,newnode,new_level);
    }
    TX_END
}
void skiplist_destroy(PMEMobjpool *pop)
{
    sroot_pointer root = POBJ_ROOT(pop,struct sroot);
    TX_BEGIN(pop)
    {
        if(D_RO(root)->head.level < 0)  return;
        for(snode_pointer temp = D_RO(root)->head.next[0]; !TOID_IS_NULL(temp); temp=D_RO(root)->head.next[0])
        {
            D_RW(root)->head.next[0] = D_RO(temp)->next[0];
            TX_FREE(temp);
        }
        TX_SET(root,head.level,-1);
    }
    TX_END
}
void display(PMEMobjpool *pop)
{
    sroot_pointer root = POBJ_ROOT(pop,struct sroot);
    if(D_RO(root)->head.level < 0)
    {
        printf("skip_list is empty\n");
    }
    char buf[MAX_BUF_SIZE];
    char * bufp = buf;
    buf[0] = '\0';
    /****************************/
    /***********start***************/
    int levelcount[SKIP_MAX_HEIGHT];
    memset(levelcount,0,sizeof(int)*SKIP_MAX_HEIGHT);
    int num_count = 0;
    strcat(buf,"digraph structs {\n node[shape=record];ranksep=0.8;\nsplines=false;\n");
    strcat(buf,"node0[shape=record,style=filled,fillcolor = sapphire,label=\"");
    int index;
    for(int i = D_RO(root)->head.level; i>=0; i--)
    {
        bufp += strlen(bufp);
        sprintf(bufp,"<f%d> ",i);
        if(i>0)
        {
            strcat(bufp,"|");
        }
    }
    strcat(bufp,"\"];\n");
    snode_pointer np = D_RO(root)->head.next[0];
    while(!TOID_IS_NULL(np))
    {
        num_count++;
        bufp += strlen(bufp);
        sprintf(bufp,"node%d[shape=record,label=\"",num_count);
        int level = D_RO(np)->level;
        for(int i = level; i>=0; i--)
        {
            bufp += strlen(bufp);
            sprintf(bufp,"{ %d | <f%d> }",D_RO(np)->key,i);
            if(i > 0)
            {
                strcat(bufp,"|");
            }
        }
        strcat(bufp,"\"];\n");
        np = D_RO(np)->next[0];
    }
    bufp += strlen(bufp);
    sprintf(bufp,"node%d[shape = record,style=filled,fillcolor = yellow,label=\"",num_count);
    for(int i = D_RO(root)->head.level; i>=0; i--)
    {
        bufp += strlen(bufp);
        sprintf(bufp,"{<f%d>NULL}",i);
        if(i > 0)
        {
            strcat(bufp,"|");
        }
    }
    strcat(bufp,"\"];\n");
    np = D_RO(root)->head.next[0];
    num_count = 0;
    while(!TOID_IS_NULL(np))
    {
        num_count++;
        int level = D_RO(np)->level;
        for(int i = level; i >= 0; i--)
        {
            bufp += strlen(bufp);
            sprintf(bufp,"node%d:f%d -> node%d;\n",levelcount[i],i,num_count);
            levelcount[i] = num_count;
        }
    }
    for(int i = D_RO(root)->head.level; i>=0; i--)
    {
        bufp += strlen(bufp);
        sprintf(bufp,"node%d:f%d -> node%d;\n",levelcount[i],i,num_count);
    }
    strcat(bufp,"}\n");
    printf("%s",buf);
}


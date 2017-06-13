#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "skip_list.h"
#define MAX_BUF_SIZE 100000
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
    if(level < 0){
        return 0;
    }
    while(level>=0){
        if(TOID_IS_NULL(D_RO(root)->head.next[level]) || D_RO(D_RO(root)->head.next[level])->key > key){
            level--;
        }else{
            break;
        }
    }if(level < 0){
        return 0;
    }
    snode_pointer temp = D_RO(root)->head.next[level];
    while(level>=0)
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
        if(TOID_IS_NULL(D_RO(temp)->next[level]) || D_RO(D_RO(temp)->next[level])->key > key)
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
    for(;level >=0&&(TOID_IS_NULL(D_RO(root)->head.next[level])||(D_RO(D_RO(root)->head.next[level])->key > key));level--);
    if(level < 0)
    {
        return OID_NULL;
    }
    snode_pointer temp = D_RO(root)->head.next[level];
    while(level>=0)
    {
        int temp_key = D_RO(temp)->key;
        if(temp_key==key)
        {
            return D_RO(temp)->value;
        }
        if(TOID_IS_NULL(D_RO(temp)->next[level])||(D_RO(D_RO(temp)->next[level])->key > key))
        {
            level--;
        }
        else
        {
            temp = D_RO(temp)->next[level];
        }
    }
    return OID_NULL;
}
static void skiplist_remove_level(snode_pointer n,int key,int level)
{
    snode_pointer temp = n;
    for(;(!TOID_IS_NULL(D_RO(temp)->next[level]))&&D_RO(D_RO(temp)->next[level])->key<key; temp=D_RO(temp)->next[level]);
    if(TOID_IS_NULL(D_RO(temp)->next[level])){
        skiplist_remove_level(temp,key,level-1);
        return;
    }
    snode_pointer dnode = D_RO(temp)->next[level];
    //printf("dnode key is %d\n",D_RO(dnode)->key);
    TX_SET(temp,next[level],D_RO(dnode)->next[level]);
    if(level>0)
    {
        skiplist_remove_level(temp,key,level-1);
    }else{
        TX_FREE(dnode);
    }
}
void skiplist_remove(PMEMobjpool *pop,int key)
{
    if(!skiplist_is_in(pop,key))
    {
        //printf("key %d is not in skip list\n",key);
        return;
    }//printf("here!\n");
    sroot_pointer root = POBJ_ROOT(pop,struct sroot);
    int level = D_RO(root)->head.level;
    if(level < 0)
    {
        return;
    }
    TX_BEGIN(pop)
    {

        for(int i = level;i>=0;i--){
            if(!TOID_IS_NULL(D_RO(root)->head.next[i])){
                snode_pointer temp = D_RO(root)->head.next[i];
                if(D_RO(temp)->key < key){
                    skiplist_remove_level(temp,key,i);
                    break;
                }else if(D_RO(temp)->key==key){
                    TX_SET(root,head.next[i],D_RO(temp)->next[i]);
                    if(i==0){
                        TX_FREE(temp);
                    }
                }
            }
        }
        level = D_RO(root)->head.level;
        for(int i = level; i >= 0; i--)
        {
            if(TOID_IS_NULL(D_RO(root)->head.next[i]))
            {
                TX_SET(root,head.level,i-1);
            }
            else
            {
                break;
            }
        }
    }TX_END
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
        //printf("key %d is already in skip list\n",key);
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
        }for(int i = new_level;i>=0;i--){
            if(TOID_IS_NULL(D_RO(root)->head.next[i]) || D_RO(D_RO(root)->head.next[i])->key > key){
                TX_SET(newnode,next[i],D_RO(root)->head.next[i]);
                TX_SET(root,head.next[i],newnode);
            }else{
                skiplist_insert_level(D_RO(root)->head.next[i],key,newnode,i);
                break;
            }
        }
    }
    TX_ONCOMMIT{
        //printf("key %d insert successfully!\n",key);
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
        return;
    }
    char *graph = (char *)malloc(MAX_BUF_SIZE);
    graph[0] = '\0';
    int levelcount[SKIP_MAX_HEIGHT];
    for(int i=0;i<=D_RO(root)->head.level;i++){
        levelcount[i] = 0;
    }
    int num_count = 0;
    strcat(graph,"digraph structs {\n node[shape=record];rankdir=LR;ranksep=0.4;\nsplines=false;\n");
    strcat(graph,"node0[shape=record,style=filled,fillcolor=aquamarine,label=\"");
    int index;
    for(int i = D_RO(root)->head.level; i>=0; i--)
    {
        sprintf(graph+strlen(graph),"<f%d> ",i);
        if(i>0)
        {
            strcat(graph,"|");
        }
    }
    strcat(graph,"\"];\n");
    snode_pointer np = D_RO(root)->head.next[0];
    while(!TOID_IS_NULL(np))
    {
        num_count++;
        sprintf(graph+strlen(graph),"node%d[shape=record,label=\"",num_count);
        int level = D_RO(np)->level;
        for(int i = level; i>=0; i--)
        {
            sprintf(graph+strlen(graph),"{ <b%d> %d|<f%d> }",i,D_RO(np)->key,i);
            if(i > 0)
            {
                strcat(graph,"|");
            }
        }
        strcat(graph,"\"];\n");
        np = D_RO(np)->next[0];
    }
    num_count++;
    sprintf(graph+strlen(graph),"node%d[shape = record,style=filled,fillcolor = yellow,label=\"",num_count);
    for(int i = D_RO(root)->head.level; i>=0; i--)
    {
        sprintf(graph+strlen(graph),"{<f%d>NULL}",i);
        if(i > 0)
        {
            strcat(graph,"|");
        }
    }
    strcat(graph,"\"];\n");
    num_count = 0;
    for(np = D_RO(root)->head.next[0];!TOID_IS_NULL(np);np = D_RO(np)->next[0]){
        num_count++;
        int level = D_RO(np)->level;
        for(int i = level; i >= 0; i--)
        {
            int length = strlen(graph);
            sprintf(graph+length,"node%d:f%d -> node%d:b%d;\n",levelcount[i],i,num_count,i);
            levelcount[i] = num_count;
        }
    }
    num_count++;
    for(int i = D_RO(root)->head.level; i>=0; i--)
    {
        sprintf(graph+strlen(graph),"node%d:f%d -> node%d:f%d;\n",levelcount[i],i,num_count,i);
    }
    strcat(graph,"}\n");
    if(strlen(graph) >= MAX_BUF_SIZE){
        perror("The MAX_BUF_SIZE is smaller than required");
        exit(EXIT_FAILURE);
    }
    FILE * fp=fopen("display.dot","w");
    fprintf(fp,"%s",graph);
    fclose(fp);
    free(graph);
    pid_t child = fork();
    if(child < 0){
        perror("create subprocess failed!\n");
        exit(EXIT_FAILURE);
    }if(child==0){
        execlp("dot","dot","-Tpng","display.dot","-o","display.png",(char*)0);
    }wait(NULL);
    child = fork();
    if(child < 0){
        perror("create subprocess failed!\n");
        exit(EXIT_FAILURE);
    }if(child==0){
        execlp("rm","rm","display.dot",(char*)0);
    }wait(NULL);
    child = fork();
    if(child < 0){
        perror("create subprocess failed!\n");
        exit(EXIT_FAILURE);
    }if(child==0){
        execlp("eog","eog","display.png",(char*)0);
    }wait(NULL);
}

static void print_skiplist_level(snode_pointer n,int level){
    printf("level %3d head -> ",level);
    while(!TOID_IS_NULL(n)){
        printf("%3d -> ",D_RO(n)->key);
        n = D_RO(n)->next[level];
    }printf("NULL\n");
}
void printf_skiplist(PMEMobjpool *pop){
    sroot_pointer root = POBJ_ROOT(pop,struct sroot);
    for(int i = D_RO(root)->head.level;i>=0;i--){
        print_skiplist_level(D_RO(root)->head.next[i],i);
    }
}


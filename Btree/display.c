#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpmemobj.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "btree.h"
#define MAX_HEIGHT 15
#define MAX_BUFFER_SIZE 10000
int start_index[MAX_HEIGHT];
int layout_count[MAX_HEIGHT];
void init_start_index(int h)
{
    for(int i = 0; i < h; i++)
    {
        start_index[i] = 0;
    }
}
int get_height(node_pointer root)
{
    int i = 1;
    while(!D_RO(root)->is_leaf)
    {
        i++;
        TOID_ASSIGN(root,D_RO(root)->pointers[0]);
    }
    return i;
}
int get_leaf_num(node_pointer root)
{
    while(!D_RO(root)->is_leaf)
    {
        TOID_ASSIGN(root,D_RO(root)->pointers[0]);
    }
    int i = 0;
    while(!OID_IS_NULL(D_RO(root)->pointers[BTREE_ORDER]))
    {
        TOID_ASSIGN(root,D_RO(root)->pointers[BTREE_ORDER]);
        i++;
    }
    return i;
}
void declare_node(char *buf,node_pointer n,int layout,int leaf_sum)
{
    int index = 0;
    strcat(buf,"node");
    index = strlen(buf);
    sprintf(buf+index,"%d_%d[shape=record, style=filled, ",layout,start_index[layout]);
    if(D_RO(n)->is_leaf){
        strcat(buf,"fillcolor=green, ");
    }else{
        strcat(buf,"fillcolor=yellow, ");
    }
    strcat(buf,"label=\"");
    index = strlen(buf);
    start_index[layout]++;
    if(D_RO(n)->is_leaf)
    {
        for(int i = 0; i < D_RO(n)->num_keys; i++)
        {
            index = strlen(buf);
            sprintf(buf+index,"%d",D_RO(n)->keys[i]);
            if(i<D_RO(n)->num_keys-1)
            {
                strcat(buf,"|");
            }
        }
        strcat(buf,"\"];\n");
    }
    else
    {
        for(int i = 0; i < D_RO(n)->num_keys; i++)
        {
            index = strlen(buf);
            sprintf(buf+index,"<f%d>|%d|",i,D_RO(n)->keys[i]);
        }
        index = strlen(buf);
        sprintf(buf+index,"<f%d>\"];\n",D_RO(n)->num_keys);
        for(int i = 0; i <= D_RO(n)->num_keys; i++)
        {
            node_pointer temp = TOID_NULL(struct tree_node);
            TOID_ASSIGN(temp,D_RO(n)->pointers[i]);
            declare_node(buf,temp,layout+1,leaf_sum);
        }
    }
}
int print_edge(char *buf,node_pointer n,int layout,int parent,int f,int leaf_num)
{
    int edges = 0;
    int index = 0;
    if(layout)  //n is not root
    {
        index = strlen(buf);
        sprintf(buf+index,"node%d_%d:f%d -> node%d_%d;\n",layout-1,parent,f,layout,start_index[layout]);
        edges += 1;
    }
    if(D_RO(n)->is_leaf)
    {
        //printf("in a leaf,start_index is %d layout is %d\n",start_index[layout],layout);
        if(start_index[layout] < leaf_num)
        {
            index = strlen(buf);
            sprintf(buf+index,"node%d_%d -> node%d_%d;\n",layout,start_index[layout],layout,start_index[layout]+1);
            start_index[layout]++;
            edges++;
        }
        return edges;
    }
    for(int i = 0; i <= D_RO(n)->num_keys; i++)
    {
        node_pointer temp;
        TOID_ASSIGN(temp,D_RO(n)->pointers[i]);
        edges += print_edge(buf,temp,layout+1,start_index[layout],i,leaf_num);
    }
    start_index[layout]++;
    return edges;
}
void init_layout_count(){
    for(int i = 0;i < MAX_HEIGHT;i++){
        layout_count[i] = 0;
    }
}

void print_rank_r(node_pointer n,int layout){
    layout_count[layout]++;
    if(D_RO(n)->is_leaf){
        return;
    }for(int i = 0;i <= D_RO(n)->num_keys;i++){
        node_pointer temp;
        TOID_ASSIGN(temp,D_RO(n)->pointers[i]);
        print_rank_r(temp,layout+1);
    }
}
void print_rank(char *buf,node_pointer root,int height){
    init_layout_count();
    print_rank_r(root,0);
    for(int i = 0;i< height;i++){
        strcat(buf,"{ rank=same; ");
        int index;
        for(int j=0;j<layout_count[i];j++){
            index = strlen(buf);
            sprintf(buf+index," \"node%d_%d\";",i,j);
        }strcat(buf,"}\n");
    }
}
char* print_declare(PMEMobjpool *pop)
{
    TOID(struct tree) t = POBJ_ROOT(pop,struct tree);
    node_pointer root = D_RO(t)->root;
    char *buf = (char*)malloc(sizeof(char)*MAX_BUFFER_SIZE);
    if(buf==NULL){
        perror("MALLOC FAILED");
        exit(EXIT_FAILURE);
    }
    buf[0] = '\0';
    strcat(buf,"digraph structs {\n node[shape=record];\nsplines=false;\n");
    int height = get_height(root),leaf_sum = get_leaf_num(root);
    init_start_index(height);
    declare_node(buf,root,0,leaf_sum);
    init_start_index(height);
    print_rank(buf,root,0);
    init_start_index(height);
    print_edge(buf,root,0,-1,-1,leaf_sum);
    print_rank(buf,root,height);
    strcat(buf,"}\n");
    int length = strlen(buf)+1;
    //printf("buf length = %d\n",length);
    return buf;
}
void display(PMEMobjpool *pop){
    char * buf = print_declare(pop);
    FILE * fp = fopen("display.dot","w");
    fwrite(buf,strlen(buf)+1,sizeof(char),fp);
    fclose(fp);
    free(buf);
    pid_t child;
    child = fork();
    if(child < 0){
        perror("FORK FAILED");
        exit(EXIT_FAILURE);
    }if(child==0){
        execlp("dot","dot","-Tjpg","display.dot","-o","display.jpg",(char *)0);
    }else{
        wait(NULL);
        //execlp("rm","rm","display.dot",(char*)0);
        child = fork();
        if(child < 0){
           perror("FORK FAILED");
           exit(EXIT_FAILURE);
        }if(child==0){
            execlp("rm","rm","display.dot",(char*)0);
        }else{
            wait(NULL);
            execlp("eog","eog","display.jpg",(char*)0);
         }
    }
}
int main()
{
    PMEMobjpool * pop = pmemobj_open("treefile",POBJ_LAYOUT_NAME(example));
    if(pop==NULL)
    {
        perror("OPEN FAILED");
        exit(EXIT_FAILURE);
    }
    //display(pop);
    display(pop);
    pmemobj_close(pop);
    return 0;
}



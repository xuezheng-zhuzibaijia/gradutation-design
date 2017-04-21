#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libpmemobj.h>
#include "btree.h"
char Usage[] = "[-wrdsh]";
void read_value(PMEMobjpool *pop,int key)
{
    PMEMoid value = find_value(pop,key);
    if(OID_IS_NULL(value))
    {
        printf("NOT FOUND\n");
    }
    printf("key = %d,value = %d\n",key,*((int *)pmemobj_direct(value)));
}
int find_colon_index(char * s,int len){
    for(int i = 0;i < len;i++){
        if(s[i]==':'){
            return i;
        }
    }return -1;
}
int main(int argc,char**argv)
{
    PMEMobjpool * pop = pmemobj_open("treefile",POBJ_LAYOUT_NAME(example));
    char ch;
    while((ch=getopt(argc,argv,"i:r:d:sh"))!=-1)
    {
        switch(ch)
        {
        case 'h':
            printf("Usage:\n");
            printf("-d key        delete key from  B+tree\n");
            printf("-i key:value  insert k/v pair into B+tree\n");
            printf("-r key        read the value of the key from B+tree\n");
            printf("-s            show the tree in a picture if tree is not empty,else note empty\n");
            printf("-h            print this information\n");
            break;
        case 'i':
            if(pop==NULL)
            {
                pop = pmemobj_create("treefile",POBJ_LAYOUT_NAME(example),PMEMOBJ_MIN_POOL,0666);
                if(pop==NULL)
                {
                    perror("CREATION FAILED");
                    exit(EXIT_FAILURE);

                }tree_init(pop);
            }
            int colon_index = find_colon_index(optarg,strlen(optarg));
            if(colon_index==-1){
                printf("The usage is \"-w key:value\"\n");
            }else{
                int key = atoi(optarg),value = atoi(optarg+colon_index+1);
                tree_insert(pop,key,(void *)&value,sizeof(int));
            }break;
        case 'r':
            if(pop==NULL){
                printf("OPEN FAILED\n");
            }else{
                read_value(pop,atoi(optarg));
            }break;
        case 's':
            if(pop==NULL){
                printf("OPEN FAILED\n");
            }else{
                display(pop);
            }break;
        case 'd':
            if(pop==NULL){
                printf("OPEN FAILED\n");
            }else{
                tree_delete(pop,atoi(optarg));
            }break;
        case '?':
            printf("UNKNOWN OPTION\n");
            break;
        }
    }if(pop!=NULL){
        pmemobj_close(pop);
    }return 0;
}

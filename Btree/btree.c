#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "btree.h"

/*************************************************************************/
/******************************display part****************************************/
#define MAX_HEIGHT 15
#define MAX_BUFFER_SIZE 200000
static int start_index[MAX_HEIGHT];
static int layout_count[MAX_HEIGHT];
static void init_start_index(int h)
{
    for(int i = 0; i < h; i++)
    {
        start_index[i] = 0;
    }
}
static int get_height(node_pointer root)
{
    int i = 1;
    while(!D_RO(root)->is_leaf)
    {
        i++;
        TOID_ASSIGN(root,D_RO(root)->pointers[0]);
    }
    return i;
}
static int get_leaf_num(node_pointer root)
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
static void declare_node(char *buf,node_pointer n,int layout,int leaf_sum)
{
    int index = 0;
    strcat(buf,"node");
    index = strlen(buf);
    sprintf(buf+index,"%d_%d[shape=record, style=filled, ",layout,start_index[layout]);
    if(D_RO(n)->is_leaf)
    {
        strcat(buf,"fillcolor=green, ");
    }
    else
    {
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
static int print_edge(char *buf,node_pointer n,int layout,int parent,int f,int leaf_num)
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
static void init_layout_count()
{
    for(int i = 0; i < MAX_HEIGHT; i++)
    {
        layout_count[i] = 0;
    }
}

static void print_rank_r(node_pointer n,int layout)
{
    layout_count[layout]++;
    if(D_RO(n)->is_leaf)
    {
        return;
    }
    for(int i = 0; i <= D_RO(n)->num_keys; i++)
    {
        node_pointer temp;
        TOID_ASSIGN(temp,D_RO(n)->pointers[i]);
        print_rank_r(temp,layout+1);
    }
}
static void print_rank(char *buf,node_pointer root,int height)
{
    init_layout_count();
    print_rank_r(root,0);
    for(int i = 0; i< height; i++)
    {
        strcat(buf,"{ rank=same; ");
        int index;
        for(int j=0; j<layout_count[i]; j++)
        {
            index = strlen(buf);
            sprintf(buf+index," \"node%d_%d\";",i,j);
        }
        strcat(buf,"}\n");
    }
}
static char* print_declare(node_pointer root)
{
    if(TOID_IS_NULL(root)){
        perror("The tree is empty!\n");
        exit(EXIT_FAILURE);
    }
    char *buf = (char*)malloc(sizeof(char)*MAX_BUFFER_SIZE);
    if(buf==NULL)
    {
        perror("MALLOC FAILED");
        exit(EXIT_FAILURE);
    }
    buf[0] = '\0';
    strcat(buf,"digraph structs {\n node[shape=record];ranksep=1.5;\nsplines=false;\n");
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
    if(length > MAX_BUFFER_SIZE){
        perror("MAX_BUFFER_SIZE is too small");
        exit(EXIT_FAILURE);
    }
    return buf;
}
static void display_node(node_pointer root)
{
    char * buf = print_declare(root);
    FILE * fp = fopen("display.dot","w");
    fwrite(buf,strlen(buf)+1,sizeof(char),fp);
    fclose(fp);
    free(buf);
    pid_t child;
    child = fork();
    if(child < 0)
    {
        perror("FORK FAILED");
        exit(EXIT_FAILURE);
    }
    if(child==0)
    {
        execlp("dot","dot","-Tjpg","display.dot","-o","display.jpg",(char *)0);
    }
    else
    {
        wait(NULL);
        //execlp("rm","rm","display.dot",(char*)0);
        child = fork();
        if(child < 0)
        {
            perror("FORK FAILED");
            exit(EXIT_FAILURE);
        }
        if(child==0)
        {
            execlp("rm","rm","display.dot",(char*)0);
        }
        else
        {
            wait(NULL);
            child = fork();
            if(child < 0)
            {
                perror("FORK FAILED");
                exit(EXIT_FAILURE);
            }
            if(child==0)
            {
                execlp("eog","eog","display.jpg",(char*)0);
            }
            else
            {
                wait(NULL);
            }
        }
    }
}
void display(PMEMobjpool *pop){
    TOID(struct tree) myroot = POBJ_ROOT(pop,struct tree);
    node_pointer root = D_RO(myroot)->root;
    display_node(root);
}



/************************************************************************/
/******************************insert part****************************************/


/*
 *make_record--return a PMEMoid point to value
 */
PMEMoid make_record(void *value,size_t len)
{
    PMEMoid temp = pmemobj_tx_alloc(len,TOID_TYPE_NUM(void));
    TX_MEMCPY(pmemobj_direct(temp),value,len);
    return temp;
}
/*
 *tree_init--initialize the tree
 */
void tree_init(PMEMobjpool*pop)
{
    TX_BEGIN(pop)
    {
        TOID(struct tree) t = POBJ_ROOT(pop,struct tree);
        TX_SET(t,root,TOID_NULL(struct tree_node));
    }
    TX_END
}
/*
 *binary_search_exact--return index of key in the keys,if not exists,return -1
 */
int binary_search_exact(node_pointer n,int key)
{
    int left = 0,right = D_RO(n)->num_keys - 1;
    while(left<=right)
    {
        int mid = (left+right)/2;
        if(D_RO(n)->keys[mid]==key)
        {
            return mid;
        }
        if(D_RO(n)->keys[mid]>key)
        {
            right = mid-1;
        }
        else
        {
            left = mid+1;
        }
    }
    return -1;
}
/*
 *binary_search--return the index of pointers,which may include the key
 */
int binary_search(node_pointer n,int key)
{

    int left = 0,right = D_RO(n)->num_keys - 1;
    while(left <= right)
    {
        int mid = (left + right)/2;
        if(D_RO(n)->keys[mid] > key)
        {
            right =  mid - 1;
        }
        else
        {
            left = mid + 1;
        }
    }
    return left;
}
//Output
/*
 *find_leaf--return the leaf which may contain the key
 */
node_pointer find_leaf(node_pointer root,int key)
{
    node_pointer c = root;
    if(TOID_IS_NULL(c))
    {
        return c;
    }
    while(!D_RO(c)->is_leaf)
    {
        int index =  binary_search(c,key);
        TOID_ASSIGN(c,D_RO(c)->pointers[index]);
    }
    return c;
}
/*
 *find_value--return the value of key if exists,else return OID_NULL
 */
PMEMoid find_value(PMEMobjpool *pop,int key)
{
    TOID(struct tree) myroot = POBJ_ROOT(pop,struct tree);
    node_pointer root = D_RO(myroot)->root;
    node_pointer leaf = find_leaf(root,key);
    if(TOID_IS_NULL(leaf))
    {
        return OID_NULL;
    }
    int index = binary_search_exact(leaf,key);
    return (index==-1)?OID_NULL:D_RO(leaf)->pointers[index];
}
//Insertion
/*
 *make_node--return TOID(struct tree_node) point to a new allocated node
 */
node_pointer make_node()
{
    node_pointer n = TX_NEW(struct tree_node);
    TX_SET(n,is_leaf,FALSE);
    TX_SET(n,num_keys,0);
    TX_SET(n,parent,TOID_NULL(struct tree_node));
    for(int i = 0; i <= BTREE_ORDER; i++)
    {
        TX_SET(n,pointers[i],OID_NULL);
    }
    return n;
}
/*
 *make_leaf--same as make_node except return a pointer pointing to leaf
 */
node_pointer make_leaf()
{
    node_pointer leaf = make_node();
    TX_SET(leaf,is_leaf,TRUE);
    TX_SET(leaf,pointers[BTREE_ORDER],OID_NULL);
    return leaf;
}
/*
 *insert_into_leaf--insert key/value to leaf then return leaf,in this situation the leaf don't need to split
 */
node_pointer insert_into_leaf( node_pointer leaf,int key,PMEMoid value)
{
    int index = binary_search(leaf,key);
    for(int i = D_RO(leaf)->num_keys-1; i >= index; --i)
    {
        TX_SET(leaf,keys[i+1],D_RO(leaf)->keys[i]);
        TX_SET(leaf,pointers[i+1],D_RO(leaf)->pointers[i]);
    }
    TX_SET(leaf,keys[index],key);
    TX_SET(leaf,pointers[index],value);
    TX_SET(leaf,num_keys,D_RO(leaf)->num_keys+1);
    return leaf;
}
/*
 *cut--get the index where to split
 */
int cut(int length)
{
    return (length%2==0)?(length/2):(length/2+1);
}
/*
 *insert_into_leaf_after_splitting--insert key/value to leaf then cause the leaf splitting
 */
node_pointer insert_into_leaf_after_splitting( node_pointer root,node_pointer leaf,int key,PMEMoid value)
{
    node_pointer new_leaf;
    new_leaf = make_leaf();
    int split_index = cut(BTREE_ORDER),index = binary_search(leaf,key);
    for(int i = 0,j = split_index; j < D_RO(leaf)->num_keys; j++,i++)
    {
        TX_SET(new_leaf,keys[i],D_RO(leaf)->keys[j]);
        TX_SET(new_leaf,pointers[i],D_RO(leaf)->pointers[j]);
    }
    int new_leaf_index = D_RO(leaf)->num_keys-split_index;
    TX_SET(leaf,num_keys,split_index);
    TX_SET(new_leaf,num_keys,new_leaf_index);
    if(index<split_index)
    {
        leaf = insert_into_leaf(leaf,key,value);
    }
    else
    {
        new_leaf = insert_into_leaf(new_leaf,key,value);
    }
    TX_SET(new_leaf,pointers[BTREE_ORDER],D_RO(leaf)->pointers[BTREE_ORDER]);
    TX_SET(leaf,pointers[BTREE_ORDER],new_leaf.oid);
    return insert_into_parent(root,leaf,D_RO(new_leaf)->keys[0],new_leaf);
}
/*
 *insert_into_node--sample as insert_into_leaf,but not to a leaf instead of a interval node
 */
node_pointer insert_into_node(node_pointer root,node_pointer n,int key,node_pointer child)
{
    int index = binary_search(n,key);
    for(int i = D_RO(n)->num_keys-1; i >= index; --i)
    {
        TX_SET(n,keys[i+1],D_RO(n)->keys[i]);
        TX_SET(n,pointers[i+2],D_RO(n)->pointers[i+1]);
    }
    TX_SET(n,keys[index],key);
    TX_SET(n,pointers[index+1],child.oid);
    TX_SET(child,parent,n);
    TX_SET(n,num_keys,D_RO(n)->num_keys+1);
    return root;
}
/*
 *insert_into_node_after_splitting--sample as insert_into_leaf_after_splitting,but not to a leaf instead of a interval node
 */
node_pointer insert_into_node_after_splitting(node_pointer root,node_pointer n,int key,node_pointer child)
{
    node_pointer new_n = make_node(),temp;
    int split_index = cut(BTREE_ORDER),index = binary_search(n,key);
    int new_key = D_RO(n)->keys[split_index];
    for(int i=0,j = split_index+1; j < D_RO(n)->num_keys; j++,i++)
    {
        TX_SET(new_n,keys[i],D_RO(n)->keys[j]);
        TX_SET(new_n,pointers[i+1],D_RO(n)->pointers[j+1]);
        TOID_ASSIGN(temp,D_RO(new_n)->pointers[i+1]);
        TX_SET(temp,parent,new_n);
    }
    int new_n_index = D_RO(n)->num_keys-split_index-1;
    TX_SET(new_n,pointers[0],D_RO(n)->pointers[split_index+1]);
    TX_SET(n,num_keys,split_index);
    TX_SET(new_n,num_keys,new_n_index);
    TOID_ASSIGN(temp,D_RO(new_n)->pointers[D_RO(new_n)->num_keys]);
    TX_SET(temp,parent,new_n);
    if(index<split_index)
    {
        insert_into_node(root,n,key,child);
    }
    else
    {
        insert_into_node(root,new_n,key,child);
    }
    return insert_into_parent(root,n,new_key,new_n);
}
/*
 *insert_into_parent--insert a new child to parent
 *return root
 */
node_pointer insert_into_parent(node_pointer root,node_pointer left,int key,node_pointer right)
{
    node_pointer parent = D_RO(left)->parent;
    if(TOID_IS_NULL(parent))
    {
        return insert_into_new_root(left,key,right);
    }
    else
    {
        TX_SET(right,parent,parent);
        if(D_RO(parent)->num_keys < BTREE_ORDER)
        {
            root = insert_into_node(root,parent,key,right);
        }
        else
        {
            root = insert_into_node_after_splitting(root,parent,key,right);
        }
    }
    return root;
}
/*
 *insert_into_new_root--build a new root contains two children
 */
node_pointer insert_into_new_root(node_pointer left,int key,node_pointer right)
{
    node_pointer root = make_node();
    TX_SET(root,num_keys,1);
    TX_SET(root,keys[0],key);
    TX_SET(root,pointers[0],left.oid);
    TX_SET(root,pointers[1],right.oid);
    TX_SET(left,parent,root);
    TX_SET(right,parent,root);
    return root;
}
/*
 *start_new_tree--build a leaf as root
 */
node_pointer start_new_tree(int key,PMEMoid value)
{
    node_pointer root = make_leaf();
    TX_SET(root,keys[0],key);
    TX_SET(root,pointers[0],value);
    TX_SET(root,num_keys,1);
    return root;
}
/*
 *tree_insert--the abstract interface for user to use to insert a key/value
 */
void tree_insert(PMEMobjpool *pop,int key,void *value,size_t len)
{
    PMEMoid v = find_value(pop,key);
    if(!OID_IS_NULL(v))
    {
        printf("key %d has already inserted,value is %d!\n",key,*((int *)pmemobj_direct(v)));
    }
    else
    {
        TOID(struct tree) myroot = POBJ_ROOT(pop,struct tree);
        node_pointer root = D_RO(myroot)->root;
        TX_BEGIN(pop)
        {
            PMEMoid pvalue = make_record(value,len);
            if(TOID_IS_NULL(root))
            {
                TX_SET(myroot,root,start_new_tree(key,pvalue));
            }
            else
            {
                node_pointer leaf = find_leaf(root,key);
                if(D_RO(leaf)->num_keys < BTREE_ORDER)
                {
                    insert_into_leaf(leaf,key,pvalue);
                }
                else
                {
                    TX_SET(myroot,root,insert_into_leaf_after_splitting(root,leaf,key,pvalue));
                }
            }
        }
        TX_ONCOMMIT
        {
            //printf("key %d insert successfully!\n",key);
        }
        TX_END
    }
}



/******************************************************************/
/**************************delete part*************************************/



//Deletion
/*
 *get_neighbor_index--get_neighbor_index--find the index of pointer left to n
 *if n is the leftest,return -1
 */
int get_neighbor_index(node_pointer n)
{
    node_pointer parent = D_RO(n)->parent;
    return binary_search(parent,D_RO(n)->keys[0]) - 1;
}
/*
 *adjust_root--To maintain the balance of B+Tree after deletion,
 *we have to adjust root
 */
node_pointer adjust_root(node_pointer root)
{
    if(D_RO(root)->num_keys>0)
    {
        return root;
    }
    node_pointer new_root;
    if(!D_RO(root)->is_leaf)
    {
        TOID_ASSIGN(new_root,D_RO(root)->pointers[0]);
        TX_SET(new_root,parent,TOID_NULL(struct tree_node));
    }
    else
    {
        new_root = TOID_NULL(struct tree_node);
    }TX_FREE(root);
    return new_root;
}
/*
 *delete_entry--delete entry from node n
 */
node_pointer remove_entry_from_node(node_pointer n,int key)
{
    int index = binary_search_exact(n,key);
    for(int i = index+1; i < D_RO(n)->num_keys; i++)
    {
        TX_SET(n,keys[i-1],D_RO(n)->keys[i]);
        if(!D_RO(n)->is_leaf)
        {
            TX_SET(n,pointers[i],D_RO(n)->pointers[i+1]);
        }
        else
        {
            TX_SET(n,pointers[i-1],D_RO(n)->pointers[i]);
        }
    }
    TX_SET(n,num_keys,D_RO(n)->num_keys-1);
    return n;
}

/*
 *coalesce_nodes--after deletion,the num keys of n is too few,so has to collapse with his neighbor
 *neighbor is the left neighbor of n
 *return root
 */
node_pointer coalesce_nodes(node_pointer root,node_pointer n,node_pointer neighbor,int neighbor_index,int k_prime)
{
    /*
     *case n is the leftmost child then swap it with its neighbor
     */
    node_pointer temp = TOID_NULL(struct tree_node);
    if(neighbor_index==-1)
    {
        temp = n;
        n = neighbor;
        neighbor = temp;
    }
    if(!D_RO(n)->is_leaf)
    {
        TX_SET(neighbor,keys[D_RO(neighbor)->num_keys],k_prime);
        TX_SET(neighbor,pointers[D_RO(neighbor)->num_keys+1],D_RO(n)->pointers[0]);
        TOID_ASSIGN(temp,D_RO(neighbor)->pointers[D_RO(neighbor)->num_keys+1]);
        TX_SET(temp,parent,neighbor);
        for(int i = D_RO(neighbor)->num_keys+1,j = 0; j < D_RO(n)->num_keys; i++,j++)
        {
            TX_SET(neighbor,keys[i],D_RO(n)->keys[j]);
            TX_SET(neighbor,pointers[i+1],D_RO(n)->pointers[j+1]);
            TOID_ASSIGN(temp,D_RO(neighbor)->pointers[i+1]);
            TX_SET(temp,parent,neighbor);
        }
        TX_SET(neighbor,num_keys,D_RO(n)->num_keys+D_RO(neighbor)->num_keys+1);
    }
    else
    {
        for(int i = D_RO(neighbor)->num_keys,j = 0; j < D_RO(n)->num_keys; i++,j++)
        {
            TX_SET(neighbor,keys[i],D_RO(n)->keys[j]);
            TX_SET(neighbor,pointers[i],D_RO(n)->pointers[j]);
        }
        TX_SET(neighbor,num_keys,D_RO(n)->num_keys+D_RO(neighbor)->num_keys);
        TX_SET(neighbor,pointers[BTREE_ORDER],D_RO(n)->pointers[BTREE_ORDER]);
    }
    TX_FREE(n);
    root = delete_entry(root,D_RO(neighbor)->parent,k_prime);
    return root;
}
/*
 *redistribute--borrow a k_v pair from neighbor
 */
node_pointer redistribute_nodes(node_pointer root,node_pointer n,node_pointer neighbor,int neighbor_index,int k_prime,int k_prime_index)
{
    /*
     *case the n is leftest node of parent,than borrow a k_v from right neighbor
     */
    node_pointer temp = TOID_NULL(struct tree_node);
    if(neighbor_index==-1)
    {
        if(!D_RO(n)->is_leaf)
        {
            TX_SET(n,keys[D_RO(n)->num_keys],k_prime);
            TX_SET(D_RW(n)->parent,keys[k_prime_index],D_RO(neighbor)->keys[0]);
            TX_SET(n,pointers[D_RO(n)->num_keys+1],D_RO(neighbor)->pointers[0]);
            TOID_ASSIGN(temp,D_RO(n)->pointers[D_RO(n)->num_keys+1]);
            TX_SET(temp,parent,n);
            TX_SET(neighbor,pointers[0],D_RO(neighbor)->pointers[1]);
            for(int i = 1; i < D_RO(neighbor)->num_keys; i++)
            {
                TX_SET(neighbor,keys[i-1],D_RO(neighbor)->keys[i]);
                TX_SET(neighbor,pointers[i],D_RO(neighbor)->pointers[i+1]);
            }
            TX_SET(n,num_keys,D_RO(n)->num_keys+1);
            TX_SET(neighbor,num_keys,D_RO(neighbor)->num_keys-1);
        }
        else
        {

            for(int i = 1; i < D_RO(neighbor)->num_keys; i++)
            {
                TX_SET(neighbor,keys[i-1],D_RO(neighbor)->keys[i]);
                TX_SET(neighbor,pointers[i-1],D_RO(neighbor)->pointers[i]);
            }
            TX_SET(n,num_keys,D_RO(n)->num_keys+1);
            TX_SET(neighbor,num_keys,D_RO(neighbor)->num_keys-1);
        }
    }
    else
    {
        if(!D_RO(n)->is_leaf)
        {
            for(int i = D_RO(n)->num_keys-1; i>=0; i--)
            {
                TX_SET(n,keys[i+1],D_RO(n)->keys[i]);
                TX_SET(n,pointers[i+2],D_RO(n)->pointers[i+1]);
            }
            TX_SET(n,pointers[1],D_RO(n)->pointers[0]);
            TX_SET(n,pointers[0],D_RO(neighbor)->pointers[D_RO(n)->num_keys]);
            TX_SET(n,keys[0],k_prime);
            TX_SET(D_RW(n)->parent,keys[k_prime_index],D_RO(neighbor)->keys[D_RO(neighbor)->num_keys-1]);
            TOID_ASSIGN(temp,D_RO(n)->pointers[0]);
            TX_SET(temp,parent,n);
            TX_SET(n,num_keys,D_RO(n)->num_keys+1);
            TX_SET(neighbor,num_keys,D_RO(neighbor)->num_keys-1);
        }
        else
        {
            for(int i = D_RO(n)->num_keys-1; i>=0; i--)
            {
                TX_SET(n,keys[i+1],D_RO(n)->keys[i]);
                TX_SET(n,pointers[i+1],D_RO(n)->pointers[i]);
            }
            TX_SET(n,keys[0],D_RO(neighbor)->keys[D_RO(neighbor)->num_keys-1]);
            TX_SET(n,pointers[0],D_RO(neighbor)->pointers[D_RO(n)->num_keys-1]);
            TX_SET(D_RW(n)->parent,keys[k_prime_index],D_RO(n)->keys[0]);
            TX_SET(n,num_keys,D_RO(n)->num_keys+1);
            TX_SET(neighbor,num_keys,D_RO(neighbor)->num_keys-1);
        }
    }
    return root;
}
/*
 *delete_entry--delete key and its corresponding value from n
 *return root
 */
node_pointer delete_entry(node_pointer root,node_pointer n,int key)
{
    n = remove_entry_from_node(n,key);
    if(TOID_EQUALS(n,root))
    {
        return adjust_root(root);
    }
    if(D_RO(n)->num_keys >= BTREE_MIN)
    {
        return root;
    }
    int neighbor_index = get_neighbor_index(n);
    node_pointer parent = D_RO(n)->parent;
    int k_prime_index = (neighbor_index==-1)?0:neighbor_index;
    int k_prime = D_RO(parent)->keys[k_prime_index];
    node_pointer neighbor = TOID_NULL(struct tree_node);
    TOID_ASSIGN(neighbor,(neighbor_index==-1)?D_RO(parent)->pointers[1]:D_RO(parent)->pointers[neighbor_index]);
    if(D_RO(n)->num_keys+D_RO(neighbor)->num_keys <= BTREE_ORDER)
    {
        return coalesce_nodes(root,n,neighbor,neighbor_index,k_prime);
    }
    return redistribute_nodes(root,n,neighbor,neighbor_index,k_prime,k_prime_index);
}
/*
 *tree_delete--interface for user to delete key
 */
void tree_delete(PMEMobjpool *pop,int key)
{
    TOID(struct tree) myroot = POBJ_ROOT(pop,struct tree);
    node_pointer root = D_RO(myroot)->root;
    PMEMoid value = find_value(pop,key);
    if(OID_IS_NULL(value))
    {
        return;
    }
    TX_BEGIN(pop)
    {
        TX_SET(myroot,root,delete_entry(root,find_leaf(root,key),key));
    }
    TX_END
}
/*
 *destroy_node--destory node
 */
void destroy_tree_nodes(node_pointer root)
{
    node_pointer temp = TOID_NULL(struct tree_node);
    if(!D_RO(root)->is_leaf)
    {
        for(int i = 0; i <= D_RO(root)->num_keys; i++)
        {
            TOID_ASSIGN(temp,D_RO(root)->pointers[i]);
            destroy_tree_nodes(temp);
        }
    }
    else
    {
        for(int i = 0; i < D_RO(root)->num_keys; i++)
        {
            TOID_ASSIGN(temp,D_RO(root)->pointers[i]);
            TX_FREE(temp);
        }
    }
    TX_FREE(root);
}
void destroy_tree(PMEMobjpool *pop)
{
    TOID(struct tree) myroot = POBJ_ROOT(pop,struct tree);
    node_pointer root = D_RO(myroot)->root;
    TX_BEGIN(pop)
    {
        destroy_tree_nodes(root);
        TX_SET(myroot,root,TOID_NULL(struct tree_node));
    }
    TX_END
}

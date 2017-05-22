#include <stdio.h>
#include <stdlib.h>
#include "palm.h"
/***************tree operation********/


/*****************************PALM*********************************/
void palm_init(node_pointer * root){
        /*the tree has one leaf at least*/
        if(TOID_IS_NULL(*root)){
                *root = make_leaf();
        }
        //全局变量优化
        leftest_leaf = get_leftest_leaf(*root);
        operation_lst_count = 0;  //操作序列
        range_set_count = 0;　　　　　　　　　//区间操作序列
        record_list_count = 0;　　　　　//新插入记录
        task_list_count = 0 　　　　　　　　　　　//任务序列
        read_result_count = 0;　　　　　//读操作结果序列
        result_list_count = 0;　　　　　//结果序列
}











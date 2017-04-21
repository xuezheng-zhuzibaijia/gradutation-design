# gradutation-design

* 作者：薛正　
* 创建时间：2017/4/7
* 描述：在linux环境下，使用c语言实现基于NVM Library的B+tree。实现B+树的建立、销毁、插入、删除、查找等动作。

项目结构如下:

* Btree/btree.h---定义数据结构以及函数原型
* Btree/btree.c---实现btree.h中所定义的函数
* Btree/display.c---用于展示树的状态，调用dot与eog将树以图片的方式显示出来
* Btree/makefile---自动编译项目
* Btree/reader.c---测试B+tree的读操作
* Btree/writer.c---测试B+tree的写操作	
* README.md---显示当前页面



展示树需要:[graphviz](http://www.graphviz.org/)和eog


安装eog
```shell
    sudo apt install eog;
```

编译
```shell
    cd Btree;make all;
```
测试插入数据
```shell
    ./writer
```
测试读取数据
```shell
    ./reader
```
展示树
```shell
   ./display 
```
NVM的相关介绍　[NVM Library](http://pmem.io/)
相关参考资源：C语言实现的[B+Tree](http://www.amittai.com/prose/bpt.c) 

# gradutation-design

* 作者：薛正　
* 创建时间：2017/4/7
* 描述：c语言实现基于NVM Library的B+tree。实现B+树的建立、销毁、插入、删除、查找等动作。

项目结构
├── Btree
│   ├── btree.c
│   ├── btree.h
│   ├── display.c
│   ├── makefile
│   ├── reader.c
│   └── writer.c
└── README.md

展示树需要:[graphviz](http://www.graphviz.org/),eog
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

###  跳表(SkipList)

描述：在Linux环境下，实现了基于NVM Library的跳表

目录结构

*  　reader.c --- 以命令行方式打印跳表
* 　 skip_list.h --- 定义了跳表相关的数据结构和函数原型
*   skip_list.c --- 实现了skip_list.h中的函数原型
*   main.c --- 测试文件
*   README.md --- 项目描述
##### 可能用到的命令
编译全部
```shell
	make all
```
编译reader
```shell
	make reader
```
编译main
```shell
	make main
```

显示帮助
```shell
	./main -h
```

插入key/value
```shell
	./main -i key:value
```
查看跳表
```shell
	./main -s
```
读取key对应的键值
```shell
	./main -r key
```

## B+树

描述：在Linux环境下，基于NVM　Library开发了B+tree

目录结构

* btree.h --- 定义数据结构和函数原型
* btree.c --- 实现btree.h里面定义的函数
* writer.c --- 写B+tree测试文件
* delete_test.c --- 删除B+tree测试文件
* reader.c --- 读B+tree测试文件
* makefile --- 编译工具文件
* display.c --- 展示b+tree测试文件
* main.c --- 命令行接口文件

##### 相关命令
编译
```shell
	make all
```

运行

显示帮助
```shell
	./main -h
```
测试插入
```shell
	./writer

```
测试读
```shell
	./reader
```
插入key/value
```shell
	./main -i key:value
```
查看B+tree
```shell
	./main -s 
```
 或者
```shell
	./display
```
读取key对应的键值
```shell
	./main -r key
```
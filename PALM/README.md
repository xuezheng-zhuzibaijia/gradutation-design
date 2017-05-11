####PLAM模型

PALM模型是一种支持高并发的、无锁的B+树模型

六个阶段：

* 1.多线程分配操作流，寻找叶子节点，重新分配操作使得对于每个节点的所有操作都由一个线程来完成
* 2.处理节点，返回结果,并向上传递Modified-List
* 3.处理内部节点，传递Modified-List
* 4.处理根节点
* 5.修改树，处理Modified-List
* 6.处理Range操作

[PALM模型论文链接](https://www.researchgate.net/publication/220538843_PALM_Parallel_Architecture-Friendly_Latch-Free_Modifications_to_B_Trees_on_Many-Core_Processors)
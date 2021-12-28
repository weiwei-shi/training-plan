# Task #1 - System Catalog
## catalog.h函数
#### 1.CreateTable
将新创建的表加入两个hash表以及index_names_的hash表中
#### 2.GetTable
分别由name和oid得到表
#### 3.CreateIndex
将新创建的索引加入两个hash表中
#### 4.GetIndex
分别由索引name，表name和索引oid得到索引
#### 5.GetTableIndexes
找到表名对应的所有索引的元数据

## catalog.h参数
* tables_：表hash，由表标识符得到表的元数据
* names_：表名字hash，由此得到表标识符
* next\_table\_oid_：下一个表标识符
* indexes_：索引hash，由索引标识符得到索引的元数据
* index\_names_：索引名字hash，由表名字得到索引名字，再得到索引标识符
* next\_index\_oid_：下一个索引标识符


# Task #2 - Executors
对于每种查询计划运算符类型，都有一个相应的executor对象，该对象需实现Init和Next方法。   
Init方法用于设置有关操作调用的内部状态。  
Next方法提供了迭代器接口，该接口在每次调用时返回一个元组（如果没有更多的元组，则返回null）。

## Sequential Scan（顺序扫描）
遍历一个表并逐个返回它的元组。顺序扫描由SeqScanPlanNode指定。计划节点指定要迭代的表。计划节点还可以包含谓词，如果一个元组不满足谓词，则跳过它。
#### 添加成员变量
	TableHeap *table_heap_;
	TableIterator table_iterator_ = {nullptr, RID(), nullptr};
#### 1.Init
初始化一个表迭代器
#### 2.Next
从头开始遍历整个表的所有tuple。找到满足要求的tuple。并将tuple中的满足要求的value和outSchema组合成新的tuple。

## Index Scan（索引扫描）
遍历一个索引以获得元组rid。这些rid用于在与索引对应的表中查找元组。最后，这些元组一次一个地返回。索引扫描由IndexScanPlanNode指定。plan节点指定要迭代的索引。计划节点还可以包含谓词，如果一个元组不满足谓词，则跳过它。
#### 添加成员变量
	TableHeap *table_heap_;
	IndexIterator<GenericKey<8>, RID, GenericComparator<8>> index_iterator_;
	IndexIterator<GenericKey<8>, RID, GenericComparator<8>> end_iterator_;
注意：索引的类型需假设为`<GenericKey<8>, RID, GenericComparator<8>>`
#### 1.Init
初始化一个索引迭代器（利用B+树索引里面的迭代器）
#### 2.Next
从头开始遍历整个索引，获得元组的rid，从而找到表中的对应元组。判断是否满足要求。并将满足要求的tuple中的满足要求的value和outSchema组合成新的tuple。

## Insert（插入）
将元组插入到表中并更新索引。插入由InsertPlanNode指定。  
有两种类型的插入:  
1.直接插入(只有插入语句);  
2.非直接插入(从子语句中获取要插入的值)。  
#### 添加成员变量
	std::unique_ptr<AbstractExecutor> child_executor_;
	TableMetadata *table_metadata_;
	std::vector<IndexInfo *> table_indexes_;
	std::vector<std::vector<Value>>::const_iterator iterator_;
#### 1.Init
1.如果是直接插入，初始化一个指向插入数值的迭代器  
2.如果不是，则需要对子执行器进行初始化
#### 2.Next
1.如果是直接插入，对每组插入的元组，需要将其插入表中，同时对被插入的表对应的索引（B+树）进行更新（插入新的节点）；  
2.如果不是，需要利用子执行器的Next函数，得到子语句的结果（即需要插入的元组），再进行插入表中以及更新索引。

## Update（更新）
修改指定表中的现有元组并更新其索引。子执行器将提供更新执行器将修改的元组(及其RID)。
#### 添加成员变量
	std::vector<IndexInfo *> table_indexes_;
#### 1.Init
对子执行器进行初始化
#### 2.Next
利用子执行器的Next函数，得到子语句的结果（即需要更新的元组），再在表中更新对应的元组，同时需要更新表对应的索引（删除原来的索引，插入新的索引）。

## Delete（删除）
从表中删除元组并从索引中删除它们的条目。与更新一样，目标元组的集合来自子执行器。
#### 添加成员变量
	const TableMetadata *table_info_;
	std::vector<IndexInfo *> table_indexes_;
#### 1.Init
对子执行器进行初始化
#### 2.Next
利用子执行器的Next函数，得到子语句的结果（即需要删除的元组），再在表中删除对应的元组（调用MarkDelete函数使该元组不可见。删除将在事务提交时应用），同时需要删除表对应的索引。

## Nested Loop Join（嵌套循环连接）
实现一个基本的嵌套循环连接，将来自它的两个子执行器的元组组合在一起。
#### 添加成员变量
	std::unique_ptr<AbstractExecutor> left_executor_;
	std::unique_ptr<AbstractExecutor> right_executor_;
#### 1.Init
对左右两个子执行器进行初始化
#### 2.Next
使用内外两个循环，对于左执行器的每个元组（利用Next函数得到），在右执行器的元组中查找符合条件的元组进行连接。

## Index Nested Loop Join（嵌套索引连接）
这个执行器将只有一个子执行器，它提出与连接的外部表相对应的元组。对于每一个元组，需要在内部表中通过使用目录中的索引匹配谓词找到相应元组。
#### 添加成员变量
	std::unique_ptr<AbstractExecutor> child_executor_;
	TableHeap *inner_table_;
	Index *bplustree_index;
#### 1.Init
对子执行器进行初始化
#### 2.Next
利用子执行器的Next函数，得到子语句的结果（左表的每个元组）。利用左表元组的key_tuple在右表的索引中找到对应的rid，再利用这个rid得到右表中对应的元组，最后进行连接。

## Aggregation（聚合）
这个执行器将来自单个子执行器的多个元组结果组合为一个元组。在这个项目中，将实现COUNT、SUM、MIN和MAX。  
项目提供了一个具有所有必要的聚合功能的SimpleAggregationHashTable。
需要使用HAVING子句实现GROUP BY。  
项目还提供了一个SimpleAggregationHashTable Iterator，它可以遍历哈希表。
#### 已提供函数说明
* 1.GenerateInitialAggregateValue  
为聚合操作提供不同的初始值：  
为count和sum操作提供的初始值为0。为求最小值操作提供的初始值是32位的最大值。max函数则提供了32位的最小值。
* 2.CombineAggregateValues  
实现聚合操作  
* 3.InsertCombine  
插入一个值到hash表中并进行聚合
* 4.Begin  
hash表指向头部的迭代器
* 5.End  
hash表指向尾部的迭代器
* 6.MakeKey
该元组对应的进行Group By的key（分组的其中一个）
* 7.MakeVal
该元组要进行聚合的列值
#### 添加成员变量
将给出的成员变量的注释取消
#### 1.Init
初始化一个hash表的迭代器，对子执行器进行初始化。同时要利用子执行器的Next函数，得到子语句的结果（满足条件的元组），将元组插入hash表中（完成分组和聚合操作）。
#### 2.Next
从头开始遍历整个hash表，对于每一对映射（每个分组对应组内的聚合），判断是否满足Having的条件，并将tuple中的满足要求的value和outSchema组合成新的tuple。

## Limit（限制）
限制从它的单个子程序输出的数量。偏移值指示执行器在发出元组之前需要跳过的元组数目。
#### 添加成员变量
	size_t counter_;
#### 1.Init
对子执行器进行初始化
#### 2.Next
利用子执行器的Next函数，得到子语句的结果（满足条件的元组），如果该元组不需要跳过（偏移值决定），且在输出的限制数量内，则可以输出。
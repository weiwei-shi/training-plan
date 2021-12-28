# Task #1 - Lock Manager
## 任务描述
这个任务只需要修改两个文件`concurrency/lock_manager.cpp`和`concurrency/lock_manager.h`。这里cmu已经在`include/concurrency/transaction.h`中提供了和事物相关的一些函数，。这里的锁管理(即LM)针对于tuple级别。我们需要对lock/unlock请求作出正确的行为。如果出现错误应该抛出异常。

## 一些小tips
1. 仔细阅读位于`lock_manager.h`内的`LockRequestQueue`类， 这会帮助确定哪些事物在等待一个锁
2. 建议使用`std::condition_variable`来通知那些等待锁的事物
3. 使用`shared_lock_set_`和`exclusive_lock_set_`来区别共享锁和排他锁。这样当TransactionManager想要提交和abort事物的时候，LM就可以合理的释放锁
4. 读`TransactionManager::Abort`来了解对于Abort状态的事物，LM是如何释放锁的

## 实现
#### LockShared(Transaction, RID)
事务txn尝试在记录ID上获取共享锁。这应该在等待时被阻止，并在授予时返回 true。如果事务回滚（中止），则返回 false。  
1.检查事务的隔离级别，如果是IsolationLevel::READ_UNCOMMITTED，不需要shared lock，直接将事务状态设为aborted，然后抛出异常。  
2.判断事务的状态。  
3.如果事务处于增长阶段，需要在锁表中查找是否有对应RID的锁请求队列。  
（1）如果没有，说明是该元组的第一个请求，在锁表中插入一个映射。  
（2）如果已有RID对应的请求队列，将该请求加入队列，并检查当前rid是否已有排他锁或者已有更新锁，如果已有冲突的锁，设置cv进行等待。  
（3）否则检测回滚（主要是由死锁导致的），需要将请求队列里的请求删除，并抛出异常。  
（4）否则加共享锁。  
4.如果事务处于收缩阶段，不能进行加锁，将事务的状态置为回滚，抛出异常。
#### LockExclusive(Transaction, RID)
事务txn尝试在记录ID上获取排他锁。这应该在等待时被阻止，并在授予时返回 true。如果事务回滚（中止），则返回 false。  
1.判断事务的状态。  
2.如果事务处于增长阶段，需要在锁表中查找是否有对应RID的锁请求队列。  
（1）如果没有，说明是该元组的第一个请求，在锁表中插入一个映射。  
（2）如果已有RID对应的请求队列，将该请求加入队列，并检查当前rid是否已有排他锁、共享锁或者更新锁，如果已有冲突的锁，设置cv进行等待。  
（3）否则检测回滚（主要是由死锁导致的），需要将请求队列里的请求删除，并抛出异常。  
（4）否则加排他锁。  
3.如果事务处于收缩阶段，不能进行加锁，将事务的状态置为回滚，抛出异常。
#### LockUpgrade(Transaction, RID)
事务txn尝试在记录ID上将共享升级到排他锁。这应该在等待时被阻止，并在授予时返回 true。如果事务回滚（中止），则返回 false。这也应该中止事务，如果另一个事务已经在等待升级其锁，则返回 false。  
1.判断事务的状态。  
2.如果事务处于增长阶段，需要在锁表中查找是否有对应RID的锁请求队列。  
（1）检查当前rid的锁冲突。  
（2）如果当前rid已有另一个事务在等待升级其锁，将事务的状态置为回滚，抛出异常。  
（3）如果当前rid已有排他锁、排除自己以外的共享锁，设置cv进行等待。  
（4）否则检测回滚（主要是由死锁导致的），需要将请求队列里的请求删除，并抛出异常。  
（5）否则更新对应的锁类型为排他锁，事务锁列表更新。  
3.如果事务处于收缩阶段，不能进行加锁，将事务的状态置为回滚，抛出异常。
#### Unlock(Transaction, RID)
解锁由事务保存的给定记录 ID 标识的记录。  
1.首先判断事务的状态。 如果是增长阶段，修改成为收缩阶段。注意：read commited如果unlock的是sharelock则不用修改。  
2.从事务的锁列表中，释放该锁。  
3.锁表中清除该事务的请求，并通过cv其他事务。  


# Task #2 - Deadlock Detection
## 任务描述
后台线程应该周期性地建立一个waits for graph并打破任何的cycles。

## 一些小tips
1. 后台线程应该在每次被唤醒时build the graph on the fly。
2. DFS环检测算法必须是确定性的。每次先选择探索最小的事务id。这意味着当选择从哪一个没有探索过的node来运行DFS时，永远选择事务id最小的node。这同样意味着当探索邻居时，按照事务id从小到大的顺序探索它们。
3. 当发现一个环，应该abort最年轻的事务（运行时间最短的）来打破cycle，通过将该事务的状态设为ABORTED。
4. 当detection thread被唤醒时，负责打破所有存在的cycles。当建立图时，不应该为aborted txns增加node或者画指向aborted txns的边。
5. 后台环检测算法可能需要使用txn_id获得对应的txn的指针。使用方法：
`Transaction* GetTransaction(txn_id_t txn_id)`
6. 使用`std::this_thread::sleep_for`来排序threads to write test cases。可以在测试用例中`tweak CYCLE_DETECTION_INTERVAL in common/config.h`。

## 实现
#### 添加变量
	std::unordered_set<txn_id_t> cycle_point_set;//在环中的点
	std::unordered_set<txn_id_t> visited;//被遍历过的点集
	txn_id_t id;//发现有环时的开始的点
	std::unordered_map<txn_id_t, RID> txn_tuple;//事务访问的元组
#### AddEdge(txn_id_t t1, txn_id_t t2)
在图表中添加从 t1 到 t2 的边。如果边缘已存在，则无需执行任何操作。
#### RemoveEdge(txn_id_t t1, txn_id_t t2)
从图形中删除边缘 t1 到 t2。如果不存在这样的边缘，则无需执行任何操作。
#### DFS(txn_id_t txn_id)
使用深度优先搜索 （DFS）算法从某一顶点开始查找环。  
1.将该点添加到已访问的点集中。  
2.对该点的邻居节点排序，依次进行判断。  
3.如果邻居节点已经在已访问的点集中，说明存在环，将该点以及邻居节点加入环的点集中，返回true。  
4.如果邻居节点的DFS为true，只要该点在环中，就将该点加入环的点集中，返回true。否则返回false，表示该点不在环中。
#### HasCycle(txn_id_t& txn_id)
如果找到一个环，则应将环中最年轻的事务ID（ID最大的）存储在其中，并返回 true。函数应返回它找到的第一个环。如果图没有环，则应返回 false。  
1.对所有有出边的点进行排序。  
2.从最小的点开始依次执行DFS，如果发现环的点集不为空，说明有环，返回点集中ID最大的。
#### GetEdgeList()
返回表示图中边缘的元组列表。使用它来测试图表的正确性。一对 （t1，t2） 对应于从 t1 到 t2 的边。
#### RunCycleDetection()
包含用于在后台运行循环检测的代码。在此处实现循环检测逻辑。每次唤醒时动态地构建和销毁等待图，将事务状态设置为 ABORTED 来中断循环。  
1.构建等待图。由未授权的指向已授权的。  
2.当发现有环时，将环中事务号最大的事务状态设置为 ABORTED ，并在等待图中删除该点及其相关的边，重新唤醒该事务影响的等待中的事务。  
3.对wait_for_进行清除。


# Task #3 - Concurrent Query Execution
## 任务描述
在执行并发的query execution时，执行器被要求适当地lock/unlock元组，来实现对应事务指定的隔离级别。为了简化这个任务，可以忽视并发的index execution，只专注于table tuples。  
任务需要更新一些执行器（sequential scans，inserts, updates, deletes, nested loop joins, and aggregations）的Next()方法。注意当lock/unlock失败时事务需要回滚。尽管这里没有对concurrent index execution的要求，当事务被回滚时，我们仍然需要undo所有先前在table tuples和indexes上的写操作。为了实现这点，需要在事务中维持写集，在事务管理器中会使用这个写集执行Abort()。  
不应该假设一个事务只包含一个查询。特别的，这意味着一个元组可能会在同一个事务中被不同的查询多次访问。  
可重复读隔离级别，读一个tuple时需要获取该tuple的读锁，写tuple时需要获取该tuple的写锁，只有在事务提交时才释放所有的锁。  
已提交读隔离级别，读锁在读后可以立即释放。这可能会导致不可重复读！写锁只有在事务提交时才释放。  
未提交读隔离级别，读tuple时不使用锁，这不仅会导致不可重复读，还会导致脏读。写锁只有在事务提交时才释放。

## 实现
#### SeqScanExecutor
1.在读之前加读锁：首先先检查事务是否已经有写锁了，有的话直接返回。否则判断隔离级别，是`READ_UNCOMMITTED`时直接返回。另外两种隔离级别先判断是否已经持有读锁了，否则再获取读锁。  
2.在next的结尾，如果事务隔离级别是READ_COMMITTED，直接释放该读锁。  
注意：由于使用tableIterator遍历表，每次next结束时会将tableIterator++，使其指向下一个未访问的tuple。而如果++tableIterator后，此时本线程还未获取tableIterator指向的tuple的读锁，其他线程这时将tableIterator指向的tuple删除了，本线程获得读锁之后就会访问到一个已经删除的tuple。  
解决：获取读锁后，再使用GetTuple读一次tuple的值。如果false，说明该tuple被删除了，则跳过该tuple。
#### update/delete/insert/join/Agg
1.update/delete/insert在修该元组之前加写锁：先检查事务是否获取了它的sharedLock。如果已经有shardLock，则调用LockUpgrade升级为ExclusiveLock。如果没有sharedLock，则先检查是否持有ExclusiveLock。若有则直接返回，否则调用LockExclusive获得写锁。
2.update/delete/insert在修改元组后需要将修改记录加入索引的写集种，当回滚时会进行撤销这些记录。   
3.join和agg都是调用seq，所以不需要单独处理。


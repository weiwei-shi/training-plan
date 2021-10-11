# lru-replacer
### lru_replacer.h
1. 引用 `replacer.h`  
自我感觉没什么用，只是单纯的继承
2. 引用 `config.h`  
应该会用到一个帧的数据类型`frame_id_t`
3. 函数说明
	* Victim：实现替换策略
	* Pin：实现系统对数据进行访问的时候，将数据从替换链表中移除
	* Unpin：实现当系统完成访问后，将数据重新插入替换链表
	* Size：返回在替换中能被替换的元素数量
	* 私有成员需定义双向链表+unordered map
	* 
# buffer-pool-manager
###buffer_pool_manager.h
1. 引用`lru_replacer.h`：用于使用替换策略  
应该会用到Victim函数、Pin函数、Unpin函数和Size函数
2. 引用`log_manager.h`：用于进行修改数据时的日志管理
3. 引用`disk_manager`：用于数据换出时进行磁盘管理  
应该会用到ReadPage函数和WritePage函数
4. 引用`page.h`：主要用于对页面的管理  
应该会用到GetData函数、pin_count_数据和is_dirty_数据，以及几个锁函数
5. 函数说明
	* BufferPoolManager：构造函数，传入缓冲池大小pool_size，磁盘管理和日志管理的对象
	* FetchPageImpl：从缓冲池得到需要的页
	* UnpinPageImpl：从缓冲池中将目标页unpin
	* FlushPageImpl：将目标页刷新到磁盘（目标页成为脏页后）
	* NewPageImpl：在缓冲池中创造一个新的页
	* DeletePageImpl：从缓冲池中删除一个页
	* FlushAllPagesImpl：将缓冲池所有的页刷新到磁盘
6. 参数说明
	* pool_size_：在缓冲池中页的个数
	* *pages_：页对象，是缓冲池中页的数组
	* *disk_manager_：指向磁盘管理
	* *log_manager_：指向日志管理
	* page_table_：容器页表用来追踪缓冲池中的页，<page_id,frame_id>
	* *replacer_：替换器
	* free_list_：有空页的链表
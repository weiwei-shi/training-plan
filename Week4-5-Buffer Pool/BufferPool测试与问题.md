# Buffer Pool测试说明
## 环境构建
##### 1. 将项目克隆到本地
##### 2. 将本人服务器上的项目目录挂载到Ubuntu容器中  
	docker container run -it -v /home/shiweiwei/bustub:/bustub --name=sww_ubuntu ubuntu /bin/bash
##### 3. 进入容器  
	docker start sww_ubuntu  
    docker exec -it sww_ubuntu /bin/bash
##### 4. 将本地文件传到服务器挂载的目录中  
	scp -r /Users/dscl/Desktop/training-plan-sww/"Week4-5-Buffer Pool"/bustub/ shiweiwei@192.168.2.242:/home/shiweiwei/bustub
##### 5. 进入项目bustub  
	$ build_support/packages.sh
##### 6. 执行命令
	$ mkdir build
	$ cd build
	$ cmake ..
	$ make

## 本地测试
##### 测试
1.为实现单独运行各个测试，进入test目录中，对于待测试的lrureplacer\_test.cpp和buffer\_pool\_manager\_test.cpp文件，将Test中待调试中函数相应的参数中的DISABLED_前缀去掉  
2.测试替换器  

	$ make lru_replacer_test  
	$ ./test/lru_replacer_test --gtest_also_run_disabled_tests  
3.测试缓冲池  

	$ make buffer_pool_manager_test
	$ ./test/buffer_pool_manager_test --gtest_also_run_disabled_tests
4.可以启动调试功能  
将之前`cmake ..`语句换为`cmake -DCMAKE_BUILD_TYPE=DEBUG ..`，再进行`make`
##### 测试结果
![替换器](/images/lru_replacer.png)

![缓冲池](/images/bufferpool_manager.png)  

## 网页测试
##### 账号注册
在https://www.gradescope.com/ 中注册一个账号。6位课程码为：5VX7JZ。这是专供外校人员测试的课程码。  
注册后再重新登陆。
##### 提交代码
将四个文件打包为zip压缩文件，目录应与下面保持一致： 
 
* src/include/buffer/lru_replacer.h
* src/buffer/lru_replacer.cpp
* src/include/buffer/buffer_pool_manager.h
* src/buffer/buffer_pool_manager.cpp
##### 测试结果
![网页测试](images/test.png)

## 遇到的问题
##### 1. 运行脚本的时候出现权限问题  
`build_support/packages.sh: Permission denied`  
解决：`chmod +x build_support/packages.sh`
##### 2. 运行脚本出现和侯磊差不多的问题  
`build_support/packages.sh: /bin/bash^M: bad interpreter: No such file or directory`  
解决：同侯磊
##### 3. 测试过程中显示
	Running main() from gmock_main.cc
	[==========] Running 0 tests from 0 test suites.
	[==========] 0 tests from 0 test suites ran. (0 ms total)
	[  PASSED  ] 0 tests.
	
	YOU HAVE 1 DISABLED TEST
解决：使用命令`./test/lru_replacer_test --gtest_also_run_disabled_tests`


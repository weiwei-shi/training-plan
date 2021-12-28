##一、从远程仓库下载到本地
####1、进入想下载到的文件夹
    cd  ../../..
####2、使用`git clone`下载文件
    git clone git@github.com:weiwei-shi/training-plan.git
	git clone -b training-plan-2021 git@github.com:weiwei-shi/training-plan.git（克隆指定分支）
    注：使用http链接会因为代理出问题，直接使用git@

##二、在本地做修改并上传
####1、进入该文件夹
    cd
####2、连接远程仓库
    git remote add origin https://github.com/weiwei-shi/training-plan.git
	git remote rm origin(删除关联的origin的远程库)
	git remote add origin （地址）（添加远程库）
####3、切换到想要上传的分支
    git checkout training-plan-2021
	git checkout -b newBranch(创建并切换分支）
	git branch -d <branch>（删除分支）
    此时本地文件已切换到对应分支
####4、在本地文件中添加自己的文件
####5、将本地仓库的文件添加到暂缓区
    git add .
####6、提交更改并添加备注信息
    git commit -m "sww commit"
####7、将本地仓库的文件push到远程仓库
    git push origin training-plan-2021

##三、向原仓库提交修改
####1、进入相应分支，点击 `Contribute` 再点击 `Open pull request`
####2、修改提交信息后点击 `Create pull request`
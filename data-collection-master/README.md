# data-collection 

#### 介绍
{**以下是 Gitee 平台说明，您可以替换此简介**
Gitee 是 OSCHINA 推出的基于 Git 的代码托管平台（同时支持 SVN）。专为开发者提供稳定、高效、安全的云端软件开发协作平台
无论是个人、团队、或是企业，都能够用 Gitee 实现代码托管、项目管理、协作开发。企业项目请看 [https://gitee.com/enterprises](https://gitee.com/enterprises)}

#### 软件架构
软件架构说明


#### 安装教程

1.  编译
    cd ./src/tools
    sudo ./build.sh pc  #pc 端编译
    ./build.sh          #sdk 端编译
    生成的文件在 ./src/output/xxxxx.deb
2.  安装
    dpkg -i xxxx.deb
    安装后，系统会reboot,reboot后，app-xxx service会自动启动

3.  应用配置与记录
    /app-cjq/

#### 使用说明

1.  添加其他app与库文件
    在 ./src/tools/deb.sh 修改

        # 程序拷贝,当前目录为 ./src/output/
        # cp yourappdir/yourapp               temp/usr/bin/yourapp
        # cp yourlibdir/libdatabase.so        temp/lib/libdatabase1.so
        # cp yourservicedir/yourapp.service   temp/lib/systemd/system/yourapp.service
    
2.  其他app自动启动
    在 ./src/tools/deb.sh 修改
        #===========1. 开机运行=================================================
        systemctl enable $appname.service
        systemctl enable $daemon.service
        systemctl enable yourapp.service
    
3.  其他app安装后运行脚本
   在 ./src/tools/deb.sh 
        cat >temp/DEBIAN/postinst <<EOF
   添加    

#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request


#### 特技

1.  使用 Readme\_XXX.md 来支持不同的语言，例如 Readme\_en.md, Readme\_zh.md
2.  Gitee 官方博客 [blog.gitee.com](https://blog.gitee.com)
3.  你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解 Gitee 上的优秀开源项目
4.  [GVP](https://gitee.com/gvp) 全称是 Gitee 最有价值开源项目，是综合评定出的优秀开源项目
5.  Gitee 官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)
6.  Gitee 封面人物是一档用来展示 Gitee 会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)


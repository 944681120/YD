#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <sys/syslog.h>
#include "VersionConfig.h"
#include "ftpclient.h"

//程序名字
#define NAME "app-cjq"
//查找进程中程序名字
#define NAME_FIND "app-cjq"
//输出目录
#define DIR_OUT_FILE "/app-cjq/out"
//要运行的程序
#define RUN_NAME "app-cjq &"
int daemon(int nochdir, int noclose)
{
    pid_t pid;
    //让init进程成为新产生进程的父进程
    pid = fork();
    //如果创建进程失败
    if (pid < 0)
    {
        perror("fork");
        return -1;
    }
    //父进程退出运行
    if (pid != 0)
    {
        exit(0);
    }
    //创建新的会话
    pid = setsid();
    if (pid < -1)
    {
        perror("set sid");
        return -1;
    }
    //更改当前工作目录,将工作目录修改成根目录
    if (!nochdir)
    {
        chdir("/");
    }
    //关闭文件描述符，并重定向标准输入，输出合错误输出
    //将标准输入输出重定向到空设备
    if (!noclose)
    {
        int fd;
        fd = open("/dev/null", O_RDWR, 0);
        if (fd != -1)
        {
            dup2(fd, STDIN_FILENO);
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            if (fd > 2)
            {
                close(fd);
            }
        }
    }
    //设置守护进程的文件权限创建掩码
    umask(0027);
    return 0;
}
//是否有匹配的字符，有则返回1，没有返回0
// src:源字符串
// dst：目标字符串
// len:源字符串被比较的长度
int match(char *src, char *dst, int len)
{
    int i = 0;
    int j = 0;
    int size_dst = 0;
    //获得目标字符串的长度
    size_dst = strlen(dst);
    //如果目标字符串的长度大于len，返回失败
    if (size_dst > len)
    {
        return 0;
    }
    //开始比较
    for (i = 0; i < len; i++)
    {
        for (j = 0; j < size_dst; j++)
        {
            if (src[i + j] != dst[j])
            {
                break;
            }
        }
        if (j == size_dst)
        {
            return 1;
        }
    }
    return 0;
}
int main(int argc, char *argv[])
{
    if (argc>1)
    {
        if(strcmp("--version",argv[1])==0)
        {
            printf("%s\n",VERSION_BUILD_TIME);
            return 0;
        }
        return 0;
    }

    int fd = 0;
    char buf[100];
    //开启守护进程
//    daemon(0, 0);
    while (1)
    {
        // //打开日志
        // openlog(argv[0], LOG_CONS | LOG_PID, LOG_USER);
        // //查看程序是否运行
        // //新建输出文件
        // system("touch " DIR_OUT_FILE);
        // //获得程序ID
        // system("ps -w|grep " NAME_FIND " >> " DIR_OUT_FILE);
        // //打开输出文件
        // fd = open(DIR_OUT_FILE, O_CREAT | O_RDONLY, 0777);
        // //清空缓存
        // memset(buf, 0, 100);
        // //读取全部
        // read(fd, buf, 100);
        // //判断是否有程序文件运行
        // if (match(buf, (char*)NAME, 90))
        // {
        //     syslog(LOG_INFO, "%s success!",NAME);
        // }
        // else
        // {
        //     syslog(LOG_INFO, "%s fail!",NAME);
        //     //运行程序
        //     system(RUN_NAME);
        // }
        // //删除输出文件
        // system("rm " DIR_OUT_FILE);        
        //休眠
        printf("wait...\n");
        sleep(5);
        //升级检查
        {
            extern int updata_poll(int interval);
            updata_poll(5);
        }                
    }
    //关闭日志
    closelog();
    return 0;
}
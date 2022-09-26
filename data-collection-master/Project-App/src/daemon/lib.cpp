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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>




//system函数扩展，加入超时值(0表示永久等待)
//超时时返回-2，其他情况返回不变。
int system_ex(const char *cmdstring, unsigned int timeout)   /* with appropriate signal handling */
{
	pid_t               pid;
	int                 status;
	struct sigaction    ignore, saveintr, savequit;
	sigset_t            chldmask, savemask;
 
	//精度换成十分之一秒
	timeout *= 10;
	if (timeout == 0)
		timeout = 0xFFFFFFFF;
 
	if (cmdstring == NULL)
		return(1);      /* always a command processor with UNIX */
 
	ignore.sa_handler = SIG_IGN;    /* ignore SIGINT and SIGQUIT */
	sigemptyset(&ignore.sa_mask);
	ignore.sa_flags = 0;
	if (sigaction(SIGINT, &ignore, &saveintr) < 0)
		return(-1);
	if (sigaction(SIGQUIT, &ignore, &savequit) < 0)
		return(-1);
	sigemptyset(&chldmask);         /* now block SIGCHLD */
	sigaddset(&chldmask, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &chldmask, &savemask) < 0)
		return(-1);
 
	if ((pid = fork()) < 0) {
		return -1;    /* probably out of processes */
	} else if (pid == 0) {          /* child */
		/* restore previous signal actions & reset signal mask */
		sigaction(SIGINT, &saveintr, NULL);
		sigaction(SIGQUIT, &savequit, NULL);
		sigprocmask(SIG_SETMASK, &savemask, NULL);
/*通常exec会放在fork() 函数的子进程部分, 来替代子进程执行啦, 执行成功后子程序就会消失,  但是执行失败的话, 必须用exit()函数来让子进程退出!*/
		execl("/bin/sh", "sh", "-c", cmdstring, (char *)0);/*exec函数会取代执行它的进程,  也就是说, 一旦exec函数执行成功, 它就不会返回了, 进程结束.   但是如果exec函数执行失败, 它会返回失败的信息,  而且进程继续执行后面的代码!*/
		_exit(127);     /* exec error */
	}
 
	/* parent */
	int ret = 0;
	while (timeout-- > 0 &&
		(ret = waitpid(pid, &status, WNOHANG)) == 0)
		usleep(100*1000);
 
	/* restore previous signal actions & reset signal mask */
	if (sigaction(SIGINT, &saveintr, NULL) < 0)
		return(-1);
	if (sigaction(SIGQUIT, &savequit, NULL) < 0)
		return(-1);
	if (sigprocmask(SIG_SETMASK, &savemask, NULL) < 0)
		return(-1);
 
	if (ret < 0)
		return -1;
 
	if (ret > 0)
		return status;
 
	kill(pid, SIGKILL);
	waitpid(pid, &status, 0);
	return -2;
}

//1. 
/**
 *@brief          执行系统命令，并返回输出的结果
 *@param[in]      cmdstring，命令串
 *@param[in]      buf，存放命令结果的缓冲区
 *@param[in]      size，缓冲区的大小
 *@param[out]     
 *@return         返回写入到buf中的字符串长度，不含\0 ; -1: 失败;
 *@remark         
 *@version        V1.0.0
 *@note     	  buf中最多返回size-1个字符，字符串始终以\0结尾。
*/
int get_cmd_results(const char *cmdstring, char *buff, int size)
{
	char cmd_string[200] = {0};
	char tmpfile[100] = {0};
	char tmp_buf[100] = {0};
	int fd;
	int tmp_fd;
	int nbytes;
 
    memset(buff, 0, size);
 
	if((cmdstring == NULL) ||
		(strlen(cmdstring) > (sizeof(tmpfile) + 8)) ||
		((strlen(cmdstring) + strlen(tmpfile) + 5) > sizeof(cmd_string)))
	{
        printf("cmd is too long or NULL!\n");
        return -1;
	}
	sscanf(cmdstring, "%[a-Z]", tmp_buf);/*%[a-z] 表示匹配a到z中任意字符，贪婪性(尽可能多的匹配) */
	sprintf(tmpfile, "/tmp/%s-XXXXXX", tmp_buf);
 
	tmp_fd = mkstemp(tmpfile);
	if(tmp_fd < 0)
    {
		printf("mkstemp failed\n");
		return -1;
	}
	close(tmp_fd);
 
	sprintf(cmd_string, "%s > %s 2>&1", cmdstring, tmpfile);/*标准输出（1），标准错误（2）都输出到临时文件*/
	if(system_ex(cmd_string, 20) < 0)
    {
		printf("run \"%s\" ret < 0!\n", cmd_string);
	}
 
	fd = open(tmpfile, O_RDONLY);
	if(fd < 0)
    {
		printf("open %s failed!\n", tmpfile);
		nbytes = -1;
	}
	else
	{
		nbytes = read(fd, buff, size - 1);
		close(fd);
	}
 
	memset(cmd_string, 0, sizeof(cmd_string));
	sprintf(cmd_string, "rm -rf /tmp/%s-*", tmp_buf);
	system_ex(cmd_string, 20);
 
	return nbytes;
}


#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

/*dir operation*/
int mk_dir(char *dir)
{
#define MODE (S_IRWXU | S_IRWXG | S_IRWXO)
    DIR *mydir = NULL;
    if ((mydir = opendir(dir)) == NULL) //判断目录
    {
        int ret = mkdir(dir, MODE); //创建目录
        if (ret != 0)
        {
            return -1;
        }
        printf("[mk_dir]:%s created sucess!\n", dir);
    }
    else
    {
        return 2;
        //INFO("[mk_dir]:%s exist!\n", dir);
    }
    return 0;
}
int mk_all_dir(char *dir)
{
    bool flag = true;
    char *pDir = dir;
    while (flag)
    {
        char *pIndex = index(pDir, '/');
        if (pIndex != NULL && pIndex != dir)
        {
            char buffer[512] = {0};
            int msg_size = pIndex - dir;
            memcpy(buffer, dir, msg_size);
            int ret = mk_dir(buffer);
            if (ret < 0)
            {
                printf("[mk_all_dir]:->%s created failed!\n", dir);
            }
        }
        else if (pIndex == NULL && pDir == dir)
        {
            printf("dir is not directory!\n");
            return -1;
        }
        else if (pIndex == NULL && pDir != dir)
        {
            int ret = mk_dir(dir);
            if (ret < 0)
            {
                printf("[mk_all_dir]:-->%s created failed!\n", dir);
            }
            break;
        }
        pDir = pIndex + 1;
    }
    return 0;
}

char *get_cur_time()
{
    static char s[32] = {0};
    time_t t;
    struct tm* ltime;
    struct timeval stamp;
    gettimeofday(&stamp, NULL);
    ltime = localtime(&stamp.tv_sec);
    s[0] = '[';
    strftime(&s[1], 20, "%Y-%m-%d %H:%M:%S", ltime);
    sprintf(&s[strlen(s)], ".%03ld]", (stamp.tv_usec/1000));
    return s;
}
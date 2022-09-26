
#ifndef __DAEMON_LIB_H__
#define __DAEMON_LIB_H__

//通用头文件
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>

int get_cmd_results(const char *cmdstring, char *buff, int size);

/*dir operation*/
int mk_dir(char *dir);
int mk_all_dir(char *dir);

// 方法3
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
char *get_cur_time();

#define INFO(fmt, ...) printf("%s %s %d" fmt,"Msg ",get_cur_time(),(int)gettid(),##__VA_ARGS__ )
#define ERROR(fmt, ...) printf("%s %s" fmt,"Err ",get_cur_time(),##__VA_ARGS__ )


#endif
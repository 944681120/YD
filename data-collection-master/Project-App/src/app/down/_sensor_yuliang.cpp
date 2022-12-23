
#include <stddef.h>
#include <stdint.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include "lib.h"

extern void rain_caculation(int yuliangzengliang);

#define __DI_BASE 'I' /* 内部宏定义 */

#define DI_IOC_SET_DEBOUNCE _IOW(__DI_BASE, 0, unsigned int)
#define DI_IOC_GET_DEBOUNCE _IOR(__DI_BASE, 1, unsigned int)

typedef struct
{
    unsigned long long time;
    unsigned int value;
} di_read_cfg_s; /* 内部结构体 */

di_read_cfg_s ucDioCfg;

int di_get(char *name)
{
    static di_read_cfg_s last = {0, 0};
    if (last.time == last.value && last.time == 0)
    {
        last = ucDioCfg;
    }
    if (last.value != ucDioCfg.value)
    {
        last = ucDioCfg;
        if (ucDioCfg.value == 0)
        {
            INFO("[di-get]:%s  低脉冲", name);
            rain_caculation(1);
        }
    }
    return 0;
}

int di_open(char *pFilePath)
{
    int fd = 0;
    if (0 == strncmp(pFilePath, "/dev/di", strlen("/dev/di")) || 0 == strncmp(pFilePath, "/dev/plugin/di", strlen("/dev/plugin/di")))
    {
        /* 打开文件 */
        fd = open(pFilePath, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd < 0)
        {
            // ERROR("[di-open] %s : open fail", pFilePath);
            return -2;
        }
        // INFO("[di-open]:fd=%d. open OK = %s",fd, pFilePath);
        return fd;
    }
    return -1;
}

int di_close(int fd)
{
    close(fd);
    return 0;
}

int di_read(int fd, char *name)
{
    int iDataLen = read(fd, &ucDioCfg, sizeof(ucDioCfg));
    if (0 > iDataLen)
    {
        ERROR("[di-read] %s : open fail", name);
        return -1;
    }
    INFO("[di-read] time=[%d]   value=[%d]", (int)ucDioCfg.time, (int)ucDioCfg.value);
    return 1;
}
int di_poll(int fd, char *name)
{
    int ret = 0;
    fd_set rfds;
    int maxfd;
    /* 读数据 */
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    maxfd = fd;
    int lastValue = 0;

    while (1)
    {
        // INFO("[di-poll]: select. name = %s", name);
        ret = select((int)maxfd + 1, &rfds, NULL, NULL, NULL);
        if (ret <= 0)
        {
            INFO("[di-poll] no data. name=%s", name);
            return -1;
        }

        if (FD_ISSET(fd, &rfds))
        {
            int iDataLen = read(fd, &ucDioCfg, sizeof(ucDioCfg));
            if (0 > iDataLen)
            {
                ERROR("[di-poll] %s : open fail", name);
                return -1;
            }
            if ( lastValue != ucDioCfg.time )
            {
                lastValue = ucDioCfg.time;
                if( (0xffff & ucDioCfg.time) == 0x3031 || (0xffff & ucDioCfg.time) == 0x3030 )
                {
                    ucDioCfg.value = (0xffff & ucDioCfg.time) - 0x3030;    //A5303
                    ucDioCfg.time = std::time(nullptr);
                }
                INFO("[di-poll] time=[%d]   value=[%d]", (int)ucDioCfg.time, (int)ucDioCfg.value);
                di_get(name);
            }

        }
        else
        {
            printf("[di-poll]FD_ISSET empty");
        }
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
    }
}

//===============雨量计数器==========================
#include "app.h"
#include "lib.h"
void *yuliang_thread(void *arg)
{
    static int lastfd = 0;
    static int lastret = 0;
    INFO("[yuliang_thread] is running...");
    while (1)
    {
        int ret = 0;
        // 1.open
        int fd = di_open((char *)"/dev/di1");
        if (fd > 0)
        {
            ret = di_poll(fd, (char *)"di1");
        }

        // print for error
        if (lastret != ret)
        {
            lastret = ret;
            INFO("[di-poll]: exit. ret=%d ", ret);
        }
        if (lastfd != fd)
        {
            lastfd = fd;
            if (lastfd == -2)
            {
                ERROR("[di-open] %s : open fail", "/dev/di1");
            }
        }
        sleep(1);
        di_close(fd);
        fd = -1;
    }
}

pthread_t thread_yu_init(void)
{
    pthread_t id;
    // mxserial.init((char*)"com1",NULL,(void*)down_packet_process);
    // id = mxserial.open((void*)"/dev/ttyS1");
    if (pthread_create(&id, NULL, yuliang_thread, NULL) != 0)
    {
        ERROR("[线程创建]:ERR =  yuliang_thread.");
        return 0;
    }
    INFO("[线程创建]:OK = yuliang_thread.");
    return id;
}

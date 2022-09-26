
#include "tcp_client.h"
#include "mxdevice.h"
#include "report.h"
#include "sendmode.h"
#include "rtu_setting.hpp"
#include "lib.h"
#include "unistd.h"

void *thread_up(void *arg);

MxDeviceTcpClient client;
MxDeviceSerialPort serial;

// init
pthread_t thread_up_init(void)
{
    pthread_t id;
    if (pthread_create(&id, NULL, thread_up, NULL) != 0)
    {
        ERROR("[thread_up_init]: can not create thread_up");
        return 0;
    }
    INFO("[thread_up_init]:create thread:thread_up");
    return id;
}

/*=================================================================
    接收数据包处理,请异步
===================================================================*/
extern int center_process(SendMode *s, UDPacket *packet);
void down_packet_process(void *arg)
{
    u8 etx = 0;
    SendMode *s = (SendMode *)arg;
    UDpacketCache *packet = &s->packet;
    etx = s->packet.p.etx;

    if (packet->p.len >= 2)
    {
        u16 liushui = 0;
        liushui |= packet->p.data[0];
        liushui <<= 8;
        liushui |= packet->p.data[1];
        //set_current_liushui(liushui);
    }
    s->packet_flag = true;
    // center_process(s,&packet->p);
    s->etx = (ETX)etx;
}

/*=================================================================
    远程探测
===================================================================*/
//  主：0x04,0x06,0x08,0x0A
//备用 :0x05,0x07,0x09,0x0B
static char ipport[8][50];
static u8 remoters[4];
char *get_ip_port(int id, u8 *c)
{
    if (id < 0x04 || id > 0x0B)
        return NULL;
    int index = id - 0x04;
    if (index > rtu.remote.size())
        return NULL;
    if ((index / 2) > rtu.center.size())
        return NULL;
    *c = rtu.center[index / 2];
    return (char *)rtu.remote[index].c_str();
}

int get_yaosu(YaoSu *yaosu)
{
    u8 da[16];
    memcpy(da, param[0x0d].hex, 8);
    memcpy(yaosu, da, 8);
    return 8;
}
// 0 等待  1:ok  -1:error
int get_sendmode(SendMode **sm)
{
    // 1.判断tcp状态
    if (tcp_sendmode.isopen())
    {
        *sm = &tcp_sendmode;
        return 1;
    }

    // 2.等待已经配置的链接
    switch (tcp_sendmode.get_device_state())
    {
    //已经关闭-》重连
    case DS_NULL:
        break;
    case DS_CLOSE:
        break;
    //正在链接-》等待
    case DS_CONNECTING:
        return 0;
    //应经打开->返回
    case DS_OPEN:
    {
        *sm = &tcp_sendmode;
        return 1;
    }
    //等待
    case DS_WAIT:
        return 0;
    }

    // 3.链接失败，重新尝试链接
    // for(int i=0;i<8;i++)
    for (int i = 0; i < 8; i++)
    {
        u8 center = 0;
        char *ip = get_ip_port(0x04 + i, &center);
        if (ip != NULL)
        {
            //尝试链接
            tcp_sendmode.config(&client, NULL, (void *)ip);
            tcp_sendmode.center = &down_packet_process;
            client.keepalive(true);
            tcp_sendmode.connect();
            set_current_remote(center);
            return 0;
        }
    }
    return -1;
}


// 上报处理
void thread_up_poll(void)
{
    zhudong_report_process();
    if (tcp_sendmode.packet_flag)
    {
        center_process(&tcp_sendmode, &tcp_sendmode.packet.p);
        tcp_sendmode.packet_flag = false;
    }

    if (serial_sendmode.packet_flag)
    {
        center_process(&serial_sendmode, &serial_sendmode.packet.p);
        serial_sendmode.packet_flag = false;
    }
}

void *thread_up(void *arg)
{
    SendMode *sm = NULL;
    get_sendmode(&sm);

    /*----------------------232----------------------------------*/
    char rate[20];
    char sport[50];
    sprintf(rate, "%d", rtu.bdBaud);
    serial_sendmode.config(&serial, (void *)rtu.bdSerialPort.data(), (void *)rate);
    // serial_sendmode.config(&serial,(void*)"/dev/ttyRS1",(void*)"115200");
    serial_sendmode.center = &down_packet_process;
    serial_sendmode.connect();

    //初始化驱动
    //注册监听函数
    while (1)
    {
        thread_up_poll();
        usleep(300000);
    }
}

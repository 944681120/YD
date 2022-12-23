#pragma once 
#ifndef __DFSM_H__
#define __DFSM_H__
#include <iostream>
#include "mxdevice.h"



//用于控制 与 设备 交互过程
class dfsm
{
public:    
    //收发缓冲
    u8 sb[200];
    int slen = 0;
    u8 rb[200];
    int rlen = 0;
    bool getpacket = false;
    int wann = 0;
    int ATflag = 0;     //AT指令 1接收到 2接收完毕
    int ATlen = 0;
    u8 ATbuf[256] = {0};
    int longflag = 0;   //接收长数据 1接收到 2接收完毕
    int longlen = 0;
    u8 longbuf[256] = {0};
    //状态机
    int step;
    long ms_timeout;
    long ms_timeout_max = 2000;
    int stimeout = 20;
    int retry = 0;
    //设备
    MxDeviceSerialPort dev;

    //超时处理
    Callback *timeout;
    Callback *packet;

    //speed
    int speed;

    dfsm();
    ~dfsm();
    int start(void*arg,int rate);
    int init(u8*b,int l,int retries, Callback* to, Callback* p);
    int process(void);
    int setbaud(int rate);
};

typedef enum{
    DFSM_STATE_IDLE = 0,
    DFSM_STATE_SEND,
    DFSM_STATE_WAIT,
    DFSM_STATE_TIMEOUT,
    DFSM_STATE_GET,
}DFSM_STATE;

// extern dfsm ddeviceRS1;
// extern dfsm ddeviceRS2;

extern dfsm *dfsm_device_find(string s);
extern void dfsm_device_init(void);
 //下行通道
//  dfsm ddevice;

// //==================数据包================//
// typedef enum{
//     DPCMD_READ_COIL_STATUS=0x01     ,//H 读取线圈状态
//     DPCMD_READ_INPUT_STATUS=0x02    ,//H 读取输入状态
//     DPCMD_READ_HOLDING_REGISTER=0x03,//H 读取保持寄存器
//     DPCMD_READ_INPUT_REGISTER=0x04  ,//H 读取输入寄存器 
//     DPCMD_WRITE_A_COIL=0x05         ,//H 写单个线圈 
//     DPCMD_WRITE_A_REGISTER=0x06     ,//H 写单个寄存器 
//     DPCMD_READ_EXCEPTION_STATUS=0x07,//H 读取异常状态
//     DPCMD_SEND_CHECKSUM=0x08        ,//H 回送诊断校验
//     DPCMD_READ_EVENT_COUNTER=0x0B   ,//H 读取事件计数
//     DPCMD_WRITE_COILDS=0x0F         ,//H 写多个线圈
//     DPCMD_WRITE_REGISTERS=0x10      ,//H 写多个寄存器
//     DPCMD_REPORT_DEVID=0x11         ,//H 报告从机标识
//     DPCMD_RESET_COM=0x13            ,//H 重置通信链路
// }DPCMD;


#endif
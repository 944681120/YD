#pragma once 

#ifndef __MY_DEVICE_H__
#define __MY_DEVICE_H__
#include "lib.h"

typedef int *onreceive(void *device, void *buffer, int len);

using namespace std;

typedef enum{
    DS_NULL = 0,
    DS_INITED,
    DS_CONNECTING,
    DS_OPEN,
    DS_WAIT,
    DS_CLOSE,
    DS_ERROR,
}DeviceState;


//上行通道驱动接口
#include "serialport.h"
#include "tcp_client.h"
class MxDevice
{
public:
    /*-------device------------*/
    char *name;  //设备名
    int fd;      //设备符
    void *pdata; //设备数据
    /*-------tx----------------*/
    u8 tx[1024]; //发送缓冲
    int txlen;
    int txoffset;
    int txflag;
    /*-------rx----------------*/
    u8 rx[1024]; //接收缓冲
    int rxlen;
    int rxoffset;
    int rxflag;

    /*-------operate----------*/
    void *receive_process;
    virtual int init(char *n, void *pd, void *r) = 0;
    virtual int config(void *arg) = 0;
    virtual int open(void *arg) = 0;
    virtual int close(void *arg) = 0;
    virtual int keepalive(bool k) = 0;
    virtual int send(void *buffer, int len, int sms,int rms) = 0;
    virtual bool is_keepalive(void)=0;
    virtual bool is_send_end(void) = 0;
    virtual void set_rx_arg(void*arg)=0;
    virtual bool is_open(void)=0;
    virtual DeviceState device_state(void)=0;
};

#include "tcp_client.h"
class MxDeviceSerialPort : public MxDevice
{

public:
    SerialPort s;
    int init(char *n, void *pd, void *r);
    int config(void *arg);
    int open(void *arg);
    int close(void *arg);
    int keepalive(bool k);
    int send(void *buffer, int len, int sms,int rms);
    bool is_keepalive(void);
    bool is_send_end(void);
    void set_rx_arg(void*arg);
    bool is_open(void);
    DeviceState device_state(void);
};

class MxDeviceTcpClient : public MxDevice
{

public:
    tcp_client s;
    int init(char *n, void *pd, void *r);
    int config(void *arg);
    int open(void *arg);
    int close(void *arg);
    int keepalive(bool k);
    int send(void *buffer, int len, int sms,int rms);
    bool is_keepalive(void);
    bool is_send_end(void);
    void set_rx_arg(void*arg);
    bool is_open(void);
    DeviceState device_state(void);
};

#endif

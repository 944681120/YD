#pragma once
#ifndef __SENDMODE_H__
#define __SENDMODE_H__
#include "lib.h"
#include "upacket.h"
#include "mxdevice.h"
#include "MultPacket.hpp"

typedef enum
{
    M1 = 5, //发送，不应答
    M2,     //发送,应答
    M3,
    M4,
} MXType;

/*-----------------------M1,M2 M3 M4 过程 --------------------*/
class SendMode
{
private:
    MPBase *_imp;
    int get_sn_size(int sn);
    int m3_send(int sn, int ms);

public:
    SendMode(/* args */);
    ~SendMode();

    void *devstring;
    void *devarg;

    MxDevice *dev;
    MXType type;
    u8 *data;
    int len;
    int psize;
    int packet_count = 0;

    UPCMD cmd;
    u8 remote;

    bool packet_flag = false;
    UUPacket up;
    UDpacketCache down;
    UDpacketCache packet;

    Callback *cb_timeout;
    Callback *cb_getpacket;

    Callback *center;

    void *recv_process;

    bool get_packet = false;

    void config(MxDevice *mx, void *ds, void *da);
    bool connect(void);
    bool isopen(void);
    void init(MXType _type, u8 _remote, UPCMD _cmd, u8 *_data, int _len, int _size_per_packet);
    void initMul(MXType _type, u8 _remote, UPCMD _cmd, MPBase *im3);
    int wait(int ms, Callback *to, Callback *get);
    void onsend(UUPacket *sp, bool ok);

    DeviceState get_device_state(void);

    //============控制状态===================
    ul64 utimeout = 0; //ms
    ETX etx;

    //============序列号
    u16 seial_number = 1;
};

#include "beidou.h"

extern class SendMode tcp_sendmode;
extern class SendModeBeiDou serial_sendmode;
#endif
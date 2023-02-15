#ifndef __BEIDOU_H__
#define __BEIDOU_H__

#pragma once

//北斗数据上报，打包与解包
class MxDeviceBeidou : public MxDeviceSerialPort
{
private:
    uint8_t MsgSend_485[100] = {0};
    uint32_t BD_Add = 000000; //北斗中心站地址
public:
    int send(void *buffer, int len, int sms, int rms);
};

//========================================数据包解包================================================//
// //格式: $TXXX[包长2H,L][目的用户地址3][信息类别1][发送用户地址3][时间2 Hour min][报文长度2][报文内容n][crc1][检验和]
// static uint8_t txxx_buf[1024];
// static int txxx_len = 0;
// static int txxx_step;
// static int reset_txxx()
// {
//     txxx_len = txxx_step = 0;
//     return -1;
// }
// #pragma pack(1)
// struct TXXX
// {
//     char Txxx[5];
//     uint8_t H;
//     uint8_t L;
//     uint8_t user[3];
//     uint8_t type;
//     uint8_t remote[3];
//     uint8_t hour;
//     uint8_t min;
//     uint8_t payloadH;
//     uint8_t payloadL;
//     uint8_t payload[300];
// };
// #pragma pack()

extern int beidou_txxx(char data);
//按 TXXX 方式解包
extern void *up_beidou_packet_process(void *ss, u8 *data, int len);
class SendModeBeiDou : public SendMode
{
public:
    void config(MxDevice *mx, void *ds, void *da);
};
extern SendModeBeiDou serial_sendmode;

#endif

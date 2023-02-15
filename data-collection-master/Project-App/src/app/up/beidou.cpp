
#include "tcp_client.h"
#include "mxdevice.h"
#include "report.h"
#include "sendmode.h"
#include "rtu_setting.hpp"
#include "lib.h"
#include "unistd.h"
#include "beidou.h"
static uint8_t wubJTT808CalculateChecksum(uint8_t *aubData_p, uint8_t auwDataLength, uint8_t Num_sat)
{
    uint8_t aubChecksum = 0;
    uint8_t auwCnt = Num_sat; //跳过开头

    while (auwCnt < auwDataLength)
    {
        aubChecksum ^= aubData_p[auwCnt];
        auwCnt++;
    }
    //		printf("aubChecksum%0*x\r\n",2,aubChecksum);
    return aubChecksum;
}

int MxDeviceBeidou::send(void *buffer, int len, int sms, int rms)
{

    if (len > 78)
    {
        ERROR("[北斗] 长度超过78,发送失败 len=%d", len);
        return 0;
    }
    char addr[10] = {0};
    str2hexarray(rtu.param.bdAddr.c_str(), rtu.param.bdAddr.size() > 6 ? 6 : rtu.param.bdAddr.size(), addr, 10);
    BD_Add = *((uint32_t *)addr);

    int revaddr= atol(rtu.param.bdAddr.c_str());

    //发送时打包为北斗数据 4.0 协议
    uint8_t BD_Length = 0;
    const char *TXSQ = "$TXSQ";
    uint8_t BD_add_temp, Xor_temp;
    uint16_t BD_data_bit = 0;
    uint8_t *Send_Buf = (uint8_t *)buffer;
    //1.头部
    for (int i = 0; i < 5; i++)
        MsgSend_485[BD_Length++] = TXSQ[i]; //头部
                                            /*--------4.0协议-------------*/
    //2.//长度（全部数据）
    MsgSend_485[BD_Length++] = 0;
    MsgSend_485[BD_Length++] = 0;

    //3.地址
    BD_add_temp = (BD_Add >> 16) & 0x000000ff;
    MsgSend_485[BD_Length++] = BD_add_temp;
    BD_add_temp = (BD_Add >> 8) & 0x000000ff;
    MsgSend_485[BD_Length++] = BD_add_temp;
    BD_add_temp = BD_Add & 0x000000ff;
    MsgSend_485[BD_Length++] = BD_add_temp;

    MsgSend_485[BD_Length++] = 0x46; //代码/混发

    BD_add_temp = (revaddr >> 16) & 0x000000ff;
    MsgSend_485[BD_Length++] = BD_add_temp;
    BD_add_temp = (revaddr >> 8) & 0x000000ff;
    MsgSend_485[BD_Length++] = BD_add_temp;
    BD_add_temp = revaddr & 0x000000ff;
    MsgSend_485[BD_Length++] = BD_add_temp;

    //4.报文bits
    BD_data_bit = len * 8;
    MsgSend_485[BD_Length++] = (BD_data_bit >> 8) & 0x00ff; //电文长度bit
    MsgSend_485[BD_Length++] = BD_data_bit & 0x00ff;        //电文长度bit

    MsgSend_485[BD_Length++] = 00;
    /*--------4.0协议-------------*/

    //5.正文数据
    for (int i = 0; i < len; i++)
    {
        MsgSend_485[BD_Length++] = Send_Buf[i];
    }
    //6.长度
    MsgSend_485[6] = BD_Length + 1;
    //7. 加最后一个字节，CRC
    Xor_temp = wubJTT808CalculateChecksum(MsgSend_485, BD_Length, 0);
    MsgSend_485[BD_Length++] = Xor_temp; //校验
    INFO("[>> 北斗] 上报长度=%d data=%s", BD_Length, hexarray2string((char *)MsgSend_485, BD_Length).c_str());
    return MxDeviceSerialPort::send((void *)MsgSend_485, BD_Length, sms, rms);
}

//========================================数据包解包================================================//
//格式: $TXXX[包长2H,L][目的用户地址3][信息类别1][发送用户地址3][时间2 Hour min][报文长度2][报文内容n][crc1][检验和]
uint8_t txxx_buf[1024];
volatile int txxx_len = 0;
volatile int txxx_step = 0;
volatile ul64  txxx_timeout = 0;
static int reset_txxx()
{
    txxx_len = txxx_step = 0;
    return -1;
}
#pragma pack(1)
struct TXXX
{
    char Txxx[5];
    uint8_t H;
    uint8_t L;
    uint8_t user[3];
    uint8_t type;
    uint8_t remote[3];
    uint8_t hour;
    uint8_t min;
    uint8_t payloadH;
    uint8_t payloadL;
    uint8_t payload[300];
};
#pragma pack()

int beidou_txxx(char _data)
{
    //
    uint8_t header[]={'$','T','X','X','X'};
    uint8_t data = (uint8_t)_data;

    //超时丢掉
    if(txxx_timeout==0 || ((txxx_timeout+2000)< get_ms_clock()))
    {
        txxx_timeout = get_ms_clock();
        reset_txxx();
    }
    //过长丟掉
    if (txxx_len >= sizeof(txxx_buf))
    {
        reset_txxx();
    }
    txxx_buf[txxx_len] = (uint8_t)data;
    txxx_len++;
    //INFO("bd: %02X",data);
    switch (txxx_step)
    {
    //"$TXXX"
    case 0:
        if (txxx_len < 6)
        {
            if (header[txxx_len-1] != data)
                return reset_txxx();
        }
        if (txxx_len == 5)
            txxx_step = 5;
        break;
    //H,L
    case 5:
    case 6:
        txxx_step++;
        break;
    case 7:
    {
        //长度OK
        struct TXXX *ptxxx = (TXXX *)txxx_buf;
        uint16_t _len = ptxxx->H;
        _len <<= 8;
        _len |= ptxxx->L;
        if (_len <= txxx_len)
        {
            INFO("[<< 北斗]:len=%d data=%s", txxx_len, hexarray2string((char *)txxx_buf, txxx_len).c_str());
            int rl = txxx_len;
            reset_txxx();
            return rl;
        }
        break;
    }
    default:
        return reset_txxx();
    }
    return 0;
}

//按 TXXX 方式解包
void *up_beidou_packet_process(void *ss, u8 *data, int len)
{
    for (int i = 0; i < len; i++)
    {
        int result = 0;
        if ((result=beidou_txxx((char)data[i])) > 0)
        {
            SendMode *s = (SendMode *)ss;

            //数据包解码
            if (upacket_decode(&s->down, txxx_buf, result, false) != NULL)
            {
                s->get_packet = true;
                s->packet = s->down;
                //大小端转换: 正文在处理时再转化
                inverse((u8 *)&s->packet.p.len, 2);
                inverse((u8 *)&s->packet.p.local, 5);
                inverse((u8 *)&s->packet.p.passwd, 2);

                INFO("[up_packet_process]:get a packet: cmd=%02x,len =%d", s->packet.p.cmd, s->packet.p.len & 0xFFF);
                // center 处理数据包
                if (s->center)
                {
                    s->center(s);
                }
            }
            upacket_decode(&s->down, NULL, 0, true);
            reset_txxx();
            return (void *)1;
        }
    }

    return NULL;
}

void SendModeBeiDou::config(MxDevice *mx, void *ds, void *da)
{
    this->dev = mx;
    this->recv_process = (void *)up_beidou_packet_process;
    mx->pdata = this;
    mx->receive_process = (void *)up_beidou_packet_process;
    mx->set_rx_arg(this);

    mx->config(da);
    this->devarg = da;
    this->devstring = ds;
}
SendModeBeiDou serial_sendmode;

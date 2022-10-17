
#include "dfsm.h"

dfsm::dfsm() {}
dfsm::~dfsm() {}

/*================================================================*/
// read data packet process
void *down_packet_process(void *ss, u8 *data, int len)
{
    dfsm *f = (dfsm *)ss; //&ddevice; //(dfsm*)ss;
    MxDeviceSerialPort *mx = (MxDeviceSerialPort *)&f->dev;
    INFO("getdata,len=%d", len);

    // copy data
    if ((f->rlen + len) > sizeof(f->rb))
        len = sizeof(f->rb) - f->rlen;
    if (len)
    {
        memcpy(f->rb + f->rlen, data, len);
        f->rlen += len;
        if (f->rlen >= f->wann)
        {
            PRINTBYTES("down get packet", f->rb, f->rlen);
            f->getpacket = true;
        }
    }
    mx->s.uart_receive.temp_ptr = 0;
    mx->s.uart_receive.data_len = 0;
    return (void *)f;
}

int dfsm::start(void *arg, int rate)
{
    dev.init((char *)"com1", (void *)this, (void *)down_packet_process);
    dev.s.baudrate = rate;
    if (this->dev.open(arg) < 0)
    {
        ERROR("无法打开 %s", (char *)arg);
        return -1;
    }
    this->speed = rate;
    return 0;
}

int dfsm::init(u8 *b, int l, int retries, Callback *to, Callback *p)
{
    if (l > 0)
    {
        if (l > sizeof(this->sb))
            l = sizeof(this->sb);
        memcpy(this->sb, b, l);
        this->slen = l;
    }

    this->rlen = 0;
    memset(this->rb, 0, sizeof(this->rb));
    this->rlen = 0;
    this->timeout = to;
    this->packet = p;
    this->getpacket = false;
    this->retry = retries;
    step = DFSM_STATE_SEND;
    return 0;
}

int dfsm::process(void)
{
    switch (this->step)
    {
    case DFSM_STATE_IDLE:
        break;
    //下发
    case DFSM_STATE_SEND:
    {
        dev.send((void *)this->sb, this->slen, this->stimeout, 0);
        ms_timeout = ms_timeout_max + get_ms_clock();
        step = DFSM_STATE_WAIT;
    }
    break;

    //等待超时
    case DFSM_STATE_WAIT:
    {
        if (ms_timeout < get_ms_clock())
        {
            if (retry > 0)
            {
                retry--;
                step = DFSM_STATE_SEND;
                break;
            }
            else
            {
                step = DFSM_STATE_TIMEOUT;
                break;
            }
        }
        if (this->getpacket)
        {
            this->packet((void *)this);
            this->getpacket = false;
            step = DFSM_STATE_GET;
        }
    }
    break;
    case DFSM_STATE_GET:
        step = DFSM_STATE_IDLE;
        break; //收到数据
    case DFSM_STATE_TIMEOUT:
        this->timeout((void *)this);
        step = DFSM_STATE_IDLE;
        break; //超时
    default:
        step = DFSM_STATE_IDLE;
        break;
    }
    return 0;
}

int dfsm::setbaud(int rate)
{
    char speed[50];
    if(this->speed == rate)return 0;
    sprintf(speed, "%d", rate);
    return this->dev.config((void *)speed);
}

//dfsm ddeviceRS1;
//dfsm ddeviceRS2;

//=============================动态创建========================//
#include "rtu_setting.hpp"
#include <map>
using namespace std;
map<string, dfsm*> mddevices;
dfsm *dfsm_device_find(string s)
{
    map<string, dfsm*>::iterator iter;
    iter = mddevices.find(s);
    if (iter == mddevices.end())
        return NULL;
    else
        return (dfsm *)iter->second;
}

void dfsm_device_init(void)
{
    for (int i = 0; i < rtu.device.rs485.size(); i++)
    {
        const char* p = rtu.device.rs485[i].port.data();
        int band = rtu.device.rs485[i].baud;
        if (dfsm_device_find(p) == NULL)
        {
            dfsm* pd = new dfsm();
            int result = pd->start((void*)p, band);
            mddevices[p] = pd;
            INFO("[初始化]: %s dev = %s baud = %d  ",((result==0)?"OK":"Error"), p, band);
        }else{
            INFO("[初始化]:dev = %s already exist.",p);
        }
    }
}

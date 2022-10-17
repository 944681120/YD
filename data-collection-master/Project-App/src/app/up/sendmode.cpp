
#include "sendmode.h"
#include "upacket.h"
#include "unistd.h"

SendMode::SendMode() {}
SendMode::~SendMode() {}

void *up_packet_process(void *ss, u8 *data, int len)
{
    SendMode *s = (SendMode *)ss;

    // INFO("[up_packet_process]:get data,len = %d :",len);
    // {
    //     int i = 0;
    //     u8*d = data;
    //     while (i<len)
    //     {
    //         printf("%02x",d[i]);i++;
    //     }
    //     printf("");
    // }
    //数据包解码
    if (upacket_decode(&s->down, data, len, false) != NULL)
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

    return (void *)1;
}

DeviceState SendMode::get_device_state(void)
{
    if (this->dev == NULL)
        return DS_NULL;
    return this->dev->device_state();
}

int SendMode::get_sn_size(int sn)
{
    // sn = 1----packet_count
    if (sn == 0)
        return 0;
    if (sn == this->packet_count)
    {

        return this->len - ((sn - 1) * this->psize);
    }
    else
    {
        return this->psize;
    }
}

int SendMode::m3_send(int sn, int ms)
{
    int nowlen = get_sn_size(sn);
    u8 *d = NULL;
    if (_imp == NULL)
    {
        d = &this->data[(sn - 1) * this->psize];
    }
    else
    {
        //读取数据
        int actlen = _imp->reportdata((sn - 1) * this->psize, nowlen, _imp->buffer);
        if (actlen == 0)
        {
            ERROR("_imp,读取不到数据发送");
            return 0;
        }
        //分包发送
        // u8 *d = &this->data[(sn - 1) * this->psize];
        nowlen = actlen;
        d = (u8 *)_imp->buffer;
    }

    this->get_packet = false;

    // 采用 HEX 码。高 12 位表示包总数，低 12 位表示本次发 送数据包的序列号，范围为 1～4095。
    int total = this->packet_count;
    int dlen = upacket_encodehex_m3(&this->up, true, this->remote, this->cmd, total, sn, d, nowlen,
                                    (this->packet_count == 1) ? STX_STX : STX_SYN,
                                    (sn == this->packet_count) ? ETX_ETX : ETX_ETB);
    INFO("[M3] send packet: size=%d, cmd=%s", dlen, cmd_to_string(this->up.cmd));
    this->dev->send((void *)&this->up, this->up.packetlen, ms, ms);
    usleep(100000); // 100ms
    //等待发送完成
    int t = 0;
    while (t < ms)
    {
        usleep(10000);
        t += 10;
        if (this->dev->is_send_end())
            break;
    }
    //超时
    if (this->dev->is_send_end() == false)
        return -1; //链路层超时
    return 0;
}

void SendMode::config(MxDevice *mx, void *ds, void *da)
{
    this->dev = mx;
    this->recv_process = (void *)up_packet_process;
    mx->pdata = this;
    mx->receive_process = (void *)up_packet_process;
    mx->set_rx_arg(this);

    mx->config(da);
    this->devarg = da;
    this->devstring = ds;
}

void SendMode::init(MXType _type, u8 _remote, UPCMD _cmd, u8 *_data, int _len, int _size_per_packet)
{
    this->_imp = NULL;
    this->type = _type;
    this->data = _data;

    this->psize = _size_per_packet;
    this->cmd = _cmd;
    this->remote = _remote;
    if (_len == _size_per_packet) //不分包时
    {
        if ((_len + 16) > PDSIZE)
            _len = PDSIZE - 16;
    }
    // memcpy(this->up.data,data,_len);
    this->len = _len;
    if (_len <= 0)
        return;
    this->packet_count = _len / _size_per_packet + ((_len % _size_per_packet) == 0 ? 0 : 1); //有余，加一包
    if (this->packet_count == 0)
        this->packet_count = 1;
}

void SendMode::initMul(MXType _type, u8 _remote, UPCMD _cmd, MPBase *imp)
{
    int _len = imp->size();
    this->init(_type, _remote, _cmd, (u8 *)imp->buffer, _len, imp->persize);
    this->_imp = imp;
}

bool SendMode::connect(void)
{
    bool r = this->dev->open(this->devstring);
    usleep(500);
    return r;
}

void SendMode::onsend(UUPacket *sp, bool ok)
{
    extern int get_current_liushui(void);
    extern int set_current_liushui(int liu);
    int p = get_current_liushui();
    p++;
    p &= 0xFFFF;
    if (p == 0)
        p = 1;
    this->seial_number = p;
    INFO("序列号++, 发送=%d, 当前(下一包)=%d ", get_current_liushui(), p);
    set_current_liushui(p);
}
//流程控制
// 0:ok  -1:timeout  -2:error
int SendMode::wait(int ms, Callback *to, Callback *get)
{
    if (this->len <= 0)
        return 0;
    //========================M1=================================================================================
    if (this->type == M1)
    {
        int t = 0;
        int dlen = upacket_encodehex(&this->up, true, this->remote, this->cmd, this->data, this->len, STX_STX, ETX_ETX);
        INFO("[M1] send packet: size=%d, cmd=%s", dlen, cmd_to_string(this->up.cmd));
        this->dev->send((void *)&this->up, this->up.packetlen, ms, ms);
        while (t < ms)
        {
            usleep(1000);
            t++;
            if (this->dev->is_send_end())
            {
                INFO("[M1]:return 0");
                if (this->cmd != UPCMD_2FH_LianluWeichiBao) //链路报没有下行，sn不++
                    this->onsend(&this->up, true);
                return 0;
            }
        }
        this->onsend(&this->up, false);
        INFO("[M1]:return -1");
        return -1;
    }

    //========================M2=================================================================================
    //一包 一响应
    else if (this->type == M2 || this->type == M4)
    {
        int c = 0;
        int result = 0;
        while (c < this->packet_count)
        {
            int nowsize = get_sn_size(c + 1);
            int retry = 3;
            //分包发送
            u8 *d = NULL;
            if (_imp == NULL)
                d = &this->data[c * this->psize];
            else
            {
                //读取数据
                int actlen = _imp->reportdata(c * this->psize, nowsize, _imp->buffer);
                if (actlen == 0)
                {
                    ERROR("_imp,读取不到数据发送");
                    return 0;
                }
                //分包发送
                // u8 *d = &this->data[(sn - 1) * this->psize];
                nowsize = actlen;
                d = (u8 *)_imp->buffer;
            }
            int t = 0;
            this->get_packet = false;
            int dlen = 0;
            //不分包
            if (packet_count == 1)
            {
                dlen = upacket_encodehex(&this->up, true, this->remote, this->cmd, d, nowsize,
                                         (this->packet_count == 1) ? STX_STX : STX_SYN,
                                         (c == (this->packet_count - 1)) ? ETX_ETX : ETX_ETB);
            }
            else
            {
                dlen = upacket_encodehex_m3(&this->up, true, this->remote, this->cmd, this->packet_count, c + 1, d, nowsize,
                                            (this->packet_count == 1) ? STX_STX : STX_SYN,
                                            (c == (this->packet_count - 1)) ? ETX_ETX : ETX_ETB);
            }
            this->onsend(&this->up, true);
            c++;
            result = 0;
            while (retry > 0)
            {
                retry--;
                //发送
                this->get_packet = false;
                INFO("[%s] send packet: size=%d cmd=%s", (type == M2 ? "M2" : "M4"), dlen, cmd_to_string(this->up.cmd));
                this->dev->send((void *)&this->up, this->up.packetlen, ms, ms);
                t = 0;
                while (t <= ms)
                {
                    usleep(10000);
                    t += 10;
                    // usleep(100000);t+=100;
                    if (this->get_packet == true)
                    {
                        if (this->cb_getpacket)
                        {
                            this->cb_getpacket((void *)this);
                        }
                        break;
                    }
                }
                //收到应答
                if (this->get_packet == true && this->packet.p.cmd == this->cmd)
                {
                    //分包应答 ACK
                    //最后应答 EOT
                    if (this->etx == ETX_EOT || this->etx == ETX_ESC)
                    {

                        INFO("[%s] %s,return 0.", (type == M2 ? "M2" : "M4"), ext_to_string(this->etx));
                        return 0;
                    }
                    result = 0;
                    INFO("[%s] %s return 0.", (type == M2 ? "M2" : "M4"), ext_to_string(this->etx));
                    break;
                }
                //超时
                else
                {
                    result = 2;
                    INFO("[%s]:cmd=%02x ,timeout", (type == M2 ? "M2" : "M4"), this->cmd);
                    if (this->cb_timeout)
                    {
                        this->cb_timeout((void *)this);
                    }
                    if (retry <= 0)
                    {
                        INFO("[%s] retry <0,return -1.", (type == M2 ? "M2" : "M4"));
                        return -1;
                    }
                }
            }

            if (result == 2)
            {
                INFO("[%s] timeout result=2,return -1.", (type == M2 ? "M2" : "M4"));
                return -1; //超时
            }
        }
        INFO("[%s] return -1,result = %d.", (type == M2 ? "M2" : "M4"), result);
        return result;
    }

    //========================M3=================================================================================
    //多包包 一响应  (括图片信息报、均匀时段水文信息报、人工置数报)
    else if (type == M3)
    {
        int c = 0;
        int result = 0;
        //一次发送所有包
        while (c < this->packet_count)
        {
            int r = m3_send(c + 1, ms);
            this->onsend(&this->up, true);
            if (r < 0)
                return -1; //链路超时
            c++;
        }

        //等待应答 EOT
        int t = 0;
        while (t < ms)
        {
            usleep(1000);
            t++;
            if (this->get_packet == true)
            {
                if (this->cb_getpacket)
                {
                    this->cb_getpacket((void *)this);
                }

                //收到的必须是 EOT NAK包
                ETX eot = (ETX)this->packet.p.etx;
                if (eot == ETX_EOT)
                {
                    INFO("[M3] recv EOT. cmd=%02x, len=%d, count=%d", this->cmd, this->len, packet_count);
                    return 0; //发送成功
                }
                if (eot == ETX_ESC)
                {
                    INFO("[M3] recv ESC. cmd=%02x, len=%d, count=%d", this->cmd, this->len, packet_count);
                    return 0; //发送成功
                }
                //收到 NAK 重发
                if (eot == ETX_NAK && this->packet.p.cmd == this->cmd)
                {
                    //重发 nak包
                    u32 tmp = 0;
                    tmp |= this->packet.p.data[0];
                    tmp <<= 8;
                    tmp |= this->packet.p.data[1];
                    tmp <<= 8;
                    tmp |= this->packet.p.data[2];

                    INFO("[M3] recv NAK.cmd=%02x, total=%d, sn=%d.\n ", this->packet.p.cmd, (tmp >> 12), (tmp & 0xFFF));
                    m3_send(tmp & 0xFFF, ms);
                    t = 0;
                }
            }
        }
        INFO("[M3]:cmd=%02x ,timeout", this->cmd);
        return -2;
    }
    //========================M4=================================================================================
    //分包上报，确认
    // M4，中心站为通信发起端。
    // 1.中心站发出查询请求报文后，遥测站接收请求报文正确，应发送响应帧；
    // 2.如遥测站接收请求报文无效，则不响应。
    // 3.用于查询遥测站数据，设置（修改）遥测站运行状态参数、控制遥测站运行。
    // 4.下行帧为“查询/确认”帧，报文结束符为 ENQ/ACK/EOT；上行帧为响应帧，报文结束符为 ETB/ETX。

    return -2;
}

bool SendMode::isopen(void)
{
    if (this->dev == NULL)
        return false;
    return this->dev->is_open();
}

SendMode tcp_sendmode;
SendMode serial_sendmode;
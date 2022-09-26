
#include "mxdevice.h"
#include "serialport.h"

/*=================================================================
    SerialPort 封装成 MxDevice
===================================================================*/
int MxDeviceSerialPort::init(char *n, void *pd, void *r)
{
    this->pdata = pd;
    this->name = n;
    this->receive_process = r;
    this->s.user_data = pd;
    return 0;
}
int MxDeviceSerialPort::config(void *arg) { 
    int speed;
    speed = atoi((const char*)arg);
    this->s.baudrate=speed;
    return this->s.change_speed(*((int*)arg));
 }
int MxDeviceSerialPort::open(void *arg)
{
    this->s.set_packet_process(this->receive_process, 20000);
    return this->s.start((const char *)arg,this->s.baudrate);
}
int MxDeviceSerialPort::close(void *arg) { return this->s.quit(); }
int MxDeviceSerialPort::keepalive(bool k) { return 1; }
int MxDeviceSerialPort::send(void *buffer, int len, int sms,int rms) { return this->s.send((u8 *)buffer, len); }
bool MxDeviceSerialPort::is_keepalive(void){return true;}
bool MxDeviceSerialPort:: is_send_end(void){return this->s.is_send_end();}
void MxDeviceSerialPort::set_rx_arg(void*arg)
{
    this->s.user_data = arg;
}
bool MxDeviceSerialPort::is_open(void)
{
    return (this->fd>0);
}
DeviceState MxDeviceSerialPort::device_state(void)
{
    if(this->is_open())
        return DS_OPEN;
    else
        return DS_CLOSE;
}

/*=================================================================
    Tcp client 封装成 MxDevice
===================================================================*/
#include "tcp_client.h"
int MxDeviceTcpClient::init(char *n, void *pd, void *r)
{
    this->pdata = pd;
    this->name = n;
    this->receive_process = r;
    this->s.user_data = this;
    return 0;
}
//"ip:port"
int MxDeviceTcpClient::config(void *arg)
{
    int ip1, ip2, ip3, ip4, port = 0;
    char ip[20];
    sscanf((const char *)arg, "%d.%d.%d.%d:%d", &ip1, &ip2, &ip3, &ip4, &port);
    sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
    return this->s.config((const char *)ip, port);
}

int MxDeviceTcpClient::open(void *arg)
{
    this->s.set_packet_process(this->receive_process, 20000);
    return this->s.start(arg);
}
int MxDeviceTcpClient::close(void *arg)
{
    return this->s.quit();
}
int MxDeviceTcpClient::keepalive(bool k)
{
    return this->s.keepalive(k);
}

int MxDeviceTcpClient::send(void *buffer, int len, int sms,int rms)
{
    return this->s.senddata(buffer, len,sms,rms,1024);
}

bool MxDeviceTcpClient::is_keepalive(void){return this->s.keep;}
bool MxDeviceTcpClient::is_send_end(void){return this->s.is_send_end();}
void MxDeviceTcpClient::set_rx_arg(void*arg)
{
    this->s.user_data = arg;
}

bool MxDeviceTcpClient::is_open(void)
{
    return this->s.opened;
}

DeviceState MxDeviceTcpClient::device_state(void)
{
    switch (this->s.step)
    {
        case TCP_STEP_NULL:return DS_NULL;
        case TCP_STEP_OPEN:
        case TCP_STEP_CONNECT:
             return DS_CONNECTING;
        case TCP_STEP_OPEN_OK:
        case TCP_STEP_SEND:
        case TCP_STEP_SEND_OK:
        case TCP_STEP_RECV_WAIT:
        case TCP_STEP_RECV_GET_OK:
             return DS_OPEN;
        case TCP_STEP_OPEN_ERROR:
        case TCP_STEP_RESEND:
        case TCP_STEP_SEND_ERROR:
             return DS_WAIT;
        default:
             return DS_WAIT;
    }
}
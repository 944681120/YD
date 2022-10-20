
// #pragma once
// #include "lib.h"
// #include "rtu_setting.hpp"
// #include "dfsm.h"

// class Imodbus_device
// {

// protected:
//     //配置文件
//     rtu_485 config;
//     //通信
//     dfsm device;

//     void *down_packet_process(void *ss, u8 *data, int len)
//     {

//     }

// public:
//     Imodbus_device(rtu_485 cfg)
//     {
//         config = cfg;
//     }

// public:
//     virtual bool opendevice() = 0;
//     virtual int buildpacket() = 0;
//     virtual int sendpacket() = 0;
//     virtual int readpacket() = 0;

//     virtual int parse() = 0;
//     virtual int poll() = 0;
// };

// class modbus_device_base : public Imodbus_device
// {
// private:
//     /* data */
// public:
//     modbus_device_base(rtu_485 cfg) : Imodbus_device(cfg) {}
//     ~modbus_device_base() {}
// };

// class modbus_device_mqtt : public Imodbus_device

// {
// private:
// public:
//     modbus_device_mqtt() : Imodbus_device(cfg) {}
//     ~modbus_device_mqtt() {}
// };


//====================================统一规划modbus or tcp 通讯类型======================================//
//0. 设备配置统一 class
//1. 设备接口统一 rw
//2. 数据收发协议(策略)配置     decode/decode
//3. 数据处理配置(解码，保存等)  prcess
//4. 自定义事件                event

template<class Tcfg,class Tdev,class Tpro>
class Idevice
{
private:
    /* data */
public:
    Idevice(/* args */){}
    ~Idevice(){}
};
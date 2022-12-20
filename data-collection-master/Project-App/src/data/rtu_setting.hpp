#include "mytype.h"
#pragma once
#ifndef __RTU_SETTING_H__
#define __RTU_SETTING_H__
#include "json.hpp"
#include "iostream"
#include <fstream>

using namespace configor;
using namespace std;

typedef struct rtu_ftp
{
    int enable;       //":1,
    string ip;        //":"192.168.3.223",
    string user;      //":"kaihua",
    string password;  //":"111111",
    string downdir;   //":"/app-cjq",
    string serverdir; //":"ftp",
    int interval;     //":60
    JSON_BIND(rtu_ftp, enable, ip, user, password, downdir, serverdir, interval);
} RtuFtp;

typedef struct rtu_485
{

    string factorType;
    string port;
    int baud;
    string brandName;
    string requestCmd;
    string resultDataFilter;
    string dataType;
    string dataFormatter;
    int err_cnt;
    JSON_BIND(rtu_485, factorType, port, baud, brandName, requestCmd, resultDataFilter, dataType, dataFormatter);

} Rtu485;

typedef struct rtu_guangzhou
{
    bool enable;      //是否上传     规约版本,RTU序号
    int version;      // 180         表示V1.8
    int sn;           // 123         表示bcd码为0123
    string link_mode; //链路使用方式  "M2"
    JSON_BIND(rtu_guangzhou, enable, version, sn, link_mode);
} Rtuguangzhou;

// //定制功能，与远动不一样的个性化都在这里配置
// typedef struct rtu_dingzhi{

//    string protocol;              //协议版本
//    vector<string> protocol_list; //当前支持的所有协议的版本

//    //=============各个版本的个性化定制配置====================//
//    //《广州市“智慧排水”项目物联数据接入标准》_V2.0_20220630
//    //不同点:1.上报协议版本 2.上报rtu序号 3.链路咱使用 M2方式

//     JSON_BIND(rtu_dingzhi,protocol,protocol_list,gz);

// }Rtudingzhi;

//对时
typedef struct rtu_ntp
{
    string server;
    int port;
    int interval;
    int timeout;
    JSON_BIND(rtu_ntp, server, port, interval, timeout);
} Rtuntp;
//图片设置
typedef struct rtu_image
{
    string dir;
    string mode; // M2 or M3
    int retry_delays;
    int persize;
    JSON_BIND(rtu_image, dir, mode, retry_delays, persize);
} Rtuimage;

typedef struct rtu_Smartstation
{
    /*是否启用 大坝中心-智慧站 功能*/
    bool enable;
    /*连接参数*/
    string host;     //服务器
    int port;        //连接端口
    string clientid; //设备id
    string user;     //用户名
    string passwd;   //密码
    bool tls;        //是否使用tls传输
    string ca;       //证书

    /*主题*/
    string topic_realtime;      //实时数据上传    $iot/real/deviceid   Qos=1
    string topic_realtime_back; //实时数据上传应答 deviceid_sub/back    Qos=1

    string topic_cmd;      //软件控制命令     deviceid_sub/com   Qos=1
    string topic_cmd_back; //软件控制命令应答  $iot/control/deviceid    Qos=1

    string topic_publiccmd;      //第三方平台下发控制命     $iot/public/com/deviceid        Qos=1
    string topic_publiccmd_back; //第三方平台下发控制命应答  $iot/public/comback/deviceid    Qos=1

} RtuMqtt;

// rtu setting file
#define RTU_SETTING_DEFAULT "/app-cjq/setting/default_rtu_setting.json"
#define RTU_SETTING_FULLNAME "/app-cjq/setting/rtu_setting.json"
#define RTU_SETTING_SAVE_FULLNAME "/app-cjq/setting/rtu_setting.json" //"/home/kaihua/Desktop/rtu_setting.json" //

//=============================分类1 param参数===============================//
struct rtu_param
{
    // param参数部分
    string terminalNo;
    int terminalType;
    string model;
    string passwd;
    vector<int> center;
    vector<string> remote;
    string bdSerialPort;
    int bdBaud;
    string bdAddr; //"HEX string"
    vector<string> shishi;
    vector<string> dingshi;
    vector<string> xiaoshi;
    map<string, vector<string>> jiabao;
    map<string, l64> runtime;        // runtime 部分
    map<string, string> personalset; // personalset

    JSON_BIND(rtu_param, terminalNo, terminalType, model, passwd, center, remote, bdSerialPort, bdBaud, bdAddr, shishi, dingshi, xiaoshi, jiabao, runtime, personalset);
};

//=============================分类2 设备端参数===============================//
struct rtu_device
{
    rtu_guangzhou gz;      //广州配置:rtu序列号
    vector<rtu_485> rs485; //传感器部分
    rtu_image image;       //图片配置
    JSON_BIND(rtu_device, gz, rs485, image);
};
//=============================分类3 服务端参数===============================//
struct rtu_server
{
    rtu_ntp ntp;        //对时配置
    struct rtu_ftp ftp; //升级参数部分
    JSON_BIND(rtu_server, ntp, ftp);
};
//修改rtu_setting 必须置位dirty=true,并同步到rtu_setting.json
class rtu_setting
{
public:
    // dirty
    bool dirty;

    // devdata 的5min 数据,保留日志天数
    int logsmax_5min;

    rtu_param param;
    rtu_device device;
    rtu_server server;

    rtu_setting()
    {
        printf("新建 rtu_setting.\n");
    }
    ~rtu_setting()
    {
        printf("销毁 rtu_setting.\n");
    }
    JSON_BIND(rtu_setting,
              logsmax_5min,
              param,
              device,
              server);

    //============文件操作===================================//
    string load(string fullname)
    {
        json j;
        // rtu_setting rr;
        std::ifstream ifs(fullname);
        ifs >> j;
        // rr = (rtu_setting)j;
        //*this = rr;
        *this = (rtu_setting)j;
        ifs.close();
        return j.dump(1, ' ');
    }
    string load() { return this->load(RTU_SETTING_FULLNAME); }
    string save(string fullname)
    {
        json j = (*this);
        // 序列化为字符串
        std::string json_str = j.dump();
        // 美化输出，使用 4 个空格对输出进行格式化
        std::string pretty_str = j.dump(4, ' ');
        // 将 JSON 内容输出到文件
        std::ofstream ofs(fullname);
        ofs << j.dump(4, ' ') << std::endl;
        ofs.flush();
        ofs.close();
        return j.dump(4, ' ');
    }
    string save() { return this->save(RTU_SETTING_SAVE_FULLNAME); }
};
extern class rtu_setting rtu;

#endif
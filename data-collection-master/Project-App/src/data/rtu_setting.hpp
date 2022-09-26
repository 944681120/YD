#pragma once
#ifndef __RTU_SETTING_H__
#define __RTU_SETTING_H__

#include "mytype.h"
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

//修改rtu_setting 必须置位dirty=true,并同步到rtu_setting.json
class rtu_setting
{
public:
    // dirty
    bool dirty;

    // devdata 的5min 数据,保留日志天数
    int logsmax_5min;

    // param参数部分
    string terminalNo;
    int terminalType;
    string model;
    string passwd;
    vector<int> center;
    vector<string> remote;
    string bdSerialPort;
    int bdBaud;
    vector<string> shishi;
    vector<string> dingshi;
    vector<string> xiaoshi;
    map<string, vector<string>> jiabao;

    //传感器部分
    vector<rtu_485> rs485;

    //升级参数部分
    struct rtu_ftp ftp;

    // runtime 部分
    map<string, l64> runtime;

    map<string, string> personalset;

    // mqtt配置参数

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
              terminalNo,
              terminalType,
              model,
              passwd,
              center,
              remote,
              bdSerialPort,
              bdBaud,
              shishi,
              dingshi,
              xiaoshi,
              jiabao,
              rs485,
              ftp,
              runtime,
              personalset);

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
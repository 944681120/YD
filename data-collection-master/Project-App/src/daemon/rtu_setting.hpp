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

// rtu setting file
#define RTU_SETTING_DEFAULT "/app-cjq/setting/default_rtu_setting.json"
#define RTU_SETTING_FULLNAME "/app-cjq/setting/rtu_setting.json"
#define RTU_SETTING_SAVE_FULLNAME "/app-cjq/setting/rtu_setting.json" //"/home/kaihua/Desktop/rtu_setting.json"

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
    int passwd;
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
        rtu_setting rr;
        std::ifstream ifs(fullname);
        ifs >> j;
        rr = (rtu_setting)j;
        *this = rr;
        return j.dump(1, ' ');
    }
    string load() { return this->load(RTU_SETTING_FULLNAME); }
    string save(string fullname)
    {
        json j = (json) * this;
        // 序列化为字符串
        std::string json_str = j.dump();
        // 美化输出，使用 4 个空格对输出进行格式化
        std::string pretty_str = j.dump(4, ' ');
        // 将 JSON 内容输出到文件
        std::ofstream ofs(fullname);
        ofs << j.dump(4, ' ') << std::endl;
        return j.dump(4, ' ');
    }
    string save() { return this->save(RTU_SETTING_SAVE_FULLNAME); }
};
extern class rtu_setting rtu;

#endif
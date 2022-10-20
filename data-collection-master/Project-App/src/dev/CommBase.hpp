#pragma once 
#include "asio2/asio2.hpp"
#include <map>
using namespace std;
//通用通讯接口
class IComm
{
protected:
    map<string,string> cfg; //它可以是  字符串,json字符串，以适配各种配置参数       
public:
    IComm(map<string,string> _cfg){cfg = _cfg;}
    ~IComm();

    int reconfig(map<string,string>_cfg) = 0;
    int connect() = 0;
    int disconnect() = 0;
};

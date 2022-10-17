#pragma once

//通用头文件
#include <sys/types.h>
#define NNNN(a, b) a, b
#define CCCC(a) a, 0

#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
#include "regex.h"
#include "rtu_setting.hpp"
#include <map>
#include "lib.h"
extern ul64 hex2bcd(u8 *hex, int bytes);
using namespace std;

#define DEVICE_DATA_VALUE_NULL 0x7AAAAAAAAAAAAAAA

enum DataClassConfigType
{
    Type_Dev,
    Type_Param,
    Type_Runtime,
    Type_Personalset,
    Type_AppData,
};

enum DataFormat
{
    DF_BCD = 1,      // bcd 使用 int64
    DF_HEXNum,       // hex number 使用 int64
    DF_HEX,          // hex 使用 char[]
    DF_CHAR,         // char使用 char[]
    DF_STRING,       // str 使用 std::string
    DF_JPG,          // char[]
    DF_StringVector, // list <string>
    DF_HHmm,         //
};
struct DataClassItemHHmm
{
    unsigned char HH;
    unsigned char mm;
};

struct DataClassItemConfig
{
    u16 id;
    string name;
    int len; //整数长度
    int fl;  //小数长度
    DataFormat format;
    string comment;
};

union DataClassValue
{
    l64 v64;
    unsigned char *vhex;
    char *vstr;
    DataClassItemHHmm hm;
};

class DataClassItem
{
private:
    int _to_report(char *out)
    {
        switch (this->config.format)
        {
            // 123->[0-n]={01,23}
        case DF_BCD:
        {
            int a = 0;
            l64 vv = this->val;
            if (vv < 0)
                vv *= -1;
            int i = 0;
            int bytes = this->len();
            while (i < bytes)
            {
                a = vv % 100;
                vv /= 100;
                out[bytes - 1 - i] = (a % 10) | ((a / 10) << 4);
                i++;
            }
            return bytes;
        }
        case DF_HEXNum:
            memcpy(out, (void *)&this->val, this->len());
            inverse((u8 *)out, this->len());
            return this->len();
        case DF_HEX:
            memcpy(out, this->hex, this->len());
            return this->len();
        case DF_CHAR:
            memcpy(out, this->chars, this->len());
            return this->len();
        case DF_STRING:
        {
            int s = this->str.size();
            if (s > this->len())
                s = this->len();
            memset(out, 0, this->len());
            memcpy(out, this->str.c_str(), s);
            return s;
        }
        case DF_StringVector:
        {
            string ss = list_merge(list, " ");
            int s = ss.length();
            if (s > this->len())
                s = this->len();
            memset(out, 0, this->len());
            memcpy(out, ss.c_str(), s);
            return s;
        }
        case DF_HHmm:
        {
            out[0] = (u8)hex2bcd(&this->hhmm.HH, 1);
            out[1] = (u8)hex2bcd(&this->hhmm.mm, 1);
            return this->len();
        }
        default:
            return 0;
        }
    }

public:
    //==========config===========//
    DataClassItemConfig config;
    //==========data=============//
    l64 val;
    unsigned char *hex;
    char *chars;
    string str;
    vector<string> list;
    DataClassItemHHmm hhmm;

#define DATA_INVALID_VALUE 0x7FFFFFFFFFFFFFFF

    u8 reportlen()
    {
        return (this->len() << 3) | (this->config.fl);
    }
    u8 negreportlen()
    {
        return (this->len() + 1) << 3 | (this->config.fl);
    }
    int len()
    {
        return (this->config.len + 1) / 2;
    }
    l64 Value()
    {
        if (this->val == DATA_INVALID_VALUE)
            return 0;
        else
            return this->val;
    }
    void clear(void)
    {
        if (this->len() == 0)
            return;
        switch (this->config.format)
        {
        case DataFormat::DF_HHmm:
            this->hhmm.HH = 0xFF;
            this->hhmm.mm = 0xFF;
            break;
        case DataFormat::DF_HEXNum:
        case DataFormat::DF_BCD:
            this->val = DATA_INVALID_VALUE;
            break;
        case DataFormat::DF_HEX:
            memset(this->hex, 0xFF, this->len());
            break;
        default:
        case DataFormat::DF_StringVector:
            list.clear();
            break;
        case DataFormat::DF_CHAR:
            memset(this->chars, 0, this->len());
            break;
        }
    }
    bool isZero(void)
    {
        if (this->len() == 0)
            return true;
        switch (this->config.format)
        {
        case DataFormat::DF_HHmm:
            if (this->hhmm.HH == 0xFF && this->hhmm.mm == 0xFF)
                return true;
            return false;
        case DataFormat::DF_HEXNum:
        case DataFormat::DF_BCD:
            if (this->val == DATA_INVALID_VALUE)
                return true;
            return false;
        case DataFormat::DF_HEX:

            for (int i = 0; i < this->len(); i++)
            {
                if (this->hex[i] != 0xFF)
                    return false;
            }
            return true;

        case DataFormat::DF_STRING:
            if (this->str.size() == 0)
                return true;
            return false;
        case DataFormat::DF_StringVector:
            return list.size() == 0;
        default:
        case DataFormat::DF_CHAR:
            if (this->chars[0] == 0)
                return true;
            else
                return false;
        }
    }
    bool isValue0(void)
    {
        if (this->len() == 0)
            return true;
        switch (this->config.format)
        {
        case DataFormat::DF_HHmm:
            if (this->hhmm.HH == 0 && this->hhmm.mm == 0)
                return true;
            return false;
        case DataFormat::DF_HEXNum:
        case DataFormat::DF_BCD:
            if (this->val == 0)
                return true;
            return false;
        case DataFormat::DF_HEX:
            for (int i = 0; i < this->len(); i++)
                if (this->hex[i] != 0)
                    return false;
            return true;
            break;
        default:
        case DataFormat::DF_CHAR:
            if (this->chars[0] == 0)
                return true;
            else
                return false;
        }
    } // val ==0 hex==0 char=null hhmm=0.0
    string toString(const char *varname)
    {
        char strbuffer[500];
        memset(strbuffer, 0, sizeof(strbuffer));
        switch (this->config.format)
        {
        case DataFormat::DF_BCD:
            sprintf(strbuffer, "%s[%04xH %-5s %-15s] = %jd", varname, config.id, config.name.c_str(), config.comment.c_str(), isZero() ? 0 : val);
            break;
        case DataFormat::DF_HEXNum:
            char hexnumber[20];
            char stringnumber[40];
            memset(hexnumber, 0, sizeof(hexnumber));
            memset(stringnumber, 0, sizeof(stringnumber));
            this->toreport(hexnumber);
            hexarray2str(hexnumber + 2, this->len(), stringnumber, sizeof(stringnumber));
            sprintf(strbuffer, "%s[%04xH %-5s %-15s] = %s", varname, config.id, config.name.c_str(), config.comment.c_str(), stringnumber);
            break;
        case DataFormat::DF_HEX:
            sprintf(strbuffer, "%s[%04xH %-5s %-15s] = ", varname, config.id, config.name.c_str(), config.comment.c_str());
            hexarray2str((char *)hex, len(), strbuffer + strlen(strbuffer), sizeof(strbuffer) - strlen(strbuffer));
            break;
        case DataFormat::DF_CHAR:
            sprintf(strbuffer, "%s[%04xH %-5s %-15s] = %s", varname, config.id, config.name.c_str(), config.comment.c_str(), chars);
            break;
        case DataFormat::DF_HHmm:
            sprintf(strbuffer, "%s[%04xH %-5s %-15s] = hh=%d mm=%d", varname, config.id, config.name.c_str(), config.comment.c_str(), hhmm.HH, hhmm.mm);
            break;
        case DataFormat::DF_STRING:
            sprintf(strbuffer, "%s[%04xH %-5s %-15s] = %s", varname, config.id, config.name.c_str(), config.comment.c_str(), str.c_str());
            break;
        case DataFormat::DF_StringVector:
            sprintf(strbuffer, "%s[%04xH %-5s %-15s] = %s", varname, config.id, config.name.c_str(), config.comment.c_str(), list_merge(list, " ").c_str());
            break;
        default:
            break;
        }
        return string(strbuffer);
    }

    //生成report格式=>2019xxxx
    int toreport(char *bytesout)
    {
        int _size = 0;
        char *bigbcd = bytesout;
        if (this->config.len <= 0)
            return 0;

        if (this->config.id > 0xFF00)
        {
            //自定义id
            bigbcd[0] = 0xFF;
            bigbcd++;
            _size = 1;
        }

        bigbcd[0] = this->config.id & 0xFF;
        bigbcd[1] = this->reportlen();
        _size += this->len() + 2;
        if (this->isZero())
        {
            bigbcd[1] = reportlen();
            memset(bigbcd + 2, 0xFF, this->len());
            return _size;
        }
        else
        {
            switch (config.format)
            {
            case DF_BCD:
                if (this->val < 0)
                {
                    bigbcd[1] = negreportlen();
                    bigbcd[2] = 0xFF;
                    _size++;
                    _to_report(bigbcd + 3);
                    return _size;
                }
                else
                {
                    bigbcd[1] = reportlen();
                    _to_report(bigbcd + 2);
                    return _size;
                }
                break;
            case DF_HHmm:
            case DF_HEX:
            case DF_CHAR:
            case DF_HEXNum:
                _to_report(bigbcd + 2);
                break;
            case DF_STRING:
            case DF_StringVector:
                _size = _to_report(bigbcd + 2);
                break;
                bigbcd[1] = _size << 3;
                break;
            }

            return _size;
        }
        return 0;
    }

    string toreport()
    {
        char buf[1024];
        char str[2048];
        memset(buf, 0, sizeof(buf));
        memset(str, 0, sizeof(str));
        int size = toreport(buf);
        int len = hexarray2str(buf, size, str, sizeof(str));
        return string(str);
    }

    int fromreport(string str)
    {
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        str2hexarray(str.c_str(), str.size(), buffer, sizeof(buffer));
        return fromreport(buffer);
    }

    //从report格式恢复 =>2019xxxx
    int fromreport(char *_bytes)
    {
        u8 *reportbytes = (u8 *)_bytes;
        if (reportbytes[0] == 0xFF)
            reportbytes++;
        char *pStopString;
        int rsize = reportbytes[1] >> 3;
        int jump = rsize;
        switch (this->config.format)
        {
        case DataFormat::DF_BCD:
        {
            l64 f = 1;
            if (rsize > this->len() && reportbytes[2] == 0xFF)
            {
                this->val = bigbcd2hex((u8 *)(reportbytes + 3), rsize - 1);
            }
            else
            {
                if (rsize > this->len())
                    rsize = this->len();
                this->val = bigbcd2hex((u8 *)(reportbytes + 2), rsize);
            }
            INFO("set %02x. size=%d,value=%ld", this->config.id, rsize, (l64)this->val);
        }
        break;
        case DataFormat::DF_HEXNum:
        {
            if (this->len() < rsize)
                rsize = this->len();
            memcpy(&this->val, (u8 *)(reportbytes + 2), rsize);
            inverse((u8 *)&this->val, rsize);
            INFO("set %02x. size=%d,value=%llx", this->config.id, rsize, (long long)(this->val));
        }
        break;
        case DataFormat::DF_HEX:
        {
            if (this->len() < rsize)
                rsize = this->len();
            memcpy(this->hex, (u8 *)(reportbytes + 2), rsize);
            // little2big((u8 *)(reportbytes + 2), this->hex, rsize);
            PRINTBYTES("set %02x. size=%d", this->hex, rsize, this->config.id, rsize);
        }
        break;
        case DataFormat::DF_CHAR:
            if (this->len() < rsize)
                rsize = this->len();
            memcpy(this->chars, reportbytes + 2, rsize);
            this->chars[rsize] = 0;
            INFO("set %02x. size=%d, chars=%s", this->config.id, rsize, this->chars);
            break;
        case DataFormat::DF_STRING:
            char strbuffer[1024];
            memset(strbuffer, 0, sizeof(strbuffer));
            if (this->len() == 0)
                break;
            if (this->len() < rsize)
                rsize = this->len();
            memcpy(strbuffer, reportbytes + 2, rsize);
            this->str = string(strbuffer);
            INFO("set %02x. size=%d, str=%s", this->config.id, rsize, this->str.c_str());
            break;
        case DataFormat::DF_HHmm:
            this->hhmm.HH = reportbytes[2];
            this->hhmm.mm = reportbytes[3];
            INFO("set %02x.hhmm=%d %d", this->config.id, this->hhmm.HH, this->hhmm.mm);
            break;
        case DataFormat::DF_StringVector:
        {
            char strbuffer[1024];
            memset(strbuffer, 0, sizeof(strbuffer));
            if (this->len() == 0)
                break;
            if (this->len() < rsize)
                rsize = this->len();

            memcpy(strbuffer, reportbytes + 2, rsize);
            strbuffer[sizeof(strbuffer) - 1] = 0;
            this->list = split(string(strbuffer), " ");
            INFO("set %02x. size=%d, list=%s", this->config.id, rsize, strbuffer);
            break;
        }
        break;
        }

        return jump;
    }
};

extern const DataClassItemConfig param_config_items[];
extern int param_config_items_size;

extern const DataClassItemConfig runtime_config_items[];
extern int runtime_config_items_size;

extern const DataClassItemConfig personalset_config_items[];
extern int personalset_config_items_size;

extern const DataClassItemConfig devdata_config_items[];
extern int devdata_config_items_size;

extern const DataClassItemConfig appdata_config_items[];
extern int appdata_config_items_size;

class DataClass
{
private:
    string name;

    vector<DataClassItem> items;
    map<u32, DataClassItem *> int_items;
    map<string, DataClassItem *> str_items;
    DataClassItem nitem = {{0x00, "NULL", 0, 0, DF_BCD, "NULLITEM"}, DATA_INVALID_VALUE};

public:
    DataClassConfigType type;
    time_t now;
    int size(void) { return this->items.size(); }
    string Name(void) { return this->name; }
    DataClass(const DataClassItemConfig config[], int size)
    {
        DataClassInit(config, size);
    }
    DataClass() {}
    DataClass(DataClassConfigType t, const char *n)
    {
        DataClassInit(t);
        this->name = string(n);
    }
    DataClass(DataClassConfigType t, string n)
    {
        DataClassInit(t);
        this->name = n;
    }

    //清空数据
    void clearAll()
    {
        for (int i = 0; i < items.size(); i++)
        {
            items.clear();
        }
    }

    void DataClassInit(const DataClassItemConfig config[], int size)
    {
        items.reserve(size);
        for (int i = 0; i < size; i++)
        {
            DataClassItem item = {config[i], DATA_INVALID_VALUE};
            item.chars = NULL;
            item.hex = NULL;
            int _size = (item.config.fl + item.config.len + 1) / 2;
            switch (item.config.format)
            {
            case DF_STRING:
                item.str = "";
                break;
            case DF_StringVector:
                item.list = vector<string>();
                item.list.clear();
            case DF_BCD:
                break; // value
            case DF_CHAR:
                if (_size > 0)
                {
                    item.chars = new char[_size + 2];
                    memset(item.chars, 0, _size + 2);
                    item.val = (l64)item.chars;
                }

                break;
            case DF_HEX:
                if (_size > 0)
                {
                    item.hex = new unsigned char[_size];
                    memset(item.hex, 0xFF, _size);
                    item.val = (l64)item.hex;
                }

                break;
            case DF_HHmm:
                item.val = 0;
                item.hhmm.HH = 0;
                item.hhmm.mm = 0;
            default:
                break;
            }
            items.push_back(item);
            int_items[item.config.id] = &items.back();
            str_items[item.config.name] = &items.back();
        }
    }
    void DataClassInit(DataClassConfigType t)
    {
        type = t;
        switch (t)
        {
        default:
        case DataClassConfigType::Type_Dev:
            DataClassInit(devdata_config_items, devdata_config_items_size);
            break;
        case DataClassConfigType::Type_Param:
            DataClass::DataClassInit(param_config_items, param_config_items_size);
            break;
        case DataClassConfigType::Type_Personalset:
            DataClassInit(personalset_config_items, personalset_config_items_size);
            break;
        case DataClassConfigType::Type_Runtime:
            DataClassInit(runtime_config_items, runtime_config_items_size);
            break;
        case DataClassConfigType::Type_AppData:
            DataClassInit(appdata_config_items, appdata_config_items_size);
            break;
        }
    }

    DataClassItem &operator[](u32 nn)
    {
        u16 n = (u16)nn;
        map<u32, DataClassItem *>::iterator iter;
        iter = int_items.find(n);
        if (iter == int_items.end())
        {
            ERROR("无法访问数据: id=%02X", n);
            return nitem;
        }
        else
            return *((DataClassItem *)iter->second);
    }
    DataClassItem &operator[](string n)
    {
        map<string, DataClassItem *>::iterator iter;
        iter = str_items.find(n);
        if (iter == str_items.end())
        {
            ERROR("无法访问数据: name=%s", n.c_str());
            return nitem;
        }
        else
            return *((DataClassItem *)iter->second);
    }
    DataClassItem &operator()(int index)
    {
        if (index >= this->items.size())
            return nitem;
        return items[index];
    }

    //---------------------应用接口------------------------------//
    // 1.report all
    int reportall(char *bytesout)
    {
        int _size = 0;
        for (int i = 0; i < items.size(); i++)
        {
            if (items[i].isZero())
                continue;
            else
                _size += items[i].toreport(bytesout + _size);
        }
        return _size;
    }
    int fromreport(char *bytes, int size)
    {
        int offset = 0;
        while (offset < size)
        {
            u32 id = bytes[offset];
            offset += 2 + (*this)[id].fromreport((char *)(bytes + offset));
        }
        return offset;
    }

    // 2.from report //[0-9a-fA-F]+    "2019xxxx,2119xxx,"
    int fromlog(string logstr)
    {
        u16 sid = 0;
        char substring[1000];
        char *log = (char *)logstr.c_str();
        /** 待匹配字符串 */
        regex_t reg;
        /** 正则表达式 */
        const char *pattern = "[-0-9a-fA-F]+";
        INFO("Pattern     :%s", pattern);
        INFO("Input String:%s", log);
        search_match_t _smatch;
        int c = rx_serach(log, pattern, 0, 0, &_smatch);
        if (c > 0)
        {
            size_t off = 0;
            for (int i = 0; i < c; ++i, off += _smatch.groupcnt)
            {
                regmatch_t *gm = _smatch.pmatch + off;
                for (int g = 0; g < _smatch.groupcnt; ++g)
                {
                    int _offset = gm[g].rm_so;
                    int _size = gm[g].rm_eo - gm[g].rm_so;
                    memcpy(substring, log + _offset, _size);
                    substring[_size] = 0;
                    u8 bytes[500];
                    str2hexarray(substring, _size, (char *)bytes, sizeof(bytes));
                    // INFO("match=%s", substring);
                    {
                        if (bytes[0] == 0xFF)
                        {
                            sid = 0xFF00 | bytes[1];
                        }
                        else
                        {
                            sid = bytes[0];
                        }
                        (*this)[sid].fromreport((char *)bytes);
                    }
                }
            }
        }
        /************************************************************************/
        /* 调用 rx_serach 之后必须调用 rx_search_match_uninit 释放分配的内存    */
        /* 否则会产生内存泄露                                                   */
        /************************************************************************/
        rx_search_match_uninit(&_smatch);
        return 0;
    }
    int tolog(char *outstr, int outsize)
    {
        char bytesout[1024];
        int _size = 0;
        int _offset = 0;
        memset(outstr, 0, outsize);
        memset(bytesout, 0, sizeof(bytesout));
        for (int i = 0; i < items.size(); i++)
        {
            if (items[i].isZero())
                continue;
            else
            {
                _size = items[i].toreport(bytesout);
                _offset += hexarray2str(bytesout, _size, outstr + _offset, outsize - _offset);
                outstr[_offset] = ',';
                _offset++;
            }
        }
        outstr[_offset] = 0;
        return _offset;
    }
    string tolog(void)
    {
        char bytesout[1024];
        char outstr[2048];
        int _size = 0;
        int _offset = 0;
        memset(outstr, 0, sizeof(outstr));
        memset(bytesout, 0, sizeof(bytesout));
        for (int i = 0; i < items.size(); i++)
        {
            if (items[i].isZero())
                continue;
            else
            {
                _size = items[i].toreport(bytesout);
                _offset += hexarray2str(bytesout, _size, outstr + _offset, sizeof(outstr) - _offset);
                outstr[_offset] = ',';
                _offset++;
            }
        }
        outstr[_offset] = 0;
        return string(outstr);
    }

    string toString(void)
    {
        stringstream ss;
        for (int i = 0; i < items.size(); i++)
        {
            if (items[i].isZero() == false && items[i].config.len > 0)
            {
                ss << this->items[i].toString(this->name.c_str()) << endl;
            }
        }
        return ss.str();
    }
};

extern DataClass devdata;
extern DataClass param;
extern DataClass runtime;
extern DataClass personalset;
extern DataClass A;

extern void init_appdata(void);

//======================表58 遥测站状态和报警信息定义表======================//
class StateWarn
{
public:
    union
    {
        u32 JiaoliudianChongdian : 1,
            XudianchiVoltage : 1,
            ShuiweiChaoxian : 1,
            LiuliangChaoxian : 1,
            Shuizhi : 1,
            LiuliangYibiao : 1,
            ShuiweiYibiao : 1,
            ZhongduanXiangmen : 1,
            Memory : 1,
            ICcard : 1,
            Moter : 1,
            ShengyuShuiliang : 1,
            unused : 20;
        u32 _d;
    };
    StateWarn() {}
    StateWarn(u8 *bytes4) { memcpy((void *)&this->_d, bytes4, 4); }
    string toString()
    {
        stringstream ss;
        ss << "遥测站状态和报警信息定义表:" << endl;
        ss << "1 交流电充电状态  :" << (JiaoliudianChongdian == 1 ? "1" : "0") << endl; //
        ss << "2 蓄电池电压状态  :" << (XudianchiVoltage == 1 ? "1" : "0") << endl;     // BIT1 0：正常；1：电压低
        ss << "3 水位超限报警状态:" << (ShuiweiChaoxian == 1 ? "1" : "0") << endl;      // BIT2 0：正常；1：报警
        ss << "4 流量超限报警状态:" << (LiuliangChaoxian == 1 ? "1" : "0") << endl;     // BIT3 0：正常；1：报警
        ss << "5 水质超限报警状态:" << (Shuizhi == 1 ? "1" : "0") << endl;              // BIT4 0：正常；1：报警
        ss << "6 流量仪表状态   :" << (LiuliangYibiao == 1 ? "1" : "0") << endl;        // BIT5 0：正常；1：故障
        ss << "7 水位仪表状态   :" << (ShuiweiYibiao == 1 ? "1" : "0") << endl;         // BIT6 0：正常；1：故障
        ss << "8 终端箱门状态   :" << (ZhongduanXiangmen == 1 ? "1" : "0") << endl;     // BIT7 0：开启；1：关闭
        ss << "9 存储器状态     :" << (Memory == 1 ? "1" : "0") << endl;                // BIT8 0：正常；1：异常
        ss << "10 IC 卡功能有效 :" << (ICcard == 1 ? "1" : "0") << endl;                // BIT9 0：关闭；1：IC 卡有效
        ss << "11 水泵工作状态  :" << (Moter == 1 ? "1" : "0") << endl;                 // BIT10 0：水泵工作；1：水泵停机
        ss << "12 剩余水量报警  :" << (ShengyuShuiliang == 1 ? "1" : "0") << endl;      // BIT11 0：未超限；1：水量超限
        string s;
        ss >> s;
        return s;
    }
};

//======================遥测站类别==========================================//
struct YaoCeZhanStyle
{
    int sn;
    string name;
    u8 code;
    u8 ascii;
    string info;
};

class YaoCeZhanStyleClass
{
    u8 code;

public:
    YaoCeZhanStyleClass(u8 _code) { code = _code; }
    ~YaoCeZhanStyleClass() {}
    YaoCeZhanStyle getCode()
    {
        YaoCeZhanStyle none = {0, "未知", 0, 'N', "未知"};
        YaoCeZhanStyle sets[] = {
            {1, "降水", 0x50, 'P', "①降水;②蒸发③气象"},
            {2, "河道", 0x48, 'H', "①降水;②蒸发;③河道水情④气象⑤水质"},
            {3, "水库(湖泊)", 0x4B, 'K', "①降水;②蒸发;③水库水情④气象⑤水质"},
            {4, "闸坝", 0x5A, 'Z', "①降水;②蒸发;③闸坝水情④气象⑤水质"},
            {5, "泵站", 0x44, 'D', "①降水;②蒸发;③泵站水情④气象⑤水质"},
            {6, "潮汐", 0x54, 'T', "①降水;②蒸发;③潮汐水情④气象"},
            {7, "墒情", 0x4D, 'M', "①降水;②蒸发;③墒情"},
            {8, "地下水", 0x47, 'G', "①埋深;②水质;③开采量"},
            {9, "水质", 0x51, 'Q', "①水质;②流量;③水位"},
            {10, "取水口", 0x49, 'I', "①水位;②水质;③水量④水压等"},
            {11, "排水口", 0x4F, 'O', "①水位;②水质;③水量④水压等"}};
        for (int i = 0; i < 11; i++)
        {
            if (sets[i].code == code)
                return sets[i];
        }
        return none;
    }
};

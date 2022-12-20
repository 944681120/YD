
#include "header.h"
#include <time.h>
#include "lib.h"
#include <string>
//=====================================
static u8 remote = 1;
u8 get_current_remote(void)
{
    return remote;
}
u8 set_current_remote(u8 r)
{
    remote = r;
    return remote;
}

//=====================================
static int current_liushui = 1;
int get_current_liushui(void)
{
    if (current_liushui == 0)
        current_liushui = 1;
    return current_liushui;
}
int set_current_liushui(int liu)
{
    current_liushui = liu;
    return current_liushui;
}
//=====================================
static ul64 get_time_bcd(time_t tt)
{
    ul64 result = 0;
    struct tm *t;
    // time_t tt;
    // timestd&tt);
    t = localtime(&tt);
    result |= bytehex2bcd((u8)((t->tm_year + 1900) % 100)) & 0xFF;
    result <<= 8;
    result |= bytehex2bcd((u8)(t->tm_mon + 1)) & 0xFF;
    result <<= 8;
    result |= bytehex2bcd((u8)(t->tm_mday)) & 0xFF;
    result <<= 8;
    result |= bytehex2bcd((u8)(t->tm_hour)) & 0xFF;
    result <<= 8;
    result |= bytehex2bcd((u8)(t->tm_min)) & 0xFF;
    result <<= 8;
    result |= bytehex2bcd((u8)(t->tm_sec)) & 0xFF;
    return result;
}

time_t get_current_report_time(void)
{
    return std::time(0);
    // time_t tt;
    // timestd(&tt);
    // return get_time_bcd(tt);
}
//=====================================
static time_t observetime = timestd(0);
time_t get_observetime_time(void)
{
    // 1. 观测时间码应以暴雨观测时间末为准
    // 2. 对瞬时值（或状态）类要素，观测时间码表示要素值的观测时间（或发生时间）
    // 3. 对时段类要素，观测时间码表示要素值观测时段末的时间
    // 4. 对均匀时段信息报，观测时间码表示第一组数据的观测时间
    // 5. 一份报文中有不同观测时间的要素数据时，应同时编报要素对应的观测时间，要素的数据信息 编报在相应的观测时间组之后。观测时间组由观测时间标识符与观测时间组成
    // return get_current_report_time()>>8;
    return observetime -= observetime % 60;
    // return get_time_bcd(observetime) >> 8;
}
void set_observetime_time(time_t t) { observetime = t; }

//=====================================
Header::Header()
{
    this->liushui = get_current_liushui();
    this->local = param[0x02].val;
    this->observetime = get_observetime_time();
    this->sendtime = get_current_report_time();
    this->type = (u8)param["terminalType"].val; //自定义 遥测站分类码
}
Header::~Header() {}
int Header::add_extbytes(u8 *d)
{
    //广州:添加 rtu序号 + 版本信息
    if (rtu.device.gz.enable)
    {
        char buffer[10];
        // FF9F
        param[0xFF9F].toreport(buffer);
        memcpy(d, buffer, 2);
        memcpy(d + 2, buffer + 3, 2);

        // FFA1
        d += 4;
        param[0xFFA1].toreport(buffer);
        memcpy(d, buffer, 2);
        memcpy(d + 2, buffer + 3, 2);
        return 8;
    }
    return 0;
}
int Header::to_array_minimum(u8 *d)
{
    return add_extbytes(d);
}
int Header::to_array(u8 *d)
{
    ul64 tmp = 0;
    u8 *start = d;
    // 1.流水
    d += blreverse(&this->liushui, d, 2);
    // 2.发送时间
    tmp = get_time_bcd(sendtime);
    d += blreverse(&tmp, d, 6);
    // 3.终端
    *d = this->ST;
    d++;
    *d = this->ST;
    d++;
    tmp = this->local;
    inverseto((u8 *)&tmp, d, 5);
    // hex2bigbcd((u8 *)&tmp, 5);
    // memcpy(d, &tmp, 5);
    d += 5;

    // 4.终端类型
    *d = this->type;
    d++;
    // 5.观测时间
    *d = this->TT;
    d++;
    *d = this->TT;
    d++;
    tmp = get_time_bcd(observetime);
    d += blreverse((((u8 *)&tmp) + 1), d, 5);

    // 6.固定地额外数据
    d += add_extbytes(d);

    return (int)(d - start);
}

int Header::to_array_front3(u8 *d)
{
    ul64 tmp = 0;
    u8 *start = d;
    // 1.流水
    d += blreverse(&this->liushui, d, 2);
    // 2.发送时间
    tmp = get_time_bcd(sendtime);
    d += blreverse(&tmp, d, 6);
    // 3.终端
    *d = this->ST;
    d++;
    *d = this->ST;
    d++;
    tmp = this->local;
    inverseto((u8 *)&tmp, d, 5);
    // hex2bigbcd((u8 *)&tmp, 5);
    // memcpy(d, &tmp, 5);
    d += 5;
    // 4.固定地额外数据
    d += add_extbytes(d);
    return (int)(d - start);
}

string Header::toString()
{
    string s = toStringfront3();
    stringstream ss;
    ss << s;
    ss << "  终端类型:" << __gnu_cxx::__to_xstring<string>(&std::vsnprintf, 4 * sizeof(u8), "%02X", type)
       << "[" << YaoCeZhanStyleClass(type).getCode().name << "](" << YaoCeZhanStyleClass(type).getCode().info << ")"
       << "  观测时间:" << time2string(observetime).c_str() << endl;
    return ss.str();
}

string Header::toStringfront3()
{
    stringstream ss;
    ss << "流水序号:" << std::to_string(liushui) << "  发送时间:" << time2string(sendtime).c_str() << "  终端序号:" << std::to_string(local);
    return ss.str();
}

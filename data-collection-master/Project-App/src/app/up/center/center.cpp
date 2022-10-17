#include "ZhengWen.hpp"
#include "time.h"
#include "app.h"
#include "data_save.h"
//=============================实时数据===================================
#include "rtu_setting.hpp"
int get_shishi_all(u8 *dataout)
{
    ZhengWen<string> z;
    z.init(rtu.param.shishi, &devdata);
    return z.to_array(dataout);
}

//=============================时段数据===================================
static DataClass mylog(DataClassConfigType::Type_Dev, "mylog");
int get_shiduan_all(u8 *dataout, time_t begin, time_t end, time_t step, u8 id)
{
    return 0;
    // timestd(&end);
    // begin = end - LOG_TIME_INTERVAL * 3;
    // step = LOG_TIME_INTERVAL;
    // id = 0x20;

    // int size = 0;
    // time_t t = begin;
    // ZhengWen<u32> z;
    // vector<u32> ids;ids.push_back(id);
    // z.init(ids,&mylog);
    // int hsize = z.to_array(dataout);
    // int idlen = 0;

    // // header
    // dataout += hsize;
    // size += hsize;

    // //时间步长码 0418H
    // *dataout = 0x04;
    // dataout++;
    // *dataout = 0x18;
    // dataout++;
    // size += 2;

    // //要素标识符
    // *dataout = id & 0xFF;
    // dataout++;
    // *dataout = devdata[id].reportlen();
    // dataout++;
    // size += 2;

    // //时段数值
    // char reportdata[100];
    // idlen = devdata[id].len();
    // while (t <= end)
    // {
    //     devdata_db_read("hour",t-)
    //     // mylog.devdatafind(t,LOG_TIME_INTERVAL, DataLogUpDownType::ONLY_TIME);
    //     mylog[id].toreport(reportdata);
    //     memcpy(dataout, reportdata + 2, idlen);
    //     size += idlen;
    //     dataout += idlen;
    //     t += step;
    // }
    // return size;
}
// 步长单位 数据结构 范围
// 日 d0000 d 取值：01～31
// 小时 00h00 h 取值：01～23
// 分钟 0000m m 取值：01～59
int get_shiduan(u8 *datain, int len, u8 *dataout)
{
    // 2 sn
    // 6 time
    // 4 start
    // 4 end
    // 3 step
    // 1 id
    struct tm tptr;
    time_t start = 0, stop = 0, step = 0;

    tptr.tm_year = bcd2hex(datain + 8, 1) + 100;
    tptr.tm_mon = bcd2hex(datain + 9, 1) - 1;
    tptr.tm_mday = bcd2hex(datain + 10, 1);
    tptr.tm_hour = bcd2hex(datain + 11, 1);
    tptr.tm_sec = 0;
    tptr.tm_min = 0;
    start = mktime(&tptr);

    tptr.tm_year = bcd2hex(datain + 12, 1) + 100;
    tptr.tm_mon = bcd2hex(datain + 13, 1) - 1;
    tptr.tm_mday = bcd2hex(datain + 14, 1);
    tptr.tm_hour = bcd2hex(datain + 15, 1);
    tptr.tm_sec = 0;
    tptr.tm_min = 0;
    stop = mktime(&tptr);

    step = bcd2hex(datain + 16, 1) * 24 * 3600;
    step += bcd2hex(datain + 17, 1) * 3600;
    step += bcd2hex(datain + 18, 1) * 60;

    u8 id = datain[19];

    return get_shiduan_all(dataout, start, stop, step, id);
}

//=============================查人工置数===================================
// report.cpp

//=============================指定要素实时数据=============================

int get_zhidingids_shishi(u8 *dataout, u8 *ids, int count)
{
    ZhengWen<u32> z;
    vector<u32> items;
    for (int i = 0; i < count; i++)
        items.push_back(ids[i]);
    z.init(items, &devdata);
    return z.to_array(dataout);
}
int get_zhiding_ids(u8 *datain, int len, u8 *dataout)
{
    int count = (len - 8) / 2;
    if (count <= 0)
        return 0;
    u8 ids[200];
    for (int i = 0; i < count; i++)
    {
        ids[i] = datain[8 + i * 2];
    }
    return get_zhidingids_shishi(dataout, ids, count);
}
//=============================修改配置表=============================
// 7e7e019900000001123440801602001122032411102203202244 0D4011223344556677880390AB
int set_param(u8 *datain, int size, u8 *dataout)
{
    u8 *startdata = dataout;
    u8 ids[200];
    int count = 0;
    int offset = 8;
    memset(ids, 0, sizeof(ids));
    //设置参数
    while (offset < size)
    {
        ids[count] = datain[offset];
        int ilen = param[ids[count]].fromreport((char *)(datain + offset));
        count++;
        offset += 2;
        if (ilen > 0)
        {
            offset += ilen;
            continue;
        }
        else
        {
            PRINTBYTES("set_param:invalid", datain + offset, size - offset);
            return 0;
        }
    }
    //重读参数
    {
        Header h;
        int rlen = h.to_array_front3(dataout);
        dataout += rlen;
        int i = 0;
        while (i < count)
        {
            int ilen = param[ids[i]].toreport((char *)dataout);
            if (ilen > 0)
            {
                dataout += ilen;
            }
            else
            {
                ERROR("[set_param:: can not get:%02d", ids[i]);
            }
            i++;
        }
    }

    // param保存到文件
    param_save(&rtu);

    return (int)(dataout - startdata);
}

//=============================读取配置表=============================
int get_param(u8 *datain, int size, u8 *dataout)
{
    //重读参数
    u8 *startdata = dataout;
    u8 *sd = datain + 8;
    size -= 8;
    Header h;
    dataout += h.to_array_front3(dataout);

    int i = 0;
    while (i < (size / 2))
    {
        int ilen = param[sd[i * 2]].toreport((char *)dataout);
        INFO("%s", param[sd[i * 2]].toString(param.Name().c_str()).c_str());
        if (ilen > 0)
        {
            dataout += ilen;
        }
        else
        {
            ERROR("[get_param:: can not get:%02d", sd[i * 2]);
        }
        i++;
    }
    return (int)(dataout - startdata);
}

//=============================修改运行表=============================
int set_runtime(u8 *datain, int size, u8 *dataout)
{
    u8 *startdata = dataout;
    u8 ids[200];
    int count = 0;
    int offset = 8;
    //设置参数
    while (offset < size)
    {
        ids[count] = datain[offset];
        int ilen = runtime[ids[count]].fromreport((char *)(datain + offset)); //(datain+offset);
        count++;
        offset += 2;
        if (ilen > 0)
        {
            offset += ilen;
            continue;
        }
        else
        {
            PRINTBYTES("set_runtime:invalid", datain + offset, size - offset);
            return 0;
        }
    }
    //重读参数
    {
        Header h;
        int rlen = h.to_array_front3(dataout);
        dataout += rlen;
        int i = 0;
        while (i < count)
        {
            int ilen = runtime[ids[i]].toreport((char *)dataout); //,NULL,dataout);
            if (ilen > 0)
            {
                dataout += ilen;
            }
            else
            {
                ERROR("[set_runtime:: can not get:%02d", ids[i]);
            }
            i++;
        }
    }
    runtime_save(&rtu);

    return (int)(dataout - startdata);
}

//=============================读取运行表=============================
int get_runtime(u8 *datain, int size, u8 *dataout)
{
    //重读参数
    int ilen = 0;
    u8 *startdata = dataout;
    u8 *sd = datain + 8;
    size -= 8;
    Header h;
    dataout += h.to_array_front3(dataout);

    int i = 0;
    while (i < (size / 2))
    {
        u32 id = sd[i * 2];
        DataClassItem *item = &runtime[id];
        INFO("%s", item->toString(runtime.Name().c_str()).c_str());
        if (item->config.len != 0)
        {
            ilen += item->toreport((char *)dataout);
            dataout += ilen;
        }
        else
        {
            ERROR("[get_runtime:: can not get:%02d", sd[i * 2]);
        }
        i++;
    }
    return (int)(dataout - startdata);
}

//=============================查询水泵电机实时工作数据==================
int get_shishi_motor(u8 *dataout)
{
    return 0;
    // ZhengWen<string> z;
    // //三相电压，电流
    // const char *item[] = {"VTA", "VTB", "VTC", "VIA", "VIB", "VIC"};
    // z.init(item, sizeof(item) / sizeof(char *));
    // return z.to_array(dataout);
}

//=============================站查询遥测站软件版本=====================
#include "VersionConfig.h"
int get_version(u8 *dataout)
{
    // 1 流水号 流水号 2字节HEX码，范围1～65535
    // 2 发报时间 发报时间 6字节BCD码，YYMMDDHHmmSS
    // 3 遥测站地址 地址标识符
    // 遥测站地址 编码规则见6.2.3.2
    // 4 遥测站软件版本信息
    // 版本信息字节数 1字节HEX
    // 遥测站软件版本信息
    u8 *start = dataout;
    char *version = (char *)VERSION_BUILD_TIME;
    int vlen = strlen(version);
    Header h;
    dataout += h.to_array_front3(dataout);
    *dataout = vlen;
    dataout++;
    memcpy(dataout, version, vlen);
    dataout += vlen;
    INFO("当前版本=%s", VERSION_BUILD_TIME);
    return (int)(dataout - start);
}

//============================遥测站状态和报警信息定义表=================
int get_state_warn_info(u8 *dataout)
{
    u8 *start = dataout;
    Header h;
    dataout += h.to_array_front3(dataout);
    dataout += devdata["ZT"].toreport((char *)dataout);
    StateWarn sw(devdata["ZT"].hex);
    INFO("%s", sw.toString().c_str());
    return (int)(dataout - start);
}

//============================初始化固态存储数据========================
//遥测站固态数据区全部初始化，清除历史数据，功能码为47H
int clear_history(u8 *datain, int len, u8 *dataout)
{
    //删除记录
    extern bool data_memory_clear(void);
    bool result = data_memory_clear();
    // //返回 front3
    Header h;
    return h.to_array_front3(dataout);
}

//============================恢复遥测站出厂设置========================
int reset_to_factory(u8 *datain, int len, u8 *dataout)
{
    // 1.参数初始化
    extern bool data_set_factory(void);
    bool result = data_set_factory();
    //返回 front3
    Header h;
    return h.to_array_front3(dataout);
}

//============================修改密码=================================
int change_password(u8 *datain, int len, u8 *dataout)
{
    u8 result[50];
    u8 *pdata = datain + 8;
    u16 pass = 0;
    pass = param[0x03].val;
    u16 opass = pdata[2];
    opass <<= 8;
    opass |= pdata[3];
    u16 npass = pdata[6];
    npass <<= 8;
    npass |= pdata[7];
    //流水，发报时间
    // 03H + password
    if (pdata[0] == 0x03 && opass == pass)
    {
        Header h;
        param[0x03].val = npass;
        param_save(&rtu);
        npass = param[0x03].val;
        int l = h.to_array_front3(result); //丢掉 类型，观测时间 8 bytes
        memcpy(dataout, result, l);
        dataout[l] = 0x03;
        l++;
        dataout[l] = 0x10;
        l++;
        dataout[l] = (npass >> 8) & 0xFF;
        l++;
        dataout[l] = (npass & 0xFF);
        l++;
        return l;
    }
    return 0;
}
//=============================设置遥测站时钟============================
int set_time(u8 *datain, int len, u8 *dataout)
{
    //时间大于5min,要2次校时
    static bool second_time = false;

    // YYMMDDHHmmss = datain[2-7]
    int YY = bcd2hex(datain + 2, 1) + 2000;
    int MM = bcd2hex(datain + 3, 1);
    int DD = bcd2hex(datain + 4, 1);
    int HH = bcd2hex(datain + 5, 1);
    int mm = bcd2hex(datain + 6, 1);
    int ss = bcd2hex(datain + 7, 1);

    tm stm;
    stm.tm_year = YY - 1900;
    stm.tm_mon = MM - 1;
    stm.tm_mday = DD;
    stm.tm_hour = HH;
    stm.tm_min = mm;
    stm.tm_sec = ss;
    time_t t = mktime(&stm);
    time_t now = timestd(0);

    if (second_time == false)
    {
        second_time = false;
        //大于5min，二次校时
        if ((t > (now + 300)) || (t < (now - 300)))
        {
            INFO("[设置时钟]: 失败. 大于 5min, 当前=%s, 设置时间为=%s ", time2string(now).c_str(), time2string(t).c_str());
            second_time = true;
            return -1;
        }
    }
    INFO("[设置时钟]: 成功. 当前=%s, 设置时间为=%s ", time2string(now).c_str(), time2string(t).c_str());
    second_time = false;
    TimeSet(YY, MM, DD, HH, mm, ss);
    Header h;
    return h.to_array_front3(dataout);
}
int get_time(u8 *dataout) //中心站查询遥测站时钟
{
    Header h;
    return h.to_array_front3(dataout);
}

//=============================设置遥测站 IC 卡状态============================
int set_IC_state(u8 *datain, int len, u8 *dataout)
{

    return 0;
}

//=============================控制水泵开关命令/水泵状态自报=====================
int set_moter_onoff(u8 *datain, int len, u8 *dataout)
{

    return 0;
}
//=============================控制阀门开关命令/阀门状态信息自报=============================
int set_famen_onoff(u8 *datain, int len, u8 *dataout)
{

    return 0;
}
//=============================控制闸门开关命令/闸门状态信息自报=============================
int set_zhamen_onoff(u8 *datain, int len, u8 *dataout)
{

    return 0;
}

//=============================水量定值控制命令=============================
int set_shuiliang_control(u8 *datain, int len, u8 *dataout)
{

    return 0;
}

//=============================中心站查询遥测站事件记录=============================
int get_event_state(u8 *datain, int len, u8 *dataout)
{
    return 0;
}

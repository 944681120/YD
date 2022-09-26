
#include "app.h"
#include "lib.h"
#include "DB.hpp"
#include "DataClass.hpp"
#include "data_save.h"
#define WATERLEVEL_DATA_NULL DEVICE_DATA_VALUE_NULL

/*===================================================================
功能介绍:

1 采集:
  8路水位传感器 （3次失败，表58 遥测站状态和报警信息定义表对应位置位)
    Z1-Z8  = 传感器值 + 基值 + 修正值
  Z 默认使用Z1 的值

2.5min记录
    DRZ1-DRZ8

3.加报
    Z1-Z8 加报水位:
                  水位值 与 加报水位 比较 =>是否加报
           上加报：
           下加报：
                  (当前5min水位) - (上5min水位) <0 比较下水位是否加报
                                              >0 比较上水位是否加报

    加报定时在采集后检测,需要加报时，添加到 table = Jia

流程 ：
       数据采集 ==> 计算Z1-Z8 ==> 记录 5min数据 ==> 加报比较
==========================================================================*/
#define WATERLEVEL_ID_MIN 0x3C
#define WATERLEVEL_ID_MAX 0x43
#define WATERLEVEL_DRZID_MIN 0xF5
#define WATERLEVEL_DRZID_MAX 0xFC
#define WATERLEVEL_BASEID_MIN 0x28
#define WATERLEVEL_FIXID_MIN 0x30
// DRZ 是2位小数点
static int set_DRZ(u16 id, int val, int off)
{
    if (id < WATERLEVEL_DRZID_MIN || id > WATERLEVEL_DRZID_MAX)
        return 1;
    devdata[id].hex[off * 2] = (val >> 8) & 0xFF;
    devdata[id].hex[off * 2 + 1] = (val)&0xFF;
    return 0;
}
static u16 get_DRZ(DataClass *pdata, int index, int off)
{
    u16 v = 0;
    u32 id = 0xF5 + index;
    off %= 12;
    if (pdata == nullptr)
        return 0xFFFF;
    v = (*pdata)[id].hex[off * 2] & 0xFF;
    v <<= 8;
    v |= (*pdata)[id].hex[off * 2 + 1] & 0xFF;
    return v;
}
static int reset_DRZ(void)
{
    INFO("DRZ(1-8) reset to FFFF");
    for (int i = 0; i < 8; i++)
    {
        devdata[WATERLEVEL_DRZID_MIN + i].clear();
    }
    return 0;
}

// z1-zn
static int waterlevel_addbase(u32 id, l64 val, bool clear)
{
    if (id < WATERLEVEL_ID_MIN || id > WATERLEVEL_ID_MAX)
        return 1;

    int offset = id - WATERLEVEL_ID_MIN;
    int baseid = offset + WATERLEVEL_BASEID_MIN;
    int fixid = offset + WATERLEVEL_FIXID_MIN;
    int DRZid = offset + WATERLEVEL_DRZID_MIN;
    string Zn = "Z" + std::to_string(1 + offset);

    if (clear)
    {
        devdata[id].clear();
        set_DRZ(DRZid, 0xFFFF, TimeOffset5Min(std::time(0)));
        ERROR("[水位 %s],清零", Zn.c_str());
    }
    else
    {
        // dev data
        devdata[id].val = val + runtime[baseid].Value() + runtime[fixid].Value();
        INFO("[水位 %s] (%jd) = 传感器(%jd) + 基值(%jd) + 修正值(%jd)", Zn.c_str(), devdata[id].Value(), val, runtime[baseid].Value(), runtime[fixid].Value());
        // DRZ data
        set_DRZ(DRZid, devdata[id].val / 10, TimeOffset5Min(std::time(0)));
    }
    return 0;
}

extern DataClass wl_log;
void waterlevel_warn_check(void)
{
    time_t t = timestd(0);
    struct tm *tt = localtime((const time_t *)&t);
    bool last_log = false;
    // limit
    volatile l64 limit = 0;
    volatile l64 up_limit = runtime[0x40].Value();
    volatile l64 down_limit = runtime[0x40].Value();
    volatile l64 last = 0xFFFF;
    volatile l64 now = 0xFFFF;
    DataClass *pdata = nullptr;

    //前5分钟水位
    if (tt->tm_min < 5)
    {
        pdata = new DataClass(DataClassConfigType::Type_Dev, "waterlevel");
        if (devdata_db_read("hour", (t - (t % 3600) - 3600), pdata) > 0)
            last_log = true;
    }

    //多路水位检测
    for (int i = 0; i < 8; i++)
    {
        //最后水位
        if (tt->tm_min < 5)
        {
            if (last_log)
                last = get_DRZ(pdata, i, 11);
        }
        else
        {
            last = get_DRZ(&devdata, i, (tt->tm_min / 5) - 1);
        }
        //当前水位
        now = get_DRZ(&devdata, i, (tt->tm_min / 5));
        if (now == 0xFFFF)
            continue;

        //上次不记录，
        if (last == 0xFFFF)
            last = now;
        else
        {
            int cha = now - last;
            //上水位
            if (cha > 0 && up_limit > 0 && cha > up_limit)
            {
                INFO("[上水位报警] 水位:%d  上次水位=%jd  当前水位=%jd   报警值=%jd", i + 1, last, now, up_limit);
                jia_add_flags("waterlevel");
            }

            //下水位
            if (cha < 0 && down_limit > 0 && (-cha) > down_limit)
            {
                INFO("[下水位报警] 水位:%d  上次水位=%jd  当前水位=%jd   报警值=%jd", i + 1, last, now, down_limit);
                jia_add_flags("waterlevel");
            }
        }
        //加报水位
        limit = runtime[0x38 + i].Value();

        if (limit > 0 && now != 0xFFFF)
        {
            if (now > limit)
            {
                INFO("[加报水位报警] 水位:%d  上次水位=%jd  当前水位=%jd   报警值=%jd", i + 1, last, now, limit);
                jia_add_flags("waterlevel");
            }
        }
    }

    if (pdata != nullptr)
        delete pdata;
}

//===================================水位初始化==============================================//
int init_waterlevel(void)
{
    time_t tnow = timestd(0);
    if (is_diff_hour(tnow, devdata.now) == true)
    {
        INFO("跨时水位重新计算: now =%s log=%s", time2string(tnow, "%F %X").c_str(), time2string(devdata.now).c_str());
        reset_DRZ();
        return 0;
    }
    devdata["Z"].clear();
    for (int i = WATERLEVEL_ID_MIN; i <= WATERLEVEL_ID_MAX; i++)
    {
        devdata[i].clear();
    }
    return 0;
}

//===================================水位计算==============================================//
void waterlevel_caculation(l64 wl, DataClassItemConfig *pconfig, bool result, bool clear)
{
    // 3次读取失败，空值 0xAA
    if (clear == true)
    {
        INFO("水位传感器 3次 失败清零AA (name=%s,id=%02x)", pconfig->name.c_str(), pconfig->id);
        waterlevel_addbase(pconfig->id, wl, clear);
    }
    else if (result == true)
    {
        // Z1-Zn水位 = 传感器 + base + fix值
        if (pconfig->id >= WATERLEVEL_ID_MIN && pconfig->id <= WATERLEVEL_ID_MAX)
        {
            waterlevel_addbase(pconfig->id, wl, false);
        }
        else
        {
            devdata[pconfig->id].val = wl;
            INFO("%s", devdata[pconfig->id].toString(devdata.Name().c_str()).c_str());
        }

        // warn check
        if (result == true)
            waterlevel_warn_check();
    }

    // Z 取 水位1 Z1
    devdata[0x39].val = devdata[0x3C].val;
}

//===================================水位poll==============================================//
void waterlevel_poll(void)
{
    static time_t lasttime = 0;
    if (lasttime == 0)
        lasttime = timestd(0);

    // INFO("%s ============= %s", time2string(lasttime).c_str(), time2string(timestd(0)).c_str());
    if (is_diff_hour(lasttime, timestd(0)))
    {
        INFO("[水位跨时] DRZ clear.");
        lasttime = timestd(0);
        reset_DRZ();
    }
}
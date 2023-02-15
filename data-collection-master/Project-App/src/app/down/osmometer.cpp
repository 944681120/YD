
#include "app.h"
#include "lib.h"
#include "DB.hpp"
#include "json.hpp"
#include "data_save.h"

/*==========================================================================
功能介绍:
==========================================================================*/
#define osmometer_ID_MIN 0x28
#define osmometer_ID_MAX 0x2F
#define OSMOMETER_FRE_ID_MIN 0xFFC0     //渗压频率
#define OSMOMETER_FRE_ID_MAX 0xFFDF
#define OSMOMETER_OTH_ID_MIN 0xFF00     //渗压温度
#define OSMOMETER_OTH_ID_MAX 0xFF42
#define OSMOMETER_WATER_ID_MIN 0xFF20   //渗压水位
#define OSMOMETER_WATER_ID_MAX 0xFF3F   
#define OSMOMETER_SETTING_FILE "/app-cjq/setting/syj_setting.json"
        
struct osmometer_cfg_s
{
    string describe;        //"describe": "渗压计 通道2 参数配置",
    double data_g;          // "data_g": 1.1,
    double data_r;          // "data_r": 2.2,
    double elevation;       // "elevation": 3.3

    JSON_BIND(osmometer_cfg_s, describe, data_g, data_r, elevation);
};

// osmometer 直接保存
static int osmometer_addbase(u32 id, l64 val, bool clear)
{
    if (clear)
    {
        devdata[id].clear();
        INFO("复位渗压 %s", devdata[id].toString(devdata.Name().c_str()).c_str());
        return 0;
    }

    if ( (id >= OSMOMETER_OTH_ID_MIN && id <= OSMOMETER_OTH_ID_MAX) || (id >= OSMOMETER_FRE_ID_MIN && id <= OSMOMETER_FRE_ID_MAX) )
    {
        devdata[id].val = val;
        INFO("渗压计赋值 %s", devdata[id].toString(devdata.Name().c_str()).c_str());
        return 0;
    }
    return 1;
}

static void osmometer_warn_check(void)
{
    // l64 limit = runtime[0x42].Value();
    // if (limit > 0)
    // {
    //     for (int i = 0x27; i < 0x31; i++)
    //     {
    //         l64 q = devdata[i].Value();
    //         if (q > limit)
    //         {
    //             jia_add_flags("liuliang");
    //         }
    //     }
    // }
    // //渗压加报阈值 42H N(6,3) 数据单位：立方米每秒
    // // 27H-31H
}
//===================================渗压初始化==============================================//
int init_osmometer(void)
{
    time_t tnow = timestd(0);
    if (is_diff_hour(tnow, devdata.now) == true)
    {
        // INFO("跨时渗压重新计算: now =%s log=%s", time2string(tnow, "%F %X").c_str(), time2string(devdata.now).c_str());
        return 0;
    }

    for (int i = OSMOMETER_OTH_ID_MIN; i <= OSMOMETER_OTH_ID_MAX; i++)
    {
        devdata[i].clear();
    }

    for (int i = OSMOMETER_FRE_ID_MIN; i <= OSMOMETER_FRE_ID_MAX; i++)
    {
        devdata[i].clear();
    }

    return 0;
}

//===================================渗压计算==============================================//
uint16_t osmometer_getStep(uint16_t id)   
{
    uint16_t step = 0;

    if ( id >= OSMOMETER_FRE_ID_MIN )
    {
        step = id - OSMOMETER_FRE_ID_MIN;
    }
    else
    {
        step = id - OSMOMETER_OTH_ID_MIN;
    }
    
    return step;
}

void osmometer_caculation_waterlevel(uint16_t id, void* para)
{
    struct _DEALAT_ST
    {
        int dealAtFlag;
        int ch;
        double T, P, A, B, C, a, b, c, R;
    }* pDealat = (struct _DEALAT_ST*)para;;
    /* 读取配置文件 */
    json jarr;
    jarr.clear();
    std::ifstream ifs(OSMOMETER_SETTING_FILE);
    ifs >> jarr;
    /* 分捡配置数组 */
    json item;
    string preChName = "water_ch";
    osmometer_cfg_s osmcfg[128];      //最多支持128通道水位
    for (int i = 0; i < 128; i++)
    {
        item = jarr[ (preChName + std::to_string(i + 1)).c_str() ];
        if ( item.empty() == true ) 
        {
            continue;
        }
        osmcfg[i] = (osmometer_cfg_s)item;
    }
    
    /* 水位计算 */
    uint16_t step = osmometer_getStep(id);
    l64 freVal  = devdata[step + OSMOMETER_FRE_ID_MIN].val;
    l64 tempVal = devdata[step + OSMOMETER_OTH_ID_MIN].val;
    INFO("水位计算................%f", pDealat->P);
    if ( freVal != DATA_INVALID_VALUE && tempVal != DATA_INVALID_VALUE )
    {
        devdata[step + OSMOMETER_WATER_ID_MIN].val = pDealat->P;
    }
}

void osmometer_caculation(l64 wl, DataClassItemConfig *pconfig, bool result, bool clear, void* para)
{
    // 3次读取失败，空值
    if (clear == true)
    {
        INFO("渗压传感器 3次 失败清零 (name=%s,id=%02x)", pconfig->name.c_str(), pconfig->id);
        osmometer_addbase(pconfig->id, wl, clear);
    }

    if (result == true)
    {
        // Q1-Q8渗压 = 传感器   QC1-QC8
        if (osmometer_addbase(pconfig->id, wl, false) == 1)
        {
            devdata[pconfig->id].val = wl;
            INFO("其他渗压 %s", devdata[pconfig->id].toString(devdata.Name().c_str()).c_str());
        }

        osmometer_warn_check();

        osmometer_caculation_waterlevel(pconfig->id, para);
    }
    // #define DATA_INVALID_VALUE 0x7FFFFFFFFFFFFFFF
}


//===================================渗压poll==============================================//
void osmometer_poll(void)
{
    static time_t lasttime = 0;
    if (lasttime == 0)
        lasttime = timestd(0);

    if (is_diff_hour(lasttime, timestd(0)))
    {
        lasttime = timestd(0);
    }
    //报警检查
    osmometer_warn_check();
}
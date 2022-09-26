
#include "app.h"
#include "lib.h"
#include "DB.hpp"
#include "data_save.h"

#define FLOW_DATA_NULL DEVICE_DATA_VALUE_NULL
/*===================================================================
功能介绍:

1 采集:
  8路流量传感器 （3次失败，表58 遥测站状态和报警信息定义表对应位置位)
    瞬时: Q1-Q8  = 传感器值
  Q 默认使用Q1 的值
    累积: QC1-QC8 = 传感器   （自定义）

3.加报
    Q1-Q8 加报流量:
                 流量加报阈值 42H N(6,3) 数据单位：立方米每秒

    加报定时在采集后检测,需要加报时，添加到 table = Jia

流程 ：
       数据采集 ==> 计算Q1-Q8 ==> 加报比较
==========================================================================*/
#define FLOW_ID_MIN 0x28
#define FLOW_ID_MAX 0x2F
#define FLOWACC_ID_MIN 0xFFA1
#define FLOWACC_ID_MAX 0xFFA8
// Q1-Qn
// flow 直接保存
static int flow_addbase(u32 id, l64 val, bool clear)
{
    if (clear)
    {
        devdata[id].clear();
        INFO("复位流量 %s", devdata[id].toString(devdata.Name().c_str()).c_str());
        return 0;
    }

    if ((id >= FLOW_ID_MIN) && (id <= FLOW_ID_MAX))
    {
        devdata[id].val = val;
        INFO("瞬时流量 %s", devdata[id].toString(devdata.Name().c_str()).c_str());
        return 0;
    }
    else if ((id >= FLOWACC_ID_MIN) && (id <= FLOWACC_ID_MAX))
    {
        devdata[id].val = val;
        INFO("累积流量 %s", devdata[id].toString(devdata.Name().c_str()).c_str());
        return 0;
    }
    return 1;
}

static void flow_warn_check(void)
{
    l64 limit = runtime[0x42].Value();
    if (limit > 0)
    {
        for (int i = 0x27; i < 0x31; i++)
        {
            l64 q = devdata[i].Value();
            if (q > limit)
            {
                jia_add_flags("liuliang");
            }
        }
    }
    //流量加报阈值 42H N(6,3) 数据单位：立方米每秒
    // 27H-31H
}
//===================================流量初始化==============================================//
int init_flow(void)
{
    time_t tnow = timestd(0);
    if (is_diff_hour(tnow, devdata.now) == true)
    {
        // INFO("跨时流量重新计算: now =%s log=%s", time2string(tnow, "%F %X").c_str(), time2string(devdata.now).c_str());
        return 0;
    }
    devdata["Q"].clear();
    for (int i = FLOW_ID_MIN; i <= FLOW_ID_MAX; i++)
    {
        devdata[i].clear();
    }
    return 0;
}

//===================================流量计算==============================================//
void flow_caculation(l64 wl, DataClassItemConfig *pconfig, bool result, bool clear)
{
    // 3次读取失败，空值 0xAA
    if (clear == true)
    {
        INFO("流量传感器 3次 失败清零AA (name=%s,id=%02x)", pconfig->name.c_str(), pconfig->id);
        flow_addbase(pconfig->id, wl, clear);
    }
    else if (result == true)
    {
        // Q1-Q8流量 = 传感器   QC1-QC8
        if (flow_addbase(pconfig->id, wl, false) == 1)
        {
            devdata[pconfig->id].val = wl;
            INFO("其他流量 %s", devdata[pconfig->id].toString(devdata.Name().c_str()).c_str());
        }

        flow_warn_check();
    }

    // Q 取 Q1
    devdata[0x27].val = devdata[0x28].val;
}

//===================================流量poll==============================================//
void flow_poll(void)
{
    static time_t lasttime = 0;
    if (lasttime == 0)
        lasttime = timestd(0);

    // INFO("%s ============= %s", time2string(lasttime).c_str(), time2string(timestd(0)).c_str());
    if (is_diff_hour(lasttime, timestd(0)))
    {
        // INFO("[流量跨时] DRZ clear.");
        lasttime = timestd(0);
    }
    //报警检查
    flow_warn_check();
}
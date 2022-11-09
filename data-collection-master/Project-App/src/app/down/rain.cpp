
#include "lib.h"
#include "data_save.h"
#include "DB.hpp"

//累积雨量
bool rainacc_save(void)
{
    DB<TbImportant> dblog(DBName, TableNameImportant);
    TbImportant tb;
    tb.name = "rainacc";
    tb.date = time2string(timestd(0));
    tb.data = A["rainAcc"].toreport();
    u8 crc = crc8((u8 *)tb.data.c_str(), tb.data.size());
    tb.code = std::to_string(crc);
    dblog.update("name=\"rainacc\"", &tb);
    INFO("[保存]%s", tb.toString().c_str());
    return true;
}

bool rainacc_recovery(void)
{
    DB<TbImportant> dblog(DBName, TableNameImportant);
    vector<TbImportant> msgs;
    dblog.find("name=\"rainacc\"", msgs);
    if (msgs.size() > 0)
    {
        TbImportant msg = msgs[0];
        INFO("[读取]%s", msg.toString().c_str());

        u8 crc = crc8((u8 *)msg.data.c_str(), msg.data.size());
        if (msg.code.compare(std::to_string(crc)) == 0)
        {
            INFO("[crc] OK = %02x", crc);
            A["rainAcc"].fromreport(msg.data);

            if (devdata["PT"].val != A["rainAcc"].val)
            {
                ERROR("[rainacc]: PT=%jd, rainAcc=%jd, devdata[PT] = rainAcc.", devdata["PT"].val, A["rainAcc"].val);
                devdata["PT"].val = A["rainAcc"].val;
            }

            return true;
        }
        else
        {
            ERROR("[crc] ERR = %02x,but table=%s", crc, msg.code.c_str());
            return false;
        }
    }
    else
    {
        ERROR("无法从Important表中读取 rainacc. 清零 ！");
        A["rainAcc"].val = 0;
        rainacc_save();
        return false;
    }
    return false;
}

/*===================================雨量计算==============================================
//devdata:所有有雨量单位都是 0.1mm
//runtime:分辨率单位 0.1mm
//runtime:雨量加报阈值 1mm

1.累积雨量     0x26 PT    ==>单独保存到 important 中，平时备份到 A()中
2.小时累计雨量  0x1A P1
3.5min时段雨量 0xF4 DRP


4.当前降雨量   0x20 PJ
5.日降雨量     0x1F PD
相关的参数是：  {	0x22	,"RainDayStartTime"   ,	N(2,0)	,	DF_BCD,	"降水量日起始时间"},

              |<---------24 Hour--------->||<-------------24 Hour--------->|
--------------[---------------------------][-------------------------------]---------
              |<---------日降雨量--------->||<-------当前降雨量-----> |
        RainDayStartTime              RainDayStartTime           当前时间


关于数据保存：
    1.5min =>log
    2.不保存时/数据无效时，为0

===================================雨量计算==============================================*/
time_t GetRainDayStartTime(time_t tnow)
{
    tnow -= tnow % 3600; //小时取整
    struct tm *pnow = localtime((const time_t *)&tnow);
    time_t start = 0;
    int RainDayStartTime = (int)runtime["RainDayStartTime"].val;
    INFO("雨量日起始时间 = %d Hour", RainDayStartTime);
    if (pnow->tm_hour >= RainDayStartTime)
        start = tnow - (pnow->tm_hour - RainDayStartTime) * 3600;
    else
        start = tnow - 24 * 3600 + (RainDayStartTime - pnow->tm_hour) * 3600;
    return start;
}

static l64 P1_rain(time_t from, time_t to)
{
    from -= from %3600;
    to -= to % 3600;
    l64 current = 0;
    vector<TbBase> msgs;
    DB<TbBase> dblog(DBName, TableNameHour);
    dblog.find("date>=\"" + time2string(from) + "\" and date<\"" + time2string(to) + "\"", msgs);

    if (msgs.size() > 0)
    {
        for (int i = 0; i < msgs.size(); i++)
        {
            DataClass tlog(DataClassConfigType::Type_Dev, "tlog");
            TbBase tb = msgs[i];
            tlog.fromlog(tb.data);
            // cout<< tb.toString()<<endl;
            if (tlog["P1"].isZero())
                continue;
            current += tlog["P1"].val;
        }
    }
    return current;
}

void init_currentRain(time_t t)
{
    time_t from = GetRainDayStartTime(timestd(0));
    time_t now = timestd(0);
    now -= now % 3600;
    l64 current = P1_rain(from,now);
    INFO("当前雨量=%jd", current);
    devdata["PJ"].val = current;
}

void init_dayRain(time_t t)
{
    //超过1小时:从历史hour中计算
    time_t from = GetRainDayStartTime(timestd(0)-24*3600);
    time_t to   = GetRainDayStartTime(timestd(0));
    l64 val = P1_rain(from,to);
    INFO("日降雨量=%jd",val);
    devdata["PD"].val = val;
}

static int init_5min(void)
{
    memset(devdata["DRP"].hex, 0xFF, devdata["DRP"].len());
    devdata["P1"].val = 0;
    return 0;
}

//++
static int increase(u8 id, int v)
{
    if (devdata[id].isZero())
    {
        devdata[id].val = v;
    }
    else
        devdata[id].val += v;

    return devdata[id].val;
}

// DRP
static int increase_DRP(int v, int off)
{
    if (devdata[0xF4].hex[off] == 0xFF)
        devdata[0xF4].hex[off] = v;
    else
        devdata[0xF4].hex[off] += v;
    INFO("DRP off=%d inc=%d val =%d", off, v,devdata[0xF4].hex[off]);
    return v;
}

// Alarm
static void rain_warn_check(void)
{
    if (A["rainIncFlag"].val == 1)
    {
        time_t t = timestd(0);
        struct tm *tt = localtime((const time_t *)&t);
        int index = tt->tm_min / 5;
        A["rainIncFlag"].val = 0;
        u8 F4 = devdata[0xF4].hex[index];
        int yuzhi = runtime[0x27].val * 10;

        // F4 0.1mm    27   1mm
        if (F4 != 0xFF && F4 >= yuzhi)
        {
            INFO("雨量加报 , 当前=%d  加报阈值=%d", F4, yuzhi);
            A["rainAlarmFlags"].val |= 0x01;
            jia_add_flags("rain");
        }
    }
}

//===================================雨量初始化==============================================//
//重新加载(当前降雨量,日降雨量)
int init_rain(void)
{
    time_t tnow = timestd(0);
    if (is_diff_hour(tnow, devdata.now) == true)
    {
        INFO("跨时雨量重新计算: now =%s log=%s", time2string(tnow, "%F %X").c_str(), time2string(devdata.now).c_str());
        init_currentRain(tnow);
        init_dayRain(tnow);
        init_5min();
        return 0;
    }
    INFO("雨量OK: now =%s log=%s", time2string(tnow, "%F %X").c_str(), time2string(devdata.now).c_str());
    return 0;
}

//===================================雨量计算==============================================//
void rain_caculation(int yuliangzengliang)
{
    tm *tm_now;
    time_t t = timestd(0);
    struct tm *tt = localtime((const time_t *)&t);
    int inc = yuliangzengliang;

    //------------------------单位
    u16 u = (u16)runtime[0x25].val;
    INFO("雨量计数 =%d, 雨量单位=%d", yuliangzengliang, u);
    inc *= yuliangzengliang * u;

    if (yuliangzengliang > 0)
    {
        extern void set_observetime_time(time_t t);
        set_observetime_time(timestd(0));

        A["rainIncFlag"].val = 1; //标志

        // 0.1小时内降雨量 P1
        increase(0x1A,inc);
        INFO("小时雨量 P1=%jd",devdata[0x1A].val);

        // 1.当前降水 20H PJ
        // 是指最近日起始时间开始统计至当前时刻的降水总量。
        increase(0x20, inc);
        INFO("当前降水 PJ=%jd", devdata[0x20].val);

        // 2.F4H DRP  1 小时内每 5 分钟时段雨量 e（每组雨量占 1 字节 HEX，最大值 25.4 毫米，数据中不含小数点；FFH表示非法数据。）
        increase_DRP(inc, tt->tm_min / 5);

        // 4.累计雨量  26H PT
        increase(0x26, inc);
        INFO("累计雨量 PT=%jd", devdata[0x26].val);
        if(A["rainAcc"].isZero())
        {
            A["rainAcc"].val = devdata[0x26].val;
        }else{
            A["rainAcc"].val+=inc;
        }
    }
}

//===================================雨量时间poll==============================================//
void rain_poll(void)
{
    static time_t lasttime = 0;
    if (lasttime == 0)
        lasttime = timestd(0);

    if ( is_diff_hour(lasttime,timestd(0)))
    {
        INFO("last :%s ============= now :%s",time2string(lasttime).c_str(),time2string(timestd(0)).c_str());
        lasttime = timestd(0);
        init_5min();
        init_dayRain(timestd(0));
        init_currentRain(timestd(0));
    }

    //加报检测
    rain_warn_check();
}
#include "lib.h"
#include "json.hpp"
#include "DB.hpp"
#include "data_save.h"
#include "app.h"
#include "DataClass.hpp"
#include <signal.h> //头文件

#include "data_save.h"
#include "unistd.h"

rtu_setting rtu;

//延时重启
bool yanshichongqi = false;

//雨量计算
int init_rain(void);
void rain_caculation(int yuliangzengliang);
void rain_poll(void);
//水位计算
int init_waterlevel(void);
void waterlevel_caculation(int yuliangzengliang);
void waterlevel_poll(void);
//流量计算//
int init_flow(void);
void flow_caculation(int yuliangzengliang);
void flow_poll(void);
/*----------------------------------------------------------------------------------
                数据清0
-----------------------------------------------------------------------------------*/
bool data_memory_clear(void)
{
    bool result = true;
    extern bool bao_clear(void);
    INFO("[初始化固态存储数据]");
    //上报数据清0
    result = bao_clear();
    // import数据清0
    DB<TbImportant> dblog(DBName, TableNameImportant);
    if (dblog.delall() == false)
        result = false;
    //日志清0

    // devdata 清0
    devdata.clearAll();

    // A 清0
    A.clearAll();

    return result;
}
bool data_set_factory(void)
{
    // 0.原配置备份
    mk_all_dir((char *)"/app-cjq/bak");
    INFO("%s", get_cmd_results("cp  -f /app-cjq/setting/rtu_setting.json /app-cjq/bak/").c_str());

    rtu.load(RTU_SETTING_DEFAULT);
    param_recovery();
    runtime_recovery();
    rtu.dirty = true;
    param_save(&rtu);
    runtime_save(&rtu);
    rtu.save();
    //可能要延时重启一下才可能生效
    yanshichongqi = true;
    return true;
}
/*----------------------------------------------------------------------------------
                读取 report 中 特定时间的日志
-----------------------------------------------------------------------------------*/
int devdata_db_read(string tablename, string q, vector<TbBase> &msgs)
{
    DB<TbBase> dlog(DBName, tablename);
    return dlog.find(q, msgs);
}
int devdata_db_read(string tablename, time_t t, vector<TbBase> &msgs)
{
    string s = "date=\"" + time2string(t) + "\"";
    return devdata_db_read(tablename, s, msgs);
}
int devdata_db_read(string tablename, time_t t, DataClass *data)
{
    vector<TbBase> msgs;
    int result = devdata_db_read(tablename, t, msgs);
    if (result > 0 && msgs.size() > 0)
    {
        TbBase msg = msgs[0];
        data->fromlog(msg.data);
        data->now = string2time(msg.date);
        INFO("读取记录为: %s  数据为 :%s", msg.date.c_str(), devdata.tolog().c_str());
        INFO("数据内容为:\r\n%s", data->toString().c_str());
        return 1;
    }
    return 0;
}
// from <= date < t
int devdata_db_read(string tablename, time_t from, time_t to, vector<TbBase> &msgs)
{
    string s = "date>=\"" + time2string(from) + "\"" + "and date<\"" + time2string(to) + "\"";
    return devdata_db_read(tablename, s, msgs);
}

int devdata_db_save(string tablename, string code, time_t t, DataClass *pdata)
{
    DB<TbBase> dlog(DBName, tablename);
    char outbuffer[2048];
    memset(outbuffer, 0, sizeof(outbuffer));
    TbBase h;
    h.code = code;
    h.date = time2string(t, "%F %T");
    pdata->tolog(outbuffer, sizeof(outbuffer));
    if (strlen(outbuffer) > 0)
    {
        h.data = string(outbuffer);
    }
    INFO("table=%s 时间=%s 数据= \r\n%s", tablename.c_str(), h.date.c_str(), pdata->toString().c_str());
    dlog.update(string("date=\"") + h.date + "\"", &h);
    return 0;
}

int devdata_db_save(string tablename, time_t t, DataClass *pdata)
{
    return devdata_db_save(tablename, "", t, pdata);
}

/*----------------------------------------------------------------------------------
                recovery
-----------------------------------------------------------------------------------*/
int devdata_recovery()
{
    DB<TbBase> dlog(DBName, TableNameHour);
    vector<TbBase> msgs;
    dlog.find(string("select * from hour order by date desc limit 1;"), msgs);
    if (msgs.size() > 0)
    {
        TbBase msg;
        msg = msgs[0];
        devdata.fromlog((char *)msg.data.c_str());
        devdata.now = string2time(msg.date);
        INFO("devdata 恢复日期为: %s  数据为 :%s", msg.date.c_str(), devdata.tolog().c_str());
        INFO("数据内容为:\r\n%s", devdata.toString().c_str());
        return 1;
    }
    else
    {
        ERROR("devdaata 无法读取数据，清零");
        devdata.now = timestd(0);
        return 0;
    }
    return 0;
}

int param_recovery()
{
    char buf[50];
    char *stopString = NULL;
    param[0x02].val = std::strtoull(rtu.param.terminalNo.c_str(), &stopString, 16);
    param[0x03].val = std::strtoull(rtu.param.passwd.c_str(), &stopString, 16);
    for (int i = 0; i < 4 && i < rtu.param.center.size(); i++)
    {
        param[0x01].hex[i] = (u8)rtu.param.center[i];
    }
    for (int i = 0; i < 8 && i < rtu.param.remote.size(); i++)
    {
        char buffer[40];
        DataClassItem *it = &param[0x04 + i];
        ipstring2hex((char *)rtu.param.remote[i].c_str(), buffer);
        memcpy(it->hex, buffer, 20);
        PRINTBYTES("param.%02X", it->hex, 20, it->config.id);
    }

    if (rtu.param.model.compare("normal") == 0)
        param[0x0c].hex[0] = 0x02;

    param["terminalType"].val = rtu.param.terminalType;

    param["protocolVersion"].val = rtu.device.gz.version;
    param["rtuSN"].val = rtu.device.gz.sn;

    INFO("param 读入的配置为 :%s", param.tolog().c_str());
    INFO("数据内容为:\r\n%s", param.toString().c_str());
    return 0;
}

int runtime_recovery()
{
    map<string, l64>::iterator it;
    for (it = rtu.param.runtime.begin(); it != rtu.param.runtime.end(); it++)
        runtime[it->first].val = it->second;
    INFO("runtime 读入的配置为 :%s", runtime.tolog().c_str());
    INFO("数据内容为:\r\n%s", runtime.toString().c_str());
    return 0;
}

int personalset_recovery()
{
    return 0;
}

// 开机恢复
int startup_recovery()
{
    // rtu
    rtu.load();

    devdata_recovery();
    param_recovery();
    runtime_recovery();
    personalset_recovery();

    init_rain();
    init_waterlevel();
    init_flow();
    init_appdata();

    return 0;
}

/*----------------------------------------------------------------------------------
                save
-----------------------------------------------------------------------------------*/
int devdata_save(time_t t)
{
    return devdata_db_save(TableNameHour, (t - (t % 3600)), &devdata);
}
int devdata_save()
{
    return devdata_save(timestd(0));
}
int param_save(rtu_setting *pr)
{
    char buf[50];
    char *stopString = NULL;
    // 0x02
    if (std::strtoull(pr->param.terminalNo.c_str(), &stopString, 16) != param[0x02].val)
    {
        pr->param.terminalNo = std::to_string(param[0x02].val);
        pr->dirty = true;
    }
    // 0x03
    if (param[0x03].val != std::strtoull(pr->param.passwd.c_str(), &stopString, 16))
    {
        pr->param.passwd = tostring16(param[0x03].val);
        pr->dirty = true;
    }

    // center 0x01
    u8 cc[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    for (int i = 0; i < pr->param.center.size(); i++)
        cc[i] = pr->param.center[i];
    if (memcmp(cc, param[0x01].hex, 4) != 0)
    {
        pr->dirty = true;
        pr->param.center.clear();
        pr->param.center.push_back(param[0x01].hex[0]);
        pr->param.center.push_back(param[0x01].hex[1]);
        pr->param.center.push_back(param[0x01].hex[2]);
        pr->param.center.push_back(param[0x01].hex[3]);
    }

    // ip address
    char ipbuffer[50];
    memset(ipbuffer, 0, sizeof(ipbuffer));

    for (int i = 0; i < 8; i++)
    {
        DataClassItem *it = &param[0x04 + i];
        if (it->hex[0] == 0xFF)
            break;
        hex2ipstring((char *)it->hex, ipbuffer);
        if (((i + 1) > pr->param.remote.size()) ||
            (pr->param.remote[i].compare(string(ipbuffer)) != 0))
        {
            pr->dirty = true;
            pr->param.remote[i] = string(ipbuffer);
        }
    }

    // model
    if (param[0x0c].hex[0] == 0x02)
    {
        pr->param.model = ("normal");
    }
    rtu.param.terminalType = param["terminalType"].val;

    // version.sn
    if (param[0xFFA1].val != pr->device.gz.version)
    {
        pr->dirty = true;
        pr->device.gz.version = param[0xFFA1].val;
    }

    if (param[0xFF9F].val != pr->device.gz.sn)
    {
        pr->dirty = true;
        pr->device.gz.sn = param[0xFF9F].val;
    }

    // output test
    {
        char buffer[1024];
        param.tolog(buffer, sizeof(buffer));
        INFO("all setting in param=%s", buffer);
        INFO("%s", param.toString().c_str());
    }
    return 0;
}

int runtime_save(rtu_setting *pr)
{
    map<string, l64> *rt = &pr->param.runtime;
    for (int i = 0; i < runtime.size(); i++)
    {
        DataClassItem p = runtime(i);
        if (p.isZero() == false)
        {
            if ((rt->count(p.config.name) == 0) ||
                ((*rt)[p.config.name] != p.val))
            {
                (*rt)[p.config.name] = p.val;
                pr->dirty = true;
            }
        }
    }
    return 0;
}

int personalset_save(rtu_setting *pr)
{
    return 0;
}

int data_save_if_need(void)
{
    // param_save(&rtu);
    // runtime_save(&rtu);
    // personalset_save(&rtu);

    if (rtu.dirty)
    {
        rtu.save();
        rtu.dirty = false;
    }
    return 0;
}
//关机保存
int powerdown_save()
{
    rainacc_save();
    devdata_save();
    data_save_if_need();
    return 0;
}

/*----------------------------------------------------------------------------------
                data process
-----------------------------------------------------------------------------------*/
/*=============================升级 ===========================*/
#define SIG_UPDATA SIGUSR1

//升级信号退出
static bool need_updata = false;
void stop_to_updata(int sig)
{
    INFO("[updata] get signal=%d.\n", sig);
    need_updata = true;
}
void init_stop_to_updata(void)
{
    signal(SIG_UPDATA, stop_to_updata);
}

/*=============================数据处理===========================*/
void *data_process_thread(void *arg)
{
    time_t tnow = 0;
    time_t last = 0;

    INFO("[data_process_thread] is running.");
    init_stop_to_updata();
    timestd(&tnow);
    last = tnow - tnow % LOG_TIME_INTERVAL; // 5min

    //初始化
    startup_recovery();

    // 6.按采集器类型
    while (1)
    {
        //每5min冻结数据
        timestd(&tnow);
        if (last != (tnow - tnow % LOG_TIME_INTERVAL))
        {
            char *stringbuffer = NULL;
            last = (tnow - tnow % LOG_TIME_INTERVAL);
            devdata_save();
        }

        //数据检测
        waterlevel_poll();
        flow_poll();
        rain_poll();

        usleep(200000);
        data_save_if_need();

        //升级时，保存数据
        if (need_updata)
        {
            char *stringbuffer = NULL;
            last = tnow;
            powerdown_save();
            INFO("[need_updata] save data and quit the app.");
            drop_zlog();
            exit(0);
        }

        //延时重启
        if (yanshichongqi)
        {
            INFO("延时5秒后,重启.....");
            sleep(5); //延时5秒
            rainacc_save();
            devdata_save();
            drop_zlog();
            exit(0);
        }
    }
}

// init
pthread_t thread_data_init(void)
{
    pthread_t id;
    if (pthread_create(&id, NULL, data_process_thread, NULL) != 0)
    {
        ERROR("[线程创建]:ERR = data_process_thread");
        return 0;
    }
    INFO("[线程创建]:OK = data_process_thread");
    return id;
}

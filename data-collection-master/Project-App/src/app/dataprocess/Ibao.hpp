/*
    统一上报流程
    包括:
        小时，定时,加报，补报
*/
#include "DataClass.hpp"
#pragma once
#include <string>
#include "data_save.h"
#include "lib.h"
#include <vector>
#include "ZhengWen.hpp"
using namespace std;
class Ibao
{
protected:
    time_t last_record_check_time = std::time(0); //用于检测超时,保存record检测时的最的时间
    time_t last_report_check_time = std::time(0);

public:
    string name;
    //保存并记录
    virtual bool record_check() = 0;

    //记录
    virtual bool record() = 0;

    //上报检测
    virtual bool report_check(bool reset) = 0;

    //上报记录数据
    virtual int report_data(char *dataout) = 0;
    void *report_data() { return NULL; }

    //失败补报
    virtual bool report_fail() = 0;
    virtual bool report_ok() = 0;

    // run 1 second
    virtual bool poll() = 0;

    // cmd
    virtual UPCMD Cmd() = 0;

    // clear all
    virtual bool clearAll() = 0;
};

/*参数*/
struct BaoBaseParam
{
    int cmd;
    DataClass *data;
    string save_table_name;
    string triglename;
    string warnname;
};

/*==========================流程控制========================
BaoBase: 周期性上报
1.检测是否要保存记录
2.保存记录
3.检测是否要上报数据BaoBase
4.读取上报数据
5.上报失败，转记录到补报
===========================================================*/
/*无 补报的定时报*/
class BaoBase : public Ibao
{
protected:
    //记录相关变量
    string code = "";
    time_t record_time;
    time_t obtime;
    string record_table;

    //被记录的数据集
    DataClass *pdata = NULL;
    vector<string> ids;
    int record_seconds = 0;
    int record_delays = 0;
    bool record_enable = false;

    //上报想关的数据
    int report_delays = 0;
    bool report_enable = false;
    string report_log = "";
    UPCMD report_cmd;

    //条件触发
    string triglename = "";

protected:
    //补报
    bool bu_add_one_record()
    {
        stringstream ss;
        TbBu msg;
        msg.name = name;
        msg.date = time2string(std::time(0));
        msg.ids = list_merge(this->ids, " ");
        msg.tb = record_table;
        msg.tdate = time2string(record_time);
        msg.cmd = std::to_string(report_cmd);
        msg.valid = 0x01;
        DB<TbBu> db(DBName, TableNameBu);
        ss.clear();
        ss << "name=\"" << name << "\" and cmd=\"" << msg.cmd << "\" and tdate=\"" << msg.tdate << "\"";
        db.update(ss.str(), &msg);
        INFO("添加补报:%s", msg.toString().c_str());
        return false;
    }

    bool bu_invalid_one_record(TbBu msg)
    {
        msg.valid = 0;
        stringstream ss;
        ss << "name=\"" << msg.name << "\" and cmd=\"" << msg.cmd << "\" and tdate=\"" << msg.tdate << "\"";
        DB<TbBu> db(DBName, TableNameBu);
        INFO("使无效补报:%s", msg.toString().c_str());
        return (db.update(ss.str(), &msg) == 0);
    }

    //时间检测(可以在不连续的时间内检测，不必每秒都要检测，因为上报数据时，会阻塞好几秒)
    //(n*cycle - delays) 在 (last,now] 之间，返回true
    //   last < (n*cycle+delays) <= now
    //   (last-delays) < n*cycle <= now-delays
    //   (last-delays)/cycle < n <= (now-delays)/cycle > (last-delays+cycle)/cycle >=(n+1)
    bool Check_Time(time_t *last, time_t now, int cycle, int delays)
    {
        int nn = 0;
        if (cycle == 0)
            return false; // 0时，不支持定时检测
        nn = (*last - delays) / cycle;
        if (((now - delays) / cycle) >= (nn + 1))
        {
            *last = now;
            return true;
        }
        return false;
    }

public:
    bool clearAll()
    {
        if (record_table != "" && record_table.size() > 0)
        {
            DB<TbBu> db(DBName, record_table);
            INFO("[清空表格] %s", record_table.c_str());
            return db.delall();
        }
        return true;
    }

public:
    BaoBase(string _n, BaoBaseParam p)
    {
        name = _n;
        pdata = p.data;
        report_cmd = (UPCMD)p.cmd;
        record_table = p.save_table_name;
        triglename = p.triglename;
    }
    ~BaoBase() {}
    UPCMD Cmd() { return report_cmd; }
    //------------------------------------------------------
    bool record_check()
    {
        if (record_seconds < 60)
            return false;
        // if (record_enable == false && Cycle_Check(std::time(0), record_seconds, record_delays))
        if (record_enable == false && Check_Time(&this->last_record_check_time, std::time(0), record_seconds, record_delays))
        {
            INFO("[上报检测]  %s ,周期=%d,记录时间:%d,上报时间:%d", this->name.c_str(), record_seconds, record_delays, report_delays);
            return true;
        }
        return false;
    }
    //保存并记录
    bool record()
    {
        time_t t = std::time(0);
        t -= t % record_seconds;
        record_time = t;
        obtime = std::time(0);
        INFO("[%s 保存后上报]: 时间=%s  cycle = %d S ", this->name.c_str(), time2string(record_time).c_str(), record_seconds);
        devdata_db_save(record_table, code, record_time, pdata);
        report_log = pdata->tolog();
        return true;
    }

    //上报检测
    bool report_check(bool reset)
    {
        if (reset)
            report_enable = false;
        return report_enable;
    }

    //上报记录数据
    int report_data(char *dataout)
    {
        DataClass log(pdata->type, ("上报:" + record_table).c_str());
        log.fromlog(report_log);
        ZhengWen<string> z;
        z.init(ids, &log, obtime);
        z.header.observetime = record_time;
        return z.to_array((u8 *)dataout);
    }

    //失败补报
    bool report_fail()
    {
        INFO("[无需补报], name=%s", name.c_str());
        return true;
    }
    bool report_ok()
    {
        INFO("[上报成功], name=%s", name.c_str());
        return true;
    }

    // run 1 second
    bool poll()
    {
        if (record_check())
        {
            record();
            record_enable = true;
        }
        if (record_seconds < 60)
        {
            report_enable = false;
        }
        /// if (record_enable && Cycle_Check(std::time(0), record_seconds, (report_delays)))
        if (record_enable && Check_Time(&this->last_report_check_time, std::time(0), record_seconds, (report_delays)))
        {
            record_enable = false;
            report_enable = true;
        }
        return true;
    }

    //------------------------------------------------------
    //设置数据与时间
    void set_interval(int _cycle_seconds, int _record_delays, int _report_delays)
    {
        record_seconds = _cycle_seconds;
        record_delays = _record_delays;
        report_delays = _report_delays;
        INFO("[报文时间间隔] %s ,周期=%d,记录时间:%d,上报时间:%d", this->name.c_str(), _cycle_seconds, record_delays, _report_delays);
    }
    void set_report_ids(vector<string> _ids)
    {
        ids = _ids;
    }
};

/*有 补报定时报*/
class CycleBaoBase : public BaoBase
{

public:
    CycleBaoBase(string _n, BaoBaseParam p) : BaoBase(_n, p) {}
    ~CycleBaoBase() {}

    //失败补报
    bool report_fail() override
    {
        return this->bu_add_one_record();
    }
};

/*加报*/
class JiaBaoBase : public CycleBaoBase
{
public:
    JiaBaoBase(string _n, BaoBaseParam p) : CycleBaoBase(_n, p) { record_table = "jia"; }
    ~JiaBaoBase() {}

    /*检测各模块的加报标志，并保存*/
    bool record_check() override
    {
        if (record_seconds < 60)
            return false;
        // if (record_enable == false && Cycle_Check(std::time(0), record_seconds, record_delays))
        if (record_enable == false && Check_Time(&this->last_record_check_time, std::time(0), record_seconds, record_delays))
        {
            INFO("[上报检测]  %s ,周期=%d,记录时间:%d,上报时间:%d", this->name.c_str(), record_seconds, record_delays, report_delays);
            /*检测标志*/
            if (A[triglename].list.size() > 0)
            {
                code = list_merge(A[triglename].list, " ");
                return true;
            }
            /*配置code值*/
        }
        return false;
    }
};

//直接上报数据
class RawBao : public BaoBase
{
    int (*get_report_data)(u8 *) = nullptr;

public:
    RawBao(string _n, int (*get_data)(u8 *)) : BaoBase(_n, {})
    {
        get_report_data = get_data;
        record_table = "";
    }
    ~RawBao() {}
    int report_data(char *dataout) override
    {
        char bufferstring[200];
        char buffer[100];
        memset(buffer, 0, sizeof(buffer));
        memset(bufferstring, 0, sizeof(bufferstring));
        if (get_report_data == nullptr)
            return 0;
        int size = get_report_data((u8 *)buffer);
        hexarray2str(buffer, size, bufferstring, sizeof(bufferstring));
        INFO("%s 上报数据为:%s", name.c_str(), bufferstring);
        memcpy(dataout, buffer, size);
        return size;
    }
    bool record() override
    {
        return true;
    }
};

/*补报==>读取补报列表，上报数据*/
class BuBaoBase : public BaoBase
{
public:
    BuBaoBase(string _n) : BaoBase(_n, {})
    {
        record_table = TableNameBu;
        db_bu = new DB<TbBu>(DBName, TableNameBu);
        pdata = new DataClass(DataClassConfigType::Type_Dev, name);
    }
    ~BuBaoBase()
    {
        delete db_bu;
    }

    DB<TbBu> *db_bu;
    vector<TbBu> msgs;
    TbBu m;
    string report_tb;
    string report_date;

    /*检测数据库，是否有数据要上报*/
    bool record_check() override
    {
        if (record_seconds < 60)
            return false;
        // if (record_enable == false && Cycle_Check(std::time(0), record_seconds, record_delays))
        if (record_enable == false && Check_Time(&this->last_record_check_time, std::time(0), record_seconds, record_delays))
        {
            INFO("[上报检测]  %s ,周期=%d,记录时间:%d,上报时间:%d", this->name.c_str(), record_seconds, record_delays, report_delays);
            //倒序,一条一条来报
            msgs.clear();
            db_bu->find("valid!=0 LIMIT 1", msgs);

            if (msgs.size() > 0)
            {
                char *stopString;
                m = msgs[0];
                //设置数据
                ids = split(m.ids, " ");
                report_cmd = (UPCMD)std::strtol(m.cmd.c_str(), &stopString, 10);
                report_tb = m.tb;
                report_date = m.tdate;
                record_enable = true;
                report_log = "";
                INFO("找到一条补报:%s", m.toString().c_str());
                return true;
            }
        }
        return false;
    }

    bool report_ok() override
    {
        BaoBase::report_ok();
        INFO("[补报成功],在报表中，无效本条.");
        bu_invalid_one_record(m);
        return true;
    }

    int report_data(char *dataout) override
    {
        //查找对应表数据
        report_log.clear();
        if (report_log.empty())
        {
            vector<TbBase> _msgs;
            string q = "date=\"" + report_date + "\"";
            devdata_db_read(report_tb, q, _msgs);
            if (_msgs.size() > 0)
            {
                report_log = _msgs[0].data;
                obtime = string2time(_msgs[0].date); //观测时间
            }
            else
            {
                ERROR("补报无法查找原表格数据,原表格=%s, 时间=%s", report_tb.c_str(), report_date.c_str());
                record_enable = false;
                bu_invalid_one_record(m);
                return 0;
            }
        }

        if (!report_log.empty())
        {
            record_time = obtime;   //观测时间，就是record时间
            return BaoBase::report_data(dataout);
        }

        return 0;
    }
    bool record() override
    {
        return true;
    }
};

//图片报，用于拍照，上传数据图片，结合 MPImageBase 使用，实现分包上报
#include <map>
class BaoImageBase : public Ibao
{

private:
    std::vector<std::string> fileList;
    std::vector<std::string> nameList;
    map<std::string, time_t> TimeoutfileList;
    string _info = "";

public:
    BaoImageBase()
    {
        name = "图片报";
    }
    ~BaoImageBase() {}

    bool record_check()
    {
        return false;
    }

    bool record()
    {
        return true;
    }

    //上报检测
    bool report_check(bool reset)
    {
        if (fileList.size() == 0)
        {
            string dir = rtu.device.image.dir;
            if (dir.size() > 0)
            {
                if (dir[dir.size() - 1] != '/')
                {
                    dir = dir + "/";
                }
                int cnt = findfileinfolder(dir.c_str(), "jpg", fileList, nameList);
                if (cnt != 0)
                {
                    _info = "";
                    //INFO("图片报，当前图片个数:%d", cnt);
                    return true;
                }
                else
                {
                    if (_info == "")
                    {
                        _info = "Has no images";
                        INFO("当前目录无图片上报:%s", dir.c_str());
                    }
                }
            }
            return false;
        }
        else
            return true;
    }

    //上报记录数据
    int report_data(char *dataout) { return 0; }
    void *report_data()
    {
        // 1. 当前文件如果在 失败列表，就要等待超时才可以上报
        bool get = false;
        string s;
        while (fileList.size() > 0)
        {
            s = fileList.back();

            if(rtu.device.image.retry_delays==0 || TimeoutfileList.size() == 0)
            {
                return (void*)fileList.back().c_str();
            }
            else
            {
                map<string, time_t>::iterator iter;
                iter = TimeoutfileList.find(s);
                if (iter == TimeoutfileList.end())
                {
                    return (void*)fileList.back().c_str();
                }
                else
                {
                    time_t lasttime = (time_t)iter->second;
                    time_t delays = rtu.device.image.retry_delays;
                    // 10分钟
                    if (delays < 30)
                        delays = 600;
                    if (abs(std::time(0) - lasttime) > delays)
                    {
                        INFO("时间到: timeout=%d, 重传文件:%s", (int)delays, s.c_str());
                        return (void*)fileList.back().c_str();
                    }
                    else
                    {
                        fileList.pop_back();
                        continue;
                    }
                }
            }
        }
        return NULL;
    }

    //失败补报
    bool report_fail()
    {
        if (fileList.size() > 0)
        {
            if (rtu.device.image.retry_delays == 0)
            {
                return report_ok();
            }
            else
            {
                //删除文件
                string str = fileList.back();
                INFO("上报失败，文件等待重新上报:%s", str.c_str());
                TimeoutfileList[str] = std::time(0);
                fileList.pop_back();
            }
        }
    }
    bool report_ok()
    {
        if (fileList.size() > 0)
        {
            //删除文件
            const char *file = fileList.back().c_str();
            TimeoutfileList.erase(fileList.back());
            remove(file);
            fileList.pop_back();
        }

        return true;
    }

    // run 1 second
    bool poll()
    {
        return false;
    }

    // cmd
    UPCMD Cmd()
    {
        return UPCMD_36H_Image;
    }

    // clear all
    bool clearAll()
    {
        //清空图片记录与缓存
        return false;
    }
};

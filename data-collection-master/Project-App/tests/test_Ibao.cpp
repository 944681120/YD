#include "gtest/gtest.h"
#include "rtu_setting.hpp"
#include "DataClass.hpp"
#include "data_save.h"
#include "app.h"
#include "sendmode.h"
#include "Ibao.hpp"

extern void flow_caculation(l64 wl, DataClassItemConfig *pconfig, bool result, bool clear);
extern void flow_poll(void);

extern int get_sendmode(SendMode **sm);
extern void thread_up_poll(void);
extern int get_lianlu_report(u8 *dataout);

//模拟雨量运行
TEST(TESTCASE, Ibao)
{
    startup_recovery();
    BaoBase dingshibao("定时报", {.cmd = (int)UPCMD_32H_DingshiBao,
                                  .data = &devdata,
                                  .save_table_name = TableNameDingshi});
    CycleBaoBase xiaoshibao("小时报", {.cmd = (int)UPCMD_34H_XiaoshiBao,
                                       .data = &devdata,
                                       .save_table_name = TableNameHour});
    JiaBaoBase rainjiabao("雨量加报", {.cmd = (int)UPCMD_33H_JiabaoBao,
                                       .data = &devdata,
                                       .save_table_name = TableNameJia,
                                       .triglename = "jia",
                                       .warnname = "rain"});
    JiaBaoBase waterleveljiabao("水位加报", {.cmd = (int)UPCMD_33H_JiabaoBao,
                                             .data = &devdata,
                                             .save_table_name = TableNameJia,
                                             .triglename = "jia",
                                             .warnname = "waterlevel"});

    BuBaoBase bubao("补报");
    RawBao lianlubao("链路报", get_lianlu_report);

    dingshibao.set_interval(60, 1, 6);
    xiaoshibao.set_interval(60, 2, 7);
    rainjiabao.set_interval(60, 3, 8);
    waterleveljiabao.set_interval(60, 4, 9);
    bubao.set_interval(60, -2, 2);
    lianlubao.set_interval(60, -3, -1);

    dingshibao.set_report_ids(rtu.dingshi);
    xiaoshibao.set_report_ids(rtu.xiaoshi);
    rainjiabao.set_report_ids(rtu.jiabao["rain"]);
    waterleveljiabao.set_report_ids(rtu.jiabao["waterlevel"]);

    vector<Ibao *> baos = {
        &dingshibao, &xiaoshibao,
        &rainjiabao,
        &waterleveljiabao,
        &bubao,
        &lianlubao};

    A["jia"].list.push_back("rain");
    A["jia"].list.push_back("waterlevel");
    INFO("%s", A.tolog().c_str());

    // DataClass B(DataClassConfigType::Type_AppData, "B");
    // B.fromlog(A.tolog());
    // INFO("%s", B.tolog().c_str());

    //开机数据恢复
    time_t timemax = 2;

    INFO("\r\n%s", devdata.toString().c_str());

    // tcp
    SendMode *sm = &tcp_sendmode;
    //get_sendmode(&sm);

    //设置日
    // TimeSet(2022, 6, 11, 7, 59, 55);
    time_t tn = std::time(0);
    time_t last = tn - tn % LOG_TIME_INTERVAL; // 5min
    time_t tnow = tn - tn % LOG_TIME_INTERVAL;

    // config
    DataClassItemConfig *pconf1 = &devdata["Q1"].config;
    DataClassItemConfig *pconf2 = &devdata["Q2"].config;
    DataClassItemConfig *pconfqc1 = &devdata["QC1"].config;

    // simulate val
    int flow1 = 123;
    int flow2 = 222;
    int flowQC1 = 56;
    while (timemax > 0)
    {
        //水位传感器返回,计算
        // Z1
        flow1++;
        flow2 += 2;
        flowQC1 += 9;
        flow_caculation(flow1, pconf1, true, false);     //数据正确
        flow_caculation(flow2, pconf2, true, false);     //数据正确
        flow_caculation(flowQC1, pconfqc1, true, false); //数据正确

        // flow_caculation(0,pconf,false,true); //3次失败

        timemax -= 1;
        tn += 1;
        // TimeSet(time2string(tn));

        // poll
        flow_poll();
        // thread_up_poll();

        for (int i = 0; i < baos.size(); i++)
        {
            char buffer[1000];
            char bufferstring[1000];
            memset(buffer, 0, sizeof(buffer));
            memset(bufferstring, 0, sizeof(bufferstring));
            baos[i]->poll();
            if (baos[i]->report_check(false))
            {
                INFO("%s ===>正在上报.....", baos[i]->name.c_str());
                ERROR("===上报失败");
                int rs = baos[i]->report_data(buffer);
                hexarray2str(buffer, rs, bufferstring, sizeof(bufferstring));
                INFO("上报数据为:%s", bufferstring);
                sm->init(M1, get_current_remote(), UPCMD_2FH_LianluWeichiBao, (u8 *)buffer, rs, rs);
                if (sm->wait(2000, NULL, NULL) != 0)
                {
                    ERROR("上报失败,转到补报");
                    baos[i]->report_fail();
                }
                else
                {
                    baos[i]->report_ok();
                }
                // baos[i]->report_fail();
                baos[i]->report_ok();
                baos[i]->report_check(true);
            }
        }
        usleep(500000);
    }
    // get_cmd_results("ntpdate cn.pool.ntp.org", NULL, 0);
}
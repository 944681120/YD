// #include "gtest/gtest.h"
// #include "rtu_setting.hpp"
// #include "DataClass.hpp"
// #include "data_save.h"
// #include "app.h"
// #include "sendmode.h"

// extern void flow_caculation(l64 wl, DataClassItemConfig *pconfig, bool result, bool clear);
// extern void flow_poll(void);

// extern int get_sendmode(SendMode **sm);
// extern void thread_up_poll(void);

// //模拟雨量运行
// TEST(TESTCASE, hour)
// {
//     //开机数据恢复
//     time_t timemax = 2*3600;
//     startup_recovery();

//     INFO("\r\n%s", devdata.toString().c_str());

//     //tcp
//     SendMode* sm = NULL;
//     get_sendmode(&sm);

//     //设置日期
//     string tn_str = "2022-6-10 7:30:59";
//     //TimeSet(2022, 6, 10, 7, 30, 59);
//     time_t tn = std::time(0);
//     time_t last = tn - tn % LOG_TIME_INTERVAL; // 5min
//     time_t tnow = tn - tn % LOG_TIME_INTERVAL;

//     // config
//     DataClassItemConfig *pconf1 = &devdata["Q1"].config;
//     DataClassItemConfig *pconf2 = &devdata["Q2"].config;
//     DataClassItemConfig *pconfqc1 = &devdata["QC1"].config;

//     // simulate val
//     int flow1 = 123;
//     int flow2 = 222;
//     int flowQC1 = 56;
//     while (timemax > 0)
//     {
//         //水位传感器返回,计算
//         // Z1
//         flow1++;
//         flow2 += 2;
//         flowQC1 += 9;
//         flow_caculation(flow1, pconf1, true, false);     //数据正确
//         flow_caculation(flow2, pconf2, true, false);     //数据正确
//         flow_caculation(flowQC1, pconfqc1, true, false); //数据正确

//         // flow_caculation(0,pconf,false,true); //3次失败

//         timemax -= 1;
//         tn += 1;
//         //TimeSet(time2string(tn));

//         // 5minv保存数据
//         tnow = tn - tn % LOG_TIME_INTERVAL;
//         if (last != tnow)
//         {
//             devdata_save();
//         }

//         // poll
//         flow_poll();
//         thread_up_poll();

//         //等待 1min 上报小时
//         sleep(1);
//         {
//             static time_t xt =  std::time(0) + 15;
//             if(xt< std::time(0))
//             {
//                 extern void set_observetime_time(time_t t);
//                 set_observetime_time(std::time(0)-std::time(0)%3600);
//                 xt = std::time(0) + 120;
//                 devdata_save();
//                 extern bool test_xiaoshi_enable ;
//                 test_xiaoshi_enable = true;
//             }
//         }
//     }

//     get_cmd_results("ntpdate cn.pool.ntp.org", NULL, 0);
// }
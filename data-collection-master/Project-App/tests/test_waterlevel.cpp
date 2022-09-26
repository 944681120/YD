// #include "gtest/gtest.h"
// #include "rtu_setting.hpp"
// #include "DataClass.hpp"
// #include "data_save.h"
// #include "app.h"

// extern void waterlevel_caculation(l64 wl, DataClassItemConfig * pconfig,bool result,bool clear);
// extern void waterlevel_poll(void);

// //模拟雨量运行 
// TEST(TESTCASE, waterlevel_run)
// {
//     time_t timemax = 10 * 3600;
//     startup_recovery();

//     //设置日期
//     string tn_str = "2022-6-10 7:30:59";
//     TimeSet(2022, 6, 10, 7, 30, 59);
//     time_t tn = std::time(0);
//     time_t last = tn - tn % LOG_TIME_INTERVAL; // 5min
//     time_t tnow = tn - tn % LOG_TIME_INTERVAL;

//     //config
//     DataClassItemConfig *pconf1 = &devdata["Z1"].config;
//     DataClassItemConfig *pconf2 = &devdata["Z2"].config;

//     //simulate val
//     int waterlevel1 = 123;
//     int waterlevel2 = 222;
//     while( timemax >0 )
//     {
//         //水位传感器返回,计算
//         //Z1 
//         waterlevel1 ++;
//         waterlevel2 +=2;
//         waterlevel_caculation(waterlevel1,pconf1,true,false); //数据正确
//         waterlevel_caculation(waterlevel2,pconf2,true,false); //数据正确

//         //waterlevel_caculation(0,pconf,false,true); //3次失败

//         timemax -=100;
//         tn += 100;
//         TimeSet(time2string(tn));

//         //5minv保存数据
//         tnow = tn - tn % LOG_TIME_INTERVAL;
//         if(last != tnow)
//         {
//             devdata_save();
//         }        
        
//         //poll
//         waterlevel_poll();

//     }

//     get_cmd_results("ntpdate cn.pool.ntp.org",NULL,0);

// }
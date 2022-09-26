// #include "gtest/gtest.h"
// #include "rtu_setting.hpp"
// #include "DataClass.hpp"
// #include "data_save.h"
// #include "app.h"

// extern void rain_caculation(int yuliangzengliang);
// extern void rain_poll(void);

// //模拟雨量运行
// TEST(TESTCASE, rain_run)
// {
//     time_t timemax = 2 * 3600;
//     startup_recovery();

    

//     //设置日期
//     string tn_str = "2022-6-10 7:30:59";
//     TimeSet(2022, 6, 10, 7, 30, 59);
//     time_t tn = std::time(0);
//     time_t last = tn - tn % LOG_TIME_INTERVAL; // 5min
//     time_t tnow = tn - tn % LOG_TIME_INTERVAL;
//     INFO("%s",time2string(std::time(0)).c_str());

//     while (timemax > 0)
//     {
//         //脉冲
//         rain_caculation(8);

//         timemax -= 60;
//         tn += 60;
//         TimeSet(time2string(tn));

//         // 5minv保存数据
//         tnow = tn - tn % LOG_TIME_INTERVAL;
//         if (last != tnow)
//         {
//             devdata_save();
//             rainacc_save();
//         }

//         // poll
//         rain_poll();
//     }

//     get_cmd_results("ntpdate cn.pool.ntp.org", NULL, 0);
// }
// #include "gtest/gtest.h"
// #include "rtu_setting.hpp"
// #include "DataClass.hpp"
// #include "data_save.h"

// void insert_hour(void)
// {
//     extern int devdata_save(time_t t);
//     time_t t = std::time(0) - 20*3600;
//     for(int i =0;i<20;i++)
//     {
//         devdata["P1"].val = 1+i;
//         devdata_save(t);
//         t+=3600;
//     }
// }


// TEST(TESTCASE, rainfunction)
// {
//     extern bool rainacc_save(void);
//     extern bool rainacc_recovery(void);
//     startup_recovery();

//     rainacc_recovery();
//     if(A["rainAcc"].isZero())
//         A["rainAcc"].val = 0;
//     else A["rainAcc"].val +=2;
//     l64 old = A["rainAcc"].val;
//     rainacc_save();
//     rainacc_recovery();
//     l64 n = A["rainAcc"].val;

//     EXPECT_EQ(old,n);

//    // insert_hour();

//     // {
//     //     extern time_t GetRainDayStartTime(time_t tnow);
//     //     int RainDayStartTime = (int)runtime["RainDayStartTime"].val;
//     //     std::cout << "雨量日起始时间 = "<< std::to_string(RainDayStartTime)<<endl;
//     //     std::cout<< "当前时间  " << time2string(timestd(0)) <<endl;
//     //     time_t uptime = GetRainDayStartTime(timestd(0));
//     //     std::cout<< "日起始时间" << time2string(uptime) <<endl;

//     // }

//     //当前雨量计算
//     // {
//     //     extern void  init_currentRain(time_t t);
//     //     init_currentRain(timestd(0));
//     // }
//     //日降雨量计算
//     {
//         extern void  init_dayRain(time_t t);
//         init_dayRain(timestd(0));
//     }

// }

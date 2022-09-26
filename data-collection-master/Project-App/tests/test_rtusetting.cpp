// #include "gtest/gtest.h"
// #include "rtu_setting.hpp"
// #include "DataClass.hpp"
// #include "data_save.h"

// TEST(TESTCASE, rtu)
// {
//     char*stopString = NULL;
//     rtu_setting rr;

//     startup_recovery();

//     // //runtime
//     // runtime["H8B"].val = param[0x01].hex[0];
//     // runtime["H8B"].val+=2;
//     // runtime_save(&rtu);
//     // rr.load(RTU_SETTING_SAVE_FULLNAME);
//     // EXPECT_EQ(rr.runtime["H8B"],runtime["H8B"].val);

//     // //param
//     // param[0x01].hex[0]+=1;
//     // param[0x02].val+=1;
//     // param_save(&rtu);
//     // rtu.save();
//     // rr.load(RTU_SETTING_SAVE_FULLNAME);
//     // EXPECT_EQ(param[0x01].hex[0],rr.center[0]);
//     // EXPECT_EQ(param[0x02].val,std::strtoull(rr.terminalNo.c_str(), &stopString, 16));

//     // //devdata 
//     // string now = devdata.tolog();
//     // std::cout<<"now="<<now<<endl;   
//     // if(devdata["TP"].isZero())
//     //     devdata["TP"].val = 12345;
//     // else 
//     //     devdata["TP"].val++;
//     // string presave = devdata.tolog();
//     // devdata_save();
//     // devdata_recovery();
//     // string aftersave = devdata.tolog();
//     // std::cout<< "pre="<<presave<<"   after="<<aftersave<<endl;
//     // EXPECT_EQ(presave,aftersave);


//     {
//         extern bool data_set_factory(void);
//         data_set_factory();

//     }
// }

// #include "center.h"
// #include "report.h"
// #include "upacket.h"
// #include "mxdevice.h"
// #include "sendmode.h"
// #include "lib.h"
// using namespace std;

// #include "gtest/gtest.h"

// // PRINTBYTES("param.%02X", it->hex, 20, it->config.id);

// TEST(TESTCASE, packet)
// {
//     u8 d[1024];
//     UUPacket up;
//     //版本
//     int len = get_version(d);
//     int sendlen = upacket_encodehex(&up, true, get_current_remote(), UPCMD::UPCMD_45H_getVersion, d, len, STX_STX, ETX_ETX);
//     PRINTBYTES("packetlen=%d  cmd=%s", (u8 *)&up, sendlen, sendlen, cmd_to_string(up.cmd));

//     //读id
//     u8 ids[] = {0,0,0,0,0,0,0,0,   0x20, 0, 0x21, 0};
//     len = get_zhiding_ids(ids, sizeof(ids), d);
//     sendlen = upacket_encodehex(&up, true, get_current_remote(), UPCMD::UPCMD_3AH_zhidingids, d, len, STX_STX, ETX_ETX);
//     PRINTBYTES("packetlen=%d  cmd=%s", (u8 *)&up, sendlen, sendlen, cmd_to_string(up.cmd));

//     //读参数
//     u8 ids1[] = {0,0,0,0,0,0,0,0,   0x02, 0, 0x03, 0};
//     len = get_param(ids1, sizeof(ids1), d);
//     sendlen = upacket_encodehex(&up, true, get_current_remote(), UPCMD::UPCMD_41H_getparams, d, len, STX_STX, ETX_ETX);
//     PRINTBYTES("packetlen=%d  cmd=%s", (u8 *)&up, sendlen, sendlen, cmd_to_string(up.cmd));
//     //设置参数
//     string str = "00000000000000000228990000000103101239";
//     u8 setparambuffer[100];
//     int bsize = str2hexarray(str.c_str(),str.length(),(char*)setparambuffer,sizeof(setparambuffer));
//     len = set_param(setparambuffer, bsize, d);
//     sendlen = upacket_encodehex(&up, true, get_current_remote(), UPCMD::UPCMD_40H_setparams, d, len, STX_STX, ETX_ETX);
//     PRINTBYTES("packetlen=%d  cmd=%s", (u8 *)&up, sendlen, sendlen, cmd_to_string(up.cmd));
//     //修改密码

//     //运行参数
//     str = "0000000000000000282312345678";
//     memset(setparambuffer,0,sizeof(setparambuffer));
//     bsize = str2hexarray(str.c_str(),str.length(),(char*)setparambuffer,sizeof(setparambuffer));
//     len = set_runtime(setparambuffer, bsize, d);
//     sendlen = upacket_encodehex(&up, true, get_current_remote(), UPCMD::UPCMD_42H_setruntimes, d, len, STX_STX, ETX_ETX);
//     PRINTBYTES("packetlen=%d  cmd=%s", (u8 *)&up, sendlen, sendlen, cmd_to_string(up.cmd)); 

//     //所有数据
//     len = get_shishi_all(d);
//     sendlen = upacket_encodehex(&up, true, get_current_remote(), UPCMD::UPCMD_37H_shishi, d, len, STX_STX, ETX_ETX);
//     PRINTBYTES("packetlen=%d  cmd=%s", (u8 *)&up, sendlen, sendlen, cmd_to_string(up.cmd));

//     {
//         int data_save_if_need(void);
//         data_save_if_need(); 
//     }
// }
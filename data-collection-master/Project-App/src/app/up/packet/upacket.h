#ifndef __UPACKET_H__
#define __UPACKET_H__

#include "lib.h"


#define PDSIZE 4048

#pragma pack(1)

//上行
typedef struct 
{
   u16 start;
   u8 remote;
   u8 local[5];
   u16 passwd;
   u8  cmd;
   u16 len;
   u8 stx;
   u8 data[PDSIZE+20]; // data + end + crc

   //报文长度
   int packetlen;
}UUPacket;

//下行
typedef struct 
{
   u16 start;
   u8 local[5]; //遥测站地址
   u8 remote;
   u16 passwd;
   u8  cmd;
   u16 len;
   u8 stx;     //报文起始符
   u8 data[PDSIZE+20]; // data + end + crc
   
   //报文长度
   int packetlen;
   //etx
   u8 etx;

   ul64 rlocal;
   u16 rpasswd;

}UDPacket;

//用于底层接受缓存
typedef struct 
{
    int state;
    int len;
    UDPacket p;
    /* data */
}UDpacketCache;

typedef enum{
   STX_STX = 0x02,// 传输正文起始
   STX_SYN = 0x16,// 多包传输正文起始
}STX;

typedef enum{
   ETX_NULL= 0x00, //自定义 poll
   ETX_ETX=0x03,// 报文结束，后续无报文 作为报文结束符，表示传输完成，等待退出通信
   ETX_ETB=0x17,// 报文结束，后续有报文在报文分包传输时作为报文结束符，表示传输未完成，不可退出通信
   ETX_ENQ=0x05,//询问 作为下行查询及控制命令帧的报文结束符。
   ETX_EOT=0x04,//传输结束，退出作为传输结束确认帧报文结束符，表示可以退出通信。
   ETX_ACK=0x06,//H 肯定确认，继续发送 作为有后续报文帧的“确认帧”报文结束符。
   ETX_NAK=0x15,//H 否定应答，反馈重发 用于要求对方重发某数据包的报文结束符。
   ETX_ESC=0x1B,//H 传输结束，终端保持在线在下行确认帧代替 EOT 作为报文结束符，要求终端在线。保持在线 10 分钟内若没有接收到中心站命令，终端退回原先设定的工作状态。
}ETX;

typedef enum{
      UPCMD_2FH_LianluWeichiBao	=0x2F,//	链路维持报	遥测站定时向中心站发送链路维持信息	
      UPCMD_30H_CeshiBao	=0x30,//	测试报	报送实时数据	
      UPCMD_31H_JunyunShiduanBao	=0x31,//	均匀时段水文信息报	报送等时间间隔数据	
      UPCMD_32H_DingshiBao	=0x32,//	遥测站定时报	报送由时间触发的实时数据	
      UPCMD_33H_JiabaoBao	=0x33,//	遥测站加报报	报送由时间或事件触发的加报实时数据	
      UPCMD_34H_XiaoshiBao	=0x34,//	遥测站小时报	报送以小时为基本单位的历史数据和实时数据	
      UPCMD_35H	=0x35,//	遥测站人工置数报	报送人工置数	
      UPCMD_36H_Image	=0x36,//	遥测站图片报或中心站查询遥测站图片采集信息	查询/报送	JPG
      UPCMD_37H_shishi	=0x37,//	中心站查询遥测站实时数据		
      UPCMD_38H	=0x38,//	中心站查询遥测站时段数据	以小时为基本单位查询历史数据	
      UPCMD_39H	=0x39,//	中心站查询遥测站人工置数		
      UPCMD_3AH_zhidingids	=0x3A,//	中心站查询遥测站指定要素数据		
      UPCMD_40H_setparams	=0x40,//	中心站修改遥测站基本配置表	遥测站基本配置	
      UPCMD_41H_getparams	=0x41,//	中心站读取遥测站基本配置表/遥测站自报基本配置表		
      UPCMD_42H_setruntimes	=0x42,//	中心站修改遥测站运行参数配置表	遥测站运行参数配置	
      UPCMD_43H_getruntimes	=0x43,//	中心站读取遥测站运行参数配置表/遥测站自报运行参数配置表		
      UPCMD_44H	=0x44,//	查询水泵电机实时工作数据		
      UPCMD_45H_getVersion	=0x45,//	查询遥测终端软件版本		
      UPCMD_46H	=0x46,//	查询遥测站状态和报警信息		
      UPCMD_47H_initmemory	=0x47,//	初始化固态存储数据	应与标识符配合使用以提高安全性	
      UPCMD_48H_factorysetting	=0x48,//	恢复终端出厂设置	应与标识符配合使用以提高安全性	
      UPCMD_49H	=0x49,//	修改密码		
      UPCMD_4AH_settime	=0x4A,//	设置遥测站时钟		
      UPCMD_4BH	=0x4B,//	设置遥测终端	IC	卡状态
      UPCMD_4CH	=0x4C,//	控制水泵开关命令/水泵状态信息自报		
      UPCMD_4DH	=0x4D,//	控制阀门开关命令/阀门状态信息自报		
      UPCMD_4EH	=0x4E,//	控制闸门开关命令/闸门状态信息自报		
      UPCMD_4FH	=0x4F,//	水量定值控制命令		
      UPCMD_50H	=0x50,//	中心站查询遥测站事件记录		
      UPCMD_51H	=0x51,//	中心站查询遥测站时钟		
}UPCMD;
#pragma pack()

UDPacket* upacket_decode(UDpacketCache*ud,u8*datain,int len,bool reset);

int upacket_encode(UUPacket*packet,bool up,u8 remote, ul64 local, u16 passwd, UPCMD cmd, u8*data1,int len1,u8* data2, int len2, STX stx,ETX etx);
int upacket_encodehex(UUPacket*packet,bool up, u8 remote, UPCMD cmd, u8* data, int len,STX stx,ETX etx);
int upacket_encodehex_m3(UUPacket*packet,bool up, u8 remote, UPCMD cmd,int total,int sn, u8* data, int len,STX stx,ETX etx);


const char*cmd_to_string(int cmd);
const char*ext_to_string(int ext);

#endif
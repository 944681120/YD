#include "upacket.h"


/*=================================================================
    上行协议
===================================================================*/

UDPacket* upacket_decode(UDpacketCache*ud,u8*datain,int len,bool reset)
{
    int i = 0;
    if(ud!=NULL){
        
        u8*d = (u8*)&ud->p;
        if(reset){
            ud->len=ud->state = 0;
            memset((void*)&ud->p,0,sizeof(ud->p));
            return NULL;
        }
        //接受数据
        while (i<len)
        {
            u8 data = datain[i];
            if(ud->len >= sizeof(ud->p))goto DECODE_ERROR;  
            d[ud->len] = data;ud->len++;
            i++;
            switch(ud->state)
            {
                //start
                case 0:
                    if(data==0x7E)
                    {
                        ud->state = 0;
                        if(ud->len==2){
                            ud->state=1;
                        }
                    }
                    else goto DECODE_ERROR;
                    break;

                //local address [5]
                case 1:
                    if(ud->len==7){
                        //比较地址
                        ud->state++;
                    }break;                            
                //center address
                case 2:
                       ud->state++;break;
                //password
                case 3:
                      if(ud->len ==  10){
                          //比密码
                          ud->state++;
                      }
                      break;
                //cmd //hlen //llen
                case 4: 
                case 5:
                case 6:
                    ud->state++;break;
                //stx
                case 7:ud->state++;break;	       
                //接收数据
                case 8:
                {
                    u16 dlen = ((ud->p.len>>8)&0xFF)| ((ud->p.len&0xFF)<<8);
                    if((dlen&0x8000)!=0x8000)//不是一个下行包，丢掉
                    {
                        ERROR("[upacket_decode][get]:error,get a up dir packet!");
                        return NULL;
                    }
                    dlen &=0x0FFF;
                    if(ud->len < (dlen+14+1+2))break;  //data+stop+crc
                    else{	
                        u16 crc = d[ud->len-2];
                        crc<<=8;
                        crc|=d[ud->len-1];

    // {
    //     int ii = 0;
    //     u8*dd = (u8*)&ud->p;
    //     while (ii<ud->len)
    //     {
    //         printf("%02x",dd[ii]);ii++;
    //     }
    //     printf("\n");
    // }
                        if(crc_calculate((u8*)&ud->p,ud->len-2)==crc)
                        {
                            //接收到一个数据包
                            ud->p.packetlen = ud->len;
                            ud->len=ud->state=0;
                            ud->p.etx = ((u8*)&ud->p)[ud->p.packetlen-3];

                            //转换 local passwd
                            //ud->p.rlocal = bigbcd2hex((u8*)ud->p.local,5);
                            inverseto(ud->p.local,(u8*)&ud->p.rlocal,5);
                            inverseto((u8*)&ud->p.passwd,(u8*)&ud->p.rpasswd,2);
                            //ud->p.rpasswd = bigbcd2hex((u8*)&(ud->p.passwd),2);
                            return &ud->p;
                        }else goto DECODE_ERROR;
                    }
                }
                default:
                        goto DECODE_ERROR;
            }
            continue;
            DECODE_ERROR:
                ud->len=ud->state=0;
            continue;
        }   
        return NULL; 
    }
    return NULL;
}

int upacket_encode(UUPacket*packet,bool up,u8 remote, ul64 local, u16 passwd, UPCMD cmd, u8*data1,int len1,u8* data2, int len2, STX stx,ETX etx)
{
    u16 crc = 0;
    int len = len1 + len2;
    packet->start = 0x7E7E;    
    packet->stx = (u8)stx;
    packet->remote = remote;
    //hex2bigbcd((u8*)&local,5);
    inverseto((u8*)&local,(u8*)packet->local,5);
    inverseto((u8*)&passwd,(u8*)&packet->passwd,2);
    //hex2bigbcd((u8*)&passwd,2);
    //packet->passwd = passwd;
    packet->cmd = cmd;
    //长度高4bit 0000 :上行   1000: 下行
    if(len > (PDSIZE-3))len = PDSIZE;
    packet->len = 0;
    if(up==false){packet->len|=0x0080;}
    packet->len |= (len&0xff)<<8;
    packet->len |= (len>>8)&0xFF;
    if(data1!=NULL)
        memcpy(packet->data,data1,len1);
    if(data2!=NULL)
        memcpy(packet->data+len1,data2,len2);
    packet->data[len] = (u8)etx;
    crc = crc_calculate((u8*)packet,len+15);
    packet->data[len+1] =  (crc>>8)&0xFF;
    packet->data[len+2] =  crc&0xff;
    packet->packetlen = len+17;

    {
        char bytes[1024];memset(bytes,0,sizeof(bytes));
        hexarray2str((char*)packet->data,len,bytes,sizeof(bytes));
        INFO("上行 cmd =%s, data-len =%d data =%s",cmd_to_string(cmd),len1+len2,bytes);   
    }

    return len+17;
}

#include "DataClass.hpp"
int upacket_encodehex(UUPacket*packet,bool up, u8 remote, UPCMD cmd, u8* data, int len,STX stx,ETX etx)
{
    ul64 local = param[0x02].val;
    u16  passwd = param[0x03].val;
    return upacket_encode(packet,up,remote, local, passwd, cmd,NULL,0, data, len, stx,etx);
}

int upacket_encodehex_m3(UUPacket*packet,bool up, u8 remote, UPCMD cmd,int total,int sn, u8* data, int len,STX stx,ETX etx)
{
    u32 tmp = 0;
    tmp = total<<12;tmp &=0xFFF000;
    tmp |= sn&0xFFF;
    ul64 local = param[0x02].val;
    u16  passwd = param[0x03].val;
    return upacket_encode(packet,up,remote, local, passwd, cmd,(u8*)&tmp,3, data, len, stx,etx);
}



const char*cmd_to_string(int cmd)
{
    switch (cmd)
    {
      case UPCMD_2FH_LianluWeichiBao	: return "0x2F	链路维持报";//	遥测站定时向中心站发送链路维持信息	
      case UPCMD_30H_CeshiBao	: return "0x30	测试报";//	报送实时数据	
      case UPCMD_31H_JunyunShiduanBao	: return "0x31	均匀时段水文信息报";//	报送等时间间隔数据	
      case UPCMD_32H_DingshiBao	: return "0x32	遥测站定时报";//	报送由时间触发的实时数据	
      case UPCMD_33H_JiabaoBao	: return "0x33	遥测站加报报";//	报送由时间或事件触发的加报实时数据	
      case UPCMD_34H_XiaoshiBao	: return "0x34	遥测站小时报";//	报送以小时为基本单位的历史数据和实时数据	
      case UPCMD_35H	: return "0x35	遥测站人工置数报";//	报送人工置数	
      case UPCMD_36H	: return "0x36	遥测站图片报";//或中心站查询遥测站图片采集信息	查询/报送	JPG
      case UPCMD_37H_shishi	: return "0x37	中心站查询遥测站实时数据";//		
      case UPCMD_38H	: return "0x38	中心站查询遥测站时段数据";//	以小时为基本单位查询历史数据	
      case UPCMD_39H	: return "0x39	中心站查询遥测站人工置数";//		
      case UPCMD_3AH_zhidingids	: return "0x3A	中心站查询遥测站指定要素数据";//		
      case UPCMD_40H_setparams	: return "0x40	中心站修改遥测站基本配置表";//	遥测站基本配置	
      case UPCMD_41H_getparams	: return "0x41	中心站读取遥测站基本配置表";///遥测站自报基本配置表		
      case UPCMD_42H_setruntimes	: return "0x42	中心站修改遥测站运行参数配置表";//	遥测站运行参数配置	
      case UPCMD_43H_getruntimes	: return "0x43	中心站读取遥测站运行参数配置表";///遥测站自报运行参数配置表		
      case UPCMD_44H	: return "0x44	查询水泵电机实时工作数据";//		
      case UPCMD_45H_getVersion	: return "0x45	查询遥测终端软件版本";//		
      case UPCMD_46H	: return "0x46	查询遥测站状态和报警信息";//		
      case UPCMD_47H_initmemory	: return "0x47	初始化固态存储数据";//	应与标识符配合使用以提高安全性	
      case UPCMD_48H_factorysetting	: return "0x48	恢复终端出厂设置";//	应与标识符配合使用以提高安全性	
      case UPCMD_49H	: return "0x49	修改密码";//		
      case UPCMD_4AH_settime	: return "0x4A	设置遥测站时钟";//		
      case UPCMD_4BH	: return "0x4B	设置遥测终端";//	IC	卡状态
      case UPCMD_4CH	: return "0x4C	控制水泵开关命令";///水泵状态信息自报		
      case UPCMD_4DH	: return "0x4D	控制阀门开关命令";///阀门状态信息自报		
      case UPCMD_4EH	: return "0x4E	控制闸门开关命令";///闸门状态信息自报		
      case UPCMD_4FH	: return "0x4F	水量定值控制命令";//		
      case UPCMD_50H	: return "0x50	中心站查询遥测站事件记录";//		
      case UPCMD_51H	: return "0x51	中心站查询遥测站时钟";//
      default:return "unkonwn";	
    }
    return NULL;
}
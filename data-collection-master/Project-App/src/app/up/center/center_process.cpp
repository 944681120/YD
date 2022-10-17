
#include "center.h"
#include "report.h"
#include "upacket.h"
#include "mxdevice.h"
#include "sendmode.h"

#define WAITMS 4000

static u8 centerdata[4096];
//   UPCMD_36H	=0x36,//	遥测站图片报或中心站查询遥测站图片采集信息	查询/报送	JPG
//   UPCMD_37H	=0x37,//	中心站查询遥测站实时数据
//   UPCMD_38H	=0x38,//	中心站查询遥测站时段数据	以小时为基本单位查询历史数据
//   UPCMD_39H	=0x39,//	中心站查询遥测站人工置数

int center_process(SendMode *s, UDPacket *packet)
{
    int len = 0;
    packet = packet;
    u8 *d = centerdata;
    u8 *datain = packet->data;
    int inlen = packet->len & 0xFFF;
    u16 pass = 0;
    //比较 地址
    if (packet->rlocal != param[0x02].val)
    {
        ERROR("地址错误, local=0x%llx packet=0x%llx", (unsigned long long)param[0x02].val, (unsigned long long)packet->rlocal);
        return -1;
    }

    //比较密码
    if (packet->rpasswd != param[0x03].val)
    {
        ERROR("密码错误, packet=0x%x", packet->rpasswd);
        return -1;
    }

    {
        char bytes[1024];
        memset(bytes, 0, sizeof(bytes));
        hexarray2str((char *)packet->data, packet->len & 0x7FFF, bytes, sizeof(bytes));
        INFO("下行 cmd =%s, data-len =%d data =%s", cmd_to_string(packet->cmd), packet->len & 0x7FFF, bytes);
    }

    // packet 必须是下行包
    if ((packet->len & 0x8000) == 0x8000)
    {
        switch (packet->cmd)
        {
        case UPCMD_36H_Image: //读图片
        {
            extern bool report_a_image();
            report_a_image();
        }
        break;
        case UPCMD_37H_shishi: //中心站查询遥测站实时数据
            len = get_shishi_all(d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
            break;
        case UPCMD_38H: //	中心站查询遥测站时段数据	以小时为基本单位查询历史数据
            len = get_shiduan(datain, inlen, d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
            break;
        case UPCMD_39H: //	中心站查询遥测站人工置数
            len = get_rengong_report(d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
            break;
        case UPCMD_3AH_zhidingids: //   UPCMD_3AH	=0x3A,//	中心站查询遥测站指定要素数据
            len = get_zhiding_ids(datain, inlen, d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
            break;
        case UPCMD_40H_setparams: //   UPCMD_40H	=0x40,//	中心站修改遥测站基本配置表	遥测站基本配置
            len = set_param(datain, inlen, d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
            break;
        case UPCMD_41H_getparams: //   UPCMD_41H	=0x41,//	中心站读取遥测站基本配置表/遥测站自报基本配置表
            len = get_param(datain, inlen, d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
            break;
        case UPCMD_42H_setruntimes: //   UPCMD_42H	=0x42,//	中心站修改遥测站运行参数配置表	遥测站运行参数配置
            len = set_runtime(datain, inlen, d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
            break;
        case UPCMD_43H_getruntimes: //   UPCMD_43H	=0x43,//	中心站读取遥测站运行参数配置表/遥测站自报运行参数配置表
            len = get_runtime(datain, inlen, d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
            break;
        case UPCMD_44H: //   UPCMD_44H	=0x44,//	查询水泵电机实时工作数据
            len = get_shishi_motor(d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
            break;
        case UPCMD_45H_getVersion: //   UPCMD_45H	=0x45,//	查询遥测终端软件版本
            len = get_version(d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
            break;
        case UPCMD_46H: //   UPCMD_46H	=0x46,//	查询遥测站状态和报警信息
            len = get_state_warn_info(d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
            break;
        case UPCMD_47H_initmemory: //   UPCMD_47H	=0x47,//	初始化固态存储数据	应与标识符配合使用以提高安全性
            len = clear_history(datain, inlen, d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
            break;
        case UPCMD_48H_factorysetting: //   UPCMD_48H	=0x48,//	恢复终端出厂设置	应与标识符配合使用以提高安全性
            len = reset_to_factory(datain, inlen, d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
        case UPCMD_49H: //   UPCMD_49H	=0x49,//	修改密码
            len = change_password(datain, inlen, d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
            break;
        case UPCMD_4AH_settime: //   UPCMD_4AH	=0x4A,//	设置遥测站时钟
            len = set_time(datain, inlen, d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
            break;
        case UPCMD_4BH: //   UPCMD_4BH	=0x4B,//	设置遥测终端	IC	卡状态
            len = set_IC_state(datain, inlen, d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
            break;
        case UPCMD_4CH: //   UPCMD_4CH	=0x4C,//	控制水泵开关命令/水泵状态信息自报
            len = set_moter_onoff(datain, inlen, d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
            break;
        case UPCMD_4DH: //   UPCMD_4DH	=0x4D,//	控制阀门开关命令/阀门状态信息自报
            len = set_famen_onoff(datain, inlen, d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
            break;
        case UPCMD_4EH: //   UPCMD_4EH	=0x4E,//	控制闸门开关命令/闸门状态信息自报
            len = set_zhamen_onoff(datain, inlen, d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
            break;
        case UPCMD_4FH: //   UPCMD_4FH	=0x4F,//	水量定值控制命令
            len = set_shuiliang_control(datain, inlen, d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
            break;
        case UPCMD_50H: //   UPCMD_50H	=0x50,//	中心站查询遥测站事件记录
            len = get_event_state(datain, inlen, d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
            break;
        case UPCMD_51H: //   UPCMD_51H	=0x51,//	中心站查询遥测站时钟
            len = get_time(d);
            s->init(M4, get_current_remote(), (UPCMD)packet->cmd, d, len, PDSIZE);
            s->wait(WAITMS, NULL, NULL);
            break;
        default:
            //丢掉数据，不处理
            break;
        }
    }
    INFO("[center]:exit,cmd=%s.", cmd_to_string(packet->cmd));
    return 0;
}
#include "report.h"
#include "upacket.h"
#include "mxdevice.h"
#include "sendmode.h"
#include "unistd.h"
#include "Ibao.hpp"

/*=================================================================
    图片报
===================================================================*/
BaoImageBase image;

bool _report_a_image(SendMode *sm)
{
    if (image.report_check(false))
    {
        const char *file = (const char *)image.report_data();
        if (file != NULL)
        {
            MPBase *im3 = new MPImageBase(file, rtu.device.image.persize, std::time(0));
            INFO("图片上报=====>方式:%s, 文件:%s", rtu.device.image.mode.c_str(), file);
            if (rtu.device.image.mode == "M3" || rtu.device.image.mode == "m3")
            {
                sm->initMul(M3, get_current_remote(), UPCMD_36H_Image, im3);
            }
            else
            {
                sm->initMul(M2, get_current_remote(), UPCMD_36H_Image, im3);
            }
            if (sm->wait(4000, NULL, NULL) != 0)
            {
                ERROR("上传图片失败");
                image.report_fail();
            }
            else
            {
                INFO("成功");
                image.report_ok();
            }
            delete im3;
            return true;
        }
    }
    return false;
}

bool report_a_image()
{
    return _report_a_image(&tcp_sendmode);
}

/*=================================================================
    定时，小时，加报，补报，统一流程控制 使用Ibao流程
===================================================================*/
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
JiaBaoBase liuliangjiabao("流量加报", {.cmd = (int)UPCMD_33H_JiabaoBao,
                                       .data = &devdata,
                                       .save_table_name = TableNameJia,
                                       .triglename = "jia",
                                       .warnname = "liuliang"});

BuBaoBase bubao("补报");
RawBao lianlubao("链路报", get_lianlu_report);
vector<Ibao *> baos = {
    &dingshibao, &xiaoshibao,
    &rainjiabao, &waterleveljiabao, &liuliangjiabao,
    &bubao, &lianlubao, &image};

//清空所有数据表
bool bao_clear(void)
{
    bool result = true;
    for (int i = 0; i < baos.size(); i++)
    {
        if (baos[i]->clearAll() == false)
            result = false;
    }
    return result;
}

static void bao_init(void)
{

    /*设置报文时间,具体从 runtime中读取*/
    int dingshi_interval = runtime[0x20].Value() * 3600;
    int xiaoshi_interval = 60 * 60;
    int jiabao_interval = runtime[0x21].Value() * 60;
    int bubao_interval = 15 * 60;
    int lianlu_interval = 5 * 60;

    /*报文重发3次，耗时 15s,时间间隔为 15s*/
    dingshibao.set_interval(dingshi_interval, 3, 10);
    xiaoshibao.set_interval(xiaoshi_interval, 4, 25);
    rainjiabao.set_interval(jiabao_interval, 5, 40);
    waterleveljiabao.set_interval(jiabao_interval, 5, 55);
    bubao.set_interval(bubao_interval, 6, 70);
    lianlubao.set_interval(lianlu_interval, -21, -20);

    /*设置上报数据项*/
    dingshibao.set_report_ids(rtu.param.dingshi);
    xiaoshibao.set_report_ids(rtu.param.xiaoshi);
    rainjiabao.set_report_ids(rtu.param.jiabao["rain"]);
    waterleveljiabao.set_report_ids(rtu.param.jiabao["waterlevel"]);
}
static char buffer[4096];
static char bufferstring[4096];
static void bao_poll(SendMode *s)
{
    for (int i = 0; i < baos.size(); i++)
    {
        memset(buffer, 0, sizeof(buffer));
        memset(bufferstring, 0, sizeof(bufferstring));
        baos[i]->poll();
        if (baos[i]->report_check(false))
        {
            int rs = baos[i]->report_data(buffer);
            // hexarray2str(buffer, rs, bufferstring, sizeof(bufferstring));
            // INFO("上报数据为:%s", bufferstring);
            if (s == &serial_sendmode)
            {
                ERROR("端口未打开,转到补报");
                baos[i]->report_fail();
                baos[i]->report_check(true);
            }
            else
            {
                //上传图片
                if (baos[i]->Cmd() == UPCMD_36H_Image)
                {
                    report_a_image();
                }
                //上传其他数据
                else
                {
                    // report data
                    INFO("%s ===============>正在上报<=============", baos[i]->name.c_str());
                    int rs = baos[i]->report_data(buffer);
                    hexarray2str(buffer, rs, bufferstring, sizeof(bufferstring));
                    INFO("上报数据为:%s", bufferstring);
                    if (baos[i] == &lianlubao)
                    {
                        if (rtu.device.gz.link_mode == "M2" || rtu.device.gz.link_mode == "m2")
                        {
                            s->init(M2, get_current_remote(), UPCMD_2FH_LianluWeichiBao, (u8 *)buffer, rs, rs);
                        }
                        else
                        {
                            s->init(M1, get_current_remote(), UPCMD_2FH_LianluWeichiBao, (u8 *)buffer, rs, rs);
                        }
                    }

                    else
                        s->init(M2, get_current_remote(), baos[i]->Cmd(), (u8 *)buffer, rs, rs);
                    if (s->wait(2000, NULL, NULL) != 0)
                    {
                        ERROR("上报失败,转到补报");
                        baos[i]->report_fail();
                    }
                    else
                    {
                        baos[i]->report_ok();
                    }
                    baos[i]->report_check(true);
                }
            }
        }
    }
}

//============根据控制符，管理通道状态========================================================
//工作方式 0CH N(2)   BCD 码，1-自报工作状态；2-自报确认工作状态；3-查询/应答工作状态；4-调试或维修状态
int get_work_state(void)
{
    u8 byte = 0;
    byte = (u8)param[0x0C].val;
    switch (byte)
    {
    case 1:
        return M1;
    case 2:
        return M2;
    case 3:
        return M3;
    default:
        return M4;
        break;
    }
    return M2;
}
int communication_state(SendMode *sm)
{
    ETX e = (ETX)sm->etx;
    if ((int)e > (int)ETX_NULL)
    {
        switch (e)
        {
        //保持10min
        case ETX_ESC:
        {
            sm->dev->keepalive(true);
            sm->utimeout = get_ms_clock() + 600000;
            INFO("收到 EXT_EOT,关闭连接,目前默认 keepalive.");
        }
        break;
        //退出通讯
        case ETX_EOT:
        {
            INFO("收到 EXT_EOT,关闭连接,目前默认 keepalive.");
            // sm->dev->keepalive(false);
            // sm->dev->close(NULL);
            sm->utimeout = 0;
        }
        break;
        default:
            break;
        }
        sm->etx = ETX_NULL;
    }
    else
    {

        // 10min，返回工作状态
        if (sm->utimeout < get_ms_clock())
        {
            //关掉通道
            if ((get_work_state() == M1 || get_work_state() == M2) && sm->utimeout != 0)
            {
                if (sm->dev->is_open())
                {
                    INFO("收到 EXT_EOT,关闭连接,目前默认 keepalive.");
                    // INFO("无操作,10min 断网");
                    // sm->dev->keepalive(false);
                    // sm->dev->close(NULL);
                }
            }
            sm->utimeout = get_ms_clock() + 600000;
        }
    }
    return 0;
}
extern int get_sendmode(SendMode **sm);

/*=================================================================
    主动上报
1.上报时间错开策略:
   (1) 整点上报的为:
       小时报/定时报
   (2) 不定时间间隔的为
       链路/加报/测试
   (3) 每次链路报时，服务器会远程读取配置信息：
       参数 时钟 版本信息

===================================================================*/
void zhudong_report_process(void)
{
    SendMode *s = NULL;
    static bool inited = false;
    if (inited == false)
    {
        inited = true;
        bao_init();
    }
    /*---------------工作状态--------------------------------*/
    communication_state(&tcp_sendmode);
    if (get_sendmode(&s) == -1)
    {
        s = &serial_sendmode;
    }
    bao_poll(s);
}


#include "app.h"
#include "serialport.h"
#include "mxdevice.h"
#include "dfsm.h"
#include "lib.h"
#include "header.h"
#include "rtu_setting.hpp"
#include "unistd.h"

/*-------------------------------------------------------------------------------------
                 传感器转码
--------------------------------------------------------------------------------------*/
u8 string_to_byte(char *st)
{
  u8 h = st[0], l = st[1];
  if (l >= '0' && l <= '9')
    l -= '0';
  else
    l -= 'A' - 10;
  if (h >= '0' && h <= '9')
    h -= '0';
  else
    h -= 'A' - 10;
  return (h << 4) | (l & 0x0F);
}

int recv_patten(char *patten, u8 *dataout)
{
  int sl = strlen((const char *)patten);
  int i = 0;
  u8 pre[20];
  int precount = 0;
  while (i < sl && i < 58)
  {
    pre[precount] = string_to_byte(((char *)patten) + i);
    i += 2;
    precount++;
  }

  memcpy(dataout, pre, precount);
  return precount;
}

//统一返回 xxxxxx.xxx * 1000
__int64_t parse_data(char *recv, char *shunxu, char *fomat, u8 *frame, int len, int *ferr)
{
  // 1. 去掉前面的数据
  u8 pre[20];
  int prec = 0;
  prec = recv_patten((char *)recv, pre);

  PRINTBYTES("[parse_data:pre] ", pre, prec);
  PRINTBYTES("[frame         ] ", frame, len);

  *ferr = 0;
  if (memcmp(pre, frame, prec) != 0)
  {
    INFO("帧头错误,无法解析.");
    *ferr = 1;
    return 0;
  }
  else
  {
    *ferr = 0;
    INFO("帧头OK.");
  }

  // 2. 顺序提取
  u8 data[10];
  u8 *d = frame + prec;
  int size = strlen((const char *)shunxu);
  for (int i = 0; i < size; i++)
  {
    data[i] = d[((const char *)shunxu)[i] - 'A'];
  }

  // 3. 格式转换
  if (memcmp("int", fomat, 3) == 0)
  {
    __int64_t value = 0;
    blreverse(data, &value, size);
    if (strlen((const char *)fomat) > 4)
    {
      int b = atoi(((char *)fomat) + 4);
      if (b != 0)
      {
        return (value * 1000) / b;
      }
    }
    return value * 1000;
  }
  else if (memcmp("float", (const char *)fomat, 4) == 0)
  {
    float value = 0;
    blreverse(data, &value, size);
    if (strlen((const char *)fomat) > 6)
    {
      int b = atoi(((char *)fomat) + 6);
      if (b != 0)
      {
        return (l64)((value * 1000) / b);
      }
    }
    return (l64)(value * 1000);
  }
  else if (memcmp("double", fomat, 6) == 0)
  {
    double value = 0;
    blreverse(data, &value, size);
    // PRINTBYTES("double",(u8*)&value,sizeof(value));
    if (strlen((const char *)fomat) > 7)
    {
      int b = atoi(((char *)fomat) + 7);
      if (b != 0)
      {
        return (l64)((value * 1000) / b);
      }
    }
    return (l64)(value * 1000);
  }
  else if (memcmp("bcd", fomat, 3) == 0)
  {
    l64 value = 0;
    inverse(data, size);
    value = (l64)bcd2hex(data, size);
    if (strlen((const char *)fomat) > 4)
    {
      int b = atoi(((char *)fomat) + 4);
      if (b != 0)
      {
        return (l64)((value * 1000) / b);
      }
    }
    return (l64)(value * 1000);
  }

  return 0;
}

#include "lib.h"
void test_format(void)
{
  const char *line1 = "01 03 00 08 00 02 45 C9";
  const char *line2 = "01 03 04 06 51 3F 9E 3B 32";
  const char *line3 = "01 04 00 00 76 45 23 01 AA";
  const char *line19 = "01 04 08 40 A3 EA 06 A3 D2 4A B4 98 97";
  u8 f[50];
  int ferr = 0;
  int size = recv_patten((char *)line19, f);
  // l64 value = parse_data(&device_cmd_configs[18],f,size,&ferr);
  // INFO("[test-format] ferr=%d, value = %ld.\n",ferr,value);
}

/*-------------------------------------------------------------------------------------
                 传感器读取
--------------------------------------------------------------------------------------*/
l64 read_a_data(dfsm *dev, int index, int timeoutms, int *result)
{
  rtu_485 item = rtu.rs485[index - 1];
  INFO("[读传感器] %s, port=%s  baud=%d  name=%s cmd=%s result=%s datatype=%s formatter=%s 超时=%d ms",
       (const char *)item.factorType.data(),
       (const char *)item.port.data(),
       item.baud,
       (const char *)item.brandName.data(),
       (const char *)item.requestCmd.data(),
       (const char *)item.resultDataFilter.data(),
       (const char *)item.dataType.data(),
       (const char *)item.dataFormatter.data(), timeoutms);

  // 1.send
  u8 sbuffer[100];
  int sendlen = recv_patten((char *)item.requestCmd.data(), sbuffer);
  if (sendlen > 0)
  {

    dev->wann = (strlen((char *)item.resultDataFilter.data()) + strlen((char *)item.dataFormatter.data())) / 2;
    INFO("[读传感器] wann=%d  step=%d.", dev->wann, dev->step);
    if (dev->step == DFSM_STATE_IDLE || dev->step == DFSM_STATE_WAIT)
    {
      dev->init(
          sbuffer, sendlen, 2, [](void *a)
          { INFO("[读传感器]:超时"); },
          [](void *b) {});
      long timeout = get_ms_clock() + timeoutms; // 2s
      while ((timeout > get_ms_clock()) && (dev->getpacket == false))
      {
        dev->process();
        usleep(10000);
      }
      if (dev->getpacket == true)
      {
        int ferr = 0;
        l64 value = (l64)parse_data(
            (char *)item.resultDataFilter.data(),
            (char *)item.dataFormatter.data(),
            (char *)item.dataType.data(),
            dev->rb, dev->rlen, &ferr);

        // reset
        dev->step = DFSM_STATE_IDLE;
        dev->slen = dev->rlen = 0;

        if (ferr == 1)
        {
          *result = -1;
          INFO("[%s]: Frame err. exit.", (char *)item.factorType.data());
          return 0;
        }
        else
        {
          *result = 1;
          INFO("[读传感器]: %s  成功.  value=%jd", (char *)item.factorType.data(), value);
          return value;
        }
      }
    }
  }
  else
  {
    *result = -1;
    ERROR("[读传感器]: %s 失败 exit.", (char *)item.factorType.data());
    return 0;
  }

  INFO("[读传感器]:  exit.");
  return 0;
}

/*================================================================*/
bool is_down(bool reset)
{
  static time_t last = 0;
  int s = 0;
  // 1. 定时读  //采样间隔 23H N(4) 0 ～9999 秒
  s = (int)runtime[0x23].val;
  time_t now;
  timestd(&now);
  if ((last + s) < now)
  {
    if (reset)
      last = now + s;
    return true;
  }
  return false;
}

/*================================================================*/

// down process
void *thread_down_deal(void *arg)
{
  INFO("[thread_down_deal] is running");
  dfsm_device_init();
  dfsm *pdd = NULL;

  while (1)
  {
    //===============采样数据==========================
    if (is_down(true))
    {
      //采样一次
      for (int i = 0; i < rtu.rs485.size(); i++)
      {
        DataClassItemConfig *pconfig;
        l64 value = 0;
        int returnValue = 0;
        bool result = false, clear = false;
        int index = 1 + i;
        rtu_485 item = rtu.rs485[index - 1];

        // 选择下行通道
        pdd = dfsm_device_find(rtu.rs485[i].port);
        if (pdd == NULL)
        {
          ERROR("[读传感器接口获取失败] %s port=%s  baud=%d  name=%s cmd=%s result=%s datatype=%s formatter=%s",
                (const char *)item.factorType.data(),
                (const char *)item.port.data(),
                item.baud,
                (const char *)item.brandName.data(),
                (const char *)item.requestCmd.data(),
                (const char *)item.resultDataFilter.data(),
                (const char *)item.dataType.data(),
                (const char *)item.dataFormatter.data());
          continue;
        }
        pdd->setbaud(item.baud);
        value = read_a_data(pdd, index, 4000, &returnValue);
        pdd->process();

        //设置观测时间
        if (returnValue > 0)
        {
          set_observetime_time(timestd(0));
          result = true;
          item.err_cnt = 0;
          clear = false;
        }
        else
        {
          result = false;
          item.err_cnt++;
          if (item.err_cnt >= 3) //数据读取3次失败，清零
            clear = true;
        }

        //分组处理数据
        pconfig = &devdata[item.factorType].config;
        if (pconfig->id != 0 && devdata[pconfig->id].config.id != 0)
        {
          // 21H - 31H 流量 //A1H - A8H 累计流量
          if ((pconfig->id >= 0x21 && pconfig->id <= 0x31) || (pconfig->id >= 0xFFA1 && pconfig->id <= 0xFFA8))
          {
            extern void flow_caculation(l64 wl, DataClassItemConfig * pconfig, bool result, bool clear);
            flow_caculation(value, pconfig, result, clear);
          }
          // 39H - 43H 水位
          else if (pconfig->id >= 0x39 && pconfig->id <= 0x43)
          {
            extern void waterlevel_caculation(l64 wl, DataClassItemConfig * pconfig, bool result, bool clear);
            waterlevel_caculation(value, pconfig, result, clear);
          }
          else
          {
            INFO("数据的直接保存");
            devdata[pconfig->id].val = value;
            INFO("[%s] = %jd.\n", pconfig->name.c_str(), value);
            devdata[pconfig->id].toString(devdata.Name().c_str());
          }
        }
        else
        {
          ERROR("无法处理数据:%s", item.factorType.c_str());
        }
        pdd->process();
        usleep(50000);
      }
      pdd->process();
    }

    usleep(10000);
    if (pdd != NULL)
      pdd->process();
  }
}

/*================================================================*/
// init
pthread_t thread_down_init(void) //
{
  pthread_t id;
  if (pthread_create(&id, NULL, thread_down_deal, NULL) != 0)
  {
    ERROR("[线程创建]:ERR = thread_down_deal");
    return 0;
  }
  INFO("[线程创建]:OK = thread_down_deal");
  return id;
}
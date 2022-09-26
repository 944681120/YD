#pragma once

#ifndef __LIB_H__
#define __LIB_H__

//通用头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "mytype.h"
#include "rtu_setting.hpp"

typedef void *packet_process(void *s, u8 *data, int len);

// ip <->hex
//"120.238.064.059:33048"
void ipstring2hex(char *str, char *hexout);
void hex2ipstring(char *hex, char *str);

extern u16 crc_calculate(u8 *check_buffer, int len);

int little2big(u8 *little, u8 *big, int size);
#define blreverse(little, big, size) little2big((u8 *)little, (u8 *)big, size)

// 1234 [0]=(1234)低字节  [1]=(1234)高字节  return:0x1234
ul64 hex2bcd(u8 *hex, int bytes);
// 0x1234 d[0]=0x34 d[1]=0x12  return 1234
ul64 bcd2hex(u8 *d, int bytes);
int hex2bigbcd(u8 *d, int bytes);
// 0x1234  bigbcd:[0]=0x12 [1]=0x34   转换后 [0]=(1234)低字节   [1]=(1234)高字节
ul64 bigbcd2hex(u8 *d, int bytes);

u16 bytehex2bcd(u8 hex);

long get_ms_clock(void);

typedef void Callback(void *);

void TimeSet(int year, int month, int day, int hour, int min, int sec);
void TimeSet(string str);
void inverse(u8 *data, int len);
void inverseto(u8 *data,u8*dataout,int len);

char *get_time(time_t t);
char *get_cur_time();

// 方法3
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)

int str2hexarray(const char *str, int slen, char *dataout, int outsize);
int hexarray2str(char *data, int size, char *str, int slen);
string tostring16(l64 v);

bool is_zero(u8 *d, int len);

// #define INFO(fmt, ...) printf("%s %s %d" fmt,"Msg ",get_cur_time(),(int)gettid(),##__VA_ARGS__ )
// #define ERROR(fmt, ...) printf("%s %s" fmt,"Err ",get_cur_time(),##__VA_ARGS__ )
#include "zlog.h"
extern bool zlog_inited;
extern zlog_category_t *zcat;
extern zlog_category_t *zcat_data;
#define INFO(fmt, ...)                     \
  do                                       \
  {                                        \
    if (zlog_inited == 1)                  \
      zlog_info(zcat, fmt, ##__VA_ARGS__); \
  } while (0)
#define ERROR(fmt, ...)                     \
  do                                        \
  {                                         \
    if (zlog_inited == 1)                   \
      zlog_error(zcat, fmt, ##__VA_ARGS__); \
  } while (0)

void init_zlog(void);
void drop_zlog(void);

/************************************************************************/
/* 保存执行 regexec 多次匹配数据                                          */
/************************************************************************/
#include "regex.h"
typedef struct search_match_t
{
  /** 捕获组数量(包含group 0) */
  size_t groupcnt;
  /** 可保存匹配次数 */
  size_t capacity;
  /** 匹配次数 */
  size_t matchcnt;
  /************************************************************************/
  /* 以在字符串中的顺序保存每一次匹配的数据,                              */
  /* 数组长度为 capacity*groupcnt,                                        */
  /* rx_search在执行时会根据需要自动对数组长度扩容                        */
  /************************************************************************/
  regmatch_t *pmatch;
} search_match_t;
void rx_search_match_uninit(search_match_t *_psmatch);
int rx_serach(const char *input, const char *pattern, size_t groupcnt, int eflags, search_match_t *_psmatch);

void time2string(time_t t, char *buffer, const char *format);
string time2string(time_t t, const char *format);
string time2string(time_t t);
time_t string2time(std::string str);

u8 crc8(u8 *data, int size);

#pragma pack(1)

typedef struct
{
  /*
  第一组要素（A1）
  D7 D6 D5 D4 D3 D2 D1 D0
  降水量 蒸发量 风向 风速 气温 湿度 地温 气压
  */
  u8 qiya : 1,
      dishi : 1,
      shidu : 1,
      qiwen : 1,
      fengsu : 1,
      fengxiang : 1,
      zhengfaliang : 1,
      jianshuiliang : 1;
  /*
  第二组要素（A2）
  D7 D6 D5 D4 D3 D2 D1 D0
  水位 8 水位 7 水位 6 水位 5 水位 4 水位 3 水位 2 水位 1
  */
  u8 shuiwei1 : 1, shuiwei2 : 1, shuiwei3 : 1, shuiwei4 : 1, shuiwei5 : 1, shuiwei6 : 1, shuiwei7 : 1, shuiwei8 : 1;

  /*
  第三组要素（A3）
  D7 D6 D5 D4 D3 D2 D1 D0
  地下水埋深 图片 波浪 闸门开度 水量 流速 流量 水压

  */
  u8 shuiya : 1,
      liuliang : 1,
      liusu : 1,
      shuiliang : 1,
      zaimengkaidu : 1,
      bolang : 1,
      tupian : 1,
      dixiangshuimaishen : 1;
  /*
  第四组要素（A4）
  D7 D6 D5 D4 D3 D2 D1 D0
  水表 8 水表 7 水表 6 水表 5 水表 4 水表 3 水表 2 水表 1
  */
  u8 shuibiao;

  /*
  第五组要素（A5）
  D7 D6 D5 D4 D3 D2 D1 D0
  100CM 80CM 60CM 50CM 40CM 30CM 20CM 10CM 墒情
  */
  u8 shangqing;

  /*
  第六组要素（A6）
  D7 D6 D5 D4 D3 D2 D1 D0
  pH 值 溶解氧 电导率 浊度 氧化还原电位 高锰酸 盐指数 氨氮 水温
  */
  /*
  第七组要素（A7）
  D7 D6 D5 D4 D3 D2 D1 D0
  总有机碳 总氮 总磷 锌 硒 砷 总汞 镉

  第八组要素（A8）
  D3 D2 D1 D0
  叶绿素a 铜 铅
  */
  u8 A6, A7, A8;
} YaoSu;

int get_yaosu(YaoSu *yao);

#pragma pack()

int get_cmd_results(const char *cmdstring, char *buff, int size);
string get_cmd_results(const char *cmdstring);

int mk_all_dir(char *dir);

// void PRINTBYTES(const char*header,u8*data,int size);
// inline void PRINTBYTES(const char*header,u8*data,int size)
#define PRINTBYTES(a__header, a__data, a__size, ...)                   \
  {                                                                    \
    u8 *__data = (u8 *)(a__data);                                      \
    int __size = (int)(a__size);                                       \
    char __buffer[1024];                                               \
    memset(__buffer, 0, sizeof(__buffer));                             \
    sprintf(__buffer, "[" a__header "]", ##__VA_ARGS__);               \
    sprintf(__buffer + strlen(__buffer), ":len=%d ,", __size);         \
    int __iii = 0;                                                     \
    int __offset = strlen(__buffer);                                   \
    while (__iii < __size && __iii < 512)                              \
    {                                                                  \
      sprintf(__buffer + __offset + __iii * 2, "%02x", __data[__iii]); \
      __iii++;                                                         \
    }                                                                  \
    zlog_info(zcat, "%s", __buffer);                                   \
  }

int zlog_delect_log(const char *dir, int count);

bool is_diff_hour(time_t t1, time_t t2);

bool Cycle_Check(time_t now, int cycle_seconds, int delays);
#define timestd std::time

int TimeOffset5Min(time_t t);

std::vector<std::string> split(std::string str, std::string pattern);

std::string list_merge(std::vector<std::string> list, std::string parttern);
#include "DataClass.hpp"

#endif
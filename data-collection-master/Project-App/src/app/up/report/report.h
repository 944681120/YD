#ifndef __REPORT_REPORT_H__
#define __REPORT_REPORT_H__


#include "ZhengWen.hpp"
#include "mxdevice.h"
#include "sendmode.h"

int get_lianlu_report(u8*dataout);
/*=============================测试报===========================*/
int get_ceshi_report(u8*dataout);
/*=============================均匀时段水文信息报=================*/
int get_junyunshiduan_report(u8*dataout);
/*=============================均匀时段水文信息报=================*/
int get_junyunshiduan_report(u8*dataout);
/*=============================定时报水文信息报:实时数据=================
1.  定时报，按 附录E,根据类型来定
====================================================================*/
int get_E1_JiangShui(u8*data);
int get_E2_HeDao(u8*data);
int get_E3_ShuiKu(u8*data);
int get_E4_ZhaBa(u8*data);
int get_E5_BengZhan(u8*data);
int get_E6_Chaoxi(u8*data);
int get_dingshi_report(u8*dataout);
/*=============================加时报===============================*/
int get_jia_report(u8*dataout);
/*=============================小时报===============================*/
int get_xiaoshi_report(u8*dataout);
/*===========================人工置数报============================*/
int get_rengong_report(u8*dataout);
/*===========================图片报===============================*/
int get_tupian_report(u8**dataout);


void zhudong_report_process(void);
#endif
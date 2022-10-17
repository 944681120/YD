#pragma once

#ifndef __REPORT_REPORT_HEADER_H__
#define __REPORT_REPORT_HEADER_H__
#include "lib.h"

#pragma pack(1)
// Header
class Header
{
    const u8 ST = 0xF1; //地址， 引导符
    const u8 TT = 0xF0; //观测时间,引导符

    //添加额外的数据
    int add_extbytes(u8 *dataout);

public:
    u16 liushui;
    time_t sendtime;
    ul64 local;
    u8 type;
    time_t observetime;

    Header();
    ~Header();
    int to_array(u8 *dataout);
    string toString();
    int to_array_front3(u8 *d);
    string toStringfront3();
};

#pragma pack()

u8 get_current_remote(void);
u8 set_current_remote(u8 r);

int get_current_liushui(void);
int set_current_liushui(int liu);
time_t get_current_report_time(void);
time_t get_observetime_time(void);
void set_observetime_time(time_t t);

#endif

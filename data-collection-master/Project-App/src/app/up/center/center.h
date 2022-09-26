#ifndef __CENTER_H__
#define __CENTER_H__

#include "sendmode.h"
int center_process(SendMode*s,UDPacket*packet);

int get_shishi_all(u8*dataout);
int get_shiduan(u8*datain,int len,u8*dataout);
int get_shiduan_all(u8*dataout,time_t begin,time_t end,time_t step,u8 id);
int get_zhidingids_shishi(u8*dataout,u8*ids,int count);
int get_zhiding_ids(u8*datain,int len,u8*dataout);
int set_param(u8*datain ,int size,u8*dataout);
int get_param(u8*datain ,int size,u8*dataout);
int set_runtime(u8*datain ,int size,u8*dataout);
int get_runtime(u8*datain ,int size,u8*dataout);
int get_shishi_motor(u8*dataout);
int get_version(u8*dataout);
int get_state_warn_info(u8*dataout);
int clear_history(u8*datain,int len,u8*dataout);
int reset_to_factory(u8*datain,int len,u8*dataout);
int change_password(u8*datain,int len,u8*dataout);
int set_time(u8*datain,int len,u8*dataout);
int get_time(u8*dataout);
int set_IC_state(u8*datain,int len,u8*dataout);
int set_moter_onoff(u8*datain,int len,u8*dataout);
int set_famen_onoff(u8*datain,int len,u8*dataout);
int set_zhamen_onoff(u8*datain,int len,u8*dataout);
int set_shuiliang_control(u8*datain,int len,u8*dataout);
int get_event_state(u8*datain,int len,u8*dataout);
#endif
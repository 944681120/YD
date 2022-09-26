#pragma once 
#ifndef __SERIALPORT_H__
#define __SERIALPORT_H__


//串口时钟头文件
#include <signal.h>
#include <time.h>

//线程， 信号量头文件
#include <pthread.h>
#include <semaphore.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "lib.h"

#define Send_Rec_Buffer_Len_Max 109 //数据包最大长度

typedef struct
{
  u8 finish_flag;
  u16 data_len;
  u16 temp_ptr;
  u8 data_buffer[Send_Rec_Buffer_Len_Max]; //避免接收处理延迟导致的过长问题，扩充接收缓冲区到2倍长度
} Send_Rec_Data_Structure;

typedef struct
{
  u8 finish_flag;
  u16 data_len;
  u16 temp_ptr;
  u8 data_buffer[Send_Rec_Buffer_Len_Max *2]; //避免接收处理延迟导致的过长问题，扩充接收缓冲区到2倍长度
} Send_Rec_Extend_Structure;

//串口接收缓冲区地址链表结构体
typedef struct
{
  Send_Rec_Data_Structure *ptr;
  u8 finish_flag;
  sem_t sem_t_buf;
} Send_Rec_Data_Port_Structure;

//定时器结构体声明
typedef struct
{
  struct sigevent evp;
  struct itimerspec ts;
  timer_t timer;
  int ret;
} Timer_Irq_Structure;

class SerialPort
{
private:
public:    
    SerialPort();
    ~SerialPort();

    pthread_t thread_id = -1;
    const char*COM;
    int baudrate=9600;
    void*user_data = NULL;
    u8 uart_init_flag = 0;
    int frame_timeout_us = 20000;
    packet_process *packet;             //packet process for read data
    Send_Rec_Extend_Structure uart_receive;  
    Send_Rec_Data_Structure uart_send;      //串口发送缓冲区 
    Send_Rec_Data_Port_Structure uart_rec_ptr_form[65535]; //串口接收数据表指针  
    void init_uart_bus_select(void); 

    int fd = -1;
    int rec_len_temp = 0;
    long   receive_timeout;
    long   packet_timeout = 50;//50ms

    //set packet process
    int set_packet_process(void* func,int frame_timeout);

    //change bandrate
    int change_speed(int rate);
    //start
    pthread_t start(const char *__path,int b);

    //send
    int send(u8*data,int len);
    
    //close
    int quit(void);

    bool is_send_end(void);

    int ctrl(int value);

};






#endif
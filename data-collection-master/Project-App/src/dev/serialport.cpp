#include "serialport.h"
#include "lib.h"
#include "termios.h"
#include "fcntl.h"
#include "unistd.h"

static int UART0_Set(int fd, int speed, int flow_ctrl, int databits, int stopbits, int parity)
{
    int ret;
    int i;
    int speed_arr[] = {B115200, B19200, B9600, B4800, B2400, B1200, B300};
    int name_arr[] =  { 115200,  19200,  9600,  4800,  2400,  1200,  300};
    struct termios options;

    ret = tcgetattr(fd, &options);
    if (ret != 0) {
        printf("Setup Serial failed");
        return -1;
    }

    /* 设置输入波特率和输出波特率 */
    for (i = 0; i < sizeof(speed_arr) / sizeof(speed_arr[0]); i++) {
        if (speed == name_arr[i]) {
            cfsetispeed(&options, speed_arr[i]);
            cfsetospeed(&options, speed_arr[i]);
            break;
        }
    }

    /* 修改控制模式，保证程序不会占用串口 */
    options.c_cflag |= CLOCAL;

    /* 修改控制模式，使得能够从串口中读取输入数据 */
    options.c_cflag |= CREAD;

    /* 设置数据流控制 */
    switch (flow_ctrl) {
        case 0: /* 不使用流控制 */
            options.c_cflag &= ~CRTSCTS;
            break;
        case 1: /* 使用硬件流控制 */
            options.c_cflag |= CRTSCTS;
            break;
        case 2: /* 使用软件流控制 */
            options.c_cflag |= IXON | IXOFF | IXANY;
            break;
        default:
            printf("Unsupported flow_ctrl");
            return -1;
    }

    /* 设置数据位 */
    /* 屏蔽其他标志位 */
    options.c_cflag &= ~CSIZE;
    switch (databits) {
        case 7:
            options.c_cflag |= CS7;
            break;
        case 8:
            options.c_cflag |= CS8;
            break;
        default:
            printf("Unsupported data size");
            return -1;
    }

    /* 设置校验位 */
    switch (parity) {
        case 0: /* 无奇偶校验位 */
            options.c_cflag &= ~PARENB;
            options.c_iflag &= ~INPCK;
            break;
        case 1: /* 设置为奇校验 */
            options.c_cflag |= (PARODD | PARENB);
            options.c_iflag |= INPCK;
            break;
        case 2: /* 设置为偶校验 */
            options.c_cflag |= PARENB;
            options.c_cflag &= ~PARODD;
            break;
        case 3: /* 设置为空格 */
            options.c_cflag &= ~PARENB;
            options.c_cflag &= ~CSTOPB;
            break;
        default:
            printf("Unsupported parity");
            return -1;
    }

    /* 设置停止位 */
    switch (stopbits) {
        case 1:
            options.c_cflag &= ~CSTOPB;
            break;
        case 2:
            options.c_cflag |= CSTOPB;
            break;
        default:
            printf("Unsupported stop bits");
            return -1;
    }

    /* 修改输出模式，原始数据输出 */
    options.c_oflag &= ~OPOST;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    /* 设置等待时间和最小接收字符 */
    options.c_cc[VTIME] = 1; /* 读取一个字符等待1*(1/10)s */
    options.c_cc[VMIN] = 1;  /* 读取字符的最少个数为1 */

    /* 如果发生数据溢出，接收数据，但是不再读取 刷新收到的数据但是不读 */
    tcflush(fd, TCIFLUSH);

    /* 激活配置 (将修改后的termios数据设置到串口中） */
    ret = tcsetattr(fd, TCSANOW, &options);
    if (ret != 0) {
        printf("Com set error!");
        return -1;
    }

    return 0;
}


static int uart_init_select(const char *COM,int baudrate)
{
  int fd;
  struct termios opt;

  fd = open(COM, O_RDWR | O_NOCTTY | O_SYNC);
  if (fd == -1)
  {
    ERROR("[uart_init_select]: open %s error...",COM);
    return (-1);
  }
  
  UART0_Set(fd,baudrate,0,8,1,0);
  return (int)fd;
}
  
//   tcgetattr(fd, &opt);
//   bzero(&opt, sizeof(opt));
//   // tcflush(fd, TCIOFLUSH);
//   // cfsetispeed(&opt, B115200);
//   // cfsetospeed(&opt, B115200);
//   cfsetispeed(&opt, B9600);
//   cfsetospeed(&opt, B9600);
//   /*tcsetattr(fd,TCANOW,&Opt);//设置终端参数，TCANOW修改立即发生*/

//   opt.c_cflag &= ~CSIZE;
//   opt.c_cflag |= CS8;
//   opt.c_cflag &= ~CSTOPB; // 1位停止位 
//   opt.c_cflag &= ~PARENB;
//   opt.c_cflag &= ~CRTSCTS;
//   opt.c_cflag |= (CLOCAL | CREAD);

//   opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

//   opt.c_oflag &= ~OPOST;

//   opt.c_cc[VTIME] = 1; //设置超时为100ms，阻塞
//   opt.c_cc[VMIN] = 1;  //设置最小读取个数1个，阻塞

//   // tcflush(fd, TCIOFLUSH);
//   while (tcflush(fd, TCIOFLUSH) != 0) //等待清除缓冲区成功
//   {
//     usleep(1000);
//   }

//   if (tcsetattr(fd, TCSANOW, &opt) != 0)
//   {
//     ERROR("[uart_init_select]: open %s error...",COM);
//     return (-1);
//   }
//   INFO("[uart_init_select]: open %s OK...",COM);
//   return (fd);
// }


SerialPort::SerialPort()
{}
SerialPort::~SerialPort()
{}

void SerialPort::init_uart_bus_select(void)
{
  while ((this->fd = uart_init_select(this->COM,this->baudrate)) == -1)
  {
    usleep(1000);
  } //初始化ttyS1作为串口总线

  // tcflush(uart_fd, TCIOFLUSH); //清空串口接受发送缓冲区
  bzero(&this->uart_receive, sizeof(this->uart_receive)); //清除接收缓冲区结构体数据
  bzero(&this->uart_rec_ptr_form,sizeof(this->uart_rec_ptr_form)); //清除接收缓冲区指针结构体数据
  this->uart_init_flag = 1;
}

int SerialPort::change_speed(int speed)
{
  int speed_arr[] = { B38400, B19200, B9600, B4800, B2400, B1200, B300,
                      B38400, B19200, B9600, B4800, B2400, B1200, B300, };
  int name_arr[] = {38400,  19200,  9600,  4800,  2400,  1200,  300,
                    38400,  19200,  9600, 4800, 2400, 1200,  300, };
  int   i;
  int   status;
  struct termios   Opt;
  tcgetattr(fd, &Opt);
  for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++)
   {
       if  (speed == name_arr[i])
       {
          tcflush(fd, TCIOFLUSH);
          cfsetispeed(&Opt, speed_arr[i]);
          cfsetospeed(&Opt, speed_arr[i]);
          status = tcsetattr(fd, TCSANOW, &Opt);
          if (status != 0){
            ERROR("[change speed] %s = %d  fail!",this->COM,speed);
            return - 2;
          }  
          INFO("[change speed] %s = %d  OK!",this->COM,speed);
       }
      tcflush(fd,TCIOFLUSH);
      
      return 0;
   }
   return -1;
}

int SerialPort::set_packet_process(void* func,int frame_timeout)
{
  this->packet = (packet_process*)func;
  this->frame_timeout_us = frame_timeout;
  return 1;
}

void* serialport_read_thread(void*arg);
pthread_t SerialPort::start(const char*com,int b)
{
  int ret;
  pthread_t id1;
  this->baudrate=b;
  this->COM = com;
  this->fd = uart_init_select(com,baudrate);
  ret = pthread_create(&id1, NULL, serialport_read_thread,this);
	if(ret)
	{
		ERROR("[线程创建]:ERR = serialport_read_thread");
		return 0;
	}

  INFO("[线程创建]:OK = serialport_read_thread");
  
  this->thread_id = id1;
  this->ctrl(1);
  return this->fd;
}

int SerialPort::quit(void)
{
  close(this->fd);
  this->fd = 0;
  if(this->thread_id){
    pthread_cancel(this->thread_id);
    this->thread_id = 0;
  }
  return 0;
}

int SerialPort::send(u8* data,int len)
{
  PRINTBYTES("%s =>>",data,len,this->COM);
  this->ctrl(0);
  usleep(1000);
  int result = write(this->fd, data, len);
  usleep(10000);
  this->ctrl(1);
  return result;
}

bool SerialPort::is_send_end(void){return true;}

void* serialport_read_thread(void*arg)
{
    SerialPort *s = (SerialPort*)arg;
    while(1)
    {
        //阻塞读取串口数据
        //针对openwrt linux 串口特性，取消占用资源大的串口中断方法
        //实际测试，串口设置为接收1个，100ms超时，实际接收最大长度为13，可通过设置接收缓冲区长度控制接收长度
        //数据操作需要处理粘包
      if ((s->rec_len_temp = read(s->fd, &s->uart_receive.data_buffer[s->uart_receive.temp_ptr],
              (sizeof(s->uart_receive.data_buffer) - s->uart_receive.temp_ptr))) > 0)
      {
        if (s->uart_receive.temp_ptr == 0)
        {
          s->receive_timeout = get_ms_clock() + s->packet_timeout;//更新延时
        }
        else if (s->receive_timeout < get_ms_clock())             //帧超时
        {
          s->uart_receive.temp_ptr = 0;
          s->rec_len_temp = 0;
          s->uart_receive.data_len = 7;
          s->receive_timeout = get_ms_clock() + s->packet_timeout;
          ERROR("[uart_rec_handler]: packet timeout = %ld...",get_ms_clock() - s->receive_timeout);
          //分析包数据，强制同步总线
        }
        s->uart_receive.temp_ptr += s->rec_len_temp; //更新缓冲区指针
        if(s->uart_receive.temp_ptr)
        {
          PRINTBYTES("%s <<=",s->uart_receive.data_buffer,s->uart_receive.temp_ptr,s->COM);
          void * arg = (s->user_data!=NULL)?(s->user_data):s;
          s->packet(arg,s->uart_receive.data_buffer,s->uart_receive.temp_ptr);
        }
        //数据处理后清理
        s->uart_receive.temp_ptr =0;
      }
    }

}

//value = 0 send
int SerialPort::ctrl(int value)
{
  char ctrlvalue = (char)value;
  const char *io = "/dev/rs485Ctrl1";
  if (strcmp(this->COM, "/dev/ttyRS2") == 0) 
  {
    io = "/dev/rs485Ctrl2";
  }
  /* 切换到发送状态 */
  int crlfd = open(io, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (crlfd < 0) {
      ERROR("[rs485Ctrl] :%s, open fail ",io);
      return -1;
  }

  int iDataLen = write(crlfd, &ctrlvalue, sizeof(ctrlvalue));
  if (iDataLen < 0) {
      ERROR("[rs485Ctrl] :%s, write=%d fail ",io,value);
      close(crlfd);
      return -1;
  }
  INFO("[rs485Ctrl] %s ,set = %d",io,value);
  close(crlfd);
  return 0;
}

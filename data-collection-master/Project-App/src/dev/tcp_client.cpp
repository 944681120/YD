
/*=================================================================
    tcp 客户端

    1. keep alive 时，一直在线

    2. keepalive=false:
        1)开机等待
        2)发送数据

===================================================================*/
#include "lib.h"
#include "tcp_client.h"
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>


void *tcp_receive_send_thread(void *arg);
void *tcp_receive_packet_thread(void *arg);

tcp_client::tcp_client() {}
tcp_client::~tcp_client() {}

bool tcp_client_isdisconnect_byserver(int fd)
{
    struct tcp_info info;
    int len = sizeof(info);
    getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len);
    if ((info.tcpi_state == TCP_ESTABLISHED))
    {
        return false;
    }
    else
        return true;
}

int tcp_client::config(const char *ip, int port)
{
    bzero(&this->targetAddr, sizeof(this->targetAddr));
    this->targetAddr.sin_family = AF_INET;
    this->targetAddr.sin_port = htons(port);
    this->targetAddr.sin_addr.s_addr = inet_addr(ip);
    memcpy(this->setip, ip, strlen(ip) < sizeof(this->setip) ? strlen(ip) : sizeof(this->setip));
    this->setip[sizeof(this->setip) - 1] = '\0';
    this->setport = port;
    return 0;
}

int tcp_client::start(void *arg)
{
    if (this->pfd)
        pthread_cancel(this->pfd);
    int ret = pthread_create(&this->pfd, NULL, tcp_receive_send_thread, this);
    if (ret)
    {
        INFO("[线程创建]:ERR = tcp_receive_send_thread");
        return -1;
    }
    INFO("[线程创建]:OK = tcp_receive_send_thread");
    if (this->ppfd)
        pthread_cancel(this->ppfd);
    ret = pthread_create(&this->ppfd, NULL, tcp_receive_packet_thread, this);
    if (ret)
    {
        INFO("[线程创建]:ERR = tcp_receive_packet_thread");
        return -1;
    }
    INFO("[线程创建]:OK = tcp_receive_packet_thread");
    this->step = TCP_STEP_OPEN;
    return this->pfd;
}

int tcp_client::quit(void)
{
    if (this->pfd)
        pthread_cancel(this->pfd);
    if (this->socketfd)
        close(this->socketfd);
    if (this->ppfd)
        pthread_cancel(this->ppfd);
    this->pfd = 0;
    this->socketfd = 0;
    return 0;
}

int tcp_client::keepalive(bool k)
{
    this->keep = k;
    return k;
}

int tcp_client::senddata(void *buffer, int len, int to_txms, int to_rxms, int recv_wann)
{

    if (len > sizeof(this->sendbuffer))
        len = sizeof(this->sendbuffer);
    memcpy(this->sendbuffer, buffer, len);
    this->sendlen = len;
    if (recv_timeout < 0)
        this->keepalive(true);
    else
        this->recv_timeout_ms_max = to_rxms;
    if (to_txms != 0)
        this->send_timeout_ms_max = to_txms;
    this->recvlen = 0;
    this->recvwannlen = recv_wann;

    if (this->step == TCP_STEP_RECV_WAIT)
        this->step = TCP_STEP_SEND;
    INFO("[senddata]:len=%d, step=%d", len, this->step);
    return len;
}

int tcp_client::set_packet_process(void *func, int frame_timeout)
{
    this->packet = (packet_process *)func;
    return 1;
}

bool tcp_client::is_send_end(void) { return (this->sendlen > 0) ? false : true; }

void tcp_client::tcp_state_function(void)
{
    if (this->lstep != this->step)
    {
        // printf("[tcp_state_function][step][MSG]:step=%d",this->step);
        this->lstep = this->step;
    }
    switch (this->step)
    {
    case TCP_STEP_NULL:
    {
        this->opened = false;
        if (this->socketfd > 0)
        {
            close(this->socketfd);
            this->socketfd = 0;
        }
    }
    break;
    // 1.new a socket
    case TCP_STEP_OPEN:
    {
        this->opened = false;
        //使用socket()创建套接字
        if ((this->socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            this->step = TCP_STEP_OPEN_ERROR;
        }
        else
        {
            //设置socket为非阻塞
            int iocttl_block = 1;
            ioctl(this->socketfd, FIONBIO, &iocttl_block); // 1:非阻塞 0:阻塞
            this->step = TCP_STEP_CONNECT;
            this->connect_timeout = get_ms_clock() + this->connect_timeout_ms_max;
            INFO("[tcp_state_function]: connecting......... to:%s:%d", this->setip, this->setport);
        }
        break;
    }
    // 2. connect
    case TCP_STEP_CONNECT:
    {
        //使用connect()函数来配置套接字，建立一个与TCP服务器的连接
        if (connect(this->socketfd, (struct sockaddr *)&this->targetAddr, sizeof(this->targetAddr)) == -1)
        {
            if (get_ms_clock() > this->connect_timeout)
            {
                this->step = TCP_STEP_OPEN_ERROR;
                ERROR("[tcp_state_function]:tcp connect is timeout, stop connect server ...");
            }
        }
        else
        {
            this->step = TCP_STEP_OPEN_OK;
            INFO("[tcp_state_function]:connect to:%s:%d", this->setip, this->setport);
        }
        break;
    }
    // 3. 链接成功
    case TCP_STEP_OPEN_OK:
    {
        this->opened = true;
        this->send_timeout = get_ms_clock() + this->send_timeout_ms_max;
        this->step = TCP_STEP_SEND;
        break;
    }
    case TCP_STEP_OPEN_ERROR:
    {
        this->reconnect++;
        this->step = TCP_STEP_OPEN;
    }
    break;

    // 4. 发送数据
    case TCP_STEP_SEND:
    {
        int sendOrReadBytes = 0;
        if (tcp_client_isdisconnect_byserver(this->socketfd))
        {
            ERROR("[tcp_state_function]:TCP_STEP_SEND, disconnect by server");
            this->step = TCP_STEP_SEND_ERROR;
            break;
        }
        //无数据
        if (this->sendlen == 0)
        {
            if (this->recv_timeout == 0 && this->keep == false)
            {
                this->recv_timeout = get_ms_clock() + this->recv_timeout_ms_max;
            }
            this->step = TCP_STEP_RECV_WAIT;
            break;
        }

        INFO("[tcp_client]: send data. sendreconnect:%d,len = %d :", this->sendreconnect, this->sendlen);
        PRINTBYTES(">>发送数据", this->sendbuffer, this->sendlen);
        //发送数据
        // signal(SIGPIPE, SIG_IGN); //关闭 pipe broken
        if ((this->sendlen) > 0 && ((sendOrReadBytes = send(this->socketfd, this->sendbuffer, this->sendlen, 0)) == -1))
        {
            if (get_ms_clock() > this->send_timeout)
            {
                this->sendreconnect++;
                if (this->sendreconnect > 2)
                {
                    this->sendlen = 0;
                }
                this->step = TCP_STEP_SEND_ERROR;
                INFO("[tcp_state_function]: send msg is timeout, close tcp connect ...");
            }
        }
        else
        {
            this->recv_timeout = get_ms_clock() + this->recv_timeout_ms_max;
            this->recvlen = 0;
            this->step = TCP_STEP_RECV_WAIT;
            this->sendlen = 0;
            this->opened = true;
        }
        break;
    }
    // 5.接收
    case TCP_STEP_RECV_WAIT:
    {
        int sendOrReadBytes = 0;
        if (this->sendlen > 0)
        {
            this->sendreconnect = 0;
            this->step = TCP_STEP_SEND;
            this->send_timeout = get_ms_clock() + this->send_timeout_ms_max;
            break;
        }

        if (tcp_client_isdisconnect_byserver(this->socketfd))
        {
            INFO("[tcp_state_function]:TCP_STEP_RECV_WAIT, disconnect by server");
            this->step = TCP_STEP_SEND_ERROR;
            break;
        }

        // 1. 接收分    1）一直在线等待数据    2）等待超时关闭返回
        if (this->recvwannlen == 0 && this->recvlen == 0)
        {
            this->recvwannlen = sizeof(this->recvbuffer);
        }
        signal(SIGPIPE, SIG_IGN); //关闭 pipe broken
        if ((sendOrReadBytes = recv(this->socketfd, this->recvbuffer + this->recvlen, this->recvwannlen - this->recvlen, 0)) <= 0)
        {
            if (get_ms_clock() > this->recv_timeout)
            {
                if (this->keep == false)
                {
                    this->step = TCP_STEP_RECV_TIMEOUT;
                    INFO("[tcp_state_function] : tcp recv msg is timeout, close tcp connect ...");
                }
                this->recvlen = 0;
                this->recv_timeout = get_ms_clock() + this->recv_timeout_ms_max;
            }
        }
        else
        {
            INFO("[tcp_state_function]:recv:%d :", sendOrReadBytes);
            PRINTBYTES("<<接受数据", this->recvbuffer + this->recvlen, sendOrReadBytes);
            this->recv_timeout = get_ms_clock() + this->recv_timeout_ms_max;
            this->recvlen += sendOrReadBytes; //第一段数据包含数据包长度
            //根据接收到的长度判断是否接收完成
            if (this->user_data != NULL)
                user = this->user_data;
            else
                user = this;

            this->ppfd_get = true;
            // if(this->packet!=NULL)
            //     result =  this->packet(user,this->recvbuffer,this->recvlen);

            if (this->recvlen >= this->recvwannlen)
            {
                this->recvbuffer[this->recvlen] = '\0'; //补充空字符
                this->recv_timeout = get_ms_clock() + this->recv_timeout_ms_max;
                // this->step = TCP_STEP_RECV_GET_OK;
            }
        }
        break;
    }
    break;
    case TCP_STEP_RECV_GET_OK:
    {
        break;
    }

    //超时重连
    case TCP_STEP_SEND_ERROR:
    case TCP_STEP_RECV_TIMEOUT:
    {
        // printf("[tcp_receive_send_thread][Err]:sendretry:%d,sendlen:%d ",sendreconnect,sendlen);
        close(this->socketfd);
        this->socketfd = 0;
        this->opened = false;
        if (this->reconnect < 65535)
            this->reconnect++;
        //重连
        if (this->keep || this->sendlen > 0)
            this->step = TCP_STEP_OPEN;
        else
            this->step = TCP_STEP_NULL;
        INFO("[tcp_state_function]:reconnect.");
        break;
    }
    default:
    {
        this->step = TCP_STEP_NULL;
    }
    }
}

/*=================================================================
    tcp 线程
===================================================================*/
void *tcp_receive_send_thread(void *arg)
{
    int counter = 0;
    tcp_client *c = (tcp_client *)arg;
    INFO("[tcp_receive_send_thread]: is running");

    while (1)
    {
        c->tcp_state_function();
        usleep(1000);
        // printf("tcp_receive_send_thread:step=%d",c->step);
        // if(counter++>100)
        {
            static int step = 0;
            if (step != c->step)
            {
                step = c->step;
                INFO("[tcp_state_function]: step=%d", c->step);
            }
            counter = 0;
        }
    }
}

void *tcp_receive_packet_thread(void *arg)
{
    tcp_client *c = (tcp_client *)arg;
    // INFO("[线程创建]:OK =
    INFO("[tcp_receive_packet_thread]: is running");
    while (1)
    {
        if (c->ppfd_get)
        {
            c->ppfd_get = false;
            if (c->packet != NULL)
                if (c->packet(c->user, c->recvbuffer, c->recvlen) != NULL)
                {
                    c->recv_timeout = get_ms_clock() + c->recv_timeout_ms_max;
                }
            c->recvlen = 0;
        }
        usleep(1000);
    }
}

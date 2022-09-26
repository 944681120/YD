
// socket头文件
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h> // for sockaddr_in
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h> // for socket
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "ftpclient.h"
#include "malloc.h"


#define SPC_ERROR_OUT printf

//创建一个socket并返回
int FTPClient::socket_connect(const char *host, int port)
{
	struct sockaddr_in address;
	int s, opvalue;
	socklen_t slen;
 
	opvalue = 0x08;
	slen = sizeof(opvalue);
	memset(&address, 0, sizeof(address));
 
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0 ||
		setsockopt(s, IPPROTO_IP, IP_TOS, &opvalue, slen) < 0)
	{
		SPC_ERROR_OUT("socket or setsockopt create failed");
		return -1;
	}
	//设置接收和发送超时
	struct timeval timeo = { 15, 0 };
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo));
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo));
 
	address.sin_family = AF_INET;
	address.sin_port = htons((unsigned short)port);
 
	struct hostent* server = gethostbyname(host);
	if (!server)
	{
		SPC_ERROR_OUT("gethostbyname conversion failed");
		return -1;
	}
 
	memcpy(&address.sin_addr.s_addr, server->h_addr, server->h_length);
 
	if (connect(s, (struct sockaddr*) &address, sizeof(address)) == -1)
	{
		SPC_ERROR_OUT("connect host[%s] failed", host);
		return -1;
	}
 
	return s;
}
 
//连接到一个ftp的服务器，返回socket
int FTPClient::connect_server(const char *host, int port)
{
	int       ctrl_sock;
	char      buf[512];
	int       result;
	ssize_t   len;
 
	ctrl_sock = socket_connect(host, port);
	if (ctrl_sock == -1) {
		SPC_ERROR_OUT("socket_connect failed");
		return -1;
	}
 
	len = recv(ctrl_sock, buf, 512, 0);
	buf[len] = 0;
	sscanf(buf, "%d", &result);
	if (result != 220) {
		SPC_ERROR_OUT("ftp recv data failed");
		close(ctrl_sock);
		return -1;
	}
 
	return ctrl_sock;
}
 
//发送命令,返回结果
int FTPClient::ftp_sendcmd_re(int sock, const char *cmd, void *re_buf, ssize_t *len)
{
	char        buf[512];
	ssize_t     r_len;
 
	if (send(sock, cmd, strlen(cmd), 0) == -1)
	{
		SPC_ERROR_OUT("send failed");
		return -1;
	}
	r_len = recv(sock, buf, 512, 0);
	if (r_len < 1)
	{
		SPC_ERROR_OUT("recv failed");
		return -1;
	}
	buf[r_len] = 0;
 
	if (len != NULL) *len = r_len;
	if (re_buf != NULL) sprintf((char*)re_buf, "%s", buf);
 
	return 0;
}
 
//发送命令,返回编号
int FTPClient::ftp_sendcmd(int sock, const char *cmd)
{
	char     buf[512];
	int      result;
	ssize_t  len;
 
	result = ftp_sendcmd_re(sock, cmd, buf, &len);
	if (result == 0)
	{
		sscanf(buf, "%d", &result);
	}
 
	return result;
}
 
//登录ftp服务器
int FTPClient::login_server(int sock, const char *user, const char *pwd)
{
	char    buf[128];
	int     result;
 
	sprintf(buf, "USER %s\r\n", user);
	result = ftp_sendcmd(sock, buf);
	if (result == 230) return 0;
	else if (result == 331) {
		sprintf(buf, "PASS %s\r\n", pwd);
		if (ftp_sendcmd(sock, buf) != 230)
		{
			SPC_ERROR_OUT("ftp_sendcmd failed");
			return -1;
		}
		return 0;
	}
	else
	{
		SPC_ERROR_OUT("login server failed");
		return -1;
	}
}
 
int FTPClient::create_datasock(int ctrl_sock)
{
	int     lsn_sock;
	int     port;
	int     len;
	struct sockaddr_in sin;
	char    cmd[128];
 
	lsn_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (lsn_sock == -1) return -1;
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	if (bind(lsn_sock, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		close(lsn_sock);
		SPC_ERROR_OUT("bind sockaddr failed");
		return -1;
	}
 
	if (listen(lsn_sock, 2) == -1) {
		close(lsn_sock);
		SPC_ERROR_OUT("listen socket failed");
		return -1;
	}
 
	len = sizeof(struct sockaddr);
	if (getsockname(lsn_sock, (struct sockaddr *)&sin, (socklen_t *)&len) == -1)
	{
		close(lsn_sock);
		SPC_ERROR_OUT("getsockname failed");
		return -1;
	}
	port = sin.sin_port;
 
	if (getsockname(ctrl_sock, (struct sockaddr *)&sin, (socklen_t *)&len) == -1)
	{
		close(lsn_sock);
		SPC_ERROR_OUT("getsockname failed");
 
		return -1;
	}
 
	sprintf(cmd, "PORT %d,%d,%d,%d,%d,%d\r\n",
		sin.sin_addr.s_addr & 0x000000FF,
		(sin.sin_addr.s_addr & 0x0000FF00) >> 8,
		(sin.sin_addr.s_addr & 0x00FF0000) >> 16,
		(sin.sin_addr.s_addr & 0xFF000000) >> 24,
		port >> 8, port & 0xff);
 
	if (ftp_sendcmd(ctrl_sock, cmd) != 200) {
		close(lsn_sock);
		SPC_ERROR_OUT("ftp_sendcmd failed");
		return -1;
	}
	return lsn_sock;
}
 
//连接到PASV接口
int FTPClient::ftp_pasv_connect(int c_sock)
{
	int     r_sock;
	int     send_re;
	ssize_t len;
	int     addr[6];
	char    buf[512];
	char    re_buf[512];
 
	//设置PASV被动模式
	bzero(buf, sizeof(buf));
	sprintf(buf, "PASV\r\n");
	send_re = ftp_sendcmd_re(c_sock, buf, re_buf, &len);
	if (send_re == 0) {
		sscanf(re_buf, "%*[^(](%d,%d,%d,%d,%d,%d)", &addr[0], &addr[1], &addr[2], &addr[3], &addr[4], &addr[5]);
	}
 
	//连接PASV端口
	bzero(buf, sizeof(buf));
	sprintf(buf, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
	r_sock = socket_connect(buf, addr[4] * 256 + addr[5]);
 
	return r_sock;
}
 
//表示类型
int FTPClient::ftp_type(int c_sock, char mode)
{
	char    buf[128];
	sprintf(buf, "TYPE %c\r\n", mode);
	if (ftp_sendcmd(c_sock, buf) != 200)
		return -1;
	else
		return 0;
}
 
//改变工作目录
int FTPClient::ftp_cwd(int c_sock, const char *path)
{
	char    buf[128];
	int     re;
	sprintf(buf, "CWD %s\r\n", path);
	re = ftp_sendcmd(c_sock, buf);
	if (re != 250)
		return -1;
	else
		return 0;
}
 
//回到上一层目录
int FTPClient::ftp_cdup(int c_sock)
{
	int     re;
	re = ftp_sendcmd(c_sock, "CDUP\r\n");
	if (re != 250)
		return re;
	else
		return 0;
}
 
//创建目录
int FTPClient::ftp_mkd(int c_sock, const char *path)
{
	char    buf[512];
	int     re;
	sprintf(buf, "MKD %s\r\n", path);
	re = ftp_sendcmd(c_sock, buf);
	if (re != 257)
		return re;
	else
		return 0;
}
 
//列表
int FTPClient::ftp_list(int c_sock, const char *path, void **data, unsigned long long *data_len)
{
	#define LBUFFERSIZE 1024
	int     d_sock;
	char    buf[LBUFFERSIZE];
	int     send_re;
	int     result;
	ssize_t     len, buf_len, total_len;
 
	//连接到PASV接口
	d_sock = ftp_pasv_connect(c_sock);
	if (d_sock == -1) {
		return -1;
	}

	//发送ls命令
	bzero(buf, sizeof(buf));
	sprintf(buf, "LIST %s\r\n", path);
	send_re = ftp_sendcmd(c_sock, buf);
	if (send_re >= 300 || send_re == 0)
		return send_re;
 
	len = total_len = 0;
	buf_len = LBUFFERSIZE;
	void *re_buf = malloc(buf_len);
	while ((len = recv(d_sock, buf, LBUFFERSIZE, 0)) > 0)
	{
		if (total_len + len > buf_len)
		{
			buf_len *= 2;
			void *re_buf_n = malloc(buf_len);
			memcpy(re_buf_n, re_buf, total_len);
			free(re_buf);
			re_buf = re_buf_n;
		}
		char*b = (char*)((char*)re_buf + total_len);
		memcpy(b, buf, len);
		total_len += len;
	}
	close(d_sock);

	//向服务器接收返回值
	// bzero(buf, sizeof(buf));
	// len = recv(c_sock, buf, sizeof(buf)-10, 0);
	// if(len<=0)
	// 	printf("errno =%d\n",errno);
	// buf[len] = 0;
	// sscanf(buf, "%d", &result);
	// if (result != 226)
	// {
	// 	free(re_buf);
	// 	return result;
	// }
 
	*data = re_buf;
	*data_len = total_len;
 
	return 0;
}
 
int FTPClient::ftp_retrfile(int c_sock, char *s, char *d, unsigned long long *stor_size, int *stop)
{
	int     d_sock;
	ssize_t     len, write_len;
	char    buf[512];
	int     handle;
	int     result;
	int total=0;
 
	//打开本地文件
	handle = open(d, O_WRONLY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
	if (handle == -1) return -1;
 
	//设置传输模式
	ftp_type(c_sock, 'I');
 
	//连接到PASV接口
	d_sock = ftp_pasv_connect(c_sock);
	if (d_sock == -1)
	{
		SPC_ERROR_OUT("ftp pasv connect failed\n");
		close(handle);
		return -1;
	}
 
	//发送STOR命令
	bzero(buf, sizeof(buf));
	sprintf(buf, "RETR %s\r\n", s);
	result = ftp_sendcmd(c_sock, buf);
	if (result >= 300 || result == 0)
	{
		SPC_ERROR_OUT("ftp sendcmd failed\n");
		close(handle);
		return result;
	}
 
	//开始向PASV读取数据
	bzero(buf, sizeof(buf));
	printf("begin receive file.\n");
	while ((len = recv(d_sock, buf, 512, 0)) > 0) {
		write_len = write(handle, buf, len);
		if (write_len != len || (stop != NULL && *stop))
		{
			SPC_ERROR_OUT("PASV read data failed\n");
			close(d_sock);
			close(handle);
			return -1;
		}
        
		if (stor_size != NULL)
		{
			*stor_size += write_len;
			total += write_len;
			printf("receive %d bytes ,total=%d bytes.\n",(int)write_len,total);
		}
	}
	close(d_sock);
	close(handle);
 
	//向服务器接收返回值
	bzero(buf, sizeof(buf));
	len = recv(c_sock, buf, 512, 0);
	buf[len] = 0;
	sscanf(buf, "%d", &result);
	if (result >= 300) {
		SPC_ERROR_OUT("recv result[%d] failed", result);
		return result;
	}
	return 0;
}
 
//保存文件
int FTPClient::ftp_storfile(int c_sock, char *s, char *d, unsigned long long *stor_size, int *stop)
{
	int     d_sock;
	ssize_t     len, send_len;
	char    buf[512];
	int     handle;
	int send_re;
	int result;
 
	//打开本地文件
	handle = open(s, O_RDONLY);
	if (handle == -1) return -1;
 
	//设置传输模式
	ftp_type(c_sock, 'I');
 
	//连接到PASV接口
	d_sock = ftp_pasv_connect(c_sock);
	if (d_sock == -1)
	{
		SPC_ERROR_OUT("ftp pasv connect failed");
		close(handle);
		return -1;
	}
 
	//发送STOR命令
	bzero(buf, sizeof(buf));
	sprintf(buf, "STOR %s\r\n", d);
	send_re = ftp_sendcmd(c_sock, buf);
	if (send_re >= 300 || send_re == 0)
	{
		SPC_ERROR_OUT("ftp sendcmd failed");
		close(handle);
		return send_re;
	}
 
	//开始向PASV通道写数据
	bzero(buf, sizeof(buf));
	while ((len = read(handle, buf, 512)) > 0)
	{
		send_len = send(d_sock, buf, len, 0);
		if (send_len != len ||
			(stop != NULL && *stop))
		{
			close(d_sock);
			close(handle);
			return -1;
		}
 
		if (stor_size != NULL)
		{
			*stor_size += send_len;
		}
	}
	close(d_sock);
	close(handle);
 
	//向服务器接收返回值
	bzero(buf, sizeof(buf));
	len = recv(c_sock, buf, 512, 0);
	buf[len] = 0;
	sscanf(buf, "%d", &result);
	if (result >= 300) {
		SPC_ERROR_OUT("recv result[%d] failed", result);
		return result;
	}
	return 0;
}
 
//修改文件名&移动目录
int FTPClient::ftp_renamefile(int c_sock, const char *s, const char *d)
{
	char    buf[512];
	int     re;
 
	sprintf(buf, "RNFR %s\r\n", s);
	re = ftp_sendcmd(c_sock, buf);
	if (re != 350)
	{
		SPC_ERROR_OUT("ftp sendcmd[%d] failed", re);
		return re;
	}
	sprintf(buf, "RNTO %s\r\n", d);
	re = ftp_sendcmd(c_sock, buf);
	if (re != 250)
	{
		SPC_ERROR_OUT("ftp sendcmd[%d] failed", re);
		return re;
	}
	return 0;
}
 
//删除文件
int FTPClient::ftp_deletefile(int c_sock, const char *s)
{
	char    buf[512];
	int     re;
 
	sprintf(buf, "DELE %s\r\n", s);
	re = ftp_sendcmd(c_sock, buf);
	if (re != 250)
	{
		return re;
		SPC_ERROR_OUT("ftp sendcmd[%d] failed", re);
	}
	return 0;
}
 
//删除目录
int FTPClient::ftp_deletefolder(int c_sock, const char *s)
{
	char    buf[512];
	int     re;
 
	sprintf(buf, "RMD %s\r\n", s);
	re = ftp_sendcmd(c_sock, buf);
	if (re != 250)
	{
		SPC_ERROR_OUT("ftp sendcmd[%d] failed", re);
		return re;
	}
 
	return 0;
}
 
 
//链接服务器
int FTPClient::ftp_connect(const char *host, int port, const char *user, const char *pwd)
{
	int     c_sock;
	c_sock = connect_server(host, port);
	if (c_sock == -1) return -1;
	if (login_server(c_sock, user, pwd) == -1) {
		SPC_ERROR_OUT("login server failed");
		close(c_sock);
		return -1;
	}
	return c_sock;
}
 
//断开服务器
int FTPClient::ftp_quit(int c_sock)
{
	int re = 0;
	re = ftp_sendcmd(c_sock, "QUIT\r\n");
	close(c_sock);
	return re;
}



//测试代码
#include <regex.h>
void ftp_test()
{
	const char* ip = "192.168.3.223";
	const char* user = "kaihua";
	const char* password = "111111";
	FTPClient cftp;
	void*data;
	unsigned long long datalen; 

    printf("begin to connect ftp .\r\n");
	int fd = cftp.ftp_connect(ip, 21, user , password);
	if (fd < 0)
	{
		printf("ftp connect falied\n");
	}

	usleep(50000);
	
	//CFTPClinet.ftp_mkd(fd, "/tmp/test");
	printf("begin to list ftp .\r\n");
	int result = cftp.ftp_list(fd,"ftp",&data,&datalen);

	if(result!=0)
	{
		printf("ftp server does not ack.\r\n");
		cftp.ftp_quit(fd);
		return ;
	}
	printf("len = %d ,data is :%s",(int)datalen,(const char*)data);

	//app-cjq_V1.05_220427_15.51.36.deb
	if(datalen>1)
	{
		const char* d = (const char*)data;
		regex_t re_sp;
		int ret =  regcomp(&re_sp,"app-cjq_V[0-9._]*deb",REG_EXTENDED);
		regmatch_t regmatch[20];
		//读取版本
		int status = regexec(&re_sp,(const char*) data, 1, regmatch, 0);
		if (status == 0) {
			printf("match it\r\n");
			int i = 0;
			char aa[100];
			strncpy(aa, (const char*)(d+regmatch[i].rm_so), regmatch[i].rm_eo - regmatch[i].rm_so);
			aa[regmatch[i].rm_eo-regmatch[i].rm_so]='\0';
			printf("match string is: %s\r\n",aa);
		} else if (status == REG_NOMATCH) {
			printf("not match it\r\n");
		} 
		
		free(data);
	}
	cftp.ftp_quit(fd);
}
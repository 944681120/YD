#ifndef __FTPCLIENT_H__
#define __FTPCLIENT_H__


class FTPClient
{
public:
	FTPClient()
	{
		m_port = 21;
	}
	~FTPClient() {}

	//链接服务器
	int ftp_connect(const char *host, int port, const char *user, const char *pwd);
	//断开服务器
	int ftp_quit(int c_sock);
 
	//设置表示类型
	int ftp_type(int c_sock, char mode);
 
	//改变工作目录
	int ftp_cwd(int c_sock, const char *path);
	//回到上一层目录
	int ftp_cdup(int c_sock);
	//创建目录
	int ftp_mkd(int c_sock, const char *path);
	//列表
	int ftp_list(int c_sock, const char *path, void **data, unsigned long long *data_len);
 
	//下载文件
	int ftp_retrfile(int c_sock, char *s, char *d, unsigned long long *stor_size, int *stop);
	//上传文件
	int ftp_storfile(int c_sock, char *s, char *d, unsigned long long *stor_size, int *stop);
 
	//修改文件名&移动目录
	int ftp_renamefile(int c_sock, const char *s, const char *d);
	//删除文件
	int ftp_deletefile(int c_sock, const char *s);
	//删除目录
	int ftp_deletefolder(int c_sock, const char *s);
	//获取端口号
	inline int getPort() const
	{
		return m_port;
	}
private:
	//创建一个socket并返回
	int socket_connect(const char *host, int port);
	//连接到一个ftp的服务器，返回socket
	int connect_server(const char *host, int port);
	//发送命令,返回结果
	int ftp_sendcmd_re(int sock, const char *cmd, void *re_buf, ssize_t *len);
	//发送命令,返回编号
	int ftp_sendcmd(int sock, const char *cmd);
	//登录ftp服务器
	int login_server(int sock, const char *user, const char *pwd);
	int create_datasock(int ctrl_sock);
	//连接到PASV接口
	int ftp_pasv_connect(int c_sock);
 
 
private:
	int m_port;
 
};

extern 	//测试代码
	void ftp_test();
#endif


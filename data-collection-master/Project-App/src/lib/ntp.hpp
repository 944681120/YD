
#pragma once

#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h> // for sockaddr_in
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h> // for socket

#include "lib.h"
using namespace std;
string _default_ntp_server = "ntp.aliyun.com";
string _default_ntp_port = "123";
class ntp_ex
{
private:
	string _server = "ntp.aliyun.com";
	string _port = "123";

	int send_timeout_s = 5;
	int recv_timeout_s = 5;
	int64_t trys(bool set_to_system);

public:
	ntp_ex(string server, string port);
	ntp_ex(string server, string port, int to);
	~ntp_ex();

	int64_t get(bool set_to_system)
	{
		int cyc = 3;
		int64_t result = 0;
		while (cyc > 0)
		{
			cyc--;
			result = trys(set_to_system);
			if (result > 0)
				return result;
		}
		ERROR("对时失败");
		return 0;
	}

	int64_t ntp()
	{
		// 1. 使用配置对时
		int64_t r = get(true);

		// 2.失败时，使用默认配置对时
		if (r <= 0)
		{
			ERROR("使用默认配置对时:%s:%s", _default_ntp_server.c_str(), _default_ntp_port.c_str());
			_server = _default_ntp_server;
			_port = _default_ntp_port;
		}
		else
			return r;

		return get(true);
	}
};

ntp_ex::ntp_ex(string s, string p)
{
	_server = s;
	_port = p;
}
ntp_ex::ntp_ex(string s, string p, int to)
{
	_server = s;
	_port = p;
	send_timeout_s = recv_timeout_s = to;
}
ntp_ex::~ntp_ex() {}

int64_t ntp_ex::trys(bool set_to_system)
{
	const char *server = _server.c_str();
	const char *port = _port.c_str();

	INFO("对时 %s:%s", server, port);

	//初始化网络库
	//下面是获取过程
	int64_t curTm = 0;
	struct hostent *host = NULL;
	int nErrno = 0;

	addrinfo hints, *res, *ap; /* address info structs */
	socklen_t addrlen = sizeof(struct sockaddr_storage);

	struct ntpPacket
	{
		uint8_t flags;
		uint8_t stratum;
		uint8_t poll;
		uint8_t precision;
		uint32_t root_delay;
		uint32_t root_dispersion;
		uint8_t referenceID[4];
		uint32_t ref_ts_sec;
		uint32_t ref_ts_frac;
		uint32_t origin_ts_sec;
		uint32_t origin_ts_frac;
		uint32_t recv_ts_sec;
		uint32_t recv_ts_frac;
		uint32_t trans_ts_sec;
		uint32_t trans_ts_frac;
	};
#define ENDIAN_SWAP32(data) ((data >> 24) |				  /* right shift 3 bytes */ \
							 ((data & 0x00ff0000) >> 8) | /* right shift 1 byte */  \
							 ((data & 0x0000ff00) << 8) | /* left shift 1 byte */   \
							 ((data & 0x000000ff) << 24)) /* left shift 3 bytes */

	struct ntpPacket packet;
	uint8_t *ptr = (uint8_t *)(&packet); /* to read raw bytes */

	int server_sock; /* send through this socket */
	int error;		 /* error checking */
	unsigned int recv_secs;

	/* server is required, port is optional */
	memset(&packet, 0, sizeof(struct ntpPacket));
	packet.flags = 0xe3;

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_DGRAM;

	/* fill our address structs for ntp server */
	error = getaddrinfo(server, port, &hints, &res);
	/* loop through results */
	for (ap = res; ap != NULL; ap = ap->ai_next)
	{
		server_sock = socket(ap->ai_family, ap->ai_socktype, ap->ai_protocol);
		if (server_sock == -1)
			continue;
		break;
	}
	if (ap == NULL)
	{
		return curTm;
	}

	//发送超时
	struct timeval timeout;
	timeout.tv_sec = send_timeout_s;
	timeout.tv_usec = 0;

	if (setsockopt(server_sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
	{
		ERROR("设置对时发送超时失败.");
		return 0;
	}
	timeout.tv_sec = recv_timeout_s;
	if (setsockopt(server_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
	{
		ERROR("设置对时接收超时失败.");
		return 0;
	}

	error = ::sendto(server_sock, (const char *)&packet, sizeof(struct ntpPacket), 0, ap->ai_addr, addrlen);
	if (error == -1)
	{
		return curTm;
	}
	error = recvfrom(server_sock, (char *)&packet, sizeof(struct ntpPacket), 0, ap->ai_addr, &addrlen);
	if (error == -1)
	{
		return curTm;
	}

	freeaddrinfo(res); /* all done */

	/* correct for right endianess */
	packet.recv_ts_sec = ENDIAN_SWAP32(packet.recv_ts_sec);

	/* print date with receive timestamp */
	recv_secs = packet.recv_ts_sec - 2208988800L; /* convert to unix time */
	curTm = recv_secs;

	if (set_to_system)
	{
		TimeSet((time_t)curTm);
	}

	return curTm;
}

void *ntp_thread(void *arg)
{
	string s, p;
	int t;
	if (rtu.service.ntp.server == "")
		s = _default_ntp_server;
	else
		s = rtu.service.ntp.server;

	if (rtu.service.ntp.port == 0)
		p = _default_ntp_port;
	else
		p = to_string(rtu.service.ntp.port);
	if (rtu.service.ntp.timeout <= 0)
		t = 5;
	else
		t = rtu.service.ntp.timeout;

	ntp_ex n = ntp_ex(s, p, t);
	n.ntp();
}
int ntp(void)
{
	if (rtu.service.ntp.interval > 0)
	{
		static time_t time_now = 0;
		bool n = false;
		if (time_now == 0)
		{
			time_now = time(0);
			n = true;
		}
		if (time_now < (time(0) - rtu.service.ntp.interval))
		{
			time_now += rtu.service.ntp.interval;
			n = true;
		}

		if (n)
		{
			pthread_t id;
			if (pthread_create(&id, NULL, ntp_thread, NULL) != 0)
			{
				ERROR("[线程创建]:ERR = ntp_thread");
				return 0;
			}
			return id;
		}
	}
	return 0;
}
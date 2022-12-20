
#include "limits.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <sys/syslog.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include "lib.h"

// CRC校验计算函数
u16 crc_calculate(u8 *check_buffer, int len)
{
	int i, j, check_len;
	u16 crc_result_u16 = 0xffff;
	check_len = len;
	for (i = 0; i < check_len; i++)
	{
		crc_result_u16 = crc_result_u16 ^ check_buffer[i];
		for (j = 0; j < 8; j++)
		{
			if (crc_result_u16 & 0x01)
				crc_result_u16 = (crc_result_u16 >> 1) ^ 0xA001;
			else
				crc_result_u16 = crc_result_u16 >> 1;
		}
	}
	return crc_result_u16;
}

u16 bytehex2bcd(u8 hex)
{
	u16 r = hex / 100;
	r <<= 4;
	r |= (hex % 100) / 10;
	r <<= 4;
	r |= hex % 10;
	return r;
}

// 大小端转换
int little2big(u8 *little, u8 *big, int size)
{
	int len = size;
	while (size > 0)
	{
		*big = little[size - 1];
		big++;
		size--;
	}
	return len;
}
// 1234 [0]=(1234)低字节  [1]=(1234)高字节  return:0x1234
ul64 hex2bcd(u8 *hex, int bytes)
{
	ul64 r = 0, d = 0;
	int i = 0, j = 1;
	if (bytes > sizeof(d))
		bytes = sizeof(d);
	memcpy((void *)&d, hex, bytes);
	while (i < 16)
	{
		r |= (d % 10) << (4 * i);
		d /= 10;
		i++;
	}
	return r;
}

// 0x1234 d[0]=0x34 d[1]=0x12  return 1234
ul64 bcd2hex(u8 *d, int bytes)
{
	ul64 r = 0;
	int i = 0;
	while (i < bytes)
	{
		r *= 100;
		r += (d[bytes - 1 - i] >> 4) * 10 + (d[bytes - 1 - i] & 0x0F);
		i++;
	}
	return r;
}

// 0x1234 d[0]=0x12 d[1]=0x34  return 1234
ul64 bigbcd2hex(u8 *d, int bytes)
{
	ul64 r = 0;
	int i = 0;
	while (i < bytes)
	{
		r *= 100;
		r += (d[i] >> 4) * 10 + (d[i] & 0x0F);
		i++;
	}
	return r;
}

int hex2bigbcd(u8 *d, int bytes)
{
	ul64 dd = hex2bcd(d, bytes);
	blreverse(&dd, d, bytes);
	return bytes;
}

#include <time.h>
long get_ms_clock(void)
{
	long ms = 0; // Milliseconds
	struct timespec spec;
	clock_gettime(CLOCK_MONOTONIC, &spec);
	ms += spec.tv_sec * 1000 + spec.tv_nsec / 1.0e6;
	return ms;
}

// //当前 YYMMDDHHmmSS  bcd码
// ul64 get_timenow_bcd(void)
// {
//   ul64 YYMMDDHHmmSS = 0;
//   time_t tm;
//   struct tm *ptm;
//  timestd&tm);
//   ptm = localtime(&tm);
//   YYMMDDHHmmSS |= 0xFF&&bin2bcd(ptm->tm_year+1900);YYMMDDHHmmSS<<=8;
//   YYMMDDHHmmSS |= 0xFF&&bin2bcd(ptm->tm_mon+1    );YYMMDDHHmmSS<<=8;
//   YYMMDDHHmmSS |= 0xFF&&bin2bcd(ptm->tm_day      );YYMMDDHHmmSS<<=8;
//   YYMMDDHHmmSS |= 0xFF&&bin2bcd(ptm->tm_hour     );YYMMDDHHmmSS<<=8;
//   YYMMDDHHmmSS |= 0xFF&&bin2bcd(ptm->tm_min      );YYMMDDHHmmSS<<=8;
//   YYMMDDHHmmSS |= 0xFF&&bin2bcd(ptm->tm_sec      );return YYMMDDHHmmSS;
// }

void TimeSet(int year, int month, int day, int hour, int min, int sec)
{
	struct tm tptr;
	struct timeval tv;

	INFO("设置系统时间:%04d-%02d-%02d %02d:%02d:%02d",
		 year, month, day,
		 hour, min, sec);

	tptr.tm_year = year - 1900;
	tptr.tm_mon = month - 1;
	tptr.tm_mday = day;
	tptr.tm_hour = hour;
	tptr.tm_min = min;
	tptr.tm_sec = sec;

	tv.tv_sec = mktime(&tptr);
	tv.tv_usec = 0;
	settimeofday(&tv, NULL);
}
void TimeSet(string str)
{
	time_t t = string2time(str);
	struct tm *ptm = localtime(&t);
	TimeSet(ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
}

void TimeSet(time_t t)
{
	struct tm *ptm = localtime(&t);
	TimeSet(ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
}

void inverse(u8 *data, int len)
{
	u8 tmp = 0;
	int i = 0;
	for (i = 0; i < len / 2; i++)
	{
		tmp = data[i];
		data[i] = data[len - 1 - i];
		data[len - 1 - i] = tmp;
	}
}
void inverseto(u8 *data, u8 *dataout, int len)
{
	u8 tmp = 0;
	int i = 0;
	for (i = 0; i < len; i++)
	{
		dataout[i] = data[len - 1 - i];
	}
}

string tostring16(l64 v)
{
	std::stringstream stream;
	stream << std::hex << v;
	return stream.str();
}

char *get_cur_time()
{
	static char s[32] = {0};
	time_t t;
	struct tm *ltime;
	struct timeval stamp;
	gettimeofday(&stamp, NULL);
	ltime = localtime(&stamp.tv_sec);
	s[0] = '[';
	strftime(&s[1], 20, "%Y-%m-%d %H:%M:%S", ltime);
	sprintf(&s[strlen(s)], ".%03ld]", (stamp.tv_usec / 1000));
	return s;
}

char *get_time(time_t t)
{
	static char s[32] = {0};
	struct tm *tt = localtime((const time_t *)&t);
	strftime(&s[0], 20, "%Y-%m-%d %H:%M:%S", tt);
	return s;
}

// string to hex array
int str2hexarray(const char *str, int slen, char *dataout, int outsize)
{
	int s = outsize > (slen / 2) ? slen / 2 : outsize;
	int i = 0;
	u8 h, l;
	for (i = 0; i < s; i++)
	{
		h = str[i * 2];
		l = str[i * 2 + 1];
		l = (l < 'A') ? (l - '0') : (l + 10 - 'A');
		h = (h < 'A') ? (h - '0') : (h + 10 - 'A');
		dataout[i] = ((h << 4) & 0xF0) | (l & 0x0F);
	}
	return s;
}

int hexarray2str(char *data, int size, char *str, int slen)
{
	const char *t = "0123456789ABCDEF";
	int s = size > (slen / 2) ? slen / 2 : size;
	int i = 0;
	for (i = 0; i < s; i++)
	{
		str[i * 2] = t[(data[i] >> 4) & 0x0F];
		str[i * 2 + 1] = t[data[i] & 0x0F];
	}
	return s * 2;
}
string hexarray2string(char *data, int size)
{
	char buffer[4096] = {0};
	hexarray2str(data, size, buffer, sizeof(buffer));
	return string(buffer);
}

// void PRINTBYTES(const char*header,u8*data,int size)
// {
//   char buffer[1024];
//   memset(buffer,0,sizeof(buffer));
//   sprintf(buffer,"[%s]:len=%d, ",header,size);
//   int iii = 0;
//   int offset = strlen(buffer);
//   while (iii<size && iii <512)
//   {
//     sprintf(buffer+offset+iii*2,"%02x",data[iii]);
//     iii++;
//   }
//   zlog_info(zcat,"%s",buffer);
// }

bool is_zero(u8 *d, int len)
{
	int i = 0;
	for (i = 0; i < len; i++)
	{
		if (d[i] != 0xFF)
			return false;
	}
	return true;
}

#include <sstream>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
using namespace std;
void time2string(time_t t, char *buffer, const char *format)
{
	std::stringstream ss;
	ss << std::put_time(std::localtime(&t), format);
	strcpy(buffer, ss.str().c_str());
}

string time2string(time_t t, const char *format)
{
	std::stringstream ss;
	ss << std::put_time(std::localtime(&t), format);
	return ss.str();
}
string time2string(time_t t)
{
	return time2string(t, "%F %T");
}

time_t string2time(std::string str)
{
	char *cha = (char *)str.data();													// 将string转换成char*。
	tm tm_;																			// 定义tm结构体。
	int year, month, day, hour, minute, second;										// 定义时间的各个int临时变量。
	sscanf(cha, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second); // 将string存储的日期时间，转换为int临时变量。
	tm_.tm_year = year - 1900;														// 年，由于tm结构体存储的是从1900年开始的时间，所以tm_year为int临时变量减去1900。
	tm_.tm_mon = month - 1;															// 月，由于tm结构体的月份存储范围为0-11，所以tm_mon为int临时变量减去1。
	tm_.tm_mday = day;																// 日。
	tm_.tm_hour = hour;																// 时。
	tm_.tm_min = minute;															// 分。
	tm_.tm_sec = second;															// 秒。
	tm_.tm_isdst = 0;																// 非夏令时。
	time_t t_ = mktime(&tm_);														// 将tm结构体转换成time_t格式。
	return t_;																		// 返回值。
}

u8 crc8(u8 *data, int size)
{
	u8 crc = 0x00;
	u8 poly = 0x07;
	int bit;

	while (size--)
	{
		crc ^= *data++;
		for (bit = 0; bit < 8; bit++)
		{
			if (crc & 0x80)
			{
				crc = (crc << 1) ^ poly;
			}
			else
			{
				crc <<= 1;
			}
		}
	}

	return crc;
}

#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <limits.h>
#include <string.h>

/** regex 错误输出缓冲区 */
static char regerrbuf[256];
//************************************
// search_match_t 初始化,以初始容量16分配内存,并将内存清零
// 如果匹配次数超过容量,会调用 rx_search_match_ensure 自动扩容
// @param    search_match_t * _psmatch
// @param    size_t           groupcnt
// @return   int              成功返回0,否则返回-1
//************************************
static int rx_search_match_init(search_match_t *_psmatch, size_t groupcnt)
{
	if (NULL == _psmatch)
	{
		return -1;
	}
	_psmatch->capacity = 16;
	_psmatch->matchcnt = 0;
	_psmatch->groupcnt = groupcnt;
	size_t size = sizeof(regmatch_t) * groupcnt * _psmatch->capacity;
	_psmatch->pmatch = (regmatch_t *)malloc(size);
	if (!_psmatch->pmatch)
	{
		printf("%s:%d MEM ERROR\n", __FILE__, __LINE__);
		return -1;
	}
	/** 内存清零 */
	memset(_psmatch->pmatch, 0, size);
	return 0;
}

//************************************
// search_match_t 扩容,确保 search_match_t 中有足够的空闲空间保存 freecnt 指定大小的匹配数据
// 扩容部分内存清零
// @param    search_match_t * _psmatch
// @param    size_t           freecnt 要求空闲保存的匹配空间数量(每个匹配需要的regmatch_t数量为groupcnt)
// @return   regmatch_t*      扩容成功返回最后空闲的regmatch_t起始位置,否则返回NULL
//************************************
static regmatch_t *rx_search_match_ensure(search_match_t *_psmatch, size_t freecnt)
{
	regmatch_t *newbuffer = NULL;
	size_t newsize = 0;
	size_t newcapacity = 0;

	if ((_psmatch == NULL) || (_psmatch->pmatch == NULL))
	{
		printf("%s:%d NULL ARGUMENT\n", __FILE__, __LINE__);
		return NULL;
	}

	if ((_psmatch->capacity > 0) && (_psmatch->matchcnt >= _psmatch->capacity))
	{
		printf("%s:%d INVALID matchcnt %d\n", __FILE__, __LINE__, (int)_psmatch->matchcnt);
		return NULL;
	}

	if (freecnt > (INT_MAX / 64))
	{
		printf("%s:%d TOO LARGE ARGUMENT matchcnt %d\n", __FILE__, __LINE__, (int)freecnt);
		return NULL;
	}

	if (freecnt <= (_psmatch->capacity - _psmatch->matchcnt))
	{
		return _psmatch->pmatch + (_psmatch->matchcnt * _psmatch->groupcnt);
	}

	/** 以16整数倍扩容 */
	newcapacity = ((freecnt + _psmatch->matchcnt + 16 - 1) >> 4 << 4);
	newsize = newcapacity * _psmatch->groupcnt * sizeof(regmatch_t);

	/* reallocate with realloc if available */
	newbuffer = (regmatch_t *)realloc(_psmatch->pmatch, newsize);
	if (newbuffer == NULL)
	{
		printf("%s:%d MEM ERROR\n", __FILE__, __LINE__);
		free(_psmatch->pmatch);
		_psmatch->capacity = 0;
		_psmatch->groupcnt = 0;
		_psmatch->matchcnt = 0;
		_psmatch->pmatch = NULL;

		return NULL;
	}
	size_t oldsize = _psmatch->matchcnt * _psmatch->groupcnt * sizeof(regmatch_t);
	size_t expsize = (newcapacity - _psmatch->matchcnt) * _psmatch->groupcnt * sizeof(regmatch_t);
	/** 扩容部分清零 */
	memset(newbuffer + oldsize, 0, expsize);

	_psmatch->capacity = newcapacity;
	_psmatch->pmatch = newbuffer;
	printf("%s:%d pmatch buffer expand to %d match\n", __FILE__, __LINE__, (int)newcapacity);
	return _psmatch->pmatch + (_psmatch->matchcnt * _psmatch->groupcnt);
}
//************************************
// 释放search_match_t中分配的空间,
// @param    search_match_t * _psmatch
//************************************
void rx_search_match_uninit(search_match_t *_psmatch)
{
	if (_psmatch)
	{
		free(_psmatch->pmatch);
		memset(_psmatch, 0, sizeof(search_match_t));
	}
}
//************************************
// 用指定的正则表达式在字符串中查找所有匹配
// @param    const char * input    待匹配的字符串
// @param    const char * pattern  正则表达式
// @param    size_t       groupcnt 正则表达式中捕获组数量(包含默认组group 0),为0时使用默认值,即pattern编译后regex_t的re_nsub+1
//                                 regex_t.re_nsub字段为正则表达式中子表达式的数量,子表达式又分为捕获和非捕获两种.
//                                 所以re_nsub + 1肯定大于等于表达式中所有捕获组(包含默认组group 0)的数量
// @param    int          eflags   正则表达匹配执行标志,参见 regexec
// @param    search_match_t *      _psmatch [out] 保存字符串所有匹配的位置
// @return   int                   匹配成功返回匹配匹配的数量,没有匹配返回0,失败返回-1,
//                                 调用层必须调用rx_search_match_uninit释放分配的空间
//************************************
int rx_serach(const char *input, const char *pattern, size_t groupcnt, int eflags, search_match_t *_psmatch)
{
	if (NULL == input || NULL == pattern || NULL == _psmatch)
	{
		ERROR("%s:%d NULL ARGUMENT\n", __FILE__, __LINE__);
		return 0;
	}

	regex_t reg;
	/************************************************************************/
	/* 编译正则表达式,编译成功的 regex_t 对象才可以被后续的 regexec 使用    */
	/************************************************************************/
	int c = regcomp(&reg, pattern, REG_EXTENDED);
	if (0 != c)
	{
		/************************************************************************/
		/*  正则表达式编译出错输出错误信息                                      */
		/*  调用 regerror 将错误信息输出到 regerrbuf 中                         */
		/*  regerrbuf 末尾置0,确保上面调用regerror 导致 regerrbuf 溢出的情况下, */
		/*  字符串仍有有结尾0                                                   */
		/*  然后 printf 输出                                                    */
		/************************************************************************/
		regerror(c, &reg, regerrbuf, sizeof(regerrbuf));
		regerrbuf[sizeof(regerrbuf) - 1] = '\0';
		ERROR("%s:%d %s\n", __FILE__, __LINE__, regerrbuf);
		return -1;
	}

	if (0 == groupcnt)
	{
		groupcnt = reg.re_nsub + 1;
	}
	c = rx_search_match_init(_psmatch, groupcnt);
	if (0 != c)
	{
		/** search_match_t 初始化失败,释放前面初始化成功的 regex_t */
		regfree(&reg);
		return c;
	}
	/** 起始匹配的偏移量 */
	size_t offset = 0;
	/************************************************************************/
	/* regexec 不能通过一次调用找到字符串中所有满足匹配条件的字符串位置,    */
	/* 所以需要通过步进偏移的方式循环查找字符串中所有匹配的字符串,          */
	/* 每一次匹配的起始偏移是上一次匹配到的字符串结束偏移                   */
	/************************************************************************/
	do
	{
		// INFO("MATCH start %d", (int)offset);
		/** 输出缓冲区扩容 */
		regmatch_t *pmatch = rx_search_match_ensure(_psmatch, 1);
		if (NULL == pmatch)
		{
			ERROR("%s:%d MEMORY ERROR for rx_search_match_ensure\n", __FILE__, __LINE__);
			c = -1;
			break;
		}
		/** 正则表达式匹配的起始地址 */
		const char *p = input + offset;
		/************************************************************************/
		/* regmatch_t 用于记录正则表达匹配的结果,每一个 regmatch_t 记录一个捕获 */
		/* 组(catch group)的在字符串中的起始位置。                              */
		/* 如果调用 regexec 时如果不提供 regmatch_t(nmatch为0,pmatch为NULL),    */
		/* 或者提供的 regmatch_t 数组长小于正则表达式中全部捕获组的数量,        */
		/* regexec 也能正常匹配,只是无法记录匹配的位置                          */
		/* 或不能完全记录所有的匹配结果                                         */
		/************************************************************************/
		c = regexec(&reg, p, _psmatch->groupcnt, pmatch, eflags);
		if (REG_NOMATCH == c)
		{
			/************************************************************************/
			/** 没有找到匹配结束循环                                                */
			/************************************************************************/
			// INFO("MATCH FINISHED\n");
			break;
		}
		else if (0 == c)
		{
			/** 匹配计数加1 */
			_psmatch->matchcnt++;
			/** 找到匹配,则输出匹配到的所有捕获组(catch group) */
			// INFO("%d MATCH (%d-%d)", (int)_psmatch->matchcnt, pmatch[0].rm_so, pmatch[0].rm_eo);
			//  for (int i = 0; i < _psmatch->groupcnt; ++i)
			//  {
			//  	printf("group %d :<<", i);
			//  	print_str(p, pmatch[i].rm_so, pmatch[i].rm_eo);
			//  	printf(">>\n");
			//  }
			/** (group 0)的结束位置 */
			size_t eo = pmatch[0].rm_eo;
			for (int i = 0; i < _psmatch->groupcnt; ++i)
			{
				/** 偏移量修改为相对于字符串起始位置 */
				pmatch[i].rm_so += (int)offset;
				pmatch[i].rm_eo += (int)offset;
			}
			/************************************************************************/
			/* 使用整体匹配捕获组0(group 0)的结束位置的更新偏移量,                  */
			/* 下一次匹配从当前匹配的结束位置开始                                   */
			/************************************************************************/
			offset += eo;
			continue;
		}
		else
		{
			/************************************************************************/
			/** regexec 调用出错输出错误信息,结束循环                               */
			/************************************************************************/
			regerror(c, &reg, regerrbuf, sizeof(regerrbuf));
			regerrbuf[sizeof(regerrbuf) - 1] = '\0';
			ERROR("%s", regerrbuf);
			c = -1;
			break;
		}
	} while (1);
	// INFO("%d MATCH FOUND", (int)_psmatch->matchcnt);
	/************************************************************************/
	/** regfree 必须与 regcomp 配对使用,否则会发生内存泄露                  */
	/************************************************************************/
	regfree(&reg);
	/** REG_NOMATCH 为正常循环结束标志  */
	if (c != REG_NOMATCH)
	{
		/** 出错时释放 search_match_t 所占内存 */
		rx_search_match_uninit(_psmatch);
		return c;
	}
	return (int)_psmatch->matchcnt;
}

// system函数扩展，加入超时值(0表示永久等待)
//超时时返回-2，其他情况返回不变。
int system_ex(const char *cmdstring, unsigned int timeout) /* with appropriate signal handling */
{
	pid_t pid;
	int status;
	struct sigaction ignore, saveintr, savequit;
	sigset_t chldmask, savemask;

	//精度换成十分之一秒
	timeout *= 10;
	if (timeout == 0)
		timeout = 0xFFFFFFFF;

	if (cmdstring == NULL)
		return (1); /* always a command processor with UNIX */

	ignore.sa_handler = SIG_IGN; /* ignore SIGINT and SIGQUIT */
	sigemptyset(&ignore.sa_mask);
	ignore.sa_flags = 0;
	if (sigaction(SIGINT, &ignore, &saveintr) < 0)
		return (-1);
	if (sigaction(SIGQUIT, &ignore, &savequit) < 0)
		return (-1);
	sigemptyset(&chldmask); /* now block SIGCHLD */
	sigaddset(&chldmask, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &chldmask, &savemask) < 0)
		return (-1);

	if ((pid = fork()) < 0)
	{
		return -1; /* probably out of processes */
	}
	else if (pid == 0)
	{ /* child */
		/* restore previous signal actions & reset signal mask */
		sigaction(SIGINT, &saveintr, NULL);
		sigaction(SIGQUIT, &savequit, NULL);
		sigprocmask(SIG_SETMASK, &savemask, NULL);
		/*通常exec会放在fork() 函数的子进程部分, 来替代子进程执行啦, 执行成功后子程序就会消失,  但是执行失败的话, 必须用exit()函数来让子进程退出!*/
		execl("/bin/sh", "sh", "-c", cmdstring, (char *)0); /*exec函数会取代执行它的进程,  也就是说, 一旦exec函数执行成功, 它就不会返回了, 进程结束.   但是如果exec函数执行失败, 它会返回失败的信息,  而且进程继续执行后面的代码!*/
		_exit(127);											/* exec error */
	}

	/* parent */
	int ret = 0;
	while (timeout-- > 0 &&
		   (ret = waitpid(pid, &status, WNOHANG)) == 0)
		usleep(100 * 1000);

	/* restore previous signal actions & reset signal mask */
	if (sigaction(SIGINT, &saveintr, NULL) < 0)
		return (-1);
	if (sigaction(SIGQUIT, &savequit, NULL) < 0)
		return (-1);
	if (sigprocmask(SIG_SETMASK, &savemask, NULL) < 0)
		return (-1);

	if (ret < 0)
		return -1;

	if (ret > 0)
		return status;

	kill(pid, SIGKILL);
	waitpid(pid, &status, 0);
	return -2;
}

// 1.
/**
 *@brief          执行系统命令，并返回输出的结果
 *@param[in]      cmdstring，命令串
 *@param[in]      buf，存放命令结果的缓冲区
 *@param[in]      size，缓冲区的大小
 *@param[out]
 *@return         返回写入到buf中的字符串长度，不含\0 ; -1: 失败;
 *@remark
 *@version        V1.0.0
 *@note     	  buf中最多返回size-1个字符，字符串始终以\0结尾。
 */
int get_cmd_results(const char *cmdstring, char *buff, int size)
{
	char cmd_string[200] = {0};
	char tmpfile[100] = {0};
	char tmp_buf[100] = {0};
	int fd;
	int tmp_fd;
	int nbytes;

	memset(buff, 0, size);

	if ((cmdstring == NULL) ||
		(strlen(cmdstring) > (sizeof(tmpfile) + 8)) ||
		((strlen(cmdstring) + strlen(tmpfile) + 5) > sizeof(cmd_string)))
	{
		printf("cmd is too long or NULL!\n");
		return -1;
	}
	sscanf(cmdstring, "%[a-Z]", tmp_buf); /*%[a-z] 表示匹配a到z中任意字符，贪婪性(尽可能多的匹配) */
	sprintf(tmpfile, "/tmp/%s-XXXXXX", tmp_buf);

	tmp_fd = mkstemp(tmpfile);
	if (tmp_fd < 0)
	{
		printf("mkstemp failed\n");
		return -1;
	}
	close(tmp_fd);

	sprintf(cmd_string, "%s > %s 2>&1", cmdstring, tmpfile); /*标准输出（1），标准错误（2）都输出到临时文件*/
	if (system_ex(cmd_string, 20) < 0)
	{
		printf("run \"%s\" ret < 0!\n", cmd_string);
	}

	fd = open(tmpfile, O_RDONLY);
	if (fd < 0)
	{
		printf("open %s failed!\n", tmpfile);
		nbytes = -1;
	}
	else
	{
		nbytes = read(fd, buff, size - 1);
		close(fd);
	}

	memset(cmd_string, 0, sizeof(cmd_string));
	sprintf(cmd_string, "rm -rf /tmp/%s-*", tmp_buf);
	system_ex(cmd_string, 20);

	return nbytes;
}

string get_cmd_results(const char *cmdstring)
{
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	int bytes = get_cmd_results(cmdstring, buffer, sizeof(buffer));
	if (bytes > 0)
		return string(buffer);
	else
		return "";
}

#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

/*dir operation*/
int mk_dir(char *dir)
{
#define MODE (S_IRWXU | S_IRWXG | S_IRWXO)
	DIR *mydir = NULL;
	if ((mydir = opendir(dir)) == NULL) //判断目录
	{
		int ret = mkdir(dir, MODE); //创建目录
		if (ret != 0)
		{
			return -1;
		}
		printf("[mk_dir]:%s created sucess!\n", dir);
		return 1;
	}
	else
	{
		return 0;
	}
	return 0;
}
int mk_all_dir(char *dir)
{
	int mkcount = 0;
	bool flag = true;
	char *pDir = dir;
	while (flag)
	{
		char *pIndex = index(pDir, '/');
		if (pIndex != NULL && pIndex != dir)
		{
			char buffer[512] = {0};
			int msg_size = pIndex - dir;
			memcpy(buffer, dir, msg_size);
			int ret = mk_dir(buffer);
			if (ret < 0)
			{
				INFO("[创建目录]: %s fail!\n", dir);
				return -1;
			}
			if (ret > 0)
				mkcount++;
		}
		else if (pIndex == NULL && pDir == dir)
		{
			printf("dir is not directory!\n");
			return -1;
		}
		else if (pIndex == NULL && pDir != dir)
		{
			int ret = mk_dir(dir);
			if (ret < 0)
			{
				INFO("[创建目录]: %s fail!\n", dir);
			}
			if (ret > 0)
				mkcount++;
			break;
		}
		pDir = pIndex + 1;
	}
	return mkcount;
}

// ip <->hex
//"120.238.064.059:33048"
void ipstring2hex(char *str, char *hexout)
{
	int ip1, ip2, ip3, ip4, port;
	sscanf(str, "%x.%x.%x.%x:%x", &ip1, &ip2, &ip3, &ip4, &port);
	u8 tmp[20];
	memset(tmp, 0, sizeof(tmp));
	tmp[0] = 2;
	tmp[1] |= (ip1 >> 4);
	tmp[2] |= (ip1 << 4);
	tmp[2] |= (ip2 >> 8) & 0x0F;
	tmp[3] |= (ip2 & 0xFF);
	tmp[4] |= (ip3 >> 4);
	tmp[5] |= (ip3 << 4);
	tmp[5] |= (ip4 >> 8) & 0x0F;
	tmp[6] |= (ip4 & 0xFF);
	tmp[7] |= 0xFF & (port >> 16);
	tmp[8] |= 0xFF & (port >> 8);
	tmp[9] |= 0xFF & (port);
	memcpy(hexout, tmp, 20);
}
void hex2ipstring(char *hex, char *str)
{
	int ip1, ip2, ip3, ip4, port;
	u8 *tmp = (u8 *)hex;
	ip1 = tmp[1];
	ip1 <<= 4;
	ip1 |= tmp[2] >> 4;
	ip2 = tmp[2] & 0x0F;
	ip2 <<= 8;
	ip2 |= tmp[3];
	ip3 = tmp[4];
	ip3 <<= 4;
	ip3 |= tmp[5] >> 4;
	ip4 = tmp[5] & 0x0F;
	ip4 <<= 8;
	ip4 |= tmp[6];
	port = tmp[7];
	port <<= 8;
	port |= tmp[8];
	port <<= 8;
	port |= tmp[9];
	sprintf(str, "%x.%x.%x.%x:%x", ip1, ip2, ip3, ip4, port);
}

bool is_diff_hour(time_t t1, time_t t2)
{
	t1 -= t1 % 3600;
	t2 -= t2 % 3600;
	return t1 != t2;
}

bool Cycle_Check(time_t now, int cycle_seconds, int delays)
{
	if (cycle_seconds == 0)
		return false;
	tm *ptm = localtime(&now);
	int seconds = cycle_seconds;
	int temp = ptm->tm_hour * 3600 + ptm->tm_min * 60 + ptm->tm_sec;
	temp = temp - delays;
	if (temp < 0)
	{
		temp = -temp;
		temp = -(temp % seconds); //
	}
	else
	{
		temp = temp % seconds; //
	}
	if (temp == 0)
		return true;
	return false;
}

// 5min offset
int TimeOffset5Min(time_t t)
{
	struct tm *tt = localtime((const time_t *)&t);
	return tt->tm_min / 5;
}

//字符串分割函数
std::vector<std::string> split(std::string str, std::string pattern)
{
	std::string::size_type pos;
	std::vector<std::string> result;
	str += pattern; //扩展字符串以方便操作
	int size = str.size();
	for (int i = 0; i < size; i++)
	{
		pos = str.find(pattern, i);
		if (pos < size)
		{
			std::string s = str.substr(i, pos - i);
			i = pos + pattern.size() - 1;
			if (s.compare("") != 0)
				result.push_back(s);
		}
	}
	return result;
}
std::string list_merge(std::vector<std::string> list, std::string parttern)
{
	stringstream ss;
	for (int i = 0; i < list.size(); i++)
	{
		ss << list[i] << parttern;
	}
	return ss.str();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
long findfileinfolder(const char *dir_name, string extend_name, std::vector<std::string> &fileList, std::vector<std::string> &nameList) //文件路径
{
	long number = 0;
	if (NULL == dir_name)
	{
		ERROR("目录不存在:%s.", dir_name);
		return 0;
	}
	struct stat s;
	lstat(dir_name, &s);
	struct dirent *filename; // return value for readdir()
	DIR *dir;				 // return value for opendir()
	dir = opendir(dir_name);
	if (NULL == dir)
	{
		ERROR("无法打开目录:%s,创建该目录", dir_name);
		mk_all_dir((char *)dir_name);
		return 0;
	}
	/* read all the files in the dir ~ */
	while ((filename = readdir(dir)) != NULL)
	{
		//INFO("%s find: %s", dir_name, filename->d_name);
		// get rid of "." and ".."
		if (strcmp(filename->d_name, ".") == 0 ||
			strcmp(filename->d_name, "..") == 0)
			continue;

		string sFilename(filename->d_name);
		string suffixStr = sFilename.substr(sFilename.find_last_of('.') + 1); //获取文件后缀

		if (suffixStr.compare(extend_name) == 0)
		{ //根据后缀筛选文件

			// cout<<filename->d_name <<endl;
			++number;
			string st1 = dir_name;
			string st2 = filename->d_name;
			fileList.push_back(st1 + st2);
			nameList.push_back(st2);
		}
	}
	return number;
}
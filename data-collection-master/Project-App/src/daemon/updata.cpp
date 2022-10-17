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
#include "ftpclient.h"
#include "lib.h"
#include <regex.h>
#include <signal.h>

//===============================setting get=============================//

#ifdef __MD5_TAR_SUPPORT__
const char *debpartten = "app-cjq_V[0-9._]*tar";
#else
const char *debpartten = "app-cjq_V[0-9._]*deb";
#endif

const char *app = "app-cjq";
#define SIG_UPDATA SIGUSR1

#include "rtu_setting.hpp"
rtu_setting rtusetting;

FTPClient cftp;

//===============================rtu 配置  =======================//
bool load_rtu_setting(string fullname, rtu_setting *prtu)
{
    json j;
    rtu_setting rtu;
    std::ifstream ifs("/app-cjq/bill/setting/rtu_setting.json");
    ifs >> j;
    rtu = (rtu_setting)j;
    *prtu = rtu;
    std::cout << j;
    return true;
}

// save
bool save_rtu_setting(string fullname, rtu_setting rtu)
{
    json j = (json)rtu;
    // 序列化为字符串
    std::string json_str = j.dump();
    // 美化输出，使用 4 个空格对输出进行格式化
    std::string pretty_str = j.dump(4, ' ');
    // 将 JSON 内容输出到文件
    std::ofstream ofs(fullname);
    ofs << j << std::endl;
    INFO("[save rtu setting]\n");
    std::cout << j;
    return true;
}

bool load_daemon_setting(void)
{

    load_rtu_setting(RTU_SETTING_FULLNAME, &rtusetting);

    if (rtusetting.param.center[0] == 0)
    {
        ERROR("[rtu setting] center[0] ==0\n");
        return false;
    }

    return true;
}

int ftp_get_setting(void)
{
    //从文件读入配置
    // const char*ip = "192.168.3.223";
    // const char* user = "kaihua";
    // const char* password = "111111";
    // const char* dir = "/app-cjq";
    // const char* serverdir="ftp";
    // strcpy(rtusetting.server.ftp.ip,ip);
    // strcpy(rtusetting.server.ftp.user,user);
    // strcpy(rtusetting.server.ftp.password,password);
    // strcpy(rtusetting.server.ftp.downdir,dir);
    // strcpy(rtusetting.server.ftp.serverdir,serverdir);
    // rtusetting.server.ftp.enable = true;

    load_daemon_setting();

    if (rtusetting.server.ftp.enable == 1)
    {
        if (strlen(rtusetting.server.ftp.ip.data()) > 0 &&
            strlen(rtusetting.server.ftp.downdir.data()) > 0 &&
            strlen(rtusetting.server.ftp.user.data()) > 0 &&
            strlen(rtusetting.server.ftp.password.data()) > 0)
            return 0;

        else
            return -1;
    }

    return 0;
}

//===============================0. connect server =======================//
int ftp_connect_server(const char *ip, const char *user, const char *password)
{
    printf("begin to connect ftp .\r\n");
    int fd = cftp.ftp_connect(ip, 21, user, password);
    if (fd < 0)
    {
        printf("ftp connect falied\n");
    }
    return fd;
}

//===============================1. list ftp deb ========================//
int ftp_list_deb_version(int fd, char *sdir, char *bufferout, const char *partten)
{
    char *aa = bufferout;
    void *data = nullptr;
    unsigned long long datalen;
    printf("begin to list ftp .\r\n");
    int result = cftp.ftp_list(fd, sdir, &data, &datalen);

    if (result != 0)
    {
        printf("ftp server does not ack.\r\n");
        return -1;
    }
    printf("len = %d ,data is :%s", (int)datalen, (const char *)data);

    // app-cjq_V1.05_220427_15.51.36.tar
    if (datalen > 1)
    {
        char regdeb[100];
        const char *d = (const char *)data;
        regex_t re_sp;
        sprintf(regdeb, partten, app);
        int ret = regcomp(&re_sp, regdeb, REG_EXTENDED);
        regmatch_t regmatch[20];
        //读取版本
        int status = regexec(&re_sp, (const char *)data, 1, regmatch, 0);
        if (status == 0)
        {
            printf("match it\r\n");
            int i = 0;
            int offset = regmatch[i].rm_so + 1 + strlen(app);
            int oss = regmatch[i].rm_eo - regmatch[i].rm_so - strlen(app) - 1 - 4;
            strncpy(aa, (const char *)(d + offset), oss);
            aa[regmatch[i].rm_eo - regmatch[i].rm_so] = '\0';
            printf("deb version = %s\r\n", aa);
            return strlen(aa);
        }
        else if (status == REG_NOMATCH)
        {
            printf("not match it\r\n");
            return -1;
        }

        free(data);
    }

    return -1;
}

//===============================3. 当前deb version ========================//
int get_current_app_version(const char *appname, char *versionout, int outsize)
{
    char buffer[100];
    sprintf(buffer, "%s --version", appname);
    int ret = get_cmd_results(buffer, versionout, outsize);
    if (ret > 0)
    {
        printf("app version = %s\n", versionout);
    }
    return ret;
}

//===============================.4 比较版本================================//
// return:  0:eq  -1:current>ftp   1:current<ftp
int is_version_bigger(const char *current, const char *ftp)
{
    int major = 0, minor = 0, mminor = 0, date = 0, hour = 0, min = 0, second = 0;
    int fmajor = 0, fminor = 0, fmminor = 0, fdate = 0, fhour = 0, fmin = 0, fsecond = 0;
    sscanf(current, "V%d.%d.%d_%d_%d.%d.%d", &major, &minor, &mminor, &date, &hour, &min, &second);
    sscanf(ftp, "V%d.%d.%d_%d_%d.%d.%d", &fmajor, &fminor, &fmminor, &fdate, &fhour, &fmin, &fsecond);

    u_int64_t h = 0, fh = 0, l = 0, fl = 0;
    //==xxx.xxx.xxx.xxxxxx
    h |= major;
    h <<= 3;
    h |= minor;
    h <<= 3;
    h |= mminor;
    fh |= fmajor;
    fh <<= 3;
    fh |= fminor;
    fh <<= 3;
    fh |= fmminor;

    l = date;
    l *= 1000000;
    l += hour;
    l *= 100;
    l += min;
    l *= 100;
    l += second;
    fl = fdate;
    fl *= 1000000;
    fl += fhour;
    fl *= 100;
    fl += fmin;
    fl *= 100;
    fl += fsecond;

    if (h > fh)
        return -1;
    if (h == fh)
    {
        if (l > fl)
            return -1;
        if (l == fl)
            return 0;
        return 1;
    }
    return 1;
}

//===============================.5 下载文件================================//
int download_deb(int fd, const char *ddir, const char *sdir, const char *file)
{
    unsigned long long stor_len = 0;
    char sfile[200];
    char dfile[200];
    sprintf(sfile, "%s/%s", sdir, file);
    sprintf(dfile, "%s/%s", ddir, file);
    int ret = cftp.ftp_retrfile(fd, (char *)sfile, (char *)dfile, &stor_len, NULL);
    if (ret != 0)
        return -1;
    return 0;
}

//===============================.5.1 tar================================//
#ifdef __MD5_TAR_SUPPORT__
int tar_and_check(const char *ddir, const char *file)
{
    char buffer[100];
    char tmp[100];
    int ret = 0;
    // cd
    sprintf(buffer, "cd %s", ddir);
    ret = get_cmd_results(buffer, tmp, sizeof(tmp));

    // tar -vxf
    memset(buffer, 0, sizeof(buffer));
    memset(tmp, 0, sizeof(tmp));
    sprintf(buffer, "tar -xvf %s", file);
    ret = get_cmd_results(buffer, tmp, sizeof(tmp));
    if (sscanf(tmp, "app-cjq.md5") < 0)
        return -1;
}
#endif

//===============================.6 关闭当前程序================================//
int shutdown_app(const char *appname)
{
    char cmdstring[200];
    char pid[100];
    printf("entry shutdown app.\n");
    memset(pid, 0, sizeof(pid));
    memset(cmdstring, 0, sizeof(cmdstring));
    sprintf(cmdstring, "ps -ef | grep %s | grep -v \"grep\" | awk '{print $2}'", appname);

    if (get_cmd_results((const char *)cmdstring, pid, sizeof(pid)) >= 0)
    {
        if (strlen(pid) > 0)
        {
            int p = atoi(pid);
            printf("%s pid=%d.\n", appname, p);

            // 1.向pid发送 UPDATA 信号
            if (p > 0)
            {
                printf("send sig to app\n");
                kill(p, SIG_UPDATA);
                sleep(2);
                if (get_cmd_results((const char *)cmdstring, pid, sizeof(pid)) > 0)
                {
                    int pp = atoi(pid);
                    if (pp > 0)
                    {
                        //直接kill
                        printf("kill app directory.\n");
                        sprintf(cmdstring, "kill %d", pp);
                        get_cmd_results((const char *)cmdstring, pid, sizeof(pid));
                        sleep(1);
                        return 1;
                    }
                }
                else
                {
                    return 0;
                }
            }
        }
        else
        {
            printf("no app is running.\n");
            return 0;
        }
    }
    return 0;
}

//===============================.7 安装deb====================================//
int install_app(const char *dir, const char *deb, char *debversion)
{
    char buffer[500];
    char result[1024];
    sprintf(buffer, "dpkg -i %s/%s", dir, deb);
    int ret = get_cmd_results(buffer, result, sizeof(result));

    //查询安装后的版本
    sprintf(buffer, "%s --version", app);
    ret = get_cmd_results(buffer, result, sizeof(result));
    if (ret > 0)
    {
        printf("new version=%s \n", result);
    }

    //比较deb版本是否一致
    if (is_version_bigger(debversion, result) == 0)
        return 0;

    else
        return -1;
}

//===============================.8 重启app====================================//
int restart_app(const char *appname)
{
    return 0;
}

//===============================ftp 升级流程====================================//
// 1min查询一次
int updata_poll(int interval)
{
    int fd = -1;
    int ret = 0;
    char debversion[100];
    char version[100];
    bool updata = false;

    if ((ret = ftp_get_setting()) == 0)
    {
        static int t = 0;
        t += interval;

        if (rtusetting.server.ftp.enable == false)
            return 0;

        if (t < rtusetting.server.ftp.interval)
        {
            return 0;
        }
        t = 0;

        // connect
        if ((ret = ftp_connect_server(rtusetting.server.ftp.ip.data(), rtusetting.server.ftp.user.data(), rtusetting.server.ftp.password.data())) < 0)
            return -1;
        fd = ret;

        // list
        if ((ret = ftp_list_deb_version(fd, (char *)rtusetting.server.ftp.serverdir.data(), debversion, debpartten)) < 0)
            return -1;
        // curent
        if ((ret = get_current_app_version(app, version, sizeof(version))) < 0)
            return -1;
        ret = is_version_bigger(version, debversion);
        switch (ret)
        {
        case 0:
            printf("app version == deb version\n");
            break;
        case 1:
            printf("app version <  deb version\n");
            updata = true;
            break;
        case -1:
            printf("app version >  deb version\n");
            break;
        default:
            break;
        }

        // updata
        if (updata)
        {
            char filename[100];
            sprintf(filename, "%s_%s.tar", app, debversion);
            mk_all_dir((char *)rtusetting.server.ftp.downdir.data());
            if (download_deb(fd, rtusetting.server.ftp.downdir.data(), rtusetting.server.ftp.serverdir.data(), filename) != 0)
            {
                printf("down load file:%s faile\r\n", filename);
            }
            else
            {

                //关闭当前app
                if (shutdown_app(app) != 0)
                    shutdown_app(app);

                //安装
                if (install_app(rtusetting.server.ftp.downdir.data(), filename, debversion) != 0)
                {
                    if (install_app(rtusetting.server.ftp.downdir.data(), filename, debversion) != 0)
                    {
                        printf("install Error.\n");
                        return 1;
                    }
                }
                printf("install OK.\n");
                return 0;
            }
        }

        cftp.ftp_quit(fd);
        printf("ftp quit\n");
    }

    return 0;
}
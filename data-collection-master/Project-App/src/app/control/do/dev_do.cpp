#include "dev_do.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <exception> 
#include "lib.h" 
#include "cfgChange.h"
#include "configor/json.hpp"
using namespace std;
using namespace configor;

#define PATH_CFG_DO "/app-cjq/setting/do.json"

typedef struct DEV_DO_CONFIG_S
{
    int duration;
    vector<int> openTime;
}Do_s;

void* doctrl_thread(void *arg)
{
    int res = 0;
    std::stringstream strstream;
    json jStr;
    Do_s do_s;
    time_t tnow = 0;
    struct tm* timenow;

    if ( isJsonFile(PATH_CFG_DO, &jStr) < 0 )
    {
        ERROR("[修改配置]:DO配置文件错误格式错误, 请检查");
        return nullptr;
    }

    // for (auto iter = jStr.begin(); iter != jStr.end(); iter++) 
    // {
    //     strstream << iter.key() << ":" << iter.value() << std::endl;
    //     jStr[iter.key().c_str()] = iter.value();
    // }

    do_s.duration = jStr["duration"];
    for (auto iter = jStr["start_time"].begin(); iter != jStr["start_time"].end(); iter++) 
    {
        do_s.openTime.push_back(iter.value().as_integer() % 24);
        INFO("[do] do高电平时间:%d", iter.value().as_integer() % 24);
    }

    system("echo 1D > /dev/ido_do");    //上电DO1低电平10s后边高电平
    system("echo 2E > /dev/ido_do");    //DO2高电平10s后再根据实际情况变化电平
    sleep(10);

    system("echo 1E > /dev/ido_do");    //DO1高电平
    while (1)
    {
        timestd(&tnow);
        timenow = localtime(&tnow);

        for (auto iter = do_s.openTime.begin(); iter != do_s.openTime.end(); iter++) 
        {
            if ( *iter == timenow->tm_hour )
            {
                if ( timenow->tm_min*60 + timenow->tm_sec <= 3 )
                {
                    system("echo 2E > /dev/ido_do");    //DO2高电平
                    INFO("[do2] 输出高电平");
                    break;
                }

                if ( timenow->tm_min*60 + timenow->tm_sec >= do_s.duration )
                {
                    system("echo 2D > /dev/ido_do");    //DO2低电平
                    // INFO("[do2] 输出低电平");
                    break;
                }
            }
        }

        sleep(1);
    }

    return NULL;
}

pthread_t thread_doctrl_init(void)
{
    pthread_t id;
    if (pthread_create(&id, NULL, doctrl_thread, NULL) != 0)
    {
        ERROR("[线程创建]:ERR =  doctrl_thread.");
        return 0;
    }
    INFO("[线程创建]:OK = doctrl_thread.");
    return id;
}
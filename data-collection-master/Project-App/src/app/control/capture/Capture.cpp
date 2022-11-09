#include "Capture.h"
#include "dev_do.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <exception> 
#include "lib.h" 
#include "cfgChange.h"
#include "configor/json.hpp"
using namespace std;
using namespace configor;

#define PATH_CFG_JPG "/app-cjq/setting/picture.json"
#define FORMAT_HTTP_JGP "wget -O %s/%s.jpg -o /dev/null \'http://%s:%s@%s/cgi-bin/snapshot.cgi\'"
// #define FORMAT_HTTP_JGP "wget -O %s/%s.jpg -o /dev/null \'http://%s:%s@%s/onvifsnapshot/media_service/snapshot?channel=1&subtype=0\'"
// wget -O 111.jpg 'http://admin:Yd87356212@192.168.1.10/cgi-bin/snapshot.cgi'

typedef struct DEV_CAPTURE_CONFIG_S
{
    string account;     //账号
    string password;    //密码
    string picture_ip;  //相机ip
    string picture_dir; //存放目录
    string picture_name;//图片名字
    int captureFlg = 0; //抓拍标志
    int shift_time = 0; //抓拍时延
    vector<int> start_time;
}Capture_s;

void* picturectrl_thread(void *arg)
{
    int res = 0;
    std::stringstream strstream;
    json jStr;
    Capture_s Capt_s;
    time_t tnow = 0;
    struct tm* timenow;
    int captureStartFlg = 0; //开始抓拍标志 0初始化 1开始抓拍 2停止抓拍 

    /* 读取配置文件信息 */
    if ( isJsonFile(PATH_CFG_JPG, &jStr) < 0 )
    {
        ERROR("[修改配置]:抓配图片置文件错误格式错误, 请检查");
        return nullptr;
    }

    // for (auto iter = jStr.begin(); iter != jStr.end(); iter++) 
    // {
    //     strstream << iter.key() << ":" << iter.value() << std::endl;
    //     jStr[iter.key().c_str()] = iter.value();
    // }

    /* 将读取到的json信息初始化到结构体变量 */
    Capt_s.account      = jStr["account"];
    Capt_s.password     = jStr["password"];
    Capt_s.picture_ip   = jStr["picture_ip"];
    Capt_s.picture_dir  = jStr["picture_dir"];
    Capt_s.picture_name = jStr["picture_name"];
    Capt_s.captureFlg   = jStr["capture"];
    Capt_s.shift_time   = jStr["shift_time"];
    for (auto iter = jStr["start_time"].begin(); iter != jStr["start_time"].end(); iter++) 
    {
        Capt_s.start_time.push_back(iter.value().as_integer() % 24);
    }

    // /* 读取到的信息 */
    // cout << "picture account      " << Capt_s.account      << endl; 
    // cout << "picture password     " << Capt_s.password     << endl;
    // cout << "picture picture_ip   " << Capt_s.picture_ip   << endl;
    // cout << "picture picture_dir  " << Capt_s.picture_dir  << endl;
    // cout << "picture picture_name " << Capt_s.picture_name << endl;
    // cout << "picture captureFlg   " << Capt_s.captureFlg   << endl;
    // cout << "picture shift_time   " << Capt_s.shift_time   << endl;

    while (1)
    {
        timestd(&tnow);
        timenow = localtime(&tnow);

        for (auto iter = Capt_s.start_time.begin(); iter != Capt_s.start_time.end(); iter++) 
        {
            if ( *iter == timenow->tm_hour )
            {
                int timesec = timenow->tm_min*60 + timenow->tm_sec;

                if ( timesec >= Capt_s.shift_time + 120 )
                {
                    captureStartFlg = 0;
                    break;
                }

                if ( timesec >= Capt_s.shift_time )
                {
                    if ( captureStartFlg == 0 )
                    {
                        captureStartFlg = 1;
                        break;
                    }
                } 
            }
        }

        isJsonFile(PATH_CFG_JPG, &jStr);    //读取配置
        Capt_s.captureFlg = jStr["capture"];

        if ( captureStartFlg == 1 )
        {
            captureStartFlg = 2;
            Capt_s.captureFlg = 1;          //抓拍
        }

        if ( Capt_s.captureFlg == 1 )
        {
            char test[] = "_00";
            char mvcmd[64] = {0};
            static int i = 0;
            i++;
            test[1] += i/10;
            test[2] += i%10;
            Capt_s.captureFlg = 0;
            char cmd[512] = {0};
            // sprintf(cmd, FORMAT_HTTP_JGP, Capt_s.picture_dir.c_str(), (Capt_s.picture_name + test).c_str(),  Capt_s.account.c_str(), Capt_s.password.c_str(), Capt_s.picture_ip.c_str());
            sprintf(cmd, FORMAT_HTTP_JGP, "/tmp", (Capt_s.picture_name + test).c_str(),  Capt_s.account.c_str(), Capt_s.password.c_str(), Capt_s.picture_ip.c_str());
            INFO("[拍照命令]:cmd = %s", cmd);
            system(cmd);

            if( jStr["capture"] == 1 )
            {
                jStr["capture"] = 0;
                std::ofstream ofs(PATH_CFG_JPG);
                ofs << std::setw(4) << jStr << std::endl;
            }

            sleep(3);
            sprintf(mvcmd, "mv /tmp/%s.jpg %s", (Capt_s.picture_name + test).c_str(), Capt_s.picture_dir.c_str());
            system(mvcmd);
        }

        sleep(1);
    }

    return NULL;
}

pthread_t thread_picturectrl_init(void)
{
    pthread_t id;
    if (pthread_create(&id, NULL, picturectrl_thread, NULL) != 0)
    {
        ERROR("[线程创建]:ERR =  picturectrl_thread.");
        return 0;
    }
    INFO("[线程创建]:OK = picturectrl_thread.");
    return id;
}
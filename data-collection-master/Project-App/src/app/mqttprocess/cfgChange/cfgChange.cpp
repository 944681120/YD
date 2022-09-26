#include "cfgChange.h"
#include <fstream>
#include <sstream>
#include <exception> 
#include "configor/json.hpp"
using namespace configor;

extern bool yanshichongqi;
#define PATH_CFG "/app-cjq/setting/rtu_setting.json"

struct MqttClientConfig
{
    string host;
    int port;
    string clientid;
    string user;
    string passwd;

    //更新数据的topic
    string topic_update;
    string topic_update_ack;
};

int isJsonStr(const char* str, json* pOut)
{
    json obj;
    try
    {
        obj = json::parse( str );
    }
    catch(exception& e)
    {
        return -1;
    }

    if ( pOut != nullptr )
    {
        *pOut = obj;
    }
    
    return 0;
}

int isJsonFile(const char* filePath, json* pOut)
{
    json obj;
    try
    {
        std::ifstream ifs(filePath);
        ifs >> obj;
    }
    catch(exception& e)
    {
        return -1;
    }

    if ( pOut != nullptr )
    {
        *pOut = obj;
    }

    return 0;
}

static void topic_cfgChange_handler(void *client, message_data_t *msg)
{
    (void)client;
    std::stringstream strstream;
    json jRecv;     //接收的json
    json jDest;     //处理的json

    if ( isJsonStr( (const char*)msg->message->payload, &jRecv) < 0 )
    {
        ERROR("[修改配置]:发送json数据不正确 : %s", (const char *)msg->message->payload);
        return;
    }

    if ( isJsonFile( PATH_CFG, &jDest ) < 0 )
    {
        ERROR("[修改配置]:配置文件json格式错误,请检查配置文件!");
        return;
    }

    for (auto iter = jRecv.begin(); iter != jRecv.end(); iter++) 
    {
        strstream << iter.key() << ":" << iter.value() << std::endl;
        jDest[iter.key().c_str()] = iter.value();
    }

    INFO("[修改配置] 修改了配置:\n %s\n", strstream.str().c_str());

    std::ofstream ofs(PATH_CFG);
    ofs << std::setw(4) << jDest << std::endl;
    yanshichongqi = true;   //修改配置文件重新启动程序
}

void* cfgChange_thread(void *arg)
{
    int res = 0;
    mqtt_client_t *client = mqtt_lease();
    struct MqttClientConfig mcc = 
    {
        "47.92.5.227",  //http://broker-cn.emqx.io
        1883,
        "mqtt报文修改配置文件",
        "",
        "",

        //上报数据
        "topic_cfgChange",
        "topic_cfgChange_ack",
    };

    mqtt_set_host(client, (char *)mcc.host.c_str());
    mqtt_set_port(client, (char *)std::to_string(mcc.port).c_str());
    mqtt_set_client_id(client, (char *)mcc.clientid.c_str());
    // mqtt_set_user_name(client, (char *)mcc.user.c_str());
    // mqtt_set_password(client, (char *)mcc.passwd.c_str());
    mqtt_set_clean_session(client, 1);
    
    if ((res = mqtt_connect(client)) != 0)
    {
        ERROR("[修改配置]:mqtt连接失败: code = %s",res);
        return NULL;
    }
    INFO("[修改配置]:mqtt连接成功");
    mqtt_subscribe(client, (char *)mcc.topic_update.c_str(), QOS1, topic_cfgChange_handler);

    while (1)
    {
        sleep(1);
    }

    return NULL;
}

pthread_t thread_cfgChange_init(void)
{
    pthread_t id;
    if (pthread_create(&id, NULL, cfgChange_thread, NULL) != 0)
    {
        ERROR("[线程创建]:ERR =  cfgChange_thread.");
        return 0;
    }
    INFO("[线程创建]:OK = cfgChange_thread.");
    return id;
}
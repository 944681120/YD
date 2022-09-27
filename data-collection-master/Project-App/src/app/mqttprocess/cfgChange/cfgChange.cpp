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
    std::stringstream strstream;
    json jRecv;     //接收的json
    json jDest;     //处理的json

    mqtt_message_t ackMsg = 
    {
        .qos = QOS0,
        .retained = 0,
        .dup = 0,
        .id = 0,
        .payloadlen = 0,
        .payload = nullptr,
    };
    char ackCode_0[] = "{ \"ackCode\": 0, \"message\":\"修改配置文件成功!\" }";
    char ackCode_1[] = "{ \"ackCode\":-1, \"message\":\"发送json数据不正确\" }";
    char ackCode_2[] = "{ \"ackCode\":-2, \"message\":\"设备配置文件json格式错误,请检查配置文件!\" }";

    if ( isJsonStr( (const char*)msg->message->payload, &jRecv) < 0 )
    {
        ERROR("[修改配置]:发送json数据不正确 : %s", (const char *)msg->message->payload);
        ackMsg.payload = ackCode_1;
        ackMsg.payloadlen = strlen(ackCode_1);
        mqtt_publish((mqtt_client_t*)client, (char *)mcc.topic_update_ack.c_str(), &ackMsg);
        return;
    }

    if ( isJsonFile( PATH_CFG, &jDest ) < 0 )
    {
        ERROR("[修改配置]:配置文件json格式错误,请检查配置文件!");
        ackMsg.payload = ackCode_2;
        ackMsg.payloadlen = strlen(ackCode_2);
        mqtt_publish((mqtt_client_t*)client, (char *)mcc.topic_update_ack.c_str(), &ackMsg);
        return;
    }

    for (auto iter = jRecv.begin(); iter != jRecv.end(); iter++) 
    {
        strstream << iter.key() << ":" << iter.value() << std::endl;
        jDest[iter.key().c_str()] = iter.value();
    }

    INFO("[修改配置] 修改了配置:\n %s\n", strstream.str().c_str());
    ackMsg.payload = ackCode_0;
    ackMsg.payloadlen = strlen(ackCode_0);
    mqtt_publish((mqtt_client_t*)client, (char *)mcc.topic_update_ack.c_str(), &ackMsg);

    std::ofstream ofs(PATH_CFG);
    ofs << std::setw(4) << jDest << std::endl;
    // yanshichongqi = true;   //修改配置文件重新启动程序(有bug?)
}

void* cfgChange_thread(void *arg)
{
    int res = 0;
    mqtt_client_t *client = mqtt_lease();

    mqtt_set_host(client, (char *)mcc.host.c_str());
    mqtt_set_port(client, (char *)std::to_string(mcc.port).c_str());
    mqtt_set_client_id(client, (char *)mcc.clientid.c_str());
    // mqtt_set_user_name(client, (char *)mcc.user.c_str());
    // mqtt_set_password(client, (char *)mcc.passwd.c_str());
    mqtt_set_clean_session(client, 1);
    
    for (int i = 0; ; i++)
    {
        if ((res = mqtt_connect(client)) != 0)
        {
            if ( i >= 10 )
            {
                ERROR("[修改配置]:多次mqtt连接失败: code = %d",res);
                return NULL;
            }
        }
        else
        {
            INFO("[修改配置]:mqtt连接成功 i=%d", i);
            break;
        }
        sleep(5);
    }
    mqtt_subscribe(client, (char *)mcc.topic_update.c_str(), QOS0, topic_cfgChange_handler);

    while (1)
    {
        sleep(1);
    }

    mqtt_disconnect(client);
    mqtt_release(client);
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
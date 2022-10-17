// /*
//  * @Author: jiejie
//  * @Github: https://github.com/jiejieTop
//  * @Date: 2019-12-11 21:53:07
//  * @LastEditTime: 2020-06-08 20:45:33
//  * @Description: the code belongs to jiejie, please keep the author information and source code according to the license.
//  */
// #include <stdio.h>
// #include <unistd.h>
// #include <fcntl.h>
// #include <stdlib.h>
// #include <pthread.h>
// #include "mqtt_config.h"
// #include "mqtt_log.h"
// #include "mqttclient.h"

// #include <string>
// #include <iostream>
// #include <stdio.h>
// //#include "lib.h"

// using namespace std;

// #include "gtest/gtest.h"

// struct MqttClientConfig
// {
//     string host;
//     int port;
//     string clientid;
//     string user;
//     string passwd;

//     //更新数据的topic
//     string topic_update;
//     string topic_update_ack;
// };

// class mqtt_device

// {
// private:
//     //mqtt-连接参数 

//     //配置参数

//     //上报功能

//     //命令功能

//     //第三方命令
// public:
//     mqtt_device(){}
//     ~mqtt_device(){}
 
// };




// struct MqttClientConfig mcc = {
//     "a19L3BT3jUz.iot-as-mqtt.cn-shanghai.aliyuncs.com",
//     1883,
//     "a19L3BT3jUz.nibianqi|securemode=2,signmethod=hmacsha256,timestamp=1659400481938|",
//     "nibianqi&a19L3BT3jUz",
//     "88b00979d747e4ef5c08f3520c2a3c9bfbac0bbd1bc4175b710a41394357dacd",

//     //上报数据
//     "/a19L3BT3jUz/nibianqi/user/update",
//     "/a19L3BT3jUz/nibianqi/user/updateack"//"/sys/a19L3BT3jUz/nibianqi/thing/service/property/set"
//     };

// // #define TEST_USEING_TLS
// extern const char *test_ca_get();

// static void topic1_handler(void *client, message_data_t *msg)
// {
//     (void)client;
//     MQTT_LOG_I("-----------------------------------------------------------------------------------");
//     MQTT_LOG_I("%s:%d %s()...\ntopic: %s\nmessage:%s", __FILE__, __LINE__, __FUNCTION__, msg->topic_name, (char *)msg->message->payload);
//     MQTT_LOG_I("-----------------------------------------------------------------------------------");
// }

// void *mqtt_publish_thread(void *arg)
// {
//     mqtt_client_t *client = (mqtt_client_t *)arg;

//     char buf[100] = {0};
//     mqtt_message_t msg;
//     memset(&msg, 0, sizeof(msg));
//     sprintf(buf, "welcome to mqttclient, this is a publish test...");

//     // sleep(2);

//     mqtt_list_subscribe_topic(client);

//     msg.payload = (void *)buf;

//     while (1)
//     {
//         sprintf(buf, "welcome to mqttclient, this is a publish test, a rand number: %d ...", random_number());

//         // msg.qos = QOS0;
//         // mqtt_publish(client, "topic1", &msg);

//         msg.qos = QOS1;
//         mqtt_publish(client, (char *)mcc.topic_update.c_str(), &msg);

//         // msg.qos = QOS2;
//         // mqtt_publish(client, "topic3", &msg);

//         sleep(4);
//     }
// }

// TEST(TESTCASE, mqttclient)
// {
//     int res;
//     pthread_t thread1;
//     mqtt_client_t *client = NULL;

//     printf("\nwelcome to mqttclient test...\n");

//     mqtt_log_init();

//     client = mqtt_lease();

// #ifdef TEST_USEING_TLS
//     mqtt_set_port(client, "8883");
//     mqtt_set_ca(client, (char *)test_ca_get());
// #else
//     mqtt_set_port(client, (char *)std::to_string(mcc.port).c_str());
// #endif

//     mqtt_set_host(client, (char *)mcc.host.c_str());
//     mqtt_set_client_id(client, (char *)mcc.clientid.c_str());
//     mqtt_set_user_name(client, (char *)mcc.user.c_str());
//     mqtt_set_password(client, (char *)mcc.passwd.c_str());
//     mqtt_set_clean_session(client, 1);

//     if ((res = mqtt_connect(client)) != 0)
//     {
//         MQTT_LOG_E("连接失败: code = %s",res);
//         return ;
//     }

//     // mqtt_subscribe(client, "topic1", QOS0, topic1_handler);
//     mqtt_subscribe(client, (char *)mcc.topic_update_ack.c_str(), QOS1, topic1_handler);
//     // mqtt_subscribe(client, "topic3", QOS2, NULL);

//     // res = pthread_create(&thread1, NULL, mqtt_publish_thread, client);
//     // if(res != 0) {
//     //     //MQTT_LOG_E("create mqtt publish thread fail");
//     //     exit(res);
//     // }

//     mqtt_publish_thread(client);
// }

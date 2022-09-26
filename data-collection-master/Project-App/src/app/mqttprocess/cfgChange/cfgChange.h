#ifndef _CFGCHANGE_H_
#define _CFGCHANGE_H_

#include "lib.h"
#include "app.h"
#include <unistd.h>

#include "mqtt_config.h"
#include "mqtt_log.h"
#include "mqttclient.h"

void *cfgChange_thread(void *arg);
pthread_t thread_cfgChange_init(void);
int isJsonStr(const char* str, json* pOut = nullptr);
int isJsonFile(const char* filePath, json* pOut = nullptr);

#endif // _CFGCHANGE_H_
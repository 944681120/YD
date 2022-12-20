
#include "app.h"
#include "DataClass.hpp"
#include "rtu_setting.hpp"
#include "VersionConfig.h"
#include "lib.h"
#include "data_save.h"
#define __APP_DEBUG__

// thread_t
pthread_t all_threads[10];
int count_threads = 0;

// down thread
extern pthread_t thread_down_init(void);
// up  thread
extern pthread_t thread_up_init(void);
// data thread
extern pthread_t thread_data_init(void);
//雨量
extern pthread_t thread_yu_init(void);

int app_main(int argc, char **argv, bool zloginit)
{
    // printf("out argc=%d  argv[0]=%s\r\n",argc,argv[0]);
    if (argc > 1)
    {
        if (strcmp("--version", argv[1]) == 0)
        {
            printf("%s\n", VERSION_BUILD_TIME);
            return 0;
        }
        return 0;
    }

    pthread_t deamon;
    if (zloginit)
        init_zlog();
    startup_recovery();
    all_threads[count_threads++] = thread_data_init();
    all_threads[count_threads++] = thread_down_init();
    all_threads[count_threads++] = thread_up_init();
    all_threads[count_threads++] = thread_yu_init();

    pthread_join(all_threads[0], NULL);
    ERROR("%s", "=======================退出==========================");
    drop_zlog();
    return 1;
}


#include "lib.h"

// init for zlog
bool zlog_inited = false;
zlog_category_t *zcat;
zlog_category_t *zcat_data;
const char* version = "V1.1.20.00";

void init_zlog(void)
{
  int rc;
  const char *conf = "/app-cjq/setting/zlog.conf";
  printf("zlog conf: %s\n", conf);
  rc = zlog_init(conf);
  if (rc)
  {
    printf("init failed\n");
    return;
  }

  zcat = zlog_get_category("my_cat");
  if (!zcat)
  {
    printf("get cat fail\n");
    zlog_fini();
    return;
  }

  zcat_data = zlog_get_category("data");
  if (!zcat_data)
  {
    ERROR("get cat:data fail\n");
    zlog_fini();
    return ;
  }

  zlog_info(zcat, "===================================start up, version:%s==============================", version);
  zlog_info(zcat, "app-cjq,start up....");
  zlog_inited = true;
}

void drop_zlog(void)
{
  zlog_fini();
}


//dir/  只保留 count个最新文件
int zlog_delect_log(const char *dir, int count)
{
	char cmd[200];
	char res[1024];
	sprintf(cmd, "sudo ls -t %s | sed -n '%d,$p' | xargs -I {} sudo rm -f %s{}", dir, count + 1, dir);
	int r = get_cmd_results(cmd, res, sizeof(res));
	INFO("result=%s\n", res);
	return r;
}
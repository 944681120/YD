#include "center.h"
#include "report.h"
#include "upacket.h"
#include "mxdevice.h"
#include "sendmode.h"
#include "lib.h"
#include "Ibao.hpp"
#include "rtu_setting.hpp"
#include "DataClass.hpp"
#include "data_save.h"
#include "app.h"
#include "MultPacket.hpp"
using namespace std;
#include "gtest/gtest.h"

TEST(TESTCASE, app_main)
{
  extern int app_main(int argc, char **argv, bool zloginit);
  INFO("app_main test begin.");
  app_main(0,NULL,false);
}
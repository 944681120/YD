#include "gtest/gtest.h"
#include <vector>
#include "DB.hpp"
//===============================================================================
const Field definition_report1[] =
    {
        Field(FIELD_KEY_AUTOINCREMENT),
        // Field("id",  type_int,flag_autoincrement+flag_primary_key),   //标致
        Field("date", type_text), //时间
        Field("data", type_text), //数据
        Field(DEFINITION_END),
};

class TbReport1
{
public:
    int _ID;
    string date;
    string data;
    const Field *field;
    TbReport1() { field = definition_report1; }
    JSON_BIND(TbReport1, _ID, date, data);
};

TEST(TESTCASE, add)
{
    // DB<TB> dlog("/home/kaihua/Desktop/report.db", "hour");
    // TbReport1 item;
    // INFO("add");
    // item.date = time2string(timestd(0), "%F %T"); // timestd(0);
    // item.data = "201112345,34128888,201112345,34128888,201112345,34128888,201112345,34128888,201112345,34128888=>GTest:add";
    // EXPECT_EQ(0,dlog.add(&item));

    // vector<TbReport1> msgs;
    // dlog.find(string("select * from hour order by date desc limit 1;"),msgs);
    // EXPECT_EQ(1,msgs.size());
}

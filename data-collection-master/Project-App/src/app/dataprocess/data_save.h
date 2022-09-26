#pragma once

#include "lib.h"
#include "DB.hpp"

extern int init_rain(void);
extern int init_waterlevel(void);

//数据库
#define DBName "/app-cjq/report/report.db"
//数据表
//小时报
#define TableNameHour "hour"

//加报
#define TableNameJia "jia"

//自报
#define TableNameDingshi "dingshi"

//补报
#define TableNameBu "bu"

//重要数据
#define TableNameImportant "important"

/*-------------------------------------------------------
    小时报，加报，数据格式
---------------------------------------------------------
 */
const Field definition_base[] =
    {
        Field(FIELD_KEY_AUTOINCREMENT),
        // Field("id",  type_int,flag_autoincrement+flag_primary_key),   //标致
        Field("date", type_text), //时间
        Field("data", type_text), //数据
        Field("code", type_text), //校验 or 其他 用途
        Field(DEFINITION_END),
};

class TbBase
{
public:
    int _ID;
    string date;
    string data;
    string code;
    const Field *field;
    TbBase() { field = definition_base; }
    JSON_BIND(TbBase, _ID, date, data, code);
    string toString()
    {
        return string("_ID=") + std::to_string(_ID) + "  date=" + date + "  data=" + data + "  code=" + code;
    }
};

const Field definition_important[] =
    {
        Field(FIELD_KEY_AUTOINCREMENT),
        Field("name", type_text), // name
        Field("date", type_text), //时间
        Field("data", type_text), //数据
        Field("code", type_text), //校验 or 其他 用途
        Field(DEFINITION_END),
};

class TbImportant
{
public:
    int _ID;
    string name;
    string date;
    string data;
    string code;
    const Field *field;
    TbImportant() { field = definition_important; }
    JSON_BIND(TbImportant, _ID, name, date, data, code);
    string toString()
    {
        return string("_ID=") + std::to_string(_ID) + "  name=" + name + "  date=" + date + "  data=" + data + "  code=" + code;
    }
};

/*-------------------------------------------------------
    补报数据格式
---------------------------------------------------------
 */
const Field definition_bu[] =
    {
        Field(FIELD_KEY_AUTOINCREMENT),
        Field("valid", type_int),
        Field("name", type_text),
        Field("date", type_text),  //时间
        Field("cmd", type_text),   // cmd
        Field("tb", type_text),    // table name
        Field("tdate", type_text), // table date
        Field("ids", type_text),   //数据
        Field(DEFINITION_END),
};
class TbBu
{
public:
    int _ID;
    int valid;    //失效 bit[0]: 0失效  1有效
    string name;  //名称分类
    string date;  //加报被记录的时间
    string tb;    //所在table
    string tdate; //要补报的记录的时间
    string cmd;   //补报类型 rain,waterlevel
    string ids;   //其他备注
    const Field *field;
    TbBu() { field = definition_bu; }
    JSON_BIND(TbBu, _ID, valid, name, date, cmd, tb, tdate, ids);
    string toString()
    {
        stringstream ss;
        ss << "_ID=" + std::to_string(_ID);
        ss << "  valid=" << to_string(valid);
        ss << " name=" << name;
        ss << "  date=" << date;
        ss << "  cmd=" << cmd;
        ss << "  table=" << tb;
        ss << "  tabledate=" << tdate;
        ss << "  ids=" << ids;
        return ss.str();
    }
};

bool rainacc_recovery(void);
int devdata_recovery();
int param_recovery();
int runtime_recovery();
int personalset_recovery();
int startup_recovery();

int data_save_if_need(void);
int powerdown_save();
bool rainacc_save(void);
int devdata_save();
int personalset_save(rtu_setting *pr);
int runtime_save(rtu_setting *pr);
int param_save(rtu_setting *pr);

/*----------------------------------------------------------------------------------
                读取 report 中 特定时间的日志
-----------------------------------------------------------------------------------*/
int devdata_db_read(string tablename, string q, vector<TbBase> &msgs);
int devdata_db_read(string tablename, time_t t, vector<TbBase> &msgs);
int devdata_db_read(string tablename, time_t t, DataClass *data);
// from <= date < t
int devdata_db_read(string tablename, time_t from, time_t to, vector<TbBase> &msgs);
int devdata_db_save(string tablename, string code, time_t t, DataClass *pdata);
int devdata_db_save(string tablename, time_t t, DataClass *pdata);

/*----------------------------------------------------------------------------------
                加报
-----------------------------------------------------------------------------------*/
bool jia_add_flags(string name);
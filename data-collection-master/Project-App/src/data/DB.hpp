
#pragma once
#include "SqlRecordSet.h"
#include "SqlDatabase.h"
#include "SqlTable.h"
#include "lib.h"
#include "boost/pfr.hpp"
#include <typeinfo>
#include "json.hpp"
#include "SqlValue.h"
#include <string.h>
#include <stdlib.h>

using namespace sql;

template <typename T>
class DB
{
private:
    sql::Database db;
    const Field *fields;
    std::string name_db;
    std::string name_tb;
    T _m;
    string record2class(Record *pr, T *report)
    {
        stringstream ss;
        const FieldSet *fs = pr->fields();
        const Field *f;

        json j;
        ss << "{";
        for (int i = 0; i < pr->columnCount(); i++)
        {
            ss << "\"";
            f = fs->getByIndex(i);
            ss << (f->getName().data());
            ss << "\":";
            switch (f->getType())
            {
            case type_int:
            case type_time:
                ss << pr->getValue(i)->asInteger();
                break;
            default:
                ss << "\"" << pr->getValue(i)->asString() << "\"";
                break;
            }
            if (i < pr->columnCount())
                ss << ",";
        }
        ss << "}";
        ss >> j;
        *report = (T)j;
        // INFO("%s", j.dump(1, ' ').c_str());
        return j.dump();
    }

    string class2record(T *cl, Record *record)
    {
        json j = (json)*cl;
        // INFO("%s", j.dump(1, ' ').c_str());
        FieldSet fs = FieldSet(cl->field);
        Field *f;
        int i = 0;
        for (auto iter = j.begin(); iter != j.end(); iter++, i++)
        {
            if (iter.key().compare("_ID") == 0)
                continue;
            f = fs.getByName(iter.key());
            if (f == NULL)
            {
                ERROR("无法转换 class->record,class 中没有变量:%s", iter.key().c_str());
                continue;
            }
            // std::cout << iter.key() << ":" << iter.value() << std::endl;
            switch (f->getType())
            {
            case type_int:
                record->setInteger(iter.key(), iter.value().as_integer());
                break;
            case type_time:
                record->setTime(iter.key(), sql::time(iter.value().as_integer()));
                break;
            case type_float:
                record->setDouble(iter.key(), iter.value().as_float());
                break;
            default:
                record->setString(iter.key(), iter.value().as_string());
                break;
            }
        }
        return j.dump();
    }

public:
    DB(string dbname, string tablename)
    {
        name_db = dbname;
        name_tb = tablename;
        fields = _m.field;
        Init();
    };
    ~DB() { db.close(); };

public:
    void Init()
    {
        try
        {
            db.open(name_db);
            Table tb(db.getHandle(), name_tb, fields);
            if (tb.exists() == false)
            {
                INFO("create table  = %s", name_tb.c_str());
                tb.create();
            }

            db.close();
        }
        catch (Exception e)
        {
            ERROR("ERROR: %s\r\n", e.msg().c_str());
        }
    }

    //增，删，改，查
    int add(T *msg)
    {
        INFO("[add]  db(%s) table(%s)  item = [ %s ]", name_db.c_str(), name_tb.c_str(), msg->toString().c_str());
        try
        {
            db.open(name_db);
            Table tb(db.getHandle(), name_tb, fields);
            if (tb.exists() == false)
            {
                tb.create();
                INFO("create table  = %s", name_tb.c_str());
            }
            Record record(tb.fields());
            string result = class2record(msg, &record);

            if (tb.addRecord(&record) == true)
            {
                INFO("add OK . record=%s", result.c_str());
            }
            else
            {
                ERROR("add ERR. record=%s", result.c_str());
            }
            db.close();
            return 0;
        }
        catch (Exception e)
        {
            ERROR("ERROR: %s\r\n", e.msg().c_str());
            return -1;
        }
        return 0;
    }

    //增，删，改，查
    int find(string q, vector<T> &msgs)
    {
        INFO("[find]  db(%s) table(%s) query(%s)", name_db.c_str(), name_tb.c_str(), q.c_str());
        try
        {
            db.open(name_db);
            Table tb(db.getHandle(), name_tb, fields);
            if (tb.exists() == false)
            {
                tb.create();
                INFO("create table  = %s", name_tb.c_str());
                return 0;
            }

            string header("select"); // select * from hour order by date desc limit 1;
            if (q.compare(0, header.size(), header) == 0)
            {
                tb.open();
                tb.query(q);
            }
            else
                tb.open(string(q));

            if (tb.recordCount() > 0)
            {
                INFO("find record. query= %s count=%d", q.c_str(), tb.recordCount());
                for (int i = 0; i < tb.recordCount(); i++)
                {
                    Record *pr = tb.getRecord(i);
                    T msg;
                    string result = record2class(pr, &msg);
                    INFO("find record.   [%d]= %s", i, result.c_str());
                    msgs.push_back(msg);
                }
                return 1;
            }
            else if (tb.recordCount() == 0)
            {
                ERROR("NO   record. query  %s", q.c_str());
                db.close();
                return 0;
            }
            db.close();
            return tb.recordCount();
        }
        catch (Exception e)
        {
            ERROR("ERROR: %s\r\n", e.msg().c_str());
        }
        return 0;
    }

    int update(string q, T *msg)
    {
        INFO("[update]  db(%s) table(%s) query(%s)", name_db.c_str(), name_tb.c_str(), q.c_str());
        try
        {
            db.open(name_db);
            Table tb(db.getHandle(), name_tb, fields);
            if (tb.exists() == false)
            {
                INFO("create table  = %s", name_tb.c_str());
                tb.create();
            }

            tb.open(string(q));
            if (tb.recordCount() > 0)
            {
                INFO("find record. query= %s count=%d", q.c_str(), tb.recordCount());
                for (int i = 0; i < tb.recordCount(); i++)
                {
                    Record *pr = tb.getRecord(i);
                    Record nr = *pr;
                    string str = class2record(msg, &nr);
                    if (tb.updateRecord(&nr))
                    {
                        //回读
                        INFO("[更新表成功]update record  OK   = %s", str.c_str());
                    }
                    else
                    {
                        ERROR("[更新表失败]update record FAIL  = %s", str.c_str());
                        return 1;
                    }
                }
            }
            else if (tb.recordCount() == 0)
            {
                ERROR("NO   record. query  %s", q.c_str());
                db.close();
                return add(msg);
            }
            db.close();
            return 0;
        }
        catch (Exception e)
        {
            ERROR("ERROR: %s\r\n", e.msg().c_str());
            return 1;
        }
        return 0;
    }

    int del(string q, vector<T> &msgs)
    {
        INFO("[delete]  db(%s) table(%s) query(%s)", name_db.c_str(), name_tb.c_str(), q.c_str());
        try
        {
            db.open(name_db);
            Table tb(db.getHandle(), name_tb, fields);
            if (tb.exists() == false)
            {
                INFO("create table  = %s", name_tb.c_str());
                tb.create();
                return 0;
            }

            tb.open(string(q));
            if (tb.recordCount() > 0)
            {
                INFO("find record. query= %s count=%d", q.c_str(), tb.recordCount());
                for (int i = 0; i < tb.recordCount(); i++)
                {
                    T msg;
                    Record *pr = tb.getRecord(i);
                    Record nr = *pr;
                    string str = class2record(&msg, &nr);
                    msgs.pushback(msg);
                }
                tb.truncate();
            }
            else if (tb.recordCount() == 0)
            {
                ERROR("NO   record. query  %s", q.c_str());
            }
            db.close();
            return 0;
        }
        catch (Exception e)
        {
            ERROR("ERROR: %s\r\n", e.msg().c_str());
            return 1;
        }
        return 0;
    }

    int delall(void)
    {
        try
        {
            db.open(name_db);
            Table tb(db.getHandle(), name_tb, fields);
            if (tb.exists() == false)
            {
                INFO("create table  = %s", name_tb.c_str());
                tb.create();
            }
            tb.truncate();
            db.close();
        }
        catch (Exception e)
        {
            ERROR("ERROR: %s\r\n", e.msg().c_str());
            return 1;
        }
        return 0;
    }
};
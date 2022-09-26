#pragma once

#include "lib.h"
#include "header.h"
//==============正文类=======================//
//所有正文分类，都由这个派生
template <typename T>
class ZhengWen
{
private:
    vector<T> names;
    DataClass *pdata;

public:
    int size() { return names.size(); }
    Header header;
    ZhengWen() {}
    ~ZhengWen() {}

    int init(vector<T> items, DataClass *_pdata)
    {
        this->names = items;
        pdata = _pdata;
        return 0;
    }

    int init(vector<T> items, DataClass *_pdata, time_t obtime)
    {
        init(items, _pdata);
        this->header.observetime = obtime;
        return 0;
    }

    int to_array(u8 *dataout)
    {
        char cache[1024];
        u8 *d = dataout;
        int i = 0;
        // 1.header to array
        dataout += header.to_array(dataout);
        // 2.id-data to array
        for (int i = 0; i < this->size(); i++)
        {
            T n = names[i];
            int ilen = (*pdata)[n].toreport(cache);
            if (ilen > 0)
            {
                memcpy(dataout, cache, ilen);
                dataout += ilen;
            }
            else
            {
                stringstream ss;
                ss << n;
                ERROR("[ZhengWen 数据无效] name = %s", ss.str().c_str());
            }
        }
        INFO("\n%s", this->toString().c_str());
        return (int)(dataout - d);
    }

    int to_all(u8 *dataout)
    {
        char cache[1024];
        u8 *d = dataout;
        int i = 0;
        // 1.header to array
        dataout += header.to_array(dataout);
        // 2.id-data to array
        dataout += pdata->reportall((char *)dataout);
        INFO("\n%s", this->toStringAll().c_str());
        return (int)(dataout - d);
    }

    string toString()
    {
        stringstream ss;
        ss << header.toString();
        for (int i = 0; i < this->size(); i++)
        {
            T n = names[i];
            ss << (*pdata)[n].toString(pdata->Name().c_str()) << endl;
        }
        return ss.str();
    }

    string toStringAll(void)
    {
        stringstream ss;
        ss << header.toString();
        ss << pdata->toString();
        return ss.str();
    }
};

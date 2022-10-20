#pragma once

#include <sys/stat.h>
#include <stdio.h>
#include "lib.h"
#include "header.h"
using namespace std;

// M3 多包时，使用读一包，发送一包方式来实现
//本文实现 读一包接口
class MPBase
{

public:
    int persize = 1024;
    char buffer[2048];
    //读一包数据
    virtual int read(int offset, int size, char *dataout) = 0; // { return 0; }
    //总长
    virtual long size() = 0; //  { return 0; }
    //上报一包数据
    virtual int reportdata(int offset, int size, char *dataout) = 0; //  { return 0; }
};

class MPImageBase : public MPBase
{
private:
    const char *file;
    // char buffer[0x4000];
    FILE *fd = NULL;
    long _size = 0;
    time_t _guan_ce;

public:
    MPImageBase(const char *imagefile, int psize, time_t guan_ce_shi_jian)
    {
        _guan_ce = guan_ce_shi_jian;
        file = imagefile;
        fd = fopen(file, "r");
        if (!fd)
        {
            ERROR("无法打开文件:%s", file);
        }
        _size = size();
        if (psize > 0 && psize <= 1024)
            persize = psize;
    }
    ~MPImageBase()
    {
        if (fd)
        {
            fclose(fd);
        }
    }

    //读文件
    int read(int offset, int size, char *dataout)
    {
        if (!fd)
        {
            fd = fopen(file, "r");
            if (!fd)
            {
                ERROR("无法打开文件:%s", file);
                return 0;
            }
            _size = this->size();
        }
        if (offset >= _size)
            return 0;
        if ((offset + size) > _size)
        {
            size = _size - offset;
        }
        fseek(fd, offset, SEEK_SET);
        size_t actsize = fread(dataout, size, 1, fd);
        return size;
    }

    //文件大小
    long size()
    {
        if (!fd)
        {
            fd = fopen(file, "r");
            if (!fd)
            {
                ERROR("无法打开文件:%s", file);
                return -1;
            }
        }
        fseek(fd, 0, SEEK_END);
        long s = ftell(fd);
        return s;
    }

    //上报数据
    int reportdata(int offset, int size, char *dataout)
    {
        extern int get_current_liushui(void);
        if (size <= 0)
            return 0;
        int reportsize = 0, readsize = 0;
        Header h;
        h.liushui = get_current_liushui();
        h.observetime = _guan_ce;
        reportsize += h.to_array((u8 *)dataout);
        dataout += reportsize;
        dataout[0] = 0xF3;
        dataout[1] = 0xF3; //图片标识符
        dataout += 2;
        reportsize += 2;
        readsize = read(offset, size, dataout);
        if (readsize == 0)
            return 0;
        return reportsize + readsize;
    }
};
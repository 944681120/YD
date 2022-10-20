#include "ZhengWen.hpp"
#include "data_save.h"

/*=============================链路维持报===========================*/
int get_lianlu_report(u8 *dataout)
{
    Header h;
    char buffer[20];
    int size = h.to_array_front3((u8 *)buffer);
    memcpy(dataout, buffer, size);
    return size;
}

/*=============================测试报===========================*/
int get_ceshi_report(u8 *dataout)
{
    ZhengWen<string> z;
    //当前降水量    PJ
    //降水量累计值  PT
    //瞬时水位 Z
    //电压标识符  VT
    vector<string> item = {"PJ", "PT", "Z", "VT"};
    // z.init(item,sizeof(item)/sizeof(char*));
    z.init(item, &devdata);
    return z.to_array(dataout);
}
/*=============================均匀时段水文信息报=================*/
int get_junyunshiduan_report(u8 *dataout)
{
    ZhengWen<string> z;
    // 1.对均匀时段信息报，观测时间码表示第一组数据的观测时间。

    z.header.observetime = 0; //第一组的观察时间

    // DRxnn
    u32 DRxnn = 0; //第一组数据的观测时间
    blreverse(&DRxnn, dataout, 3);
    dataout += 3;
    // 2.1 小时内每 5 分钟时段雨量        DRP
    // 3.1 小时内 5 分钟间隔相对水位 1-8  DRZ1-DRZ8
    vector<string> item = {"DRP", "DRZ1", "DRZ2", "DRZ3", "DRZ4", "DRZ5", "DRZ6", "DRZ7", "DRZ8"};
    z.init(item, &devdata);
    return 3 + z.to_array(dataout);
}

/*===========================人工置数报===============================
   1.人工输入的数据
   2.
===========================人工置数报===============================*/
int get_rengong_report(u8 *dataout)
{
    ZhengWen<string> z;
    // 1.查询那些数据人工置入
    // 2.读取数据
    return z.to_array(dataout);
}

/*===========================图片报===============================*/
static u8 *imagedata = NULL;
int get_image_data(void *arg, int offset)
{
    //申请内存

    return 30000;
}
int get_tupian_report(u8 **dataout)
{
    u8 header[30];
    Header h;
    // 1.图片 F3H
    int headersize = h.to_array(header);
    int imagsize = get_image_data((void *)"shui-12:35:00.jpg", headersize + 2);
    memcpy(imagedata, header, headersize);
    imagedata[headersize] = 0xF3;
    imagedata[headersize + 1] = 0xF3;

    // headersize+2 之后是图片数据
    return headersize + 2 + imagsize;
}
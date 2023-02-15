#include "strDeal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int splitStr(const char* str, const char* sign, char** buf, int bufsize)
{
    if ( str == NULL )
    {
        printf("Err : 输入字符串为空\n");
        return -1;
    }

    if ( buf == NULL )
    {
        printf("Err : 输入缓存为空\n");
        return -2;
    }

    char _str[strlen(str) + 1] = {0};
    strcpy(_str, str);
    char* tmpStr = NULL;                    // 临时子串
    int index = 0;
    int subStrNum = 0;                      // 子串数
    int subStrindex[32] = {0};              // 子串起始位置
    int signlen = strlen(sign);

    while ( (tmpStr = strstr((char*)_str + index, sign)) != NULL )
    {
        if ( (tmpStr - _str) > index )  // 子串位置大于当前索引，则有一个子串
        {
            subStrindex[subStrNum++] = index;
            index = (tmpStr - _str);
        }

        for (int i = 0; i < signlen; i++)
        {
            tmpStr[i] = '\0';
        }
        index += signlen;
    }

    /* 处理最后分割符后面的字符串 */
    if ( index <= 0 )    // str找不到分隔符则不处理
    {
        return 0;
    }
    if ( _str[index] != '\0' )
    {
        subStrindex[subStrNum++] = index;
    }
    
    /* 将全部子串拷贝到buf，并在buf头创建子串指针 */
    memcpy(buf + subStrNum, _str, sizeof(_str));
    for (int i = 0; i < subStrNum; i++)
    {
        buf[i] = (char*)(buf + subStrNum) + subStrindex[i];
    }
    
    return subStrNum;  // 返回子串数
}
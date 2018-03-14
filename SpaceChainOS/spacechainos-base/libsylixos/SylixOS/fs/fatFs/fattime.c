/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: fattime.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 09 月 26 日
**
** 描        述: FAT 文件系统时间接口

** BUG:
2009.06.08  修改注释.
2009.06.17  get_fattime() 应该获取本地时间, 不是 UTC 时间.
2009.08.27  提高 get_fattime() 效率.
2009.09.15  修正时区错误.
2010.01.04  使用新的时区标准信息.
2010.07.10  简化 __timeToFatTime() 函数.
2012.03.11  FAT 存储的为本地时间, 所以转换函数需要加入时区信息, 将不再放在外面计算.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "diskio.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_FATFS_EN > 0)
/*********************************************************************************************************
** 函数名称: fattimeToTime
** 功能描述: 将 FAT 时间转换为系统时间
** 输　入  : dwFatTime     fat time
** 输　出  : time_t 时间 (UTC)
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
time_t  __fattimeToTime (UINT32  dwFatTime)
{
    struct tm   time;
    time_t      timeIs;
    
    /*
     *  以下时间为本地时间.
     */
    time.tm_sec  = (INT)((dwFatTime & 0x1F) * 2);
    time.tm_min  = (INT)((dwFatTime >>  5) & 0x3F);
    time.tm_hour = (INT)((dwFatTime >> 11) & 0x1F);
    time.tm_mday = (INT)((dwFatTime >> 16) & 0x1F);
    time.tm_mon  = (INT)((dwFatTime >> 21) & 0x0F);
    time.tm_year = (INT)((dwFatTime >> 25) & 0x7F);
    
    time.tm_mon  -= 1;                                                  /*  fat 月起始为 1              */
    time.tm_year += 80;                                                 /*  tm year 基点为 1900 年      */
                                                                        /*  fat 时间为 1980 年起        */
                                                                        
    timeIs = lib_mktime(&time);                                         /*  转换为 UTC time_t 时间      */
    
    return  (timeIs);
}
/*********************************************************************************************************
** 函数名称: timeToFatTime
** 功能描述: 将系统时间转换为 FAT 时间 (没有附加时区信息)
** 输　入  : ptime     time_t 时间 (UTC)
** 输　出  : fat time
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
UINT32  __timeToFatTime (time_t  *ptime)
{
    struct tm   time;
    UINT32      dwTime;
    
    lib_localtime_r(ptime, &time);                                      /*  struct tm 为本地时间        */
    
    time.tm_year -= 80;
    time.tm_mon  += 1;
    
    dwTime = (UINT32)((time.tm_year << 25) |
                      (time.tm_mon  << 21) |
                      (time.tm_mday << 16) |
                      (time.tm_hour << 11) |
                      (time.tm_min  <<  5) |
                      (time.tm_sec / 2));
                     
    return  (dwTime);
}
/*********************************************************************************************************
** 函数名称: get_fattime
** 功能描述: 获得当前系统时间
** 输　入  : NONE
** 输　出  : bit31:25 
                Year from 1980 (0..127) 
             bit24:21 
                Month (1..12) 
             bit20:16 
                Day in month(1..31) 
             bit15:11 
                Hour (0..23) 
             bit10:5 
                Minute (0..59) 
             bit4:0 
                Second / 2 (0..29) 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
UINT32   get_fattime (void)
{
    struct tm       time;
    time_t          tim;
    UINT32          dwTime;
    
    tim = lib_time(LW_NULL);
    lib_localtime_r(&tim, &time);                                       /*  转换为本地时间              */
    
    time.tm_year -= 80;
    time.tm_mon  += 1;
    
    dwTime = (UINT32)((time.tm_year << 25) |
                      (time.tm_mon  << 21) |
                      (time.tm_mday << 16) |
                      (time.tm_hour << 11) |
                      (time.tm_min  <<  5) |
                      (time.tm_sec / 2));
    
    return  (dwTime);                                                   /*  获得 FAT 时间               */
}

#endif                                                                  /*  LW_CFG_MAX_VOLUMES          */
                                                                        /*  LW_CFG_FATFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

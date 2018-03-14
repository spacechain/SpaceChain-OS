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
** 文   件   名: lib_clock.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 08 月 30 日
**
** 描        述: 系统库.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "lib_local.h"
/*********************************************************************************************************
** 函数名称: lib_gettimeofday
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_RTC_EN > 0

INT  lib_gettimeofday (struct timeval *tv, struct timezone *tz)
{
    struct timespec  timespecNew;

    if (tv) {
        lib_clock_gettime(CLOCK_REALTIME, &timespecNew);
        tv->tv_sec  = timespecNew.tv_sec;
        tv->tv_usec = timespecNew.tv_nsec / 1000;
    }
    
    if (tz) {
        tz->tz_minuteswest = (int)(timezone / SECSPERMIN);
        tz->tz_dsttime     = 0;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: lib_settimeofday
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  lib_settimeofday (const struct timeval *tv, const struct timezone *tz)
{
    struct timespec  timespecNew;

    if (tv) {
        timespecNew.tv_sec  = tv->tv_sec;
        timespecNew.tv_nsec = tv->tv_usec * 1000;
        lib_clock_settime(CLOCK_REALTIME, &timespecNew);
    }
    
    if (tz) {
        /*
         *  这里的时区仅仅是临时设置而已, 使用 tzset() 将自动从 TZ 环境变量中获取时区信息.
         */
        timezone = SECSPERMIN * (time_t)tz->tz_minuteswest;
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_RTC_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/

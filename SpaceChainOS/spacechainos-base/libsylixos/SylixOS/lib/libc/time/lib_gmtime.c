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
** 文   件   名: lib_gmttime.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 08 月 30 日
**
** 描        述: 系统库.

BUG:
2009.02.07  使用自己的 lldiv 函数.
2010.07.10  修正 lib_gmtime_r 函数的返回值.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "lib_local.h"
/*********************************************************************************************************
** 函数名称: __gettime
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_RTC_EN > 0

int __julday (int yr, /* year */ int mon, /* month */ int day /* day */)
{
    static int  jdays[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    int leap = 0;

    if (isleap (yr + TM_YEAR_BASE)) {
	/*
	 * If it is a leap year, leap only gets set if the day is
	 * after beginning of March (SPR #4251).
	 */
    	if (mon > 1)
    	    leap = 1;
	}

    return (jdays [mon] + day + leap );

}
/*********************************************************************************************************
** 函数名称: __gettime
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int  __daysSinceEpoch ( int year,	/* Years since epoch */ int yday 	/* days since Jan 1  */)
{
	if (year>=0) /* 1970 + */
    	return ( (365 * year) + (year + 1) / 4  + yday );
	else		/* 1969 - */
    	return ( (365 * year) + (year - 2) / 4  + yday );
} 
/*********************************************************************************************************
** 函数名称: __gettime
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int  __gettime (const time_t timer, struct tm *tmp)
{
    INT64	    days;
    INT64	    timeOfDay;
    INT64	    year;
    INT64	    mon;
    lib_lldiv_t	result; 

    /* Calulate number of days since epoch */

    days = timer / SECSPERDAY;
    timeOfDay = timer % SECSPERDAY;

    /* If time of day is negative, subtract one day, and add SECSPERDAY
     * to make it positive.
     */

    if(timeOfDay<0)
    	{
	timeOfDay+=SECSPERDAY;
	days-=1;
    	}

    /* Calulate number of years since epoch */

    year = days / DAYSPERYEAR;
    while ( __daysSinceEpoch((int)year, 0) > days )
    	year--;

    /* Calulate the day of the week */

    tmp->tm_wday = (int)((days + EPOCH_WDAY) % DAYSPERWEEK);

	/*
	 * If there is a negative weekday, add DAYSPERWEEK to make it positive
	 */
	if(tmp->tm_wday<0)
		tmp->tm_wday+=DAYSPERWEEK;

    /* Find year and remaining days */

    days -= __daysSinceEpoch((int)year, 0);
    year += EPOCH_YEAR;

    /* Find month */
    /* __jullday needs years since TM_YEAR_BASE (SPR 4251) */

    for  ( mon = 0; 
         (days >= __julday ((int)(year - TM_YEAR_BASE), (int)(mon + 1), 0)) && (mon < 11); 
         mon++ )
	;

    /* Initialise tm structure */

    tmp->tm_year = (int)(year - TM_YEAR_BASE); /* years since 1900 */
    tmp->tm_mon  = (int)mon;
    tmp->tm_mday = (int)(days - __julday (tmp->tm_year, (int)mon, 0)) + 1;
    tmp->tm_yday = (int)__julday (tmp->tm_year, (int)mon, tmp->tm_mday) - 1;
    tmp->tm_hour = (int)(timeOfDay / SECSPERHOUR);

    timeOfDay  %= SECSPERHOUR;
    
    {
        result = lib_lldiv(timeOfDay, SECSPERMIN);
    }
    
    tmp->tm_min = (int)result.quot;
    tmp->tm_sec = (int)result.rem;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gmtime_r
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
struct tm *lib_gmtime_r (const time_t *timer, struct tm *timeBuffer)
{
    if (!timer || !timeBuffer) {
        return  (LW_NULL);
    }
    
    __gettime(*timer, timeBuffer);

    return (timeBuffer);
}
/*********************************************************************************************************
** 函数名称: lib_gmtime
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
struct tm  *lib_gmtime (const time_t *timer)
{
    static struct tm  timeBuffer;
    
    return  (lib_gmtime_r(timer, &timeBuffer));
}

#endif                                                                  /*  LW_CFG_RTC_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/

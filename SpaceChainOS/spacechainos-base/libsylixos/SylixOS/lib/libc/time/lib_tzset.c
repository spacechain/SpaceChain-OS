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
** 文   件   名: lib_tzset.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 06 月 11 日
**
** 描        述: 系统库.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "lib_local.h"
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define __TIMEZONE_DEFAULT      (-(3600 * 8))                           /*  中国标准时间                */
#define __TIMEZONE_BUFFER_SIZE  10

#define DAYSPERNYEAR            365
#define DAYSPERLYEAR            366
/*********************************************************************************************************
  时区变量
*********************************************************************************************************/
time_t          timezone;                                               /*  UTC 时间与本地时间秒差      */
char           *tzname[2] = {"CST", "DST"};
static char     tzname_buffer[__TIMEZONE_BUFFER_SIZE] = "CST";
/*********************************************************************************************************
** 函数名称: lib_tzset
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_RTC_EN > 0

VOID    lib_tzset (VOID)
{
    char    cTzBuffer[64];
    char   *cTz;
    int     iNegdiff = 0;
    time_t  tempzone;
    int     iAlphaNum = 0;
    
    if (lib_getenv_r("TZ", cTzBuffer, 64) < 0) {
        timezone  = __TIMEZONE_DEFAULT;                                 /*  使用默认时区信息            */
        tzname[0] = tzname_buffer;                                      /*  中国标准时间                */
        return;
    }
    
    cTz = cTzBuffer;
    while (lib_isalpha(*cTz)) {
        cTz++;
    }
    iAlphaNum = cTz - cTzBuffer;
    iAlphaNum = (iAlphaNum < __TIMEZONE_BUFFER_SIZE) ? iAlphaNum : (__TIMEZONE_BUFFER_SIZE - 1);
    
    lib_strlcpy(tzname_buffer, 
                cTzBuffer, 
                iAlphaNum + 1);
    tzname[0] = tzname_buffer;
    
    /*
     * time difference is of the form:
     *
     *      [+|-]hh[:mm[:ss]]
     *
     * check minus sign first.
     */
    if (*cTz == '-') {
        iNegdiff++;
        cTz++;
    }
    
    tempzone = (time_t)(lib_atol(cTz) * 3600);
    while ((*cTz == '+') || ((*cTz >= '0') && (*cTz <= '9'))) {
        cTz++;
    }
    
    /*
     * check if minutes were specified
     */
    if ( *cTz == ':' ) {
        cTz++;
        tempzone += (time_t)(lib_atol(cTz) * 60);
        
        while ((*cTz >= '0') && (*cTz <= '9') ) {
            cTz++;
        }
        /*
         * check if seconds were specified
         */
        if (*cTz == ':' ) {
            cTz++;
            tempzone += (time_t)lib_atol(cTz);
            
            while ((*cTz >= '0') && (*cTz <= '9') ) {
                cTz++;
            }
        }
    }
    
    if (iNegdiff) {
        tempzone = -tempzone;
    }
    
    /*
     *  TODO: daylight not support!
     */
    
    timezone = tempzone;
}

#endif                                                                  /*  LW_CFG_RTC_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/

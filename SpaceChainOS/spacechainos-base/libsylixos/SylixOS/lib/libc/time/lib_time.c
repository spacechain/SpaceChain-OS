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
** 文   件   名: lib_time.c
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
** 函数名称: lib_time
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_RTC_EN > 0

time_t  lib_time (time_t *time)
{
    INTREG        iregInterLevel;
    time_t        timetmp;
    
    LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);
    timetmp = _K_tvTODCurrent.tv_sec;
    LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);

    if (time) {
        *time = timetmp;
    }
    
    return  (timetmp);
}
/*********************************************************************************************************
** 函数名称: lib_timelocal
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
time_t  lib_timelocal (time_t *time)
{
    INTREG        iregInterLevel;
    time_t        timetmp;
    
    LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);
    timetmp = UTC2LOCAL(_K_tvTODCurrent.tv_sec);
    LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);

    if (time) {
        *time = timetmp;
    }
    
    return  (timetmp);
}
/*********************************************************************************************************
** 函数名称: time
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
time_t  time (time_t *time)
{
    INTREG        iregInterLevel;
    time_t        timetmp;
    
    LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);
    timetmp = _K_tvTODCurrent.tv_sec;
    LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);

    if (time) {
        *time = timetmp;
    }
    
    return  (timetmp);
}

#endif                                                                  /*  LW_CFG_RTC_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/

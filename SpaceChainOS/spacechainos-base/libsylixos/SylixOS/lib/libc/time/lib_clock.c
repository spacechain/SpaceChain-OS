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

** BUG:
2013.11.28  加入 lib_clock_nanosleep().
2015.07.30  加入 lib_clock_getcpuclockid().
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  高分辨率时钟接口
*********************************************************************************************************/
#if LW_CFG_TIME_HIGH_RESOLUTION_EN > 0
#define LW_TIME_HIGH_RESOLUTION(tv)     bspTickHighResolution(tv)
#else
#define LW_TIME_HIGH_RESOLUTION(tv)
#endif                                                                  /*  LW_CFG_TIME_HIGH_RESOLUT... */
/*********************************************************************************************************
** 函数名称: lib_clock
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_RTC_EN > 0

clock_t  lib_clock (VOID)
{
    clock_t  clockRet;

#if LW_CFG_MODULELOADER_EN > 0
    INTREG        iregInterLevel;
    LW_LD_VPROC  *pvproc = __LW_VP_GET_CUR_PROC();
    
    if (pvproc == LW_NULL) {
        clockRet = (clock_t)API_TimeGet();

    } else {
        LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);
        clockRet = pvproc->VP_clockUser;
        LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);
    }

#else
    clockRet = (clock_t)API_TimeGet();
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    return  (clockRet);
}
/*********************************************************************************************************
** 函数名称: clock_getcpuclockid
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  lib_clock_getcpuclockid (pid_t pid, clockid_t *clock_id)
{
    if (!clock_id) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    *clock_id = CLOCK_PROCESS_CPUTIME_ID;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: lib_clock_getres
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
** 注  意  : 这里并没有判断真正的时钟精度.
*********************************************************************************************************/
INT  lib_clock_getres (clockid_t  clockid, struct timespec *res)
{
    if ((clockid < 0) || (clockid > CLOCK_THREAD_CPUTIME_ID) || !res) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    res->tv_sec  = 0;
    res->tv_nsec = 1;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: lib_clock_gettime
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  lib_clock_gettime (clockid_t  clockid, struct timespec  *tv)
{
    INTREG          iregInterLevel;
    PLW_CLASS_TCB   ptcbCur;

    if (tv == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    switch (clockid) {
    
    case CLOCK_REALTIME:
        LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);
        *tv = _K_tvTODCurrent;
        LW_TIME_HIGH_RESOLUTION(tv);
        LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);
        break;
    
    case CLOCK_MONOTONIC:
        LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);
        *tv = _K_tvTODMono;
        LW_TIME_HIGH_RESOLUTION(tv);
        LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);
        break;
        
    case CLOCK_PROCESS_CPUTIME_ID:
#if LW_CFG_MODULELOADER_EN > 0
        {
            LW_LD_VPROC *pvproc = __LW_VP_GET_CUR_PROC();
            if (pvproc == LW_NULL) {
                _ErrorHandle(ESRCH);
                return  (PX_ERROR);
            }
            LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);
            __tickToTimespec(pvproc->VP_clockUser + pvproc->VP_clockSystem, tv);
            LW_TIME_HIGH_RESOLUTION(tv);
            LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);
        }
#else
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
        break;
        
    case CLOCK_THREAD_CPUTIME_ID:
        LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);
        LW_TCB_GET_CUR(ptcbCur);
        __tickToTimespec(ptcbCur->TCB_ulCPUTicks, tv);
        LW_TIME_HIGH_RESOLUTION(tv);
        LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);
        break;
        
    default:
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: lib_clock_settime
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  lib_clock_settime (clockid_t  clockid, const struct timespec  *tv)
{
    INTREG      iregInterLevel;

    if (tv == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (clockid != CLOCK_REALTIME) {                                    /*  CLOCK_REALTIME              */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);
    _K_tvTODCurrent = *tv;
    LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: lib_clock_nanosleep
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  lib_clock_nanosleep (clockid_t  clockid, int  iFlags, 
                          const struct timespec  *rqtp, struct timespec  *rmtp)
{
    INTREG           iregInterLevel;
    struct timespec  tvValue;

    if ((clockid != CLOCK_REALTIME) && (clockid != CLOCK_MONOTONIC)) {
        _ErrorHandle(ENOTSUP);
        return  (PX_ERROR);
    }
    
    if ((!rqtp) ||
        LW_NSEC_INVALD(rqtp->tv_nsec)) {                                /*  时间格式错误                */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    tvValue = *rqtp;
    
    if (iFlags == TIMER_ABSTIME) {                                      /*  绝对时间                    */
        struct timespec  tvNow;
        
        LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);
        if (clockid == CLOCK_REALTIME) {
            tvNow = _K_tvTODCurrent;
        } else {
            tvNow = _K_tvTODMono;
        }
        LW_TIME_HIGH_RESOLUTION(&tvNow);
        LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);

        if (__timespecLeftTime(rqtp, &tvNow)) {
            return  (ERROR_NONE);                                       /*  不需要延迟                  */
        }
        
        __timespecSub(&tvValue, &tvNow);
        
        return  (nanosleep(&tvValue, LW_NULL));
    
    } else {
        return  (nanosleep(&tvValue, rmtp));
    }
}

#endif                                                                  /*  LW_CFG_RTC_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/

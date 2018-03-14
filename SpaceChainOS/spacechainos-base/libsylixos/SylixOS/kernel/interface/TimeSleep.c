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
** 文   件   名: TimeSleep.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 线程睡眠函数。

** BUG
2008.03.29  使用新的等待机制.
2008.03.30  使用新的就绪环操作.
2008.04.12  加入对信号的支持.
2008.04.13  由于很多内核机制需要精确的延迟, 所以这里区别 API_TimeSleep 和 posix sleep.
            API_TimeSleep 是不能被信号唤醒的, 而 posix sleep 是可以被信号唤醒的.
2009.10.12  加入对 SMP 多核的支持.
2012.03.20  减少对 _K_ptcbTCBCur 的引用, 尽量采用局部变量, 减少对当前 CPU ID 获取的次数.
2013.07.10  去掉 API_TimeUSleep API.
            API_TimeMSleep 至少保证一个 tick 延迟.
2015.02.04  nanosleep() 如果低于一个 tick 则使用 bspDelayNs() 进行延迟.
2017.07.15  sleep() 精确到秒即可.
2017.09.10  加入 nanosleep 算法选项设置.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  s_internal.h 中也有相关定义
*********************************************************************************************************/
#if LW_CFG_THREAD_CANCEL_EN > 0
#define __THREAD_CANCEL_POINT()         API_ThreadTestCancel()
#else
#define __THREAD_CANCEL_POINT()
#endif                                                                  /*  LW_CFG_THREAD_CANCEL_EN     */
/*********************************************************************************************************
  nanosleep
*********************************************************************************************************/
#if LW_CFG_TIME_HIGH_RESOLUTION_EN > 0
static INT      _K_iNanoSleepMethod = LW_TIME_NANOSLEEP_METHOD_THRS;
#else
static INT      _K_iNanoSleepMethod = LW_TIME_NANOSLEEP_METHOD_TICK;
#endif                                                                  /*  LW_CFG_TIME_HIGH_RESOLUTION */
/*********************************************************************************************************
** 函数名称: API_TimeSleep
** 功能描述: 线程睡眠函数 (此 API 不能被信号唤醒)
** 输　入  : ulTick            睡眠的时间
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API
VOID  API_TimeSleep (ULONG    ulTick)
{
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
	REGISTER PLW_CLASS_PCB         ppcb;
	REGISTER ULONG                 ulKernelTime;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return;
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_SLEEP, 
                      ptcbCur->TCB_ulId, ulTick, LW_NULL);
    
__wait_again:
    if (!ulTick) {                                                      /*  不进行延迟                  */
        return;
    }

    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核                    */

    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  从就绪表中删除              */
    
    ptcbCur->TCB_ulDelay = ulTick;
    __ADD_TO_WAKEUP_LINE(ptcbCur);                                      /*  加入等待扫描链              */
    
    __KERNEL_TIME_GET_NO_SPINLOCK(ulKernelTime, ULONG);                 /*  记录系统时间                */
    
    if (__KERNEL_EXIT_IRQ(iregInterLevel)) {                            /*  被信号激活                  */
        ulTick = _sigTimeoutRecalc(ulKernelTime, ulTick);               /*  重新计算等待时间            */
        goto    __wait_again;                                           /*  继续等待                    */
    }
}
/*********************************************************************************************************
** 函数名称: API_TimeSleepEx
** 功能描述: 线程睡眠函数 (精度为 TICK HZ)
** 输　入  : ulTick            睡眠的时间
**           bSigRet           是否允许信号唤醒
** 输　出  : ERROR_NONE or EINTR
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API
ULONG  API_TimeSleepEx (ULONG  ulTick, BOOL  bSigRet)
{
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
	REGISTER PLW_CLASS_PCB         ppcb;
	REGISTER ULONG                 ulKernelTime;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_SLEEP, 
                      ptcbCur->TCB_ulId, ulTick, LW_NULL);
    
__wait_again:
    if (!ulTick) {                                                      /*  不进行延迟                  */
        return  (ERROR_NONE);
    }

    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核                    */

    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  从就绪表中删除              */
    
    ptcbCur->TCB_ulDelay = ulTick;
    __ADD_TO_WAKEUP_LINE(ptcbCur);                                      /*  加入等待扫描链              */
    
    __KERNEL_TIME_GET_NO_SPINLOCK(ulKernelTime, ULONG);                 /*  记录系统时间                */
    
    if (__KERNEL_EXIT_IRQ(iregInterLevel)) {                            /*  被信号激活                  */
        if (bSigRet) {
            _ErrorHandle(EINTR);
            return  (EINTR);
        }
        ulTick = _sigTimeoutRecalc(ulKernelTime, ulTick);               /*  重新计算等待时间            */
        goto    __wait_again;                                           /*  继续等待                    */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_TimeSleepUntil
** 功能描述: 线程睡眠直到一个指定的时间 (与当前时钟差值不得超过 ULONG_MAX 个 tick)
** 输　入  : clockid           时钟类型 CLOCK_REALTIME or CLOCK_MONOTONIC
**           tv                指定的时间
**           bSigRet           是否允许信号唤醒
** 输　出  : ERROR_NONE or EINTR
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API
ULONG  API_TimeSleepUntil (clockid_t  clockid, const struct timespec  *tv, BOOL  bSigRet)
{
    struct timespec  tvNow;
    ULONG            ulTick;
    
    if ((clockid != CLOCK_REALTIME) && (clockid != CLOCK_MONOTONIC)) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    lib_clock_gettime(clockid, &tvNow);
    if (!__timespecLeftTime(&tvNow, tv)) {
        return  (ERROR_NONE);
    }
    
    ulTick = __timespecToTickDiff(&tvNow, tv);
    
    return  (API_TimeSleepEx(ulTick, bSigRet));
}
/*********************************************************************************************************
** 函数名称: API_TimeSSleep
** 功能描述: 线程睡眠函数 (精度为 TICK HZ)
** 输　入  : ulSeconds            睡眠的时间 (秒)
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
VOID    API_TimeSSleep (ULONG   ulSeconds)
{
    API_TimeSleep(ulSeconds * LW_TICK_HZ);
}
/*********************************************************************************************************
** 函数名称: API_TimeMSleep
** 功能描述: 线程睡眠函数 (精度为 TICK HZ)
** 输　入  : ulMSeconds            睡眠的时间 (毫秒)
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
VOID    API_TimeMSleep (ULONG   ulMSeconds)
{
    REGISTER ULONG      ulTicks;
    
    if (ulMSeconds == 0) {
        return;
    }
    
    ulTicks = LW_MSECOND_TO_TICK_1(ulMSeconds);
    
    API_TimeSleep(ulTicks);
}
/*********************************************************************************************************
** 函数名称: API_TimeNanoSleepMethod
** 功能描述: 设置 nanosleep 是否使用 time high resolution 方法.
** 输　入  : LW_TIME_NANOSLEEP_METHOD_TICK:        TICK 精度
**           LW_TIME_NANOSLEEP_METHOD_THRS:        time high resolution 修正 
**                                                 如果 LW_CFG_TIME_HIGH_RESOLUTION_EN > 0 则此为默认选项
** 输　出  : 之前的选项
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
INT    API_TimeNanoSleepMethod (INT  iMethod)
{
    INT   iOldMethod = _K_iNanoSleepMethod;

    if ((iMethod != LW_TIME_NANOSLEEP_METHOD_TICK) && 
        (iMethod != LW_TIME_NANOSLEEP_METHOD_THRS)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
#if LW_CFG_TIME_HIGH_RESOLUTION_EN > 0
    _K_iNanoSleepMethod = iMethod;
    
#else
    if (iMethod == LW_TIME_NANOSLEEP_METHOD_THRS) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    _K_iNanoSleepMethod = LW_TIME_NANOSLEEP_METHOD_TICK;
#endif
    
    return  (iOldMethod);
}
/*********************************************************************************************************
** 函数名称: __timeGetHighResolution
** 功能描述: 获得当前高精度时间 (使用 CLOCK_MONOTONIC)
** 输　入  : ptv       高精度时间
** 全局变量: NONE
** 调用模块: 
*********************************************************************************************************/
static VOID  __timeGetHighResolution (struct timespec  *ptv)
{
    INTREG  iregInterLevel;
    
    LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);
    *ptv = _K_tvTODMono;
#if LW_CFG_TIME_HIGH_RESOLUTION_EN > 0
    bspTickHighResolution(ptv);                                         /*  高精度时间分辨率计算        */
#endif                                                                  /*  LW_CFG_TIME_HIGH_RESOLUT... */
    LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: __timePassSpec
** 功能描述: 平静等待指定的短时间
** 输　入  : ptv       短时间
** 全局变量: NONE
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_TIME_HIGH_RESOLUTION_EN > 0

static VOID  __timePassSpec (const struct timespec  *ptv)
{
    struct timespec  tvEnd, tvNow;
    
    __timeGetHighResolution(&tvEnd);
    __timespecAdd(&tvEnd, ptv);
    do {
        __timeGetHighResolution(&tvNow);
    } while (__timespecLeftTime(&tvNow, &tvEnd));
}

#endif                                                                  /*  LW_CFG_TIME_HIGH_RESOLUT... */
/*********************************************************************************************************
** 函数名称: nanosleep 
** 功能描述: 使调用此函数的线程睡眠一个指定的时间, 睡眠过程中可能被信号唤醒. (POSIX)
** 输　入  : rqtp         睡眠的时间
**           rmtp         保存剩余时间的结构.
** 输　出  : ERROR_NONE  or  PX_ERROR

             error == EINTR    表示被信号激活.
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
INT  nanosleep (const struct timespec  *rqtp, struct timespec  *rmtp)
{
             INTREG             iregInterLevel;
             
             PLW_CLASS_TCB      ptcbCur;
    REGISTER PLW_CLASS_PCB      ppcb;
	REGISTER ULONG              ulKernelTime;
	REGISTER INT                iRetVal;
	         INT                iSchedRet;
	
	REGISTER ULONG              ulError;
             ULONG              ulTick;
             
             struct timespec    tvStart;
             struct timespec    tvTemp;
    
    if ((!rqtp) ||
        LW_NSEC_INVALD(rqtp->tv_nsec)) {                                /*  时间格式错误                */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
#if LW_CFG_TIME_HIGH_RESOLUTION_EN > 0
    if (_K_iNanoSleepMethod) {                                          /*  使用 thrs 精密延迟          */
        ulTick = __timespecToTickNoPartial(rqtp);
        if (!ulTick) {                                                  /*  不到一个 tick               */
            __timePassSpec(rqtp);                                       /*  平静度过                    */
            if (rmtp) {
                rmtp->tv_sec  = 0;                                      /*  不存在时间差别              */
                rmtp->tv_nsec = 0;
            }
            return  (ERROR_NONE);
        }
    
    } else 
#endif                                                                  /*  LW_CFG_TIME_HIGH_RESOLUT... */

    {                                                                   /*  计算 tick 精度              */
        ulTick = LW_TS_TIMEOUT_TICK(LW_TRUE, rqtp);
        if (!ulTick) {
            if (rmtp) {
                rmtp->tv_sec  = 0;                                      /*  不存在时间差别              */
                rmtp->tv_nsec = 0;
            }
            return  (ERROR_NONE);
        }
    }
    
    __timeGetHighResolution(&tvStart);                                  /*  记录开始的时间              */
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_SLEEP, 
                      ptcbCur->TCB_ulId, ulTick, LW_NULL);
    
__wait_again:
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核                    */
    
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  从就绪表中删除              */
    
    ptcbCur->TCB_ulDelay = ulTick;
    __ADD_TO_WAKEUP_LINE(ptcbCur);                                      /*  加入等待扫描链              */
    
    __KERNEL_TIME_GET_NO_SPINLOCK(ulKernelTime, ULONG);                 /*  记录系统时间                */
    
    iSchedRet = __KERNEL_EXIT_IRQ(iregInterLevel);                      /*  调度器解锁                  */
    if (iSchedRet == LW_SIGNAL_EINTR) {
        iRetVal = PX_ERROR;                                             /*  被信号激活                  */
        ulError = EINTR;
    
    } else {
        if (iSchedRet == LW_SIGNAL_RESTART) {                           /*  信号要求重启等待            */
            ulTick = _sigTimeoutRecalc(ulKernelTime, ulTick);           /*  重新计算等待时间            */
            if (ulTick != 0ul) {
                goto    __wait_again;                                   /*  重新等待剩余的 tick         */
            }
        }
        iRetVal = ERROR_NONE;                                           /*  自然唤醒                    */
        ulError = ERROR_NONE;
    }
    
    if (ulError ==  ERROR_NONE) {                                       /*  tick 已经延迟结束           */
#if LW_CFG_TIME_HIGH_RESOLUTION_EN > 0
        if (_K_iNanoSleepMethod) {                                      /*  使用 thrs 精密延迟          */
            __timeGetHighResolution(&tvTemp);
            __timespecSub(&tvTemp, &tvStart);                           /*  计算已经延迟的时间          */
            if (__timespecLeftTime(&tvTemp, rqtp)) {                    /*  还有剩余时间需要延迟        */
                struct timespec  tvNeed;
                __timespecSub2(&tvNeed, rqtp, &tvTemp);
                __timePassSpec(&tvNeed);                                /*  平静度过                    */
            }
        }
#endif                                                                  /*  LW_CFG_TIME_HIGH_RESOLUT... */
        if (rmtp) {
            rmtp->tv_sec  = 0;                                          /*  不存在时间差别              */
            rmtp->tv_nsec = 0;
        }
    
    } else {                                                            /*  信号唤醒                    */
        if (rmtp) {
            *rmtp = *rqtp;
            __timeGetHighResolution(&tvTemp);
            __timespecSub(&tvTemp, &tvStart);                           /*  计算已经延迟的时间          */
            if (__timespecLeftTime(&tvTemp, rmtp)) {                    /*  没有延迟够                  */
                __timespecSub(rmtp, &tvTemp);                           /*  计算没有延迟够的时间        */
            }
        }
    }
             
    _ErrorHandle(ulError);                                              /*  设置 errno 值               */
    return  (iRetVal);
}
/*********************************************************************************************************
** 函数名称: usleep 
** 功能描述: 使调用此函数的线程睡眠一个指定的时间(us 为单位), 睡眠过程中可能被信号唤醒. (POSIX)
** 输　入  : usecondTime       时间 (us)
** 输　出  : ERROR_NONE  or  PX_ERROR
             error == EINTR    表示被信号激活.
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
INT  usleep (usecond_t   usecondTime)
{
    struct timespec ts;

    ts.tv_sec  = (time_t)(usecondTime / 1000000);
    ts.tv_nsec = (LONG)(usecondTime % 1000000) * 1000ul;
    
    return  (nanosleep(&ts, LW_NULL));
}
/*********************************************************************************************************
** 函数名称: sleep 
** 功能描述: sleep() 会令目前的线程暂停, 直到达到参数 uiSeconds 所指定的时间, 或是被信号所唤醒. (POSIX)
** 输　入  : uiSeconds         睡眠的秒数
** 输　出  : 若进程暂停到参数 uiSeconds 所指定的时间则返回 0, 若有信号中断则返回剩余秒数.
**           error == EINTR    表示被信号激活.
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
UINT  sleep (UINT    uiSeconds)
{
    ULONG            ulTick;
    struct timespec  tsStart, tsEnd;
    UINT             uiPass;
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    tsStart.tv_sec  = uiSeconds;
    tsStart.tv_nsec = 0;
    
    ulTick = LW_TS_TIMEOUT_TICK(LW_TRUE, &tsStart);
    
    __timeGetHighResolution(&tsStart);
    if (API_TimeSleepEx(ulTick, LW_TRUE) == EINTR) {
        __timeGetHighResolution(&tsEnd);
        if (tsEnd.tv_sec >= tsStart.tv_sec) {
            uiPass = (UINT)(tsEnd.tv_sec - tsStart.tv_sec);
            if (uiPass < uiSeconds) {
                return  (uiSeconds - uiPass);
            }
        }
    }
    
    return  (0);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

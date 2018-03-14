/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: _TimeCvt.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 07 月 28 日
**
** 描        述: 时间变换.
*********************************************************************************************************/
#define  __TIMECVT_MAIN_FILE
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  方法选择, 方法 1 速度快, 适用于 292,471,208 年内, 超过这个时间会出错. 其实够用了, 本着严谨的精神,
  这里提供了更为严格的方法 2 , 到宇宙消失都有效.
*********************************************************************************************************/
/*********************************************************************************************************
  时间转换函数
*********************************************************************************************************/
static ULONG  __timespecTimeoutTickSimple(BOOL  bRel, const struct timespec  *ptv);
static INT64  __timespecTimeoutTick64Simple(BOOL  bRel, const struct timespec  *ptv);
/*********************************************************************************************************
  使用 simple 初始化
*********************************************************************************************************/
ULONG  (*_K_pfuncTimespecTimeoutTick)()   = __timespecTimeoutTickSimple;
INT64  (*_K_pfuncTimespecTimeoutTick64)() = __timespecTimeoutTick64Simple;
/*********************************************************************************************************
** 函数名称: __timespecToTickDiff
** 功能描述: 计算两个时间点只差, 并转换为 tick
** 输　入  : ptvS, ptvE     时间点开始与结束
** 输　出  : tick
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __timespecToTickDiff (const struct timespec  *ptvS, const struct timespec  *ptvE)
{
    REGISTER ULONG    ulRes = LW_TIME_BILLION / LW_TICK_HZ;

#ifdef __SYLIXOS_TIMECVT_METHOD_2
    struct   timespec tvS = *ptvS;
    struct   timespec tvE = *ptvE;
    REGISTER INT64    i64S, i64E;
             
    tvE.tv_sec -= tvS.tv_sec;
    tvS.tv_sec  = 0;
    
    i64S = LW_CONVERT_TO_TICK_NO_PARTIAL(&tvS, tv_nsec, ulRes, INT64);
    i64E = LW_CONVERT_TO_TICK(&tvE, tv_nsec, ulRes, INT64);

#else
    REGISTER INT64  i64S = LW_CONVERT_TO_TICK_NO_PARTIAL(ptvS, tv_nsec, ulRes, INT64);
    REGISTER INT64  i64E = LW_CONVERT_TO_TICK(ptvE, tv_nsec, ulRes, INT64);
#endif

    return  ((ULONG)(i64E - i64S));
}
/*********************************************************************************************************
** 函数名称: __timespecToTickDiff64
** 功能描述: 计算两个时间点只差, 并转换为 tick
** 输　入  : ptvS, ptvE     时间点开始与结束
** 输　出  : tick
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT64  __timespecToTickDiff64 (const struct timespec  *ptvS, const struct timespec  *ptvE)
{
    REGISTER ULONG    ulRes = LW_TIME_BILLION / LW_TICK_HZ;

#ifdef __SYLIXOS_TIMECVT_METHOD_2
    struct   timespec tvS = *ptvS;
    struct   timespec tvE = *ptvE;
    REGISTER INT64    i64S, i64E;
             
    tvE.tv_sec -= tvS.tv_sec;
    tvS.tv_sec  = 0;
    
    i64S = LW_CONVERT_TO_TICK_NO_PARTIAL(&tvS, tv_nsec, ulRes, INT64);
    i64E = LW_CONVERT_TO_TICK(&tvE, tv_nsec, ulRes, INT64);

#else
    REGISTER INT64  i64S = LW_CONVERT_TO_TICK_NO_PARTIAL(ptvS, tv_nsec, ulRes, INT64);
    REGISTER INT64  i64E = LW_CONVERT_TO_TICK(ptvE, tv_nsec, ulRes, INT64);
#endif

    return  (i64E - i64S);
}
/*********************************************************************************************************
** 函数名称: __timespecTimeoutTickSimple
** 功能描述: 通过 timespec 计算超时时间
** 输　入  : bRel          相对时间还是绝对时间
**           ptv           timespec
** 输　出  : tick
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ULONG  __timespecTimeoutTickSimple (BOOL  bRel, const struct timespec  *ptv)
{
    struct timespec  tvNow, tvEnd;
    REGISTER ULONG   ulTimeout;

    if (bRel) {
        ulTimeout = __timespecToTick(ptv);

    } else {
        lib_clock_gettime(CLOCK_REALTIME, &tvNow);                      /*  获得当前系统时间            */
        if (__timespecLeftTime(&tvNow, ptv)) {
            __timespecSub2(&tvEnd, ptv, &tvNow);
            ulTimeout = __timespecToTick(&tvEnd);

        } else {
            ulTimeout = LW_OPTION_NOT_WAIT;
        }
    }

    return  (ulTimeout);
}
/*********************************************************************************************************
** 函数名称: __timespecTimeoutTick
** 功能描述: 通过 timespec 计算超时时间
** 输　入  : bRel          相对时间还是绝对时间
**           ptv           timespec
** 输　出  : tick
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ULONG  __timespecTimeoutTick (BOOL  bRel, const struct timespec  *ptv)
{
    struct timespec  tvNow, tvEnd;
    REGISTER ULONG   ulTimeout;

    if (bRel) {
        lib_clock_gettime(CLOCK_REALTIME, &tvNow);                      /*  获得当前系统时间            */
        __timespecAdd2(&tvEnd, &tvNow, ptv);
        if (__timespecLeftTime(&tvNow, &tvEnd)) {
            ulTimeout = __timespecToTickDiff(&tvNow, &tvEnd);

        } else {
            ulTimeout = LW_OPTION_NOT_WAIT;
        }

    } else {
        lib_clock_gettime(CLOCK_REALTIME, &tvNow);                      /*  获得当前系统时间            */
        if (__timespecLeftTime(&tvNow, ptv)) {
            ulTimeout = __timespecToTickDiff(&tvNow, ptv);

        } else {
            ulTimeout = LW_OPTION_NOT_WAIT;
        }
    }

    return  (ulTimeout);
}
/*********************************************************************************************************
** 函数名称: __timespecTimeoutTick64Simple
** 功能描述: 通过 timespec 计算超时时间
** 输　入  : bRel          相对时间还是绝对时间
**           ptv           timespec
** 输　出  : tick
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT64  __timespecTimeoutTick64Simple (BOOL  bRel, const struct timespec  *ptv)
{
    struct timespec  tvNow, tvEnd;
    REGISTER INT64   i64Timeout;

    if (bRel) {
        i64Timeout = __timespecToTick64(ptv);

    } else {
        lib_clock_gettime(CLOCK_REALTIME, &tvNow);                      /*  获得当前系统时间            */
        if (__timespecLeftTime(&tvNow, ptv)) {
            __timespecSub2(&tvEnd, ptv, &tvNow);
            i64Timeout = __timespecToTick64(&tvEnd);

        } else {
            i64Timeout = LW_OPTION_NOT_WAIT;
        }
    }

    return  (i64Timeout);
}
/*********************************************************************************************************
** 函数名称: __timespecTimeoutTick64
** 功能描述: 通过 timespec 计算超时时间
** 输　入  : bRel          相对时间还是绝对时间
**           ptv           timespec
** 输　出  : tick
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT64  __timespecTimeoutTick64 (BOOL  bRel, const struct timespec  *ptv)
{
    struct timespec  tvNow, tvEnd;
    REGISTER INT64   i64Timeout;

    if (bRel) {
        lib_clock_gettime(CLOCK_REALTIME, &tvNow);                      /*  获得当前系统时间            */
        __timespecAdd2(&tvEnd, &tvNow, ptv);
        if (__timespecLeftTime(&tvNow, &tvEnd)) {
            i64Timeout = __timespecToTickDiff64(&tvNow, &tvEnd);

        } else {
            i64Timeout = LW_OPTION_NOT_WAIT;
        }

    } else {
        lib_clock_gettime(CLOCK_REALTIME, &tvNow);                      /*  获得当前系统时间            */
        if (__timespecLeftTime(&tvNow, ptv)) {
            i64Timeout = __timespecToTickDiff64(&tvNow, ptv);

        } else {
            i64Timeout = LW_OPTION_NOT_WAIT;
        }
    }

    return  (i64Timeout);
}
/*********************************************************************************************************
** 函数名称: __timeCvtInit
** 功能描述: 时间转换函数初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _TimeCvtInit (VOID)
{
    if (!LW_KERN_TMCVT_SIMPLE_EN_GET()) {
        _K_pfuncTimespecTimeoutTick   = __timespecTimeoutTick;
        _K_pfuncTimespecTimeoutTick64 = __timespecTimeoutTick64;
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

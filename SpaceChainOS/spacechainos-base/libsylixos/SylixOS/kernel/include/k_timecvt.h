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
** 文   件   名: k_timecvt.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 07 月 28 日
**
** 描        述: 时间变换.
*********************************************************************************************************/

#ifndef __K_TIMECVT_H
#define __K_TIMECVT_H

/*********************************************************************************************************
  time invald
*********************************************************************************************************/

#define LW_NSEC_INVALD(nsec)    (((nsec) < 0) || ((nsec) >= __TIMEVAL_NSEC_MAX))
#define LW_USEC_INVALD(usec)    (((usec) < 0) || ((usec) >= __TIMEVAL_USEC_MAX))

/*********************************************************************************************************
  million and billion
*********************************************************************************************************/

#define LW_TIME_MILLION     1000000
#define LW_TIME_BILLION     1000000000

#define LW_CONVERT_TO_TICK_NO_PARTIAL(pt, unsec, res, type) \
        (type)(((pt)->tv_sec * LW_TICK_HZ) + \
               ((INT64)(pt)->unsec / res))

#define LW_CONVERT_TO_TICK(pt, unsec, res, type) \
        (type)(((pt)->tv_sec * LW_TICK_HZ) + \
               ((INT64)(pt)->unsec / res) + \
               (((INT64)(pt)->unsec % res) ? 1 : 0))

/*********************************************************************************************************
  POSIX TIME timeval to Tick
*********************************************************************************************************/

static LW_INLINE ULONG  __timevalToTickNoPartial (const struct timeval  *ptv)
{
    REGISTER ULONG  ulRes = LW_TIME_MILLION / LW_TICK_HZ;

    return  (LW_CONVERT_TO_TICK_NO_PARTIAL(ptv, tv_usec, ulRes, ULONG));
}

static LW_INLINE ULONG  __timevalToTick (const struct timeval  *ptv)
{
    REGISTER ULONG  ulRes = LW_TIME_MILLION / LW_TICK_HZ;
    
    return  (LW_CONVERT_TO_TICK(ptv, tv_usec, ulRes, ULONG));
}

static LW_INLINE INT64  __timevalToTick64 (const struct timeval  *ptv)
{
    REGISTER ULONG  ulRes = LW_TIME_MILLION / LW_TICK_HZ;
    
    return  (LW_CONVERT_TO_TICK(ptv, tv_usec, ulRes, INT64));
}

/*********************************************************************************************************
  POSIX TIME timespec to Tick
*********************************************************************************************************/

static LW_INLINE ULONG  __timespecToTickNoPartial (const struct timespec  *ptv)
{
    REGISTER ULONG  ulRes = LW_TIME_BILLION / LW_TICK_HZ;

    return  (LW_CONVERT_TO_TICK_NO_PARTIAL(ptv, tv_nsec, ulRes, ULONG));
}

static LW_INLINE ULONG  __timespecToTick (const struct timespec  *ptv)
{
    REGISTER ULONG  ulRes = LW_TIME_BILLION / LW_TICK_HZ;
    
    return  (LW_CONVERT_TO_TICK(ptv, tv_nsec, ulRes, ULONG));
}

static LW_INLINE INT64  __timespecToTick64 (const struct timespec  *ptv)
{
    REGISTER ULONG  ulRes = LW_TIME_BILLION / LW_TICK_HZ;
    
    return  (LW_CONVERT_TO_TICK(ptv, tv_nsec, ulRes, INT64));
}

/*********************************************************************************************************
  POSIX TIME Tick to timeval 
*********************************************************************************************************/

static LW_INLINE VOID  __tickToTimeval (ULONG  ulTicks, struct timeval  *ptv)
{
    ptv->tv_sec  = (time_t)(ulTicks / LW_TICK_HZ);
    ptv->tv_usec = (LONG)(ulTicks % LW_TICK_HZ) * (LW_TIME_MILLION / LW_TICK_HZ);
}

static LW_INLINE VOID  __tick64ToTimeval (INT64  i64Ticks, struct timeval  *ptv)
{
    ptv->tv_sec  = (time_t)(i64Ticks / LW_TICK_HZ);
    ptv->tv_usec = (LONG)(i64Ticks % LW_TICK_HZ) * (LW_TIME_MILLION / LW_TICK_HZ);
}

/*********************************************************************************************************
  POSIX TIME Tick to timespec 
*********************************************************************************************************/

static LW_INLINE VOID  __tickToTimespec (ULONG  ulTicks, struct timespec  *ptv)
{
    ptv->tv_sec  = (time_t)(ulTicks / LW_TICK_HZ);
    ptv->tv_nsec = (LONG)(ulTicks % LW_TICK_HZ) * (LW_TIME_BILLION / LW_TICK_HZ);
}

static LW_INLINE VOID  __tick64ToTimespec (INT64  i64Ticks, struct timespec  *ptv)
{
    ptv->tv_sec  = (time_t)(i64Ticks / LW_TICK_HZ);
    ptv->tv_nsec = (LONG)(i64Ticks % LW_TICK_HZ) * (LW_TIME_BILLION / LW_TICK_HZ);
}

/*********************************************************************************************************
  POSIX TIME add two struct timespec
*********************************************************************************************************/

static LW_INLINE VOID  __timespecAdd (struct timespec  *ptv1, const struct timespec  *ptv2)
{
    ptv1->tv_sec  += ptv2->tv_sec;
    ptv1->tv_nsec += ptv2->tv_nsec;
    
    if (ptv1->tv_nsec >= __TIMEVAL_NSEC_MAX) {
        ptv1->tv_sec++;
        ptv1->tv_nsec -= __TIMEVAL_NSEC_MAX;
    }
}

static LW_INLINE VOID  __timespecAdd2 (struct timespec        *ptv, 
                                       const struct timespec  *ptv1, 
                                       const struct timespec  *ptv2)
{
    ptv->tv_sec  = ptv1->tv_sec  + ptv2->tv_sec;
    ptv->tv_nsec = ptv1->tv_nsec + ptv2->tv_nsec;
    
    if (ptv->tv_nsec >= __TIMEVAL_NSEC_MAX) {
        ptv->tv_sec++;
        ptv->tv_nsec -= __TIMEVAL_NSEC_MAX;
    }
}

/*********************************************************************************************************
  POSIX TIME sub two struct timespec
*********************************************************************************************************/

static LW_INLINE VOID  __timespecSub (struct timespec  *ptv1, const struct timespec  *ptv2)
{
    ptv1->tv_sec  -= ptv2->tv_sec;
    ptv1->tv_nsec -= ptv2->tv_nsec;
    
    if (ptv1->tv_nsec >= __TIMEVAL_NSEC_MAX) {
        ptv1->tv_sec++; 
        ptv1->tv_nsec -= __TIMEVAL_NSEC_MAX;
    
    } else if (ptv1->tv_nsec < 0) {
        ptv1->tv_sec--; 
        ptv1->tv_nsec += __TIMEVAL_NSEC_MAX;
    }
}

static LW_INLINE VOID  __timespecSub2 (struct timespec        *ptv,
                                       const struct timespec  *ptv1, 
                                       const struct timespec  *ptv2)
{
    ptv->tv_sec  = ptv1->tv_sec  - ptv2->tv_sec;
    ptv->tv_nsec = ptv1->tv_nsec - ptv2->tv_nsec;
    
    if (ptv->tv_nsec >= __TIMEVAL_NSEC_MAX) {
        ptv->tv_sec++; 
        ptv->tv_nsec -= __TIMEVAL_NSEC_MAX;
    
    } else if (ptv->tv_nsec < 0) {
        ptv->tv_sec--; 
        ptv->tv_nsec += __TIMEVAL_NSEC_MAX;
    }
}

/*********************************************************************************************************
  POSIX TIME check if has left time
*********************************************************************************************************/

static LW_INLINE INT  __timespecLeftTime (const struct timespec  *ptv1, const struct timespec  *ptv2)
{
    return  (ptv1->tv_sec < ptv2->tv_sec ||
             (ptv1->tv_sec == ptv2->tv_sec && ptv1->tv_nsec < ptv2->tv_nsec));
}

/*********************************************************************************************************
  Macros for converting between `struct timeval' and `struct timespec'.
*********************************************************************************************************/

#define LW_TIMEVAL_TO_TIMESPEC(tv, ts) {        \
        (ts)->tv_sec  = (tv)->tv_sec;           \
        (ts)->tv_nsec = (tv)->tv_usec * 1000;   \
}

#define LW_TIMESPEC_TO_TIMEVAL(tv, ts) {        \
        (tv)->tv_sec  = (ts)->tv_sec;           \
        (tv)->tv_usec = (ts)->tv_nsec / 1000;   \
}

/*********************************************************************************************************
  POSIX timeout to tick
*********************************************************************************************************/

ULONG  __timespecToTickDiff(const struct timespec  *ptvS, const struct timespec  *ptvE);
INT64  __timespecToTickDiff64(const struct timespec  *ptvS, const struct timespec  *ptvE);

#ifndef __TIMECVT_MAIN_FILE
extern ULONG  (*_K_pfuncTimespecTimeoutTick)();
extern INT64  (*_K_pfuncTimespecTimeoutTick64)();
#endif

#define LW_TS_TIMEOUT_TICK(bRel, ptv)       _K_pfuncTimespecTimeoutTick(bRel, ptv)
#define LW_TS_TIMEOUT_TICK64(bRel, ptv)     _K_pfuncTimespecTimeoutTick64(bRel, ptv)

#endif                                                                  /*  __K_TIMECVT_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/

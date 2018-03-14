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
** 文   件   名: time.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 04 月 02 日
**
** 描        述: 兼容 C 库.
*********************************************************************************************************/

#ifndef __TIME_H
#define __TIME_H

#ifndef __SYLIXOS_H
#include "SylixOS.h"
#endif                                                                  /*  __SYLIXOS_H                 */

#ifdef __cplusplus
extern "C" {
#endif

__BEGIN_NAMESPACE_STD

extern time_t          timezone;                                        /*  UTC 时间与本地时间秒差      */
                                                                        /*  (timezone = UTC - LOCAL)    */
extern char           *tzname[2];

#if LW_CFG_RTC_EN > 0

__LW_RETU_FUNC_DECLARE(time_t, timelocal, (time_t *time))
__LW_RETU_FUNC_DECLARE(char *, ctime, (const time_t *time))
__LW_RETU_FUNC_DECLARE(char *, ctime_r, (const time_t *time, char *pcBuffer))

__LW_RETU_FUNC_DECLARE(char *, asctime_r, (const struct tm *ptm, char *pcBuffer))
__LW_RETU_FUNC_DECLARE(char *, asctime, (const struct tm *ptm))

__LW_RETU_FUNC_DECLARE(struct tm *, gmtime_r, (const time_t *time, struct tm *ptmBuffer))
__LW_RETU_FUNC_DECLARE(struct tm *, gmtime, (const time_t *time))

__LW_RETU_FUNC_DECLARE(struct tm *, localtime_r, (const time_t *time, struct tm *ptmBuffer))
__LW_RETU_FUNC_DECLARE(struct tm *, localtime, (const time_t *time))

__LW_RETU_FUNC_DECLARE(time_t, mktime, (struct tm *ptm))
__LW_RETU_FUNC_DECLARE(time_t, timegm, (struct tm *ptm))

__LW_RETU_FUNC_DECLARE(double, difftime, (time_t time1, time_t time2))

__LW_RETU_FUNC_DECLARE(clock_t, clock, (void))
__LW_RETU_FUNC_DECLARE(int, clock_getcpuclockid, (pid_t pid, clockid_t *clock_id))
__LW_RETU_FUNC_DECLARE(int, clock_getres, (clockid_t  clockid, struct timespec *res))
__LW_RETU_FUNC_DECLARE(int, clock_gettime, (clockid_t  clockid, struct timespec  *tv))
__LW_RETU_FUNC_DECLARE(int, clock_settime, (clockid_t  clockid, const struct timespec  *tv))
__LW_RETU_FUNC_DECLARE(int, clock_nanosleep, (clockid_t  clockid, int  iFlags, 
                            const struct timespec  *rqtp, struct timespec  *rmtp))

__LW_RETU_FUNC_DECLARE(int, gettimeofday, (struct timeval *tv, struct timezone *tz))
__LW_RETU_FUNC_DECLARE(int, settimeofday, (const struct timeval *tv, const struct timezone *tz))

__LW_RETU_FUNC_DECLARE(void, tzset, (void))

__LW_RETU_FUNC_DECLARE(hrtime_t, gethrtime, (void))
__LW_RETU_FUNC_DECLARE(hrtime_t, gethrvtime, (void))

__LW_RETU_FUNC_DECLARE(size_t, strftime, (char *s, size_t maxsize, const char *format, const struct tm *t))
__LW_RETU_FUNC_DECLARE(char *, strptime, (const char *buf, const char *fmt, struct tm *tm))

/*********************************************************************************************************
  POSIX library
*********************************************************************************************************/

LW_API int  adjtime(const struct timeval *delta, struct timeval *olddelta);

#endif                                                                  /*  LW_CFG_RTC_EN               */

__END_NAMESPACE_STD

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __TIME_H                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/

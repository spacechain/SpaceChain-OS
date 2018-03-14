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
** 文   件   名: ptimer.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 04 月 13 日
**
** 描        述: posix 兼容时间库.
*********************************************************************************************************/

#ifndef __PTIMER_H
#define __PTIMER_H

/*********************************************************************************************************
  posix timer_getoverrun() 最大值
*********************************************************************************************************/

#ifndef DELAYTIMER_MAX
#define DELAYTIMER_MAX      __ARCH_INT_MAX
#endif                                                                  /*  DELAYTIMER_MAX              */

/*********************************************************************************************************
  posix 定时器定时时钟其它选项
*********************************************************************************************************/

#define TIMER_ABSTIME       1                                           /*  绝对时钟                    */

#if LW_CFG_GJB7714_EN > 0
#define TIMER_RELATIVE_C    0
#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */

/*********************************************************************************************************
  posix 睡眠函数
*********************************************************************************************************/

LW_API UINT         sleep(UINT    uiSeconds);
LW_API INT          usleep(usecond_t   usecondTime);
LW_API INT          nanosleep(const struct timespec  *rqtp, struct timespec  *rmtp);

/*********************************************************************************************************
  posix 定时器函数
*********************************************************************************************************/

#if LW_CFG_PTIMER_EN > 0

LW_API INT          timer_create(clockid_t  clockid, struct sigevent *sigeventT, timer_t *ptimer);
LW_API INT          timer_delete(timer_t  timer);
LW_API INT          timer_gettime(timer_t  timer, struct itimerspec  *ptvTime);
LW_API INT          timer_getoverrun(timer_t  timer);
LW_API INT          timer_settime(timer_t  timer, INT  iFlag, 
                                  const struct itimerspec *ptvNew,
                                  struct itimerspec       *ptvOld);

/*********************************************************************************************************
  posix 进程定时器
*********************************************************************************************************/

#if LW_CFG_MODULELOADER_EN > 0

LW_API INT          setitimer(INT iWhich, const struct itimerval *pitValue, struct itimerval *pitOld);
LW_API INT          getitimer(INT iWhich, struct itimerval *pitValue);

LW_API UINT         alarm(UINT  uiSeconds);
LW_API useconds_t   ualarm(useconds_t usec, useconds_t usecInterval);

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

/*********************************************************************************************************
  posix 定时器内部扩展接口
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
#if LW_CFG_TIMERFD_EN > 0
INT  timer_create_internal(clockid_t  clockid, struct sigevent *sigeventT, 
                           timer_t *ptimer, ULONG  ulOption);
INT  timer_setfile(timer_t  timer, PVOID  pvFile);
INT  timer_getoverrun_64(timer_t  timer, UINT64  *pu64Overruns, BOOL  bClear);
#endif                                                                  /*  LW_CFG_TIMERFD_EN > 0       */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#endif                                                                  /*  LW_CFG_PTIMER_EN > 0        */

#endif                                                                  /*  __PTIMER_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: hstimerfd.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 03 日
**
** 描        述: 高频率定时器设备用户访问接口.
*********************************************************************************************************/

#ifndef __SYS_HSTIMERFD_H
#define __SYS_HSTIMERFD_H

#include "unistd.h"

/*********************************************************************************************************
 hstimerfd_create() flags
*********************************************************************************************************/

#define HSTFD_CLOEXEC       O_CLOEXEC
#define HSTFD_NONBLOCK      O_NONBLOCK

/*********************************************************************************************************
 hstimerfd_settime2() and hstimerfd_gettime2 arguments type
*********************************************************************************************************/

typedef struct hstimer_cnt {
    unsigned long   value;
    unsigned long   interval;
} hstimer_cnt_t;

/*********************************************************************************************************
 hstimerfd api
*********************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

int hstimerfd_create(int flags);
int hstimerfd_settime(int fd, const struct itimerspec *ntmr, struct itimerspec *otmr);
int hstimerfd_settime2(int fd, hstimer_cnt_t *ncnt, hstimer_cnt_t *ocnt);
int hstimerfd_gettime(int fd, struct itimerspec *currvalue);
int hstimerfd_gettime2(int fd, hstimer_cnt_t *currvalue);
int hstimerfd_hz(void);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __SYS_HSTIMERFD_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/

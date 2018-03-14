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
** 文   件   名: utmp.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 01 月 17 日
**
** 描        述: 用户数据库, 需要外部库支持.
*********************************************************************************************************/

#ifndef __UTMP_H
#define __UTMP_H

#include "sys/types.h"

#define _PATH_UTMP          "/var/run/utmp"
#define _PATH_WTMP          "/var/log/wtmp"
#define _PATH_LASTLOG       "/var/log/lastlog"

#define UT_NAMESIZE         8
#define UT_LINESIZE         8
#define UT_HOSTSIZE         16

struct lastlog {
    time_t      ll_time;
    char        ll_line[UT_LINESIZE];
    char        ll_host[UT_HOSTSIZE];
};

struct utmp {
    char        ut_line[UT_LINESIZE];
    char        ut_name[UT_NAMESIZE];
    char        ut_host[UT_HOSTSIZE];
    time_t      ut_time;
};

__BEGIN_DECLS

int             utmpname(const char *);
void            setutent(void);
struct utmp    *getutent(void);
void            endutent(void);

__END_DECLS

#endif                                                                  /*  __UTMP_H                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: utmpx.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 01 月 17 日
**
** 描        述: 用户数据库, 需要外部库支持.
*********************************************************************************************************/

#ifndef __UTMPX_H
#define __UTMPX_H

#include "sys/cdefs.h"
#include "sys/socket.h"
#include "sys/time.h"

#define _PATH_UTMPX         "/var/run/utmpx"
#define _PATH_WTMPX         "/var/log/wtmpx"
#define _PATH_LASTLOGX      "/var/log/lastlogx"
#define _PATH_UTMP_UPDATE   "/usr/libexec/utmp_update"

#define _UTX_USERSIZE       32
#define _UTX_LINESIZE       32
#define _UTX_IDSIZE         4
#define _UTX_HOSTSIZE       256

#if defined(_NETBSD_SOURCE)
#define UTX_USERSIZE        _UTX_USERSIZE
#define UTX_LINESIZE        _UTX_LINESIZE
#define UTX_IDSIZE          _UTX_IDSIZE
#define UTX_HOSTSIZE        _UTX_HOSTSIZE
#endif

#define EMPTY               0
#define RUN_LVL             1
#define BOOT_TIME           2
#define OLD_TIME            3
#define NEW_TIME            4
#define INIT_PROCESS        5
#define LOGIN_PROCESS       6
#define USER_PROCESS        7
#define DEAD_PROCESS        8

#if defined(_NETBSD_SOURCE)
#define ACCOUNTING          9
#define SIGNATURE           10
#define DOWN_TIME           11

/*********************************************************************************************************
 Strings placed in the ut_line field to indicate special type entries
*********************************************************************************************************/

#define RUNLVL_MSG          "run-level %c"
#define BOOT_MSG            "system boot"
#define OTIME_MSG           "old time"
#define NTIME_MSG           "new time"
#define DOWN_MSG            "system down"
#endif

/*********************************************************************************************************
 The following structure describes the fields of the utmpx entries
 stored in _PATH_UTMPX or _PATH_WTMPX. This is not the format the
 entries are stored in the files, and application should only access
 entries using routines described in getutxent(3).
*********************************************************************************************************/

#define ut_user             ut_name
#define ut_xtime            ut_tv.tv_sec

struct utmpx {
    char                    ut_name[_UTX_USERSIZE];                     /* login name                   */
    char                    ut_id[_UTX_IDSIZE];                         /* inittab id                   */
    char                    ut_line[_UTX_LINESIZE];                     /* tty name                     */
    char                    ut_host[_UTX_HOSTSIZE];                     /* host name                    */
    uint16_t                ut_session;                                 /* session id used for windowing*/
    uint16_t                ut_type;                                    /* type of this entry           */
    pid_t                   ut_pid;                                     /* process id creating the entry*/
    struct {
        uint16_t            e_termination;                              /* process termination signal   */
        uint16_t            e_exit;                                     /* process exit status          */
    } ut_exit;
    struct sockaddr_storage ut_ss;                                      /* where entry was made from    */
    struct timeval          ut_tv;                                      /* time entry was created       */
    uint32_t                ut_pad[10];                                 /* reserved for future use      */
};

#if defined(_NETBSD_SOURCE)
struct lastlogx {
    struct timeval          ll_tv;                                      /* time entry was created       */
    char                    ll_line[_UTX_LINESIZE];                     /* tty name                     */
    char                    ll_host[_UTX_HOSTSIZE];                     /* host name                    */
    struct sockaddr_storage ll_ss;                                      /* where entry was made from    */
};
#endif                                                                  /* _NETBSD_SOURCE */

__BEGIN_DECLS

void                 setutxent(void);
void                 endutxent(void);
struct utmpx        *getutxent(void);
struct utmpx        *getutxid(const struct utmpx *);
struct utmpx        *getutxline(const struct utmpx *);
struct utmpx        *pututxline(const struct utmpx *);

#if defined(_NETBSD_SOURCE)
int                  updwtmpx(const char *, const struct utmpx *);
#ifndef __LIBC12_SOURCE__
struct lastlogx     *getlastlogx(const char *, uid_t, struct lastlogx *);
#endif
int                  updlastlogx(const char *, uid_t, struct lastlogx *);
struct utmp;
void                 getutmp(const struct utmpx *, struct utmp *);
void                 getutmpx(const struct utmp *, struct utmpx *);

int                  utmpxname(const char *);
#endif /* _NETBSD_SOURCE */

__END_DECLS

#endif                                                                  /*  __UTMPX_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/

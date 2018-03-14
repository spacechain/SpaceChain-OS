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
** 文   件   名: limits.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 04 月 02 日
**
** 描        述: 兼容 C 库.
*********************************************************************************************************/

#ifndef __LIMITS_H
#define __LIMITS_H

#ifdef  __SYLIXOS_KERNEL
#define __SYLIXOS_LIMITS
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#ifndef __SYLIXOS_H
#include "SylixOS.h"
#else
#include "../SylixOS/lib/libc/limits/lib_limits.h"
#endif                                                                  /*  __SYLIXOS_H                 */

/*********************************************************************************************************
  POSIX limits
*********************************************************************************************************/

#ifndef SSIZE_MAX
#define SSIZE_MAX                           LONG_MAX
#endif

#define AIO_LISTIO_MAX                      LONG_MAX
#define AIO_MAX                             LONG_MAX
#define AIO_PRIO_DELTA_MAX                  0
#define ARG_MAX                             LW_CFG_SHELL_MAX_KEYWORDLEN
#define ATEXIT_MAX                          INT_MAX
#define CHILD_MAX                           0
#define IOV_MAX                             LONG_MAX
#define LOGIN_NAME_MAX                      PATH_MAX
#define MQ_OPEN_MAX                         (LW_CFG_MAX_EVENTS / 3)
#define PAGESIZE                            LW_CFG_VMM_PAGE_SIZE

#ifndef PAGE_SIZE
#define PAGE_SIZE                           PAGESIZE
#endif

#define LINK_MAX                            10
#define MAX_CANON                           255
#define MAX_INPUT                           255
#define LINE_MAX                            LW_CFG_SHELL_MAX_COMMANDLEN
#define NAME_MAX                            PATH_MAX
#define TZNAME_MAX                          10
#define NGROUPS_MAX                         16

#define PTHREAD_DESTRUCTOR_ITERATIONS       LONG_MAX
#define PTHREAD_KEYS_MAX                    LONG_MAX
#define PTHREAD_STACK_MIN                   LW_CFG_PTHREAD_MIN_STK_SIZE
#define PTHREAD_THREADS_MAX                 LW_CFG_MAX_THREADS
#define RTSIG_MAX                           (SIGRTMAX - SIGRTMIN)
#define SEM_NSEMS_MAX                       LW_CFG_MAX_EVENTS
#define SIGQUEUE_MAX                        LW_CFG_MAX_MSGQUEUES
#define STREAM_MAX                          0
#define PIPE_BUF                            4096

#define HOST_NAME_MAX                       64
#define FILESIZEBITS                        64

#define PASS_MAX                            128

/*********************************************************************************************************
  POSIX values
  Maximum Values
*********************************************************************************************************/

#define _POSIX_CLOCKRES_MIN                 20000000
#define _POSIX_AIO_LISTIO_MAX               AIO_LISTIO_MAX
#define _POSIX_AIO_MAX                      AIO_MAX
#define _POSIX_ARG_MAX                      ARG_MAX
#define _POSIX_CHILD_MAX                    25
#define _POSIX_DELAYTIMER_MAX               32
#define _POSIX_HOST_NAME_MAX                HOST_NAME_MAX
#define _POSIX_LINK_MAX                     LINK_MAX
#define _POSIX_LOGIN_NAME_MAX               LOGIN_NAME_MAX

#define _POSIX_MAX_CANON                    MAX_CANON
#define _POSIX_MAX_INPUT                    MAX_INPUT
#define _POSIX_MQ_OPEN_MAX                  MQ_OPEN_MAX
#define _POSIX_MQ_PRIO_MAX                  32                                  /*  MQ_PRIO_MAX         */
#define _POSIX_NAME_MAX                     NAME_MAX
#define _POSIX_NGROUPS_MAX                  NGROUPS_MAX
#define _POSIX_OPEN_MAX                     LW_CFG_MAX_FILES
#define _POSIX_PATH_MAX                     OPEN_MAX
#define _POSIX_PIPE_BUF                     PIPE_BUF
#define _POSIX_RE_DUP_MAX                   255
#define _POSIX_RTSIG_MAX                    8
#define _POSIX_SEM_NSEMS_MAX                LW_CFG_MAX_EVENTS
#define _POSIX_SEM_VALUE_MAX                INT_MAX
#define _POSIX_SIGQUEUE_MAX                 LW_CFG_MAX_SIGQUEUE_NODES
#define _POSIX_SSIZE_MAX                    32767
#define _POSIX_STREAM_MAX                   8
#define _POSIX_SS_REPL_MAX                  4
#define _POSIX_SYMLINK_MAX                  LINK_MAX
#define _POSIX_SYMLOOP_MAX                  LINK_MAX
#define _POSIX_THREAD_DESTRUCTOR_ITERATIONS 4
#define _POSIX_THREAD_KEYS_MAX              128
#define _POSIX_THREAD_THREADS_MAX           LW_CFG_MAX_THREADS
#define _POSIX_TIMER_MAX                    LW_CFG_MAX_TIMERS
#define _POSIX_TRACE_EVENT_NAME_MAX         30
#define _POSIX_TRACE_NAME_MAX               8
#define _POSIX_TRACE_SYS_MAX                8
#define _POSIX_TRACE_USER_EVENT_MAX         32
#define _POSIX_TTY_NAME_MAX                 9
#define _POSIX_TZNAME_MAX                   TZNAME_MAX
#define _POSIX_NO_TRUNC                     (-1)
#define _POSIX_CHOWN_RESTRICTED             1
#define _POSIX_VDISABLE	                    ((unsigned char)'\377')

#define _POSIX2_BC_BASE_MAX                 99
#define _POSIX2_BC_DIM_MAX                  2048
#define _POSIX2_BC_SCALE_MAX                99
#define _POSIX2_BC_STRING_MAX               1000
#define _POSIX2_COLL_WEIGHTS_MAX            2
#define _POSIX2_EXPR_NEST_MAX               32
#define _POSIX2_LINE_MAX                    LINE_MAX
#define _POSIX2_SYMLINKS                    LINK_MAX
#define _POSIX2_RE_DUP_MAX                  255
 
#define _XOPEN_IOV_MAX                      16
#define _XOPEN_NAME_MAX                     NAME_MAX
#define _XOPEN_PATH_MAX                     PATH_MAX

/*********************************************************************************************************
  BSD Like
*********************************************************************************************************/

#define NL_LANGMAX                          31                          /** max LANG name length        */
#define NL_NMAX                             1

#define MB_LEN_MAX                          5                           /** max number bytes in wchar   */

#endif                                                                  /*  __LIMITS_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

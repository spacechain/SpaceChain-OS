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
** 文   件   名: unistd.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 01 月 07 日
**
** 描        述: unix 兼容库.
*********************************************************************************************************/

#ifndef __UNISTD_H
#define __UNISTD_H

#ifdef  __SYLIXOS_KERNEL
#define __SYLIXOS_STDIO
#endif                                                                  /*  __SYLIXOS_KERNEL            */

/*********************************************************************************************************
  The following relate to configurable pathname variables. POSIX Table 5-2
*********************************************************************************************************/

#define _PC_LINK_MAX                1
#define _PC_MAX_CANON               2
#define _PC_MAX_INPUT               3
#define _PC_NAME_MAX                4
#define _PC_PATH_MAX                5
#define _PC_PIPE_BUF                6
#define _PC_CHOWN_RESTRICTED        7
#define _PC_NO_TRUNC                8
#define _PC_VDISABLE                9
#define _PC_SYNC_IO                 10
#define _PC_ASYNC_IO                11
#define _PC_PRIO_IO                 12
#define _PC_SOCK_MAXBUF             13
#define _PC_FILESIZEBITS            14
#define _PC_REC_INCR_XFER_SIZE      15
#define _PC_REC_MAX_XFER_SIZE       16
#define _PC_REC_MIN_XFER_SIZE       17
#define _PC_REC_XFER_ALIGN          18
#define _PC_ALLOC_SIZE_MIN          19
#define _PC_SYMLINK_MAX             20
#define _PC_2_SYMLINKS              21

/*********************************************************************************************************
  POSIX-style system configuration variable accessors (for the
  sysconf function).  The kernel does not directly implement the
  sysconf() interface; rather, a C library stub translates references
  to sysconf() into calls to sysctl() using a giant switch statement.
  Those that are marked `user' are implemented entirely in the C
  library and never query the kernel.  pathconf() is implemented
  directly by the kernel so those are not defined here.
*********************************************************************************************************/

#define _SC_ARG_MAX                 1
#define _SC_CHILD_MAX               2
#define _SC_CLK_TCK                 3
#define _SC_NGROUPS_MAX             4
#define _SC_OPEN_MAX                5
#define _SC_JOB_CONTROL             6
#define _SC_SAVED_IDS               7
#define _SC_VERSION                 8
#define _SC_BC_BASE_MAX             9                                   /*  user                        */
#define _SC_BC_DIM_MAX              10                                  /*  user                        */
#define _SC_BC_SCALE_MAX            11                                  /*  user                        */
#define _SC_BC_STRING_MAX           12                                  /*  user                        */
#define _SC_COLL_WEIGHTS_MAX        13                                  /*  user                        */
#define _SC_EXPR_NEST_MAX           14                                  /*  user                        */
#define _SC_LINE_MAX                15                                  /*  user                        */
#define _SC_RE_DUP_MAX              16                                  /*  user                        */
#define _SC_2_VERSION               17                                  /*  user                        */
#define _SC_2_C_BIND                18                                  /*  user                        */
#define _SC_2_C_DEV                 19                                  /*  user                        */
#define _SC_2_CHAR_TERM             20                                  /*  user                        */
#define _SC_2_FORT_DEV              21                                  /*  user                        */
#define _SC_2_FORT_RUN              22                                  /*  user                        */
#define _SC_2_LOCALEDEF             23                                  /*  user                        */
#define _SC_2_SW_DEV                24                                  /*  user                        */
#define _SC_2_UPE                   25                                  /*  user                        */
#define _SC_STREAM_MAX              26                                  /*  user                        */
#define _SC_TZNAME_MAX              27                                  /*  user                        */

#define _SC_ASYNCHRONOUS_IO         28
#define _SC_MAPPED_FILES            29
#define _SC_MEMLOCK                 30
#define _SC_MEMLOCK_RANGE           31
#define _SC_MEMORY_PROTECTION       32
#define _SC_MESSAGE_PASSING         33
#define _SC_PRIORITIZED_IO          34
#define _SC_PRIORITY_SCHEDULING     35
#define _SC_REALTIME_SIGNALS        36
#define _SC_SEMAPHORES              37
#define _SC_FSYNC                   38
#define _SC_SHARED_MEMORY_OBJECTS   39
#define _SC_SYNCHRONIZED_IO         40
#define _SC_TIMERS                  41
#define _SC_AIO_LISTIO_MAX          42
#define _SC_AIO_MAX                 43
#define _SC_AIO_PRIO_DELTA_MAX      44
#define _SC_DELAYTIMER_MAX          45
#define _SC_MQ_OPEN_MAX             46
#define _SC_PAGESIZE                47
#define _SC_RTSIG_MAX               48
#define _SC_SEM_NSEMS_MAX           49
#define _SC_SEM_VALUE_MAX           50
#define _SC_SIGQUEUE_MAX            51
#define _SC_TIMER_MAX               52

#define _SC_2_PBS                   59                                  /*  user                        */
#define _SC_2_PBS_ACCOUNTING        60                                  /*  user                        */
#define _SC_2_PBS_CHECKPOINT        61                                  /*  user                        */
#define _SC_2_PBS_LOCATE            62                                  /*  user                        */
#define _SC_2_PBS_MESSAGE           63                                  /*  user                        */
#define _SC_2_PBS_TRACK             64                                  /*  user                        */
#define _SC_ADVISORY_INFO           65
#define _SC_BARRIERS                66                                  /*  user                        */
#define _SC_CLOCK_SELECTION         67
#define _SC_CPUTIME                 68
#define _SC_FILE_LOCKING            69
#define _SC_GETGR_R_SIZE_MAX        70                                  /*  user                        */
#define _SC_GETPW_R_SIZE_MAX        71                                  /*  user                        */
#define _SC_HOST_NAME_MAX           72
#define _SC_LOGIN_NAME_MAX          73
#define _SC_MONOTONIC_CLOCK         74
#define _SC_MQ_PRIO_MAX             75
#define _SC_READER_WRITER_LOCKS     76                                  /*  user                        */
#define _SC_REGEXP                  77                                  /*  user                        */
#define _SC_SHELL                   78                                  /*  user                        */
#define _SC_SPAWN                   79                                  /*  user                        */
#define _SC_SPIN_LOCKS              80                                  /*  user                        */
#define _SC_SPORADIC_SERVER         81
#define _SC_THREAD_ATTR_STACKADDR   82                                  /*  user                        */
#define _SC_THREAD_ATTR_STACKSIZE   83                                  /*  user                        */
#define _SC_THREAD_CPUTIME          84                                  /*  user                        */
#define _SC_THREAD_DESTRUCTOR_ITERATIONS 85                             /*  user                        */
#define _SC_THREAD_KEYS_MAX         86                                  /*  user                        */
#define _SC_THREAD_PRIO_INHERIT     87                                  /*  user                        */
#define _SC_THREAD_PRIO_PROTECT     88                                  /*  user                        */
#define _SC_THREAD_PRIORITY_SCHEDULING 89                               /*  user                        */
#define _SC_THREAD_PROCESS_SHARED   90                                  /*  user                        */
#define _SC_THREAD_SAFE_FUNCTIONS   91                                  /*  user                        */
#define _SC_THREAD_SPORADIC_SERVER  92                                  /*  user                        */
#define _SC_THREAD_STACK_MIN        93                                  /*  user                        */
#define _SC_THREAD_THREADS_MAX      94                                  /*  user                        */
#define _SC_TIMEOUTS                95                                  /*  user                        */
#define _SC_THREADS                 96                                  /*  user                        */
#define _SC_TRACE                   97                                  /*  user                        */
#define _SC_TRACE_EVENT_FILTER      98                                  /*  user                        */
#define _SC_TRACE_INHERIT           99                                  /*  user                        */
#define _SC_TRACE_LOG               100                                 /*  user                        */
#define _SC_TTY_NAME_MAX            101                                 /*  user                        */
#define _SC_TYPED_MEMORY_OBJECTS    102
#define _SC_V6_ILP32_OFF32          103                                 /*  user                        */
#define _SC_V6_ILP32_OFFBIG         104                                 /*  user                        */
#define _SC_V6_LP64_OFF64           105                                 /*  user                        */
#define _SC_V6_LPBIG_OFFBIG         106                                 /*  user                        */
#define _SC_IPV6                    118
#define _SC_RAW_SOCKETS             119
#define _SC_SYMLOOP_MAX             120

#define _SC_ATEXIT_MAX              107                                 /*  user                        */
#define _SC_IOV_MAX                 56
#define _SC_PAGE_SIZE               _SC_PAGESIZE
#define _SC_XOPEN_CRYPT             108                                 /*  user                        */
#define _SC_XOPEN_ENH_I18N          109                                 /*  user                        */
#define _SC_XOPEN_LEGACY            110                                 /*  user                        */
#define _SC_XOPEN_REALTIME          111
#define _SC_XOPEN_REALTIME_THREADS  112
#define _SC_XOPEN_SHM               113
#define _SC_XOPEN_STREAMS           114
#define _SC_XOPEN_UNIX              115
#define _SC_XOPEN_VERSION           116
#define _SC_XOPEN_XCU_VERSION       117                                 /*  user                        */

#define _SC_NPROCESSORS_CONF        57
#define _SC_NPROCESSORS_ONLN        58

#define	_SC_PHYS_PAGES			    500
#define	_SC_AVPHYS_PAGES		    501

/*********************************************************************************************************
  posix version (temp!)
*********************************************************************************************************/
#ifndef _POSIX_VERSION
#define _POSIX_VERSION              200112L
#endif

#ifndef _POSIX2_VERSION
#define _POSIX2_VERSION             200112L
#endif

#ifndef _POSIX2_C_VERSION
#define _POSIX2_C_VERSION           200112L
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE             200112L
#endif

#ifndef _XOPEN_VERSION
#define _XOPEN_VERSION              600
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE               _XOPEN_VERSION
#endif

#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED      1
#endif

/*********************************************************************************************************
  config macro
*********************************************************************************************************/

#ifndef __SYLIXOS_H
#include "SylixOS.h"
#endif                                                                  /*  __SYLIXOS_H                 */

#include "posix_opt.h"                                                  /*  POSIX support               */

#ifndef MAXPATHLEN
#define	MAXPATHLEN	                PATH_MAX                            /*  defined in sys/param.h too  */
#endif

#ifndef MNAMELEN
#define MNAMELEN                    MAXPATHLEN
#endif

/*********************************************************************************************************
  API
*********************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

LW_API pid_t    getpid(void);

#if LW_CFG_MODULELOADER_EN > 0
LW_API pid_t    fork(void);
LW_API int      setpgid(pid_t pid, pid_t pgid);
LW_API pid_t    getpgid(pid_t pid);
LW_API pid_t    setpgrp(void);
LW_API pid_t    getpgrp(void);
LW_API pid_t    getppid(void);
LW_API pid_t    setsid(void);
LW_API int      issetugid(void);
LW_API int      daemon(int nochdir, int noclose);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

LW_API int      getpagesize(void);

LW_API gid_t    getgid(void);
LW_API int      setgid(gid_t  gid);
LW_API gid_t    getegid(void);
LW_API int      setegid(gid_t  egid);
LW_API uid_t    getuid(void);
LW_API int      setuid(uid_t  uid);
LW_API uid_t    geteuid(void);
LW_API int      seteuid(uid_t  euid);
LW_API int      setgroups(int groupsun, const gid_t grlist[]);
LW_API int      getgroups(int groupsize, gid_t grlist[]);

LW_API int      getlogin_r(char *name, size_t namesize);
LW_API char    *getlogin(void);

LW_API char    *getpass_r(const char *prompt, char *buffer, size_t buflen);
LW_API char    *getpass(const char *prompt);

#include "../SylixOS/lib/libc/stdio/lib_stdio.h"						/*  include stdio.h				*/
#include "../SylixOS/loader/include/loader_exec.h"                      /*  exec family of functions    */

#if LW_CFG_POSIX_EN > 0
LW_API int      nice(int incr);
LW_API long     sysconf(int name);
LW_API long     fpathconf(int fd, int name);
LW_API long     pathconf(const char *path, int name);
LW_API int	    gethostname(char *name, size_t namelen);
LW_API int	    sethostname(const char *name, size_t namelen);
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __UNISTD_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

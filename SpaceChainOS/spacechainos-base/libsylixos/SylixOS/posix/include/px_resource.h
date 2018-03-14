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
** 文   件   名: resource.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 01 月 09 日
**
** 描        述: posix resource 兼容库.
*********************************************************************************************************/

#ifndef __PX_RESOURCE_H
#define __PX_RESOURCE_H

#include "SylixOS.h"                                                    /*  操作系统头文件              */
#include "sys/types.h"

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0

/*********************************************************************************************************
 * Process priority specifications to get/setpriority.
*********************************************************************************************************/

#define PRIO_MIN            (1)
#define PRIO_MAX            (LW_PRIO_LOWEST - (1))

/*********************************************************************************************************
  Process priority specifications to get/setpriority.
*********************************************************************************************************/

#define PRIO_PROCESS        0
#define PRIO_PGRP           1
#define PRIO_USER           2

/*********************************************************************************************************
  Resource utilization information.
*********************************************************************************************************/

#define RUSAGE_SELF         0
#define RUSAGE_CHILDREN     -1

struct rusage {
    struct timeval          ru_utime;                                   /* user time used               */
    struct timeval          ru_stime;                                   /* system time used             */
                                                                        /* the following is unsupport   */
    long                    ru_maxrss;                                  /* max resident set size        */
#define ru_first            ru_ixrss
    long                    ru_ixrss;                                   /* integral shared memory size  */
    long                    ru_idrss;                                   /* integral unshared data "     */
    long                    ru_isrss;                                   /* integral unshared stack "    */
    long                    ru_minflt;                                  /* page reclaims                */
    long                    ru_majflt;                                  /* page faults                  */
    long                    ru_nswap;                                   /* swaps                        */
    long                    ru_inblock;                                 /* block input operations       */
    long                    ru_oublock;                                 /* block output operations      */
    long                    ru_msgsnd;                                  /* messages sent                */
    long                    ru_msgrcv;                                  /* messages received            */
    long                    ru_nsignals;                                /* signals received             */
    long                    ru_nvcsw;                                   /* voluntary context switches   */
    long                    ru_nivcsw;                                  /* involuntary "                */
#define ru_last             ru_nivcsw
};

/*********************************************************************************************************
  Resource limits
*********************************************************************************************************/

#define RLIMIT_CPU          0                                           /* cpu time in milliseconds     */
#define RLIMIT_FSIZE        1                                           /* maximum file size            */
#define RLIMIT_DATA         2                                           /* data size                    */
#define RLIMIT_STACK        3                                           /* stack size                   */
#define RLIMIT_CORE         4                                           /* core file size               */
#define RLIMIT_RSS          5                                           /* resident set size            */
#define RLIMIT_MEMLOCK      6                                           /* locked-in-memory addr space  */
#define RLIMIT_NPROC        7                                           /* number of processes          */
#define RLIMIT_OFILE        8                                           /* number of open files         */
#define RLIMIT_NOFILE       RLIMIT_OFILE
#define RLIMIT_AS           9                                           /* Address space limit.         */

#define RLIM_NLIMITS        15                                          /* number of resource limits    */

#define RLIM_INFINITY       0x7fffffff

#ifndef __rlim_t_defined
typedef long                rlim_t;
#define __rlim_t_defined    1
#endif

struct rlimit {
    rlim_t                  rlim_cur;                                   /* current (soft) limit         */
    rlim_t                  rlim_max;                                   /* maximum value for rlim_cur   */
};

/*********************************************************************************************************
  API
*********************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

LW_API int      getpriority(int which, id_t who);
LW_API int      setpriority(int which, id_t who, int value);
LW_API int      getrlimit(int resource, struct rlimit *rlp);
LW_API int      setrlimit(int resource, const struct rlimit *rlp);
LW_API int      getrusage(int  who, struct rusage *r_usage);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
#endif                                                                  /*  __PX_RESOURCE_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/

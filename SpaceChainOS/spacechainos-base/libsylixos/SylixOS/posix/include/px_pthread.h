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
** 文   件   名: px_pthread.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 30 日
**
** 描        述: pthread 兼容库.
**
** 注        意: SylixOS pthread_rwlock_rdlock() 不支持同一线程嵌套调用.
*********************************************************************************************************/

#ifndef __PX_PTHREAD_H
#define __PX_PTHREAD_H

#include "SylixOS.h"                                                    /*  操作系统头文件              */

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0

#if LW_CFG_GJB7714_EN > 0
#include "px_gjbext.h"
#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

#include "px_pthread_attr.h"
#include "px_sched.h"

/*********************************************************************************************************
  pthread once & cancel
*********************************************************************************************************/

typedef BOOL                pthread_once_t;

#define PTHREAD_ONCE_INIT   LW_FALSE

#ifndef PTHREAD_CANCEL_ASYNCHRONOUS
#define PTHREAD_CANCEL_ASYNCHRONOUS         LW_THREAD_CANCEL_ASYNCHRONOUS
#define PTHREAD_CANCEL_DEFERRED             LW_THREAD_CANCEL_DEFERRED

#define PTHREAD_CANCEL_ENABLE               LW_THREAD_CANCEL_ENABLE
#define PTHREAD_CANCEL_DISABLE              LW_THREAD_CANCEL_DISABLE

#define PTHREAD_CANCELED                    LW_THREAD_CANCELED
#endif                                                                  /*  PTHREAD_CANCEL_ASYNCHRONOUS */

/*********************************************************************************************************
  pthread barrier type
*********************************************************************************************************/
#define PTHREAD_BARRIER_SERIAL_THREAD       1

typedef INT                 pthread_barrierattr_t;
typedef struct {
    LW_OBJECT_HANDLE        PBARRIER_ulSync;                            /*  同步信号量                  */
    volatile INT            PBARRIER_iCounter;                          /*  当前计数值                  */
             INT            PBARRIER_iInitValue;                        /*  初始值                      */
} pthread_barrier_t;

/*********************************************************************************************************
  pthread cond type
*********************************************************************************************************/
#define PTHREAD_PROCESS_SHARED              LW_THREAD_PROCESS_SHARED    /*  default                     */
#define PTHREAD_PROCESS_PRIVATE             LW_THREAD_PROCESS_PRIVATE

typedef ULONG               pthread_condattr_t;
typedef LW_THREAD_COND      pthread_cond_t;

#define PTHREAD_COND_INITIALIZER            LW_THREAD_COND_INITIALIZER

/*********************************************************************************************************
  pthread mutex type
*********************************************************************************************************/
#define PTHREAD_PRIO_NONE                   0
#define PTHREAD_PRIO_INHERIT                1
#define PTHREAD_PRIO_PROTECT                2

#define PTHREAD_MUTEX_NORMAL                0                           /*  重复 lock 产生死锁          */
#define PTHREAD_MUTEX_ERRORCHECK            1                           /*  重复 lock 返回错误          */
#define PTHREAD_MUTEX_RECURSIVE             2                           /*  重复 lock 支持递归          */

#define PTHREAD_MUTEX_FAST_NP               PTHREAD_MUTEX_NORMAL        /*  fix to linux                */
#define PTHREAD_MUTEX_ERRORCHECK_NP         PTHREAD_MUTEX_ERRORCHECK
#define PTHREAD_MUTEX_RECURSIVE_NP          PTHREAD_MUTEX_RECURSIVE

#define PTHREAD_MUTEX_DEFAULT               PTHREAD_MUTEX_RECURSIVE     /*  默认支持递归调用            */
#define PTHREAD_MUTEX_CEILING               PX_PRIORITY_CONVERT(LW_PRIO_REALTIME)
                                                                        /*  默认天花板优先级            */
#define PTHREAD_WAITQ_PRIO                  1
#define PTHREAD_WAITQ_FIFO                  0

#define PTHREAD_CANCEL_SAFE                 1
#define PTHREAD_CANCEL_UNSAFE               0

typedef struct {
    int                     PMUTEXATTR_iIsEnable;                       /*  此属性块是否有效            */
    int                     PMUTEXATTR_iType;                           /*  互斥量类型                  */
    int                     PMUTEXATTR_iPrioceiling;                    /*  天花板优先级                */
    unsigned long           PMUTEXATTR_ulOption;                        /*  算法类型                    */
} pthread_mutexattr_t;

typedef struct {
    LW_OBJECT_HANDLE        PMUTEX_ulMutex;                             /*  互斥信号量句柄              */
    int                     PMUTEX_iType;                               /*  互斥量类型                  */
} pthread_mutex_t;

#define PTHREAD_MUTEX_INITIALIZER           {0, PTHREAD_MUTEX_DEFAULT}

/*********************************************************************************************************
  pthread spinlock type
*********************************************************************************************************/
typedef spinlock_t          pthread_spinlock_t;

/*********************************************************************************************************
  pthread rwlock type
*********************************************************************************************************/
#define PTHREAD_RWLOCK_PREFER_READER_NP                 0
#define PTHREAD_RWLOCK_PREFER_WRITER_NP                 1
#define PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP    2
#define PTHREAD_RWLOCK_DEFAULT_NP                       PTHREAD_RWLOCK_PREFER_READER_NP

typedef int                 pthread_rwlockattr_t;

typedef struct {
    LW_OBJECT_HANDLE        PRWLOCK_ulRwLock;                           /*  内核读写锁                  */
    LW_OBJECT_HANDLE        PRWLOCK_ulReserved[3];
    unsigned int            PRWLOCK_uiReserved[4];
} pthread_rwlock_t;

#define PTHREAD_RWLOCK_INITIALIZER          {0, {0, 0, 0}, {0, 0, 0, 0}}

/*********************************************************************************************************
  pthread key type 注意: pthread_key_create 中的 destructor 函数中不能删除 key 键, 否则可能造成错误.
*********************************************************************************************************/

typedef long                pthread_key_t;

#define PTHREAD_KEY_INITIALIZER             (long)NULL

/*********************************************************************************************************
  pthread 中断扩展操作
*********************************************************************************************************/

#if LW_CFG_POSIXEX_EN > 0
typedef INTREG              pthread_int_t;
#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */

/*********************************************************************************************************
  pthread api
*********************************************************************************************************/

LW_API int          pthread_attr_init(pthread_attr_t  *pattr);
LW_API int          pthread_attr_destroy(pthread_attr_t  *pattr);
LW_API int          pthread_attr_setstack(pthread_attr_t *pattr, void *pvStackAddr, size_t stSize);
LW_API int          pthread_attr_getstack(const pthread_attr_t *pattr, void **ppvStackAddr, size_t *pstSize);
LW_API int          pthread_attr_setguardsize(pthread_attr_t  *pattr, size_t  stGuard);
LW_API int          pthread_attr_getguardsize(pthread_attr_t  *pattr, size_t  *pstGuard);
LW_API int          pthread_attr_setstacksize(pthread_attr_t  *pattr, size_t  stSize);
LW_API int          pthread_attr_getstacksize(const pthread_attr_t  *pattr, size_t  *pstSize);
LW_API int          pthread_attr_setstackaddr(pthread_attr_t  *pattr, void  *pvStackAddr);
LW_API int          pthread_attr_getstackaddr(const pthread_attr_t  *pattr, void  **ppvStackAddr);
LW_API int          pthread_attr_setdetachstate(pthread_attr_t  *pattr, int  iDetachState);
LW_API int          pthread_attr_getdetachstate(const pthread_attr_t  *pattr, int  *piDetachState);
LW_API int          pthread_attr_setschedpolicy(pthread_attr_t  *pattr, int  iPolicy);
LW_API int          pthread_attr_getschedpolicy(const pthread_attr_t  *pattr, int  *piPolicy);
LW_API int          pthread_attr_setschedparam(pthread_attr_t            *pattr, 
                                               const struct sched_param  *pschedparam);
LW_API int          pthread_attr_getschedparam(const pthread_attr_t      *pattr, 
                                               struct sched_param  *pschedparam);
LW_API int          pthread_attr_setinheritsched(pthread_attr_t  *pattr, int  iInherit);
LW_API int          pthread_attr_getinheritsched(const pthread_attr_t  *pattr, int  *piInherit);
LW_API int          pthread_attr_setscope(pthread_attr_t  *pattr, int  iScope);
LW_API int          pthread_attr_getscope(const pthread_attr_t  *pattr, int  *piScope);
LW_API int          pthread_attr_setname(pthread_attr_t  *pattr, const char  *pcName);
LW_API int          pthread_attr_getname(const pthread_attr_t  *pattr, char  **ppcName);

#if LW_CFG_POSIXEX_EN > 0
LW_API int          pthread_attr_get_np(pthread_t  thread, pthread_attr_t *pattr);
LW_API int          pthread_getattr_np(pthread_t thread, pthread_attr_t *pattr);
LW_API int          pthread_setname_np(pthread_t  thread, const char  *name);
LW_API int          pthread_getname_np(pthread_t  thread, char  *name, size_t len);
LW_API void         pthread_safe_np(void);
LW_API void         pthread_unsafe_np(void);
#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */

LW_API int          pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));
LW_API int          pthread_create(pthread_t              *pthread, 
                                   const pthread_attr_t   *pattr, 
                                   void                   *(*start_routine)(void *),
                                   void                   *arg);
LW_API int          pthread_cancel(pthread_t  thread);
LW_API void         pthread_testcancel(void);
LW_API int          pthread_join(pthread_t  thread, void **ppstatus);
LW_API int          pthread_detach(pthread_t  thread);
LW_API int          pthread_equal(pthread_t  thread1, pthread_t  thread2);
LW_API void         pthread_exit(void *status);
LW_API pthread_t    pthread_self(void);
LW_API int          pthread_yield(void);

#if LW_CFG_SIGNAL_EN > 0
LW_API int          pthread_kill(pthread_t  thread, int signo);
LW_API int          pthread_sigmask(int  how, const sigset_t  *newmask, sigset_t *oldmask);
#endif

LW_API void         pthread_cleanup_pop(int  iNeedRun);
LW_API void         pthread_cleanup_push(void (*pfunc)(void *), void *arg);
LW_API int          pthread_getschedparam(pthread_t            thread, 
                                          int                 *piPolicy, 
                                          struct sched_param  *pschedparam);
LW_API int          pthread_setschedparam(pthread_t                  thread, 
                                          int                        iPolicy, 
                                          const struct sched_param  *pschedparam);
LW_API int          pthread_once(pthread_once_t  *once, void (*pfunc)(void));
LW_API int          pthread_setschedprio(pthread_t  thread, int  prio);
LW_API int          pthread_getschedprio(pthread_t  thread, int  *prio);
LW_API int          pthread_setcancelstate(int  newstate, int  *poldstate);
LW_API int          pthread_setcanceltype(int  newtype, int  *poldtype);
LW_API int          pthread_setconcurrency(int  newlevel);
LW_API int          pthread_getconcurrency(void);
LW_API int          pthread_getcpuclockid(pthread_t thread, clockid_t *clock_id);

LW_API int          pthread_barrierattr_init(pthread_barrierattr_t  *pbarrierattr);
LW_API int          pthread_barrierattr_destroy(pthread_barrierattr_t  *pbarrierattr);
LW_API int          pthread_barrierattr_setpshared(pthread_barrierattr_t  *pbarrierattr, int  shared);
LW_API int          pthread_barrierattr_getpshared(const pthread_barrierattr_t  *pbarrierattr, int  *pshared);
LW_API int          pthread_barrier_init(pthread_barrier_t            *pbarrier, 
                                         const pthread_barrierattr_t  *pbarrierattr,
                                         unsigned int                  count);
LW_API int          pthread_barrier_destroy(pthread_barrier_t  *pbarrier);
LW_API int          pthread_barrier_wait(pthread_barrier_t  *pbarrier);

LW_API int          pthread_condattr_init(pthread_condattr_t  *pcondattr);
LW_API int          pthread_condattr_destroy(pthread_condattr_t  *pcondattr);
LW_API int          pthread_condattr_setpshared(pthread_condattr_t  *pcondattr, int  ishare);
LW_API int          pthread_condattr_getpshared(const pthread_condattr_t  *pcondattr, int  *pishare);
LW_API int          pthread_condattr_setclock(pthread_condattr_t  *pcondattr, clockid_t  clock_id);
LW_API int          pthread_condattr_getclock(const pthread_condattr_t  *pcondattr, clockid_t  *pclock_id);
LW_API int          pthread_cond_init(pthread_cond_t  *pcond, const pthread_condattr_t  *pcondattr);
LW_API int          pthread_cond_destroy(pthread_cond_t  *pcond);
LW_API int          pthread_cond_signal(pthread_cond_t  *pcond);
LW_API int          pthread_cond_broadcast(pthread_cond_t  *pcond);
LW_API int          pthread_cond_wait(pthread_cond_t  *pcond, pthread_mutex_t  *pmutex);
LW_API int          pthread_cond_timedwait(pthread_cond_t         *pcond, 
                                           pthread_mutex_t        *pmutex,
                                           const struct timespec  *abs_timeout);
#if LW_CFG_POSIXEX_EN > 0
LW_API int          pthread_cond_reltimedwait_np(pthread_cond_t         *pcond, 
                                                 pthread_mutex_t        *pmutex,
                                                 const struct timespec  *rel_timeout);
#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */

LW_API int          pthread_mutexattr_init(pthread_mutexattr_t *pmutexattr);
LW_API int          pthread_mutexattr_destroy(pthread_mutexattr_t *pmutexattr);
LW_API int          pthread_mutexattr_setprioceiling(pthread_mutexattr_t *pmutexattr, int  prioceiling);
LW_API int          pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *pmutexattr, int  *prioceiling);
LW_API int          pthread_mutexattr_setprotocol(pthread_mutexattr_t *pmutexattr, int  protocol);
LW_API int          pthread_mutexattr_getprotocol(const pthread_mutexattr_t *pmutexattr, int  *protocol);
LW_API int          pthread_mutexattr_setpshared(pthread_mutexattr_t *pmutexattr, int  pshared);
LW_API int          pthread_mutexattr_getpshared(const pthread_mutexattr_t *pmutexattr, int  *pshared);
LW_API int          pthread_mutexattr_settype(pthread_mutexattr_t *pmutexattr, int  type);
LW_API int          pthread_mutexattr_gettype(const pthread_mutexattr_t *pmutexattr, int  *type);
LW_API int          pthread_mutexattr_setwaitqtype(pthread_mutexattr_t  *pmutexattr, int  waitq_type);
LW_API int          pthread_mutexattr_getwaitqtype(const pthread_mutexattr_t *pmutexattr, int *waitq_type);
LW_API int          pthread_mutexattr_setcancelsafe(pthread_mutexattr_t  *pmutexattr, int  cancel_safe);
LW_API int          pthread_mutexattr_getcancelsafe(const pthread_mutexattr_t *pmutexattr, int *cancel_safe);
LW_API int          pthread_mutex_init(pthread_mutex_t  *pmutex, const pthread_mutexattr_t *pmutexattr);
LW_API int          pthread_mutex_destroy(pthread_mutex_t  *pmutex);
LW_API int          pthread_mutex_lock(pthread_mutex_t  *pmutex);
LW_API int          pthread_mutex_trylock(pthread_mutex_t  *pmutex);
LW_API int          pthread_mutex_timedlock(pthread_mutex_t  *pmutex, const struct timespec *abs_timeout);
#if LW_CFG_POSIXEX_EN > 0
LW_API int          pthread_mutex_reltimedlock_np(pthread_mutex_t  *pmutex, 
                                                  const struct timespec *rel_timeout);
#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */
LW_API int          pthread_mutex_unlock(pthread_mutex_t  *pmutex);
LW_API int          pthread_mutex_setprioceiling(pthread_mutex_t  *pmutex, int  prioceiling);
LW_API int          pthread_mutex_getprioceiling(pthread_mutex_t  *pmutex, int  *prioceiling);

LW_API int          pthread_spin_init(pthread_spinlock_t  *pspinlock, int  pshare);
LW_API int          pthread_spin_destroy(pthread_spinlock_t  *pspinlock);
LW_API int          pthread_spin_lock(pthread_spinlock_t  *pspinlock);
LW_API int          pthread_spin_unlock(pthread_spinlock_t  *pspinlock);
LW_API int          pthread_spin_trylock(pthread_spinlock_t  *pspinlock);
#if LW_CFG_POSIXEX_EN > 0
LW_API int          pthread_spin_lock_irq_np(pthread_spinlock_t  *pspinlock, pthread_int_t *irqctx);
LW_API int          pthread_spin_unlock_irq_np(pthread_spinlock_t  *pspinlock, pthread_int_t irqctx);
LW_API int          pthread_spin_trylock_irq_np(pthread_spinlock_t  *pspinlock, pthread_int_t *irqctx);
#endif                                                                  /*  W_CFG_POSIXEX_EN > 0        */

LW_API int          pthread_rwlockattr_init(pthread_rwlockattr_t  *prwlockattr);
LW_API int          pthread_rwlockattr_destroy(pthread_rwlockattr_t  *prwlockattr);
LW_API int          pthread_rwlockattr_setpshared(pthread_rwlockattr_t *prwlockattr, int  pshared);
LW_API int          pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *prwlockattr, int  *pshared);
#if LW_CFG_POSIXEX_EN > 0
LW_API int          pthread_rwlockattr_setkind_np(pthread_rwlockattr_t *prwlockattr, int pref);
LW_API int          pthread_rwlockattr_getkind_np(const pthread_rwlockattr_t *prwlockattr, int *pref);
#endif                                                                  /*  W_CFG_POSIXEX_EN > 0        */
LW_API int          pthread_rwlock_init(pthread_rwlock_t  *prwlock, const pthread_rwlockattr_t  *prwlockattr);
LW_API int          pthread_rwlock_destroy(pthread_rwlock_t  *prwlock);
LW_API int          pthread_rwlock_rdlock(pthread_rwlock_t  *prwlock);
LW_API int          pthread_rwlock_tryrdlock(pthread_rwlock_t  *prwlock);
LW_API int          pthread_rwlock_timedrdlock(pthread_rwlock_t *prwlock,
                                               const struct timespec *abs_timeout);
#if LW_CFG_POSIXEX_EN > 0
LW_API int          pthread_rwlock_reltimedrdlock_np(pthread_rwlock_t *prwlock,
                                                     const struct timespec *rel_timeout);
#endif                                                                  /*  W_CFG_POSIXEX_EN > 0        */

LW_API int          pthread_rwlock_wrlock(pthread_rwlock_t  *prwlock);
LW_API int          pthread_rwlock_trywrlock(pthread_rwlock_t  *prwlock);
LW_API int          pthread_rwlock_timedwrlock(pthread_rwlock_t *prwlock,
                                               const struct timespec *abs_timeout);
#if LW_CFG_POSIXEX_EN > 0
LW_API int          pthread_rwlock_reltimedwrlock_np(pthread_rwlock_t *prwlock,
                                                     const struct timespec *rel_timeout);
#endif                                                                  /*  W_CFG_POSIXEX_EN > 0        */
LW_API int          pthread_rwlock_unlock(pthread_rwlock_t  *prwlock);

LW_API int          pthread_key_create(pthread_key_t  *pkey, void (*fdestructor)(void *));
LW_API int          pthread_key_delete(pthread_key_t  key);
LW_API int          pthread_setspecific(pthread_key_t  key, const void  *pvalue);
LW_API void        *pthread_getspecific(pthread_key_t  key);

#if LW_CFG_POSIXEX_EN > 0
LW_API int          pthread_int_lock_np(pthread_int_t *irqctx);
LW_API int          pthread_int_unlock_np(pthread_int_t irqctx);

LW_API int          pthread_setaffinity_np(pthread_t  thread, size_t setsize, const cpu_set_t *set);
LW_API int          pthread_getaffinity_np(pthread_t  thread, size_t setsize, cpu_set_t *set);
#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */

/*********************************************************************************************************
  pthread GJB7714 extern api
*********************************************************************************************************/

#if LW_CFG_GJB7714_EN > 0

#define PTHREAD_PRI_MIN             PX_PRIORITY_CONVERT((LW_PRIO_LOWEST - 1))
#define PTHREAD_PRI_MAX             PX_PRIORITY_CONVERT(1)

#define PTHREAD_STACK_FILLED        1
#define PTHREAD_NO_STACK_FILLED     0

#define PTHREAD_BREAK_ALLOWED       1
#define PTHREAD_BREAK_DISALLOWED    0

#define PTHREAD_FP_ALLOWED          1
#define PTHREAD_FP_DISALLOWED       0

typedef struct {
    int         value;
    int         inherit;
    int         prioceiling;
    int         wait_type;
    int         cancel_type;
    int         blocknum;
    pthread_t   ownner;
    ULONG       reservepad[6];
} pthread_mutex_info_t;

typedef struct {
    LW_OBJECT_HANDLE    signal;
    LW_OBJECT_HANDLE    mutex;
    ULONG               cnt;
    ULONG               reservepad[6];
} pthread_cond_info_t;

typedef struct {
    LW_OBJECT_HANDLE    owner;
    unsigned int        opcnt;
    unsigned int        rpend;
    unsigned int        wpend;
    ULONG               reservepad[10];
} pthread_rwlock_info_t;

LW_API int          pthread_attr_getstackfilled(const pthread_attr_t *pattr, int *stackfilled);
LW_API int          pthread_attr_setstackfilled(pthread_attr_t *pattr, int stackfilled);
LW_API int          pthread_attr_getbreakallowed(const pthread_attr_t *pattr, int *breakallowed);
LW_API int          pthread_attr_setbreakallowed(pthread_attr_t *pattr, int breakallowed);
LW_API int          pthread_attr_getfpallowed(const pthread_attr_t *pattr, int *fpallowed);
LW_API int          pthread_attr_setfpallowed(pthread_attr_t *pattr, int fpallowed);

LW_API int          pthread_getid(const char *name, pthread_t *pthread);
LW_API int          pthread_getname(pthread_t thread, char *name);
LW_API int          pthread_suspend(pthread_t thread);
LW_API int          pthread_resume(pthread_t thread);
LW_API int          pthread_delay(int  ticks);
LW_API int          pthread_lock(void);
LW_API int          pthread_unlock(void);
LW_API boolean      pthread_is_ready(pthread_t thread);
LW_API boolean      pthread_is_suspend(pthread_t thread);
LW_API int          pthread_verifyid(pthread_t thread);
LW_API int          pthread_cancelforce(pthread_t thread);
LW_API int          pthread_getinfo(pthread_t thread, pthread_info_t *info);
LW_API int          pthread_getregs(pthread_t thread, REG_SET *pregs);
LW_API int          pthread_show(pthread_t thread, int level);
LW_API int          pthread_showstack(pthread_t thread);
LW_API int          pthread_showstackframe(pthread_t thread);

/*********************************************************************************************************
  var function (ONLY 32bit UP system can use the following routine)
*********************************************************************************************************/

#if (LW_CFG_SMP_EN == 0) && (LW_CFG_CPU_WORD_LENGHT == 32)
LW_API int          pthread_addvar(pthread_t thread, int *pvar);
LW_API int          pthread_delvar(pthread_t thread, int *pvar);
LW_API int          pthread_setvar(pthread_t thread, int *pvar, int value);
LW_API int          pthread_getvar(pthread_t thread, int *pvar, int *value);
#endif                                                                  /*  SMP ...                     */

/*********************************************************************************************************
  hook function (ONLY kernel program can use the following routine)
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL
LW_API int          pthread_create_hook_add(OS_STATUS (*create_hook)(pthread_t));
LW_API int          pthread_create_hook_delete(OS_STATUS (*create_hook)(pthread_t));
LW_API int          pthread_switch_hook_add(OS_STATUS (*switch_hook)(pthread_t, pthread_t));
LW_API int          pthread_switch_hook_delete(OS_STATUS (*switch_hook)(pthread_t, pthread_t));
LW_API int          pthread_close_hook_add(OS_STATUS (*close_hook)(pthread_t));
LW_API int          pthread_close_hook_delete(OS_STATUS (*close_hook)(pthread_t));
#endif                                                                  /*  __SYLIXOS_KERNEL            */

LW_API int          pthread_mutex_getinfo(pthread_mutex_t  *pmutex, pthread_mutex_info_t  *info);
LW_API int          pthread_mutex_show(pthread_mutex_t  *pmutex, int  level);

LW_API int          pthread_cond_getinfo(pthread_cond_t  *pcond, pthread_cond_info_t  *info);
LW_API int          pthread_cond_show(pthread_cond_t  *pcond, int  level);

LW_API int          pthread_rwlock_getinfo(pthread_rwlock_t  *prwlock, pthread_rwlock_info_t  *info);

#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
#endif                                                                  /*  __PX_PTHREAD_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/

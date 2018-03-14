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
** 文   件   名: loader_wait.c
**
** 创   建   人: Han.hui (韩辉)
**
** 文件创建日期: 2012 年 08 月 24 日
**
** 描        述: posix 进程兼容 api.

** BUG:
2012.12.06  简化 getpid 操作.
2012.12.17  waitpid 如果是等待指定的子进程, 则首先应该当前进程是否存在指定的子进程.
2013.01.10  加入 waitid 函数.
2013.06.07  加入 detach 函数, 用于解除子进程与父进程的关系.
2014.05.04  加入 daemon 函数.
2014.11.04  加入 reclaimchild 函数.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "paths.h"
#include "sys/wait.h"
#include "sys/resource.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_lib.h"
#include "../include/loader_symbol.h"
#include "../include/loader_error.h"
/*********************************************************************************************************
** 函数名称: fork
** 功能描述: posix fork
** 输　入  : NONE
** 输　出  : 进程号
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
pid_t fork (void)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: getpid
** 功能描述: 获得当前进程号 (此函数不用加锁, 因为进程内所有线程退出后才删除进程控制块)
** 输　入  : NONE
** 输　出  : 进程号
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
pid_t getpid (void)
{
    LW_LD_VPROC    *pvproc;
    pid_t           pid = 0;
    
    pvproc = __LW_VP_GET_CUR_PROC();
    if (pvproc) {
        pid = pvproc->VP_pid;
    }
    
    return  (pid);
}
/*********************************************************************************************************
** 函数名称: setpgid
** 功能描述: set process group ID for job control
** 输　入  : pid       进程号
**           pgid      进程组号
** 输　出  : 
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
int setpgid (pid_t pid, pid_t pgid)
{
    if (pid == 0) {
        pid = getpid();
    }
    
    if (pgid == 0) {
        pgid = getpid();
    }

    return  (vprocSetGroup(pid, pgid));
}
/*********************************************************************************************************
** 函数名称: getpgid
** 功能描述: get the process group ID for a process
** 输　入  : pid       进程号
** 输　出  : 进程组号
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
pid_t getpgid (pid_t pid)
{
    if (pid == 0) {
        pid = getpid();
    }

    return  (vprocGetGroup(pid));
}
/*********************************************************************************************************
** 函数名称: setpgrp
** 功能描述: set the process group ID of the calling process
** 输　入  : NONE
** 输　出  : 进程组号
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
pid_t setpgrp (void)
{
    if (setpgid(0, 0) == 0) {
        return  (getpgid(0));
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: getpgrp
** 功能描述: get the process group ID of the calling process
** 输　入  : NONE
** 输　出  : 进程组号
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
pid_t getpgrp (void)
{
    return  getpgid(0);
}
/*********************************************************************************************************
** 函数名称: getpgrp
** 功能描述: get the parent process ID
** 输　入  : NONE
** 输　出  : 进程号
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
pid_t getppid (void)
{
    return  (vprocGetFather(getpid()));
}
/*********************************************************************************************************
** 函数名称: setsid
** 功能描述: create session and set process group ID
** 输　入  : NONE
** 输　出  : 进程号
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
pid_t setsid (void)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: issetugid
** 功能描述: 若启动进程时使用了 setuid 或 setgid 位的话，就返回真。
** 输　入  : 
** 输　出  : 是否设置了 setuid 或 setgid 位
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
int issetugid (void)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_LD_VPROC    *pvproc;
    int             ret = 0;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pvproc = (LW_LD_VPROC *)ptcbCur->TCB_pvVProcessContext;
    if (pvproc) {
        ret = (int)pvproc->VP_bIssetugid;
    }
    
    return  (ret);
}
/*********************************************************************************************************
** 函数名称: __haveThisChild
** 功能描述: 检查当前进程是否存在一个指定 pid 号的子进程
** 输　入  : pvproc        当前进程
**           pidChild      子进程
** 输　出  : 有或者没有
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL __haveThisChild (LW_LD_VPROC  *pvproc, pid_t  pidChild)
{
    PLW_LIST_LINE   plineList;
    LW_LD_VPROC    *pvprocChild;
    BOOL            bRet = LW_FALSE;

    LW_LD_LOCK();
    for (plineList  = pvproc->VP_plineChild;
         plineList != LW_NULL;
         plineList  = _list_line_get_next(plineList)) {
         
        pvprocChild = _LIST_ENTRY(plineList, LW_LD_VPROC, VP_lineBrother);
        if (pvprocChild->VP_pid == pidChild) {
            bRet = LW_TRUE;
            break;
        }
    }
    LW_LD_UNLOCK();
    
    return  (bRet);
}
/*********************************************************************************************************
** 函数名称: __reclaimAChild
** 功能描述: 回收一个子进程
** 输　入  : pvproc        当前进程
**           pidChild      需要回收的子进程
**           pid           子进程号
**           stat_loc      返回值
**           option        选项
**           prusage       进程信息
** 输　出  : 回收进程的数量 1: 回收一个 0: 没有回收 -1: 没有子进程
** 全局变量:
** 调用模块:
** 注  意  : 参数 pidChild 有以下意义:
             pidChild >  0      等待指定 PID 的子进程
             pidChild == 0      子进程中组 ID 相同的
             pidChild == -1     任何子进程
             pidChild <  -1     子进程中组 ID 为 pid 绝对值的
*********************************************************************************************************/
static INT __reclaimAChild (LW_LD_VPROC   *pvproc, 
                            pid_t          pidChild, 
                            pid_t         *pid, 
                            int           *stat_loc, 
                            int            option,
                            struct rusage *prusage)
{
    PLW_LIST_LINE   plineList;
    LW_LD_VPROC    *pvprocChild;
    BOOL            bHasEvent    = LW_FALSE;
    BOOL            bNeedReclaim = LW_FALSE;
    pid_t           gpid;
    
    if (pidChild < -1) {
        gpid = 0 - pidChild;                                            /*  绝对值                      */
    }
    
    LW_LD_LOCK();
    if (pvproc->VP_plineChild == LW_NULL) {                             /*  没有子进程                  */
        LW_LD_UNLOCK();
        errno = ECHILD;
        return  (PX_ERROR);
    }
    plineList = pvproc->VP_plineChild;
    while (plineList) {                                                 /*  遍历子进程                  */
        pvprocChild = _LIST_ENTRY(plineList, LW_LD_VPROC, VP_lineBrother);
        plineList = _list_line_get_next(plineList);
        
        if (pidChild > 0) {                                             /*  等待指定 PID 的子进程       */
            if (pidChild != pvprocChild->VP_pid) {
                continue;
            }
        
        } else if (pidChild == 0) {                                     /*  子进程中组 ID 相同的        */
            if (pvproc->VP_pidGroup != pvprocChild->VP_pidGroup) {
                continue;
            }
            
        } else if (pidChild < -1) {                                     /*  指定的子进程组              */
            if (gpid != pvprocChild->VP_pidGroup) {
                continue;
            }
        }                                                               /*  -1 表示任何子进程           */
        
        /*
         *  检查满足条件的子进程是否处于指定的状态
         */
        if (pvprocChild->VP_iStatus == __LW_VP_EXIT) {                  /*  子进程已经结束              */
            if (pid) {
                *pid = pvprocChild->VP_pid;
            }
            if (stat_loc) {
                *stat_loc = SET_EXITSTATUS(pvprocChild->VP_iExitCode);  /*  子进程正常退出              */
            }
            bHasEvent    = LW_TRUE;
            bNeedReclaim = LW_TRUE;
            break;
        
        } else if (pvprocChild->VP_iStatus == __LW_VP_STOP) {           /*  子进程停止                  */
            if (option & WUNTRACED) {
                if (pid) {
                    *pid = 0;                                           /*  返回值为 0                  */
                }
                if (stat_loc) {
                    *stat_loc = SET_STOPSIG(pvprocChild->VP_iSigCode);
                }
                bHasEvent = LW_TRUE;
                break;
            }
        
        } else if (pvprocChild->VP_ulFeatrues & __LW_VP_FT_DAEMON) {    /*  子进程进入 deamon 状态      */
            if (pid) {
                *pid = pvprocChild->VP_pid;
            }
            if (stat_loc) {
                *stat_loc = 0;                                          /*  子进程正常退出              */
            }
            bHasEvent = LW_TRUE;
            break;
        }
    }
    LW_LD_UNLOCK();
    
    if (bHasEvent) {
        if (prusage) {
            __tickToTimeval(pvprocChild->VP_clockUser,   &prusage->ru_utime);
            __tickToTimeval(pvprocChild->VP_clockSystem, &prusage->ru_stime);
        }
        if (bNeedReclaim) {
            vprocReclaim(pvprocChild, LW_TRUE);                         /*  回收子进程资源              */
        }
        return  (1);
    
    } else {
        return  (0);
    }
}
/*********************************************************************************************************
** 函数名称: wait
** 功能描述: wait for a child process to stop or terminate
** 输　入  : stat_loc
** 输　出  : child pid
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
pid_t wait (int *stat_loc)
{
    INT               iError;
    
    sigset_t          sigsetSigchld;
    struct  siginfo   siginfoChld;
    pid_t             pidChld;
    
    LW_LD_VPROC      *pvproc = __LW_VP_GET_CUR_PROC();
    
    if (pvproc == LW_NULL) {
        errno = ECHILD;
        return  ((pid_t)-1);
    }
    
    sigemptyset(&sigsetSigchld);
    sigaddset(&sigsetSigchld, SIGCHLD);
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    do {
        iError = __reclaimAChild(pvproc, -1, &pidChld, 
                                 stat_loc, 0, LW_NULL);                 /*  试图回收一个子进程          */
        if (iError < 0) {
            return  (iError);                                           /*  没有子线程                  */
        
        } else if (iError > 0) {
            return  (pidChld);                                          /*  子线程已经被回收            */
        }
        
        iError = sigwaitinfo(&sigsetSigchld, &siginfoChld);             /*  等待子进程信号              */
        if (iError < 0) {
            return  (iError);                                           /*  被其他信号打断              */
        }
    } while (1);
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: waitid
** 功能描述: wait for a child process to change state
** 输　入  : idtype        类型 (P_PID  P_PGID  P_ALL)
**           id            P_PID: will wait for the child with a process ID equal to (pid_t)id.
**                         P_PGID: will wait for any child with a process group ID equal to (pid_t)id.
**                         P_ALL: waitid() will wait for any children and id is ignored.
**           options       选项
** 输　出  : If waitid() returns due to the change of state of one of its children, 0 is returned. 
**           Otherwise, -1 is returned and errno is set to indicate the error.
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
int  waitid (idtype_t idtype, id_t id, siginfo_t *infop, int options)
{
    INT               iError;
    INT               iStat;
    
    sigset_t          sigsetSigchld;
    struct  siginfo   siginfoChld;
    pid_t             pidChld;
    pid_t             pid;
    
    LW_LD_VPROC      *pvproc = __LW_VP_GET_CUR_PROC();
    
    if (pvproc == LW_NULL) {
        errno = ECHILD;
        return  (PX_ERROR);
    }
    
    if (infop == LW_NULL) {
        infop =  &siginfoChld;
    }
    
    switch (idtype) {
    
    case P_PID:
        pid = (pid_t)id;
        break;
        
    case P_PGID:
        pid = (pid_t)(-id);
        if (pid == -1) {
            errno = EINVAL;
            return  (PX_ERROR);
        }
        break;
        
    case P_ALL:
        pid = (pid_t)(-1);
        break;
        
    default:
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    sigemptyset(&sigsetSigchld);
    sigaddset(&sigsetSigchld, SIGCHLD);
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    if (pid > 0) {                                                      /*  等待指定的子进程            */
        if (!__haveThisChild(pvproc, pid)) {
            errno = ECHILD;
            return  (PX_ERROR);                                         /*  当前进程不存在指定的子进程  */
        }
    }
    
    do {
        iError = __reclaimAChild(pvproc, pid, &pidChld, 
                                 &iStat, options, LW_NULL);             /*  试图回收一个子进程          */
        if (iError < 0) {
            return  (PX_ERROR);                                         /*  没有子线程                  */
        
        } else if (iError > 0) {
            return  (ERROR_NONE);                                       /*  子线程已经被回收            */
        }
        
        if (options & WNOHANG) {                                        /*  不等待                      */
            return  (ERROR_NONE);
        }
        
        iError = sigwaitinfo(&sigsetSigchld, infop);                    /*  等待子进程信号              */
        if (iError < 0) {
            return  (iError);                                           /*  被其他信号打断              */
        }
    } while (1);
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: waitpid
** 功能描述: wait for a child process to stop or terminate
** 输　入  : pid           指定的子线程
**           stat_loc      返回值
**           options       选项
** 输　出  : child pid 
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
pid_t waitpid (pid_t pid, int *stat_loc, int options)
{
    INT               iError;
    
    sigset_t          sigsetSigchld;
    struct  siginfo   siginfoChld;
    pid_t             pidChld;
    
    LW_LD_VPROC      *pvproc = __LW_VP_GET_CUR_PROC();
    
    if (pvproc == LW_NULL) {
        errno = ECHILD;
        return  ((pid_t)-1);
    }
    
    sigemptyset(&sigsetSigchld);
    sigaddset(&sigsetSigchld, SIGCHLD);
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    if (pid > 0) {                                                      /*  等待指定的子进程            */
        if (!__haveThisChild(pvproc, pid)) {
            errno = ECHILD;
            return  ((pid_t)-1);                                        /*  当前进程不存在指定的子进程  */
        }
    }
    
    do {
        iError = __reclaimAChild(pvproc, pid, &pidChld, 
                                 stat_loc, options, LW_NULL);           /*  试图回收一个子进程          */
        if (iError < 0) {
            return  (iError);                                           /*  没有子线程                  */
        
        } else if (iError > 0) {
            return  (pidChld);                                          /*  子线程已经被回收            */
        }
        
        if (options & WNOHANG) {                                        /*  不等待                      */
            return  (0);
        }
        
        iError = sigwaitinfo(&sigsetSigchld, &siginfoChld);             /*  等待子进程信号              */
        if (iError < 0) {
            return  (iError);                                           /*  被其他信号打断              */
        }
    } while (1);
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: wait3
** 功能描述: wait for a child process to stop or terminate
** 输　入  : stat_loc      返回值
**           options       选项
**           prusage       被回收进程资源使用情况
** 输　出  : child pid 
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
pid_t wait3 (int *stat_loc, int options, struct rusage *prusage)
{
    INT               iError;
    
    sigset_t          sigsetSigchld;
    struct  siginfo   siginfoChld;
    pid_t             pidChld;
    
    LW_LD_VPROC      *pvproc = __LW_VP_GET_CUR_PROC();
    
    if (pvproc == LW_NULL) {
        errno = ECHILD;
        return  ((pid_t)-1);
    }
    
    sigemptyset(&sigsetSigchld);
    sigaddset(&sigsetSigchld, SIGCHLD);
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    do {
        iError = __reclaimAChild(pvproc, -1, &pidChld, 
                                 stat_loc, options, prusage);           /*  试图回收一个子进程          */
        if (iError < 0) {
            return  (iError);                                           /*  没有子线程                  */
        
        } else if (iError > 0) {
            return  (pidChld);                                          /*  子线程已经被回收            */
        }
        
        if (options & WNOHANG) {                                        /*  不等待                      */
            return  (0);
        }
        
        iError = sigwaitinfo(&sigsetSigchld, &siginfoChld);             /*  等待子进程信号              */
        if (iError < 0) {
            return  (iError);                                           /*  被其他信号打断              */
        }
    } while (1);
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: wait4
** 功能描述: wait for a child process to stop or terminate
** 输　入  : pid           指定的子线程
**           stat_loc      返回值
**           options       选项
**           prusage       被回收进程资源使用情况
** 输　出  : child pid 
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
pid_t wait4 (pid_t pid, int *stat_loc, int options, struct rusage *prusage)
{
    INT               iError;
    
    sigset_t          sigsetSigchld;
    struct  siginfo   siginfoChld;
    pid_t             pidChld;
    
    LW_LD_VPROC      *pvproc = __LW_VP_GET_CUR_PROC();
    
    if (pvproc == LW_NULL) {
        errno = ECHILD;
        return  ((pid_t)-1);
    }
    
    sigemptyset(&sigsetSigchld);
    sigaddset(&sigsetSigchld, SIGCHLD);
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    if (pid > 0) {                                                      /*  等待指定的子进程            */
        if (!__haveThisChild(pvproc, pid)) {
            errno = ECHILD;
            return  ((pid_t)-1);                                        /*  当前进程不存在指定的子进程  */
        }
    }
    
    do {
        iError = __reclaimAChild(pvproc, pid, &pidChld, 
                                 stat_loc, options, prusage);           /*  试图回收一个子进程          */
        if (iError < 0) {
            return  (iError);                                           /*  没有子线程                  */
        
        } else if (iError > 0) {
            return  (pidChld);                                          /*  子线程已经被回收            */
        }
        
        if (options & WNOHANG) {                                        /*  不等待                      */
            return  (0);
        }
        
        iError = sigwaitinfo(&sigsetSigchld, &siginfoChld);             /*  等待子进程信号              */
        if (iError < 0) {
            return  (iError);                                           /*  被其他信号打断              */
        }
    } while (1);
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: reclaimchild
** 功能描述: reclaim child process
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
void reclaimchild (void)
{
    INT             iError;
    pid_t           pidChld;
    LW_LD_VPROC    *pvproc = __LW_VP_GET_CUR_PROC();
    
    if (pvproc == LW_NULL) {
        return;
    }
    
    __THREAD_CANCEL_POINT();                                            /*  测试取消点                  */
    
    do {
        iError = __reclaimAChild(pvproc, -1, &pidChld, 
                                 LW_NULL, 0, LW_NULL);                  /*  试图回收一个子进程          */
        if (iError <= 0) {
            break;
        }
    } while (1);
}
/*********************************************************************************************************
** 函数名称: detach
** 功能描述: 脱离与父进程关系
** 输　入  : pid           指定的子线程
** 输　出  : 结果
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
int detach (pid_t pid)
{
    return  (vprocDetach(pid));
}
/*********************************************************************************************************
** 函数名称: daemon
** 功能描述: 将进程转换成守护进程
** 输　入  : nochdir       0 chdir root
**           noclose       0 change stdin stdout stderr to /dev/null
** 输　出  : 0: 成功  -1: 失败
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
int daemon (int nochdir, int noclose)
{
    INT                 iFd;
    pid_t               pid = getpid();
    LW_OBJECT_HANDLE    ulId;
    
    if (pid <= 0) {
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    if (!nochdir) {
        chdir("/");
    }
    
    if (!noclose) {
        iFd = open(_PATH_DEVNULL, O_RDWR);
        if (iFd < 0) {
            return  (PX_ERROR);
        }
        dup2(iFd, STD_IN);
        dup2(iFd, STD_OUT);
        dup2(iFd, STD_ERR);
        close(iFd);
    }
    
    detach(pid);                                                        /*  从父进程脱离                */
    setsid();
    
    ulId = vprocMainThread(pid);
    if (ulId != LW_OBJECT_HANDLE_INVALID) {
        API_ThreadDetach(ulId);                                         /*  主线程 detach               */
    }
    
    return  (ERROR_NONE);
}
#else                                                                   /*  NO MODULELOADER             */
/*********************************************************************************************************
** 函数名称: getpid
** 功能描述: 获得当前进程号
** 输　入  : 
** 输　出  : 进程号
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
pid_t getpid (void)
{
    return  (0);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

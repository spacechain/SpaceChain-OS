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
** 文   件   名: loader_vppatch.h
**
** 创   建   人: Han.hui (韩辉)
**
** 文件创建日期: 2010 年 12 月 08 日
**
** 描        述: 进程库
*********************************************************************************************************/

#ifndef __LOADER_VPPATCH_H
#define __LOADER_VPPATCH_H

#include "SylixOS.h"

/*********************************************************************************************************
  POSIX 支持
*********************************************************************************************************/

#if LW_CFG_POSIX_EN > 0
#include "../SylixOS/posix/include/posixLib.h"
#endif

/*********************************************************************************************************
  进程最大文件描述符 
  (因为进程0 1 2标准文件与内核一样映射方式不用, 这里的标准文件为真实打开的文件, 所以没有 STD_UNFIX 操作.
   为了继承内核文件描述符, 这里必须为 LW_CFG_MAX_FILES + 3)
*********************************************************************************************************/

#define LW_VP_MAX_FILES     (LW_CFG_MAX_FILES + 3)

/*********************************************************************************************************
  进程定时器
*********************************************************************************************************/

typedef struct lw_vproc_timer {
    ULONG                   VPT_ulCounter;                              /*  定时器当前定时时间          */
    ULONG                   VPT_ulInterval;                             /*  定时器自动装载值            */
} LW_LD_VPROC_T;

/*********************************************************************************************************
  进程控制块
*********************************************************************************************************/

typedef struct lw_ld_vproc {
    LW_LIST_LINE            VP_lineManage;                              /*  管理链表                    */
    LW_LIST_RING_HEADER     VP_ringModules;                             /*  模块链表                    */
    
    FUNCPTR                 VP_pfuncProcess;                            /*  进程主入口                  */
    PVOIDFUNCPTR            VP_pfuncMalloc;                             /*  进程私有内存分配            */
    VOIDFUNCPTR             VP_pfuncFree;                               /*  进程私有内存回收            */
    
    LW_OBJECT_HANDLE        VP_ulModuleMutex;                           /*  进程模块链表锁              */
    
    BOOL                    VP_bRunAtExit;                              /*  是否允许 atexit 安装的函数  */
    BOOL                    VP_bImmediatelyTerm;                        /*  是否是被立即退出            */
    
    pid_t                   VP_pid;                                     /*  进程号                      */
    BOOL                    VP_bIssetugid;                              /*  是否设置的 ugid             */
    PCHAR                   VP_pcName;                                  /*  进程名称                    */
    LW_OBJECT_HANDLE        VP_ulMainThread;                            /*  主线程句柄                  */
    PVOID                   VP_pvProcInfo;                              /*  proc 文件系统信息           */
    
    clock_t                 VP_clockUser;                               /*  times 对应的 utime          */
    clock_t                 VP_clockSystem;                             /*  times 对应的 stime          */
    clock_t                 VP_clockCUser;                              /*  times 对应的 cutime         */
    clock_t                 VP_clockCSystem;                            /*  times 对应的 cstime         */
    
    PLW_IO_ENV              VP_pioeIoEnv;                               /*  I/O 环境                    */
    LW_OBJECT_HANDLE        VP_ulWaitForExit;                           /*  主线程等待结束              */

#define __LW_VP_INIT        0
#define __LW_VP_RUN         1
#define __LW_VP_EXIT        2
#define __LW_VP_STOP        3
    INT                     VP_iStatus;                                 /*  当前进程状态                */
    INT                     VP_iExitCode;                               /*  结束代码                    */
    INT                     VP_iSigCode;                                /*  iSigCode                    */
    
#define __LW_VP_FT_DAEMON   0x01                                        /*  daemon 进程                 */
    ULONG                   VP_ulFeatrues;                              /*  进程 featrues               */
    
    struct lw_ld_vproc     *VP_pvprocFather;                            /*  父亲 (NULL 表示孤儿进程)    */
    LW_LIST_LINE_HEADER     VP_plineChild;                              /*  儿子进程链表头              */
    LW_LIST_LINE            VP_lineBrother;                             /*  兄弟进程                    */
    pid_t                   VP_pidGroup;                                /*  组 id 号                    */
    LW_LIST_LINE_HEADER     VP_plineThread;                             /*  子线程链表                  */
    
    LW_FD_DESC              VP_fddescTbl[LW_VP_MAX_FILES];              /*  进程 fd 表                  */
    
    INT                     VP_iExitMode;                               /*  退出模式                    */
    LW_LD_VPROC_T           VP_vptimer[3];                              /*  REAL / VIRTUAL / PROF 定时器*/
    INT64                   VP_i64Tick;                                 /*  itimer tick 辅助变量        */
    
    LW_LIST_LINE_HEADER     VP_plineMap;                                /*  虚拟内存空间                */
    
#if LW_CFG_POSIX_EN > 0
    __PX_VPROC_CONTEXT      VP_pvpCtx;                                  /*  POSIX 进程上下文            */
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
} LW_LD_VPROC;

/*********************************************************************************************************
  进程装载完成停止控制 (sigval.sival_int == 0 表明装载完成并停止, < 0 表示装载异常, 进程正在退出)
*********************************************************************************************************/

typedef struct {
    INT                     VPS_iSigNo;                                 /*  装载完成后发送的信号        */
    LW_OBJECT_HANDLE        VPS_ulId;                                   /*  目标线程 (或进程)           */
} LW_LD_VPROC_STOP;

/*********************************************************************************************************
  内核进程控制块
*********************************************************************************************************/

extern LW_LD_VPROC          _G_vprocKernel;

/*********************************************************************************************************
  vprocess 操作锁
*********************************************************************************************************/

#define LW_VP_LOCK(a)       API_SemaphoreMPend(a->VP_ulModuleMutex, LW_OPTION_WAIT_INFINITE)
#define LW_VP_UNLOCK(a)     API_SemaphoreMPost(a->VP_ulModuleMutex)

/*********************************************************************************************************
  vprocess 控制块操作
*********************************************************************************************************/

#define __LW_VP_GET_TCB_PROC(ptcb)      ((LW_LD_VPROC *)(ptcb->TCB_pvVProcessContext))
#define __LW_VP_GET_CUR_PROC()          vprocGetCur()
#define __LW_VP_SET_CUR_PROC(pvproc)    vprocSetCur(pvproc)

/*********************************************************************************************************
  vprocess 内部操作
*********************************************************************************************************/

VOID                vprocThreadExitHook(PVOID  pvVProc, LW_OBJECT_HANDLE  ulId);
INT                 vprocSetGroup(pid_t  pid, pid_t  pgid);
pid_t               vprocGetGroup(pid_t  pid);
pid_t               vprocGetFather(pid_t  pid);
INT                 vprocDetach(pid_t  pid);
LW_LD_VPROC        *vprocCreate(CPCHAR  pcFile);
INT                 vprocDestroy(LW_LD_VPROC *pvproc);
LW_LD_VPROC        *vprocGet(pid_t  pid);
LW_LD_VPROC        *vprocGetCur(VOID);
VOID                vprocSetCur(LW_LD_VPROC  *pvproc);
pid_t               vprocGetPidByTcb(PLW_CLASS_TCB ptcb);
pid_t               vprocGetPidByTcbNoLock(PLW_CLASS_TCB  ptcb);
pid_t               vprocGetPidByTcbdesc(PLW_CLASS_TCB_DESC  ptcbdesc);
pid_t               vprocGetPidByThread(LW_OBJECT_HANDLE  ulId);
LW_OBJECT_HANDLE    vprocMainThread(pid_t pid);
INT                 vprocNotifyParent(LW_LD_VPROC *pvproc, INT  iSigCode, BOOL  bUpDateStat);
VOID                vprocReclaim(LW_LD_VPROC *pvproc, BOOL  bFreeVproc);
INT                 vprocSetImmediatelyTerm(pid_t  pid);
VOID                vprocExit(LW_LD_VPROC *pvproc, LW_OBJECT_HANDLE  ulId, INT  iCode);
VOID                vprocExitNotDestroy(LW_LD_VPROC *pvproc);
INT                 vprocRun(LW_LD_VPROC      *pvproc, 
                             LW_LD_VPROC_STOP *pvpstop,
                             CPCHAR            pcFile, 
                             CPCHAR            pcEntry, 
                             INT              *piRet,
                             INT               iArgC, 
                             CPCHAR            ppcArgV[],
                             CPCHAR            ppcEnv[]);
VOID                vprocTickHook(VOID);
PLW_IO_ENV          vprocIoEnvGet(PLW_CLASS_TCB  ptcb);
FUNCPTR             vprocGetMain(VOID);
pid_t               vprocFindProc(PVOID  pvAddr);
INT                 vprocGetPath(pid_t  pid, PCHAR  pcPath, size_t stMaxLen);

#if LW_CFG_GDB_EN > 0
ssize_t             vprocGetModsInfo(pid_t  pid, PCHAR  pcBuff, size_t stMaxLen);
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

/*********************************************************************************************************
  进程内部线程操作
*********************************************************************************************************/

VOID                vprocThreadAdd(PVOID   pvVProc, PLW_CLASS_TCB  ptcb);
VOID                vprocThreadDelete(PVOID   pvVProc, PLW_CLASS_TCB  ptcb);
INT                 vprocThreadNum(pid_t  pid, ULONG  *pulNum);

#if LW_CFG_GDB_EN > 0
VOID                vprocThreadDebugStop(PVOID  pvVProc);
VOID                vprocThreadDebugContinue(PVOID  pvVProc);
UINT                vprocThreadDebugGet(PVOID  pvVProc, LW_OBJECT_HANDLE  ulThread[], UINT   uiTableNum);
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

VOID                vprocThreadKill(PVOID  pvVProc);

#if LW_CFG_SIGNAL_EN > 0
INT                 vprocThreadSigaction(PVOID  pvVProc, VOIDFUNCPTR  pfunc, INT  iSigIndex, 
                                         const struct sigaction  *psigactionNew);
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */

#if LW_CFG_SMP_EN > 0
INT                 vprocThreadAffinity(PVOID  pvVProc, size_t  stSize, const PLW_CLASS_CPUSET  pcpuset);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

/*********************************************************************************************************
  进程 CPU 亲和度
*********************************************************************************************************/

#if LW_CFG_SMP_EN > 0
INT                 vprocSetAffinity(pid_t  pid, size_t  stSize, const PLW_CLASS_CPUSET  pcpuset);
INT                 vprocGetAffinity(pid_t  pid, size_t  stSize, PLW_CLASS_CPUSET  pcpuset);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

/*********************************************************************************************************
  vprocess 文件描述符操作
*********************************************************************************************************/

VOID                vprocIoFileInit(LW_LD_VPROC *pvproc);               /*  vprocCreate  内被调用       */
VOID                vprocIoFileDeinit(LW_LD_VPROC *pvproc);             /*  vprocDestroy 内被调用       */
PLW_FD_ENTRY        vprocIoFileGet(INT  iFd, BOOL  bIsIgnAbn);
PLW_FD_ENTRY        vprocIoFileGetEx(LW_LD_VPROC *pvproc, INT  iFd, BOOL  bIsIgnAbn);
PLW_FD_DESC         vprocIoFileDescGet(INT  iFd, BOOL  bIsIgnAbn);
INT                 vprocIoFileDup(PLW_FD_ENTRY pfdentry, INT  iMinFd);
INT                 vprocIoFileDup2(PLW_FD_ENTRY pfdentry, INT  iNewFd);
INT                 vprocIoFileRefInc(INT  iFd);
INT                 vprocIoFileRefDec(INT  iFd);
INT                 vprocIoFileRefGet(INT  iFd);

/*********************************************************************************************************
  文件描述符传递操作
*********************************************************************************************************/

INT                 vprocIoFileDupFrom(pid_t  pidSrc, INT  iFd);
INT                 vprocIoFileRefIncByPid(pid_t  pid, INT  iFd);
INT                 vprocIoFileRefDecByPid(pid_t  pid, INT  iFd);

/*********************************************************************************************************
  资源管理器调用以下函数
*********************************************************************************************************/

VOID                vprocIoReclaim(pid_t  pid, BOOL  bIsExec);

/*********************************************************************************************************
  进程线程堆栈
*********************************************************************************************************/

PVOID               vprocStackAlloc(PLW_CLASS_TCB  ptcbNew, ULONG  ulOption, size_t  stSize);
VOID                vprocStackFree(PLW_CLASS_TCB  ptcbDel, PVOID  pvStack);

/*********************************************************************************************************
  进程定时器
*********************************************************************************************************/

#if LW_CFG_PTIMER_EN > 0
VOID                vprocItimerMainHook(VOID);
VOID                vprocItimerEachHook(PLW_CLASS_CPU  pcpu, LW_LD_VPROC  *pvproc);

INT                 vprocSetitimer(INT        iWhich, 
                                   ULONG      ulCounter,
                                   ULONG      ulInterval,
                                   ULONG     *pulCounter,
                                   ULONG     *pulInterval);
INT                 vprocGetitimer(INT        iWhich, 
                                   ULONG     *pulCounter,
                                   ULONG     *pulInterval);
#endif                                                                  /*  LW_CFG_PTIMER_EN > 0        */
#endif                                                                  /*  __LOADER_SYMBOL_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/

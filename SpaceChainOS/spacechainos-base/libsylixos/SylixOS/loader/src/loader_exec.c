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
** 文   件   名: loader_exec.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 03 月 26 日
**
** 描        述: unistd exec family of functions.
**
** 注        意: sylixos 所有环境变量均为全局属性, 所以当使用 envp 时, 整个系统的环境变量都会随之改变.

** BUG:
2012.03.30  加入 spawn 系列函数接口.
2012.08.24  spawn 接口, P_WAIT 时返回新进程的返回值, 如果不是返回进程 id.
2012.08.25  修正创建进程对参数的操作错误.
2012.12.07  __processShell() P_OVERLAY 模式将不再退出.
2012.12.10  开始支持进程内环境变量, 这里进程升级.
2013.04.01  修正 GCC 4.7.3 引发的新 warning.
2013.06.07  进程创建参数加入工作目录选项.
2013.09.21  exec 在当前进程上下文中运行新的文件不再切换主线程.
2015.07.24  修正 spawnle execle 函数对环境变量表操作错误.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_STDARG
#define  __SYLIXOS_SPAWN
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_lib.h"
#include "../include/loader_exec.h"
#include "process.h"
#include "sys/wait.h"
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
INT  __ldGetFilePath(CPCHAR  pcParam, PCHAR  pcPathBuffer, size_t  stMaxLen);
/*********************************************************************************************************
** 函数名称: __spawnArgFree
** 功能描述: free a spawn args (由于使用 lib_strdup 分配, 所以这里使用 lib_free 释放)
** 输　入  : psarg
** 输　出  : NONE
** 全局变量:
** 调用模块: 
*********************************************************************************************************/
VOID  __spawnArgFree (__PSPAWN_ARG  psarg)
{
    INT             i;
    PLW_LIST_LINE   plineTemp;
    __spawn_action *pspawnactFree;
    
    if (!psarg) {
        return;
    }
    
    plineTemp = psarg->SA_plineActions;
    while (plineTemp) {
        pspawnactFree = (__spawn_action *)plineTemp;
        plineTemp = _list_line_get_next(plineTemp);
        lib_free(pspawnactFree);
    }
    
    for (i = 0; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {
        if (psarg->SA_pcParamList[i]) {
            lib_free(psarg->SA_pcParamList[i]);
        }
        if (psarg->SA_pcpcEvn[i]) {
            lib_free(psarg->SA_pcpcEvn[i]);
        }
    }
    
    if (psarg->SA_pcWd) {
        lib_free(psarg->SA_pcWd);
    }
    
    if (psarg->SA_pcPath) {
        lib_free(psarg->SA_pcPath);
    }
    
    lib_free(psarg);
}
/*********************************************************************************************************
** 函数名称: __spawnArgCreate
** 功能描述: create a spawn args
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块: 
*********************************************************************************************************/
__PSPAWN_ARG  __spawnArgCreate (VOID)
{
    __PSPAWN_ARG        psarg;

    psarg = (__PSPAWN_ARG)lib_malloc(sizeof(__SPAWN_ARG));
    if (!psarg) {
        return  (LW_NULL);
    }
    lib_bzero(psarg, sizeof(__SPAWN_ARG));
    
    return  (psarg);
}
/*********************************************************************************************************
** 函数名称: __spawnArgActionDup
** 功能描述: 复制一个 file action
** 输　入  : plineAction     action 表头
** 输　出  : 新的 action 表
** 全局变量:
** 调用模块: 
*********************************************************************************************************/
LW_LIST_LINE_HEADER  __spawnArgActionDup (LW_LIST_LINE_HEADER  plineAction)
{
    PLW_LIST_LINE           plineTemp;
    LW_LIST_LINE_HEADER     plineNew = LW_NULL;
    __spawn_action         *pspawnactOrg;
    __spawn_action         *pspawnactNew;
    
    if (plineAction == LW_NULL) {
        return  (LW_NULL);
    }
    
    plineTemp = plineAction;
    while (plineTemp) {
        pspawnactOrg = (__spawn_action *)plineTemp;
        plineTemp = _list_line_get_next(plineTemp);
        
        switch (pspawnactOrg->SFA_iType) {
        
        case __FILE_ACTIONS_DO_CLOSE:
        case __FILE_ACTIONS_DO_DUP2:
            pspawnactNew = (__spawn_action *)lib_malloc(sizeof(__spawn_action));
            if (pspawnactNew == LW_NULL) {
                goto    __fail;
            }
            *pspawnactNew = *pspawnactOrg;
            break;
            
        case __FILE_ACTIONS_DO_OPEN:
            pspawnactNew = (__spawn_action *)lib_malloc(sizeof(__spawn_action) + 
                                lib_strlen(pspawnactOrg->SFA_pcPath) + 1);
            if (pspawnactNew == LW_NULL) {
                goto    __fail;
            }
            *pspawnactNew = *pspawnactOrg;
            pspawnactNew->SFA_pcPath = (PCHAR)pspawnactNew + sizeof(__spawn_action);
            lib_strcpy(pspawnactNew->SFA_pcPath, pspawnactOrg->SFA_pcPath);
            break;
            
        default:
            goto    __fail;
        }
        
        _List_Line_Add_Ahead(&pspawnactNew->SFA_lineManage,
                             &plineNew);
    }
    
    return  (plineNew);
    
__fail:
    plineTemp = plineNew;
    while (plineTemp) {
        pspawnactNew = (__spawn_action *)plineTemp;
        plineTemp = _list_line_get_next(plineTemp);
        lib_free(pspawnactNew);
    }
    
    _ErrorHandle(ENOMEM);
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __spawnArgProc
** 功能描述: 处里一个进程参数
** 输　入  : psarg     spawn args
** 输　出  : INT
** 全局变量:
** 调用模块: 
*********************************************************************************************************/
INT  __spawnArgProc (__PSPAWN_ARG  psarg)
{
    LW_LD_VPROC     *pvproc = psarg->SA_pvproc;
    PLW_LIST_LINE    plineTemp;
    __spawn_action  *pspawnact;
    INT              iFd, iFdNew;
    
    /*
     *  TODO: POSIX_SPAWN_RESETIDS 和 POSIX_SPAWN_SETSIGDEF 暂时不支持
     */
    if (psarg->SA_pcWd) {
        chdir(psarg->SA_pcWd);                                          /*  设置工作目录                */
    }
     
    if (psarg->SA_spawnattr.SPA_sFlags & POSIX_SPAWN_SETPGROUP) {       /*  设置 group                  */
        if (psarg->SA_spawnattr.SPA_pidGroup == 0) {
            pvproc->VP_pidGroup = pvproc->VP_pid;
        } else if (psarg->SA_spawnattr.SPA_pidGroup > 0) {
            pvproc->VP_pidGroup = psarg->SA_spawnattr.SPA_pidGroup;
        }
    }
    
    if (psarg->SA_spawnattr.SPA_sFlags & POSIX_SPAWN_SETSIGMASK) {      /*  设置信号屏蔽码              */
        sigprocmask(SIG_BLOCK, &psarg->SA_spawnattr.SPA_sigsetMask, LW_NULL);
    }
    
    if (psarg->SA_spawnattr.SPA_sFlags & POSIX_SPAWN_SETSCHEDULER) {    /*  设置调度器参数              */
        API_ThreadSetSchedParam(API_ThreadIdSelf(), 
                                (UINT8)psarg->SA_spawnattr.SPA_iPolicy,
                                LW_OPTION_RESPOND_AUTO);
    }
    
    if (psarg->SA_spawnattr.SPA_sFlags & POSIX_SPAWN_SETSCHEDPARAM) {   /*  设置调度器优先级            */
        UINT8               ucPriority;
        struct sched_param *pschedparam = &psarg->SA_spawnattr.SPA_schedparam;
        if ((pschedparam->sched_priority < __PX_PRIORITY_MIN) ||
            (pschedparam->sched_priority > __PX_PRIORITY_MAX)) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        ucPriority= (UINT8)PX_PRIORITY_CONVERT(pschedparam->sched_priority);
        API_ThreadSetPriority(API_ThreadIdSelf(), ucPriority);
    }
    
    for (plineTemp  = psarg->SA_plineActions;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  设置 file action            */
        
        pspawnact = (__spawn_action *)plineTemp;
        
        switch (pspawnact->SFA_iType) {
        
        case __FILE_ACTIONS_DO_CLOSE:
            close(pspawnact->SFA_iCloseFd);
            break;
            
        case __FILE_ACTIONS_DO_OPEN:
            iFd = open(pspawnact->SFA_pcPath, pspawnact->SFA_iFlag, pspawnact->SFA_mode);
            if (iFd < 0) {
                return  (PX_ERROR);
            }
            iFdNew = dup2(iFd, pspawnact->SFA_iOpenFd);
            close(iFd);                                                 /*  需要关闭打开的文件          */
            if (iFdNew != pspawnact->SFA_iOpenFd) {
                return  (PX_ERROR);
            }
            break;
            
        case __FILE_ACTIONS_DO_DUP2:
            iFdNew = dup2(pspawnact->SFA_iDup2Fd, pspawnact->SFA_iDup2NewFd);
            if (iFdNew != pspawnact->SFA_iDup2NewFd) {
                return  (PX_ERROR);
            }
            break;
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __spawnStopProc
** 功能描述: 处里一个进程 stop 属性
** 输　入  : psarg     spawn args
**           pvpstop   stop 保存缓冲
** 输　出  : 返回的处理结果
** 全局变量:
** 调用模块: 
*********************************************************************************************************/
static LW_LD_VPROC_STOP *__spawnStopGet (__PSPAWN_ARG  psarg, LW_LD_VPROC_STOP *pvpstop)
{
    posix_spawnopt_t *popt = &psarg->SA_spawnattr.SPA_opt;
    
    if (!__issig(popt->SPO_iSigNo)) {
        return  (LW_NULL);
    }
    
    pvpstop->VPS_iSigNo   = popt->SPO_iSigNo;
    pvpstop->VPS_ulId     = popt->SPO_ulId;
    
    return  (pvpstop);
}
/*********************************************************************************************************
** 函数名称: __execShell
** 功能描述: exec shell (注意, __execShell 本身就是进程主线程, 不管装载成功与否都需要通过 vprocExit 退出)
** 输　入  : pvArg
** 输　出  : return code
** 全局变量:
** 调用模块: 
** 注  意  : vprocExitNotDestroy() 调用结束后, 才能安装新的 cleanup 函数.
*********************************************************************************************************/
static INT  __execShell (PVOID  pvArg)
{
    __PSPAWN_ARG        psarg = (__PSPAWN_ARG)pvArg;
    LW_LD_VPROC_STOP    vpstop;
    LW_LD_VPROC_STOP   *pvpstop;
    INT                 iError;
    INT                 iRet = PX_ERROR;
    
    vprocExitNotDestroy(psarg->SA_pvproc);                              /*  等待进程结束当前环境        */
    
    vprocReclaim(psarg->SA_pvproc, LW_FALSE);                           /*  回收进程但不释放进程控制块  */
    
    API_ThreadCleanupPush(__spawnArgFree, (PVOID)psarg);                /*  主线程退出时释放 pvArg      */
    
    pvpstop = __spawnStopGet(psarg, &vpstop);                           /*  获得停止属性                */
    
    iError = vprocRun(psarg->SA_pvproc, pvpstop,
                      psarg->SA_pcPath, LW_LD_DEFAULT_ENTRY, 
                      &iRet, psarg->SA_iArgs, 
                      (CPCHAR *)psarg->SA_pcParamList,
                      (CPCHAR *)psarg->SA_pcpcEvn);                     /*  此线程将变成进程内主线程    */
    if (iError) {
        iRet = iError;                                                  /*  加载失败                    */
    }
    
    vprocExit(psarg->SA_pvproc, API_ThreadIdSelf(), iRet);              /*  进程退出, 本线程就是主线程  */
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __processShell
** 功能描述: 新的进程执行外壳
** 输　入  : pvArg
** 输　出  : return code
** 全局变量:
** 调用模块: 
*********************************************************************************************************/
static INT  __processShell (PVOID  pvArg)
{
    __PSPAWN_ARG        psarg = (__PSPAWN_ARG)pvArg;
    LW_LD_VPROC_STOP    vpstop;
    LW_LD_VPROC_STOP   *pvpstop;
    INT                 iError;
    INT                 iRet = PX_ERROR;
    CHAR                cName[LW_CFG_OBJECT_NAME_SIZE];

    API_ThreadCleanupPush(__spawnArgFree, pvArg);                       /*  主线程退出时释放 pvArg      */
    
    __LW_VP_SET_CUR_PROC(psarg->SA_pvproc);                             /*  主线程记录进程控制块        */
                                                                        /*  保证下面的 file actions 正常*/
    iError = __spawnArgProc(psarg);                                     /*  初始化进程参数              */
    if (iError < ERROR_NONE) {
        vprocDestroy(psarg->SA_pvproc);
        return  (iError);
    }
    
    lib_strlcpy(cName, _PathLastNamePtr(psarg->SA_pcPath), LW_CFG_OBJECT_NAME_SIZE);
    
    API_ThreadSetName(API_ThreadIdSelf(), cName);                       /*  设置新名字                  */
    
    pvpstop = __spawnStopGet(psarg, &vpstop);                           /*  获得停止属性                */
    
    iError = vprocRun(psarg->SA_pvproc, pvpstop, 
                      psarg->SA_pcPath, LW_LD_DEFAULT_ENTRY, 
                      &iRet, psarg->SA_iArgs, 
                      (CPCHAR *)psarg->SA_pcParamList,
                      (CPCHAR *)psarg->SA_pcpcEvn);                     /*  此线程将变成进程内主线程    */
    if (iError) {
        iRet = iError;                                                  /*  加载失败                    */
    }
    
    vprocExit(psarg->SA_pvproc, API_ThreadIdSelf(), iRet);              /*  进程退出, 本线程就是主线程  */
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __processStart
** 功能描述: start a process shell (新的继承继承当前线程优先级和堆栈大小)
** 输　入  : mode          run mode
**           psarg
** 输　出  : return code
** 全局变量:
** 调用模块: 
** 注  意  : 这里新建的线程为内核线程, 待 vprocRun 时, 自动转为进程内主线程.
*********************************************************************************************************/
INT  __processStart (INT  mode, __PSPAWN_ARG  psarg)
{
    LW_CLASS_THREADATTR threadattr;
    LW_OBJECT_HANDLE    hHandle;
    PLW_CLASS_TCB       ptcbCur;
    INT                 iRetValue = ERROR_NONE;
    pid_t               pid;
    ULONG               ulOption;
    size_t              stStackSize;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    ulOption = LW_OPTION_THREAD_STK_CHK
             | LW_OPTION_OBJECT_GLOBAL
             | LW_OPTION_THREAD_STK_MAIN
             | psarg->SA_spawnattr.SPA_opt.SPO_ulMainOption;
             
    if (psarg->SA_spawnattr.SPA_opt.SPO_stStackSize) {
        stStackSize = psarg->SA_spawnattr.SPA_opt.SPO_stStackSize;
    } else {
        stStackSize = ptcbCur->TCB_stStackSize * sizeof(LW_STACK);
    }
    
    API_ThreadAttrBuild(&threadattr,
                        stStackSize,
                        ptcbCur->TCB_ucPriority,
                        ulOption,
                        (PVOID)psarg);
                        
    if (psarg->SA_spawnattr.SPA_sFlags & POSIX_SPAWN_SETSCHEDPARAM) {   /*  设置优先级                  */
        threadattr.THREADATTR_ucPriority = 
            (UINT8)PX_PRIORITY_CONVERT(psarg->SA_spawnattr.SPA_schedparam.sched_priority);
    }

    if (mode == P_OVERLAY) {                                            /*  替换当前进程空间            */
        psarg->SA_pvproc = __LW_VP_GET_CUR_PROC();                      /*  获得当前进程控制块          */
        if (psarg->SA_pvproc == LW_NULL) {
            __spawnArgFree(psarg);
            _ErrorHandle(ENOTSUP);
            return  (PX_ERROR);
        }
        
        if (psarg->SA_pvproc->VP_ulMainThread != API_ThreadIdSelf()) {  /*  必须是主线程                */
            __spawnArgFree(psarg);
            _ErrorHandle(ENOTSUP);
            return  (PX_ERROR);
        }
        
        if (API_ThreadRestartEx(psarg->SA_pvproc->VP_ulMainThread,
                                (PTHREAD_START_ROUTINE)__execShell, 
                                (PVOID)psarg)) {                        /*  重启主线程                  */
            __spawnArgFree(psarg);
            return  (PX_ERROR);
        }
        
        return  (PX_ERROR);                                             /*  理论上运行不到这里          */
        
    } else {                                                            /*  创建一个新的进程            */
        psarg->SA_pvproc = vprocCreate(psarg->SA_pcPath);
        if (psarg->SA_pvproc == LW_NULL) {
            __spawnArgFree(psarg);
            return  (PX_ERROR);
        }
        pid = psarg->SA_pvproc->VP_pid;                                 /*  子进程 id                   */
                            
        hHandle = API_ThreadInit("t_spawn", (PTHREAD_START_ROUTINE)__processShell, 
                                 &threadattr, LW_NULL);
        if (!hHandle) {
            vprocDestroy(psarg->SA_pvproc);
            __spawnArgFree(psarg);
            return  (PX_ERROR);
        }
        
        API_ThreadStart(hHandle);
        
        if (mode == P_WAIT) {
            if (waitpid(pid, &iRetValue, 0) < 0) {                      /*  等待进程结束                */
                return  (PX_ERROR);
            
            } else {
                return  (iRetValue);
            }
            
        } else {
            return  (pid);                                              /*  子进程 id                   */
        }
    }
}
/*********************************************************************************************************
** 函数名称: spawnl
** 功能描述: spawn family of functions
** 输　入  : mode          run mode
**           path          pathname that identifies the new module image file.
**           argv0         args...
** 输　出  : P_WAIT     The exit status of the child process.
**           P_NOWAIT   The process ID of the child process.
**           P_NOWAITO  The process ID of the child process.
** 全局变量:
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int spawnl (int mode, const char *path, const char *argv0, ...)
{
    __PSPAWN_ARG        psarg;
    INT                 i;
    va_list             valist;
    CPCHAR              pcArg;
    
    if (!path) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    psarg = __spawnArgCreate();
    if (!psarg) {
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
#if __STDC__
    va_start(valist, argv0);
#else
    va_start(valist);
#endif

    if (argv0) {
        psarg->SA_pcParamList[0] = lib_strdup(argv0);
        if (!psarg->SA_pcParamList[0]) {
            __spawnArgFree(psarg);
            errno = ENOMEM;
            return  (PX_ERROR);
        }
        
        for (i = 1; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {               /*  循环读取参数                */
            pcArg = (CPCHAR)va_arg(valist, const char *);
            if (pcArg == LW_NULL) {
                break;
            }
            psarg->SA_pcParamList[i] = lib_strdup(pcArg);
            if (!psarg->SA_pcParamList[i]) {
                __spawnArgFree(psarg);
                errno = ENOMEM;
                return  (PX_ERROR);
            }
        }
        psarg->SA_iArgs = i;
    
    } else {
        psarg->SA_iArgs = 0;
    }
    
    va_end(valist);
    
    psarg->SA_pcPath = lib_strdup(path);
    if (!psarg->SA_pcPath) {
        __spawnArgFree(psarg);
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
    return  (__processStart(mode, psarg));
}
/*********************************************************************************************************
** 函数名称: spawnle
** 功能描述: spawn family of functions
** 输　入  : mode          run mode
**           path          pathname that identifies the new module image file.
**           argv0         args...
** 输　出  : P_WAIT     The exit status of the child process.
**           P_NOWAIT   The process ID of the child process.
**           P_NOWAITO  The process ID of the child process.
** 全局变量:
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int spawnle (int mode, const char *path, const char *argv0, ...)
{
    __PSPAWN_ARG        psarg;
    INT                 i;
    va_list             valist;
    CPCHAR              pcArg;
    char * const       *envp;
    
    if (!path) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    psarg = __spawnArgCreate();
    if (!psarg) {
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
#if __STDC__
    va_start(valist, argv0);
#else
    va_start(valist);
#endif

    if (argv0) {
        psarg->SA_pcParamList[0] = lib_strdup(argv0);
        if (!psarg->SA_pcParamList[0]) {
            __spawnArgFree(psarg);
            errno = ENOMEM;
            return  (PX_ERROR);
        }
        
        for (i = 1; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {               /*  循环读取参数                */
            pcArg = (CPCHAR)va_arg(valist, const char *);
            if (pcArg == LW_NULL) {
                break;
            }
            psarg->SA_pcParamList[i] = lib_strdup(pcArg);
            if (!psarg->SA_pcParamList[i]) {
                __spawnArgFree(psarg);
                errno = ENOMEM;
                return  (PX_ERROR);
            }
        }
        psarg->SA_iArgs = i;
        
        envp = (char * const *)va_arg(valist, char * const *);
        
        for (i = 0; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {               /*  循环读取环境变量            */
            if (envp[i]) {
                psarg->SA_pcpcEvn[i] = lib_strdup(envp[i]);
                if (!psarg->SA_pcpcEvn[i]) {
                    __spawnArgFree(psarg);
                    errno = ENOMEM;
                    return  (PX_ERROR);
                }
            } else {
                break;
            }
        }
        psarg->SA_iEvns = i;
    
    } else {
        psarg->SA_iArgs = 0;
    }
    
    va_end(valist);
    
    psarg->SA_pcPath = lib_strdup(path);
    if (!psarg->SA_pcPath) {
        __spawnArgFree(psarg);
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
    return  (__processStart(mode, psarg));
}
/*********************************************************************************************************
** 函数名称: spawnlp
** 功能描述: spawn family of functions
** 输　入  : mode          run mode
**           file          pathname that identifies the new module image file.
**           argv0         args...
** 输　出  : P_WAIT     The exit status of the child process.
**           P_NOWAIT   The process ID of the child process.
**           P_NOWAITO  The process ID of the child process.
** 全局变量:
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int spawnlp (int mode, const char *file, const char *argv0, ...)
{
    __PSPAWN_ARG        psarg;
    INT                 i;
    va_list             valist;
    CPCHAR              pcArg;
    CHAR                cFilePath[MAX_FILENAME_LENGTH];
    
    if (!file) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    if (__ldGetFilePath(file, cFilePath, MAX_FILENAME_LENGTH) != ERROR_NONE) {
        errno = ENOENT;
        return  (PX_ERROR);                                             /*  无法找到命令                */
    }
    
    psarg = __spawnArgCreate();
    if (!psarg) {
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
#if __STDC__
    va_start(valist, argv0);
#else
    va_start(valist);
#endif

    if (argv0) {
        psarg->SA_pcParamList[0] = lib_strdup(argv0);
        if (!psarg->SA_pcParamList[0]) {
            __spawnArgFree(psarg);
            errno = ENOMEM;
            return  (PX_ERROR);
        }
        
        for (i = 1; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {               /*  循环读取参数                */
            pcArg = (CPCHAR)va_arg(valist, const char *);
            if (pcArg == LW_NULL) {
                break;
            }
            psarg->SA_pcParamList[i] = lib_strdup(pcArg);
            if (!psarg->SA_pcParamList[i]) {
                __spawnArgFree(psarg);
                errno = ENOMEM;
                return  (PX_ERROR);
            }
        }
        psarg->SA_iArgs = i;
        
    } else {
        psarg->SA_iArgs = 0;
    }
    
    va_end(valist);
    
    psarg->SA_pcPath = lib_strdup(cFilePath);
    if (!psarg->SA_pcPath) {
        __spawnArgFree(psarg);
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
    return  (__processStart(mode, psarg));
}
/*********************************************************************************************************
** 函数名称: spawnv
** 功能描述: spawn family of functions
** 输　入  : mode          run mode
**           path          pathname that identifies the new module image file.
**           argv          args...
** 输　出  : P_WAIT     The exit status of the child process.
**           P_NOWAIT   The process ID of the child process.
**           P_NOWAITO  The process ID of the child process.
** 全局变量:
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int spawnv (int mode, const char *path, char * const *argv)
{
    __PSPAWN_ARG        psarg;
    INT                 i;
    PCHAR               argvNull[1] = {NULL};
    
    if (!path) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    if (!argv) {
        argv = (char * const *)argvNull;
    }
    
    psarg = __spawnArgCreate();
    if (!psarg) {
        errno = ENOMEM;
        return  (PX_ERROR);
    }

    for (i = 0; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {
        if (argv[i]) {
            psarg->SA_pcParamList[i] = lib_strdup(argv[i]);
            if (!psarg->SA_pcParamList[i]) {
                __spawnArgFree(psarg);
                errno = ENOMEM;
                return  (PX_ERROR);
            }
        } else {
            break;
        }
    }
    psarg->SA_iArgs = i;
    
    psarg->SA_pcPath = lib_strdup(path);
    if (!psarg->SA_pcPath) {
        __spawnArgFree(psarg);
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
    return  (__processStart(mode, psarg));
}
/*********************************************************************************************************
** 函数名称: spawnve
** 功能描述: spawn family of functions
** 输　入  : mode          run mode
**           path          pathname that identifies the new module image file.
**           argv          args...
**           envp[]        evar array need to set.
** 输　出  : P_WAIT     The exit status of the child process.
**           P_NOWAIT   The process ID of the child process.
**           P_NOWAITO  The process ID of the child process.
** 全局变量:
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int spawnve (int mode, const char *path, char * const *argv, char * const *envp)
{
    __PSPAWN_ARG        psarg;
    INT                 i;
    PCHAR               argvNull[1] = {NULL};
    
    if (!path) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    if (!argv) {
        argv = (char * const *)argvNull;
    }
    if (!envp) {
        envp = (char * const *)argvNull;
    }
    
    psarg = __spawnArgCreate();
    if (!psarg) {
        errno = ENOMEM;
        return  (PX_ERROR);
    }

    for (i = 0; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {
        if (argv[i]) {
            psarg->SA_pcParamList[i] = lib_strdup(argv[i]);
            if (!psarg->SA_pcParamList[i]) {
                __spawnArgFree(psarg);
                errno = ENOMEM;
                return  (PX_ERROR);
            }
        } else {
            break;
        }
    }
    psarg->SA_iArgs = i;
    
    for (i = 0; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {
        if (envp[i]) {
            psarg->SA_pcpcEvn[i] = lib_strdup(envp[i]);
            if (!psarg->SA_pcpcEvn[i]) {
                __spawnArgFree(psarg);
                errno = ENOMEM;
                return  (PX_ERROR);
            }
        } else {
            break;
        }
    }
    psarg->SA_iEvns = i;
    
    psarg->SA_pcPath = lib_strdup(path);
    if (!psarg->SA_pcPath) {
        __spawnArgFree(psarg);
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
    return  (__processStart(mode, psarg));
}
/*********************************************************************************************************
** 函数名称: spawnvp
** 功能描述: spawn family of functions
** 输　入  : mode          run mode
**           file          pathname that identifies the new module image file.
**           argv          args...
** 输　出  : P_WAIT     The exit status of the child process.
**           P_NOWAIT   The process ID of the child process.
**           P_NOWAITO  The process ID of the child process.
** 全局变量:
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int spawnvp (int mode, const char *file, char * const *argv)
{
    __PSPAWN_ARG        psarg;
    INT                 i;
    PCHAR               argvNull[1] = {NULL};
    CHAR                cFilePath[MAX_FILENAME_LENGTH];
    
    if (!file) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    if (!argv) {
        argv = (char * const *)argvNull;
    }
    if (__ldGetFilePath(file, cFilePath, MAX_FILENAME_LENGTH) != ERROR_NONE) {
        errno = ENOENT;
        return  (PX_ERROR);                                             /*  无法找到命令                */
    }
    
    psarg = __spawnArgCreate();
    if (!psarg) {
        errno = ENOMEM;
        return  (PX_ERROR);
    }

    for (i = 0; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {
        if (argv[i]) {
            psarg->SA_pcParamList[i] = lib_strdup(argv[i]);
            if (!psarg->SA_pcParamList[i]) {
                __spawnArgFree(psarg);
                errno = ENOMEM;
                return  (PX_ERROR);
            }
        } else {
            break;
        }
    }
    psarg->SA_iArgs = i;
    
    psarg->SA_pcPath = lib_strdup(cFilePath);
    if (!psarg->SA_pcPath) {
        __spawnArgFree(psarg);
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
    return  (__processStart(mode, psarg));
}
/*********************************************************************************************************
** 函数名称: spawnvpe
** 功能描述: spawn family of functions
** 输　入  : mode          run mode
**           file          pathname that identifies the new module image file.
**           argv          args...
**           envp[]        evar array need to set.
** 输　出  : P_WAIT     The exit status of the child process.
**           P_NOWAIT   The process ID of the child process.
**           P_NOWAITO  The process ID of the child process.
** 全局变量:
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int spawnvpe (int mode, const char *file, char * const *argv, char * const *envp)
{
    __PSPAWN_ARG        psarg;
    INT                 i;
    PCHAR               argvNull[1] = {NULL};
    CHAR                cFilePath[MAX_FILENAME_LENGTH];
    
    if (!file) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    if (!argv) {
        argv = (char * const *)argvNull;
    }
    if (!envp) {
        envp = (char * const *)argvNull;
    }
    if (__ldGetFilePath(file, cFilePath, MAX_FILENAME_LENGTH) != ERROR_NONE) {
        errno = ENOENT;
        return  (PX_ERROR);                                             /*  无法找到命令                */
    }
    
    psarg = __spawnArgCreate();
    if (!psarg) {
        errno = ENOMEM;
        return  (PX_ERROR);
    }

    for (i = 0; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {
        if (argv[i]) {
            psarg->SA_pcParamList[i] = lib_strdup(argv[i]);
            if (!psarg->SA_pcParamList[i]) {
                __spawnArgFree(psarg);
                errno = ENOMEM;
                return  (PX_ERROR);
            }
        } else {
            break;
        }
    }
    psarg->SA_iArgs = i;
    
    for (i = 0; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {
        if (envp[i]) {
            psarg->SA_pcpcEvn[i] = lib_strdup(envp[i]);
            if (!psarg->SA_pcpcEvn[i]) {
                __spawnArgFree(psarg);
                errno = ENOMEM;
                return  (PX_ERROR);
            }
        } else {
            break;
        }
    }
    psarg->SA_iEvns = i;
    
    psarg->SA_pcPath = lib_strdup(cFilePath);
    if (!psarg->SA_pcPath) {
        __spawnArgFree(psarg);
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
    return  (__processStart(mode, psarg));
}
/*********************************************************************************************************
** 函数名称: execl
** 功能描述: exec family of functions
** 输　入  : path          pathname that identifies the new module image file.
**           arg0          args...
** 输　出  : module return
** 全局变量:
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int execl (const char *path, const char *arg0, ...)
{
    INT         i;
    va_list     valist;
    CPCHAR      pcParamList[LW_CFG_SHELL_MAX_PARAMNUM];                 /*  参数列表                    */
    
    if (!path) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
#if __STDC__
    va_start(valist, arg0);
#else
    va_start(valist);
#endif

    pcParamList[0] = arg0;
    for (i = 1; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {                   /*  循环读取参数                */
        pcParamList[i] = (CPCHAR)va_arg(valist, const char *);
        if (pcParamList[i] == LW_NULL) {
            break;
        }
    }
    
    va_end(valist);

    return  (spawnv(P_OVERLAY, path, (char * const *)pcParamList));     /*  替换当前环境运行进程        */
}
/*********************************************************************************************************
** 函数名称: execl
** 功能描述: exec family of functions
** 输　入  : path          pathname that identifies the new module image file.
**           arg0          args...
**           ...
**           envp[]        evar array need to set.
** 输　出  : module return
** 全局变量:
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int execle (const char *path, const char *arg0, ...)
{
    INT            i;
    va_list        valist;
    CPCHAR         pcParamList[LW_CFG_SHELL_MAX_PARAMNUM];              /*  参数列表                    */
    char * const  *envp;
    
    if (!path) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
#if __STDC__
    va_start(valist, arg0);
#else
    va_start(valist);
#endif

    pcParamList[0] = arg0;
    for (i = 1; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {                   /*  循环读取参数                */
        pcParamList[i] = (CPCHAR)va_arg(valist, const char *);
        if (pcParamList[i] == LW_NULL) {
            break;
        }
    }
    
    envp = (char * const *)va_arg(valist, char * const *);              /*  获取环境变量数组            */
    
    va_end(valist);
                                                                        /*  替换当前环境运行进程        */
    return  (spawnve(P_OVERLAY, path, (char * const *)pcParamList, envp));
}
/*********************************************************************************************************
** 函数名称: execlp
** 功能描述: exec family of functions
** 输　入  : file          execlp() 会从 PATH 环境变量所指的目录中查找符合参数file的文件名.
**           arg0          args...
** 输　出  : module return
** 全局变量:
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int execlp (const char *file, const char *arg0, ...)
{
    INT         i;
    va_list     valist;
    CPCHAR      pcParamList[LW_CFG_SHELL_MAX_PARAMNUM];                 /*  参数列表                    */
    
    if (!file) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
#if __STDC__
    va_start(valist, arg0);
#else
    va_start(valist);
#endif

    pcParamList[0] = arg0;
    for (i = 1; i < LW_CFG_SHELL_MAX_PARAMNUM; i++) {                   /*  循环读取参数                */
        pcParamList[i] = (CPCHAR)va_arg(valist, const char *);
        if (pcParamList[i] == LW_NULL) {
            break;
        }
    }
    
    va_end(valist);
    
    return  (spawnvp(P_OVERLAY, file, (char * const *)pcParamList));    /*  替换当前环境运行进程        */
}
/*********************************************************************************************************
** 函数名称: execv
** 功能描述: exec family of functions
** 输　入  : path          pathname that identifies the new module image file.
**           argv          arglist
** 输　出  : module return
** 全局变量:
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int execv (const char *path, char * const *argv)
{
    if (!path) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (spawnv(P_OVERLAY, path, argv));                            /*  替换当前环境运行进程        */
}
/*********************************************************************************************************
** 函数名称: execv
** 功能描述: exec family of functions
** 输　入  : path          pathname that identifies the new module image file.
**           argv          arglist
**           envp[]        evar array need to set.
** 输　出  : module return
** 全局变量:
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int execve (const char *path, char *const argv[], char *const envp[])
{
    if (!path) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (spawnve(P_OVERLAY, path, argv, envp));                     /*  替换当前环境运行进程        */
}
/*********************************************************************************************************
** 函数名称: execvp
** 功能描述: exec family of functions
** 输　入  : file          execlp() 会从 PATH 环境变量所指的目录中查找符合参数file的文件名.
**           argv          arglist
** 输　出  : module return
** 全局变量:
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int execvp (const char *file, char *const argv[])
{
    if (!file) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (spawnvp(P_OVERLAY, file, argv));                           /*  替换当前环境运行进程        */
}
/*********************************************************************************************************
** 函数名称: execvpe
** 功能描述: exec family of functions
** 输　入  : file          execlp() 会从 PATH 环境变量所指的目录中查找符合参数file的文件名.
**           argv          arglist
**           envp          环境变量
** 输　出  : module return
** 全局变量:
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int execvpe (const char *file, char * const *argv, char * const *envp)
{
    if (!file) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (spawnvpe(P_OVERLAY, file, argv, envp));                    /*  替换当前环境运行进程        */
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

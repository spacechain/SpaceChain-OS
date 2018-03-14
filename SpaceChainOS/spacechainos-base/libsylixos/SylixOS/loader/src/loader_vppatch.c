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
** 文   件   名: loader_vppatch.c
**
** 创   建   人: Han.hui (韩辉)
**
** 文件创建日期: 2011 年 08 月 03 日
**
** 描        述: 模块对 VPROCESS 进程模型的补丁支持.

** BUG:
2011.11.02  加入对进程内存堆信息获取的接口.
            修正之前的一些拼写错误.
2011.12.22  加入进程表.
2012.03.08  初始化进程补丁时, 将 EMOD_pcModulePath 参数传入, 这样可以使内存泄露工具检查进程更加准确.
2012.04.02  加入 vprocSetsid() 如果文件包拥有 SETUID SETGID 位, 这里需要设置保存 ID 为文件所有者 ID. 
2012.05.10  加入通过地址反差符号的 api.
2012.08.24  加入 vprocRun() 以便支持 pid 与 wait 相关操作.
2012.09.21  vprocSetsid() 需要保存 setugid 标志位.
2012.10.24  加入 vprocTickHook() 实现 posix times.h 相关功能.
            用户接口为 API_ModuleTimes() 
2012.10.25  每个进程拥有自己独立的 IO 环境 (相对路径).
2012.12.07  修正进程独立的运行环境.
2012.12.09  加入进程树功能.
2012.12.14  atexit 安装的函数必须在进程上下文中被执行.
2013.01.09  vprocTickHook 加入对所有子进程消耗时间的统计.
2013.01.12  vprocGetEntry 需要获取 _start 与主模块的 main 符号.
2013.01.15  从 1.0.0.rc37 开始需要进程补丁版本为 1.1.3.
            vp 中加入 VP_bRunAtExit.
2013.03.16  在进程控制块创建和删除的时候调用相应的系统回调.
            更新 pid 生成算法.
2013.04.01  修正 GCC 4.7.3 引发的新 warning.
2013.05.02  通过 vprocFindProc 来查找进程.
2013.06.07  加入 vprocDetach 函数用来与父进程脱离关系.
2013.06.11  vprocRun 在执行进程文件之前, 必须回收 FD_CLOEXEC 属性的文件.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
2013.09.21  exec 在当前进程上下文中运行新的文件不再切换主线程.
2014.05.13  支持进程启动后等待调试器信号后运行.
2014.05.17  加入 GDB 调试所需的一些获取信息的函数.
2014.05.20  用更加快捷的方法判断进程是否允许退出.
2014.05.21  notify parent 将信号同时发送给调试器.
2014.05.31  加入退出模式的选择.
2014.07.03  加入获取进程虚拟空间组的函数.
2014.09.29  API_ModuleGetBase() 加入长度参数.
2015.03.02  修正 vprocNotifyParent() 获取父系进程主线程错误.
2015.08.17  vprocDetach() 需要通知父进程.
2015.11.25  加入进程内线程独立堆栈管理体系.
2017.05.27  避免 vprocTickHook() 处理一个进程拥有多个线程, 且同时在多个 CPU 上执行时间重复计算的问题.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "sys/wait.h"
#include "sys/vproc.h"
#if LW_CFG_GDB_EN > 0
#include "dtrace.h"
#endif
#if LW_CFG_POSIX_EN > 0
#include "dlfcn.h"
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_lib.h"
#include "../include/loader_symbol.h"
#include "../include/loader_error.h"
#include "../elf/elf_loader.h"
/*********************************************************************************************************
  模块全局变量
*********************************************************************************************************/
LW_LIST_LINE_HEADER      _G_plineVProcHeader = LW_NULL;
LW_OBJECT_HANDLE         _G_ulVProcMutex     = LW_OBJECT_HANDLE_INVALID;
/*********************************************************************************************************
  内核进程控制卡
*********************************************************************************************************/
LW_LD_VPROC              _G_vprocKernel;
LW_LD_VPROC             *_G_pvprocTable[LW_CFG_MAX_THREADS];
/*********************************************************************************************************
  宏
*********************************************************************************************************/
#define __LW_VP_PATCH_VERSION        "__vp_patch_version"
#define __LW_VP_PATCH_HEAP           "__vp_patch_heap"
#define __LW_VP_PATCH_VMEM           "__vp_patch_vmem"
#define __LW_VP_PATCH_CTOR           "__vp_patch_ctor"
#define __LW_VP_PATCH_DTOR           "__vp_patch_dtor"
#define __LW_VP_PATCH_AERUN          "__vp_patch_aerun"
/*********************************************************************************************************
  proc 文件系统操作
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0
INT  vprocProcAdd(LW_LD_VPROC *pvproc);
INT  vprocProcDelete(LW_LD_VPROC *pvproc);
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
/*********************************************************************************************************
** 函数名称: __moduleVpPatchHeap
** 功能描述: vp 补丁初始化
** 输　入  : pmodule       进程主模块句柄
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PCHAR __moduleVpPatchVersion (LW_LD_EXEC_MODULE *pmodule)
{
    PCHAR       (*funcVpVersion)();
    PCHAR         pcVersion = LW_NULL;
    
    funcVpVersion = (PCHAR (*)())API_ModuleSym(pmodule, __LW_VP_PATCH_VERSION);
    if (funcVpVersion) {
        LW_SOFUNC_PREPARE(funcVpVersion);
        pcVersion = funcVpVersion(pmodule->EMOD_pvproc);
    }
    
    return  (pcVersion);
}
/*********************************************************************************************************
** 函数名称: __moduleVpPatchHeap
** 功能描述: vp 补丁初始化
** 输　入  : pmodule       进程主模块句柄
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_VMM_EN == 0

PVOID __moduleVpPatchHeap (LW_LD_EXEC_MODULE *pmodule)
{
    PVOIDFUNCPTR    funcVpHeap;
    PVOID           pvHeap = LW_NULL;

    funcVpHeap = (PVOIDFUNCPTR)API_ModuleSym(pmodule, __LW_VP_PATCH_HEAP);
    if (funcVpHeap) {
        LW_SOFUNC_PREPARE(funcVpHeap);
        pvHeap = funcVpHeap(pmodule->EMOD_pvproc);
    }
    
    return  (pvHeap);
}

#endif                                                                  /*  LW_CFG_VMM_EN == 0          */
/*********************************************************************************************************
** 函数名称: __moduleVpPatchVmem
** 功能描述: vp 补丁获得虚拟地址空间
** 输　入  : pmodule       进程主模块句柄
**           ppvArea       内存空间数组
**           iSize         数组空间大小
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __moduleVpPatchVmem (LW_LD_EXEC_MODULE *pmodule, PVOID  ppvArea[], INT  iSize)
{
    FUNCPTR     funcVpVmem;
    INT         iRet = PX_ERROR;

    funcVpVmem = (FUNCPTR)API_ModuleSym(pmodule, __LW_VP_PATCH_VMEM);
    if (funcVpVmem) {
        LW_SOFUNC_PREPARE(funcVpVmem);
        iRet = funcVpVmem(pmodule->EMOD_pvproc, ppvArea, iSize);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __moduleVpPatchInit
** 功能描述: vp 补丁初始化
** 输　入  : pmodule       进程主模块句柄
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID __moduleVpPatchInit (LW_LD_EXEC_MODULE *pmodule)
{
    VOIDFUNCPTR     funcVpCtor;
    
    funcVpCtor = (VOIDFUNCPTR)API_ModuleSym(pmodule, __LW_VP_PATCH_CTOR);
    if (funcVpCtor) {
        LW_SOFUNC_PREPARE(funcVpCtor);
        funcVpCtor(pmodule->EMOD_pvproc, 
                   &pmodule->EMOD_pvproc->VP_pfuncMalloc,
                   &pmodule->EMOD_pvproc->VP_pfuncFree);
    }
}
/*********************************************************************************************************
** 函数名称: __moduleVpPatchFini
** 功能描述: vp 补丁卸载
** 输　入  : pmodule       进程主模块句柄
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID __moduleVpPatchFini (LW_LD_EXEC_MODULE *pmodule)
{
    VOIDFUNCPTR     funcVpDtor;

    funcVpDtor = (VOIDFUNCPTR)API_ModuleSym(pmodule, __LW_VP_PATCH_DTOR);
    if (funcVpDtor) {
        LW_SOFUNC_PREPARE(funcVpDtor);
        funcVpDtor(pmodule->EMOD_pvproc);
    }
}
/*********************************************************************************************************
** 函数名称: __moduleVpPatchAerun
** 功能描述: vp 补丁运行进程 atexit 节点
** 输　入  : pmodule       进程主模块句柄
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __moduleVpPatchAerun (LW_LD_EXEC_MODULE *pmodule)
{
    VOIDFUNCPTR     funcVpAerun;

    funcVpAerun = (VOIDFUNCPTR)API_ModuleSym(pmodule, __LW_VP_PATCH_AERUN);
    if (funcVpAerun) {
        LW_SOFUNC_PREPARE(funcVpAerun);
        funcVpAerun(pmodule->EMOD_pvproc);
    }
}
/*********************************************************************************************************
** 函数名称: vprocThreadExitHook
** 功能描述: 进程内线程退出 hook 函数 (进程内的线程删除后会自动调用这个函数)
** 输　入  : pvVProc     进程控制块
**           ulId        句柄
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID vprocThreadExitHook (PVOID  pvVProc, LW_OBJECT_HANDLE  ulId)
{
    LW_LD_VPROC *pvproc = (LW_LD_VPROC *)pvVProc;

    if (pvproc) {
        API_SemaphoreBPost(pvproc->VP_ulWaitForExit);
    }
}
/*********************************************************************************************************
** 函数名称: vprocSetGroup
** 功能描述: 进程控制块设置组ID 
** 输　入  : pid       进程 ID
**           pgid      进程组 ID
** 输　出  : 是否设置成功
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  vprocSetGroup (pid_t  pid, pid_t  pgid)
{
    LW_LD_VPROC *pvproc;

    if ((pid < 1) || (pid > LW_CFG_MAX_THREADS)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if ((pgid < 1) || (pgid > LW_CFG_MAX_THREADS)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LW_LD_LOCK();
    pvproc = _G_pvprocTable[pid - 1];
    if (pvproc) {
        pvproc->VP_pidGroup = pgid;
    } else {
        LW_LD_UNLOCK();
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    LW_LD_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: vprocSetGroup
** 功能描述: 进程控制块获取组ID
** 输　入  : pid       进程 ID
** 输　出  : 进程组 ID
** 全局变量:
** 调用模块:
*********************************************************************************************************/
pid_t  vprocGetGroup (pid_t  pid)
{
    LW_LD_VPROC *pvproc;
    pid_t        pgid = -1;

    if ((pid < 1) || (pid > LW_CFG_MAX_THREADS)) {
        _ErrorHandle(EINVAL);
        return  (pgid);
    }
    
    LW_LD_LOCK();
    pvproc = _G_pvprocTable[pid - 1];
    if (pvproc) {
        pgid = pvproc->VP_pidGroup;
    } else {
        LW_LD_UNLOCK();
        _ErrorHandle(EINVAL);
        return  (pgid);
    }
    LW_LD_UNLOCK();
    
    return  (pgid);
}
/*********************************************************************************************************
** 函数名称: vprocGetFather
** 功能描述: 获取父进程 ID
** 输　入  : pid       进程 ID
** 输　出  : 父进程 ID
** 全局变量:
** 调用模块:
*********************************************************************************************************/
pid_t  vprocGetFather (pid_t  pid)
{
    LW_LD_VPROC *pvproc;
    LW_LD_VPROC *pvprocFather;
    pid_t        ppid = -1;
    
    if ((pid < 1) || (pid > LW_CFG_MAX_THREADS)) {
        _ErrorHandle(EINVAL);
        return  (ppid);
    }
    
    LW_LD_LOCK();
    pvproc = _G_pvprocTable[pid - 1];
    if (pvproc) {
        pvprocFather = pvproc->VP_pvprocFather;
        if (pvprocFather) {
            ppid = pvprocFather->VP_pid;
        }
    }
    LW_LD_UNLOCK();
    
    if (ppid < 0) {
        _ErrorHandle(EINVAL);
    }
    
    return  (ppid);
}
/*********************************************************************************************************
** 函数名称: vprocDetach
** 功能描述: 脱离与父进程关系
** 输　入  : pid       进程 ID
** 输　出  : 是否成功
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  vprocDetach (pid_t  pid)
{
    LW_LD_VPROC *pvproc;
    LW_LD_VPROC *pvprocFather = LW_NULL;
    
    if ((pid < 1) || (pid > LW_CFG_MAX_THREADS)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    LW_LD_LOCK();
    pvproc = _G_pvprocTable[pid - 1];
    if (pvproc) {
        pvprocFather = pvproc->VP_pvprocFather;
        if (pvprocFather) {
            _List_Line_Del(&pvproc->VP_lineBrother,
                           &pvprocFather->VP_plineChild);               /*  从父系链表中退出            */
            pvproc->VP_pvprocFather = LW_NULL;
        }
        pvproc->VP_ulFeatrues |= __LW_VP_FT_DAEMON;                     /*  进入 deamon 状态            */
    }
    LW_LD_UNLOCK();
    
    if (pvprocFather) {
        pvproc->VP_iExitCode  |= SET_EXITSTATUS(0);
        vprocNotifyParent(pvproc, CLD_EXITED, LW_FALSE);                /*  向父进程发送子进程退出信号  */
        pvproc->VP_iExitCode   = 0;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: vprocCreate
** 功能描述: 创建进程控制块 (创建完成后已经确定的进程树关系)
** 输　入  : pcFile      进程文件名
** 输　出  : 创建好的进程控制块指针，如果失败，输出NULL。
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_LD_VPROC *vprocCreate (CPCHAR  pcFile)
{
    static UINT  uiIndex = 0;
           CHAR  cVarValue[2];
           
           INT   i;
           INT   iExitMode = LW_VPROC_EXIT_NORMAL;                      /*  正常模式退出                */
           INT   iErrLevel = 0;
           
    LW_LD_VPROC *pvproc;
    LW_LD_VPROC *pvprocFather = __LW_VP_GET_CUR_PROC();
    
    if (API_TShellVarGetRt("VPROC_EXIT_FORCE", cVarValue, 2) > 0) {
        if (cVarValue[0] == '1') {
            iExitMode = LW_VPROC_EXIT_FORCE;                            /*  主线程结束自动删除子线程    */
        }
    }
    
    pvproc = (LW_LD_VPROC *)LW_LD_SAFEMALLOC(sizeof(LW_LD_VPROC));
    if (LW_NULL == pvproc) {
        return  (LW_NULL);
    }
    lib_bzero(pvproc, sizeof(LW_LD_VPROC));
    
    pvproc->VP_pioeIoEnv = _IosEnvCreate();                             /*  创建当前进程 IO 环境        */
    if (pvproc->VP_pioeIoEnv == LW_NULL) {
        iErrLevel = 1;
        goto    __error_handle;
    }

    pvproc->VP_pcName = (PCHAR)LW_LD_SAFEMALLOC(lib_strlen(pcFile) + 1);
    if (LW_NULL == pvproc->VP_pcName) {
        iErrLevel = 2;
        goto    __error_handle;
    }
    lib_strcpy(pvproc->VP_pcName, pcFile);

    pvproc->VP_ulModuleMutex = API_SemaphoreMCreate("vproc_lock",
                                                    LW_PRIO_DEF_CEILING,
                                                    LW_OPTION_WAIT_PRIORITY |
                                                    LW_OPTION_INHERIT_PRIORITY | 
                                                    LW_OPTION_DELETE_SAFE |
                                                    LW_OPTION_OBJECT_GLOBAL,
                                                    LW_NULL);
    if (pvproc->VP_ulModuleMutex == LW_OBJECT_HANDLE_INVALID) {         /*  进程锁                      */
        iErrLevel = 3;
        goto    __error_handle;
    }
    
    pvproc->VP_ulWaitForExit = API_SemaphoreBCreate("vproc_wfe",
                                                    LW_FALSE,
                                                    LW_OPTION_OBJECT_GLOBAL,
                                                    LW_NULL);
    if (pvproc->VP_ulWaitForExit == LW_OBJECT_HANDLE_INVALID) {         /*  主线程等待其他线程运行结束  */
        iErrLevel = 4;
        goto    __error_handle;
    }
    
    _IosEnvInherit(pvproc->VP_pioeIoEnv);                               /*  当前进程继承创建者 IO 环境  */
    
    /*
     *  进程树初始化
     */
    LW_LD_LOCK();
    
    for (i = 0; i < LW_CFG_MAX_THREADS; i++) {                          /*  开始分配 pid 号             */
        if (uiIndex >= LW_CFG_MAX_THREADS) {                            /*  pid 号从 1 ~ MAX_THREADS    */
            uiIndex  = 0;
        }
        if (_G_pvprocTable[uiIndex] == LW_NULL) {
            _G_pvprocTable[uiIndex] =  pvproc;
            pvproc->VP_pid = uiIndex + 1;                               /*  start with 1, 0 is kernel   */
            uiIndex++;
            break;
        
        } else {
            uiIndex++;
        }
    }
    
    if (i >= LW_CFG_MAX_THREADS) {                                      /*  分配 ID 失败                */
        LW_LD_UNLOCK();
        iErrLevel = 5;
        goto    __error_handle;
    }
    
    pvproc->VP_bRunAtExit       = LW_TRUE;                              /*  运行 atexit 安装的函数      */
    pvproc->VP_bImmediatelyTerm = LW_FALSE;                             /*  非强制退出                  */
    
    pvproc->VP_iStatus   = __LW_VP_INIT;                                /*  没有装载运行, 不需要发信号  */
    pvproc->VP_iExitCode = 0;
    pvproc->VP_iSigCode  = 0;
    
    pvproc->VP_plineChild   = LW_NULL;                                  /*  没有儿子                    */
    pvproc->VP_pvprocFather = pvprocFather;                             /*  当前进程为父系进程          */
    
    if (pvprocFather) {
        pvproc->VP_pidGroup = pvprocFather->VP_pidGroup;                /*  继承父亲组 ID               */
        _List_Line_Add_Ahead(&pvproc->VP_lineBrother, 
                             &pvprocFather->VP_plineChild);             /*  加入父亲子进程链表          */
    } else {
        pvproc->VP_pidGroup = pvproc->VP_pid;                           /*  与 pid 相同                 */
    }
    
    pvproc->VP_iExitMode = iExitMode;
    pvproc->VP_i64Tick   = -1ull;
    
    _List_Line_Add_Tail(&pvproc->VP_lineManage, 
                        &_G_plineVProcHeader);                          /*  加入进程表                  */
    LW_LD_UNLOCK();

    vprocIoFileInit(pvproc);                                            /*  初始化文件描述符系统        */

#if LW_CFG_PROCFS_EN > 0
    vprocProcAdd(pvproc);
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */

    __LW_VPROC_CREATE_HOOK(pvproc->VP_pid);                             /*  进程创建回调                */

    MONITOR_EVT_INT1(MONITOR_EVENT_ID_VPROC, MONITOR_EVENT_VPROC_CREATE, pvproc->VP_pid, pcFile);

    return  (pvproc);
    
__error_handle:
    if (iErrLevel > 4) {
        API_SemaphoreBDelete(&pvproc->VP_ulWaitForExit);
    }
    if (iErrLevel > 3) {
        API_SemaphoreMDelete(&pvproc->VP_ulModuleMutex);
    }
    if (iErrLevel > 2) {
        LW_LD_SAFEFREE(pvproc->VP_pcName);
    }
    if (iErrLevel > 1) {
        _IosEnvDelete(pvproc->VP_pioeIoEnv);
    }
    if (iErrLevel > 0) {
        LW_LD_SAFEFREE(pvproc);
    }
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: vprocDestroy
** 功能描述: 销毁进程控制块
** 输　入  : pvproc     进程控制块指针
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT vprocDestroy (LW_LD_VPROC *pvproc)
{
    LW_LD_VPROC *pvprocFather;
    
    __LW_VPROC_DELETE_HOOK(pvproc->VP_pid, pvproc->VP_iExitCode);       /*  进程回收回调                */
    
    MONITOR_EVT_INT2(MONITOR_EVENT_ID_VPROC, MONITOR_EVENT_VPROC_DELETE, 
                     pvproc->VP_pid, pvproc->VP_iExitCode, LW_NULL);

#if LW_CFG_PROCFS_EN > 0
    vprocProcDelete(pvproc);
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
    
    vprocIoFileDeinit(pvproc);                                          /*  回收还残余的文件描述符      */
    
    LW_LD_LOCK();
    pvprocFather = pvproc->VP_pvprocFather;
    if (pvprocFather) {
        _List_Line_Del(&pvproc->VP_lineBrother,
                       &pvprocFather->VP_plineChild);                   /*  从父系链表中退出            */
    }
    _List_Line_Del(&pvproc->VP_lineManage, 
                   &_G_plineVProcHeader);                               /*  从进程表中删除              */
    _G_pvprocTable[pvproc->VP_pid - 1] = LW_NULL;
    LW_LD_UNLOCK();

    API_SemaphoreMDelete(&pvproc->VP_ulModuleMutex);
    API_SemaphoreBDelete(&pvproc->VP_ulWaitForExit);

    _IosEnvDelete(pvproc->VP_pioeIoEnv);                                /*  删除当前进程 IO 环境        */
    
    LW_LD_SAFEFREE(pvproc->VP_pcName);
    LW_LD_SAFEFREE(pvproc);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: vprocGet
** 功能描述: 通过 pid 获得进程控制块
** 输　入  : pid       进程号
** 输　出  : 进程控制块指针
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_LD_VPROC *vprocGet (pid_t  pid)
{
    if ((pid < 1) || (pid > LW_CFG_MAX_THREADS)) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    return  (_G_pvprocTable[pid - 1]);
}
/*********************************************************************************************************
** 函数名称: vprocGetCur
** 功能描述: 获得当前进程控制块
** 输　入  : NONE
** 输　出  : 进程控制块指针
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_LD_VPROC *vprocGetCur (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    return  ((LW_LD_VPROC *)ptcbCur->TCB_pvVProcessContext);
}
/*********************************************************************************************************
** 函数名称: vprocSetCur
** 功能描述: 设置当前进程控制块
** 输　入  : pvproc    进程控制块指针
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  vprocSetCur (LW_LD_VPROC  *pvproc)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    ptcbCur->TCB_pvVProcessContext = (PVOID)pvproc;
}
/*********************************************************************************************************
** 函数名称: vprocGetPidByTcb
** 功能描述: 通过 tcb 获得进程 id
** 输　入  : ptcb      任务控制块
** 输　出  : 进程 pid
** 全局变量:
** 调用模块:
*********************************************************************************************************/
pid_t  vprocGetPidByTcb (PLW_CLASS_TCB  ptcb)
{
    pid_t        pid = 0;
    LW_LD_VPROC *pvproc;

    if (ptcb) {
        LW_LD_LOCK();
        pvproc = __LW_VP_GET_TCB_PROC(ptcb);
        if (pvproc) {
            pid = pvproc->VP_pid;
        }
        LW_LD_UNLOCK();
    }
    
    return  (pid);
}
/*********************************************************************************************************
** 函数名称: vprocGetPidByTcbNoLock 
** 功能描述: 通过 tcb 获得进程 id (无锁)
** 输　入  : ptcb      任务控制块
** 输　出  : 进程 pid
** 全局变量:
** 调用模块:
*********************************************************************************************************/
pid_t  vprocGetPidByTcbNoLock (PLW_CLASS_TCB  ptcb)
{
    pid_t        pid = 0;
    LW_LD_VPROC *pvproc;

    if (ptcb) {
        pvproc = __LW_VP_GET_TCB_PROC(ptcb);
        if (pvproc) {
            pid = pvproc->VP_pid;
        }
    }
    
    return  (pid);
}
/*********************************************************************************************************
** 函数名称: vprocGetPidByTcbdesc
** 功能描述: 通过 tcbdesc 获得进程 id
** 输　入  : ptcbdesc   任务信息描述
** 输　出  : 进程 pid
** 全局变量:
** 调用模块:
*********************************************************************************************************/
pid_t  vprocGetPidByTcbdesc (PLW_CLASS_TCB_DESC  ptcbdesc)
{
    pid_t        pid = 0;
    LW_LD_VPROC *pvproc;

    if (ptcbdesc) {
        LW_LD_LOCK();
        pvproc = (LW_LD_VPROC *)ptcbdesc->TCBD_pvVProcessContext;
        if (pvproc) {
            pid = pvproc->VP_pid;
        }
        LW_LD_UNLOCK();
    }
    
    return  (pid);
}
/*********************************************************************************************************
** 函数名称: vprocGetPidByThread
** 功能描述: 通过线程 ID 获得进程 id
** 输　入  : ulId   线程 ID
** 输　出  : 进程 pid
** 全局变量:
** 调用模块:
*********************************************************************************************************/
pid_t  vprocGetPidByThread (LW_OBJECT_HANDLE  ulId)
{
    pid_t           pid = 0;
    LW_LD_VPROC    *pvproc;
    UINT16          usIndex;
    PLW_CLASS_TCB   ptcb;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (pid);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (pid);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (pid);
    }
    
    ptcb   = _K_ptcbTCBIdTable[usIndex];
    pvproc = __LW_VP_GET_TCB_PROC(ptcb);
    if (pvproc) {
        pid = pvproc->VP_pid;
    }
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (pid);
}
/*********************************************************************************************************
** 函数名称: vprocMainThread
** 功能描述: 通过进程号, 查找对应的主线程.
** 输　入  : pid       进程号
** 输　出  : 对应主线程
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_OBJECT_HANDLE vprocMainThread (pid_t pid)
{
    LW_OBJECT_HANDLE  lId = LW_OBJECT_HANDLE_INVALID;
    
    if (pid > 0 && pid <= LW_CFG_MAX_THREADS) {
        LW_LD_LOCK();
        if (_G_pvprocTable[pid - 1]) {
            lId = _G_pvprocTable[pid - 1]->VP_ulMainThread;
        }
        LW_LD_UNLOCK();
    }
    
    return  (lId);
}
/*********************************************************************************************************
** 函数名称: vprocNotifyParent
** 功能描述: 进程通知父系进程
** 输　入  : pvproc         进程控制块指针
**           sigevent       通知信息
**           iSigCode       sigcode
**           bUpDateStat    是否更新进程状态
** 输　出  : 是否成功
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  vprocNotifyParent (LW_LD_VPROC *pvproc, INT  iSigCode, BOOL  bUpDateStat)
{
    siginfo_t           siginfoChld;
    sigevent_t          sigeventChld;
    LW_OBJECT_HANDLE    ulFatherMainThread;
    
    if (!pvproc) {
        return  (PX_ERROR);
    }
    
    if (pvproc->VP_pvprocFather) {
        ulFatherMainThread = pvproc->VP_pvprocFather->VP_ulMainThread;  /*  获得父系进程主线程          */
    }

    LW_LD_LOCK();
    pvproc->VP_iSigCode = iSigCode;
    
    if (bUpDateStat) {
        if ((iSigCode == CLD_EXITED) || 
            (iSigCode == CLD_KILLED) || 
            (iSigCode == CLD_DUMPED)) {                                 /*  当前进程退出, 通知父亲      */
            pvproc->VP_iStatus = __LW_VP_EXIT;
        
        } else if (iSigCode == CLD_CONTINUED) {                         /*  当前进程被恢复执行          */
            pvproc->VP_iStatus = __LW_VP_RUN;
        
        } else {
            pvproc->VP_iStatus = __LW_VP_STOP;                          /*  当前进程暂停                */
        }
    }
    
    siginfoChld.si_signo = SIGCHLD;
    siginfoChld.si_errno = ERROR_NONE;
    siginfoChld.si_code  = iSigCode;
    siginfoChld.si_pid   = pvproc->VP_pid;
    siginfoChld.si_uid   = getuid();
    
    if (iSigCode == CLD_EXITED) {
        siginfoChld.si_status = pvproc->VP_iExitCode;
        siginfoChld.si_utime  = pvproc->VP_clockUser;
        siginfoChld.si_stime  = pvproc->VP_clockSystem;
    }

    sigeventChld.sigev_signo           = SIGCHLD;
    sigeventChld.sigev_value.sival_int = pvproc->VP_pid;
    sigeventChld.sigev_notify          = SIGEV_SIGNAL;
    
#if LW_CFG_GDB_EN > 0
    if (bUpDateStat) {
        API_DtraceChildSig(pvproc->VP_pid, &sigeventChld, &siginfoChld);/*  发送给调试器                */
    }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
    
    if (pvproc->VP_pvprocFather) {
        _doSigEventEx(ulFatherMainThread, &sigeventChld, &siginfoChld); /*  产生 SIGCHLD 信号           */
        LW_LD_UNLOCK();
        return  (ERROR_NONE);
    
    } else {
        LW_LD_UNLOCK();
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: vprocReclaim
** 功能描述: 进程资源回收函数. 资源回收器调用此函数回收进程资源
** 输　入  : pvproc     进程控制块指针
**           bFreeVproc 是否释放进程控制块
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  vprocReclaim (LW_LD_VPROC *pvproc, BOOL  bFreeVproc)
{
    if (!pvproc) {
        return;
    }
    
    if (bFreeVproc) {                                                   /*  exit() or _exit()           */
        API_SemaphoreBPend(pvproc->VP_ulWaitForExit, 
                           LW_OPTION_WAIT_INFINITE);                    /*  等待进程所有线程被彻底删除  */
        
        __resPidReclaim(pvproc->VP_pid);                                /*  回收指定进程资源            */
        
        API_ModuleTerminal((PVOID)pvproc);                              /*  回收装载的可执行文件空间    */
        
        vprocDestroy(pvproc);                                           /*  删除进程控制块              */
        
    } else {                                                            /*  exec...()                   */
        __resPidReclaimOnlyRaw(pvproc->VP_pid);                         /*  仅回收原始资源              */
        
        API_ModuleTerminal((PVOID)pvproc);                              /*  回收装载的可执行文件空间    */
    }
}
/*********************************************************************************************************
** 函数名称: vprocCanExit
** 功能描述: 进程是否能够退出
** 输　入  : NONE
** 输　出  : LW_TRUE 允许 LW_FALSE 不允许
** 全局变量:
** 调用模块: 
*********************************************************************************************************/
static BOOL vprocCanExit (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (_list_line_get_next(&ptcbCur->TCB_lineProcess)) {
        return  (LW_FALSE);
    
    } else {
        return  (LW_TRUE);
    }
}
/*********************************************************************************************************
** 函数名称: vprocAtExit
** 功能描述: 进程自行退出时, 会调用此函数运行 atexit 函数以及整个进程的析构函数.
** 输　入  : pvproc     进程控制块指针
** 输　出  : 是否执行了 atexit
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL vprocAtExit (LW_LD_VPROC *pvproc)
{
    if (pvproc->VP_ringModules) {
        if (pvproc->VP_bRunAtExit) {                                    /*  是否需要运行 atexit 安装函数*/
            LW_LD_EXEC_MODULE *pmodule = _LIST_ENTRY(pvproc->VP_ringModules, 
                                                     LW_LD_EXEC_MODULE, 
                                                     EMOD_ringModules);
            __moduleVpPatchAerun(pmodule);                              /*  运行 atexit 安装的函数      */
        }
        
        API_ModuleFinish((PVOID)pvproc);                                /*  进程结束                    */
        return  (LW_TRUE);
    
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** 函数名称: vprocExit
** 功能描述: 进程自行退出时, 会调用此函数, (如果允许退出, 则会请求资源管理器回收进程资源)
** 输　入  : pvproc     进程控制块指针
**           ulId       进程内调用 exit 线程句柄.
**           iCode      exit() 参数
** 输　出  : NONE
** 全局变量:
** 调用模块: 
** 注  意  : 这里需要提前释放主线程 cleanup, 因为卸载模可执行文件后, 用户安装的 cleanup 将不能正确执行.
*********************************************************************************************************/
VOID  vprocExit (LW_LD_VPROC *pvproc, LW_OBJECT_HANDLE  ulId, INT  iCode)
{
    BOOL            bIsRunAtExit = LW_FALSE;
    PLW_LIST_LINE   plineList;
    INT             iError;
    
#if LW_CFG_THREAD_EXT_EN > 0
    PLW_CLASS_TCB   ptcbCur;
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */

    if (!pvproc) {                                                      /*  不是进程直接退出            */
        return;
    }
    
    pvproc->VP_iExitCode |= SET_EXITSTATUS(iCode);                      /*  保存结束代码                */
    
    if (pvproc->VP_ulMainThread != ulId) {                              /*  不是主线程                  */
        API_ThreadDelete(&ulId, (PVOID)iCode);
        return;
    }
    
#if LW_CFG_THREAD_EXT_EN > 0
    LW_TCB_GET_CUR_SAFE(ptcbCur);
__recheck:
    _TCBCleanupPopExt(ptcbCur);                                         /*  提前执行 cleanup pop 操作   */
#else
__recheck:
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
    
    if (pvproc->VP_iExitMode == LW_VPROC_EXIT_FORCE) {                  /*  强制退出删除除主线程外的线程*/
        vprocThreadKill(pvproc);
    }
    
    do {                                                                /*  等待所有的线程安全退出      */
        if (vprocCanExit() == LW_FALSE) {                               /*  进程是否可以退出            */
            API_SemaphoreBPend(pvproc->VP_ulWaitForExit, LW_OPTION_WAIT_INFINITE);
        } else {
            break;                                                      /*  只有这一个线程了            */
        }
    } while (1);
    
    if (bIsRunAtExit == LW_FALSE) {                                     /*  需要运行 atexit 安装函数    */
        bIsRunAtExit = vprocAtExit(pvproc);
        if (bIsRunAtExit) {                                             /*  已经运行了 atexit           */
            goto    __recheck;
        }
    }
    
    API_SemaphoreBClear(pvproc->VP_ulWaitForExit);                      /*  清空信号量                  */
    
    LW_LD_LOCK();
    for (plineList  = pvproc->VP_plineChild;
         plineList != LW_NULL;
         plineList  = _list_line_get_next(plineList)) {                 /*  遍历子进程                  */
        LW_LD_VPROC *pvprocChild = _LIST_ENTRY(plineList, LW_LD_VPROC, VP_lineBrother);
        
        pvprocChild->VP_pvprocFather = LW_NULL;                         /*  子进程设为孤儿进程          */
        
        if (pvprocChild->VP_iStatus == __LW_VP_EXIT) {                  /*  子进程为僵尸进程            */
            __resReclaimReq((PVOID)pvprocChild);                        /*  请求释放进程资源            */
        }
    }
    LW_LD_UNLOCK();
    
    iError = vprocNotifyParent(pvproc, CLD_EXITED, LW_TRUE);            /*  通知父亲, 子进程退出        */
    if (iError < 0) {                                                   /*  此进程为孤儿进程            */
        __resReclaimReq((PVOID)pvproc);                                 /*  请求释放进程资源            */
    }

    API_ThreadForceDelete(&ulId, (PVOID)pvproc->VP_iExitCode);          /*  这个线程彻底删除时, 才会回收*/
}
/*********************************************************************************************************
** 函数名称: vprocExitNotDestroy
** 功能描述: 进程自行退出但是不会删除进程控制块, exec 系列函数用此来加载新的可执行文件
** 输　入  : pvproc     进程控制块指针
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 这里需要提前释放主线程 cleanup, 因为卸载模可执行文件后, 用户安装的 cleanup 将不能正确执行.
*********************************************************************************************************/
VOID  vprocExitNotDestroy (LW_LD_VPROC *pvproc)
{
    BOOL              bIsRunAtExit = LW_FALSE;
    LW_OBJECT_HANDLE  ulId = API_ThreadIdSelf();
    
#if LW_CFG_THREAD_EXT_EN > 0
    PLW_CLASS_TCB   ptcbCur;
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */

    if (!pvproc) {
        return;
    }
    
    if (pvproc->VP_ulMainThread != ulId) {                              /*  不是主线程                  */
        _ErrorHandle(ENOTSUP);
        return;
    }
    
    pvproc->VP_iExitCode = 0;
    
#if LW_CFG_THREAD_EXT_EN > 0
    LW_TCB_GET_CUR_SAFE(ptcbCur);
__recheck:
    _TCBCleanupPopExt(ptcbCur);                                         /*  提前执行 cleanup pop 操作   */
#else
__recheck:
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */

    do {                                                                /*  等待所有的线程安全退出      */
        if (vprocCanExit() == LW_FALSE) {                               /*  进程是否可以退出            */
            API_SemaphoreBPend(pvproc->VP_ulWaitForExit, LW_OPTION_WAIT_INFINITE);
        } else {
            break;                                                      /*  只有这一个线程了            */
        }
    } while (1);
    
    if (bIsRunAtExit == LW_FALSE) {                                     /*  需要运行 atexit 安装函数    */
        bIsRunAtExit = vprocAtExit(pvproc);
        if (bIsRunAtExit) {                                             /*  已经运行了 atexit           */
            goto    __recheck;
        }
    }
    
    API_SemaphoreBClear(pvproc->VP_ulWaitForExit);                      /*  清空信号量                  */
}
/*********************************************************************************************************
** 函数名称: vprocExitModeGet
** 功能描述: 获取进程退出模式
** 输　入  : pid       进程 id
**           piMode    退出模式
** 输　出  : ERROR
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  vprocExitModeGet (pid_t  pid, INT  *piMode)
{
    LW_LD_VPROC *pvproc;
    
    if (!piMode) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (pvproc == LW_NULL) {
        LW_LD_UNLOCK();
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    
    *piMode = pvproc->VP_iExitMode;
    LW_LD_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: vprocExitModeSet
** 功能描述: 设置进程退出模式
** 输　入  : pid       进程 id
**           iMode     退出模式
** 输　出  : ERROR
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  vprocExitModeSet (pid_t  pid, INT  iMode)
{
    LW_LD_VPROC *pvproc;
    
    if ((iMode != LW_VPROC_EXIT_NORMAL) &&
        (iMode != LW_VPROC_EXIT_FORCE)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (pvproc == LW_NULL) {
        LW_LD_UNLOCK();
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    
    pvproc->VP_iExitMode = iMode;
    LW_LD_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: vprocSetImmediatelyTerm
** 功能描述: 将进程设置为强制立即关闭模式
** 输　入  : pvproc      进程控制块
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  vprocSetImmediatelyTerm (pid_t  pid)
{
    LW_LD_VPROC *pvproc;
    
    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (pvproc == LW_NULL) {
        LW_LD_UNLOCK();
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    
    pvproc->VP_bImmediatelyTerm = LW_TRUE;
    LW_LD_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: vprocSetFilesid
** 功能描述: 设置保存用户 uid gid (setuid 和 setgid 位)
** 输　入  : pvproc      进程控制块
**           pcFile      进程文件名
** 输　出  : 
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  vprocSetFilesid (LW_LD_VPROC *pvproc, CPCHAR  pcFile)
{
    struct stat     statFile;
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (stat(pcFile, &statFile) == ERROR_NONE) {
        if (statFile.st_mode & S_ISUID) {
            if (ptcbCur->TCB_euid != statFile.st_uid) {
                pvproc->VP_bIssetugid = LW_TRUE;
            }
            ptcbCur->TCB_euid = statFile.st_uid;                        /*  设置 euid 等于文件所有者 uid*/
            ptcbCur->TCB_suid = statFile.st_uid;                        /*  保存 euid                   */
        }
        if (statFile.st_mode & S_ISGID) {
            if (ptcbCur->TCB_egid != statFile.st_gid) {
                pvproc->VP_bIssetugid = LW_TRUE;
            }
            ptcbCur->TCB_egid = statFile.st_gid;                        /*  设置 egid 等于文件所有者 gid*/
            ptcbCur->TCB_sgid = statFile.st_gid;                        /*  保存 egid                   */
        }
    }
}
/*********************************************************************************************************
** 函数名称: vprocPatchVerCheck
** 功能描述: 检查进程补丁的版本, 如果没有则不允许运行 (补丁至少要是 2.0.0 版本)
** 输　入  : pvproc      进程控制块
** 输　出  : 
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  vprocPatchVerCheck (LW_LD_VPROC *pvproc)
{
    LW_LD_EXEC_MODULE *pmodule = LW_NULL;
    PCHAR              pcVersion;
    ULONG              ulMajor = 0, ulMinor = 0, ulRevision = 0;
    
    ULONG              ulLowVpVer = __SYLIXOS_MAKEVER(2, 0, 0);         /*  最低进程补丁版本要求        */
    ULONG              ulCurrent;
    
#if defined(LW_CFG_CPU_ARCH_C6X)
    LW_LD_EXEC_MODULE *pmodTemp  = LW_NULL;
    LW_LIST_RING      *pringTemp = pvproc->VP_ringModules;
    PCHAR              pcModuleName;
    BOOL               bStart;

    LW_VP_LOCK(pvproc);
    /*
     *  手动加载的libvpmpdm.so文件模块不在应用程序elf的依赖树中，
     *  需要先查找到libvpmpdm.so模块再在模块中查找符号
     */
    for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
         pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
         pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {

        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

        _PathLastName(pmodTemp->EMOD_pcModulePath, &pcModuleName);
        if (lib_strcmp(pcModuleName, "libvpmpdm.so") == 0) {
            pmodule = pmodTemp;
        	break;
        }
    }
    LW_VP_UNLOCK(pvproc);
#else
    pmodule = _LIST_ENTRY(pvproc->VP_ringModules, LW_LD_EXEC_MODULE, EMOD_ringModules);
#endif

    pcVersion = __moduleVpPatchVersion(pmodule);
    if (pcVersion == LW_NULL) {                                         /*  没有补丁, 不能执行          */
        fprintf(stderr, "[ld]No vprocess patch(libvpmpdm.so) found.\n");
        return  (PX_ERROR);
    
    } else {
        if (sscanf(pcVersion, "%ld.%ld.%ld", &ulMajor, &ulMinor, &ulRevision) != 3) {
            goto    __bad_version;
        }
        
        ulCurrent = __SYLIXOS_MAKEVER(ulMajor, ulMinor, ulRevision);
        if (ulCurrent < ulLowVpVer) {
            goto    __bad_version;                                      /*  补丁版本过低                */
        }
        
        return  (ERROR_NONE);
    }
    
__bad_version:
    fprintf(stderr, "[ld]Bad version of vprocess patch(libvpmpdm.so), "
                    "the minimum version of vprocess patch MUST higher than 2.0.0\n");
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: vprocLibcVerCheck
** 功能描述: 检查进程 C 库的版本, 如果没有则不允许运行.
** 输　入  : pvproc      进程控制块
** 输　出  : 
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  vprocLibcVerCheck (LW_LD_VPROC *pvproc)
{
    (VOID)pvproc;

    return  (ERROR_NONE);                                               /*  未来需要时将加入版本检测    */
}
/*********************************************************************************************************
** 函数名称: vprocGetEntry
** 功能描述: 获得进程入口
** 输　入  : pvproc        进程控制块
** 输　出  : 进程入口 "_start" 符号位置.
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static FUNCPTR  vprocGetEntry (LW_LD_VPROC  *pvproc)
{
    BOOL                bStart;
    LW_LIST_RING       *pringTemp;
    LW_LD_EXEC_MODULE  *pmodTemp;
    
    for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
         pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
         pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {
        
        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
        
        if (bStart) {
            if (__moduleFindSym(pmodTemp, LW_LD_PROCESS_ENTRY, 
                                (addr_t *)&pvproc->VP_pfuncProcess,
                                LW_LD_SYM_FUNCTION) != ERROR_NONE) {
                return  (LW_NULL);                                      /*  无法获取 main 符号          */
            }
        }
        if (pmodTemp->EMOD_pfuncEntry && pmodTemp->EMOD_bIsSymbolEntry) {
            return  (pmodTemp->EMOD_pfuncEntry);                        /*  获取 _start 符号            */
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: vprocGetMain
** 功能描述: 获得当前进程 main 入口.
** 输　入  : NONE
** 输　出  : "main" 符号位置.
** 全局变量:
** 调用模块:
*********************************************************************************************************/
FUNCPTR  vprocGetMain (VOID)
{
    LW_LD_VPROC  *pvproc = __LW_VP_GET_CUR_PROC();
    
    if (pvproc) {
        return  (pvproc->VP_pfuncProcess);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: vprocRun
** 功能描述: 加载并执行elf文件.
** 输　入  : pvproc           进程控制块
**           pvpstop          是否等待调试器继续执行信号才能执行
**           pcFile           文件路径
**           pcEntry          入口函数名，如果为LW_NULL，表示不需要条用初始化函数
**           piRet            进程返回值
**           iArgC            程序参数个数
**           ppcArgV          程序参数数组
**           ppcEnv           环境变量数组
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  vprocRun (LW_LD_VPROC      *pvproc, 
               LW_LD_VPROC_STOP *pvpstop,
               CPCHAR            pcFile, 
               CPCHAR            pcEntry, 
               INT              *piRet,
               INT               iArgC, 
               CPCHAR            ppcArgV[],
               CPCHAR            ppcEnv[])
{
    INT                iErrLevel;
    LW_LD_EXEC_MODULE *pmodule = LW_NULL;
    INT                iError  = ERROR_NONE;
    FUNCPTR            pfunEntry;
    union sigval       sigvalue;

    _ThreadMakeMain(API_ThreadIdSelf(), (PVOID)pvproc);                 /*  当前线程即为主线程          */
                                                                        /*  此线程不再是全局线程        */
                                                                        /*  而且为此线程重新确定 pid    */
    if (LW_NULL == pcFile || LW_NULL == pcEntry) {
        _ErrorHandle(ERROR_LOADER_PARAM_NULL);
        iErrLevel = 0;
        goto    __error_handle;
    }
    
    vprocIoReclaim(pvproc->VP_pid, LW_TRUE);                            /*  回收进程 FD_CLOEXEC 的文件  */

    pmodule = (LW_LD_EXEC_MODULE *)API_ModuleLoadEx(pcFile, LW_OPTION_LOADER_SYM_GLOBAL, 
                                                    LW_NULL, LW_NULL, 
                                                    pcEntry, LW_NULL, 
                                                    pvproc);
    if (LW_NULL == pmodule) {
        iErrLevel = 0;
        goto    __error_handle;
    }

    pfunEntry = vprocGetEntry(pvproc);                                  /*  进程寻找入口                */

#if defined(LW_CFG_CPU_ARCH_C6X)
    /*
     *  当应用程序没有使用libvpmpdm.so中的函数时，C6X的ccs和gcc编译器会忽略-lvpmpdm链接选项，导致加载器
     *  找不到入口函数"_start"，因此需要手动加载libvpmpdm.so
     */
    if (LW_NULL == pfunEntry) {
		API_ModuleLoadEx("libvpmpdm.so", LW_OPTION_LOADER_SYM_GLOBAL,
						 LW_NULL, LW_NULL,
						 pcEntry, LW_NULL,
						 pvproc);
		pfunEntry = vprocGetEntry(pvproc);
    }
#endif

    if (LW_NULL == pfunEntry) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not find entry function.\r\n");
        _ErrorHandle(ERROR_LOADER_NO_ENTRY);
        iErrLevel = 1;
        goto    __error_handle;
    }
    
    if ((vprocPatchVerCheck(pvproc) < ERROR_NONE) ||
        (vprocLibcVerCheck(pvproc) < ERROR_NONE)) {                     /*  patch & libc 必须符合规定   */
        _ErrorHandle(ERROR_LOADER_VERSION);
        iErrLevel = 1;
        goto    __error_handle;
    }

    vprocSetFilesid(pvproc, pmodule->EMOD_pcModulePath);                /*  如果允许, 设置 save uid gid */
    
    pvproc->VP_iStatus = __LW_VP_RUN;                                   /*  开始执行                    */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_VPROC, MONITOR_EVENT_VPROC_RUN, 
                      pvproc->VP_pid, pvproc->VP_ulMainThread, pcFile);
                      
    if (pvpstop) {
        sigvalue.sival_int = ERROR_NONE;
        sigqueue(pvpstop->VPS_ulId, pvpstop->VPS_iSigNo, sigvalue);
        API_ThreadStop(pvproc->VP_ulMainThread);                        /*  等待调试器命令              */
    }
    
    LW_SOFUNC_PREPARE(pfunEntry);
    iError = pfunEntry(iArgC, ppcArgV, ppcEnv);                         /*  执行进程入口函数            */
    if (piRet) {
        *piRet = iError;
    }
    
    return  (ERROR_NONE);                                               /*  需要调用 vp exit            */
    
__error_handle:
    if (pvpstop) {
        sigvalue.sival_int = PX_ERROR;                                  /*  进程运行失败                */
        sigqueue(pvpstop->VPS_ulId, pvpstop->VPS_iSigNo, sigvalue);
    }
    
    if (iErrLevel > 0) {
        API_ModuleFinish((PVOID)pvproc);                                /*  结束进程                    */
        API_ModuleTerminal((PVOID)pvproc);                              /*  卸载进程内所有的模块        */
    }
    
    pvproc->VP_iExitCode |= SET_EXITSTATUS(-1);                         /*  保存结束代码                */
    
    return  (PX_ERROR);                                                 /*  只需调用 vp destroy         */
}
/*********************************************************************************************************
** 函数名称: vprocTickHook
** 功能描述: 进程控制块 tick 回调 (中断上下文中且锁定内核状态被调用)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  vprocTickHook (VOID)
{
    REGISTER INT   i;
    PLW_CLASS_TCB  ptcb;
    PLW_CLASS_CPU  pcpu;
    LW_LD_VPROC   *pvproc;
    LW_LD_VPROC   *pvprocFather;
    
#if LW_CFG_PTIMER_EN > 0
    vprocItimerMainHook();                                              /*  ITIMER_REAL 定时器          */
#endif                                                                  /*  LW_CFG_PTIMER_EN > 0        */
    
#if LW_CFG_SMP_EN > 0
    LW_CPU_FOREACH (i) {                                                /*  遍历所有的核                */
#else
    i = 0;
#endif                                                                  /*  LW_CFG_SMP_EN               */
        pcpu = LW_CPU_GET(i);
        if (LW_CPU_IS_ACTIVE(pcpu)) {                                   /*  CPU 必须被激活              */
            ptcb = pcpu->CPU_ptcbTCBCur;
            pvproc = __LW_VP_GET_TCB_PROC(ptcb);
            if (pvproc && (_K_i64KernelTime != pvproc->VP_i64Tick)) {
                pvproc->VP_i64Tick = _K_i64KernelTime;                  /*  避免时间被重复计算          */
                
#if LW_CFG_PTIMER_EN > 0
                vprocItimerEachHook(pcpu, pvproc);                      /*  ITIMER_VIRTUAL/ITIMER_PROF  */
#endif                                                                  /*  LW_CFG_PTIMER_EN > 0        */
                if (pcpu->CPU_iKernelCounter) {
                    pvproc->VP_clockSystem++;
                } else {
                    pvproc->VP_clockUser++;
                }
                
                pvprocFather = pvproc->VP_pvprocFather;
                if (pvprocFather) {
                    if (pcpu->CPU_iKernelCounter) {
                        pvprocFather->VP_clockCSystem++;
                    } else {
                        pvprocFather->VP_clockCUser++;
                    }
                }
            }
        }
        
#if LW_CFG_SMP_EN > 0
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
}
/*********************************************************************************************************
** 函数名称: vprocIoEnvGet
** 功能描述: 获得当前进程 IO 环境
** 输　入  : ptcb      任务控制块
** 输　出  : IO 环境
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PLW_IO_ENV  vprocIoEnvGet (PLW_CLASS_TCB  ptcb)
{
    LW_LD_VPROC  *pvproc;
    
    if (ptcb && ptcb->TCB_pvVProcessContext) {
        pvproc = (LW_LD_VPROC *)ptcb->TCB_pvVProcessContext;
        if (pvproc) {
            return  (pvproc->VP_pioeIoEnv);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: vprocFindProc
** 功能描述: 通过地址查询所属进程
** 输　入  : pvAddr        通过地址查询模块
** 输　出  : 进程 pid
** 全局变量:
** 调用模块:
*********************************************************************************************************/
pid_t  vprocFindProc (PVOID  pvAddr)
{
    BOOL                bStart;

    LW_LIST_LINE       *plineTemp;
    LW_LIST_RING       *pringTemp;
    LW_LD_VPROC        *pvproc;
    LW_LD_EXEC_MODULE  *pmodTemp;
    pid_t               pid = 0;
    
    LW_LD_LOCK();
    for (plineTemp  = _G_plineVProcHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pvproc = _LIST_ENTRY(plineTemp, LW_LD_VPROC, VP_lineManage);
        
        LW_VP_LOCK(pvproc);
        for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
             pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
             pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {
             
            pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
            
            if ((pvAddr >= pmodTemp->EMOD_pvBaseAddr) &&
                (pvAddr < (PVOID)((PCHAR)pmodTemp->EMOD_pvBaseAddr + pmodTemp->EMOD_stLen))) {
                pid = pvproc->VP_pid;
                break;
            }
        }
        LW_VP_UNLOCK(pvproc);
    
        if (pid) {
            break;
        }
    }
    LW_LD_UNLOCK();

    return  (pid);
}
/*********************************************************************************************************
** 函数名称: vprocGetPath
** 功能描述: 获取进程主程序文件路径
** 输　入  : pid         进程id
**           stMaxLen    pcModPath缓冲区长度
** 输　出  : pcPath      模块路径
**           返回值      ERROR_NONE表示成功, PX_ERROR表示失败.
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  vprocGetPath (pid_t  pid, PCHAR  pcPath, size_t stMaxLen)
{
    LW_LD_VPROC        *pvproc;
    LW_LIST_RING       *pringTemp;
    LW_LD_EXEC_MODULE  *pmodTemp;

    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (pvproc == LW_NULL) {
        LW_LD_UNLOCK();
        return  (PX_ERROR);
    }

    pringTemp  = pvproc->VP_ringModules;
    pmodTemp   = _LIST_ENTRY(pringTemp,
                             LW_LD_EXEC_MODULE,
                             EMOD_ringModules);                         /* 取第一个模块的路径           */
    lib_strlcpy(pcPath, pmodTemp->EMOD_pcModulePath, stMaxLen);

    LW_LD_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: vprocModuleFind
** 功能描述: 查找进程模块
** 输　入  : pid         参数个数
**           pcModPath   模块路径
** 输　出  : 模块结构指针
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_LD_EXEC_MODULE  *vprocModuleFind (pid_t  pid, PCHAR  pcModPath)
{
    BOOL                bStart;

    LW_LIST_RING       *pringTemp;
    LW_LD_VPROC        *pvproc;
    LW_LD_EXEC_MODULE  *pmodTemp;
    PCHAR               pcFileMod;
    PCHAR               pcFileFind;

    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (!pvproc) {
        LW_LD_UNLOCK();
        _ErrorHandle(ESRCH);
        return  (LW_NULL);
    }

    LW_VP_LOCK(pvproc);
    for (pringTemp  = pvproc->VP_ringModules, bStart = LW_TRUE;
         pringTemp && (pringTemp != pvproc->VP_ringModules || bStart);
         pringTemp  = _list_ring_get_next(pringTemp), bStart = LW_FALSE) {

        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

        _PathLastName(pmodTemp->EMOD_pcModulePath, &pcFileMod);
        _PathLastName(pcModPath, &pcFileFind);

        if (lib_strcmp(pcFileMod, pcFileFind) == 0) {
            LW_VP_UNLOCK(pvproc);
            LW_LD_UNLOCK();
            return  (pmodTemp);
        }
    }
    LW_VP_UNLOCK(pvproc);
    LW_LD_UNLOCK();
    
    _ErrorHandle(ERROR_LOADER_NO_MODULE);
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_ModulePid
** 功能描述: 外部进程补丁使用本 API 获取当前进程 pid
** 输　入  : pvproc     进程控制块指针
** 输　出  : 进程描述符
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
pid_t API_ModulePid (PVOID   pvVProc)
{
    LW_LD_VPROC  *pvproc = (LW_LD_VPROC *)pvVProc;

    if (pvproc) {
        return  (pvproc->VP_pid);
        
    } else {
        return  (0);
    }
}
/*********************************************************************************************************
** 函数名称: API_ModuleRun
** 功能描述: 在当前线程上下文中加载并执行elf文件. 创建新的进程, 当前线程变为进程的主线程
             (这里初始化了进程的外部补丁库)
** 输　入  : pcFile        文件路径
**           pcEntry       入口函数名，如果为LW_NULL，表示不需要条用初始化函数
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_ModuleRun (CPCHAR  pcFile, CPCHAR  pcEntry)
{
    return  (API_ModuleRunEx(pcFile, pcEntry, 0, LW_NULL, LW_NULL));
}
/*********************************************************************************************************
** 函数名称: API_ModuleRunEx
** 功能描述: 在当前线程上下文中加载并执行elf文件. 创建新的进程, 当前线程变为进程的主线程
             (这里初始化了进程的外部补丁库)
** 输　入  : pcFile        文件路径
**           pcEntry       入口函数名，如果为LW_NULL，表示不需要条用初始化函数
**           iArgC         程序参数个数
**           ppcArgV       程序参数数组
**           ppcEnv        环境变量数组
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_ModuleRunEx (CPCHAR             pcFile, 
                      CPCHAR             pcEntry, 
                      INT                iArgC, 
                      CPCHAR             ppcArgV[], 
                      CPCHAR             ppcEnv[])
{
    LW_LD_VPROC       *pvproc = LW_NULL;
    INT                iError;
    INT                iRet = PX_ERROR;

    if (LW_NULL == pcFile || LW_NULL == pcEntry) {
        _ErrorHandle(ERROR_LOADER_PARAM_NULL);
        return  (PX_ERROR);
    }

    pvproc = vprocCreate(pcFile);                                       /*  创建进程控制块              */
    if (LW_NULL == pvproc) {
        return  (PX_ERROR);
    }
    
    iError = vprocRun(pvproc, LW_NULL, pcFile, pcEntry, &iRet, 
                      iArgC, ppcArgV, ppcEnv);                          /*  装载并运行进程              */
    if (iError) {                                                       /*  装载失败                    */
        iRet = iError;
    }
    
    vprocExit(pvproc, pvproc->VP_ulMainThread, iRet);                   /*  退出进程                    */

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_ModuleTimes
** 功能描述: 获得进程执行时间信息
** 输　入  : pvVProc       进程控制块
**           pclockUser    用户执行时间
**           pclockSystem  系统执行时间
**           pclockCUser   子进程用户执行时间
**           pclockCSystem 子进程系统执行时间
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_ModuleTimes (PVOID    pvVProc, 
                      clock_t *pclockUser, 
                      clock_t *pclockSystem,
                      clock_t *pclockCUser, 
                      clock_t *pclockCSystem)
{
    LW_LD_VPROC *pvproc = (LW_LD_VPROC *)pvVProc;
    
    if (!pvVProc) {
        _ErrorHandle(ERROR_LOADER_PARAM_NULL);
        return  (PX_ERROR);
    }
    if (pclockUser) {
        *pclockUser = pvproc->VP_clockUser;
    }
    if (pclockSystem) {
        *pclockSystem = pvproc->VP_clockSystem;
    }
    if (pclockCUser) {
        *pclockCUser = pvproc->VP_clockCUser;
    }
    if (pclockCSystem) {
        *pclockCSystem = pvproc->VP_clockCSystem;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  遍历参数
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
typedef struct {
    BOOL                TSBA_bFound;
    PCHAR               TSBA_pcAddr;
    PLW_SYMBOL          TSBA_psymbol;
    LW_LD_EXEC_MODULE  *TSBA_pmod;
    size_t              TSBA_stDistance;
} LW_LD_TSB_ARG;
/*********************************************************************************************************
** 函数名称: vprocTraverseSymCb
** 功能描述: 遍历符号表回调函数
** 输　入  : pvArg         参数
**           psymbol       符号
**           pmod          所属模块
** 输　出  : 是否找到最合适的
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL vprocTraverseSymCb (PVOID pvArg, PLW_SYMBOL psymbol, LW_LD_EXEC_MODULE  *pmod)
{
    size_t          stDistance;
    LW_LD_TSB_ARG  *ptsba = (LW_LD_TSB_ARG *)pvArg;

    if (ptsba->TSBA_pcAddr == psymbol->SYM_pcAddr) {
        ptsba->TSBA_psymbol = psymbol;
        ptsba->TSBA_bFound  = LW_TRUE;
        ptsba->TSBA_pmod    = pmod;
        return  (LW_TRUE);
    }
    
    if (ptsba->TSBA_pcAddr > psymbol->SYM_pcAddr) {
        stDistance = ptsba->TSBA_pcAddr - psymbol->SYM_pcAddr;
        if (ptsba->TSBA_stDistance > stDistance) {
            ptsba->TSBA_stDistance = stDistance;
            ptsba->TSBA_psymbol    = psymbol;
            ptsba->TSBA_pmod       = pmod;
        }
    }
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: vprocTraverseKernelSymCb
** 功能描述: 遍历内核符号表回调函数
** 输　入  : pvArg         参数
**           psymbol       符号
** 输　出  : 是否找到最合适的
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL vprocTraverseKernelSymCb (PVOID pvArg, PLW_SYMBOL psymbol)
{
    size_t          stDistance;
    LW_LD_TSB_ARG  *ptsba = (LW_LD_TSB_ARG *)pvArg;

    if (ptsba->TSBA_pcAddr == psymbol->SYM_pcAddr) {
        ptsba->TSBA_psymbol = psymbol;
        ptsba->TSBA_bFound  = LW_TRUE;
        ptsba->TSBA_pmod    = LW_NULL;
        return  (LW_TRUE);
    }
    
    if (ptsba->TSBA_pcAddr > psymbol->SYM_pcAddr) {
        stDistance = ptsba->TSBA_pcAddr - psymbol->SYM_pcAddr;
        if (ptsba->TSBA_stDistance > stDistance) {
            ptsba->TSBA_stDistance = stDistance;
            ptsba->TSBA_psymbol    = psymbol;
            ptsba->TSBA_pmod       = LW_NULL;
        }
    }
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: API_ModuleAddr
** 功能描述: 查询指定的模块中地址对应的符号
** 输　入  : pvAddr        地址
**           pvDlinfo      Dl_info 结构
**           pvVProc       进程句柄
** 输　出  : 函数地址
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_ModuleAddr (PVOID   pvAddr, 
                     PVOID   pvDlinfo,
                     PVOID   pvVProc)
{
#define __LW_MODULE_MAX_DISTANCE    (8 * 1024)                          /*  默认没有超过 128 kByte 函数 */

    Dl_info     *pdlinfo = (Dl_info *)pvDlinfo;
    LW_LD_VPROC *pvproc  = (LW_LD_VPROC *)pvVProc;
    
    LW_LD_EXEC_MODULE  *pmodTemp;
    PLW_LIST_RING       pringTemp;
    
    LW_LD_TSB_ARG       tsba;
    
    if (!pvAddr || !pdlinfo) {
        _ErrorHandle(ERROR_LOADER_PARAM_NULL);
        return  (PX_ERROR);
    }
    
    tsba.TSBA_bFound     = LW_FALSE;
    tsba.TSBA_pcAddr     = (PCHAR)pvAddr;
    tsba.TSBA_psymbol    = LW_NULL;
    tsba.TSBA_pmod       = LW_NULL;
    tsba.TSBA_stDistance = __LW_MODULE_MAX_DISTANCE;
    
    if (pvproc) {
        LW_VP_LOCK(pvproc);
        pringTemp = pvproc->VP_ringModules;
        if (pringTemp) {
            pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);
            __moduleTraverseSym(pmodTemp, vprocTraverseSymCb, (PVOID)&tsba);
        }
        LW_VP_UNLOCK(pvproc);
    }
    
    if (tsba.TSBA_bFound) {
        pdlinfo->dli_fname = tsba.TSBA_pmod->EMOD_pcModulePath;
        pdlinfo->dli_fbase = (void *)tsba.TSBA_pmod;
        pdlinfo->dli_sname = tsba.TSBA_psymbol->SYM_pcName;
        pdlinfo->dli_saddr = (void *)tsba.TSBA_psymbol->SYM_pcAddr;
        return  (ERROR_NONE);
    }
    
    API_SymbolTraverse(vprocTraverseKernelSymCb, (PVOID)&tsba);         /*  查找内核符号表               */
    
    if (tsba.TSBA_stDistance == __LW_MODULE_MAX_DISTANCE) {
        return  (PX_ERROR);
    }
    
    if (tsba.TSBA_pmod) {
        pdlinfo->dli_fname = tsba.TSBA_pmod->EMOD_pcModulePath;
        pdlinfo->dli_fbase = (void *)tsba.TSBA_pmod;
    } else {
        pdlinfo->dli_fname = "kernel";
        pdlinfo->dli_fbase = LW_NULL;
    }
    
    if (tsba.TSBA_psymbol) {
        pdlinfo->dli_sname = tsba.TSBA_psymbol->SYM_pcName;
        pdlinfo->dli_saddr = (void *)tsba.TSBA_psymbol->SYM_pcAddr;
    } else {
        pdlinfo->dli_sname = LW_NULL;
        pdlinfo->dli_saddr = LW_NULL;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ModuleGetBase
** 功能描述: 查找进程模块
** 输　入  : pid         进程id
**           pcModPath   模块路径
**           pulAddrBase 模块基地址
**           pstLen      模块长度
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_ModuleGetBase (pid_t  pid, PCHAR  pcModPath, addr_t  *pulAddrBase, size_t  *pstLen)
{
    LW_LD_EXEC_MODULE  *pmodule;

    pmodule = vprocModuleFind(pid, pcModPath);
    if (LW_NULL == pmodule) {
        _ErrorHandle(ERROR_LOADER_NO_MODULE);
        return  (PX_ERROR);
    }

    if (pulAddrBase) {
        *pulAddrBase = (addr_t)pmodule->EMOD_pvBaseAddr;
    }
    
    if (pstLen) {
        *pstLen = pmodule->EMOD_stLen;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: vprocGetModsInfo
** 功能描述: 获取进程模块重定位信息，封装成gdb可识别的 xml 格式
** 输　入  : pid         进程id
**           stMaxLen    pcModPath 缓冲区长度
** 输　出  : pcBuff      输出缓冲区
**           返回值      xml 长度
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0

ssize_t  vprocGetModsInfo (pid_t  pid, PCHAR  pcBuff, size_t stMaxLen)
{
    LW_LIST_RING       *pringTemp;
    LW_LD_VPROC        *pvproc;
    LW_LD_EXEC_MODULE  *pmodTemp;
    size_t              stXmlLen;
    
    if (!pcBuff || !stMaxLen) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LW_LD_LOCK();
    pvproc = vprocGet(pid);
    if (pvproc == LW_NULL) {
        LW_LD_UNLOCK();
        return  (PX_ERROR);
    }
    
    stXmlLen = bnprintf(pcBuff, stMaxLen, 0, "<library-list>");

    LW_VP_LOCK(pvproc);
    for (pringTemp  = _list_ring_get_next(pvproc->VP_ringModules);
         pringTemp && (pringTemp != pvproc->VP_ringModules);
         pringTemp  = _list_ring_get_next(pringTemp)) {                 /*  返回除自身之外的所有库      */

        pmodTemp = _LIST_ENTRY(pringTemp, LW_LD_EXEC_MODULE, EMOD_ringModules);

        stXmlLen = bnprintf(pcBuff, stMaxLen, stXmlLen, 
                            "<library name=\"%s\"><segment address=\"0x%llx\"/></library>",
                            pmodTemp->EMOD_pcModulePath,
                            (INT64)(LONG)pmodTemp->EMOD_pvBaseAddr);
    }
    LW_VP_UNLOCK(pvproc);
    LW_LD_UNLOCK();

    stXmlLen = bnprintf(pcBuff, stMaxLen, stXmlLen, "</library-list>");

    return  ((ssize_t)stXmlLen);
}

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

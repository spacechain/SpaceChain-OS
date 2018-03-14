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
** 文   件   名: ttinyShell.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 07 月 27 日
**
** 描        述: 一个超小型的 shell 系统, 使用 tty/pty 做接口, 主要用于调试与简单交互.

** BUG
2008.08.12  在 linux shell 中, 传入的第一个参数为, 命令完整关键字.
2008.10.19  加入转义字符串发送函数.
2009.04.04  今天是清明节, 纪念为国捐躯的英雄们.
            在任务删除时, 需要清除 getopt 相关内存.
2009.05.27  加入 shell 对 ctrl+C 的支持.
2009.05.30  API_TShellCtlCharSend() 需要立即输出.
2009.07.13  加入创建 shell 的扩展方式.
2009.11.23  加入 heap 命令初始化.
2010.01.16  shell 堆栈大小可以设置.
2010.07.28  加入 modem 命令.
2011.06.03  初始化 shell 系统时, 从 /etc/profile 同步环境变量.
2011.07.08  取消 shell 初始化时, load 环境变量. 确保 etc 目录挂载后, 从 bsp 中 load.
2012.12.07  shell 为系统线程.
2012.12.24  shell 支持从进程创建, 创建函数自动将进程内文件描述符 dup 到内核中, 然后再启动 shell.
            API_TShellExec() 如果是在进程中调用, 则必须以同步背景方式运行.
            API_TShellExecBg() 如果在背景中运行则必须将指定的文件 dup 到内核中再运行背景 shell.
2013.01.21  加入用户 CACHE 功能.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
2013.08.26  shell 系统命令添加 format 和 help 第二个参数为 const char *.
2014.07.10  shell 系统加入对新的颜色系统的初始化.
            shell 去除老式色彩控制函数.
2014.11.22  加入选项设置函数.
2016.03.25  修正 API_TShellExecBg() 调用 shell 背景任务创建函数参数错误.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  应用级 API
*********************************************************************************************************/
#include "../SylixOS/api/Lw_Api_Kernel.h"
#include "../SylixOS/api/Lw_Api_System.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#include "ttinyShell.h"
#include "ttinyShellLib.h"
#include "ttinyShellReadline.h"
#include "ttinyShellSysCmd.h"
#include "ttinyShellSysVar.h"
#include "../SylixOS/shell/ttinyVar/ttinyVarLib.h"
#include "../SylixOS/shell/extLib/ttinyShellExtCmd.h"
#include "../SylixOS/shell/fsLib/ttinyShellFsCmd.h"
#include "../SylixOS/shell/tarLib/ttinyShellTarCmd.h"
#include "../SylixOS/shell/modemLib/ttinyShellModemCmd.h"
#include "../SylixOS/shell/heapLib/ttinyShellHeapCmd.h"
#include "../SylixOS/shell/perfLib/ttinyShellPerfCmd.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
LW_OBJECT_HANDLE        _G_hTShellLock      = LW_OBJECT_HANDLE_INVALID;
static BOOL             _G_bIsInstallSysCmd = LW_FALSE;
/*********************************************************************************************************
  shell 执行线程堆栈大小 (默认与 shell 相同)
*********************************************************************************************************/
size_t                  _G_stShellStackSize = LW_CFG_SHELL_THREAD_STK_SIZE;
/*********************************************************************************************************
** 函数名称: API_TShellTermAlert
** 功能描述: tty 终端响铃
** 输　入  : iFd       输出目标
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_TShellTermAlert (INT  iFd)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (!(__TTINY_SHELL_GET_OPT(ptcbCur) & LW_OPTION_TSHELL_VT100)) {
        return;
    }
    
    fdprintf(iFd, "\a");
}
/*********************************************************************************************************
** 函数名称: API_TShellSetTitel
** 功能描述: tty 终端设置标题
** 输　入  : iFd       输出目标
**           pcTitel   标题
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_TShellSetTitel (INT  iFd, CPCHAR  pcTitel)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (!(__TTINY_SHELL_GET_OPT(ptcbCur) & LW_OPTION_TSHELL_VT100)) {
        return;
    }
    
    fdprintf(iFd, "\x1B]0;%s\x07", pcTitel);
}
/*********************************************************************************************************
** 函数名称: API_TShellScrClear
** 功能描述: tty 终端清屏
** 输　入  : iFd       输出目标
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_TShellScrClear (INT  iFd)
{
    fdprintf(iFd, "\x1B" "\x5B" "\x48" "\x1B" "\x5B" "\x32" "\x4A");
}
/*********************************************************************************************************
** 函数名称: API_TShellSetStackSize
** 功能描述: 设定新的 shell 堆栈大小 (设置后仅对后来创建的 shell 和 背景执行 shell 起效)
** 输　入  : stNewSize     新的堆栈大小 (0 表示不设置)
**           pstOldSize    先前堆栈大小 (返回)
** 输　出  : ERROR_NONE 表示没有错误, -1 表示错误,
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_TShellSetStackSize (size_t  stNewSize, size_t  *pstOldSize)
{
    if (pstOldSize) {
        *pstOldSize = _G_stShellStackSize;
    }
    
    if (stNewSize) {
        _G_stShellStackSize = stNewSize;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_TShellInit
** 功能描述: 安装 tshell 程序, 相当于初始化 tshell 平台
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_TShellInit (VOID)
{
    if (_G_hTShellLock == 0) {
        _G_hTShellLock = API_SemaphoreMCreate("tshell_lock", LW_PRIO_DEF_CEILING, 
                         __TTINY_SHELL_LOCK_OPT, LW_NULL);              /*  创建锁                      */
        if (!_G_hTShellLock) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, 
                         "tshell lock can not create.\r\n");
            _DebugHandle(__LOGMESSAGE_LEVEL,
                         "ttniy shell system is not initialize.\r\n");
            return;
        }
        API_SystemHookAdd(__tShellOptDeleteHook, 
                          LW_OPTION_THREAD_DELETE_HOOK);                /*  安装回调函数                */
        API_SystemHookAdd(__tshellReadlineClean, 
                          LW_OPTION_THREAD_DELETE_HOOK);                /*  readline 历史删除           */
                          
        __tshellColorInit();                                            /*  初始化 shell 颜色系统       */
    }
    
    if (_G_bIsInstallSysCmd == LW_FALSE) {
        _G_bIsInstallSysCmd =  LW_TRUE;
        __tshellSysVarInit();                                           /*  初始化系统环境变量          */
        __tshellSysCmdInit();                                           /*  初始化系统命令              */
        __tshellFsCmdInit();                                            /*  初始化文件系统命令          */
        __tshellModemCmdInit();                                         /*  初始化 modem 命令           */
        
#if defined(__SYLIXOS_LITE)
        __tshellExtCmdInit();
#endif                                                                  /*  __SYLIXOS_LITE              */
        
#if LW_CFG_SHELL_USER_EN > 0
        __tshellUserCmdInit();                                          /*  初始化用户管理命令          */
#endif                                                                  /*  LW_CFG_SHELL_USER_EN        */
#if LW_CFG_SHELL_HEAP_TRACE_EN > 0
        __tshellHeapCmdInit();                                          /*  初始化内存堆命令            */
#endif                                                                  /*  LW_CFG_SHELL_HEAP_TRACE_EN  */
#if LW_CFG_SYSPERF_EN > 0 && LW_CFG_SHELL_PERF_TRACE_EN > 0
        __tshellPerfCmdInit();                                          /*  初始化性能分析命令          */
#endif                                                                  /*  LW_CFG_SHELL_PERF_TRACE_EN  */
#if LW_CFG_SHELL_TAR_EN > 0
        __tshellTarCmdInit();                                           /*  初始化 tar 命令             */
#endif                                                                  /*  LW_CFG_SHELL_TAR_EN         */
    }
}
/*********************************************************************************************************
** 函数名称: API_TShellStartup
** 功能描述: 系统启动时 shell 调用 startup.sh 脚本
** 输　入  : NONE
** 输　出  : 运行结果
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_TShellStartup (VOID)
{
    static BOOL     bInit      = LW_FALSE;
    INT             iOldOption = OPT_TERMINAL;
    INT             iNewOption;
    INT             iRet, iIgn = 0;
    INT             iWaitIgnSec;
    CHAR            cWaitIgnSec[16], cRead;
    struct timeval  tv;
    
    if (bInit) {
        return  (PX_ERROR);
    }
    bInit = LW_TRUE;
    
    if (API_TShellVarGetRt("STARTUP_WAIT_SEC", cWaitIgnSec, 16) > 0) {
        iWaitIgnSec = lib_atoi(cWaitIgnSec);
        if (iWaitIgnSec < 0) {
            iWaitIgnSec = 0;
        
        } else if (iWaitIgnSec > 10) {
            iWaitIgnSec = 10;
        }
        
    } else {
        iWaitIgnSec = 0;
    }
    
    printf("Press <n> to NOT execute /etc/startup.sh (timeout: %d sec(s))\n", iWaitIgnSec);
    
    tv.tv_sec  = iWaitIgnSec;
    tv.tv_usec = 0;
    
    ioctl(STD_IN, FIOGETOPTIONS, &iOldOption);
    iNewOption = iOldOption & ~(OPT_ECHO | OPT_LINE);                   /*  no echo no line mode        */
    ioctl(STD_IN, FIOSETOPTIONS, iNewOption);

    if (waitread(STD_IN, &tv) > 0) {
        if (read(STD_IN, &cRead, 1) == 1) {
            if ((cRead == 'n') || (cRead == 'N')) {
                iIgn = 1;
            }
        }
    }
    
    ioctl(STD_IN, FIOSETOPTIONS, iOldOption);

    if (iIgn) {
        printf("Abort executes /etc/startup.sh\n");
        iRet = PX_ERROR;
        
    } else {
        iRet = API_TShellExec("shfile /etc/startup.sh^");
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_TShellSigEvent
** 功能描述: 向一个 shell 终端发送信号.
** 输　入  : ulShell               终端线程
**           psigevent             信号信息
**           iSigCode              信号
** 输　出  : 发送是否成功.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_SIGNAL_EN > 0

LW_API  
ULONG  API_TShellSigEvent (LW_OBJECT_HANDLE  ulShell, struct sigevent *psigevent, INT  iSigCode)
{
    REGISTER PLW_CLASS_TCB    ptcbShell;
    REGISTER PLW_CLASS_TCB    ptcbJoin;
             LW_OBJECT_HANDLE ulJoin = LW_OBJECT_HANDLE_INVALID;
             UINT16           usIndex;
    
    usIndex = _ObjectGetIndex(ulShell);
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        return  (ERROR_THREAD_NULL);
    }
    
    ptcbShell = __GET_TCB_FROM_INDEX(usIndex);
    ptcbJoin  = ptcbShell->TCB_ptcbJoin;
    if (ptcbJoin) {                                                     /*  等待其他线程结束            */
        ulJoin = ptcbJoin->TCB_ulId;
    }
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    if (ulJoin) {
        _doSigEvent(ulJoin, psigevent, iSigCode);
    
    } else {
        _doSigEvent(ulShell, psigevent, iSigCode);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
/*********************************************************************************************************
** 函数名称: API_TShellCreateEx
** 功能描述: 创建一个 ttiny shell 系统, SylixOS 支持多个终端设备同时运行.
**           tshell 可以使用标准 tty 设备, 或者 pty 虚拟终端设备.
** 输　入  : iTtyFd                终端设备的文件描述符
**           ulOption              启动参数
**           pfuncRunCallback      初始化完毕后, 启动回调
** 输　出  : shell 线程的句柄.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
LW_OBJECT_HANDLE  API_TShellCreateEx (INT  iTtyFd, ULONG  ulOption, FUNCPTR  pfuncRunCallback)
{
    LW_CLASS_THREADATTR     threadattrTShell;
    LW_OBJECT_HANDLE        hTShellHandle;
    INT                     iKernelFile;
    ULONG                   ulOldOption = ulOption;
    PLW_CLASS_TCB           ptcbShell;
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "ttniy shell system initialize...\r\n");
    
    if (!isatty(iTtyFd)) {                                              /*  检测是否为终端设备          */
        _DebugHandle(__ERRORMESSAGE_LEVEL,
                     "is not a tty or pty device.\r\n");
        _DebugHandle(__LOGMESSAGE_LEVEL, 
                     "ttniy shell system is not initialize.\r\n");
        return  (0);
    }
    
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  在进程中启动                */
        if (_G_hTShellLock == 0) {
            _DebugHandle(__ERRORMESSAGE_LEVEL,
                         "shell sub-system not initialization.\r\n");
            return  (0);
        }
        
        iKernelFile = dup2kernel(iTtyFd);                               /*  将此文件描述符 dup 到 kernel*/
        if (iKernelFile < 0) {
            return  (0);
        }
        
        ulOption         |= LW_OPTION_TSHELL_CLOSE_FD;                  /*  执行完毕后需要关闭文件      */
        pfuncRunCallback  = LW_NULL;                                    /*  进程创建不得安装回调        */
    
    } else {
        API_TShellInit();                                               /*  初始化 shell                */
        
        iKernelFile = iTtyFd;
    }
    
    API_ThreadAttrBuild(&threadattrTShell,
                        _G_stShellStackSize,                            /*  shell 堆栈大小              */
                        LW_PRIO_T_SHELL,
                        LW_CFG_SHELL_THREAD_OPTION | LW_OPTION_OBJECT_GLOBAL,
                        (PVOID)iKernelFile);                            /*  构建属性块                  */
    
    hTShellHandle = API_ThreadInit("t_tshell", __tshellThread,
                                   &threadattrTShell, LW_NULL);         /*  创建 tshell 线程            */
    if (!hTShellHandle) {
        if (__PROC_GET_PID_CUR() != 0) {                                /*  在进程中启动                */
            __KERNEL_SPACE_ENTER();
            close(iKernelFile);                                         /*  内核中关闭 dup 到内核的文件 */
            __KERNEL_SPACE_EXIT();
        }
        _DebugHandle(__ERRORMESSAGE_LEVEL, 
                     "tshell thread can not create.\r\n");
        _DebugHandle(__LOGMESSAGE_LEVEL, 
                     "ttniy shell system is not initialize.\r\n");
        return  (0);
        
    } else if ((__PROC_GET_PID_CUR() != 0) && 
               (ulOldOption & LW_OPTION_TSHELL_CLOSE_FD)) {             /*  如果是进程创建, 则关闭文件  */
        close(iTtyFd);
    }
    
    ptcbShell = __GET_TCB_FROM_INDEX(_ObjectGetIndex(hTShellHandle));
    __TTINY_SHELL_SET_OPT(ptcbShell, ulOption);                         /*  初始化选项                  */
    __TTINY_SHELL_SET_CALLBACK(ptcbShell, pfuncRunCallback);            /*  初始化启动回调              */
    
    API_ThreadStart(hTShellHandle);                                     /*  启动 shell 线程             */
    
    return  (hTShellHandle);
}
/*********************************************************************************************************
** 函数名称: API_TShellCreate
** 功能描述: 创建一个 ttiny shell 系统, SylixOS 支持多个终端设备同时运行.
**           tshell 可以使用标准 tty 设备, 或者 pty 虚拟终端设备.
** 输　入  : iTtyFd    终端设备的文件描述符
**           ulOption  启动参数
** 输　出  : shell 线程的句柄.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
LW_OBJECT_HANDLE  API_TShellCreate (INT  iTtyFd, ULONG  ulOption)
{
    return  (API_TShellCreateEx(iTtyFd, ulOption, LW_NULL));
}
/*********************************************************************************************************
** 函数名称: API_TShellLogout
** 功能描述: 注销登录.
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_TShellLogout (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    if (__TTINY_SHELL_GET_MAIN(ptcbCur)) {
        return  (__tshellRestartEx(__TTINY_SHELL_GET_MAIN(ptcbCur), LW_TRUE));
        
    } else {
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_TShellSetOption
** 功能描述: 设置新的 shell 选项.
** 输　入  : hTShellHandle   shell 线程
**           ulNewOpt        新的 shell 选项
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_TShellSetOption (LW_OBJECT_HANDLE  hTShellHandle, ULONG  ulNewOpt)
{
    PLW_CLASS_TCB   ptcbShell;
    UINT16          usIndex;
    
    usIndex = _ObjectGetIndex(hTShellHandle);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(hTShellHandle, _OBJECT_THREAD)) {               /*  检查 ID 类型有效性          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (PX_ERROR);
    }
#endif

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (PX_ERROR);
    }
    
    ptcbShell = __GET_TCB_FROM_INDEX(usIndex);
    __TTINY_SHELL_SET_OPT(ptcbShell, ulNewOpt);
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_TShellGetOption
** 功能描述: 获取新的 shell 选项.
** 输　入  : hTShellHandle   shell 线程
**           pulOpt          shell 选项
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_TShellGetOption (LW_OBJECT_HANDLE  hTShellHandle, ULONG  *pulOpt)
{
    PLW_CLASS_TCB   ptcbShell;
    UINT16          usIndex;
    
    if (!pulOpt) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    usIndex = _ObjectGetIndex(hTShellHandle);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(hTShellHandle, _OBJECT_THREAD)) {               /*  检查 ID 类型有效性          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (PX_ERROR);
    }
#endif

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (PX_ERROR);
    }
    
    ptcbShell = __GET_TCB_FROM_INDEX(usIndex);
    *pulOpt   = __TTINY_SHELL_GET_OPT(ptcbShell);
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_TShellGetUserName
** 功能描述: 通过 shell 用户缓冲获取一个用户名.
** 输　入  : uid           用户 id
**           pcName        用户名
**           stSize        缓冲区大小
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_TShellGetUserName (uid_t  uid, PCHAR  pcName, size_t  stSize)
{
    if (!pcName || !stSize) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (__tshellGetUserName(uid, pcName, stSize, LW_NULL, 0));
}
/*********************************************************************************************************
** 函数名称: API_TShellGetGrpName
** 功能描述: 通过 shell 用户缓冲获取一个组名.
** 输　入  : gid           组 id
**           pcName        组名
**           stSize        缓冲区大小
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_TShellGetGrpName (gid_t  gid, PCHAR  pcName, size_t  stSize)
{
    if (!pcName || !stSize) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (__tshellGetGrpName(gid, pcName, stSize));
}
/*********************************************************************************************************
** 函数名称: API_TShellGetUserHome
** 功能描述: 获得一个用户名 HOME 路径.
** 输　入  : uid           用户 id
**           pcHome        HOME 路径
**           stSize        缓冲区大小
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_TShellGetUserHome (uid_t  uid, PCHAR  pcHome, size_t  stSize)
{
    if (!pcHome || !stSize) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (__tshellGetUserName(uid, LW_NULL, 0, pcHome, stSize));
}
/*********************************************************************************************************
** 函数名称: API_TShellFlushCache
** 功能描述: 删除所有 shell 缓冲的用户与组信息 (当有用户或者组变动时, 需要释放 CACHE)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_TShellFlushCache (VOID)
{
    __tshellFlushCache();
}
/*********************************************************************************************************
** 函数名称: API_TShellKeywordAdd
** 功能描述: 向 ttiny shell 系统添加一个关键字.
** 输　入  : pcKeyword     关键字
**           pfuncCommand  执行的 shell 函数
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_TShellKeywordAdd (CPCHAR  pcKeyword, PCOMMAND_START_ROUTINE  pfuncCommand)
{
    return  (API_TShellKeywordAddEx(pcKeyword, pfuncCommand, LW_OPTION_DEFAULT));
}
/*********************************************************************************************************
** 函数名称: API_TShellKeywordAddEx
** 功能描述: 向 ttiny shell 系统添加一个关键字.
** 输　入  : pcKeyword     关键字
**           pfuncCommand  执行的 shell 函数
**           ulOption      选项
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_TShellKeywordAddEx (CPCHAR  pcKeyword, PCOMMAND_START_ROUTINE  pfuncCommand, ULONG  ulOption)
{
    REGISTER size_t    stStrLen;

    if (__PROC_GET_PID_CUR() != 0) {                                    /*  进程中不能注册命令          */
        _ErrorHandle(ENOTSUP);
        return  (ENOTSUP);
    }
    
    if (!pcKeyword) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pcKeyword invalidate.\r\n");
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (ERROR_TSHELL_EPARAM);
    }
    
    stStrLen = lib_strnlen(pcKeyword, LW_CFG_SHELL_MAX_KEYWORDLEN);
    
    if (stStrLen >= (LW_CFG_SHELL_MAX_KEYWORDLEN + 1)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pcKeyword is too long.\r\n");
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (ERROR_TSHELL_EPARAM);
    }

    return  (__tshellKeywordAdd(pcKeyword, stStrLen, pfuncCommand, ulOption));
}
/*********************************************************************************************************
** 函数名称: API_TShellHelpAdd
** 功能描述: 为一个关键字添加帮助信息
** 输　入  : pcKeyword     关键字
**           pcHelp        帮助信息字符串
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_TShellHelpAdd (CPCHAR  pcKeyword, CPCHAR  pcHelp)
{
             __PTSHELL_KEYWORD   pskwNode = LW_NULL;
    REGISTER size_t              stStrLen;
    
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  进程中不允许操作            */
        _ErrorHandle(ENOTSUP);
        return  (ENOTSUP);
    }
    
    if (!pcKeyword) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pcKeyword invalidate.\r\n");
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (ERROR_TSHELL_EPARAM);
    }
    if (!pcHelp) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pcHelp invalidate.\r\n");
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (ERROR_TSHELL_EPARAM);
    }
    
    stStrLen = lib_strnlen(pcKeyword, LW_CFG_SHELL_MAX_KEYWORDLEN);
    
    if (stStrLen >= (LW_CFG_SHELL_MAX_KEYWORDLEN + 1)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pcKeyword is too long.\r\n");
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (ERROR_TSHELL_EPARAM);
    }
    
    if (ERROR_NONE == __tshellKeywordFind(pcKeyword, &pskwNode)) {      /*  查找关键字                  */
        
        __TTINY_SHELL_LOCK();                                           /*  互斥访问                    */
        if (pskwNode->SK_pcHelpString) {
            __TTINY_SHELL_UNLOCK();                                     /*  释放资源                    */
            
            _DebugHandle(__ERRORMESSAGE_LEVEL, "help message overlap.\r\n");
            _ErrorHandle(ERROR_TSHELL_OVERLAP);
            return  (ERROR_TSHELL_OVERLAP);
        }
        
        stStrLen = lib_strlen(pcHelp);                                  /*  为帮助字串开辟空间          */
        pskwNode->SK_pcHelpString = (PCHAR)__SHEAP_ALLOC(stStrLen + 1);
        if (!pskwNode->SK_pcHelpString) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (ERROR_SYSTEM_LOW_MEMORY);
        }
        lib_strcpy(pskwNode->SK_pcHelpString, pcHelp);
        __TTINY_SHELL_UNLOCK();                                         /*  释放资源                    */
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_TSHELL_EKEYWORD);
        return  (ERROR_TSHELL_EKEYWORD);                                /*  关键字错误                  */
    }
}
/*********************************************************************************************************
** 函数名称: API_TShellFormatAdd
** 功能描述: 为一个关键字添加格式字串信息
** 输　入  : pcKeyword     关键字
**           pcFormat      格式字串
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_TShellFormatAdd (CPCHAR  pcKeyword, CPCHAR  pcFormat)
{
             __PTSHELL_KEYWORD   pskwNode = LW_NULL;
    REGISTER size_t              stStrLen;
    
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  进程中不允许操作            */
        _ErrorHandle(ENOTSUP);
        return  (ENOTSUP);
    }
    
    if (!pcKeyword) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pcKeyword invalidate.\r\n");
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (ERROR_TSHELL_EPARAM);
    }
    if (!pcFormat) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pcHelp invalidate.\r\n");
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (ERROR_TSHELL_EPARAM);
    }
    
    stStrLen = lib_strnlen(pcKeyword, LW_CFG_SHELL_MAX_KEYWORDLEN);
    
    if (stStrLen >= (LW_CFG_SHELL_MAX_KEYWORDLEN + 1)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pcKeyword is too long.\r\n");
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (ERROR_TSHELL_EPARAM);
    }
    
    if (ERROR_NONE == __tshellKeywordFind(pcKeyword, &pskwNode)) {      /*  查找关键字                  */
        
        __TTINY_SHELL_LOCK();                                           /*  互斥访问                    */
        if (pskwNode->SK_pcFormatString) {
            __TTINY_SHELL_UNLOCK();                                     /*  释放资源                    */
            
            _DebugHandle(__ERRORMESSAGE_LEVEL, "format string overlap.\r\n");
            _ErrorHandle(ERROR_TSHELL_OVERLAP);
            return  (ERROR_TSHELL_OVERLAP);
        }
        
        stStrLen = lib_strlen(pcFormat);                                /*  为帮助字串开辟空间          */
        pskwNode->SK_pcFormatString = (PCHAR)__SHEAP_ALLOC(stStrLen + 1);
        if (!pskwNode->SK_pcFormatString) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (ERROR_SYSTEM_LOW_MEMORY);
        }
        lib_strcpy(pskwNode->SK_pcFormatString, pcFormat);
        __TTINY_SHELL_UNLOCK();                                         /*  释放资源                    */
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_TSHELL_EKEYWORD);
        return  (ERROR_TSHELL_EKEYWORD);                                /*  关键字错误                  */
    }
}
/*********************************************************************************************************
** 函数名称: API_TShellExec
** 功能描述: ttiny shell 系统, 执行一条 shell 命令
** 输　入  : pcCommandExec    命令字符串
** 输　出  : 命令返回值(当发生错误时, 返回值为负值)
** 全局变量: 
** 调用模块: 
** 注  意  : 当 shell 检测出命令字符串错误时, 将会返回负值, 此值取相反数后即为错误编号.

                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_TShellExec (CPCHAR  pcCommandExec)
{
    if (__PROC_GET_PID_CUR() != 0) {
        return  (API_TShellExecBg(pcCommandExec, LW_NULL, LW_NULL, LW_TRUE, LW_NULL));
            
    } else {
        return  (__tshellExec(pcCommandExec, LW_NULL));
    }
}
/*********************************************************************************************************
** 函数名称: API_TShellExecBg
** 功能描述: ttiny shell 系统, 背景执行一条 shell 命令 (不过成功与否都会关闭需要关闭的文件)
** 输　入  : pcCommandExec  命令字符串
**           iFd[3]         标准文件
**           bClosed[3]     执行结束后是否关闭对应标准文件
**           bIsJoin        是否等待命令执行结束
**           pulSh          背景线程句柄, (仅当 bIsJoin = LW_FALSE 时返回)
** 输　出  : 命令返回值(当发生错误时, 返回值为负值)
** 全局变量: 
** 调用模块: 
** 注  意  : 当 shell 检测出命令字符串错误时, 将会返回负值, 此值取相反数后即为错误编号.

                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_TShellExecBg (CPCHAR  pcCommandExec, INT  iFd[3], BOOL  bClosed[3], 
                       BOOL  bIsJoin, LW_OBJECT_HANDLE *pulSh)
{
    INT     iRet;
    INT     iError;
    INT     iFdArray[3];
    BOOL    bClosedArray[3];
    
    CHAR    cCommand[LW_CFG_SHELL_MAX_COMMANDLEN + 1];
    size_t  stStrLen = lib_strnlen(pcCommandExec, LW_CFG_SHELL_MAX_COMMANDLEN + 1);
    BOOL    bNeedAsyn;
    
    if ((stStrLen > LW_CFG_SHELL_MAX_COMMANDLEN - 1) || (stStrLen < 1)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    lib_strcpy(cCommand, pcCommandExec);
    
    if (__PROC_GET_PID_CUR() != 0) {                                    /*  进程中创建                  */
        if (iFd == LW_NULL) {
            iFdArray[0] = dup2kernel(STD_IN);
            iFdArray[1] = dup2kernel(STD_OUT);
            iFdArray[2] = dup2kernel(STD_ERR);
        
        } else {
            iFdArray[0] = dup2kernel(iFd[0]);
            iFdArray[1] = dup2kernel(iFd[1]);
            iFdArray[2] = dup2kernel(iFd[2]);
        }
        
        /*
         *  由于相关文件已经 dup 到内核中, 所以这里可以关闭进程中的相关的文件
         */
        if (bClosed && iFd) {
            if (bClosed[0]) {
                close(iFd[0]);
            }
            if (bClosed[1]) {
                close(iFd[1]);
            }
            if (bClosed[2]) {
                close(iFd[2]);
            }
        }
        
        /*
         *  由于已经 dup 到内核中, 这里必须在运行结束后关闭这些文件
         */
        bClosedArray[0] = LW_TRUE;                                      /*  执行完毕后需要关闭这些       */
        bClosedArray[1] = LW_TRUE;
        bClosedArray[2] = LW_TRUE;
        
        iFd     = iFdArray;
        bClosed = bClosedArray;                                         /*  文件是 dup 出来的这里必须全关*/
    
        __tshellPreTreatedBg(cCommand, LW_NULL, &bNeedAsyn);            /*  预处理背景执行相关参数       */
        
        if (bNeedAsyn) {
            bIsJoin = LW_FALSE;
        }
        
    } else {                                                            /*  内核中调用                   */
        if (iFd == LW_NULL) {
            LW_OBJECT_HANDLE  ulMe = API_ThreadIdSelf();
            iFd = iFdArray;
            iFdArray[0] = API_IoTaskStdGet(ulMe, STD_IN);
            iFdArray[1] = API_IoTaskStdGet(ulMe, STD_OUT);
            iFdArray[2] = API_IoTaskStdGet(ulMe, STD_ERR);
        }
        
        if (bClosed == LW_NULL) {
            bClosed = bClosedArray;
            bClosedArray[0] = LW_FALSE;
            bClosedArray[1] = LW_FALSE;
            bClosedArray[2] = LW_FALSE;
        }
    }
    
    iError = __tshellBgCreateEx(iFd, bClosed, cCommand, lib_strlen(cCommand), 
                                0ul, bIsJoin, 0, pulSh, &iRet);
    if (iError < 0) {                                                   /*  背景创建失败                */
        /*
         *  运行失败, 则关闭内核中需要关闭的文件
         */
        __KERNEL_SPACE_ENTER();
        if (bClosed[0]) {
            close(iFd[0]);
        }
        if (bClosed[1]) {
            close(iFd[1]);
        }
        if (bClosed[2]) {
            close(iFd[2]);
        }
        __KERNEL_SPACE_EXIT();
    }                                                                   /*  如果运行成功, 则会自动关闭  */
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

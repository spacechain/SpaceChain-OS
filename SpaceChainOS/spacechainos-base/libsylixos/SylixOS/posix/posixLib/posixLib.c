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
** 文   件   名: posixLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 30 日
**
** 描        述: posix 内部库.

** BUG:
2011.02.26  加入对 aio 的初始化.
2011.03.11  加入对 syslog 的初始化.
2013.05.02  _posixCtxDelete() 使用 syshook 调用, 不再使用 cleanup.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/posixLib.h"                                        /*  已包含操作系统头文件        */
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0
VOID  __procFsPosixInfoInit(VOID);
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */

#if LW_CFG_SIGNAL_EN > 0
VOID  _posixAioInit(VOID);
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */

VOID  _posixPSemInit(VOID);
VOID  _posixPMutexInit(VOID);
VOID  _posixPRWLockInit(VOID);
VOID  _posixPCondInit(VOID);

#if LW_CFG_POSIX_SYSLOG_EN > 0
VOID  _posixSyslogInit(VOID);
#endif                                                                  /*  LW_CFG_POSIX_SYSLOG_EN > 0  */
/*********************************************************************************************************
  posix lock
*********************************************************************************************************/
LW_OBJECT_HANDLE        _G_ulPosixLock = 0;
/*********************************************************************************************************
** 函数名称: _posixCtxDelete
** 功能描述: 删除 posix 线程上下文
** 输　入  : ulId          线程 id
**           pvRetVal      返回值
**           ptcb          线程控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _posixCtxDelete (LW_OBJECT_HANDLE  ulId, PVOID  pvRetVal, PLW_CLASS_TCB  ptcb)
{
    if (ptcb->TCB_pvPosixContext) {
        __SHEAP_FREE(ptcb->TCB_pvPosixContext);
        ptcb->TCB_pvPosixContext = LW_NULL;                             /*  一定不能忘记!               */
                                                                        /*  否则线程重启可能造成错误    */
    }
}
/*********************************************************************************************************
** 函数名称: _posixCtxCreate
** 功能描述: 创建 posix 线程上下文
** 输　入  : ptcb          线程控制块
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _posixCtxCreate (PLW_CLASS_TCB   ptcb)
{
    API_ThreadSafe();
    ptcb->TCB_pvPosixContext =  (PVOID)__SHEAP_ALLOC(sizeof(__PX_CONTEXT));
    if (ptcb->TCB_pvPosixContext == LW_NULL) {
        API_ThreadUnsafe();                                             /*  缺少内存                    */
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    API_ThreadUnsafe();
    
    lib_bzero(ptcb->TCB_pvPosixContext, sizeof(__PX_CONTEXT));          /*  内容清零                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _posixCtxGet
** 功能描述: 获取 posix 线程上下文 (无则创建)
** 输　入  : ptcb          线程控制块
** 输　出  : 线程上下文
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
__PX_CONTEXT  *_posixCtxGet (PLW_CLASS_TCB   ptcb)
{
    if (ptcb == LW_NULL) {
        errno = EINVAL;
        return  (LW_NULL);
    }

    if (ptcb->TCB_pvPosixContext == LW_NULL) {
        _posixCtxCreate(ptcb);                                          /*  创建                        */
    }
    
    return  ((__PX_CONTEXT *)ptcb->TCB_pvPosixContext);
}
/*********************************************************************************************************
** 函数名称: _posixCtxTryGet
** 功能描述: 获取 posix 线程上下文
** 输　入  : ptcb          线程控制块
** 输　出  : 线程上下文
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
__PX_CONTEXT  *_posixCtxTryGet (PLW_CLASS_TCB   ptcb)
{
    if (ptcb == LW_NULL) {
        errno = EINVAL;
        return  (LW_NULL);
    }
    
    return  ((__PX_CONTEXT *)ptcb->TCB_pvPosixContext);
}
/*********************************************************************************************************
** 函数名称: _posixVprocCtxGet
** 功能描述: 获取 posix 进程上下文
** 输　出  : 进程上下文
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
__PX_VPROC_CONTEXT  *_posixVprocCtxGet (VOID)
{
#if LW_CFG_MODULELOADER_EN > 0
    LW_LD_VPROC  *pvproc = __LW_VP_GET_CUR_PROC();
    
    if (pvproc == LW_NULL) {
        pvproc =  &_G_vprocKernel;
    }
    
    return  (&pvproc->VP_pvpCtx);

#else
    static __PX_VPROC_CONTEXT   pvpCtx;
    
    return  (&pvpCtx);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
}
/*********************************************************************************************************
** 函数名称: API_PosixInit
** 功能描述: 初始化 posix 系统 (如果系统支持 proc 文件系统, 则必须放在 proc 文件系统安装之后!).
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_PosixInit (VOID)
{
#if LW_CFG_SHELL_EN > 0
    VOID  __tshellPosixInit(VOID);
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

    static BOOL         bIsInit = LW_FALSE;

    if (_G_ulPosixLock == 0) {
        _G_ulPosixLock =  API_SemaphoreMCreate("px_lock", LW_PRIO_DEF_CEILING, 
                                               LW_OPTION_WAIT_PRIORITY |
                                               LW_OPTION_INHERIT_PRIORITY | 
                                               LW_OPTION_DELETE_SAFE |
                                               LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }
    
    if (bIsInit) {
        return;
    }
    
    API_SystemHookAdd(_posixCtxDelete, LW_OPTION_THREAD_DELETE_HOOK);   /*  posix 线程数据销毁          */
    
#if LW_CFG_PROCFS_EN > 0
    __procFsPosixInfoInit();
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */

#if LW_CFG_POSIX_AIO_EN > 0
    _posixAioInit();
#endif                                                                  /*  LW_CFG_POSIX_AIO_EN > 0     */

    _posixPSemInit();
    _posixPMutexInit();
    _posixPRWLockInit();
    _posixPCondInit();
    
#if LW_CFG_POSIX_SYSLOG_EN > 0
    _posixSyslogInit();
#endif                                                                  /*  LW_CFG_POSIX_SYSLOG_EN > 0  */
    
#if LW_CFG_SHELL_EN > 0
    __tshellPosixInit();                                                /*  初始化 shell 命令           */
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
    
    bIsInit = LW_TRUE;
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

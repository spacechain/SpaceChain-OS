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
** 文   件   名: selectInit.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 11 月 07 日
**
** 描        述:  IO 系统 select 子系统初始化.

** BUG
2008.03.16 将 LW_HANDLE 改为 LW_OBJECT_HANDLE.
2009.07.17 升级 __selTaskDeleteHook() 回调类型.
2017.08.17 多路 IO 复用, 可被信号打断.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0)
#include "select.h"
#include "selectDrv.h"
/*********************************************************************************************************
  内部函数
*********************************************************************************************************/
static VOID     __selTaskCreateHook(LW_OBJECT_HANDLE  ulId, ULONG  ulOption);
static VOID     __selTaskDeleteHook(LW_OBJECT_HANDLE  ulId, PVOID  pvReturnVal, PLW_CLASS_TCB  ptcb);
/*********************************************************************************************************
  内部函数
*********************************************************************************************************/
extern INT      __selDoIoctls(fd_set               *pfdset,
                              INT                   iFdSetWidth,
                              INT                   iFunc,
                              PLW_SEL_WAKEUPNODE    pselwunNode,
                              BOOL                  bStopOnErr);
/*********************************************************************************************************
** 函数名称: _SelectInit
** 功能描述: 初始化 select 库
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT     _SelectInit (VOID)
{
    REGISTER LW_ERROR       ulError;
    
    ulError = API_SystemHookAdd(__selTaskCreateHook, 
                                LW_OPTION_THREAD_CREATE_HOOK);          /*  安装线程创建钩子函数        */
    if (ulError) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "couldn't install task create hook.\r\n");
        return  (PX_ERROR);                                             /*  安装失败                    */
    }
    
    ulError = API_SystemHookAdd(__selTaskDeleteHook, 
                                LW_OPTION_THREAD_DELETE_HOOK);          /*  安装线程销毁钩子函数        */
    if (ulError) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "couldn't install task delete hook.\r\n");
        return  (PX_ERROR);                                             /*  安装失败                    */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __selTaskCreateHook
** 功能描述: 线程建立时的钩子函数
** 输　入  : ulId                  线程 Id
             ulOption              线程建立选项
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID    __selTaskCreateHook (LW_OBJECT_HANDLE  ulId, ULONG  ulOption)
{
    REGISTER UINT16              usIndex = _ObjectGetIndex(ulId);
    REGISTER PLW_CLASS_TCB       ptcb;
    REGISTER LW_SEL_CONTEXT     *pselctxNew;
    
    ptcb = __GET_TCB_FROM_INDEX(usIndex);
    
    if (ulOption & LW_OPTION_THREAD_UNSELECT) {                         /*  此线程不使用 select 函数    */
        ptcb->TCB_pselctxContext = LW_NULL;
        return;
    }

    pselctxNew = (LW_SEL_CONTEXT *)__SHEAP_ALLOC(sizeof(LW_SEL_CONTEXT));
    if (!pselctxNew) {                                                  /*  开辟失败                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "couldn't allocate memory for select context.\r\n");
        return;
    }
    
    pselctxNew->SELCTX_hSembWakeup = API_SemaphoreBCreate("sel_wakeup",
                                        LW_TRUE,
                                        LW_OPTION_WAIT_FIFO |
#if LW_CFG_SELECT_INTER_EN > 0
                                        LW_OPTION_SIGNAL_INTER |        /*  可被信号打断                */
#endif                                                                  /*  LW_CFG_SELECT_INTER_EN > 0  */
                                        LW_OPTION_OBJECT_GLOBAL,
                                        LW_NULL);                       /*  创建信号量                  */
    if (!pselctxNew->SELCTX_hSembWakeup) {  
        __SHEAP_FREE(pselctxNew);                                       /*  信号量创建失败              */
        return;
    }
    
    pselctxNew->SELCTX_bPendedOnSelect = LW_FALSE;                      /*  线程没有阻塞                */
    
    ptcb->TCB_pselctxContext = pselctxNew;
}
/*********************************************************************************************************
** 函数名称: __selTaskDeleteHook
** 功能描述: 线程销毁时的钩子函数
** 输　入  : ulId                  线程 Id
**           pvReturnVal           线程返回值
**           ptcb                  线程 TCB
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID    __selTaskDeleteHook (LW_OBJECT_HANDLE  ulId, PVOID  pvReturnVal, PLW_CLASS_TCB  ptcb)
{
    REGISTER LW_SEL_CONTEXT     *pselctxDelete;
             LW_SEL_WAKEUPNODE   selwunNode;
    
    (VOID)pvReturnVal;                                                  /*  不使用此参数                */
    
    pselctxDelete = ptcb->TCB_pselctxContext;
    if (!pselctxDelete) {
        return;                                                         /*  没有 select context         */
    }
    
    if (pselctxDelete->SELCTX_bPendedOnSelect) {                        /*  被 select() 阻塞            */
        selwunNode.SELWUN_hThreadId = ulId;                             /*  释放所有节点                */

        selwunNode.SELWUN_seltypType = SELREAD;
        __selDoIoctls(&pselctxDelete->SELCTX_fdsetOrigReadFds,
                      pselctxDelete->SELCTX_iWidth,
                      FIOUNSELECT,
                      &selwunNode,
                      LW_FALSE);                                        /*  释放所有等待读使能的节点    */
        
        selwunNode.SELWUN_seltypType = SELWRITE;
        __selDoIoctls(&pselctxDelete->SELCTX_fdsetOrigWriteFds,
                      pselctxDelete->SELCTX_iWidth,
                      FIOUNSELECT,
                      &selwunNode,
                      LW_FALSE);                                        /*  释放所有等待写使能的节点    */
        
        selwunNode.SELWUN_seltypType = SELEXCEPT;
        __selDoIoctls(&pselctxDelete->SELCTX_fdsetOrigExceptFds,
                       pselctxDelete->SELCTX_iWidth,
                       FIOUNSELECT,
                      &selwunNode,
                       LW_FALSE);                                       /*  释放所有等待异常使能的节点  */
    }
    
    API_SemaphoreBDelete(&pselctxDelete->SELCTX_hSembWakeup);           /*  删除信号量                  */
    
    ptcb->TCB_pselctxContext = LW_NULL;
    __SHEAP_FREE((PVOID)pselctxDelete);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_SELECT_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: KernelIpi.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 11 月 08 日
**
** 描        述: SMP 系统核间中断.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_KernelSmpCall
** 功能描述: 指定 CPU 调用指定函数
** 输　入  : ulCPUId       CPU ID
**           pfunc         同步执行函数 (被调用函数内部不允许有锁内核操作, 否则可能产生死锁)
**           pvArg         同步参数
**           pfuncAsync    异步执行函数 (被调用函数内部不允许有锁内核操作, 否则可能产生死锁)
**           pvAsync       异步执行参数
**           iOpt          选项 IPIM_OPT_NORMAL / IPIM_OPT_NOKERN
** 输　出  : 同步调用返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

LW_API  
INT  API_KernelSmpCall (ULONG        ulCPUId, 
                        FUNCPTR      pfunc, 
                        PVOID        pvArg,
                        VOIDFUNCPTR  pfuncAsync,
                        PVOID        pvAsync, 
                        INT          iOpt)
{
    BOOL    bLock;
    INT     iRet;
    
    bLock = __SMP_CPU_LOCK();                                           /*  锁定当前 CPU 执行           */
    
    if (ulCPUId == LW_CPU_GET_CUR_ID()) {                               /*  不能自己调用自己            */
        iRet = PX_ERROR;
    
    } else {
        iRet = _SmpCallFunc(ulCPUId, pfunc, pvArg, pfuncAsync, pvAsync, iOpt);
    }
    
    __SMP_CPU_UNLOCK(bLock);                                            /*  解锁当前 CPU 执行           */
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_KernelSmpCallAll
** 功能描述: 所有激活的 CPU 调用指定函数
** 输　入  : pfunc         同步执行函数 (被调用函数内部不允许有锁内核操作, 否则可能产生死锁)
**           pvArg         同步参数
**           pfuncAsync    异步执行函数 (被调用函数内部不允许有锁内核操作, 否则可能产生死锁)
**           pvAsync       异步执行参数
**           iOpt          选项 IPIM_OPT_NORMAL / IPIM_OPT_NOKERN
** 输　出  : NONE (无法确定返回值)
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_KernelSmpCallAll (FUNCPTR      pfunc, 
                            PVOID        pvArg,
                            VOIDFUNCPTR  pfuncAsync,
                            PVOID        pvAsync,
                            INT          iOpt)
{
    BOOL    bLock;
    
    bLock = __SMP_CPU_LOCK();                                           /*  锁定当前 CPU 执行           */
    
    _SmpCallFuncAllOther(pfunc, pvArg, pfuncAsync, pvAsync, iOpt);
    
    if (pfunc) {
        pfunc(pvArg);
    }
    
    if (pfuncAsync) {
        pfuncAsync(pvAsync);
    }
    
    __SMP_CPU_UNLOCK(bLock);                                            /*  解锁当前 CPU 执行           */
}
/*********************************************************************************************************
** 函数名称: API_KernelSmpCallAllOther
** 功能描述: 其他所有激活的 CPU 调用指定函数
** 输　入  : pfunc         同步执行函数 (被调用函数内部不允许有锁内核操作, 否则可能产生死锁)
**           pvArg         同步参数
**           pfuncAsync    异步执行函数 (被调用函数内部不允许有锁内核操作, 否则可能产生死锁)
**           pvAsync       异步执行参数
**           iOpt          选项 IPIM_OPT_NORMAL / IPIM_OPT_NOKERN
** 输　出  : NONE (无法确定返回值)
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_KernelSmpCallAllOther (FUNCPTR      pfunc, 
                                 PVOID        pvArg,
                                 VOIDFUNCPTR  pfuncAsync,
                                 PVOID        pvAsync,
                                 INT          iOpt)
{
    BOOL    bLock;
    
    bLock = __SMP_CPU_LOCK();                                           /*  锁定当前 CPU 执行           */
    
    _SmpCallFuncAllOther(pfunc, pvArg, pfuncAsync, pvAsync, iOpt);
    
    __SMP_CPU_UNLOCK(bLock);                                            /*  解锁当前 CPU 执行           */
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/

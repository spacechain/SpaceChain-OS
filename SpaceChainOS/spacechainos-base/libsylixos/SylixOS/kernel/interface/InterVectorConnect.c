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
** 文   件   名: InterVectorConnect.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 02 月 02 日
**
** 描        述: 设置系统指定向量中断服务.
**
** 注        意: 此函数替换了 InterVectorSet 系列函数. 2011.03.31

** BUG:
2014.04.22  API_InterVectorConnectEx() 不允许安装重复的中断处理函数.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  显示中断向量表的过程中, 不能执行删除操作
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0
LW_OBJECT_HANDLE    _K_ulInterShowLock = LW_OBJECT_HANDLE_INVALID;
/*********************************************************************************************************
  宏操作
*********************************************************************************************************/
#define INTER_SHOWLOCK_CREATE()    \
        if (_K_ulInterShowLock == LW_OBJECT_HANDLE_INVALID) {   \
            _K_ulInterShowLock =  API_SemaphoreMCreate("ints_lock", LW_PRIO_DEF_CEILING,    \
                                                       LW_OPTION_OBJECT_GLOBAL, LW_NULL);   \
        }
#define INTER_SHOWLOCK_LOCK()       \
        API_SemaphoreMPend(_K_ulInterShowLock, LW_OPTION_WAIT_INFINITE)
#define INTER_SHOWLOCK_UNLOCK()     \
        API_SemaphoreMPost(_K_ulInterShowLock)
#endif
/*********************************************************************************************************
** 函数名称: API_InterVectorConnect
** 功能描述: 设置系统指定向量中断服务
** 输　入  : ulVector                      中断向量号
**           pfuncIsr                      服务函数
**           pvArg                         服务函数参数
**           pcName                        中断服务名称
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
ULONG  API_InterVectorConnect (ULONG            ulVector,
                               PINT_SVR_ROUTINE pfuncIsr,
                               PVOID            pvArg,
                               CPCHAR           pcName)
{
    return  (API_InterVectorConnectEx(ulVector, pfuncIsr, LW_NULL, pvArg, pcName));
}
/*********************************************************************************************************
** 函数名称: API_InterVectorConnectEx
** 功能描述: 设置系统指定向量中断服务
** 输　入  : ulVector                      中断向量号
**           pfuncIsr                      服务函数
**           pfuncClear                    附加中断清除函数(可为 NULL)
**           pvArg                         服务函数参数
**           pcName                        中断服务名称
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
ULONG  API_InterVectorConnectEx (ULONG              ulVector,
                                 PINT_SVR_ROUTINE   pfuncIsr,
                                 VOIDFUNCPTR        pfuncClear,
                                 PVOID              pvArg,
                                 CPCHAR             pcName)
{
    INTREG              iregInterLevel;
    BOOL                bNeedFree;
    
    PLW_LIST_LINE       plineTemp;
    PLW_CLASS_INTACT    piactionOld;
    PLW_CLASS_INTACT    piaction;
    PLW_CLASS_INTDESC   pidesc;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    INTER_SHOWLOCK_CREATE();

    if (_Object_Name_Invalid(pcName)) {                                 /*  检查名字有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (ERROR_KERNEL_PNAME_TOO_LONG);
    }
    
    if (_Inter_Vector_Invalid(ulVector)) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }
    
    if (pfuncIsr == LW_NULL) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }
    
    piaction = (PLW_CLASS_INTACT)__KHEAP_ALLOC(sizeof(LW_CLASS_INTACT));
    if (piaction == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);
        return  (ERROR_KERNEL_LOW_MEMORY);
    }
    lib_bzero(piaction, sizeof(LW_CLASS_INTACT));
    
    piaction->IACT_pfuncIsr   = pfuncIsr;
    piaction->IACT_pfuncClear = pfuncClear;
    piaction->IACT_pvArg      = pvArg;
    if (pcName) {
        lib_strcpy(piaction->IACT_cInterName, pcName);
    } else {
        piaction->IACT_cInterName[0] = PX_EOS;
    }
    
    pidesc = LW_IVEC_GET_IDESC(ulVector);
    
    LW_SPIN_LOCK_QUICK(&pidesc->IDESC_slLock, &iregInterLevel);         /*  关闭中断同时锁住 spinlock   */

    if (LW_IVEC_GET_FLAG(ulVector) & LW_IRQ_FLAG_QUEUE) {               /*  队列服务类型向量            */
        for (plineTemp  = pidesc->IDESC_plineAction;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            piactionOld = _LIST_ENTRY(plineTemp, LW_CLASS_INTACT, IACT_plineManage);
            if ((piactionOld->IACT_pfuncIsr == pfuncIsr) &&
                (piactionOld->IACT_pvArg    == pvArg)) {                /*  中断处理函数是否被重复安装  */
                break;
            }
        }
        
        if (plineTemp) {                                                /*  此中断被重复安装            */
            bNeedFree = LW_TRUE;
        
        } else {
            _List_Line_Add_Ahead(&piaction->IACT_plineManage,
                                 &pidesc->IDESC_plineAction);
            bNeedFree = LW_FALSE;
        }
    
    } else {                                                            /*  非队列服务式中断向量        */
        if (pidesc->IDESC_plineAction) {
            piactionOld = _LIST_ENTRY(pidesc->IDESC_plineAction, 
                                      LW_CLASS_INTACT, 
                                      IACT_plineManage);
            piactionOld->IACT_pfuncIsr   = piaction->IACT_pfuncIsr;
            piactionOld->IACT_pfuncClear = piaction->IACT_pfuncClear;
            piactionOld->IACT_pvArg      = piaction->IACT_pvArg;
            lib_strcpy(piactionOld->IACT_cInterName, piaction->IACT_cInterName);
            bNeedFree = LW_TRUE;
        
        } else {
            _List_Line_Add_Ahead(&piaction->IACT_plineManage,
                                 &pidesc->IDESC_plineAction);
            bNeedFree = LW_FALSE;
        }
    }
    
    LW_SPIN_UNLOCK_QUICK(&pidesc->IDESC_slLock, iregInterLevel);        /*  打开中断, 同时打开 spinlock */
    
    if (bNeedFree) {
        __KHEAP_FREE(piaction);
    }
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "IRQ %d : %s connect : %p\r\n",
                 (INT)ulVector, (pcName ? pcName : ""), (PVOID)pfuncIsr);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_InterVectorDisconnect
** 功能描述: 解除系统指定向量中断服务
** 输　入  : ulVector                      中断向量号
**           pfuncIsr                      服务函数
**           pvArg                         服务函数参数
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
ULONG  API_InterVectorDisconnect (ULONG             ulVector,
                                  PINT_SVR_ROUTINE  pfuncIsr,
                                  PVOID             pvArg)
{
    return  (API_InterVectorDisconnectEx(ulVector, pfuncIsr, pvArg, LW_IRQ_DISCONN_DEFAULT));
}
/*********************************************************************************************************
** 函数名称: API_InterVectorDisconnectEx
** 功能描述: 解除系统指定向量中断服务
** 输　入  : ulVector                      中断向量号
**           pfuncIsr                      服务函数
**           pvArg                         服务函数参数
**           ulOption                      删除选项
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
ULONG  API_InterVectorDisconnectEx (ULONG             ulVector,
                                    PINT_SVR_ROUTINE  pfuncIsr,
                                    PVOID             pvArg,
                                    ULONG             ulOption)
{
    INTREG              iregInterLevel;
    BOOL                bNeedFree;
    
    PLW_LIST_LINE       plineTemp;
    PLW_CLASS_INTACT    piaction;
    PLW_CLASS_INTDESC   pidesc;

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    INTER_SHOWLOCK_CREATE();

    if (_Inter_Vector_Invalid(ulVector)) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }
    
    if (!(ulOption & LW_IRQ_DISCONN_ALL) && (pfuncIsr == LW_NULL)) {
        _ErrorHandle(ERROR_KERNEL_VECTOR_NULL);
        return  (ERROR_KERNEL_VECTOR_NULL);
    }
    
__disconn:
    bNeedFree = LW_FALSE;

    INTER_SHOWLOCK_LOCK();

    pidesc = LW_IVEC_GET_IDESC(ulVector);

    LW_SPIN_LOCK_QUICK(&pidesc->IDESC_slLock, &iregInterLevel);         /*  关闭中断同时锁住 spinlock   */
    
    for (plineTemp  = pidesc->IDESC_plineAction;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        piaction = _LIST_ENTRY(plineTemp, LW_CLASS_INTACT, IACT_plineManage);
        if (ulOption & LW_IRQ_DISCONN_ALL) {                            /*  删除所有                    */
            bNeedFree = LW_TRUE;
        
        } else if (ulOption & LW_IRQ_DISCONN_IGNORE_ARG) {
            if (piaction->IACT_pfuncIsr == pfuncIsr) {                  /*  删除匹配的函数              */
                bNeedFree = LW_TRUE;
            }
        
        } else {
            if ((piaction->IACT_pfuncIsr == pfuncIsr) &&
                (piaction->IACT_pvArg    == pvArg)) {                   /*  删除匹配的函数与参数        */
                bNeedFree = LW_TRUE;
            }
        }
        
        if (bNeedFree) {
            _List_Line_Del(&piaction->IACT_plineManage,
                           &pidesc->IDESC_plineAction);
            break;
        }
    }
    
    LW_SPIN_UNLOCK_QUICK(&pidesc->IDESC_slLock, iregInterLevel);        /*  打开中断, 同时打开 spinlock */
    
    INTER_SHOWLOCK_UNLOCK();
    
    if (bNeedFree) {
        __KHEAP_FREE(piaction);
        if (ulOption & (LW_IRQ_DISCONN_ALL | LW_IRQ_DISCONN_IGNORE_ARG)) {
            goto    __disconn;
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

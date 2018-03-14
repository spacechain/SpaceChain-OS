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
** 文   件   名: ThreadOnce.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 02 月 03 日
**
** 描        述: 这是系统类 pthread_once 支持.

** BUG:
2010.01.09  使用 ATOMIC 锁进行互斥.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadOnce
** 功能描述: 线程安全的仅执行一边指定函数.
** 输　入  : pbOnce            once 相关 BOOL 全局变量, 必须初始化为 LW_FALSE.
**           pfuncRoutine      需要执行的函数.
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_THREAD_EXT_EN > 0

LW_API  
INT  API_ThreadOnce (BOOL  *pbOnce, VOIDFUNCPTR  pfuncRoutine)
{
             INTREG     iregInterLevel;
    REGISTER INT        iOk = LW_FALSE;
    
    if (!pbOnce) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __LW_ATOMIC_LOCK(iregInterLevel);                                   /*  锁定                        */
    if (*pbOnce == LW_FALSE) {                                          /*  互斥的判断是否执行          */
        *pbOnce =  LW_TRUE;
        iOk     =  LW_TRUE;                                             /*  设置独立标志                */
    }
    __LW_ATOMIC_UNLOCK(iregInterLevel);                                 /*  解锁                        */
    
    if (pfuncRoutine && iOk) {
        LW_SOFUNC_PREPARE(pfuncRoutine);
        pfuncRoutine();                                                 /*  执行                        */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ThreadOnce2
** 功能描述: 线程安全的仅执行一边指定函数.
** 输　入  : pbOnce            once 相关 BOOL 全局变量, 必须初始化为 LW_FALSE.
**           pfuncRoutine      需要执行的函数.
**           pvArg             执行参数
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_ThreadOnce2 (BOOL  *pbOnce, VOIDFUNCPTR  pfuncRoutine, PVOID  pvArg)
{
             INTREG     iregInterLevel;
    REGISTER INT        iOk = LW_FALSE;
    
    if (!pbOnce) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __LW_ATOMIC_LOCK(iregInterLevel);                                   /*  锁定                        */
    if (*pbOnce == LW_FALSE) {                                          /*  互斥的判断是否执行          */
        *pbOnce =  LW_TRUE;
        iOk     =  LW_TRUE;                                             /*  设置独立标志                */
    }
    __LW_ATOMIC_UNLOCK(iregInterLevel);                                 /*  解锁                        */
    
    if (pfuncRoutine && iOk) {
        LW_SOFUNC_PREPARE(pfuncRoutine);
        pfuncRoutine(pvArg);                                            /*  执行                        */
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/

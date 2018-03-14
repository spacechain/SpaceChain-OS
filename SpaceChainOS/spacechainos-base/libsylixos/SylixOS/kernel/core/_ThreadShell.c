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
** 文   件   名: _ThreadShell.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: 这是系统线程的外壳。

** BUG
2007.11.04  将 0xFFFFFFFF 改为 __ARCH_ULONG_MAX.
2008.01.16  API_ThreadDelete() -> API_ThreadForceDelete();
2012.03.20  减少对 _K_ptcbTCBCur 的引用, 尽量采用局部变量, 减少对当前 CPU ID 获取的次数.
2015.12.05  如果线程创建时已经使能了 FPU 则直接打开 FPU.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _BuidTCB
** 功能描述: 创建一个TCB
** 输　入  : pvThreadStartAddress              线程代码段起始地址
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  _ThreadShell (PVOID  pvThreadStartAddress)
{
    INTREG              iregInterLevel;
    PLW_CLASS_TCB       ptcbCur;
    PVOID               pvReturnVal;
    LW_OBJECT_HANDLE    ulId;
    
#if defined(LW_CFG_CPU_ARCH_C6X)
    archIntEnableForce();                                               /*  使能中断                    */
#endif                                                                  /*  LW_CFG_CPU_ARCH_C6X         */

    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
#if LW_CFG_CPU_FPU_EN > 0
    if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {             /*  强制使用 FPU                */
        iregInterLevel = KN_INT_DISABLE();                              /*  关闭中断                    */
        __ARCH_FPU_ENABLE();                                            /*  使能 FPU                    */
        KN_INT_ENABLE(iregInterLevel);                                  /*  打开中断                    */
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
    
    LW_SOFUNC_PREPARE(pvThreadStartAddress);
    pvReturnVal = ((PTHREAD_START_ROUTINE)pvThreadStartAddress)
                  (ptcbCur->TCB_pvArg);                                 /*  执行线程                    */

    ulId = ptcbCur->TCB_ulId;

#if LW_CFG_THREAD_DEL_EN > 0
    API_ThreadForceDelete(&ulId, pvReturnVal);                          /*  删除线程                    */
#endif

#if LW_CFG_THREAD_SUSPEND_EN > 0
    API_ThreadSuspend(ulId);                                            /*  阻塞线程                    */
#endif

    for (;;) {
        API_TimeSleep(__ARCH_ULONG_MAX);                                /*  睡眠                        */
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: CoroutineCreate.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 06 月 19 日
**
** 描        述: 这是协程管理库(协程是一个轻量级的并发执行单位). 
                 在当前线程中创建一个协程.
** BUG:
2009.03.05  今天是学雷锋日, 雷锋永远是我们学习的榜样.
            将堆栈大小改为 ULONG 类型.
2009.04.06  将堆栈填充字节确定为 LW_CFG_STK_EMPTY_FLAG. 以提高堆栈检查的准确性.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
2013.12.14  使用任务自旋锁确保任务协程链表操作安全性.
2013.12.17  堆栈大小改为 size_t 类型.
2016.05.03  进程内的协程使用 vmm 堆栈.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_COROUTINE_EN > 0
/*********************************************************************************************************
** 函数名称: API_CoroutineCreate
** 功能描述: 在当前线程中创建一个协程.
** 输　入  : pCoroutineStartAddr       协程启动地址
**           stStackByteSize           堆栈大小
**           pvArg                     入口参数
** 输　出  : 协程控制句柄
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PVOID   API_CoroutineCreate (PCOROUTINE_START_ROUTINE pCoroutineStartAddr,
                             size_t                   stStackByteSize,
                             PVOID                    pvArg)
{
             INTREG                iregInterLevel;
             PLW_CLASS_TCB         ptcbCur;

    REGISTER PLW_STACK             pstkTop;
    REGISTER PLW_STACK             pstkButtom;
    REGISTER PLW_STACK             pstkLowAddress;
    REGISTER PLW_STACK             pstkFristFree;
    REGISTER size_t                stStackSizeWordAlign;                /*  堆栈大小(单位：字)          */
    
             PLW_CLASS_COROUTINE   pcrcbNew;

    if (!LW_SYS_STATUS_IS_RUNNING()) {                                  /*  系统必须已经启动            */
        _ErrorHandle(ERROR_KERNEL_NOT_RUNNING);
        return  (LW_NULL);
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_NULL);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pCoroutineStartAddr) {                                         /*  指协程代码段起始地址为空    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "coroutine code segment not found.\r\n");
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    if (_StackSizeCheck(stStackByteSize)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "coroutine stack size low.\r\n");
        _ErrorHandle(ERROR_THREAD_STACKSIZE_LACK);
        return  (LW_NULL);
    }
#endif

    _ThreadSafeInternal();                                              /*  进入安全模式                */

    pstkLowAddress = _StackAllocate(ptcbCur, 0ul, stStackByteSize);     /*  分配内存                    */
    if (!pstkLowAddress) {
        _ThreadUnsafeInternal();                                        /*  退出安全模式                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);
        return  (LW_NULL);
    }
    
    stStackSizeWordAlign = _CalWordAlign(stStackByteSize);              /*  计算对齐堆栈大小            */

#if CPU_STK_GROWTH == 0                                                 /*  寻找堆栈头尾                */
    pstkTop    = pstkLowAddress;
    pstkButtom = pstkLowAddress + stStackSizeWordAlign - 1;
    
    pcrcbNew   = (PLW_CLASS_COROUTINE)pstkTop;                          /*  寻找 CRCB 区域              */
    pstkTop    = (PLW_STACK)((BYTE *)pstkTop + __CRCB_SIZE_ALIGN + sizeof(LW_STACK));    
                                                                        /*  寻找主堆栈区                */
#else
    pstkTop    = pstkLowAddress + stStackSizeWordAlign - 1;
    pstkButtom = pstkLowAddress;
    
    pstkTop    = (PLW_STACK)((BYTE *)pstkTop - __CRCB_SIZE_ALIGN - sizeof(LW_STACK));
    pcrcbNew   = (PLW_CLASS_COROUTINE)((BYTE *)pstkTop + sizeof(LW_STACK));
#endif                                                                  /*  CPU_STK_GROWTH == 0         */
    
    if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_STK_CLR) {
        lib_memset((BYTE *)pstkLowAddress,                              /*  需要清除堆栈                */
                   LW_CFG_STK_EMPTY_FLAG, 
                   stStackByteSize);                                    /*  单位：字节                  */
    }
    
    pstkFristFree = archTaskCtxCreate((PTHREAD_START_ROUTINE)_CoroutineShell, 
                                      (PVOID)pCoroutineStartAddr,       /*  真正的可执行代码体          */
                                      pstkTop, 
                                      ptcbCur->TCB_ulOption);
    
    pcrcbNew->COROUTINE_pstkStackNow = pstkFristFree;                   /*  线程当前堆栈指针            */
    
    pcrcbNew->COROUTINE_pstkStackTop     = pstkTop;                     /*  线程主堆栈栈顶              */
    pcrcbNew->COROUTINE_pstkStackBottom  = pstkButtom;                  /*  线程主堆栈栈底              */
    pcrcbNew->COROUTINE_stStackSize      = stStackSizeWordAlign;        /*  线程堆栈大小(单位：字)      */
    pcrcbNew->COROUTINE_pstkStackLowAddr = pstkLowAddress;              /*  总堆栈最低地址              */
	
	pcrcbNew->COROUTINE_pvArg    = pvArg;
	pcrcbNew->COROUTINE_ulThread = ptcbCur->TCB_ulId;
	pcrcbNew->COROUTINE_ulFlags  = LW_COROUTINE_FLAG_DYNSTK;            /*  需要删除堆栈                */
	
    LW_SPIN_LOCK_QUICK(&ptcbCur->TCB_slLock, &iregInterLevel);
    _List_Ring_Add_Last(&pcrcbNew->COROUTINE_ringRoutine,
                        &ptcbCur->TCB_pringCoroutineHeader);            /*  插入协程表                  */
    LW_SPIN_UNLOCK_QUICK(&ptcbCur->TCB_slLock, iregInterLevel);
    
    _ThreadUnsafeInternal();                                            /*  退出安全模式                */
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_COROUTINE, MONITOR_EVENT_COROUTINE_CREATE, 
                      ptcbCur->TCB_ulId, pcrcbNew, stStackByteSize, LW_NULL);
    
    return  ((PVOID)pcrcbNew);                                          /*  创建成功                    */
}

#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/

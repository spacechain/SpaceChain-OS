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
** 文   件   名: _GlobalInit.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: 这是系统初始化函数库。

** BUG
2007.03.22  加入系统没有启动时的错误处理机制
2007.04.12  加入中断堆栈区基地址初始化
2007.04.12  清空中断堆栈
2007.06.06  最高中断嵌套层数清零
2008.01.20  取消对线程调度器锁的全局变量初始化.
2008.03.29  初始化分化出来的等待链表和看门狗链表的链表头.
2009.04.29  加入对 SMP 相关内核锁的初始化.
2009.10.12  加入对 CPU ID 的初始化.
2009.11.01  修正注释.
            10.31日, 我国伟大的科学家钱学森逝世, 享年98岁. 作者对钱老无比敬佩! 
            借用<士兵突击>中高诚的一句话追思钱老, "信念这东西, 还真不是说出来的, 是做出来的!". 
            也借此勉励自己.
2010.01.22  加入内核线程扫描链尾的初始化.
2010.08.03  加入 tick spinlock 初始化.
2012.07.04  合并 hook 初始化.
2012.09.11  _GlobalInit() 中加入对 FPU 的初始化.
2013.12.19  去掉 FPU 的初始化, 放在 bsp 内核初始化回调中进行, 用户需要指定 fpu 的名称.
2017.08.17  中断堆栈 ARCH_STK_ALIGN_SIZE 字节对齐, 确保不同体系架构安全.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __KERNEL_NCPUS_SET
#define  __KERNEL_MAIN_FILE                                             /*  这是系统主文件              */
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  中断堆栈定义
*********************************************************************************************************/
LW_STACK    _K_stkInterruptStack[LW_CFG_MAX_PROCESSORS][LW_CFG_INT_STK_SIZE / sizeof(LW_STACK)];
/*********************************************************************************************************
** 函数名称: __interPrimaryStackInit
** 功能描述: 初始化中断堆栈, (SylixOS 在 SMP 中每一个 CPU 都可以接受中断)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __interPrimaryStackInit (VOID)
{
    REGISTER INT            i;
             PLW_CLASS_CPU  pcpu;
             PLW_STACK      pstk;
    
    LW_SPIN_INIT(&_K_slVectorTable);
    
    for (i = 0; i < LW_CFG_MAX_PROCESSORS; i++) {
        pcpu = LW_CPU_GET(i);
#if CPU_STK_GROWTH == 0
        pstk = &_K_stkInterruptStack[i][0];
        pcpu->CPU_pstkInterBase = (PLW_STACK)ROUND_UP(pstk, ARCH_STK_ALIGN_SIZE);
#else
        pstk = &_K_stkInterruptStack[i][(LW_CFG_INT_STK_SIZE / sizeof(LW_STACK)) - 1];
        pcpu->CPU_pstkInterBase = (PLW_STACK)ROUND_DOWN(pstk, ARCH_STK_ALIGN_SIZE);
#endif                                                                  /*  CPU_STK_GROWTH              */
        lib_memset(_K_stkInterruptStack[i], LW_CFG_STK_EMPTY_FLAG, LW_CFG_INT_STK_SIZE);
    }
}
/*********************************************************************************************************
** 函数名称: __interSecondaryStackInit
** 功能描述: 初始化中断堆栈
** 输　入  : ulCPUId   CPU ID
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

static VOID  __interSecondaryStackInit (ULONG   ulCPUId)
{
    lib_memset(_K_stkInterruptStack[ulCPUId], LW_CFG_STK_EMPTY_FLAG, LW_CFG_INT_STK_SIZE);
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** 函数名称: __cpuPrimaryInit
** 功能描述: 操作系统 CPU 控制块结构初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __cpuPrimaryInit (VOID)
{
    REGISTER INT    i;
    
    for (i = 0; i < LW_CFG_MAX_PROCESSORS; i++) {
        LW_CPU_GET(i)->CPU_ulStatus = LW_CPU_STATUS_INACTIVE;           /*  CPU INACTIVE                */
        LW_SPIN_INIT(&_K_tcbDummy[i].TCB_slLock);                       /*  初始化自旋锁                */
        
#if LW_CFG_SMP_EN > 0
        LW_CPU_CLR_IPI_PEND(i, ((ULONG)~0));                            /*  清除所有中断标志            */
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    }
}
/*********************************************************************************************************
** 函数名称: __cpuSecondaryInit
** 功能描述: 操作系统 CPU 控制块结构初始化
** 输　入  : ulCPUId   CPU ID
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

static VOID  __cpuSecondaryInit (ULONG   ulCPUId)
{
    LW_CPU_GET(ulCPUId)->CPU_ulStatus = LW_CPU_STATUS_INACTIVE;         /*  CPU INACTIVE                */
    LW_SPIN_INIT(&_K_tcbDummy[ulCPUId].TCB_slLock);                     /*  初始化自旋锁                */
    
#if LW_CFG_SMP_EN > 0
    LW_CPU_CLR_IPI_PEND(ulCPUId, ((ULONG)~0));                          /*  清除所有中断标志            */
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** 函数名称: __miscPrimarySmpInit
** 功能描述: 与 SMP 有关的全局变量初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __miscPrimarySmpInit (VOID)
{
    REGISTER INT            i;
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)
             ULONG          ulMaxPhyId = 0;
#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
             PLW_CLASS_CPU  pcpu;
    
    for (i = 0; i < LW_CFG_MAX_PROCESSORS; i++) {
        pcpu = LW_CPU_GET(i);
        
        LW_CAND_TCB(pcpu) = LW_NULL;                                    /*  候选运行表为空              */
        LW_CAND_ROT(pcpu) = LW_FALSE;                                   /*  没有优先级卷绕              */
        
        pcpu->CPU_ulCPUId = (ULONG)i;
        pcpu->CPU_iKernelCounter = 1;                                   /*  初始化 1, 当前不允许调度    */

#if LW_CFG_SMP_EN > 0
        pcpu->CPU_ulIPIVector = __ARCH_ULONG_MAX;                       /*  目前不确定核间中断向量      */
        LW_SPIN_INIT(&pcpu->CPU_slIpi);                                 /*  初始化 CPU spinlock         */
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    }
    
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)
    LW_CPU_FOREACH (i) {
        pcpu = LW_CPU_GET(i);
        pcpu->CPU_ulPhyId = bspCpuLogic2Physical((ULONG)i);             /*  获得物理 CPU ID             */
        _BugFormat(pcpu->CPU_ulPhyId >= LW_NCPUS, LW_TRUE, 
                   "Physical CPU ID error: %lu\r\n", pcpu->CPU_ulPhyId);
        if (ulMaxPhyId < pcpu->CPU_ulPhyId) {
            ulMaxPhyId = pcpu->CPU_ulPhyId;
        }
    }
    _K_ulNPhyCpus = ulMaxPhyId + 1;                                     /*  统计物理 CPU 个数           */
#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
}
/*********************************************************************************************************
** 函数名称: __miscSecondarySmpInit
** 功能描述: 与 SMP 有关的全局变量初始化
** 输　入  : ulCPUId   CPU ID
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

static VOID  __miscSecondarySmpInit (ULONG   ulCPUId)
{
    PLW_CLASS_CPU  pcpu = LW_CPU_GET(ulCPUId);
    
    LW_CAND_TCB(pcpu) = LW_NULL;                                        /*  候选运行表为空              */
    LW_CAND_ROT(pcpu) = LW_FALSE;                                       /*  没有优先级卷绕              */
    
    pcpu->CPU_iKernelCounter = 1;                                       /*  初始化 1, 当前不允许调度    */
    pcpu->CPU_ulIPIVector    = __ARCH_ULONG_MAX;                        /*  目前不确定核间中断向量      */
    LW_SPIN_INIT(&pcpu->CPU_slIpi);                                     /*  初始化 CPU spinlock         */
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** 函数名称: _GlobalPrimaryInit
** 功能描述: 初始化零散全局变量
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID _GlobalPrimaryInit (VOID)
{
    LW_SYS_STATUS_SET(LW_SYS_STATUS_INIT);                              /*  系统状态为初始化状态        */

    /*
     *  内核关键性自旋锁初始化
     */
    LW_SPIN_INIT(&_K_klKernel.KERN_slLock);                             /*  初始化内核自旋锁            */
    LW_SPIN_INIT(&_K_slAtomic);                                         /*  初始化原子操作自旋锁        */

    /*
     *  内核关键性数据结构初始化
     */
    __cpuPrimaryInit();                                                 /*  CPU 结构初始化              */
    __interPrimaryStackInit();                                          /*  首先初始化中断堆栈          */
    __miscPrimarySmpInit();                                             /*  SMP 相关初始化              */
    
    /*
     *  内核关键性状态变量初始化
     */
    _K_i64KernelTime = 0;
    
#if LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0
    _K_ulCPUUsageTicks       = 1ul;                                     /*  避免除 0 错误               */
    _K_ulCPUUsageKernelTicks = 0ul;
#endif

    _K_usThreadCounter = 0;                                             /*  线程数量                    */
    _K_plineTCBHeader  = LW_NULL;                                       /*  TCB 管理链表头              */
    _K_ulNotRunError   = 0ul;                                           /*  系统未启动时错误存放变量    */
    
    __WAKEUP_INIT(&_K_wuDelay);
#if LW_CFG_SOFTWARE_WATCHDOG_EN > 0
    __WAKEUP_INIT(&_K_wuWatchDog);
#endif                                                                  /*  LW_CFG_SOFTWARE_WATCHDOG_EN */
    
#if LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0
    __LW_TICK_CPUUSAGE_DISABLE();                                       /*  关闭 CPU 利用率测试         */
#endif                                                                  /*  LW_CFG_THREAD_CPU_USAGE_... */
}
/*********************************************************************************************************
** 函数名称: _GlobalSecondaryInit
** 功能描述: 初始化零散全局变量
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

VOID _GlobalSecondaryInit (VOID)
{
    ULONG   ulCPUId = LW_CPU_GET_CUR_ID();
    
    __cpuSecondaryInit(ulCPUId);                                        /*  CPU 结构初始化              */
    __interSecondaryStackInit(ulCPUId);                                 /*  首先初始化中断堆栈          */
    __miscSecondarySmpInit(ulCPUId);                                    /*  SMP 相关初始化              */
}

#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/

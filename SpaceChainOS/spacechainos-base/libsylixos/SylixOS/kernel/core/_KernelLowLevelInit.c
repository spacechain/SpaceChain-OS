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
** 文   件   名: _KernelLowLevelInit.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 这是系统内核底层初始化函数库。

** BUG
2007.04.12  将全局变量初始化放在初始化最开始
2007.07.13  加入 _DebugHandle() 信息功能。
2007.11.08  将用户堆改为内核堆.
2008.01.24  修改了启动顺序.
2009.04.06  加入了堆栈检查初始化.
2014.11.12  内核底层初始化改为主从核分开.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _KernelPrimaryLowLevelInit
** 功能描述: 内核底层初始化
** 输　入  : pvKernelHeapMem   内核堆首地址
**           stKernelHeapSize  内核堆大小
**           pvSystemHeapMem   系统堆首地址
**           stSystemHeapSize  系统堆大小
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
VOID  _KernelPrimaryLowLevelInit (PVOID     pvKernelHeapMem,
                                  size_t    stKernelHeapSize,
                                  PVOID     pvSystemHeapMem,
                                  size_t    stSystemHeapSize)
#else
VOID  _KernelPrimaryLowLevelInit (VOID)
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */
{
    PLW_CLASS_CPU   pcpuCur;

    _DebugHandle(__LOGMESSAGE_LEVEL, "kernel low level initialize...\r\n");
    
    _GlobalPrimaryInit();                                               /*  全局变量初始化              */
    
    _ScheduleInit();                                                    /*  调度器初始化                */
    _StackCheckInit();                                                  /*  堆栈检查初始化              */
    _InterVectInit();                                                   /*  初始化中断向量表            */
    _ReadyTableInit();                                                  /*  就绪表初始化                */
    _ThreadIdInit();                                                    /*  Thread ID 初始化            */
    _EventInit();                                                       /*  事件初始化                  */
    _ThreadVarInit();                                                   /*  全局变量私有化模块初始化    */
    _TimerInit();                                                       /*  初始化定时器                */
    _TimeCvtInit();                                                     /*  初始化超时换算函数          */
    _PriorityInit();                                                    /*  初始化优先级控制块队列      */
    _EventSetInit();                                                    /*  初始化事件集                */
    _PartitionInit();                                                   /*  初始化定长内存管理          */
    _MsgQueueInit();                                                    /*  初始化消息队列              */
    _RmsInit();                                                         /*  初始化精度单调调度器        */
    _RtcInit();                                                         /*  RTC单元初始化               */
    _HeapInit();                                                        /*  堆初始化，变长内存管理      */
    
    /*
     *  下面的初始化可能会用到当前 CPU 正在执行的线程信息, 这里只是保证 CPU_ptcbTCBCur != NULL
     *  注意, 当前是关闭中断状态, 当前的 CPU ID 作为启动 CPU.
     */
    pcpuCur = LW_CPU_GET_CUR();
    pcpuCur->CPU_ptcbTCBCur = &_K_tcbDummy[pcpuCur->CPU_ulCPUId];       /*  伪内核线程                  */
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "kernel heap build...\r\n");
#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
    _HeapKernelInit(pvKernelHeapMem, stKernelHeapSize);
#else
    _HeapKernelInit();                                                  /*  建立内核堆                  */
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */

    _DebugHandle(__LOGMESSAGE_LEVEL, "system heap build...\r\n");
#if LW_CFG_MEMORY_HEAP_CONFIG_TYPE > 0
    _HeapSystemInit(pvSystemHeapMem, stSystemHeapSize);
#else
    _HeapSystemInit();                                                  /*  建立系统堆                  */
#endif                                                                  /*  LW_CFG_MEMORY_HEAP_...      */

    LW_KERNEL_JOB_INIT();                                               /*  初始化内核工作队列          */
}
/*********************************************************************************************************
** 函数名称: _KernelSecondaryLowLevelInit
** 功能描述: 内核底层初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

VOID  _KernelSecondaryLowLevelInit (VOID)
{
    PLW_CLASS_CPU   pcpuCur;

    _DebugHandle(__LOGMESSAGE_LEVEL, "kernel secondary low level initialize...\r\n");
    
    _GlobalSecondaryInit();                                             /*  全局变量初始化              */
    
    pcpuCur = LW_CPU_GET_CUR();
    pcpuCur->CPU_ptcbTCBCur = &_K_tcbDummy[pcpuCur->CPU_ulCPUId];       /*  伪内核线程                  */
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/

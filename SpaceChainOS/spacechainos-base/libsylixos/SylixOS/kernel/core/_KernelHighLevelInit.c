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
** 文   件   名: _KernelHighLevelInit.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 这是系统内核高层初始化函数库。

** BUG
2007.06.25  将系统线程名字改为 t_??? 的形式。
2007.07.13  加入 _DebugHandle() 信息功能。
2009.04.29  IDLE 线程必须使用 FIFO 调度, 以免出现时间片到期后没有任何就绪线程时, seekthread 返回 0 的错误
2009.12.31  加入对 posix 系统支持包的初始化
            初始化上电基准时间.
2009.01.07  将 posix 系统放在外部初始化.
2011.03.07  系统初始化有错误时, 打印相关错误说明.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  声明
*********************************************************************************************************/
extern INT  _SysInit(VOID);                                             /*  系统级初始化                */
#if LW_CFG_ISR_DEFER_EN > 0
extern VOID _interDeferInit(VOID);
#endif                                                                  /*  LW_CFG_ISR_DEFER_EN > 0     */
#if LW_CFG_MPI_EN > 0
extern VOID _mpiInit(VOID);                                             /*  MPI 系统初始化              */
#endif                                                                  /*  LW_CFG_MPI_EN               */
/*********************************************************************************************************
** 函数名称: _CreateIdleThread
** 功能描述: 内核高层初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 最先建立的一定要是 idle 线程, 为每一个 SMP CPU 都建立一个.
*********************************************************************************************************/
static VOID  _CreateIdleThread (VOID)
{
#if LW_CFG_SMP_EN > 0
    REGISTER INT             i;
             LW_CLASS_CPUSET cpuset;
#endif

    LW_CLASS_THREADATTR     threadattr;
    
    API_ThreadAttrBuild(&threadattr, 
                        LW_CFG_THREAD_IDLE_STK_SIZE, 
                        LW_PRIO_IDLE, 
                        (LW_OPTION_THREAD_STK_CHK | 
                         LW_OPTION_THREAD_SAFE | 
                         LW_OPTION_OBJECT_GLOBAL | 
                         LW_OPTION_THREAD_AFFINITY_ALWAYS), 
                        (PVOID)0);
                        
#if LW_CFG_SMP_EN > 0
    LW_CPU_ZERO(&cpuset);

    LW_CPU_FOREACH (i) {
        CHAR    cIdle[LW_CFG_OBJECT_NAME_SIZE] = "t_idle";
        
        lib_itoa(i, &cIdle[6], 10);
        API_ThreadAttrSetArg(&threadattr, (PVOID)(ULONG)i);
        _K_ulIdleId[i] = API_ThreadInit(cIdle, _IdleThread, &threadattr, LW_NULL);
        _K_ptcbIdle[i] = _K_ptcbTCBIdTable[_ObjectGetIndex(_K_ulIdleId[i])];
        _K_ptcbIdle[i]->TCB_ucSchedPolicy = LW_OPTION_SCHED_FIFO;       /* idle 必须是 FIFO 调度器      */
        
        LW_CPU_SET(i, &cpuset);                                         /*  锁定到指定 CPU              */
        _ThreadSetAffinity(_K_ptcbIdle[i], sizeof(LW_CLASS_CPUSET), &cpuset);
        LW_CPU_CLR(i, &cpuset);
        
        API_ThreadStart(_K_ulIdleId[i]);
    }

#else
    _K_ulIdleId[0] = API_ThreadInit("t_idle", _IdleThread, &threadattr, LW_NULL);
    _K_ptcbIdle[0] = _K_ptcbTCBIdTable[_ObjectGetIndex(_K_ulIdleId[0])];
    _K_ptcbIdle[0]->TCB_ucSchedPolicy = LW_OPTION_SCHED_FIFO;           /* idle 必须是 FIFO 调度器      */
    API_ThreadStart(_K_ulIdleId[0]);
#endif                                                                  /* LW_CFG_SMP_EN > 0            */
}
/*********************************************************************************************************
** 函数名称: _CreateITimerThread
** 功能描述: 内核高层初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _CreateITimerThread (VOID)
{
#if	(LW_CFG_ITIMER_EN > 0) && (LW_CFG_MAX_TIMERS > 0)
    LW_CLASS_THREADATTR       threadattr;

    API_ThreadAttrBuild(&threadattr, 
                        LW_CFG_THREAD_ITMR_STK_SIZE, 
                        LW_PRIO_T_ITIMER,
                        (LW_CFG_ITIMER_OPTION | LW_OPTION_THREAD_SAFE | LW_OPTION_OBJECT_GLOBAL), 
                        LW_NULL);

    _K_ulThreadItimerId = API_ThreadInit("t_itimer", _ITimerThread, &threadattr, LW_NULL);
   
    API_ThreadStart(_K_ulThreadItimerId);
#endif
}
/*********************************************************************************************************
** 函数名称: _KernelHighLevelInit
** 功能描述: 内核高层初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _KernelHighLevelInit (VOID)
{
    VOID  _HookListInit(VOID);
    
#if LW_CFG_SIGNAL_EN > 0
    VOID  _signalInit(VOID);
#endif                                                                  /*  LW_CFG_SIGNAL_EN            */
    
    REGISTER INT    iErr;
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "kernel high level initialize...\r\n");
    
    _HookListInit();                                                    /*  系统动态 HOOK 初始化        */

#if LW_CFG_SIGNAL_EN > 0
    _signalInit();                                                      /*  信号初始化                  */
#endif                                                                  /*  LW_CFG_SIGNAL_EN            */

    _CreateIdleThread();                                                /*  建立空闲任务                */
    _CreateITimerThread();                                              /*  ITIMER 任务建立             */
    
#if LW_CFG_ISR_DEFER_EN > 0
    _interDeferInit();
#endif                                                                  /*  LW_CFG_ISR_DEFER_EN > 0     */
    
    iErr = _SysInit();
    if (iErr != ERROR_NONE) {
        _DebugHandle(__LOGMESSAGE_LEVEL, "system initialized error!\r\n");
    
    } else {
        _DebugHandle(__LOGMESSAGE_LEVEL, "system initialized.\r\n");
    }

#if LW_CFG_MPI_EN > 0
    _mpiInit();
#endif                                                                  /*  LW_CFG_MPI_EN               */
    
    bspKernelInitHook();
    __LW_KERNEL_INIT_END_HOOK(iErr);                                    /*  入口参数为系统初始化结果    */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: RmsPeriod.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 11 日
**
** 描        述: 指定精度单调调度器开始按固定周期工作

** BUG
2007.11.04  加入 _DebugHandle() 功能.
2008.01.20  改进调度器全局锁为本地锁.
2008.03.29  加入新的 wake up 机制的处理.
2008.03.30  使用新的就绪环操作.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2008.05.31  使用 __KERNEL_MODE_...().
2010.08.03  支持 SMP 多核.
2012.03.20  减少对 _K_ptcbTCBCur 的引用, 尽量采用局部变量, 减少对当前 CPU ID 获取的次数.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  注意：
        使用精度单调调度器，不可以设置系统时间，但是可是设置 RTC 时间。
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: API_RmsPeriod
** 功能描述: 指定精度单调调度器开始按固定周期工作
** 输　入  : 
**           ulId                          RMS 句柄
**           ulPeriod                      程序段执行周期
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)

LW_API  
ULONG  API_RmsPeriod (LW_OBJECT_HANDLE  ulId, ULONG  ulPeriod)
{
             INTREG                    iregInterLevel;
             
             PLW_CLASS_TCB             ptcbCur;
	REGISTER PLW_CLASS_PCB             ppcb;

    REGISTER PLW_CLASS_RMS             prms;
    REGISTER UINT16                    usIndex;
    
    REGISTER ULONG                     ulErrorCode;
             ULONG                     ulWaitTime;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!ulPeriod) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulPeriod invalidate.\r\n");
        _ErrorHandle(ERROR_RMS_TICK);
        return  (ERROR_RMS_TICK);
    }
    
    if (!_ObjectClassOK(ulId, _OBJECT_RMS)) {                           /*  对象类型检查                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "RMS handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Rms_Index_Invalid(usIndex)) {                                  /*  缓冲区索引检查              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "RMS handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核                    */
    if (_Rms_Type_Invalid(usIndex)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "RMS handle invalidate.\r\n");
        _ErrorHandle(ERROR_RMS_NULL);
        return  (ERROR_RMS_NULL);
    }
#else
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核                    */
#endif

    prms = &_K_rmsBuffer[usIndex];
    
    switch (prms->RMS_ucStatus) {                                       /*  状态机                      */
    
    case LW_RMS_INACTIVE:
        _RmsActive(prms);                                               /*  启动 RMS                    */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        return  (ERROR_NONE);
        
    case LW_RMS_ACTIVE:                                                 /*  已将初始化完成              */
        ulErrorCode = _RmsInitExpire(prms, ulPeriod, &ulWaitTime);      /*  初始化“到期”数据          */
        if (ulErrorCode) {                                              /*  发生错误                    */
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核                    */
            _ErrorHandle(ulErrorCode);
            return  (ulErrorCode);
        }
        
        if (!ulWaitTime) {                                              /*  时间刚刚好？                */
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核                    */
            return  (ERROR_NONE);
        }
        
        /*
         *  当前线程开始睡眠
         */
        ppcb = _GetPcb(ptcbCur);
        
        __DEL_FROM_READY_RING(ptcbCur, ppcb);                           /*  从就绪表中删除              */

        ptcbCur->TCB_ulDelay = ulWaitTime;
        __ADD_TO_WAKEUP_LINE(ptcbCur);                                  /*  加入等待扫描链              */
        
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核, 产生调度          */
        
        /*
         *  当前线程睡眠结束
         */
         __KERNEL_MODE_PROC(
            ulErrorCode = _RmsEndExpire(prms);                          /*  处理到期的 RMS              */
         );

        _ErrorHandle(ulErrorCode);
        return  (ulErrorCode);
        
    default:                                                            /*  转态机错误                  */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _ErrorHandle(ERROR_RMS_NULL);
        return  (ERROR_RMS_NULL);
    }
}

#endif                                                                  /*  (LW_CFG_RMS_EN > 0)         */
                                                                        /*  (LW_CFG_MAX_RMSS > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

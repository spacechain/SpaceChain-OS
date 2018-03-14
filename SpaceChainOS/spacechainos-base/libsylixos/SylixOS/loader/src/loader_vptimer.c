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
** 文   件   名: loader_vptimer.c
**
** 创   建   人: Han.hui (韩辉)
**
** 文件创建日期: 2011 年 08 月 03 日
**
** 描        述: 进程定时器支持.
**
** BUG:
2017.05.27  避免 vprocItimerHook() 在一次 tick 服务中, 在多核上多次计算同一个进程的情况.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "sys/time.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if (LW_CFG_MODULELOADER_EN > 0) && (LW_CFG_PTIMER_EN > 0)
#include "../include/loader_lib.h"
/*********************************************************************************************************
  进程列表
*********************************************************************************************************/
extern LW_LIST_LINE_HEADER      _G_plineVProcHeader;
/*********************************************************************************************************
** 函数名称: vprocItimerMainHook
** 功能描述: 进程定时器 tick hook (中断上下文中且锁定内核状态被调用, 每次 tick 中断仅调用一次)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  vprocItimerMainHook (VOID)
{
    LW_LD_VPROC     *pvproc;
    LW_LD_VPROC_T   *pvptimer;
    LW_LIST_LINE    *plineTemp;
    struct sigevent  sigeventTimer;
    struct siginfo   siginfoTimer;
    
    sigeventTimer.sigev_signo           = SIGALRM;
    sigeventTimer.sigev_notify          = SIGEV_SIGNAL;
    sigeventTimer.sigev_value.sival_ptr = LW_NULL;
    
    siginfoTimer.si_errno   = ERROR_NONE;
    siginfoTimer.si_code    = SI_TIMER;
    siginfoTimer.si_signo   = SIGALRM;
    siginfoTimer.si_timerid = ITIMER_REAL;
    
    for (plineTemp  = _G_plineVProcHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ITIMER_REAL 定时器          */
         
        pvproc = _LIST_ENTRY(plineTemp, LW_LD_VPROC, VP_lineManage);
        if (pvproc == &_G_vprocKernel) {
            continue;
        }
        
        pvptimer = &pvproc->VP_vptimer[ITIMER_REAL];
        if (pvptimer->VPT_ulCounter) {
            pvptimer->VPT_ulCounter--;
            if (pvptimer->VPT_ulCounter == 0ul) {
                _doSigEventEx(pvproc->VP_ulMainThread, &sigeventTimer, &siginfoTimer);
                pvptimer->VPT_ulCounter = pvptimer->VPT_ulInterval;
            }
        }
    }
}
/*********************************************************************************************************
** 函数名称: vprocItimerEachHook
** 功能描述: 进程定时器 tick hook (中断上下文中且锁定内核状态被调用, 需要遍历 CPU)
** 输　入  : pcpu    对应 CPU 控制块
**           pvproc  进程控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  vprocItimerEachHook (PLW_CLASS_CPU  pcpu, LW_LD_VPROC  *pvproc)
{
    LW_LD_VPROC_T   *pvptimer;
    struct sigevent  sigeventTimer;
    struct siginfo   siginfoTimer;
    LW_OBJECT_HANDLE ulThreadId;
    
    sigeventTimer.sigev_notify          = SIGEV_SIGNAL;
    sigeventTimer.sigev_value.sival_ptr = LW_NULL;
    
    siginfoTimer.si_errno = ERROR_NONE;
    siginfoTimer.si_code  = SI_TIMER;
    
    ulThreadId = pvproc->VP_ulMainThread;                               /*  主线程                      */
    
    if (pcpu->CPU_iKernelCounter == 0) {                                /*  ITIMER_VIRTUAL 定时器       */
        pvptimer = &pvproc->VP_vptimer[ITIMER_VIRTUAL];
        if (pvptimer->VPT_ulCounter) {
            pvptimer->VPT_ulCounter--;
            if (pvptimer->VPT_ulCounter == 0ul) {
                sigeventTimer.sigev_signo = SIGVTALRM;
                siginfoTimer.si_signo     = SIGVTALRM;
                siginfoTimer.si_timerid   = ITIMER_VIRTUAL;
                _doSigEventEx(ulThreadId, &sigeventTimer, &siginfoTimer);
                pvptimer->VPT_ulCounter = pvptimer->VPT_ulInterval;
            }
        }
    }
    
    pvptimer = &pvproc->VP_vptimer[ITIMER_PROF];
    if (pvptimer->VPT_ulCounter) {                                      /*  ITIMER_PROF 定时器          */
        pvptimer->VPT_ulCounter--;
        if (pvptimer->VPT_ulCounter == 0ul) {
            sigeventTimer.sigev_signo = SIGPROF;
            siginfoTimer.si_signo     = SIGPROF;
            siginfoTimer.si_timerid   = ITIMER_PROF;
            _doSigEventEx(ulThreadId, &sigeventTimer, &siginfoTimer);
            pvptimer->VPT_ulCounter = pvptimer->VPT_ulInterval;
        }
    }
}
/*********************************************************************************************************
** 函数名称: vprocSetitimer
** 功能描述: 设置进程内部定时器
** 输　入  : iWhich        类型, ITIMER_REAL / ITIMER_VIRTUAL / ITIMER_PROF
**           pitValue      定时参数
**           pitOld        当前参数
** 输　出  : PX_ERROR or ERROR_NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  vprocSetitimer (INT        iWhich, 
                     ULONG      ulCounter,
                     ULONG      ulInterval,
                     ULONG     *pulCounter,
                     ULONG     *pulInterval)
{
    INTREG           iregInterLevel;
    LW_LD_VPROC     *pvproc;
    LW_LD_VPROC_T   *pvptimer;
    PLW_CLASS_TCB    ptcbCur;

    if (LW_KERN_NO_ITIMER_EN_GET()) {
        _PrintHandle("Warning: no itimer support (kernel start parameter: noitmr=yes)!\r\n");
    }

    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pvproc = __LW_VP_GET_TCB_PROC(ptcbCur);
    if (pvproc == LW_NULL) {                                            /*  仅对进程有效                */
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    
    pvptimer = &pvproc->VP_vptimer[iWhich];
    
    LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);
    if (pulCounter) {
        *pulCounter = pvptimer->VPT_ulCounter;
    }
    if (pulInterval) {
        *pulInterval = pvptimer->VPT_ulInterval;
    }
    pvptimer->VPT_ulCounter  = ulCounter;
    pvptimer->VPT_ulInterval = ulInterval;
    LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: vprocGetitimer
** 功能描述: 获取进程内部定时器
** 输　入  : iWhich        类型, ITIMER_REAL / ITIMER_VIRTUAL / ITIMER_PROF
**           pitValue      获取当前定时信息
** 输　出  : PX_ERROR or ERROR_NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  vprocGetitimer (INT        iWhich, 
                     ULONG     *pulCounter,
                     ULONG     *pulInterval)
{
    INTREG           iregInterLevel;
    LW_LD_VPROC     *pvproc;
    LW_LD_VPROC_T   *pvptimer;
    PLW_CLASS_TCB    ptcbCur;

    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pvproc = __LW_VP_GET_TCB_PROC(ptcbCur);
    if (pvproc == LW_NULL) {                                            /*  仅对进程有效                */
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    
    pvptimer = &pvproc->VP_vptimer[iWhich];
    
    LW_SPIN_KERN_LOCK_QUICK(&iregInterLevel);
    if (pulCounter) {
        *pulCounter = pvptimer->VPT_ulCounter;
    }
    if (pulInterval) {
        *pulInterval = pvptimer->VPT_ulInterval;
    }
    LW_SPIN_KERN_UNLOCK_QUICK(iregInterLevel);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
                                                                        /*  LW_CFG_PTIMER_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: EventSetGet.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 20 日
**
** 描        述: 等待事件集相关事件.

** BUG
2007.10.28  当调度器被锁定时，不能完成等待.
2008.01.13  加入 _DebugHandle() 功能。
2008.01.20  调度器已经有了重大的改进, 可以在调度器被锁定的情况下调用此 API.
2008.05.03  加入信号令调度器返回 restart 进行重新阻塞的处理.
2009.04.08  加入对 SMP 多核的支持.
2010.08.03  修改获取系统时钟的方式.
2011.02.23  加入 LW_OPTION_SIGNAL_INTER 选项, 事件可以选择自己是否可被中断打断.
2013.05.05  判断调度器返回值, 决定是重启调用还是退出.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
2014.06.04  加入 API_EventSetGetEx.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  MACRO
*********************************************************************************************************/
#define  __EVENTSET_NOT_READY() do {                          \
             __KERNEL_EXIT_IRQ(iregInterLevel);               \
             _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);         \
             return (ERROR_THREAD_WAIT_TIMEOUT);              \
         } while (0)
         
#define  __EVENTSET_SAVE_RET()  do {                          \
             if (ulOption & LW_OPTION_EVENTSET_RETURN_ALL) {  \
                 if (pulEvent) {                              \
                     *pulEvent = pes->EVENTSET_ulEventSets;   \
                 }                                            \
             } else {                                         \
                 if (pulEvent) {                              \
                     *pulEvent = ulEventRdy;                  \
                 }                                            \
             }                                                \
         } while (0)
/*********************************************************************************************************
** 函数名称: API_EventSetGetEx
** 功能描述: 等待事件集相关事件
** 输　入  : 
**           ulId            事件集句柄
**           ulEvent         等待事件
**           ulOption        等待方法选项
**           ulTimeout       等待时间
**           pulEvent        接收到的事件
** 输　出  : 事件句柄
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)

LW_API  
ULONG  API_EventSetGetEx (LW_OBJECT_HANDLE  ulId, 
                          ULONG             ulEvent,
                          ULONG             ulOption,
                          ULONG             ulTimeout,
                          ULONG            *pulEvent)
{
    LW_CLASS_EVENTSETNODE          esnNode;
    
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENTSET    pes;
    REGISTER UINT8                 ucWaitType;
    REGISTER ULONG                 ulEventRdy;
    REGISTER ULONG                 ulWaitTime;
             ULONG                 ulTimeSave;                          /*  系统事件记录                */
             INT                   iSchedRet;
             
             ULONG                 ulEventSetOption;                    /*  事件创建选项                */
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  当前任务控制块              */
    
    ucWaitType = (UINT8)(ulOption & 0x0F);                              /*  获得等待类型                */
    
__wait_again:
    if (ulTimeout == LW_OPTION_WAIT_INFINITE) {
        ulWaitTime = 0ul;
    } else {
        ulWaitTime = ulTimeout;
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_EVENT_SET)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "eventset handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_EventSet_Index_Invalid(usIndex)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "eventset handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif
    pes = &_K_esBuffer[usIndex];
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核                    */
    if (_EventSet_Type_Invalid(usIndex, LW_TYPE_EVENT_EVENTSET)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "eventset handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENTSET_TYPE);
        return  (ERROR_EVENTSET_TYPE);
    }

    switch (ucWaitType) {
    
    case LW_OPTION_EVENTSET_WAIT_SET_ALL:                               /*  全部置位                    */
        ulEventRdy = (ulEvent & pes->EVENTSET_ulEventSets);
        if (ulEvent == ulEventRdy) {                                    /*  事件都已经存在              */
            __EVENTSET_SAVE_RET();
            if (ulOption & LW_OPTION_EVENTSET_RESET) {
                pes->EVENTSET_ulEventSets &= (~ulEventRdy);
            } else if (ulOption & LW_OPTION_EVENTSET_RESET_ALL) {
                pes->EVENTSET_ulEventSets = 0ul;
            }
            ptcbCur->TCB_ulEventSets = ulEventRdy;
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核                    */
            return  (ERROR_NONE);
            
        } else {
            if (ulTimeout == LW_OPTION_NOT_WAIT) {                      /*  不等待                      */
                __EVENTSET_NOT_READY();
            }                                                           /*  阻塞线程                    */
            __KERNEL_TIME_GET_NO_SPINLOCK(ulTimeSave, ULONG);           /*  记录系统时间                */
            _EventSetBlock(pes, &esnNode, ulEvent, ucWaitType, ulWaitTime);
        }
        break;
    
    case LW_OPTION_EVENTSET_WAIT_SET_ANY:
        ulEventRdy = (ulEvent & pes->EVENTSET_ulEventSets);
        if (ulEventRdy) {
            __EVENTSET_SAVE_RET();
            if (ulOption & LW_OPTION_EVENTSET_RESET) {
                pes->EVENTSET_ulEventSets &= (~ulEventRdy);
            } else if (ulOption & LW_OPTION_EVENTSET_RESET_ALL) {
                pes->EVENTSET_ulEventSets = 0ul;
            }
            ptcbCur->TCB_ulEventSets = ulEventRdy;
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核                    */
            return  (ERROR_NONE);
        
        } else {
            if (ulTimeout == LW_OPTION_NOT_WAIT) {                      /*  不等待                      */
                __EVENTSET_NOT_READY();
            }                                                           /*  阻塞线程                    */
            __KERNEL_TIME_GET_NO_SPINLOCK(ulTimeSave, ULONG);           /*  记录系统时间                */
            _EventSetBlock(pes, &esnNode, ulEvent, ucWaitType, ulWaitTime);
        }
        break;
        
    case LW_OPTION_EVENTSET_WAIT_CLR_ALL:
        ulEventRdy = (ulEvent & ~pes->EVENTSET_ulEventSets);
        if (ulEvent == ulEventRdy) {
            __EVENTSET_SAVE_RET();
            if (ulOption & LW_OPTION_EVENTSET_RESET) {
                pes->EVENTSET_ulEventSets |= ulEventRdy;
            } else if (ulOption & LW_OPTION_EVENTSET_RESET_ALL) {
                pes->EVENTSET_ulEventSets  = __ARCH_ULONG_MAX;
            }
            ptcbCur->TCB_ulEventSets = ulEventRdy;
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核                    */
            return  (ERROR_NONE);
        
        } else {
            if (ulTimeout == LW_OPTION_NOT_WAIT) {                      /*  不等待                      */
                __EVENTSET_NOT_READY();
            }                                                           /*  阻塞线程                    */
            __KERNEL_TIME_GET_NO_SPINLOCK(ulTimeSave, ULONG);           /*  记录系统时间                */
            _EventSetBlock(pes, &esnNode, ulEvent, ucWaitType, ulWaitTime);
        }
        break;
    
    case LW_OPTION_EVENTSET_WAIT_CLR_ANY:
        ulEventRdy = (ulEvent & ~pes->EVENTSET_ulEventSets);
        if (ulEventRdy) {
            __EVENTSET_SAVE_RET();
            if (ulOption & LW_OPTION_EVENTSET_RESET) {
                pes->EVENTSET_ulEventSets |= ulEventRdy;
            } else if (ulOption & LW_OPTION_EVENTSET_RESET_ALL) {
                pes->EVENTSET_ulEventSets  = __ARCH_ULONG_MAX;
            }
            ptcbCur->TCB_ulEventSets = ulEventRdy;
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核                    */
            return  (ERROR_NONE);
        
        } else {
            if (ulTimeout == LW_OPTION_NOT_WAIT) {                      /*  不等待                      */
                __EVENTSET_NOT_READY();
            }                                                           /*  阻塞线程                    */
            __KERNEL_TIME_GET_NO_SPINLOCK(ulTimeSave, ULONG);           /*  记录系统时间                */
            _EventSetBlock(pes, &esnNode, ulEvent, ucWaitType, ulWaitTime);
        }
        break;
    
    default:                                                            /*  方法出错                    */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _ErrorHandle(ERROR_EVENTSET_WAIT_TYPE);
        return  (ERROR_EVENTSET_WAIT_TYPE);
    }
    
    KN_INT_ENABLE(iregInterLevel);                                      /*  使能中断                    */
    
    ulEventSetOption = pes->EVENTSET_ulOption;
    
    MONITOR_EVT_LONG4(MONITOR_EVENT_ID_ESET, MONITOR_EVENT_ESET_PEND, 
                      ulId, ulEvent, ulOption, ulTimeout, LW_NULL);
    
    iSchedRet = __KERNEL_EXIT();                                        /*  调度器解锁                  */
    if (iSchedRet) {
        if ((iSchedRet == LW_SIGNAL_EINTR) && 
            (ulEventSetOption & LW_OPTION_SIGNAL_INTER)) {
            _ErrorHandle(EINTR);
            return  (EINTR);
        }
        ulTimeout = _sigTimeoutRecalc(ulTimeSave, ulTimeout);           /*  重新计算超时时间            */
        if (ulTimeout == LW_OPTION_NOT_WAIT) {
            _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);
            return  (ERROR_THREAD_WAIT_TIMEOUT);
        }
        goto    __wait_again;
    }
    
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  进入内核                    */
    if (ptcbCur->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) {               /*  等待超时                    */
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);
        return  (ERROR_THREAD_WAIT_TIMEOUT);
    
    } else {
        ulEventRdy = ptcbCur->TCB_ulEventSets;                          /*  获得相关位                  */
        
        if (ptcbCur->TCB_ucIsEventDelete == LW_EVENT_EXIST) {           /*  事件是否存在                */
            __EVENTSET_SAVE_RET();
            if ((ulOption & LW_OPTION_EVENTSET_RESET) ||
                (ulOption & LW_OPTION_EVENTSET_RESET_ALL)) {
                switch (ucWaitType) {
                
                case LW_OPTION_EVENTSET_WAIT_SET_ALL:
                case LW_OPTION_EVENTSET_WAIT_SET_ANY:
                    if (ulOption & LW_OPTION_EVENTSET_RESET) {
                        pes->EVENTSET_ulEventSets &= (~ulEventRdy);
                    } else if (ulOption & LW_OPTION_EVENTSET_RESET_ALL) {
                        pes->EVENTSET_ulEventSets = 0ul;
                    }
                    break;
                    
                case LW_OPTION_EVENTSET_WAIT_CLR_ALL:
                case LW_OPTION_EVENTSET_WAIT_CLR_ANY:
                    if (ulOption & LW_OPTION_EVENTSET_RESET) {
                        pes->EVENTSET_ulEventSets |= ulEventRdy;
                    } else if (ulOption & LW_OPTION_EVENTSET_RESET_ALL) {
                        pes->EVENTSET_ulEventSets  = __ARCH_ULONG_MAX;
                    }
                    break;
                }
            }
            
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核                    */
            return  (ERROR_NONE);
        
        } else {
            __KERNEL_EXIT_IRQ(iregInterLevel);                          /*  退出内核                    */
            _ErrorHandle(ERROR_EVENTSET_WAS_DELETED);                   /*  已经被删除                  */
            return  (ERROR_EVENTSET_WAS_DELETED);
        }
    }
}
/*********************************************************************************************************
** 函数名称: API_EventSetGet
** 功能描述: 等待事件集相关事件
** 输　入  : 
**           ulId            事件集句柄
**           ulEvent         等待事件
**           ulOption        等待方法选项
**           ulTimeout       等待时间
** 输　出  : 事件句柄
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
ULONG  API_EventSetGet (LW_OBJECT_HANDLE  ulId, 
                        ULONG             ulEvent,
                        ULONG             ulOption,
                        ULONG             ulTimeout)
{
    return  (API_EventSetGetEx(ulId, ulEvent, ulOption, ulTimeout, LW_NULL));
}

#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

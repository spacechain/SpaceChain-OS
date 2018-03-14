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
** 文   件   名: EventSetSet.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 20 日
**
** 描        述: 释放事件集相关事件.

** BUG
2007.11.13  使用链表库对链表操作进行完全封装.
2007.11.18  整理注释.
2008.01.13  加入 _DebugHandle() 功能。
2009.04.08  加入对 SMP 多核的支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_EventSetSet
** 功能描述: 释放事件集相关事件
** 输　入  : 
**           ulId            事件集句柄
**           ulEvent         释放事件
**           ulOption        释放方法选项
** 输　出  : 事件句柄
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)

LW_API  
ULONG  API_EventSetSet (LW_OBJECT_HANDLE  ulId, 
                        ULONG             ulEvent,
                        ULONG             ulOption)
{
             INTREG                    iregInterLevel;
    REGISTER UINT16                    usIndex;
    REGISTER PLW_CLASS_EVENTSET        pes;
    REGISTER PLW_CLASS_EVENTSETNODE    pesn;
             PLW_LIST_LINE             plineList;
    REGISTER ULONG                     ulEventRdy;
    
    usIndex = _ObjectGetIndex(ulId);
    
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

    switch (ulOption) {
    
    case LW_OPTION_EVENTSET_CLR:                                        /*  清除操作                    */
        pes->EVENTSET_ulEventSets &= (~ulEvent);
        break;
    
    case LW_OPTION_EVENTSET_SET:                                        /*  置位操作                    */
        pes->EVENTSET_ulEventSets |= ulEvent;
        break;
    
    default:
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulOption invalidate.\r\n");
        _ErrorHandle(ERROR_EVENTSET_OPTION);
        return  (ERROR_EVENTSET_OPTION);
    }
    
    for (plineList  = pes->EVENTSET_plineWaitList;                      /*  表头指针                    */
         plineList != LW_NULL;
         plineList  = _list_line_get_next(plineList)) {                 /*  释放所有有效线程            */
         
        pesn = _LIST_ENTRY(plineList, LW_CLASS_EVENTSETNODE, EVENTSETNODE_lineManage);
        
        switch (pesn->EVENTSETNODE_ucWaitType) {
        
        /*
         *  所有位均需满足条件
         */
        case LW_OPTION_EVENTSET_WAIT_SET_ALL:
            ulEventRdy = (ULONG)(pesn->EVENTSETNODE_ulEventSets & pes->EVENTSET_ulEventSets);
            if (ulEventRdy == pesn->EVENTSETNODE_ulEventSets) {
                MONITOR_EVT_LONG4(MONITOR_EVENT_ID_ESET, MONITOR_EVENT_ESET_POST, 
                                  ulId, ((PLW_CLASS_TCB)pesn->EVENTSETNODE_ptcbMe)->TCB_ulId, 
                                  ulEvent, ulOption, LW_NULL);
                _EventSetThreadReady(pesn, ulEventRdy);
            }
            break;
            
        case LW_OPTION_EVENTSET_WAIT_CLR_ALL:
            ulEventRdy = (ULONG)(pesn->EVENTSETNODE_ulEventSets & ~pes->EVENTSET_ulEventSets);
            if (ulEventRdy == pesn->EVENTSETNODE_ulEventSets) {
                MONITOR_EVT_LONG4(MONITOR_EVENT_ID_ESET, MONITOR_EVENT_ESET_POST, 
                                  ulId, ((PLW_CLASS_TCB)pesn->EVENTSETNODE_ptcbMe)->TCB_ulId, 
                                  ulEvent, ulOption, LW_NULL);
                _EventSetThreadReady(pesn, ulEventRdy);
            }
            break;

        /*
         *  任意位满足条件
         */
        case LW_OPTION_EVENTSET_WAIT_SET_ANY:
            ulEventRdy = (ULONG)(pesn->EVENTSETNODE_ulEventSets & pes->EVENTSET_ulEventSets);
            if (ulEventRdy) {
                MONITOR_EVT_LONG4(MONITOR_EVENT_ID_ESET, MONITOR_EVENT_ESET_POST, 
                                  ulId, ((PLW_CLASS_TCB)pesn->EVENTSETNODE_ptcbMe)->TCB_ulId, 
                                  ulEvent, ulOption, LW_NULL);
                _EventSetThreadReady(pesn, ulEventRdy);
            }
            break;
            
        case LW_OPTION_EVENTSET_WAIT_CLR_ANY:
            ulEventRdy = (ULONG)(pesn->EVENTSETNODE_ulEventSets & ~pes->EVENTSET_ulEventSets);
            if (ulEventRdy) {
                MONITOR_EVT_LONG4(MONITOR_EVENT_ID_ESET, MONITOR_EVENT_ESET_POST, 
                                  ulId, ((PLW_CLASS_TCB)pesn->EVENTSETNODE_ptcbMe)->TCB_ulId, 
                                  ulEvent, ulOption, LW_NULL);
                _EventSetThreadReady(pesn, ulEventRdy);
            }
            break;
        }
    }
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

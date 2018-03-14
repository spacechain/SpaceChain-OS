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
** 文   件   名: EventSetDelete.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 20 日
**
** 描        述: 删除事件集

** BUG
2007.11.13  使用链表库对链表操作进行完全封装.
2007.11.18  整理注释.
2008.01.13  加入 _DebugHandle() 功能。
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock(); 
2008.05.31  使用 __KERNEL_MODE_...().
2009.04.08  加入对 SMP 多核的支持.
2011.07.29  加入对象创建/销毁回调.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_EventSetDelete
** 功能描述: 删除事件集
** 输　入  : 
**           pulId                         事件句柄指针
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)

LW_API  
ULONG  API_EventSetDelete (LW_OBJECT_HANDLE  *pulId)
{
             INTREG                    iregInterLevel;
    REGISTER UINT16                    usIndex;
    REGISTER PLW_CLASS_EVENTSET        pes;
    REGISTER PLW_CLASS_EVENTSETNODE    pesn;
             PLW_LIST_LINE             plineList;
    REGISTER LW_OBJECT_HANDLE          ulId;
    
    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
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
    
    _ObjectCloseId(pulId);                                              /*  清除句柄                    */
    
    for (plineList  = pes->EVENTSET_plineWaitList;                      /*  表头指针                    */
         plineList != LW_NULL;
         plineList  = _list_line_get_next(plineList)) {                 /*  释放所有有效线程            */
                  
        pesn = _LIST_ENTRY(plineList, LW_CLASS_EVENTSETNODE, EVENTSETNODE_lineManage);
        _EventSetDeleteReady(pesn);
    }
    
    pes->EVENTSET_ucType        = LW_TYPE_EVENT_UNUSED;
    pes->EVENTSET_plineWaitList = LW_NULL;
    
    _Free_EventSet_Object(pes);                                         /*  交还缓冲区                  */
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核                    */
    
    __LW_OBJECT_DELETE_HOOK(ulId);
    
    MONITOR_EVT_LONG1(MONITOR_EVENT_ID_ESET, MONITOR_EVENT_ESET_DELETE, ulId, LW_NULL);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "eventset \"%s\" has been delete.\r\n", pes->EVENTSET_cEventSetName);

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

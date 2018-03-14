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
** 文   件   名: MsgQueueCreate.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 06 日
**
** 描        述: 建立消息队列

** BUG
2007.09.19  加入 _DebugHandle() 功能。
2007.11.18  整理注释.
2008.01.20  内核缺少内存时, Debug 信息错误.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2008.05.31  使用 __KERNEL_MODE_...().
2009.04.08  加入对 SMP 多核的支持.
2009.07.28  自旋锁的初始化放在初始化所有的控制块中, 这里去除相关操作.
2011.07.29  加入对象创建/销毁回调.
2014.05.31  使用 ROUND_UP 代替除法.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_MsgQueueCreate
** 功能描述: 建立消息队列
** 输　入  : 
**           pcName                 消息队列名缓冲区
**           ulMaxMsgCounter        最大消息数量
**           stMaxMsgByteSize       每条消息最大长度
**           ulOption               消息队列选项
**           pulId                  消息队列ID指针
** 输　出  : 消息队列句柄
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)

LW_API  
LW_OBJECT_HANDLE  API_MsgQueueCreate (CPCHAR             pcName,
                                      ULONG              ulMaxMsgCounter,
                                      size_t             stMaxMsgByteSize,
                                      ULONG              ulOption,
                                      LW_OBJECT_ID      *pulId)
{
    REGISTER PLW_CLASS_MSGQUEUE    pmsgqueue;
    REGISTER PLW_CLASS_EVENT       pevent;
    REGISTER ULONG                 ulI;
             PLW_CLASS_WAITQUEUE   pwqTemp[2];
    REGISTER ULONG                 ulIdTemp;
    
    REGISTER size_t                stMaxMsgByteSizeReal;
    REGISTER size_t                stHeapAllocateByteSize;
    REGISTER PVOID                 pvMemAllocate;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!ulMaxMsgCounter) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulMaxMsgCounter invalidate.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_MAX_COUNTER_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    if (!stMaxMsgByteSize) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "ulMaxMsgByteSize invalidate.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_MAX_LEN_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
#endif

    if (_Object_Name_Invalid(pcName)) {                                 /*  检查名字有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    __KERNEL_MODE_PROC(
        pmsgqueue = _Allocate_MsgQueue_Object();                        /*  申请消息队列                */
    );
    
    if (!pmsgqueue) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is no ID to buide a msgqueue.\r\n");
        _ErrorHandle(ERROR_MSGQUEUE_FULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    __KERNEL_MODE_PROC(
        pevent = _Allocate_Event_Object();                              /*  申请事件                    */
    );
    
    if (!pevent) {
        __KERNEL_MODE_PROC(
            _Free_MsgQueue_Object(pmsgqueue);
        );
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is no ID to buide a msgqueue.\r\n");
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    stMaxMsgByteSizeReal = stMaxMsgByteSize + sizeof(size_t);           /*  多分配 size_t 存储长度      */
    stMaxMsgByteSizeReal = ROUND_UP(stMaxMsgByteSizeReal, 
                                    sizeof(LW_STACK));                  /*  对齐大小处理                */
    
    stHeapAllocateByteSize = ((size_t)ulMaxMsgCounter
                           * stMaxMsgByteSizeReal);                     /*  计算需要开辟的缓冲区大小    */
    
    pvMemAllocate = __KHEAP_ALLOC(stHeapAllocateByteSize);              /*  申请内存                    */
    if (!pvMemAllocate) {
        __KERNEL_MODE_PROC(
            _Free_MsgQueue_Object(pmsgqueue);
            _Free_Event_Object(pevent);
        );
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
        _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (pcName) {                                                       /*  拷贝名字                    */
        lib_strcpy(pevent->EVENT_cEventName, pcName);
    } else {
        pevent->EVENT_cEventName[0] = PX_EOS;                           /*  清空名字                    */
    }
    
    pmsgqueue->MSGQUEUE_pucBufferLowAddr  = (UINT8 *)pvMemAllocate;
    pmsgqueue->MSGQUEUE_pucBufferHighAddr = (UINT8 *)pvMemAllocate + stHeapAllocateByteSize;
    pmsgqueue->MSGQUEUE_pucInputPtr       = pmsgqueue->MSGQUEUE_pucBufferLowAddr;
    pmsgqueue->MSGQUEUE_pucOutputPtr      = pmsgqueue->MSGQUEUE_pucBufferLowAddr;
    pmsgqueue->MSGQUEUE_stEachMsgByteSize = (stMaxMsgByteSizeReal - sizeof(size_t));
    
    pevent->EVENT_ucType       = LW_TYPE_EVENT_MSGQUEUE;
    pevent->EVENT_pvTcbOwn     = LW_NULL;
    pevent->EVENT_ulCounter    = 0ul;
    pevent->EVENT_ulMaxCounter = ulMaxMsgCounter;
    pevent->EVENT_ulOption     = ulOption;
    pevent->EVENT_pvPtr        = (PVOID)pmsgqueue;
    
    pwqTemp[0] = &pevent->EVENT_wqWaitQ[0];
    pwqTemp[1] = &pevent->EVENT_wqWaitQ[1];
    pwqTemp[0]->WQ_usNum = 0;                                           /*  没有线程                    */
    pwqTemp[1]->WQ_usNum = 0;
    
    for (ulI = 0; ulI < __EVENT_Q_SIZE; ulI++) {
        pwqTemp[0]->WQ_wlQ.WL_pringPrio[ulI] = LW_NULL;
        pwqTemp[1]->WQ_wlQ.WL_pringPrio[ulI] = LW_NULL;
    }
    
    ulIdTemp = _MakeObjectId(_OBJECT_MSGQUEUE, 
                             LW_CFG_PROCESSOR_NUMBER, 
                             pevent->EVENT_usIndex);                    /*  构建对象 id                 */
    
    if (pulId) {
        *pulId = ulIdTemp;
    }
    
    __LW_OBJECT_CREATE_HOOK(ulIdTemp, ulOption);
    
    MONITOR_EVT_LONG4(MONITOR_EVENT_ID_MSGQ, MONITOR_EVENT_MSGQ_CREATE, 
                      ulIdTemp, ulMaxMsgCounter, stMaxMsgByteSize, ulOption, pcName);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "msgqueue \"%s\" has been create.\r\n", (pcName ? pcName : ""));
    
    return  (ulIdTemp);
}

#endif                                                                  /*  (LW_CFG_MSGQUEUE_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_MSGQUEUES > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

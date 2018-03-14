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
** 文   件   名: ThreadSetNotePad.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 19 日
**
** 描        述: 设置线程记事本

** BUG
2007.07.19  加入 _DebugHandle() 功能
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadSetNotePad
** 功能描述: 设置线程记事本
** 输　入  : 
**           ulId                          线程ID
**           ucNoteIndex                   线程记事本索引
**           ulVal                         线程记事本值
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if (LW_CFG_THREAD_NOTE_PAD_EN > 0) && (LW_CFG_MAX_NOTEPADS > 0)

LW_API  
ULONG  API_ThreadSetNotePad (LW_OBJECT_HANDLE  ulId,
                             UINT8             ucNoteIndex,
                             ULONG             ulVal)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
	
    usIndex = _ObjectGetIndex(ulId);
	
#if LW_CFG_ARG_CHK_EN > 0
    if (ucNoteIndex >= LW_CFG_MAX_NOTEPADS) {                           /*  检查记事本索引              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "notepad invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NOTEPAD_INDEX);
        return  (ERROR_THREAD_NOTEPAD_INDEX);
    }
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    ptcb->TCB_notepadThreadNotePad.NOTEPAD_ulNotePad[ucNoteIndex] = ulVal;
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  退出内核并打开中断          */
    
    MONITOR_EVT_LONG3(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_NOTEPAD, 
                      ulId, ucNoteIndex, ulVal, LW_NULL);
    
    return  (ERROR_NONE);
}

#endif                                                                  /* (LW_CFG_THREAD_NOTE_P...     */
                                                                        /* (LW_CFG_MAX_NOTEPADS > 0)    */
/*********************************************************************************************************
  END
*********************************************************************************************************/

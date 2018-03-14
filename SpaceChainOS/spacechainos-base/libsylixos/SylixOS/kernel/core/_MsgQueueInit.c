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
** 文   件   名: _MsgQueueInit.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: 消息队列初始化。

** BUG:
2013.11.14  使用对象资源管理器结构管理空闲资源.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _MsgQueueInit
** 功能描述: 消息队列初始化。
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _MsgQueueInit (VOID)
{
#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)

#if  LW_CFG_MAX_MSGQUEUES == 1

    _K_resrcMsgQueue.RESRC_pmonoFreeHeader = &_K_msgqueueBuffer[0].MSGQUEUE_monoResrcList;
    
    _INIT_LIST_MONO_HEAD(_K_resrcMsgQueue.RESRC_pmonoFreeHeader);
    
    _K_resrcMsgQueue.RESRC_pmonoFreeTail = _K_resrcMsgQueue.RESRC_pmonoFreeHeader;
#else

    REGISTER ULONG                  ulI;
    REGISTER PLW_LIST_MONO          pmonoTemp1;
    REGISTER PLW_LIST_MONO          pmonoTemp2;
    REGISTER PLW_CLASS_MSGQUEUE     pmsgqueueTemp1;
    REGISTER PLW_CLASS_MSGQUEUE     pmsgqueueTemp2;

    _K_resrcMsgQueue.RESRC_pmonoFreeHeader = &_K_msgqueueBuffer[0].MSGQUEUE_monoResrcList;

    pmsgqueueTemp1 = &_K_msgqueueBuffer[0];
    pmsgqueueTemp2 = &_K_msgqueueBuffer[1];
    
    for (ulI = 0; ulI < ((LW_CFG_MAX_MSGQUEUES) - 1); ulI++) {
        pmonoTemp1 = &pmsgqueueTemp1->MSGQUEUE_monoResrcList;
        pmonoTemp2 = &pmsgqueueTemp2->MSGQUEUE_monoResrcList;
        
        _LIST_MONO_LINK(pmonoTemp1, pmonoTemp2);
        
        pmsgqueueTemp1++;
        pmsgqueueTemp2++;
    }
    
    pmonoTemp1 = &pmsgqueueTemp1->MSGQUEUE_monoResrcList;
    
    _INIT_LIST_MONO_HEAD(pmonoTemp1);
    
    _K_resrcMsgQueue.RESRC_pmonoFreeTail = pmonoTemp1;
#endif                                                                  /*  LW_CFG_MAX_MSGQUEUES == 1   */

    _K_resrcMsgQueue.RESRC_uiUsed    = 0;
    _K_resrcMsgQueue.RESRC_uiMaxUsed = 0;

#endif                                                                  /*  (LW_CFG_MSGQUEUE_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_MSGQUEUES > 0)  */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

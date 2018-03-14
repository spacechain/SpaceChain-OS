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
** 文   件   名: _MsgQueueLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 07 日
**
** 描        述: 消息队列内部操作库。

** BUG:
2009.06.19  修改注释.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _MsgQueueGetMsg
** 功能描述: 从消息队列中获得一个消息
** 输　入  : pmsgqueue     消息队列控制块
**         : pvMsgBuffer   接收缓冲区
**         : stMaxByteSize 缓冲区大小
**         : pstMsgLen     获得消息的长度
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)

VOID  _MsgQueueGetMsg (PLW_CLASS_MSGQUEUE    pmsgqueue,                 /*  消息队列                    */
                       PVOID                 pvMsgBuffer,               /*  缓冲区                      */
                       size_t                stMaxByteSize,             /*  缓冲区大小                  */
                       size_t               *pstMsgLen)                 /*  实际拷贝字节数              */
{
    REGISTER size_t   stMsgLen;
    
    stMsgLen = *((size_t *)pmsgqueue->MSGQUEUE_pucOutputPtr);           /*  获得本消息大小              */
    pmsgqueue->MSGQUEUE_pucOutputPtr += sizeof(size_t);                 /*  将指针移动到消息处          */
    
    *pstMsgLen = (stMaxByteSize < stMsgLen) ? 
                 (stMaxByteSize) : (stMsgLen);                          /*  确定拷贝信息数量            */
    
    lib_memcpy(pvMsgBuffer,                                             /*  拷贝消息                    */
               pmsgqueue->MSGQUEUE_pucOutputPtr,
               *pstMsgLen);
    
    pmsgqueue->MSGQUEUE_pucOutputPtr += pmsgqueue->MSGQUEUE_stEachMsgByteSize;
    
    if (pmsgqueue->MSGQUEUE_pucOutputPtr >= 
        pmsgqueue->MSGQUEUE_pucBufferHighAddr) {                        /*  调整 OUT 指针               */
        pmsgqueue->MSGQUEUE_pucOutputPtr = pmsgqueue->MSGQUEUE_pucBufferLowAddr;
    }
}
/*********************************************************************************************************
** 函数名称: _MsgQueueSendMsg
** 功能描述: 向消息队列中写入一个消息
** 输　入  : pmsgqueue     消息队列控制块 
**           pvMsgBuffer   消息缓冲区
**         : stMsgLen      消息长度
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _MsgQueueSendMsg (PLW_CLASS_MSGQUEUE    pmsgqueue,
                        PVOID                 pvMsgBuffer,
                        size_t                stMsgLen)
{
    REGISTER size_t   *pstMsgLen;
    
    pstMsgLen = (size_t *)pmsgqueue->MSGQUEUE_pucInputPtr;              /*  获得消息长短指针            */
    pmsgqueue->MSGQUEUE_pucInputPtr += sizeof(size_t);                  /*  将指针移动到消息处          */
    
    lib_memcpy((PVOID)pmsgqueue->MSGQUEUE_pucInputPtr,                  /*  拷贝消息                    */
               pvMsgBuffer,                                             
               stMsgLen);
    
    *pstMsgLen = stMsgLen;                                              /*  记录消息长短                */
    
    pmsgqueue->MSGQUEUE_pucInputPtr += pmsgqueue->MSGQUEUE_stEachMsgByteSize;
    
    if (pmsgqueue->MSGQUEUE_pucInputPtr >= 
        pmsgqueue->MSGQUEUE_pucBufferHighAddr) {                        /*  调整 IN 指针                */
        pmsgqueue->MSGQUEUE_pucInputPtr = pmsgqueue->MSGQUEUE_pucBufferLowAddr;
    }
}
/*********************************************************************************************************
** 函数名称: _MsgQueueSendMsgUrgent
** 功能描述: 向消息队列中写入一个紧急消息
** 输　入  : pmsgqueue     消息队列控制块
**         : pvMsgBuffer   消息缓冲区
**         : stMsgLen      消息长度
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _MsgQueueSendMsgUrgent (PLW_CLASS_MSGQUEUE    pmsgqueue,
                              PVOID                 pvMsgBuffer,
                              size_t                stMsgLen)
{
    REGISTER size_t   *pstMsgLen;
    REGISTER PBYTE     pucMsgQueueBuffer;
    
    if (pmsgqueue->MSGQUEUE_pucOutputPtr == 
        pmsgqueue->MSGQUEUE_pucBufferLowAddr) {                         /*  OUT 调整                    */
        pmsgqueue->MSGQUEUE_pucOutputPtr = pmsgqueue->MSGQUEUE_pucBufferHighAddr;
    }
    
    pmsgqueue->MSGQUEUE_pucOutputPtr -=
        (pmsgqueue->MSGQUEUE_stEachMsgByteSize + sizeof(size_t));
    
    pstMsgLen = (size_t *)pmsgqueue->MSGQUEUE_pucOutputPtr;             /*  获得消息长短指针            */
    pucMsgQueueBuffer = pmsgqueue->MSGQUEUE_pucOutputPtr + sizeof(size_t);
    
    lib_memcpy((PVOID)pucMsgQueueBuffer,                                /*  拷贝消息                    */
               pvMsgBuffer,                                             
               stMsgLen);
               
    *pstMsgLen = stMsgLen;                                              /*  记录消息长短                */
}
/*********************************************************************************************************
** 函数名称: _MsgQueueGetMsgLen
** 功能描述: 从消息队列中获得一个消息的长度
** 输　入  : pmsgqueue     消息队列控制块
**           pstMsgLen     消息长度
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _MsgQueueGetMsgLen (PLW_CLASS_MSGQUEUE  pmsgqueue, size_t  *pstMsgLen)
{
    *pstMsgLen = *((size_t *)pmsgqueue->MSGQUEUE_pucOutputPtr);
}

#endif                                                                  /*  (LW_CFG_MSGQUEUE_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_MSGQUEUES > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

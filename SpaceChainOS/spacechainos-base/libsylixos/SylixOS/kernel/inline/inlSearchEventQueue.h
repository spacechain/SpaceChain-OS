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
** 文   件   名: inlSearchEventQueue.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 20 日
**
** 描        述: 系统事件等待队列基本操作。

** BUG
2007.09.12  加入可裁剪宏限制。
2007.11.13  使用链表库对链表操作进行完全封装.
2007.11.18  整理注释.
*********************************************************************************************************/

#ifndef  __INLSEARCHEVENTQUEUE_H
#define  __INLSEARCHEVENTQUEUE_H

#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

/*********************************************************************************************************
  _EVENT_FIFO_Q_PTR
*********************************************************************************************************/

#define _EVENT_FIFO_Q_PTR(iQ, ppringListPtr)                        \
do {                                                                \
    ppringListPtr = &pevent->EVENT_wqWaitQ[iQ].WQ_wlQ.WL_pringFifo; \
} while (0)

/*********************************************************************************************************
  _EVENT_PRIORITY_Q_PTR
*********************************************************************************************************/

#define _EVENT_PRIORITY_Q_PTR(iQ, ppringListPtr, ucPrioIndex)                       \
do {                                                                                \
    ppringListPtr = &pevent->EVENT_wqWaitQ[iQ].WQ_wlQ.WL_pringPrio[ucPrioIndex];    \
} while (0)

/*********************************************************************************************************
  _EVENT_DEL_Q_FIFO
        
  find the pointer which point a FIFO queue, a thread removed form it.
*********************************************************************************************************/

#define _EVENT_DEL_Q_FIFO(iQ, ppringListPtr)                        \
do {                                                                \
    ppringListPtr = &pevent->EVENT_wqWaitQ[iQ].WQ_wlQ.WL_pringFifo; \
} while (0)

/*********************************************************************************************************
  _EVENT_DEL_Q_PRIORITY
        
  find the pointer which point a priority queue, a thread removed form it.
*********************************************************************************************************/

#define _EVENT_DEL_Q_PRIORITY(iQ, ppringListPtr)                        \
do {                                                                    \
    REGISTER INT    i;                                                  \
    ppringListPtr = &pevent->EVENT_wqWaitQ[iQ].WQ_wlQ.WL_pringPrio[0];  \
    for (i = 0; i < __EVENT_Q_SIZE; i++) {                              \
        if (*ppringListPtr) {                                           \
            break;                                                      \
        } else {                                                        \
            ppringListPtr++;                                            \
        }                                                               \
    }                                                                   \
} while (0)

/*********************************************************************************************************
  _EVENT_INDEX_Q_PRIORITY
*********************************************************************************************************/

#define _EVENT_INDEX_Q_PRIORITY(ucPriority, ucIndex)    \
do {                                                    \
    ucIndex = (UINT8)(ucPriority >> __EVENT_Q_SHIFT);   \
} while (0)

/*********************************************************************************************************
  _EventWaitNum
*********************************************************************************************************/

static LW_INLINE UINT16  _EventWaitNum (INT  iQ, PLW_CLASS_EVENT    pevent)
{
    return  (pevent->EVENT_wqWaitQ[iQ].WQ_usNum);
}

/*********************************************************************************************************
  _EventQGetTcbFifo (Make Oldest thread ready, So this is The Last Node In Ring)
*********************************************************************************************************/

static LW_INLINE PLW_CLASS_TCB  _EventQGetTcbFifo (PLW_CLASS_EVENT    pevent, 
                                                   PLW_LIST_RING     *ppringList)
{
    REGISTER PLW_CLASS_TCB    ptcb;
    REGISTER PLW_LIST_RING    pringOldest;
    
    pringOldest = _list_ring_get_prev(*ppringList);
    
    ptcb = _LIST_ENTRY(pringOldest, LW_CLASS_TCB, TCB_ringEvent);
    
    return  (ptcb);
}

/*********************************************************************************************************
  _EventQGetTcbPriority (Make highest priority thread ready, So this is The Frist Node In Ring)
*********************************************************************************************************/

static LW_INLINE PLW_CLASS_TCB  _EventQGetTcbPriority (PLW_CLASS_EVENT    pevent, 
                                                       PLW_LIST_RING     *ppringList)
{
    REGISTER PLW_CLASS_TCB    ptcb;

    ptcb = _LIST_ENTRY(*ppringList, LW_CLASS_TCB, TCB_ringEvent);
    
    return  (ptcb);
}

#endif                                                                  /*  (LW_CFG_EVENT_EN > 0)       */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
#endif                                                                  /*  __INLSEARCHEVENTQUEUE_H     */
/*********************************************************************************************************
  END
*********************************************************************************************************/

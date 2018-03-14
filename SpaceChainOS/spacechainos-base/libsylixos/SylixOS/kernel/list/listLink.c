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
** 文   件   名: listLink.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 17 日
**
** 描        述: 这是系统链表连接操作定义。

** BUG
2008.11.30  整理文件, 修改注释.
2010.08.16  深切哀悼舟曲泥石流中遇难的同胞!
            增加线性链表的左右插入功能.
2011.06.12  精简 _List_Line_Del() 函数.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _List_Ring_Add_Ahead
** 功能描述: 向 RING 头中插入一个节点 (Header 将指向这个节点, 等待表的操作)
** 输　入  : pringNew      新的节点
**           ppringHeader  链表头
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _List_Ring_Add_Ahead (PLW_LIST_RING  pringNew, LW_LIST_RING_HEADER  *ppringHeader)
{
    REGISTER LW_LIST_RING_HEADER    pringHeader;
    
    pringHeader = *ppringHeader;
    
    if (pringHeader) {                                                  /*  链表内还有节点              */
        pringNew->RING_plistNext = pringHeader;
        pringNew->RING_plistPrev = pringHeader->RING_plistPrev;
        pringHeader->RING_plistPrev->RING_plistNext = pringNew;
        pringHeader->RING_plistPrev = pringNew;
        
    } else {                                                            /*  链表内没有节点              */
        pringNew->RING_plistPrev = pringNew;                            /*  只有新节点                  */
        pringNew->RING_plistNext = pringNew;                            /*  自我抱环                    */
    }
    
    *ppringHeader = pringNew;                                           /*  将表头指向新节点            */
}
/*********************************************************************************************************
** 函数名称: _List_Ring_Add_Front
** 功能描述: 向 RING 头中插入一个节点 (Header 有节点时，不变化，就绪表的操作)
** 输　入  : pringNew      新的节点
**           ppringHeader  链表头
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _List_Ring_Add_Front (PLW_LIST_RING  pringNew, LW_LIST_RING_HEADER  *ppringHeader)
{
    REGISTER LW_LIST_RING_HEADER    pringHeader;
    
    pringHeader = *ppringHeader;
    
    if (pringHeader) {                                                  /*  链表内还有节点              */
        pringNew->RING_plistPrev = pringHeader;
        pringNew->RING_plistNext = pringHeader->RING_plistNext;
        pringHeader->RING_plistNext->RING_plistPrev = pringNew;
        pringHeader->RING_plistNext = pringNew;
    
    } else {                                                            /*  链表内没有节点              */
        pringNew->RING_plistPrev = pringNew;                            /*  只有新节点                  */
        pringNew->RING_plistNext = pringNew;                            /*  自我抱环                    */
        *ppringHeader = pringNew;                                       /*  更新链表头                  */
    }
}
/*********************************************************************************************************
** 函数名称: _List_Ring_Add_Last
** 功能描述: 从后面向 RING 尾中插入一个节点
** 输　入  : pringNew      新的节点
**           ppringHeader  链表头
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _List_Ring_Add_Last (PLW_LIST_RING  pringNew, LW_LIST_RING_HEADER  *ppringHeader)
{
    REGISTER LW_LIST_RING_HEADER    pringHeader;
    
    pringHeader = *ppringHeader;
    
    if (pringHeader) {                                                  /*  没有更新链表                */
        pringHeader->RING_plistPrev->RING_plistNext = pringNew;
        pringNew->RING_plistPrev = pringHeader->RING_plistPrev;
        pringNew->RING_plistNext = pringHeader;
        pringHeader->RING_plistPrev = pringNew;
        
    } else {
        pringNew->RING_plistPrev = pringNew;
        pringNew->RING_plistNext = pringNew;
        *ppringHeader = pringNew;                                       /*  更新链表头                  */
    }
}
/*********************************************************************************************************
** 函数名称: _List_Ring_Del
** 功能描述: 删除一个节点
** 输　入  : pringDel      需要删除的节点
**           ppringHeader  链表头
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _List_Ring_Del (PLW_LIST_RING  pringDel, LW_LIST_RING_HEADER  *ppringHeader)
{
    REGISTER LW_LIST_RING_HEADER    pringHeader;
    
    pringHeader = *ppringHeader;
    
    if (pringDel->RING_plistNext == pringDel) {                         /*  链表中只有一个节点          */
        *ppringHeader = LW_NULL;
        _INIT_LIST_RING_HEAD(pringDel);
        return;
    
    } else if (pringDel == pringHeader) {
        _list_ring_next(ppringHeader);
    }
    
    pringHeader = pringDel->RING_plistPrev;                             /*  pringHeader 属于临时变量    */
    pringHeader->RING_plistNext = pringDel->RING_plistNext;
    pringDel->RING_plistNext->RING_plistPrev = pringHeader;
    
    _INIT_LIST_RING_HEAD(pringDel);                                     /*  prev = next = NULL          */
}
/*********************************************************************************************************
** 函数名称: _List_Line_Add_Ahead
** 功能描述: 从前面向 Line 头中插入一个节点 (Header 将指向这个节点)
** 输　入  : plingNew      新的节点
**           pplingHeader  链表头
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _List_Line_Add_Ahead (PLW_LIST_LINE  plineNew, LW_LIST_LINE_HEADER  *pplineHeader)
{
    REGISTER LW_LIST_LINE_HEADER    plineHeader;
    
    plineHeader = *pplineHeader;
    
    plineNew->LINE_plistNext = plineHeader;
    plineNew->LINE_plistPrev = LW_NULL;
    
    if (plineHeader) {    
        plineHeader->LINE_plistPrev = plineNew;
    }
    
    *pplineHeader = plineNew;                                           /*  指向最新变量                */
}
/*********************************************************************************************************
** 函数名称: _List_Line_Add_Tail
** 功能描述: 从前面向 Line 头中插入一个节点 (Header 不变)
** 输　入  : plingNew      新的节点
**           pplingHeader  链表头
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _List_Line_Add_Tail (PLW_LIST_LINE  plineNew, LW_LIST_LINE_HEADER  *pplineHeader)
{
    REGISTER LW_LIST_LINE_HEADER    plineHeader;
    
    plineHeader = *pplineHeader;
    
    if (plineHeader) {
        if (plineHeader->LINE_plistNext) {
            plineNew->LINE_plistNext = plineHeader->LINE_plistNext;
            plineNew->LINE_plistPrev = plineHeader;
            plineHeader->LINE_plistNext->LINE_plistPrev = plineNew;
            plineHeader->LINE_plistNext = plineNew;
        
        } else {
            plineHeader->LINE_plistNext = plineNew;
            plineNew->LINE_plistPrev = plineHeader;
            plineNew->LINE_plistNext = LW_NULL;
        }
    
    } else {
        plineNew->LINE_plistPrev = LW_NULL;
        plineNew->LINE_plistNext = LW_NULL;
        
        *pplineHeader = plineNew;
    }
}
/*********************************************************************************************************
** 函数名称: _List_Line_Add_Left
** 功能描述: 将新的节点插入指定节点的左侧.
** 输　入  : plineNew      新的节点
**           plineRight    右侧节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _List_Line_Add_Left (PLW_LIST_LINE  plineNew, PLW_LIST_LINE  plineRight)
{
    REGISTER PLW_LIST_LINE      plineLeft = plineRight->LINE_plistPrev;
    
    plineNew->LINE_plistNext = plineRight;
    plineNew->LINE_plistPrev = plineLeft;
    
    if (plineLeft) {
        plineLeft->LINE_plistNext = plineNew;
    }
    
    plineRight->LINE_plistPrev = plineNew;
}
/*********************************************************************************************************
** 函数名称: _List_Line_Add_Right
** 功能描述: 将新的节点插入指定节点的右侧.
** 输　入  : plineNew      新的节点
**           plineLeft     左侧节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _List_Line_Add_Right (PLW_LIST_LINE  plineNew, PLW_LIST_LINE  plineLeft)
{
    REGISTER PLW_LIST_LINE      plineRight = plineLeft->LINE_plistNext;
    
    plineNew->LINE_plistNext = plineRight;
    plineNew->LINE_plistPrev = plineLeft;
    
    if (plineRight) {
        plineRight->LINE_plistPrev = plineNew;
    }
    
    plineLeft->LINE_plistNext = plineNew;
}
/*********************************************************************************************************
** 函数名称: _List_Line_Del
** 功能描述: 删除一个节点
** 输　入  : plingDel      需要删除的节点
**           pplingHeader  链表头
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _List_Line_Del (PLW_LIST_LINE  plineDel, LW_LIST_LINE_HEADER  *pplineHeader)
{
    
    if (plineDel->LINE_plistPrev == LW_NULL) {                          /*  表头                        */
        *pplineHeader = plineDel->LINE_plistNext;
    } else {
        plineDel->LINE_plistPrev->LINE_plistNext = plineDel->LINE_plistNext;
    }
    
    if (plineDel->LINE_plistNext) {
        plineDel->LINE_plistNext->LINE_plistPrev = plineDel->LINE_plistPrev;
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

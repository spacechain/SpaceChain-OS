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
** 文   件   名: selectNode.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 11 月 07 日
**
** 描        述:  IO 系统 select 子系统 WAKEUP 节点操作基础.

** BUG
2007.11.13  使用链表库对链表操作进行完全封装.
2007.11.20  API_SelWakeup() 需要检查线程有效性.
2007.11.21  API_SelNodeDelete() 不管是否 FREE ,只要寻到节点就立即退出.
2007.11.21  API_SelNodeDeleteAll() 不再调用 API_SelNodeDelete() 进行删除, 而使用自行删除节点.
2007.12.11  加入错误唤醒能力,可以产生一个错误唤醒.
2009.12.09  修正了 API_SelWakeupAllError() exceJobAdd() 函数指针.
2011.03.06  修正 gcc 4.5.1 相关 warning.
2014.03.03  优化代码.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SELECT_EN > 0)
#include "select.h"
/*********************************************************************************************************
  锁
*********************************************************************************************************/
#define SEL_LIST_LOCK(pselwulList)      \
        API_SemaphoreMPend(pselwulList->SELWUL_hListLock, LW_OPTION_WAIT_INFINITE)
#define SEL_LIST_UNLOCK(pselwulList)    \
        API_SemaphoreMPost(pselwulList->SELWUL_hListLock)
/*********************************************************************************************************
** 函数名称: API_SelWakeupType
** 功能描述: 获得节点的等待类型
** 输　入  : pselwunNode        select wake up node 控制结构
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
LW_SEL_TYPE API_SelWakeupType (PLW_SEL_WAKEUPNODE   pselwunNode)
{
    if (!pselwunNode) {
        return  (SELEXCEPT);                                            /*  节点错误                    */
    }
    
    return  (pselwunNode->SELWUN_seltypType);                           /*  返回节点类型                */
}
/*********************************************************************************************************
** 函数名称: API_SelWakeup
** 功能描述: 唤醒一个等待的线程
** 输　入  : pselwunNode        select wake up node 控制结构
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID    API_SelWakeup (PLW_SEL_WAKEUPNODE   pselwunNode)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
    
    PLW_SEL_CONTEXT         pselctxContext;
    
    if (!pselwunNode) {                                                 /*  指针错误                    */
        return;
    }
    
    usIndex = _ObjectGetIndex(pselwunNode->SELWUN_hThreadId);
    
    ptcb = __GET_TCB_FROM_INDEX(usIndex);
    if (!ptcb) {                                                        /*  线程不存在                  */
        return;
    }
    
    pselctxContext = ptcb->TCB_pselctxContext;                          /*  获得 select() context       */
    if (!pselctxContext) {
        return;
    }
    
    switch (pselwunNode->SELWUN_seltypType) {
    
    case SELREAD:                                                       /*  read 事件                   */
        FD_SET(pselwunNode->SELWUN_iFd, pselctxContext->SELCTX_pfdsetReadFds);
	    break;
	    
    case SELWRITE:                                                      /*  write 事件                  */
        FD_SET(pselwunNode->SELWUN_iFd, pselctxContext->SELCTX_pfdsetWriteFds);
	    break;
    
    case SELEXCEPT:                                                     /*  except 事件                 */
        FD_SET(pselwunNode->SELWUN_iFd, pselctxContext->SELCTX_pfdsetExceptFds);
        break;
    }
    
    API_SemaphoreBPost(pselctxContext->SELCTX_hSembWakeup);             /*  提前激活即将等待线程        */
}
/*********************************************************************************************************
** 函数名称: API_SelWakeupError
** 功能描述: 由于产生了错误, 唤醒一个等待的线程
** 输　入  : pselwunNode        select wake up node 控制结构
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID    API_SelWakeupError (PLW_SEL_WAKEUPNODE   pselwunNode)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
    
    PLW_SEL_CONTEXT         pselctxContext;
    
    if (!pselwunNode) {                                                 /*  指针错误                    */
        return;
    }
    
    usIndex = _ObjectGetIndex(pselwunNode->SELWUN_hThreadId);
    
    ptcb = __GET_TCB_FROM_INDEX(usIndex);
    if (!ptcb) {                                                        /*  线程不存在                  */
        return;
    }
    
    pselctxContext = ptcb->TCB_pselctxContext;                          /*  获得 select() context       */
    if (!pselctxContext) {
        return;
    }
    
    API_SemaphoreBPost(pselctxContext->SELCTX_hSembWakeup);             /*  提前激活即将等待线程        */
}
/*********************************************************************************************************
** 函数名称: API_SelWakeupAll
** 功能描述: 唤醒所有等待指定操作的线程
** 输　入  : pselwulList        select wake up list 控制结构
             seltyp             等待类型
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID    API_SelWakeupAll (PLW_SEL_WAKEUPLIST  pselwulList, LW_SEL_TYPE  seltyp)
{
    REGISTER PLW_SEL_WAKEUPNODE     pselwunNode;
    REGISTER PLW_LIST_LINE          plineNode;
    
    if (!pselwulList || !pselwulList->SELWUL_plineHeader) {             /*  没有节点                    */
        return;
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  在中断中                    */
        _excJobAdd((VOIDFUNCPTR)API_SelWakeupAll, (PVOID)pselwulList, (PVOID)seltyp, 
                   0, 0, 0, 0);                                         /*  底半中断处理                */
        return;
    }
    
    SEL_LIST_LOCK(pselwulList);
    
    for (plineNode  = pselwulList->SELWUL_plineHeader;
         plineNode != LW_NULL;
         plineNode  = _list_line_get_next(plineNode)) {                 /*  遍历所有节点                */
        
        pselwunNode = _LIST_ENTRY(plineNode, LW_SEL_WAKEUPNODE, SELWUN_lineManage);
        if (pselwunNode->SELWUN_seltypType == seltyp) {                 /*  检查类型                    */
            API_SelWakeup(pselwunNode);                                 /*  释放                        */
        }
    }
    
    SEL_LIST_UNLOCK(pselwulList);
}
/*********************************************************************************************************
** 函数名称: API_SelWakeupTerm
** 功能描述: 由于产生了错误, 唤醒所有等待指定操作的线程
** 输　入  : pselwulList        select wake up list 控制结构
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID    API_SelWakeupTerm (PLW_SEL_WAKEUPLIST  pselwulList)
{
    REGISTER PLW_SEL_WAKEUPNODE     pselwunNode;
    REGISTER PLW_LIST_LINE          plineNode;
    
    if (!pselwulList) {
        return;
    }
    
    SEL_LIST_LOCK(pselwulList);
    
    plineNode = pselwulList->SELWUL_plineHeader;
    while (plineNode) {                                                 /*  逐一删除                    */
        pselwunNode = _LIST_ENTRY(plineNode, LW_SEL_WAKEUPNODE, SELWUN_lineManage);
        plineNode   = _list_line_get_next(plineNode);                   /*  获得下一个节点              */
        
        API_SelWakeupError(pselwunNode);                                /*  释放                        */
        
        _List_Line_Del(&pselwunNode->SELWUN_lineManage, 
                       &pselwulList->SELWUL_plineHeader);               /*  删除此节点                  */
        pselwulList->SELWUL_ulWakeCounter--;                            /*  等待线程数量--              */
        
        if (pselwunNode->SELWUN_bDontFree == LW_FALSE) {                /*  需要释放内存                */
            __SHEAP_FREE((PVOID)pselwunNode);
        }
    }
    
    SEL_LIST_UNLOCK(pselwulList);
}
/*********************************************************************************************************
** 函数名称: API_SelNodeAdd
** 功能描述: 向指定的 wake up list 添加一个等待节点.
** 输　入  : pselwulList        select wake up list 控制结构
             pselwunNode        select wake up node 控制结构
** 输　出  : 返回 ERROR_NONE ,当出现系统内存匮乏时,返回 PX_ERROR.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT     API_SelNodeAdd (PLW_SEL_WAKEUPLIST  pselwulList, PLW_SEL_WAKEUPNODE   pselwunNode)
{
    REGISTER PLW_SEL_WAKEUPNODE   pselwunNodeClone;                     /*  拷贝的节点                  */  
    REGISTER BOOL                 bDontFree;                            /*  不需要释放                  */
    
    if (!pselwulList || !pselwunNode) {                                 /*  内存错误                    */
        return  (PX_ERROR);
    }
    
    SEL_LIST_LOCK(pselwulList);
    
    if (pselwulList->SELWUL_plineHeader == LW_NULL) {                   /*  没有节点                    */
        pselwunNodeClone = &pselwulList->SELWUL_selwunFrist;
        bDontFree = LW_TRUE;
    } else {                                                            /*  已经存在节点                */
        pselwunNodeClone = (PLW_SEL_WAKEUPNODE)__SHEAP_ALLOC(sizeof(LW_SEL_WAKEUPNODE));
        bDontFree = LW_FALSE;
    }
    
    if (pselwunNodeClone == LW_NULL) {                                  /*  分配失败                    */
        SEL_LIST_UNLOCK(pselwulList);
        return  (PX_ERROR);
    }
    
    *pselwunNodeClone = *pselwunNode;                                   /*  拷贝节点信息                */
    
    pselwunNodeClone->SELWUN_bDontFree = bDontFree;                     /*  记录节点是否需要释放操作    */
    
    _List_Line_Add_Ahead(&pselwunNodeClone->SELWUN_lineManage, 
                         &pselwulList->SELWUL_plineHeader);             /*  加入等待链表                */
    pselwulList->SELWUL_ulWakeCounter++;                                /*  等待线程数量++              */
                         
    SEL_LIST_UNLOCK(pselwulList);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SelNodeDelete
** 功能描述: 从指定的 wake up list 中,删除一个等待节点.
** 输　入  : pselwulList        select wake up list 控制结构
             pselwunNodeDelete  select wake up node 控制结构
** 输　出  : 返回 ERROR_NONE ,当没有找到节点时,返回 PX_ERROR.
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT     API_SelNodeDelete (PLW_SEL_WAKEUPLIST  pselwulList, PLW_SEL_WAKEUPNODE   pselwunNodeDelete)
{
    REGISTER PLW_SEL_WAKEUPNODE   pselwunNode;                          /*  遍历节点                    */
    REGISTER PLW_LIST_LINE        plineNode;
    
    SEL_LIST_LOCK(pselwulList);
    
    for (plineNode  = pselwulList->SELWUL_plineHeader;
         plineNode != LW_NULL;
         plineNode  = _list_line_get_next(plineNode)) {                 /*  遍历所有节点                */
         
        pselwunNode = _LIST_ENTRY(plineNode, LW_SEL_WAKEUPNODE, SELWUN_lineManage);
        if ((pselwunNode->SELWUN_hThreadId  == pselwunNodeDelete->SELWUN_hThreadId) &&
            (pselwunNode->SELWUN_seltypType == pselwunNodeDelete->SELWUN_seltypType)) {
            
            _List_Line_Del(&pselwunNode->SELWUN_lineManage, 
                           &pselwulList->SELWUL_plineHeader);
            pselwulList->SELWUL_ulWakeCounter--;                        /*  等待线程数量--              */
            
            if (pselwunNode->SELWUN_bDontFree == LW_FALSE) {            /*  需要释放内存                */
                __SHEAP_FREE((PVOID)pselwunNode);
            }
            
            SEL_LIST_UNLOCK(pselwulList);
            return  (ERROR_NONE);                                       /*  退出                        */
        }
    }
    
    SEL_LIST_UNLOCK(pselwulList);
    
    return  (PX_ERROR);                                                 /*  没有找到节点                */
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_SELECT_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/

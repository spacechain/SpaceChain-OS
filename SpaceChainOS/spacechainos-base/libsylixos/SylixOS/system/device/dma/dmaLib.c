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
** 文   件   名: dmaLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 01 月 06 日
**
** 描        述: 通用 DMA 设备管理库.

** BUG:
2009.04.15  修改代码风格.
2009.09.15  加入了任务队列同步功能.
2009.12.11  升级 DMA 硬件抽象结构, 支持系统同时存在多种异构 DMA 控制器.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_MAX_DMA_CHANNELS > 0) && (LW_CFG_DMA_EN > 0)
/*********************************************************************************************************
  全局变量声明
*********************************************************************************************************/
static __DMA_WAITNODE       _G_dmanBuffer[LW_CFG_MAX_DMA_LISTNODES];    /*  等待节点缓冲                */
static PLW_LIST_RING        _G_pringDmanFreeHeader = LW_NULL;           /*  空闲缓冲区节点              */
/*********************************************************************************************************
** 函数名称: _dmaInit
** 功能描述: 初始化 DMA
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID    _dmaInit (VOID)
{
    REGISTER INT    i;
    
    for (i = 0; i < LW_CFG_MAX_DMA_LISTNODES; i++) {                    /*  将所有节点串成链表          */
        _List_Ring_Add_Ahead(&_G_dmanBuffer[i].DMAN_ringManage, 
                             &_G_pringDmanFreeHeader);
    }
}
/*********************************************************************************************************
** 函数名称: _dmaWaitnodeAlloc
** 功能描述: 从缓冲区中申请一个 DMA 等待节点
** 输　入  : NONE
** 输　出  : 新申请出的节点, 缓冲区内没有空闲接点时, 返回 LW_NULL;
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
__PDMA_WAITNODE     _dmaWaitnodeAlloc (VOID)
{
    REGISTER PLW_LIST_RING      pringNewNode;
    REGISTER __PDMA_WAITNODE    pdmanNewNode;
    
    if (_G_pringDmanFreeHeader) {                                       /*  还存在空闲的节点            */
        pringNewNode = _G_pringDmanFreeHeader;
        _List_Ring_Del(_G_pringDmanFreeHeader,
                      &_G_pringDmanFreeHeader);                         /*  删除表头的节点              */
        pdmanNewNode = _LIST_ENTRY(pringNewNode, __DMA_WAITNODE, DMAN_ringManage);
        return  (pdmanNewNode);                                         /*  返回新节点                  */
    
    } else {
        return  (LW_NULL);                                              /*  没有新节点了                */
    }
}
/*********************************************************************************************************
** 函数名称: _dmaWaitnodeFree
** 功能描述: 向缓冲区中释放一个 DMA 等待节点
** 输　入  : pdmanNode     要释放的节点.
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID    _dmaWaitnodeFree (__PDMA_WAITNODE   pdmanNode)
{
    _List_Ring_Add_Ahead(&pdmanNode->DMAN_ringManage, 
                         &_G_pringDmanFreeHeader);                      /*  加入空闲队列                */
}
/*********************************************************************************************************
** 函数名称: _dmaInsertToWaitList
** 功能描述: 向指定 DMA 通道的等待队列插入一个节点 (插到最后一个节点)
** 输　入  : pdmanNode     要插入的节点.
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID    _dmaInsertToWaitList (__PDMA_CHANNEL    pdmacChannel, __PDMA_WAITNODE   pdmanNode)
{
    _List_Ring_Add_Last(&pdmanNode->DMAN_ringManage,
                        &pdmacChannel->DMAC_pringHead);                 /*  插入队列                    */
    pdmacChannel->DMAC_iNodeCounter++;                                  /*  数量 ++                     */
}
/*********************************************************************************************************
** 函数名称: _dmaDeleteFromWaitList
** 功能描述: 从指定 DMA 通道的等待队列删除一个节点
** 输　入  : pdmanNode     要删除的节点.
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID    _dmaDeleteFromWaitList (__PDMA_CHANNEL    pdmacChannel, __PDMA_WAITNODE   pdmanNode)
{
    _List_Ring_Del(&pdmanNode->DMAN_ringManage,
                   &pdmacChannel->DMAC_pringHead);                      /*  从队列中删除                */
    pdmacChannel->DMAC_iNodeCounter--;                                  /*  数量 --                     */
}
/*********************************************************************************************************
** 函数名称: _dmaGetFirstInWaitList
** 功能描述: 从指定 DMA 通道的获得等待队列中第一个节点, 但是不删除.
** 输　入  : pdmanNode     要删除的节点.
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
__PDMA_WAITNODE     _dmaGetFirstInWaitList (__PDMA_CHANNEL    pdmacChannel)
{
    REGISTER __PDMA_WAITNODE   pdmanNode;
    REGISTER PLW_LIST_RING     pringNode;
    
    pringNode = pdmacChannel->DMAC_pringHead;
    if (pringNode) {
        pdmanNode = _LIST_ENTRY(pringNode, __DMA_WAITNODE, DMAN_ringManage);
        return  (pdmanNode);
    
    } else {
        return  (LW_NULL);
    }
}

#endif                                                                  /*  LW_CFG_MAX_DMA_CHANNELS > 0 */
                                                                        /*  LW_CFG_DMA_EN   > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

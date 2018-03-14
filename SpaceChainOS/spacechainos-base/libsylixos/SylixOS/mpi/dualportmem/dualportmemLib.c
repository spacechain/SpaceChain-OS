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
** 文   件   名: dualportmemLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 04 月 10 日
**
** 描        述: 系统支持共享内存式多处理器双口内存内部功能
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#define  __DPMA_MAIN_FILE
#include "../SylixOS/mpi/include/mpi_mpi.h"
/*********************************************************************************************************
** 函数名称: _DpmaInit
** 功能描述: 双通道内存管理器初始化
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_MPI_EN > 0

VOID  _DpmaInit (VOID)
{
#if LW_CFG_MAX_MPDPMAS == 1

    REGISTER PLW_CLASS_DPMA         pdpmaTemp1;
    
    _G_resrcDpma.RESRC_pmonoFreeHeader = &_G_dpmaBuffer[0].DPMA_monoResrcList;
    
    pdpmaTemp1 = &_G_dpmaBuffer[0];
    pdpmaTemp1->DPMA_usIndex = 0;
    
    _INIT_LIST_MONO_HEAD(_G_resrcDpma.RESRC_pmonoFreeHeader);           /*  仅一个节点                  */
    
    _G_resrcDpma.RESRC_pmonoFreeTail = _G_resrcDpma.RESRC_pmonoFreeHeader;

#else
    
    REGISTER ULONG                  ulI;
    REGISTER PLW_LIST_MONO          pmonoTemp1;
    REGISTER PLW_LIST_MONO          pmonoTemp2;
    REGISTER PLW_CLASS_DPMA         pdpmaTemp1;
    REGISTER PLW_CLASS_DPMA         pdpmaTemp2;
    
    _G_resrcDpma.RESRC_pmonoFreeHeader = &_G_dpmaBuffer[0].DPMA_monoResrcList;
    
    pdpmaTemp1 = &_G_dpmaBuffer[0];                                     /*  指向缓冲池首地址            */
    pdpmaTemp2 = &_G_dpmaBuffer[1];
    
    for (ulI = 0; ulI < (LW_CFG_MAX_MPDPMAS - 1); ulI++) {
        pdpmaTemp1->DPMA_usIndex = (UINT16)ulI;
        
        pmonoTemp1 = &pdpmaTemp1->DPMA_monoResrcList;
        pmonoTemp2 = &pdpmaTemp2->DPMA_monoResrcList;
        
        _LIST_MONO_LINK(pmonoTemp1, pmonoTemp2);
        
        pdpmaTemp1++;
        pdpmaTemp2++;
    }
    
    pdpmaTemp1->DPMA_usIndex = (UINT16)ulI;
    
    pmonoTemp1 = &pdpmaTemp1->DPMA_monoResrcList;
    
    _INIT_LIST_MONO_HEAD(pmonoTemp1);
    
    _G_resrcDpma.RESRC_pmonoFreeTail = pmonoTemp1;                      /*  last node                   */
#endif                                                                  /*  LW_CFG_MAX_MPDPMAS == 1     */

    _G_resrcDpma.RESRC_uiUsed    = 0;
    _G_resrcDpma.RESRC_uiMaxUsed = 0;
}
/*********************************************************************************************************
** 函数名称: _Allocate_Dpma_Object
** 功能描述: 从空闲 DPMA 控件池中取出一个空闲 DPMA
** 输　入  : 
** 输　出  : 获得的 DPMA 地址，失败返回 NULL
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PLW_CLASS_DPMA  _Allocate_Dpma_Object (VOID)
{
    REGISTER PLW_LIST_MONO         pmonoFree;
    REGISTER PLW_CLASS_DPMA        pdpma;
    
    if (_LIST_MONO_IS_EMPTY(_G_resrcDpma.RESRC_pmonoFreeHeader)) {
        return  (LW_NULL);
    }
    
    pmonoFree = _list_mono_allocate_seq(&_G_resrcDpma.RESRC_pmonoFreeHeader, 
                                        &_G_resrcDpma.RESRC_pmonoFreeTail);
                                                                        /*  获得资源                    */
    pdpma     = _LIST_ENTRY(pmonoFree, LW_CLASS_DPMA, DPMA_monoResrcList);
    
    _G_resrcDpma.RESRC_uiUsed++;
    if (_G_resrcDpma.RESRC_uiUsed > _G_resrcDpma.RESRC_uiMaxUsed) {
        _G_resrcDpma.RESRC_uiMaxUsed = _G_resrcDpma.RESRC_uiUsed;
    }

    return  (pdpma);
}
/*********************************************************************************************************
** 函数名称: _Free_Dpma_Object
** 功能描述: 将 Dpma 控制块交还缓冲池
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _Free_Dpma_Object (PLW_CLASS_DPMA  pdpma)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    pmonoFree = &pdpma->DPMA_monoResrcList;
    
    _list_mono_free_seq(&_G_resrcDpma.RESRC_pmonoFreeHeader, 
                        &_G_resrcDpma.RESRC_pmonoFreeTail, 
                        pmonoFree);
                        
    _G_resrcDpma.RESRC_uiUsed--;
}

#endif                                                                  /*  LW_CFG_MPI_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/

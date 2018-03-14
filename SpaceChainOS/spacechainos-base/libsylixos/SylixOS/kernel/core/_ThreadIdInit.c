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
** 文   件   名: _ThreadIdInit.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 这是系统线程ID初始化函数库。

** BUG:
2013.05.07  不再区分 THREAD ID 与 TCB, 这里直接初始化 TCB 池.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _ThreadIdInit
** 功能描述: 初始化线程ID    一共初始化 LW_CFG_MAX_THREADS 个节点
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _ThreadIdInit (VOID)
{
    REGISTER ULONG                 ulI;
    
    REGISTER PLW_CLASS_TCB         ptcbTemp1;
    REGISTER PLW_CLASS_TCB         ptcbTemp2;
    
    REGISTER PLW_LIST_MONO         pmonoTemp1;
    REGISTER PLW_LIST_MONO         pmonoTemp2;
    
    _K_resrcTcb.RESRC_pmonoFreeHeader = &_K_tcbBuffer[0].TCB_monoResrcList;  
                                                                        /*  设置资源表头                */
    ptcbTemp1 = &_K_tcbBuffer[0];
    ptcbTemp2 = &_K_tcbBuffer[1];
    
    for (ulI = 0; ulI < (LW_CFG_MAX_THREADS - 1); ulI++) {
        pmonoTemp1 = &ptcbTemp1->TCB_monoResrcList;
        pmonoTemp2 = &ptcbTemp2->TCB_monoResrcList;
        LW_SPIN_INIT(&ptcbTemp1->TCB_slLock);
        
        ptcbTemp1->TCB_usIndex = (UINT16)ulI;                           /*  设置缓冲池内部编号          */
         
        _LIST_MONO_LINK(pmonoTemp1, pmonoTemp2);
        
        ptcbTemp1++;
        ptcbTemp2++;
    }
    
    pmonoTemp1 = &ptcbTemp1->TCB_monoResrcList;
    LW_SPIN_INIT(&ptcbTemp1->TCB_slLock);
    
    ptcbTemp1->TCB_usIndex = (UINT16)ulI;
    
    _INIT_LIST_MONO_HEAD(pmonoTemp1);                                   /*  设置为最后一个节点          */
    
    _K_resrcTcb.RESRC_pmonoFreeTail = pmonoTemp1;
    
    _K_resrcTcb.RESRC_uiUsed    = 0;
    _K_resrcTcb.RESRC_uiMaxUsed = 0;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: _RmsInit.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 16 日
**
** 描        述: RMS 初始化函数库。
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _RmsInit
** 功能描述: RMS 初始化
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _RmsInit (VOID)
{
#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)
#if LW_CFG_MAX_RMSS == 1

    REGISTER PLW_CLASS_RMS          prmsTemp1;
    
    _K_resrcRms.RESRC_pmonoFreeHeader = &_K_rmsBuffer[0].RMS_monoResrcList;
    
    prmsTemp1 = &_K_rmsBuffer[0];
    
    prmsTemp1->RMS_ucType  = LW_RMS_UNUSED;
    prmsTemp1->RMS_usIndex = 0;
    
    _INIT_LIST_MONO_HEAD(_K_resrcRms.RESRC_pmonoFreeHeader);
    
    _K_resrcRms.RESRC_pmonoFreeTail = _K_resrcRms.RESRC_pmonoFreeHeader;
    
#else
    
    REGISTER ULONG                  ulI;
    REGISTER PLW_LIST_MONO          pmonoTemp1;
    REGISTER PLW_LIST_MONO          pmonoTemp2;
    REGISTER PLW_CLASS_RMS          prmsTemp1;
    REGISTER PLW_CLASS_RMS          prmsTemp2;
    
    _K_resrcRms.RESRC_pmonoFreeHeader = &_K_rmsBuffer[0].RMS_monoResrcList;
    
    prmsTemp1 = &_K_rmsBuffer[0];
    prmsTemp2 = &_K_rmsBuffer[1];
    
    for (ulI = 0; ulI < (LW_CFG_MAX_RMSS - 1); ulI++) {
        prmsTemp1->RMS_ucType  = LW_RMS_UNUSED;
        prmsTemp1->RMS_usIndex = (UINT16)ulI;
        
        pmonoTemp1 = &prmsTemp1->RMS_monoResrcList;
        pmonoTemp2 = &prmsTemp2->RMS_monoResrcList;
        
        _LIST_MONO_LINK(pmonoTemp1, pmonoTemp2);
        
        prmsTemp1++;
        prmsTemp2++;
    }
    
    prmsTemp1->RMS_ucType  = LW_RMS_UNUSED;
    prmsTemp1->RMS_usIndex = (UINT16)ulI;
    
    pmonoTemp1 = &prmsTemp1->RMS_monoResrcList;
    
    _INIT_LIST_MONO_HEAD(pmonoTemp1);
    
    _K_resrcRms.RESRC_pmonoFreeTail = pmonoTemp1;
#endif                                                                  /*  LW_CFG_MAX_RMSS == 1        */

    _K_resrcRms.RESRC_uiUsed    = 0;
    _K_resrcRms.RESRC_uiMaxUsed = 0;

#endif                                                                  /*  (LW_CFG_RMS_EN > 0)         */
                                                                        /*  (LW_CFG_MAX_RMSS > 0)       */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

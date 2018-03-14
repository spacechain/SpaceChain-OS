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
** 文   件   名: _EventSetInit.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: 事件集初始化。

** BUG
2007.01.10  没有初始化 index
2007.01.10  当 LW_CFG_MAX_EVENTSETS == 1 时 _K_pmonoEventSetFreeHeader->EVENTSET_usIndex = 0
2007.04.12  当 LW_CFG_MAX_EVENTSETS == 1 时 _INIT_LIST_MONO_HEAD() 参数错误
2009.07.28  加入对自旋锁的初始化.  
2013.11.14  使用对象资源管理器结构管理空闲资源.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _EventSetInit
** 功能描述: 事件集初始化。
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _EventSetInit (VOID)
{
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)

#if LW_CFG_MAX_EVENTSETS == 1
    REGISTER PLW_CLASS_EVENTSET     pesTemp1;
    
    _K_resrcEventSet.RESRC_pmonoFreeHeader = &_K_esBuffer[0].EVENTSET_monoResrcList;
    
    pesTemp1 = &_K_esBuffer[0];
    
    pesTemp1->EVENTSET_ucType        = LW_TYPE_EVENT_UNUSED;
    pesTemp1->EVENTSET_plineWaitList = LW_NULL;
    pesTemp1->EVENTSET_usIndex       = 0;
    
    _INIT_LIST_MONO_HEAD(_K_resrcEventSet.RESRC_pmonoFreeHeader);
    
    _K_resrcEventSet.RESRC_pmonoFreeTail = _K_resrcEventSet.RESRC_pmonoFreeHeader;
    
#else
    REGISTER ULONG                  ulI;
    REGISTER PLW_LIST_MONO          pmonoTemp1;
    REGISTER PLW_LIST_MONO          pmonoTemp2;
    REGISTER PLW_CLASS_EVENTSET     pesTemp1;
    REGISTER PLW_CLASS_EVENTSET     pesTemp2;
    
    _K_resrcEventSet.RESRC_pmonoFreeHeader = &_K_esBuffer[0].EVENTSET_monoResrcList;
    
    pesTemp1 = &_K_esBuffer[0];
    pesTemp2 = &_K_esBuffer[1];
    
    for (ulI = 0; ulI < ((LW_CFG_MAX_EVENTSETS) - 1); ulI++) {
        pesTemp1->EVENTSET_ucType        = LW_TYPE_EVENT_UNUSED;
        pesTemp1->EVENTSET_plineWaitList = LW_NULL;
        pesTemp1->EVENTSET_usIndex       = (UINT16)ulI;
        
        pmonoTemp1 = &pesTemp1->EVENTSET_monoResrcList;
        pmonoTemp2 = &pesTemp2->EVENTSET_monoResrcList;
        
        _LIST_MONO_LINK(pmonoTemp1, pmonoTemp2);
        
        pesTemp1++;
        pesTemp2++;
    }
    
    pesTemp1->EVENTSET_ucType        = LW_TYPE_EVENT_UNUSED;
    pesTemp1->EVENTSET_plineWaitList = LW_NULL;
    pesTemp1->EVENTSET_usIndex       = (UINT16)ulI;
        
    pmonoTemp1 = &pesTemp1->EVENTSET_monoResrcList;
    
    _INIT_LIST_MONO_HEAD(pmonoTemp1);
    
    _K_resrcEventSet.RESRC_pmonoFreeTail = pmonoTemp1;
    
#endif                                                                  /*  LW_CFG_MAX_EVENTSETS == 1   */

    _K_resrcEventSet.RESRC_uiUsed    = 0;
    _K_resrcEventSet.RESRC_uiMaxUsed = 0;
#endif                                                                  /*  (LW_CFG_EVENTSET_EN > 0)    */
                                                                        /*  (LW_CFG_MAX_EVENTSETS > 0)  */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

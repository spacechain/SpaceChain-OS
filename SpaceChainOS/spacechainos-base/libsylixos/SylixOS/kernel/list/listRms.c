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
** 文   件   名: listRms.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 01 月 22 日
**
** 描        述: 这是系统 RMS 资源链表操作。
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: _Allocate_Rms_Object
** 功能描述: 从空闲 RMS 控件池中取出一个空闲 RMS
** 输　入  : 
** 输　出  : 获得的 RMS 地址，失败返回 NULL
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if	(LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)

PLW_CLASS_RMS  _Allocate_Rms_Object (VOID)
{
    REGISTER PLW_LIST_MONO             pmonoFree;
    REGISTER PLW_CLASS_RMS             prmsFree;
    
    if (_LIST_MONO_IS_EMPTY(_K_resrcRms.RESRC_pmonoFreeHeader)) {
        return  (LW_NULL);
    }
    
    pmonoFree = _list_mono_allocate_seq(&_K_resrcRms.RESRC_pmonoFreeHeader, 
                                        &_K_resrcRms.RESRC_pmonoFreeTail);
                                                                        /*  获得资源                    */
    prmsFree  = _LIST_ENTRY(pmonoFree, LW_CLASS_RMS, 
                            RMS_monoResrcList);                         /*  获得资源表容器地址          */
    
    _K_resrcRms.RESRC_uiUsed++;
    if (_K_resrcRms.RESRC_uiUsed > _K_resrcRms.RESRC_uiMaxUsed) {
        _K_resrcRms.RESRC_uiMaxUsed = _K_resrcRms.RESRC_uiUsed;
    }
    
    return  (prmsFree);
}
/*********************************************************************************************************
** 函数名称: _Free_Rms_Object
** 功能描述: 将 Rms 控制块交还缓冲池
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _Free_Rms_Object (PLW_CLASS_RMS    prmsFree)
{
    REGISTER PLW_LIST_MONO    pmonoFree;
    
    pmonoFree = &prmsFree->RMS_monoResrcList;
    
    _list_mono_free_seq(&_K_resrcRms.RESRC_pmonoFreeHeader, 
                        &_K_resrcRms.RESRC_pmonoFreeTail, 
                        pmonoFree);
                        
    _K_resrcRms.RESRC_uiUsed--;
}

#endif                                                                  /*  (LW_CFG_RMS_EN > 0)         */
                                                                        /*  (LW_CFG_MAX_RMSS > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

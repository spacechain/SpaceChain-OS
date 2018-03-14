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
** 文   件   名: pmAdapter.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 07 月 19 日
**
** 描        述: 电源管理适配器.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#define  __POWERM_MAIN_FILE
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if LW_CFG_POWERM_EN > 0
/*********************************************************************************************************
** 函数名称: API_PowerMAdapterCreate
** 功能描述: 创建一个电源管理适配器
** 输　入  : pcName        电源管理适配器的名字
**           uiMaxChan     电源管理适配器最大通道号
**           pmafuncs      电源管理适配器操作函数
** 输　出  : 电源管理适配器
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PLW_PM_ADAPTER  API_PowerMAdapterCreate (CPCHAR  pcName, UINT  uiMaxChan, PLW_PMA_FUNCS  pmafuncs)
{
    PLW_PM_ADAPTER  pmadapter;
    
    if (!pcName || !uiMaxChan || !pmafuncs) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    pmadapter = (PLW_PM_ADAPTER)__SHEAP_ALLOC(sizeof(LW_PM_ADAPTER) + lib_strlen(pcName));
    if (pmadapter == LW_NULL) {
        _ErrorHandle(ERROR_POWERM_FULL);
        return  (LW_NULL);
    }
    
    pmadapter->PMA_uiMaxChan = uiMaxChan;
    pmadapter->PMA_pmafunc   = pmafuncs;
    lib_strcpy(pmadapter->PMA_cName, pcName);
    
    __POWERM_LOCK();
    _List_Line_Add_Ahead(&pmadapter->PMA_lineManage, &_G_plinePMAdapter);
    __POWERM_UNLOCK();
    
    return  (pmadapter);
}
/*********************************************************************************************************
** 函数名称: API_PowerMAdapterDelete
** 功能描述: 删除一个电源管理适配器 (不推荐使用此函数)
** 输　入  : pmadapter     电源管理适配器
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_PowerMAdapterDelete (PLW_PM_ADAPTER  pmadapter)
{
    if (!pmadapter) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __POWERM_LOCK();
    _List_Line_Del(&pmadapter->PMA_lineManage, &_G_plinePMAdapter);
    __POWERM_UNLOCK();
    
    __SHEAP_FREE(pmadapter);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PowerMAdapterFind
** 功能描述: 查询一个电源管理适配器
** 输　入  : pcName        电源管理适配器的名字
** 输　出  : 电源管理适配器
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PLW_PM_ADAPTER  API_PowerMAdapterFind (CPCHAR  pcName)
{
    PLW_LIST_LINE   plineTemp;
    PLW_PM_ADAPTER  pmadapter;
    
    if (!pcName) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    __POWERM_LOCK();
    for (plineTemp  = _G_plinePMAdapter;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pmadapter = _LIST_ENTRY(plineTemp, LW_PM_ADAPTER, PMA_lineManage);
        if (lib_strcmp(pmadapter->PMA_cName, pcName) == 0) {
            break;
        }
    }
    __POWERM_UNLOCK();
    
    if (plineTemp) {
        return  (pmadapter);
    
    } else {
        return  (LW_NULL);
    }
}

#endif                                                                  /*  LW_CFG_POWERM_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: pmDev.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 07 月 19 日
**
** 描        述: 电源管理设备接口.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if LW_CFG_POWERM_EN > 0
/*********************************************************************************************************
** 函数名称: API_PowerMDevInit
** 功能描述: 初始化电源管理设备节点
** 输　入  : pmdev         电源管理, 设备节点
**           pmadapter     电源管理适配器
**           uiChan        通道号
**           pmdfunc       设备节电操作函数
** 输　出  : ERROR ok OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_PowerMDevInit (PLW_PM_DEV  pmdev,  PLW_PM_ADAPTER  pmadapter, 
                        UINT        uiChan, PLW_PMD_FUNCS   pmdfunc)
{
    if (!pmdev || !pmadapter) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (uiChan >= pmadapter->PMA_uiMaxChan) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    pmdev->PMD_pmadapter = pmadapter;
    pmdev->PMD_uiChannel = uiChan;
    pmdev->PMD_uiStatus  = LW_PMD_STAT_NOR;
    pmdev->PMD_pmdfunc   = pmdfunc;
    lib_bzero(&pmdev->PMD_wunTimer, sizeof(LW_CLASS_WAKEUP_NODE));
    
    __POWERM_LOCK();
    _List_Line_Add_Ahead(&pmdev->PMD_lineManage, &_G_plinePMDev);
    __POWERM_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PowerMDevTerm
** 功能描述: 结束电源管理设备节点
** 输　入  : pmdev         电源管理, 设备节点
** 输　出  : ERROR ok OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_PowerMDevTerm (PLW_PM_DEV  pmdev)
{
    if (!pmdev) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __POWERM_LOCK();
    _List_Line_Del(&pmdev->PMD_lineManage, &_G_plinePMDev);
    __POWERM_UNLOCK();

    return  (API_PowerMDevWatchDogOff(pmdev));
}
/*********************************************************************************************************
** 函数名称: API_PowerMDevOn
** 功能描述: 电源管理, 设备节点第一次打开需要调用此函数
** 输　入  : pmdev         电源管理, 设备节点
** 输　出  : ERROR ok OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_PowerMDevOn (PLW_PM_DEV  pmdev)
{
    PLW_PM_ADAPTER  pmadapter;
    INT             iRet = PX_ERROR;
    
    if (!pmdev) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pmadapter = pmdev->PMD_pmadapter;
    if (pmadapter && 
        pmadapter->PMA_pmafunc &&
        pmadapter->PMA_pmafunc->PMAF_pfuncOn) {
        iRet = pmadapter->PMA_pmafunc->PMAF_pfuncOn(pmadapter, pmdev);
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_PowerMDevOff
** 功能描述: 电源管理, 设备节点最后一次关闭需要调用此函数
** 输　入  : pmdev         电源管理, 设备节点
** 输　出  : ERROR ok OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_PowerMDevOff (PLW_PM_DEV  pmdev)
{
    PLW_PM_ADAPTER  pmadapter;
    INT             iRet = PX_ERROR;
    
    if (!pmdev) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pmadapter = pmdev->PMD_pmadapter;
    if (pmadapter && 
        pmadapter->PMA_pmafunc &&
        pmadapter->PMA_pmafunc->PMAF_pfuncOff) {
        iRet = pmadapter->PMA_pmafunc->PMAF_pfuncOff(pmadapter, pmdev);
    }
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_POWERM_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/

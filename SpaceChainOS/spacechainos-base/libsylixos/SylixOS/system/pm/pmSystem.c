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
** 文   件   名: pmSystem.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 07 月 19 日
**
** 描        述: 电源管理系统接口.
**
** 注        意: 以下 API 不可重入, 需要单线程管理.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if LW_CFG_POWERM_EN > 0
/*********************************************************************************************************
  系统状态查询
*********************************************************************************************************/
BOOL    _G_bPowerSavingMode = LW_FALSE;
/*********************************************************************************************************
** 函数名称: API_PowerMSuspend
** 功能描述: 系统进入休眠模式.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  API_PowerMSuspend (VOID)
{
    PLW_LIST_LINE   plineTemp;
    PLW_PM_DEV      pmdev;
    
    __POWERM_LOCK();
    for (plineTemp  = _G_plinePMDev;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pmdev = _LIST_ENTRY(plineTemp, LW_PM_DEV, PMD_lineManage);
        if (pmdev->PMD_pmdfunc && 
            pmdev->PMD_pmdfunc->PMDF_pfuncSuspend) {
            pmdev->PMD_pmdfunc->PMDF_pfuncSuspend(pmdev);
        }
    }
    __POWERM_UNLOCK();
    
    API_KernelSuspend();
}
/*********************************************************************************************************
** 函数名称: API_PowerMResume
** 功能描述: 系统从休眠模式唤醒.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  API_PowerMResume (VOID)
{
    PLW_LIST_LINE   plineTemp;
    PLW_PM_DEV      pmdev;
    
    __POWERM_LOCK();
    for (plineTemp  = _G_plinePMDev;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pmdev = _LIST_ENTRY(plineTemp, LW_PM_DEV, PMD_lineManage);
        if (pmdev->PMD_pmdfunc && 
            pmdev->PMD_pmdfunc->PMDF_pfuncResume) {
            pmdev->PMD_pmdfunc->PMDF_pfuncResume(pmdev);
        }
    }
    __POWERM_UNLOCK();
    
    API_KernelResume();
}
/*********************************************************************************************************
** 函数名称: API_PowerMCpuSet
** 功能描述: 设置 CPU 节电参数.
** 输　入  : ulNCpus       运行的 CPU 个数 (必须 >= 1)
**           uiPowerLevel  CPU 运行的能级
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  API_PowerMCpuSet (ULONG  ulNCpus, UINT  uiPowerLevel)
{
    PLW_LIST_LINE   plineTemp;
    PLW_PM_DEV      pmdev;
    UINT            uiOldPowerLevel;

#if LW_CFG_SMP_EN > 0
    ULONG           i;
    ULONG           ulActCnt = 0;
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    if (ulNCpus == 0) {
        _ErrorHandle(EINVAL);
        return;
    }

    if (ulNCpus > LW_NCPUS) {
        ulNCpus = LW_NCPUS;
    }

#if LW_CFG_SMP_EN > 0
    LW_CPU_FOREACH (i) {
        if (API_CpuIsUp(i)) {
            ulActCnt++;
        }
    }
    if (ulActCnt > ulNCpus) {                                           /*  需要关闭一些 CPU            */
#if LW_CFG_SMP_CPU_DOWN_EN > 0
        ULONG   ulDownCnt = ulActCnt - ulNCpus;
        LW_CPU_FOREACH_EXCEPT (i, 0) {
            if (API_CpuIsUp(i)) {
                API_CpuDown(i);
                ulDownCnt--;
            }
            if (ulDownCnt == 0) {
                break;
            }
        }
#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */

    } else if (ulActCnt < ulNCpus) {                                    /*  需要打开一些 CPU            */
        ULONG   ulUpCnt = ulNCpus - ulActCnt;
        LW_CPU_FOREACH_EXCEPT (i, 0) {
            if (!API_CpuIsUp(i)) {
                API_CpuUp(i);
                ulUpCnt--;
            }
            if (ulUpCnt == 0) {
                break;
            }
        }
    }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    API_CpuPowerGet(&uiOldPowerLevel);
    if (uiOldPowerLevel != uiPowerLevel) {
        API_CpuPowerSet(uiPowerLevel);
        
        __POWERM_LOCK();
        for (plineTemp  = _G_plinePMDev;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            pmdev = _LIST_ENTRY(plineTemp, LW_PM_DEV, PMD_lineManage);
            if (pmdev->PMD_pmdfunc && 
                pmdev->PMD_pmdfunc->PMDF_pfuncCpuPower) {
                pmdev->PMD_pmdfunc->PMDF_pfuncCpuPower(pmdev);
            }
        }
        __POWERM_UNLOCK();
    }
}
/*********************************************************************************************************
** 函数名称: API_PowerMCpuGet
** 功能描述: 获取 CPU 节电参数.
** 输　入  : pulNCpus       运行的 CPU 个数
**           puiPowerLevel  CPU 运行的能级
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  API_PowerMCpuGet (ULONG  *pulNCpus, UINT  *puiPowerLevel)
{
#if LW_CFG_SMP_EN > 0
    ULONG   i;
    ULONG   ulActCnt = 0;

    if (pulNCpus) {
        LW_CPU_FOREACH (i) {
            if (API_CpuIsUp(i)) {
                ulActCnt++;
            }
        }
        *pulNCpus = ulActCnt;
    }
#else
    if (pulNCpus) {
        *pulNCpus = 1ul;
    }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    if (puiPowerLevel) {
        API_CpuPowerGet(puiPowerLevel);
    }
}
/*********************************************************************************************************
** 函数名称: API_PowerMSavingEnter
** 功能描述: 系统进入省电模式.
** 输　入  : ulNCpus       保留运行的 CPU 个数 (必须 >= 1)
**           uiPowerLevel  CPU 运行的能级
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  API_PowerMSavingEnter (ULONG  ulNCpus, UINT  uiPowerLevel)
{
    PLW_LIST_LINE   plineTemp;
    PLW_PM_DEV      pmdev;
    
    if (ulNCpus == 0) {
        _ErrorHandle(EINVAL);
        return;
    }
    
    __POWERM_LOCK();
    for (plineTemp  = _G_plinePMDev;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pmdev = _LIST_ENTRY(plineTemp, LW_PM_DEV, PMD_lineManage);
        if (pmdev->PMD_pmdfunc && 
            pmdev->PMD_pmdfunc->PMDF_pfuncPowerSavingEnter) {
            pmdev->PMD_pmdfunc->PMDF_pfuncPowerSavingEnter(pmdev);
        }
    }
    _G_bPowerSavingMode = LW_TRUE;
    __POWERM_UNLOCK();
    
    API_PowerMCpuSet(ulNCpus, uiPowerLevel);
}
/*********************************************************************************************************
** 函数名称: API_PowerMSavingExit
** 功能描述: 系统退出省电模式.
** 输　入  : ulNCpus       运行的 CPU 个数 (必须 >= 1)
**           uiPowerLevel  CPU 运行的能级
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  API_PowerMSavingExit (ULONG  ulNCpus, UINT  uiPowerLevel)
{
    PLW_LIST_LINE   plineTemp;
    PLW_PM_DEV      pmdev;
    
    if (ulNCpus == 0) {
        _ErrorHandle(EINVAL);
        return;
    }
    
    __POWERM_LOCK();
    for (plineTemp  = _G_plinePMDev;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pmdev = _LIST_ENTRY(plineTemp, LW_PM_DEV, PMD_lineManage);
        if (pmdev->PMD_pmdfunc && 
            pmdev->PMD_pmdfunc->PMDF_pfuncPowerSavingExit) {
            pmdev->PMD_pmdfunc->PMDF_pfuncPowerSavingExit(pmdev);
        }
    }
    _G_bPowerSavingMode = LW_FALSE;
    __POWERM_UNLOCK();
    
    API_PowerMCpuSet(ulNCpus, uiPowerLevel);
}

#endif                                                                  /*  LW_CFG_POWERM_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/

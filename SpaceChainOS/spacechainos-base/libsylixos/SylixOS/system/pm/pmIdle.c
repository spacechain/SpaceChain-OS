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
** 文   件   名: pmIdle.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 07 月 19 日
**
** 描        述: 电源管理设备空闲时间管理接口.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if LW_CFG_POWERM_EN > 0
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_CLASS_WAKEUP  _G_wuPowerM;
/*********************************************************************************************************
** 函数名称: _PowerMThread
** 功能描述: 电源管理线程
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static  PVOID  _PowerMThread (PVOID  pvArg)
{
    PLW_PM_DEV              pmdev;
    PLW_CLASS_WAKEUP_NODE   pwun;

    for (;;) {
        ULONG   ulCounter = LW_TICK_HZ;
        
        __POWERM_LOCK();                                                /*  上锁                        */
                           
        __WAKEUP_PASS_FIRST(&_G_wuPowerM, pwun, ulCounter);
        
        pmdev = _LIST_ENTRY(pwun, LW_PM_DEV, PMD_wunTimer);
        
        _WakeupDel(&_G_wuPowerM, pwun);
        
        if (pmdev->PMD_pmdfunc &&
            pmdev->PMD_pmdfunc->PMDF_pfuncIdleEnter) {
            pmdev->PMD_uiStatus = LW_PMD_STAT_IDLE;
            __POWERM_UNLOCK();                                          /*  暂时解锁                    */
            pmdev->PMD_pmdfunc->PMDF_pfuncIdleEnter(pmdev);
            __POWERM_LOCK();                                            /*  上锁                        */
        }
        
        __WAKEUP_PASS_SECOND();
        
        __WAKEUP_PASS_END();
        
        __POWERM_UNLOCK();                                              /*  解锁                        */
        
        API_TimeSleep(LW_TICK_HZ);                                      /*  等待一秒                    */
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: _PowerMInit
** 功能描述: 初始化电源管理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _PowerMInit (VOID)
{
    LW_CLASS_THREADATTR       threadattr;
    
    _G_ulPowerMLock = API_SemaphoreMCreate("power_lock", LW_PRIO_DEF_CEILING,
                               LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                               LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                               LW_NULL);                                /*  建立锁信号量                */

    if (!_G_ulPowerMLock) {
        return;
    }
    
    API_ThreadAttrBuild(&threadattr, 
                        LW_CFG_THREAD_POWERM_STK_SIZE, 
                        LW_PRIO_T_POWER, 
                        LW_OPTION_THREAD_STK_CHK | LW_OPTION_THREAD_SAFE | LW_OPTION_OBJECT_GLOBAL, 
                        LW_NULL);
    
	_S_ulPowerMId = API_ThreadCreate("t_power", 
	                                 _PowerMThread,
	                                 &threadattr,
	                                 LW_NULL);                          /*  建立管理线程                */
}
/*********************************************************************************************************
** 函数名称: API_PowerMDevSetWatchDog
** 功能描述: 设备电源空闲时间管理激活
** 输　入  : pmdev         电源管理, 设备节点
**           ulSecs        经过指定的秒数, 设备将进入 idle 模式.
** 输　出  : ERROR ok OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_PowerMDevSetWatchDog (PLW_PM_DEV  pmdev, ULONG  ulSecs)
{
    BOOL    bBecomeNor = LW_FALSE;

    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    if (!pmdev || !ulSecs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __POWERM_LOCK();                                                    /*  上锁                        */
    if (pmdev->PMD_bInQ) {
        _WakeupDel(&_G_wuPowerM, &pmdev->PMD_wunTimer);
    }
    
    pmdev->PMD_ulCounter = ulSecs;                                      /*  复位定时器                  */
    
    _WakeupAdd(&_G_wuPowerM, &pmdev->PMD_wunTimer);
    
    if (pmdev->PMD_uiStatus == LW_PMD_STAT_IDLE) {
        pmdev->PMD_uiStatus =  LW_PMD_STAT_NOR;
        bBecomeNor          =  LW_TRUE;
    }
    __POWERM_UNLOCK();                                                  /*  解锁                        */
    
    if (bBecomeNor && 
        pmdev->PMD_pmdfunc &&
        pmdev->PMD_pmdfunc->PMDF_pfuncIdleExit) {
        pmdev->PMD_pmdfunc->PMDF_pfuncIdleExit(pmdev);                  /*  退出 idle                   */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PowerMDevGetWatchDog
** 功能描述: 设备电源空闲剩余时间
** 输　入  : pmdev         电源管理, 设备节点
**           pulSecs       设备进入 idle 模式剩余的时间.
** 输　出  : ERROR ok OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_PowerMDevGetWatchDog (PLW_PM_DEV  pmdev, ULONG  *pulSecs)
{
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    if (!pmdev || !pulSecs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __POWERM_LOCK();                                                    /*  上锁                        */
    if (pmdev->PMD_bInQ) {
        _WakeupStatus(&_G_wuPowerM, &pmdev->PMD_wunTimer, pulSecs);
    
    } else {
        *pulSecs = 0ul;
    }
    __POWERM_UNLOCK();                                                  /*  解锁                        */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PowerMDevWatchDogOff
** 功能描述: 设备电源空闲时间管理停止
** 输　入  : pmdev         电源管理, 设备节点
** 输　出  : ERROR ok OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_PowerMDevWatchDogOff (PLW_PM_DEV  pmdev)
{
    BOOL    bBecomeNor = LW_FALSE;

    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    if (!pmdev) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __POWERM_LOCK();                                                    /*  上锁                        */
    if (pmdev->PMD_bInQ) {
        _WakeupDel(&_G_wuPowerM, &pmdev->PMD_wunTimer);
    }
    if (pmdev->PMD_uiStatus == LW_PMD_STAT_IDLE) {
        pmdev->PMD_uiStatus =  LW_PMD_STAT_NOR;
        bBecomeNor          =  LW_TRUE;
    }
    __POWERM_UNLOCK();                                                  /*  解锁                        */
    
    if (bBecomeNor && 
        pmdev->PMD_pmdfunc &&
        pmdev->PMD_pmdfunc->PMDF_pfuncIdleExit) {
        pmdev->PMD_pmdfunc->PMDF_pfuncIdleExit(pmdev);                  /*  退出 idle                   */
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_POWERM_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/

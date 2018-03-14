/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: armSpinlock.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 09 日
**
** 描        述: ARM 体系构架自旋锁驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  spinlock 状态
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#include "armMpCore.h"
/*********************************************************************************************************
  L1 cache 同步请参考: http://www.cnblogs.com/jiayy/p/3246133.html
*********************************************************************************************************/
/*********************************************************************************************************
  spin lock cache 依赖处理
*********************************************************************************************************/
#if LW_CFG_ARM_SPINLOCK_DEP_CACHE > 0
static VOID                   armSpinLockDummy(volatile SPINLOCKTYPE  *psl);
static volatile SPINLOCKTYPE  armSpinTryLockDummy(volatile SPINLOCKTYPE  *psl);
static VOID                   armSpinUnlockDummy(volatile SPINLOCKTYPE  *psl);

static VOID                   (*pfuncArmSpinLock)(volatile SPINLOCKTYPE)    = armSpinLockDummy;
static volatile SPINLOCKTYPE  (*pfuncArmSpinTryLock)(volatile SPINLOCKTYPE) = armSpinTryLockDummy;
static VOID                   (*pfuncArmSpinUnlock)(volatile SPINLOCKTYPE)  = armSpinUnlockDummy;
#endif                                                                  /*  LW_CFG_ARM_SPINLOCK_DEP_C...*/
/*********************************************************************************************************
** 函数名称: armSpinLockDummy
** 功能描述: 空操作
** 输　入  : psl        spinlock 指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_ARM_SPINLOCK_DEP_CACHE > 0

static VOID  armSpinLockDummy (volatile SPINLOCKTYPE  *psl)
{
}
/*********************************************************************************************************
** 函数名称: armSpinTryLockDummy
** 功能描述: 空操作
** 输　入  : psl        spinlock 指针
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static volatile SPINLOCKTYPE  armSpinTryLockDummy (volatile SPINLOCKTYPE  *psl)
{
    return  (0);
}
/*********************************************************************************************************
** 函数名称: armSpinUnlockDummy
** 功能描述: 空操作
** 输　入  : psl        spinlock 指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  armSpinUnlockDummy (volatile SPINLOCKTYPE  *psl)
{
}
/*********************************************************************************************************
** 函数名称: archSpinWork
** 功能描述: spinlock 函数起效
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 主核开启 CACHE 后, BSP 应立即调用此函数, 使 spinlock 生效, 
             从核启动到开启 CACHE 过程中, 不得操作 spinlock.
*********************************************************************************************************/
VOID  archSpinWork (VOID)
{
    pfuncArmSpinUnlock  = armSpinUnlock;
    pfuncArmSpinTryLock = armSpinTryLock;
    pfuncArmSpinLock    = armSpinLock;
}

#endif                                                                  /*  LW_CFG_ARM_SPINLOCK_DEP_C...*/
/*********************************************************************************************************
** 函数名称: archSpinInit
** 功能描述: 初始化一个 spinlock
** 输　入  : psl        spinlock 指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archSpinInit (spinlock_t  *psl)
{
    psl->SL_sltData   = 0;                                              /*  0: 未锁定状态  1: 锁定状态  */
    psl->SL_pcpuOwner = LW_NULL;
    psl->SL_ulCounter = 0ul;                                            /*  重入锁计数                  */
    KN_SMP_WMB();
}
/*********************************************************************************************************
** 函数名称: archSpinDelay
** 功能描述: 等待事件
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archSpinDelay (VOID)
{
    volatile INT  i;

    for (i = 0; i < 3; i++) {
    }
}
/*********************************************************************************************************
** 函数名称: archSpinNotify
** 功能描述: 发送 spin 事件
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archSpinNotify (VOID)
{
}
/*********************************************************************************************************
** 函数名称: archSpinLock
** 功能描述: spinlock 上锁
** 输　入  : psl        spinlock 指针
** 输　出  : 0: 没有获取
**           1: 正常加锁
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  archSpinLock (spinlock_t  *psl)
{
    if (psl->SL_pcpuOwner == LW_CPU_GET_CUR()) {
        psl->SL_ulCounter++;
        _BugFormat((psl->SL_ulCounter > 10), LW_TRUE, 
                   "spinlock RECURSIVE %lu!\r\n", psl->SL_ulCounter);
        return  (1);                                                    /*  重复调用                    */
    }
    
#if LW_CFG_ARM_SPINLOCK_DEP_CACHE > 0
    pfuncArmSpinLock(&psl->SL_sltData);
#else
    armSpinLock(&psl->SL_sltData);
#endif                                                                  /*  LW_CFG_ARM_SPINLOCK_DEP_C...*/
    
    psl->SL_pcpuOwner = LW_CPU_GET_CUR();                               /*  保存当前 CPU                */
    
    return  (1);                                                        /*  加锁成功                    */
}
/*********************************************************************************************************
** 函数名称: archSpinTryLock
** 功能描述: spinlock 试图上锁
** 输　入  : psl        spinlock 指针
** 输　出  : 0: 没有获取
**           1: 正常加锁
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  archSpinTryLock (spinlock_t  *psl)
{
    if (psl->SL_pcpuOwner == LW_CPU_GET_CUR()) {
        psl->SL_ulCounter++;
        _BugFormat((psl->SL_ulCounter > 10), LW_TRUE, 
                   "spinlock RECURSIVE %lu!\r\n", psl->SL_ulCounter);
        return  (1);                                                    /*  重复调用                    */
    }
    
#if LW_CFG_ARM_SPINLOCK_DEP_CACHE > 0
    if (pfuncArmSpinTryLock(&psl->SL_sltData)) 
#else
    if (armSpinTryLock(&psl->SL_sltData)) 
#endif                                                                  /*  LW_CFG_ARM_SPINLOCK_DEP_C...*/
    {                                                                   /*  尝试加锁                    */
        return  (0);
    }
    
    psl->SL_pcpuOwner = LW_CPU_GET_CUR();                               /*  保存当前 CPU                */
    
    return  (1);
}
/*********************************************************************************************************
** 函数名称: archSpinUnlock
** 功能描述: spinlock 解锁
** 输　入  : psl        spinlock 指针
** 输　出  : 0: 没有获取
**           1: 正常解锁
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  archSpinUnlock (spinlock_t  *psl)
{
    if (psl->SL_pcpuOwner != LW_CPU_GET_CUR()) {
        return  (0);                                                    /*  没有权利释放                */
    }
    
    if (psl->SL_ulCounter) {
        psl->SL_ulCounter--;                                            /*  减少重复调用次数            */
        return  (1);
    }

    psl->SL_pcpuOwner = LW_NULL;                                        /*  没有 CPU 获取               */
    KN_SMP_WMB();
    
#if LW_CFG_ARM_SPINLOCK_DEP_CACHE > 0
    pfuncArmSpinUnlock(&psl->SL_sltData);                               /*  解锁                        */
#else
    armSpinUnlock(&psl->SL_sltData);
#endif                                                                  /*  LW_CFG_ARM_SPINLOCK_DEP_C...*/

    return  (1);
}
/*********************************************************************************************************
** 函数名称: archSpinLockRaw
** 功能描述: spinlock 上锁 (不进行重入判断)
** 输　入  : psl        spinlock 指针
** 输　出  : 0: 没有获取
**           1: 正常加锁
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  archSpinLockRaw (spinlock_t  *psl)
{
#if LW_CFG_ARM_SPINLOCK_DEP_CACHE > 0
    pfuncArmSpinLock(&psl->SL_sltData);
#else
    armSpinLock(&psl->SL_sltData);
#endif                                                                  /*  LW_CFG_ARM_SPINLOCK_DEP_C...*/

    return  (1);                                                        /*  加锁成功                    */
}
/*********************************************************************************************************
** 函数名称: archSpinTryLockRaw
** 功能描述: spinlock 试图上锁 (不进行重入判断)
** 输　入  : psl        spinlock 指针
** 输　出  : 0: 没有获取
**           1: 正常加锁
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  archSpinTryLockRaw (spinlock_t  *psl)
{
#if LW_CFG_ARM_SPINLOCK_DEP_CACHE > 0
    if (pfuncArmSpinTryLock(&psl->SL_sltData)) 
#else
    if (armSpinTryLock(&psl->SL_sltData)) 
#endif                                                                  /*  LW_CFG_ARM_SPINLOCK_DEP_C...*/
    {                                                                   /*  尝试加锁                    */
        return  (0);
    }
    
    return  (1);                                                        /*  加锁成功                    */
}
/*********************************************************************************************************
** 函数名称: archSpinUnlockRaw
** 功能描述: spinlock 解锁
** 输　入  : psl        spinlock 指针
** 输　出  : 0: 没有获取
**           1: 正常解锁
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  archSpinUnlockRaw (spinlock_t  *psl)
{
#if LW_CFG_ARM_SPINLOCK_DEP_CACHE > 0
    pfuncArmSpinUnlock(&psl->SL_sltData);
#else
    armSpinUnlock(&psl->SL_sltData);                                    /*  解锁                        */
#endif                                                                  /*  LW_CFG_ARM_SPINLOCK_DEP_C...*/
    
    return  (1);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/

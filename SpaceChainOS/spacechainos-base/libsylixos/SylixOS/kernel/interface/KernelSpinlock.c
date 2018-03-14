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
** 文   件   名: KernelSpinlock.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 02 月 03 日
**
** 描        述: 系统内核自旋锁库.
**
** 注        意: Upon successful completion, these functions shall return zero; otherwise, 
                 an error number shall be returned to indicate the error.
                 
2012.08.08  加入 spinlock irq 功能.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_SpinInit
** 功能描述: 自旋锁初始化
** 输　入  : psl       自旋锁
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SpinInit (spinlock_t *psl)
{
    if (psl == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_SPIN_INIT(psl);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SpinDestory
** 功能描述: 自旋锁删除
** 输　入  : psl       自旋锁
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SpinDestory (spinlock_t *psl)
{
    if (psl == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SpinLock
** 功能描述: 自旋锁 lock
** 输　入  : psl       自旋锁
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SpinLock (spinlock_t *psl)
{
    if (psl == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_SPIN_LOCK(psl);
    
    return  (ERROR_NONE);                                               /*  正常 lock                   */
}
/*********************************************************************************************************
** 函数名称: API_SpinLockIrq
** 功能描述: 自旋锁 lock 同时关闭中断
** 输　入  : psl               自旋锁
**           iregInterLevel    中断控制器上下文
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SpinLockIrq (spinlock_t *psl, INTREG  *iregInterLevel)
{
    if (psl == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_SPIN_LOCK_IRQ(psl, iregInterLevel);
    
    return  (ERROR_NONE);                                               /*  正常 lock                   */
}
/*********************************************************************************************************
** 函数名称: API_SpinLockQuick
** 功能描述: 自旋锁 lock 同时关闭中断 (快速方式)
** 输　入  : psl               自旋锁
**           iregInterLevel    中断控制器上下文
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
** 注  意  : 由于 UNLOCK QUICK 不会尝试调度, 所以 LOCK QUICK 和 UNLOCK QUICK 之间不能还有激活任务或者其他
             改变任务状态的操作, 如果有, 则需要使用 LOCK IRQ 和 UNLOCK IRQ 来取代.
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SpinLockQuick (spinlock_t *psl, INTREG  *iregInterLevel)
{
    if (psl == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_SPIN_LOCK_QUICK(psl, iregInterLevel);
    
    return  (ERROR_NONE);                                               /*  正常 lock                   */
}
/*********************************************************************************************************
** 函数名称: API_SpinTryLock
** 功能描述: trylock 一个自旋锁.
** 输　入  : psl               自旋锁
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SpinTryLock (spinlock_t *psl)
{
    if (psl == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (LW_SPIN_TRYLOCK(psl)) {
        return  (ERROR_NONE);                                           /*  正常 unlock                 */
    
    } else {
        errno = EBUSY;
        return  (EBUSY);
    }
}
/*********************************************************************************************************
** 函数名称: API_SpinTryLockIrq
** 功能描述: trylock 一个自旋锁, 同时关闭中断
** 输　入  : psl               自旋锁
**           iregInterLevel    中断控制器上下文
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SpinTryLockIrq (spinlock_t *psl, INTREG  *iregInterLevel)
{
    if (psl == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (LW_SPIN_TRYLOCK_IRQ(psl, iregInterLevel)) {
        return  (ERROR_NONE);                                           /*  正常 unlock                 */
    
    } else {
        errno = EBUSY;
        return  (EBUSY);
    }
}
/*********************************************************************************************************
** 函数名称: API_SpinUnlock
** 功能描述: unlock 一个自旋锁.
** 输　入  : psl               自旋锁
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SpinUnlock (spinlock_t *psl)
{
    if (psl == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_SPIN_UNLOCK(psl);
    
    return  (ERROR_NONE);                                           /*  正常 unlock                 */
}
/*********************************************************************************************************
** 函数名称: API_SpinUnlockIrq
** 功能描述: unlock 一个自旋锁, 同时打开中断
** 输　入  : psl               自旋锁
**           iregInterLevel    中断控制器上下文
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SpinUnlockIrq (spinlock_t *psl, INTREG  iregInterLevel)
{
    if (psl == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_SPIN_UNLOCK_IRQ(psl, iregInterLevel);
    
    return  (ERROR_NONE);                                               /*  正常 unlock                 */
}
/*********************************************************************************************************
** 函数名称: API_SpinUnlockQuick
** 功能描述: unlock 一个自旋锁, 同时打开中断
** 输　入  : psl               自旋锁
**           iregInterLevel    中断控制器上下文
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SpinUnlockQuick (spinlock_t *psl, INTREG  iregInterLevel)
{
    if (psl == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_SPIN_UNLOCK_QUICK(psl, iregInterLevel);
    
    return  (ERROR_NONE);                                               /*  正常 unlock                 */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: pthread_spinlock.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 30 日
**
** 描        述: pthread 自旋锁兼容库.
**
** 注        意: Upon successful completion, these functions shall return zero; otherwise, 
                 an error number shall be returned to indicate the error.
**
** BUG:
2015.05.15  使用 spin lock task 型自旋锁.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/px_pthread.h"                                      /*  已包含操作系统头文件        */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
** 函数名称: pthread_spin_init
** 功能描述: 创建一个自旋锁.
** 输　入  : pspinlock      自旋锁
**           pshare         共享 (目前无用)
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_spin_init (pthread_spinlock_t  *pspinlock, int  pshare)
{
    if (pspinlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_SPIN_INIT(pspinlock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_spin_destroy
** 功能描述: 销毁一个自旋锁.
** 输　入  : pspinlock      自旋锁
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_spin_destroy (pthread_spinlock_t  *pspinlock)
{
    if (pspinlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_spin_lock
** 功能描述: lock 一个自旋锁. (不能在中断中调用)
** 输　入  : pspinlock      自旋锁
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_spin_lock (pthread_spinlock_t  *pspinlock)
{
    if (pspinlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_SPIN_LOCK_TASK(pspinlock);
    
    return  (ERROR_NONE);                                               /*  正常 lock                   */
}
/*********************************************************************************************************
** 函数名称: pthread_spin_unlock
** 功能描述: unlock 一个自旋锁. (不能在中断中调用)
** 输　入  : pspinlock      自旋锁
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_spin_unlock (pthread_spinlock_t  *pspinlock)
{
    if (pspinlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_SPIN_UNLOCK_TASK(pspinlock);
    
    return  (ERROR_NONE);                                               /*  正常 unlock                 */
}
/*********************************************************************************************************
** 函数名称: pthread_spin_trylock
** 功能描述: trylock 一个自旋锁. (不能在中断中调用)
** 输　入  : pspinlock      自旋锁
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_spin_trylock (pthread_spinlock_t  *pspinlock)
{
    if (pspinlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (LW_SPIN_TRYLOCK_TASK(pspinlock)) {
        return  (ERROR_NONE);                                           /*  正常 unlock                 */
    
    } else {
        errno = EBUSY;
        return  (EBUSY);
    }
}
/*********************************************************************************************************
** 函数名称: pthread_spin_lock_irq_np
** 功能描述: lock 一个自旋锁, 同时锁定中断. (不能在中断中调用)
** 输　入  : pspinlock      自旋锁
**           irqctx         体系结构相关中断状态保存结构 (用户不需要关心具体内容)
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_POSIXEX_EN > 0

LW_API 
int  pthread_spin_lock_irq_np (pthread_spinlock_t  *pspinlock, pthread_int_t *irqctx)
{
    if ((pspinlock == LW_NULL) || (irqctx == NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_SPIN_LOCK_TASK(pspinlock);
    *irqctx = KN_INT_DISABLE();
    
    return  (ERROR_NONE);                                               /*  正常 lock                   */
}
/*********************************************************************************************************
** 函数名称: pthread_spin_unlock_irq_np
** 功能描述: unlock 一个自旋锁, 同时锁定中断. (不能在中断中调用)
** 输　入  : pspinlock      自旋锁
**           irqctx         体系结构相关中断状态保存结构 (用户不需要关心具体内容)
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_spin_unlock_irq_np (pthread_spinlock_t  *pspinlock, pthread_int_t irqctx)
{
    if (pspinlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_SPIN_UNLOCK_TASK(pspinlock);
    KN_INT_ENABLE(irqctx);
    
    return  (ERROR_NONE);                                               /*  正常 unlock                 */
}
/*********************************************************************************************************
** 函数名称: pthread_spin_trylock_irq_np
** 功能描述: trylock 一个自旋锁, 如果成功同时锁定中断. (不能在中断中调用)
** 输　入  : pspinlock      自旋锁
**           irqctx         体系结构相关中断状态保存结构 (用户不需要关心具体内容)
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_spin_trylock_irq_np (pthread_spinlock_t  *pspinlock, pthread_int_t *irqctx)
{
    if ((pspinlock == LW_NULL) || (irqctx == NULL)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (LW_SPIN_TRYLOCK_TASK(pspinlock)) {
        *irqctx = KN_INT_DISABLE();
        return  (ERROR_NONE);                                           /*  正常 unlock                 */
    
    } else {
        errno = EBUSY;
        return  (EBUSY);
    }
}

#endif                                                                  /*  W_CFG_POSIXEX_EN > 0        */
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

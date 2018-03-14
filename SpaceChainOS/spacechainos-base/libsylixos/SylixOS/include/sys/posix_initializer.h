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
** 文   件   名: px_initializer.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 09 月 18 日
**
** 描        述: pthread 应用程序中可使用本方法移植需要静态初始化的 posix 对象.
                 利用对象的构造与析构函数实现
*********************************************************************************************************/

#ifndef __SYS_POSIX_INITIALIZER_H
#define __SYS_POSIX_INITIALIZER_H

#include "SylixOS.h"                                                    /*  操作系统头文件              */

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0

#include "pthread.h"
#include "semaphore.h"

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

/*********************************************************************************************************
  SylixOS 1.0.0-rc35 版本后由于支持了进程资源回收, 所以支持了静态创建, 用户可以不再使用此方法.
*********************************************************************************************************/

/*********************************************************************************************************
  MUTEX INITIALIZER
*********************************************************************************************************/

#define SYLIXOS_INITIALIZER_MUTEX(mutex)    \
        LW_CONSTRUCTOR_BEGIN    \
        LW_LIB_HOOK_STATIC void __init_mutex_##mutex (void) \
        {   \
            pthread_mutex_init(&mutex, (void *)0); \
        }   \
        LW_CONSTRUCTOR_END(__init_mutex_##mutex)    \
        LW_DESTRUCTOR_BEGIN     \
        LW_LIB_HOOK_STATIC void __deinit_mutex_##mutex (void) \
        {   \
            pthread_mutex_destroy(&mutex); \
        }   \
        LW_DESTRUCTOR_END(__deinit_mutex_##mutex)
        
/*********************************************************************************************************
  MUTEX INITIALIZER
*********************************************************************************************************/

#define SYLIXOS_INITIALIZER_COND(cond) \
        LW_CONSTRUCTOR_BEGIN    \
        LW_LIB_HOOK_STATIC void __init_cond_##cond (void) \
        {   \
            pthread_cond_init(&cond, (void *)0); \
        }   \
        LW_CONSTRUCTOR_END(__init_cond_##cond)    \
        LW_DESTRUCTOR_BEGIN     \
        LW_LIB_HOOK_STATIC void __deinit_cond_##cond (void) \
        {   \
            pthread_cond_destroy(&cond); \
        }   \
        LW_DESTRUCTOR_END(__deinit_cond_##cond)
        
/*********************************************************************************************************
  RWLOCK INITIALIZER
*********************************************************************************************************/

#define SYLIXOS_INITIALIZER_RWLOCK(rwlock) \
        LW_CONSTRUCTOR_BEGIN    \
        LW_LIB_HOOK_STATIC void __init_cond_##rwlock (void) \
        {   \
            pthread_rwlock_init(&rwlock, (void *)0); \
        }   \
        LW_CONSTRUCTOR_END(__init_cond_##rwlock)    \
        LW_DESTRUCTOR_BEGIN     \
        LW_LIB_HOOK_STATIC void __deinit_cond_##rwlock (void) \
        {   \
            pthread_rwlock_destroy(&rwlock); \
        }   \
        LW_DESTRUCTOR_END(__deinit_cond_##rwlock)

/*********************************************************************************************************
  SEMAPHORE INITIALIZER
*********************************************************************************************************/

#define SYLIXOS_INITIALIZER_SEMAPHORE(sem) \
        LW_CONSTRUCTOR_BEGIN    \
        LW_LIB_HOOK_STATIC void __init_cond_##sem (void) \
        {   \
            sem_init(&sem, 0, 0); \
        }   \
        LW_CONSTRUCTOR_END(__init_cond_##sem)    \
        LW_DESTRUCTOR_BEGIN     \
        LW_LIB_HOOK_STATIC void __deinit_cond_##sem (void) \
        {   \
            sem_destroy(&sem); \
        }   \
        LW_DESTRUCTOR_END(__deinit_cond_##sem)

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
#endif                                                                  /*  __SYS_POSIX_INITIALIZER_H   */
/*********************************************************************************************************
  END
*********************************************************************************************************/

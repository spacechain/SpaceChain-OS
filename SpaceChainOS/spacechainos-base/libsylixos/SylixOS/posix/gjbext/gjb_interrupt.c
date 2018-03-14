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
** 文   件   名: gjb_interrupt.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 04 月 13 日
**
** 描        述: GJB7714 扩展接口中断管理相关部分.
**
** 注        意: SylixOS 不推荐使用 GJB7714 中断管理接口.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "unistd.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if (LW_CFG_POSIX_EN > 0) && (LW_CFG_GJB7714_EN > 0)
#include "../include/px_gjbext.h"
/*********************************************************************************************************
** 函数名称: int_install_handler
** 功能描述: 安装中断处理函数.
** 输　入  : name      中断名称
**           vecnum    中断向量
**           prio      中断优先级
**           handler   服务函数
**           param     参数
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  int_install_handler (const char  *name, 
                          int          vecnum, 
                          int          prio, 
                          void       (*handler)(void *),
                          void        *param)
{
    ULONG   ulFlag;
    
    if (getpid() > 0) {
        errno = EACCES;
        return  (EACCES);
    }
    
    if (!name || !handler) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (lib_strnlen(name, LW_CFG_OBJECT_NAME_SIZE) >= LW_CFG_OBJECT_NAME_SIZE) {
        errno = ENAMETOOLONG;
        return  (ENAMETOOLONG);
    }
    
    if (API_InterVectorGetFlag((ULONG)vecnum, &ulFlag)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    ulFlag |= LW_IRQ_FLAG_GJB7714;
    
    if (API_InterVectorSetFlag((ULONG)vecnum, ulFlag)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_InterVectorConnect((ULONG)vecnum,
                               (PINT_SVR_ROUTINE)handler,
                               param, name)) {
        ulFlag &= ~LW_IRQ_FLAG_GJB7714;
        API_InterVectorSetFlag((ULONG)vecnum, ulFlag);
        errno = ENOMEM;
        return  (ENOMEM);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: int_install_handler
** 功能描述: 卸载中断处理函数.
** 输　入  : vecnum    中断向量
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  int_uninstall_handler (int   vecnum)
{
    if (getpid() > 0) {
        errno = EACCES;
        return  (EACCES);
    }

    if (API_InterVectorDisconnectEx((ULONG)vecnum, 
                                    LW_NULL, LW_NULL, 
                                    LW_IRQ_DISCONN_ALL)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: int_lock
** 功能描述: 关闭当前 CPU 中断.
** 输　入  : NONE
** 输　出  : 中断状态
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  int_lock (void)
{
    INTREG  intreg;
    
    intreg = KN_INT_DISABLE();
    
    return  ((int)intreg);
}
/*********************************************************************************************************
** 函数名称: int_unlock
** 功能描述: 打开当前 CPU 中断.
** 输　入  : level     中断状态
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
void  int_unlock (int level)
{
    INTREG  intreg = (INTREG)level;
    
    KN_INT_ENABLE(intreg);
}
/*********************************************************************************************************
** 函数名称: int_enable_pic
** 功能描述: 使能指定的 CPU 中断向量.
** 输　入  : irq      中断号
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
void  int_enable_pic (uint32_t irq)
{
    API_InterVectorEnable((ULONG)irq);
}
/*********************************************************************************************************
** 函数名称: int_disable_pic
** 功能描述: 禁能指定的 CPU 中断向量.
** 输　入  : irq      中断号
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
void  int_disable_pic (uint32_t irq)
{
    API_InterVectorDisable((ULONG)irq);
}
/*********************************************************************************************************
** 函数名称: exception_handler_set
** 功能描述: 设置异常处理句柄.
** 输　入  : exc_handler   新的处理函数
** 输　出  : 先早的处理函数
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
EXC_HANDLER  exception_handler_set (EXC_HANDLER exc_handler)
{
    (VOID)exc_handler;
    
    errno = ENOSYS;
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: shared_int_install
** 功能描述: 安装共享中断处理函数.
** 输　入  : vecnum    中断向量
**           handler   服务函数
**           param     参数
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  shared_int_install (int     vecnum, 
                         void  (*handler)(void *),
                         void   *param)
{
    ULONG   ulFlag;
    
    if (getpid() > 0) {
        errno = EACCES;
        return  (PX_ERROR);
    }
    
    if (!handler) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (API_InterVectorGetFlag((ULONG)vecnum, &ulFlag)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    ulFlag |= (LW_IRQ_FLAG_GJB7714 | LW_IRQ_FLAG_QUEUE);
    
    if (API_InterVectorSetFlag((ULONG)vecnum, ulFlag)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (API_InterVectorConnect((ULONG)vecnum,
                               (PINT_SVR_ROUTINE)handler,
                               param, "GJB irq")) {
        ulFlag &= ~LW_IRQ_FLAG_GJB7714;
        API_InterVectorSetFlag((ULONG)vecnum, ulFlag);
        errno = ENOMEM;
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: int_uninstall_handler
** 功能描述: 卸载共享中断处理函数.
** 输　入  : vecnum    中断向量
**           handler   处理函数
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  shared_int_uninstall (int     vecnum, 
                           void  (*handler)(void *))
{
    if (getpid() > 0) {
        errno = EACCES;
        return  (PX_ERROR);
    }

    if (API_InterVectorDisconnectEx((ULONG)vecnum, 
                                    (PINT_SVR_ROUTINE)handler, LW_NULL, 
                                    LW_IRQ_DISCONN_IGNORE_ARG)) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_GJB7714_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: gjb_devrel.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 04 月 13 日
**
** 描        述: GJB7714 扩展接口设备相关部分.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if (LW_CFG_POSIX_EN > 0) && (LW_CFG_GJB7714_EN > 0)
#include "../include/px_gjbext.h"
/*********************************************************************************************************
** 函数名称: global_std_set
** 功能描述: 设置内核全局标准文件描述符映射.
** 输　入  : stdfd         标准文件描述符 STD_IN, STD_OUT, STD_ERR
**           newfd         新的文件描述符
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
void  global_std_set (int  stdfd, int  newfd)
{
    API_IoGlobalStdSet(stdfd, newfd);
}
/*********************************************************************************************************
** 函数名称: global_std_get
** 功能描述: 获取内核全局标准文件描述符映射.
** 输　入  : stdfd         标准文件描述符 STD_IN, STD_OUT, STD_ERR
** 输　出  : 映射的文件描述符
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  global_std_get (int  stdfd)
{
    return  (API_IoGlobalStdGet(stdfd));
}
/*********************************************************************************************************
** 函数名称: register_driver
** 功能描述: 注册驱动程序.
** 输　入  : major              推荐的主设备号
**           driver_table       文件操作组
**           registered_major   注册成功的主设备号
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  register_driver (dev_t                   major, 
                      struct file_operations *driver_table, 
                      dev_t                  *registered_major)
{
    INT    iRet;
    
    (VOID)major;
    
    if (!driver_table || !registered_major) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    iRet = API_IosDrvInstallEx(driver_table);
    if (iRet < 0) {
        return  (PX_ERROR);
    }
    
    *registered_major = (dev_t)iRet;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: unregister_driver
** 功能描述: 卸载驱动程序.
** 输　入  : major              主设备号
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
OS_STATUS  unregister_driver (dev_t  major)
{
    if (!major) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

    if (API_IosDrvRemove((INT)major, LW_TRUE)) {
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: register_device
** 功能描述: 注册设备.
** 输　入  : path               路径
**           dev                主设备号
**           registered         设备结构
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  register_device (const char *path, dev_t  dev, void *registered)
{
    if (!path || !dev || !registered) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    if (API_IosDevAdd((PLW_DEV_HDR)registered, path, (INT)dev)) {
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: unregister_device
** 功能描述: 卸载设备.
** 输　入  : path               路径
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  unregister_device (const char *path)
{
    PLW_DEV_HDR   pdev = API_IosDevMatch(path);
    
    if (!pdev) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

    API_IosDevDelete(pdev);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_GJB7714_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: randDev.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 10 月 31 日
**
** 描        述: UNIX 兼容随机数设备.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"                                /*  需要根文件系统时间          */
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
#include "randDevLib.h"
/*********************************************************************************************************
  设备宏
*********************************************************************************************************/
#define __LW_RAND_DEV_NAME      "/dev/random"
#define __LW_URAND_DEV_NAME     "/dev/urandom"
/*********************************************************************************************************
  定义全局变量
*********************************************************************************************************/
static INT _G_iRandDrvNum = PX_ERROR;
/*********************************************************************************************************
** 函数名称: API_RandDrvInstall
** 功能描述: 安装随机数发生器设备驱动程序
** 输　入  : NONE
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_RandDrvInstall (VOID)
{
    if (_G_iRandDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    _G_iRandDrvNum = iosDrvInstall(__randOpen, LW_NULL, __randOpen, __randClose,
                                   __randRead, __randWrite, __randIoctl);
                                   
    DRIVER_LICENSE(_G_iRandDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iRandDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iRandDrvNum, "random number generator.");

    return  (_G_iRandDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_RandDevCreate
** 功能描述: 创建随机数发生器设备 (random 与 urandom)
** 输　入  : NONE
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_RandDevCreate (VOID)
{
    PLW_RAND_DEV    pranddev[2];                                        /*  要创建两个设备              */

    if (_G_iRandDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    pranddev[0] = (PLW_RAND_DEV)__SHEAP_ALLOC(sizeof(LW_RAND_DEV) * 2);
    if (pranddev[0] == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pranddev[0], sizeof(LW_RAND_DEV) * 2);
    
    pranddev[1] = pranddev[0] + 1;
    
    pranddev[0]->RANDDEV_bIsURand = LW_FALSE;
    pranddev[1]->RANDDEV_bIsURand = LW_TRUE;
    
    if (iosDevAddEx(&pranddev[0]->RANDDEV_devhdr, __LW_RAND_DEV_NAME, 
                    _G_iRandDrvNum, DT_CHR) != ERROR_NONE) {            /*  创建 /dev/random            */
        __SHEAP_FREE(pranddev[0]);
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&pranddev[1]->RANDDEV_devhdr, __LW_URAND_DEV_NAME, 
                    _G_iRandDrvNum, DT_CHR) != ERROR_NONE) {            /*  创建 /dev/urandom           */
        iosDevDelete(&pranddev[0]->RANDDEV_devhdr);
        __SHEAP_FREE(pranddev[0]);
        return  (PX_ERROR);
    }
    
    __randInit();                                                       /*  初始化 rand 驱动            */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/

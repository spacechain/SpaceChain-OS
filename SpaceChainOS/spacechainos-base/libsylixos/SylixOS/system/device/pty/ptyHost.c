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
** 文   件   名: ptyHost.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 06 月 15 日
**
** 描        述: 虚拟终端主控端内部库(将虚拟设备端当作一个串口一样操作).
                 虚拟终端分为两个端口: 设备端和主控端! 
                 设备端是用软件仿真一个硬件串口.
                 主控端可以看成就是一个 TTY 设备.
                 
** BUG:
2009.08.27  打开关闭时增加对设备引用的计数处理.
2009.10.22  read write 返回值为 ssize_t.
2012.01.11  _PtyHostIoctl() 加入对 SIO 硬件参数命令和波特率命令的相应.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SIO_DEVICE_EN > 0) && (LW_CFG_PTY_DEVICE_EN > 0)
#include "ptyLib.h"
/*********************************************************************************************************
  全局变量(控制阀值)
*********************************************************************************************************/
static CHAR             _G_cPtyWrtThreshold = 20;                       /*  当输出缓冲区空闲字节数大于  */
                                                                        /*  这个值, 激活等待写的线程    */
/*********************************************************************************************************
** 函数名称: _PtyHostOpen
** 功能描述: 虚拟终端主控端打开
** 输　入  : p_ptydev      已经创建的设备控制块
**           pcName        打开的文件名
**           iFlags        方式         O_RDONLY  O_WRONLY  O_RDWR  O_CREAT
**           iMode         UNIX MODE
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LONG  _PtyHostOpen (P_PTY_DEV      p_ptydev,
                    PCHAR          pcName,   
                    INT            iFlags, 
                    INT            iMode)
{
    P_PTY_D_DEV  p_ptyddev = &p_ptydev->PTYDEV_ptyddev;

    p_ptyddev->PTYDDEV_bIsClose = LW_FALSE;                             /*  没有关闭                    */

    LW_DEV_INC_USE_COUNT(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev.TYDEV_devhdrHdr);

    return  ((LONG)(p_ptydev));                                         /*  虚拟终端控制块地址          */
}
/*********************************************************************************************************
** 函数名称: _PtyHostClose
** 功能描述: 虚拟终端主控端关闭
** 输　入  : p_ptydev       虚拟终端控制块地址
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _PtyHostClose (P_PTY_DEV      p_ptydev)
{
    P_PTY_D_DEV  p_ptyddev = &p_ptydev->PTYDEV_ptyddev;
    
    if (LW_DEV_DEC_USE_COUNT(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev.TYDEV_devhdrHdr) == 0) {
        p_ptyddev->PTYDDEV_bIsClose = LW_TRUE;                          /*  关闭                        */
        
        API_SemaphoreBPost(p_ptyddev->PTYDDEV_hRdSyncSemB);             /*  激活等待的线程              */
        SEL_WAKE_UP_ALL(&p_ptyddev->PTYDDEV_selwulList, SELREAD);       /*  select() 激活               */
        SEL_WAKE_UP_ALL(&p_ptyddev->PTYDDEV_selwulList, SELEXCEPT);     /*  select() 激活               */
    }
        
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _PtyHostRead
** 功能描述: 虚拟终端主控端读取数据
** 输　入  : p_ptydev       虚拟终端
**           pcBuffer       接收缓冲区
**           stMaxBytes     缓冲区大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t  _PtyHostRead (P_PTY_DEV     p_ptydev, 
                       PCHAR         pcBuffer, 
                       size_t        stMaxBytes)
{
    REGISTER P_PTY_D_DEV    p_ptyddev = &p_ptydev->PTYDEV_ptyddev;
    REGISTER P_PTY_H_DEV    p_ptyhdev = &p_ptydev->PTYDEV_ptyhdev;
    
    REGISTER ssize_t        sstRead   = _TyRead(&p_ptyhdev->PTYHDEV_tydevTyDev, 
                                                pcBuffer, 
                                                stMaxBytes);            /*  从接收缓冲区读取数据        */

    if (rngNBytes(p_ptyhdev->PTYHDEV_tydevTyDev.TYDEV_vxringidRdBuf)
        < _G_cPtyWrtThreshold) {                                        /*  可读数据小于 20 个          */
        SEL_WAKE_UP_ALL(&p_ptyddev->PTYDDEV_selwulList, SELWRITE);      /*  select() 激活               */
    }
    
    return  (sstRead);
}
/*********************************************************************************************************
** 函数名称: _PtyHostWrite
** 功能描述: 虚拟终端主控端写入数据
** 输　入  : p_ptydev       虚拟终端
**           pcBuffer       需要写入终端的数据
**           stNBytes       需要写入的数据大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t  _PtyHostWrite (P_PTY_DEV     p_ptydev, 
                        PCHAR         pcBuffer, 
                        size_t        stNBytes)
{
    REGISTER P_PTY_D_DEV    p_ptyddev = &p_ptydev->PTYDEV_ptyddev;
    REGISTER P_PTY_H_DEV    p_ptyhdev = &p_ptydev->PTYDEV_ptyhdev;
    
    REGISTER ssize_t        sstWrite  = _TyWrite(&p_ptyhdev->PTYHDEV_tydevTyDev, 
                                                 pcBuffer, 
                                                 stNBytes);             /*  向发送缓冲区写入数据        */

    if (rngNBytes(p_ptyhdev->PTYHDEV_tydevTyDev.TYDEV_vxringidWrBuf)
        > 0) {
        SEL_WAKE_UP_ALL(&p_ptyddev->PTYDDEV_selwulList, SELREAD);       /*  select() 激活               */
    }
    
    return  (sstWrite);
}
/*********************************************************************************************************
** 函数名称: _PtyHostIoctl
** 功能描述: 虚拟终端主控端执行控制命令
** 输　入  : p_ptydev       虚拟终端
**           pcBuffer       需要写入终端的数据
**           iNBytes        需要写入的数据大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _PtyHostIoctl (P_PTY_DEV     p_ptydev, 
                    INT           iRequest,
                    LONG          lArg)
{
    REGISTER P_PTY_H_DEV    p_ptyhdev = &p_ptydev->PTYDEV_ptyhdev;
    
    switch (iRequest) {
    
    case SIO_HW_OPTS_GET:
        *(INT *)lArg = (CREAD | CS8);
        return  (ERROR_NONE);
        
    case SIO_HW_OPTS_SET:
        return  (ERROR_NONE);
        
    case SIO_BAUD_GET:
        *(LONG *)lArg = SIO_BAUD_115200;
        return  (ERROR_NONE);
        
    case FIOBAUDRATE:
    case SIO_BAUD_SET:
        return  (ERROR_NONE);
    }
    
    return  (_TyIoctl(&p_ptyhdev->PTYHDEV_tydevTyDev, 
                      iRequest, lArg));
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SIO_DEVICE_EN > 0)  */
                                                                        /*  (LW_CFG_PTY_DEVICE_EN > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

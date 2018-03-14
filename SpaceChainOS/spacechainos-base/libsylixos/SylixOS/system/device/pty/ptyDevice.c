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
** 文   件   名: ptyDevice.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 06 月 13 日
**
** 描        述: 虚拟终端设备端(虚拟硬件))内部库.
                 虚拟终端分为两个端口: 设备端和主控端! 
                 设备端是用软件仿真一个硬件串口.
                 主控端可以看成就是一个 TTY 设备.

** BUG:
2009.05.27  加入 abort 功能.
2009.08.27  打开关闭时增加对设备引用的计数处理.
2009.10.22  read write 返回值为 ssize_t.
2010.01.14  升级 abort.
2012.03.26  返回正确的数目.
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
  函数声明
*********************************************************************************************************/
VOID    __selPtyAdd(P_PTY_D_DEV   p_ptyddev, LONG  lArg);
VOID    __selPtyDelete(P_PTY_D_DEV   p_ptyddev, LONG  lArg);
/*********************************************************************************************************
** 函数名称: _PtyDeviceOpen
** 功能描述: 虚拟终端设备端打开
** 输　入  : p_ptydev      已经创建的设备控制块
**           pcName        打开的文件名
**           iFlags        方式         O_RDONLY  O_WRONLY  O_RDWR  O_CREAT
**           iMode         UNIX MODE
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LONG  _PtyDeviceOpen (P_PTY_D_DEV    p_ptyddev,
                      PCHAR          pcName,   
                      INT            iFlags, 
                      INT            iMode)
{
    REGISTER P_PTY_DEV     p_ptydev = _LIST_ENTRY(p_ptyddev, PTY_DEV, PTYDEV_ptyddev);

    p_ptyddev->PTYDDEV_bIsClose = LW_FALSE;                             /*  没有关闭                    */
    
    LW_DEV_INC_USE_COUNT(&p_ptyddev->PTYDDEV_devhdrDevice);
    
    return  ((LONG)(p_ptydev));                                         /*  返回设备端控制地址          */
}
/*********************************************************************************************************
** 函数名称: _PtyDeviceClose
** 功能描述: 虚拟终端设备端关闭
** 输　入  : p_ptydev       虚拟终端
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _PtyDeviceClose (P_PTY_DEV     p_ptydev)
{
    if (LW_DEV_DEC_USE_COUNT(&p_ptydev->PTYDEV_ptyddev.PTYDDEV_devhdrDevice) == 0) {
        return  (_TyIoctl(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev, 
                          FIOCANCEL, 0));                               /*  停止主控端                  */
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: _PtyDeviceRead
** 功能描述: 虚拟终端设备端从主控端的发送缓冲区读取数据
** 输　入  : p_ptydev       虚拟终端
**           pcBuffer       接收缓冲区
**           stMaxBytes     缓冲区大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t  _PtyDeviceRead (P_PTY_DEV     p_ptydev, 
                         PCHAR         pcBuffer, 
                         size_t        stMaxBytes)
{
             CHAR           cGet;
    REGISTER INT            iTemp = 0;
    REGISTER INT            iError;
    REGISTER ULONG          ulError;
    
    REGISTER P_PTY_D_DEV    p_ptyddev = &p_ptydev->PTYDEV_ptyddev;
    REGISTER P_PTY_H_DEV    p_ptyhdev = &p_ptydev->PTYDEV_ptyhdev;

    if (p_ptyddev->PTYDDEV_bIsClose) {                                  /*  是否被关闭了                */
        return  (0);
    }
    
    do {
        p_ptyddev->PTYDDEV_iAbortFlag &= ~OPT_RABORT;                   /*  清除 abort                  */
    
        while (iTemp < stMaxBytes) {
            iError = _TyITx(&p_ptyhdev->PTYHDEV_tydevTyDev, &cGet);     /*  模拟串口的 TxD 中断         */
            if (iError == PX_ERROR) {
                break;
            
            } else {
                pcBuffer[iTemp++] = cGet;
            }
        }
        
        if (p_ptyddev->PTYDDEV_bIsClose) {                              /*  是否被关闭了                */
            break;
        }
        
        if (iTemp == 0) {
            ulError = API_SemaphoreBPend(p_ptyddev->PTYDDEV_hRdSyncSemB, 
                                         p_ptyddev->PTYDDEV_ulRTimeout);
            if (ulError) {                                              /*  产生超时或其他错误          */
                return  ((ssize_t)iTemp);
            }
            if (p_ptyddev->PTYDDEV_iAbortFlag & OPT_RABORT) {
                if (iTemp <= 0) {
                    _ErrorHandle(ERROR_IO_ABORT);
                }
                return  ((ssize_t)iTemp);
            }
        }
    } while (iTemp == 0);
     
    return  ((ssize_t)iTemp);
}
/*********************************************************************************************************
** 函数名称: _PtyDeviceWrite
** 功能描述: 虚拟终端设备端向主控端的接收缓冲区写入数据
** 输　入  : p_ptydev       虚拟终端
**           pcBuffer       需要写入终端的数据
**           stNBytes       需要写入的数据大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t  _PtyDeviceWrite (P_PTY_DEV     p_ptydev, 
                          PCHAR         pcBuffer, 
                          size_t        stNBytes)
{
    REGISTER INT            i;
    REGISTER P_PTY_H_DEV    p_ptyhdev = &p_ptydev->PTYDEV_ptyhdev;
    
    for (i = 0; i < stNBytes; i++) {
        if (_TyIRd(&p_ptyhdev->PTYHDEV_tydevTyDev, pcBuffer[i]) < ERROR_NONE) {
            break;
        }
    }
    
    return  ((ssize_t)i);
}
/*********************************************************************************************************
** 函数名称: _PtyDeviceIoctl
** 功能描述: 虚拟终端设备端执行控制命令
** 输　入  : p_ptydev       虚拟终端
**           pcBuffer       需要写入终端的数据
**           iNBytes        需要写入的数据大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _PtyDeviceIoctl (P_PTY_DEV     p_ptydev, 
                      INT           iRequest,
                      LONG          lArg)
{
    REGISTER P_PTY_D_DEV    p_ptyddev = &p_ptydev->PTYDEV_ptyddev;
    REGISTER P_PTY_H_DEV    p_ptyhdev = &p_ptydev->PTYDEV_ptyhdev;
             struct stat   *pstatGet;

    switch (iRequest) {
    
    case FIORTIMEOUT:                                                   /*  设置读超时时间              */
        {
            struct timeval *ptvTimeout = (struct timeval *)lArg;
            REGISTER ULONG  ulTick;
            if (ptvTimeout) {
                ulTick = __timevalToTick(ptvTimeout);                   /*  获得 tick 数量              */
                p_ptyddev->PTYDDEV_ulRTimeout = ulTick;
            } else {
                p_ptyddev->PTYDDEV_ulRTimeout = LW_OPTION_WAIT_INFINITE;
            }
        }
        break;
        
    case FIOFSTATGET:                                                   /*  获取文件属性                */
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev     = (dev_t)p_ptydev;
            pstatGet->st_ino     = (ino_t)0;                            /*  相当于唯一节点              */
            pstatGet->st_mode    = 0666 | S_IFCHR;
            pstatGet->st_nlink   = 1;
            pstatGet->st_uid     = 0;
            pstatGet->st_gid     = 0;
            pstatGet->st_rdev    = 1;
            pstatGet->st_size    = 0;
            pstatGet->st_blksize = 0;
            pstatGet->st_blocks  = 0;
            pstatGet->st_atime   = p_ptyddev->PTYDDEV_timeCreate;
            pstatGet->st_mtime   = p_ptyddev->PTYDDEV_timeCreate;
            pstatGet->st_ctime   = p_ptyddev->PTYDDEV_timeCreate;
        } else {
            return  (PX_ERROR);
        }
        break;
        
    case FIOSELECT:
        __selPtyAdd(p_ptyddev, lArg);
        break;
        
    case FIOUNSELECT:
        __selPtyDelete(p_ptyddev, lArg);
        break;
        
    case FIOWAITABORT:                                                  /*  停止当前等待 IO 线程        */
        if ((INT)lArg & OPT_RABORT) {                                   /*  仅支持 read abort           */
            ULONG  ulBlockNum;
            API_SemaphoreBStatus(p_ptyddev->PTYDDEV_hRdSyncSemB, LW_NULL, LW_NULL, &ulBlockNum);
            if (ulBlockNum) {
                p_ptyddev->PTYDDEV_iAbortFlag |= OPT_RABORT;
                API_SemaphoreBPost(p_ptyddev->PTYDDEV_hRdSyncSemB);     /*  激活写等待线程              */
            }
        }
        break;
        
    default:
        return  (_TyIoctl(&p_ptyhdev->PTYHDEV_tydevTyDev,
                          iRequest, lArg));                             /*  转向主控设备                */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _PtyDeviceStartup
** 功能描述: 虚拟终端设备端启动发送 (模拟设备端准备好接收数据)
** 输　入  : p_ptydev       虚拟终端
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _PtyDeviceStartup (P_PTY_DEV     p_ptydev)
{
    API_SemaphoreBPost(p_ptydev->PTYDEV_ptyddev.PTYDDEV_hRdSyncSemB);   /*  激活 read() 操作            */
    
    /*
     *  这里可能是 ty 的回显操作, 并不完全是 host 端的 write 操作(带有 wake up 操作)
     *  所以需要一个冗余的 wake up 操作, 这样就可以确保需要发送数据时, select() 的读
     *  等待才能被正确的激活.
     */
    SEL_WAKE_UP_ALL(&p_ptydev->PTYDEV_ptyddev.PTYDDEV_selwulList, 
                    SELREAD);                                           /*  select() 激活               */
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SIO_DEVICE_EN > 0)  */
                                                                        /*  (LW_CFG_PTY_DEVICE_EN > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

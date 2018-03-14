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
** 文   件   名: pty.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 06 月 15 日
**
** 描        述: 虚拟终端接口部分.
                 虚拟终端分为两个端口: 设备端和主控端! 
                 设备端是用软件仿真一个硬件串口.
                 主控端可以看成就是一个 TTY 设备.
                 
** BUG:
2009.09.05  在删除 pty 设备时, 将所有打开设备的文件设置为异常模式(预关闭模式), 并等待关闭.
2009.10.22  read write 返回值改为 ssize_t.
2010.09.11  创建设备时, 指定设备类型.
2012.08.06  中断中不能删除或者创建设备.
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
  内部全局变量
*********************************************************************************************************/
static INT      _G_iPtyDeviceDrvNum = PX_ERROR;                         /*  设备端驱动主设备号          */
static INT      _G_iPtyHostDrvNum   = PX_ERROR;                         /*  主控端驱动主设备号          */
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
LONG    _PtyHostOpen(P_PTY_DEV      p_ptydev,
                     PCHAR          pcName, 
                     INT            iFlags, 
                     INT            iMode);
INT     _PtyHostClose(P_PTY_DEV    p_ptydev);
ssize_t _PtyHostRead(P_PTY_DEV     p_ptydev, 
                     PCHAR         pcBuffer, 
                     size_t        stMaxBytes);
ssize_t _PtyHostWrite(P_PTY_DEV     p_ptydev, 
                      PCHAR         pcBuffer, 
                      size_t        stNBytes);
INT     _PtyHostIoctl(P_PTY_DEV     p_ptydev, 
                      INT           iRequest,
                      LONG          lArg);
LONG    _PtyDeviceOpen(P_PTY_D_DEV  p_ptyddev,
                       PCHAR        pcName,   
                       INT          iFlags, 
                       INT          iMode);
INT     _PtyDeviceClose(P_PTY_DEV    p_ptydev);
ssize_t _PtyDeviceRead(P_PTY_DEV     p_ptydev, 
                       PCHAR         pcBuffer, 
                       size_t        stMaxBytes);
ssize_t _PtyDeviceWrite(P_PTY_DEV     p_ptydev, 
                        PCHAR         pcBuffer, 
                        size_t        stNBytes);
INT     _PtyDeviceIoctl(P_PTY_DEV     p_ptydev, 
                        INT           iRequest,
                        LONG          lArg);
VOID    _PtyDeviceStartup(P_PTY_DEV   p_ptydev);
/*********************************************************************************************************
** 函数名称: API_PtyDrvInstall
** 功能描述: 安装PTY设备驱动程序
** 输　入  : VOID
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_PtyDrvInstall (VOID)
{
    if (_G_iPtyDeviceDrvNum > 0) {
        return  (ERROR_NONE);                                           /*  驱动已经安装                */
    }
    
    _G_iPtyDeviceDrvNum = iosDrvInstall(_PtyDeviceOpen,
                                        (FUNCPTR)LW_NULL,
                                        _PtyDeviceOpen,
                                        _PtyDeviceClose,
                                        _PtyDeviceRead,
                                        _PtyDeviceWrite,
                                        _PtyDeviceIoctl);
    if (_G_iPtyDeviceDrvNum == PX_ERROR) {
        return  (PX_ERROR);
    }
    
    _G_iPtyHostDrvNum = iosDrvInstall(_PtyHostOpen,
                                      (FUNCPTR)LW_NULL,
                                      _PtyHostOpen,
                                      _PtyHostClose,
                                      _PtyHostRead,
                                      _PtyHostWrite,
                                      _PtyHostIoctl);
    if (_G_iPtyHostDrvNum == PX_ERROR) {
        iosDrvRemove(_G_iPtyDeviceDrvNum, LW_TRUE);
        return  (PX_ERROR);
    }
    
    DRIVER_LICENSE(_G_iPtyDeviceDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iPtyDeviceDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iPtyDeviceDrvNum, "pty driver (device node).");
    
    DRIVER_LICENSE(_G_iPtyHostDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iPtyHostDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iPtyHostDrvNum, "pty driver (host node).");
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PtyDevCreate
** 功能描述: 创建 PTY 设备. 一次创建自动出现两个文件, 一个是主控端, 一个是设备端
**           主控端文件名为: ???.hst
**           设备端文件名为: ???.dev
**           例如: 创建一个虚拟终端设备 /dev/pty/0 主控端名为: /dev/pty/0.hst 设备端名为: /dev/pty/0.dev
** 输　入  : pcName,                       设备名
**           stRdBufSize,                  输入缓冲区大小
**           stWrtBufSize                  输出缓冲区大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_PtyDevCreate (PCHAR   pcName, 
                       size_t  stRdBufSize,
                       size_t  stWrtBufSize)
{
    REGISTER INT            iTemp;
    REGISTER P_PTY_DEV      p_ptydev;
    REGISTER P_PTY_D_DEV    p_ptyddev;
    
             CHAR           cNameBuffer[MAX_FILENAME_LENGTH];
             
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }

    if (!pcName) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }

    if ((_G_iPtyDeviceDrvNum <= 0) ||
        (_G_iPtyHostDrvNum   <= 0)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pty Driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    p_ptydev = (P_PTY_DEV)__SHEAP_ALLOC(sizeof(PTY_DEV));
    if (p_ptydev == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    iTemp = _TyDevInit(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev, 
                       stRdBufSize, 
                       stWrtBufSize,
                       (FUNCPTR)_PtyDeviceStartup);                     /*  初始化设备控制块            */
    if (iTemp != ERROR_NONE) {
        __SHEAP_FREE(p_ptydev);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    p_ptyddev = &p_ptydev->PTYDEV_ptyddev;
    lib_bzero(&p_ptyddev->PTYDDEV_devhdrDevice, sizeof(LW_DEV_HDR));
    
    p_ptyddev->PTYDDEV_iAbortFlag = 0;
    p_ptyddev->PTYDDEV_ulRTimeout = LW_OPTION_WAIT_INFINITE;            /*  初始化为永久等待            */
    SEL_WAKE_UP_LIST_INIT(&p_ptyddev->PTYDDEV_selwulList);              /*  初始化设备端 sel 结构       */
    
    p_ptyddev->PTYDDEV_hRdSyncSemB = API_SemaphoreBCreate("ptyd_rsync",
                                     LW_FALSE, 
                                     LW_OPTION_WAIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                     LW_NULL);
    if (!p_ptyddev->PTYDDEV_hRdSyncSemB) {                              /*  是否失败                    */
        SEL_WAKE_UP_LIST_TERM(&p_ptyddev->PTYDDEV_selwulList);          /*  卸载 sel 结构               */
        _TyDevRemove(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev);     /*  卸载 ty  结构               */
        __SHEAP_FREE(p_ptydev);
        return  (PX_ERROR);
    }
    
    lib_strcpy(cNameBuffer, pcName);
    lib_strcat(cNameBuffer, ".hst");                                    /*  主控端                      */
    
    iTemp = (INT)iosDevAddEx(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev.TYDEV_devhdrHdr, 
                             cNameBuffer, 
                             _G_iPtyHostDrvNum,
                             DT_CHR);
    if (iTemp) {
        SEL_WAKE_UP_LIST_TERM(&p_ptyddev->PTYDDEV_selwulList);          /*  卸载 sel 结构               */
        API_SemaphoreBDelete(&p_ptyddev->PTYDDEV_hRdSyncSemB);
        _TyDevRemove(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev);     /*  卸载 ty  结构               */
        __SHEAP_FREE(p_ptydev);
        return  (PX_ERROR);
    }
    
    lib_strcpy(cNameBuffer, pcName);
    lib_strcat(cNameBuffer, ".dev");                                    /*  设备端                      */
    
    iTemp = (INT)iosDevAddEx(&p_ptyddev->PTYDDEV_devhdrDevice, 
                             cNameBuffer, 
                             _G_iPtyDeviceDrvNum,
                             DT_CHR);
    if (iTemp) {
        iosDevDelete(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev.TYDEV_devhdrHdr);
        SEL_WAKE_UP_LIST_TERM(&p_ptyddev->PTYDDEV_selwulList);          /*  卸载 sel 结构               */
        API_SemaphoreBDelete(&p_ptyddev->PTYDDEV_hRdSyncSemB);
        _TyDevRemove(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev);     /*  卸载 ty  结构               */
        __SHEAP_FREE(p_ptydev);
        return  (PX_ERROR);
    }
    
    /*
     *  记录创建时间 (UTC 时间)
     */
    p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev.TYDEV_timeCreate = lib_time(LW_NULL);
    p_ptydev->PTYDEV_ptyddev.PTYDDEV_timeCreate = 
        p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev.TYDEV_timeCreate;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PtyDevRemove
** 功能描述: 删除 PTY 设备.
** 输　入  : pcName,                       设备名, 等同于 创建时的设备名
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_PtyDevRemove (PCHAR   pcName)
{
    REGISTER DEV_HDR     *pdevhdrHost;
    REGISTER DEV_HDR     *pdevhdrDevice;
    REGISTER P_PTY_D_DEV  p_ptyddev;
    REGISTER P_PTY_DEV    p_ptydev;
    
             CHAR         cNameBuffer[MAX_FILENAME_LENGTH];
             PCHAR        pcTail;

    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    if (!pcName) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    if ((_G_iPtyDeviceDrvNum <= 0) ||
        (_G_iPtyHostDrvNum   <= 0)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pty Driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    lib_strcpy(cNameBuffer, pcName);
    lib_strcat(cNameBuffer, ".hst");                                    /*  主控端                      */
    
    pdevhdrHost = iosDevFind(cNameBuffer, &pcTail);
    if ((pdevhdrHost == LW_NULL) || (pcTail == cNameBuffer)) {
        return  (PX_ERROR);
    }
    
    lib_strcpy(cNameBuffer, pcName);
    lib_strcat(cNameBuffer, ".dev");                                    /*  设备端                      */
    
    pdevhdrDevice = iosDevFind(cNameBuffer, &pcTail);
    if ((pdevhdrDevice == LW_NULL) || (pcTail == cNameBuffer)) {
        return  (PX_ERROR);
    }
    
    p_ptyddev = (P_PTY_D_DEV)pdevhdrDevice;
    
    iosDevFileAbnormal(&p_ptyddev->PTYDDEV_devhdrDevice);               /*  将所有从设备当前文件设为错误*/
    iosDevDelete(pdevhdrDevice);                                        /*  从 IO 系统卸载设备          */
    
    SEL_WAKE_UP_LIST_TERM(&p_ptyddev->PTYDDEV_selwulList);              /*  卸载 sel 结构               */
    
    p_ptydev = (P_PTY_DEV)pdevhdrHost;
                                                                        /*  将所有打开的文件设为异常    */
    iosDevFileAbnormal(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev.TYDEV_devhdrHdr);
    iosDevDelete(pdevhdrHost);                                          /*  从 IO 系统卸载设备          */
    
    _TyDevRemove(&p_ptydev->PTYDEV_ptyhdev.PTYHDEV_tydevTyDev);         /*  卸载 ty  结构               */
    
    API_SemaphoreBDelete(&p_ptyddev->PTYDDEV_hRdSyncSemB);              /*  删除同步信号量              */
                                                                        /*  这些操作必须在文件异常之后  */
    __SHEAP_FREE(p_ptydev);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SIO_DEVICE_EN > 0)  */
                                                                        /*  (LW_CFG_PTY_DEVICE_EN > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

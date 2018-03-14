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
** 文   件   名: nvmeDev.c
**
** 创   建   人: Qin.Fei (秦飞)
**
** 文件创建日期: 2017 年 7 月 17 日
**
** 描        述: NVMe 设备管理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_NVME_EN > 0)
#include "nvme.h"
#include "nvmeLib.h"
#include "nvmeDrv.h"
#include "nvmeDev.h"
#include "nvmeCtrl.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static UINT32                   _GuiNvmeDevTotalNum  = 0;
static LW_OBJECT_HANDLE         _GulNvmeDevLock      = LW_OBJECT_HANDLE_INVALID;
static LW_LIST_LINE_HEADER      _GplineNvmeDevHeader = LW_NULL;

#define __NVME_DEV_LOCK()       API_SemaphoreMPend(_GulNvmeDevLock, LW_OPTION_WAIT_INFINITE)
#define __NVME_DEV_UNLOCK()     API_SemaphoreMPost(_GulNvmeDevLock)
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
static VOID     __tshellNvmeDevCmdShow(NVME_DEV_HANDLE  hDev);
static INT      __tshellNvmeDevCmd(INT  iArgC, PCHAR  ppcArgV[]);
/*********************************************************************************************************
** 函数名称: API_NvmeDevDelete
** 功能描述: 删除一个 NVMe 设备
** 输　入  : hDev      设备控制句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_NvmeDevDelete (NVME_DEV_HANDLE  hDev)
{
    NVME_DEV_HANDLE     hDevNvme = LW_NULL;

    hDevNvme = API_NvmeDevHandleGet(hDev->NVMEDEV_hCtrl->NVMECTRL_uiIndex,
                                    hDev->NVMEDEV_uiNameSpaceId);
    if ((!hDevNvme) ||
        (hDevNvme != hDev)) {
        return  (PX_ERROR);
    }

    __NVME_DEV_LOCK();
    _List_Line_Del(&hDev->NVMEDEV_lineDevNode, &_GplineNvmeDevHeader);
    _GuiNvmeDevTotalNum--;
    __NVME_DEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_NvmeDevAdd
** 功能描述: 增加一个设备
** 输　入  : hCtrl      控制器句柄
**           uiDrive    驱动器编号
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_NvmeDevAdd (NVME_DEV_HANDLE    hDev)
{
    NVME_DEV_HANDLE    hDevTmep = LW_NULL;

    hDevTmep = API_NvmeDevHandleGet(hDev->NVMEDEV_uiCtrl,
                                    hDev->NVMEDEV_uiNameSpaceId);
    if (hDevTmep != LW_NULL) {
        return  (ERROR_NONE);
    }

    __NVME_DEV_LOCK();
    _List_Line_Add_Ahead(&hDev->NVMEDEV_lineDevNode, &_GplineNvmeDevHeader);
    _GuiNvmeDevTotalNum++;
    __NVME_DEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_NvmeDevHandleGet
** 功能描述: 获取一个设备的句柄
** 输　入  : uiCtrl         控制器索引
**           NamespaceId    命名空间ID
** 输　出  : 设备控制句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
NVME_DEV_HANDLE  API_NvmeDevHandleGet (UINT  uiCtrl, UINT  NamespaceId)
{
    PLW_LIST_LINE       plineTemp = LW_NULL;
    NVME_DEV_HANDLE     hDev      = LW_NULL;

    hDev = LW_NULL;
    __NVME_DEV_LOCK();
    for (plineTemp  = _GplineNvmeDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDev = _LIST_ENTRY(plineTemp, NVME_DEV_CB, NVMEDEV_lineDevNode);
        if ((hDev->NVMEDEV_uiCtrl  == uiCtrl ) &&
            (hDev->NVMEDEV_uiNameSpaceId == NamespaceId)) {
            break;
        }
    }
    __NVME_DEV_UNLOCK();

    if (plineTemp) {
        return  (hDev);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: __nvmeMonitorThread
** 功能描述: 设备管理背景线程
** 输　入  : pvArg             启动参数
** 输　出  : NULL
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
PVOID  __nvmeMonitorThread (PVOID   pvArg)
{
    PLW_LIST_LINE       plineTemp = LW_NULL;
    NVME_DEV_HANDLE     hDev      = LW_NULL;
    VOID              (*fpCallBack)(NVME_DEV_HANDLE) = (VOID (*)(NVME_DEV_HANDLE))pvArg;

    for (;;) {
        API_TimeSleep(LW_TICK_HZ);
        __NVME_DEV_LOCK();
        for (plineTemp  = _GplineNvmeDevHeader;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            hDev = _LIST_ENTRY(plineTemp, NVME_DEV_CB, NVMEDEV_lineDevNode);
            fpCallBack(hDev);
        }
        __NVME_DEV_UNLOCK();
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_NvmeDevCountGet
** 功能描述: 获取 NVMe 设备总数
** 输　入  : NONE
** 输　出  : NVMe 设备总数
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
UINT32  API_NvmeDevCountGet (VOID)
{
    UINT32      uiCount = 0;

    __NVME_DEV_LOCK();
    uiCount = _GuiNvmeDevTotalNum;
    __NVME_DEV_UNLOCK();

    return  (uiCount);
}
/*********************************************************************************************************
** 函数名称: API_NvmeDevInit
** 功能描述: NVMe 设备管理初始化
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_NvmeDevInit (VOID)
{
    static BOOL     bInitFlag = LW_FALSE;

    if (bInitFlag == LW_TRUE) {
        return  (ERROR_NONE);
    }
    bInitFlag = LW_TRUE;

    _GuiNvmeDevTotalNum  = 0;
    _GplineNvmeDevHeader = LW_NULL;

    _GulNvmeDevLock = API_SemaphoreMCreate("nvme_devlock",
                                           LW_PRIO_DEF_CEILING,
                                           LW_OPTION_WAIT_PRIORITY |
                                           LW_OPTION_INHERIT_PRIORITY |
                                           LW_OPTION_DELETE_SAFE |
                                           LW_OPTION_OBJECT_GLOBAL,
                                           LW_NULL);
    if (_GulNvmeDevLock == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

    API_TShellKeywordAdd("nvmedev", __tshellNvmeDevCmd);
    API_TShellFormatAdd("nvmedev", " [[-c] 1 ...]");
    API_TShellHelpAdd("nvmedev", "show, add, del nvme device\n"
                                 "eg. nvmedev\n"
                                 "    nvmedev 0 0\n");
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellNvmeDevCmdShow
** 功能描述: 打印 NVMe 设备列表
** 输　入  : hDev       设备句柄
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __tshellNvmeDevCmdShow (NVME_DEV_HANDLE  hDev)
{
    static PCHAR        pcNvmeDevShowHdr = \
    "INDEX CTRL DRIVER  START    SECTOR COUNT       DRVNAME         DEVNAME         BLKNAME\n"
    "----- ---- ------ ------- ---------------- --------------- --------------- ---------------\n";

    REGISTER INT        i;
    PLW_BLK_DEV         hBlkDev   = LW_NULL;
    PLW_LIST_LINE       plineTemp = LW_NULL;

    i = 0;
    if (hDev) {
        hBlkDev = &hDev->NVMEDEV_tBlkDev;
        printf("\n");
        printf(pcNvmeDevShowHdr);
        printf("%5d %4d %6d %7d %16d %-15s %-15s %-15s\n",
               i,
               hDev->NVMEDEV_uiCtrl,
               hDev->NVMEDEV_uiNameSpaceId,
               (UINT32)hDev->NVMEDEV_ulBlkOffset,
               (UINT32)hDev->NVMEDEV_ulBlkCount,
               hDev->NVMEDEV_hCtrl->NVMECTRL_hDrv->NVMEDRV_cDrvName,
               hDev->NVMEDEV_tBlkDev.BLKD_pcName,
               hBlkDev->BLKD_pcName);
        return;
    }

    printf("nvme dev number total: %d\n", _GuiNvmeDevTotalNum);

    __NVME_DEV_LOCK();
    i = 0;
    for (plineTemp  = _GplineNvmeDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDev = _LIST_ENTRY(plineTemp, NVME_DEV_CB, NVMEDEV_lineDevNode);
        __NVME_DEV_UNLOCK();
        hBlkDev = &hDev->NVMEDEV_tBlkDev;
        printf("\n");
        printf(pcNvmeDevShowHdr);
        printf("%5d %4d %6d %7d %16d %-15s %-15s %-15s\n",
               i,
               hDev->NVMEDEV_uiCtrl,
               hDev->NVMEDEV_uiNameSpaceId,
               (UINT32)hDev->NVMEDEV_ulBlkOffset,
               (UINT32)hDev->NVMEDEV_ulBlkCount,
               hDev->NVMEDEV_hCtrl->NVMECTRL_hDrv->NVMEDRV_cDrvName,
               hDev->NVMEDEV_tBlkDev.BLKD_pcName,
               hBlkDev->BLKD_pcName);
        __NVME_DEV_LOCK();
        i += 1;
    }
    __NVME_DEV_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: __tshellNvmeDevCmd
** 功能描述: NVMe 命令 "nvmedev"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellNvmeDevCmd (INT  iArgC, PCHAR  ppcArgV[])
{
    NVME_DEV_HANDLE     hNvmeDev;                                       /* 设备控制句柄                 */
    UINT                uiCtrl;                                         /* 控制器索引                   */
    UINT                uiDrive;                                        /* 驱动索引                     */

    if (iArgC == 1) {
        __tshellNvmeDevCmdShow(LW_NULL);
        return  (ERROR_NONE);
    }

    if ((iArgC != 3) ||
        (sscanf(ppcArgV[1], "%d", &uiCtrl) != 1) ||
        (sscanf(ppcArgV[2], "%d", &uiDrive) != 1)) {
        goto    __arg_error;
    
    } else {
        hNvmeDev = API_NvmeDevHandleGet(uiCtrl, uiDrive);
        if (!hNvmeDev) {
            goto    __arg_error;
        }
    }

    __tshellNvmeDevCmdShow(hNvmeDev);

    return  (ERROR_NONE);

__arg_error:
    fprintf(stderr, "argument error.\n");
    return  (PX_ERROR);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_NVME_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/

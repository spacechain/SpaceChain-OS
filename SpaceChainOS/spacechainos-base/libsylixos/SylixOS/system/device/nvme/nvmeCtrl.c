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
** 文   件   名: nvmeCtrl.c
**
** 创   建   人: Qin.Fei (秦飞)
**
** 文件创建日期: 2017 年 7 月 17 日
**
** 描        述: NVMe 控制器管理.
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
static UINT32                   _GuiNvmeCtrlTotalNum  = 0;
static UINT8                    _GucNvmeCtrlIndexMap  = 0;
static LW_OBJECT_HANDLE         _GulNvmeCtrlLock      = LW_OBJECT_HANDLE_INVALID;
static LW_LIST_LINE_HEADER      _GplineNvmeCtrlHeader = LW_NULL;

#define __NVME_CTRL_LOCK()      API_SemaphoreMPend(_GulNvmeCtrlLock, LW_OPTION_WAIT_INFINITE)
#define __NVME_CTRL_UNLOCK()    API_SemaphoreMPost(_GulNvmeCtrlLock)
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
static VOID     __tshellNvmeCtrlCmdShow(VOID);
static INT      __tshellNvmeCtrlCmd(INT  iArgC, PCHAR  ppcArgV[]);
/*********************************************************************************************************
** 函数名称: API_NvmeCtrlDelete
** 功能描述: 删除一个 NVMe 控制器
** 输　入  : hCtrl     设备控制句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_NvmeCtrlDelete (NVME_CTRL_HANDLE  hCtrl)
{
    NVME_CTRL_HANDLE    hCtrlTmep = LW_NULL;

    hCtrlTmep = API_NvmeCtrlHandleGetFromIndex(hCtrl->NVMECTRL_uiIndex);
    if ((!hCtrlTmep) ||
        (hCtrlTmep != hCtrl)) {
        return  (PX_ERROR);
    }

    __NVME_CTRL_LOCK();
    _List_Line_Del(&hCtrl->NVMECTRL_lineCtrlNode, &_GplineNvmeCtrlHeader);
    _GucNvmeCtrlIndexMap &= ~(0x01 << hCtrl->NVMECTRL_uiIndex);
    _GuiNvmeCtrlTotalNum--;
    __NVME_CTRL_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_NvmeCtrlAdd
** 功能描述: 增加一个控制器
** 输　入  : hCtrl      控制器句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_NvmeCtrlAdd (NVME_CTRL_HANDLE  hCtrl)
{
    NVME_CTRL_HANDLE    hCtrlTmep = LW_NULL;

    hCtrlTmep = API_NvmeCtrlHandleGetFromIndex(hCtrl->NVMECTRL_uiIndex);
    if (hCtrlTmep != LW_NULL) {
        return  (ERROR_NONE);
    }

    __NVME_CTRL_LOCK();
    _List_Line_Add_Ahead(&hCtrl->NVMECTRL_lineCtrlNode, &_GplineNvmeCtrlHeader);
    _GucNvmeCtrlIndexMap |= 0x01 << hCtrl->NVMECTRL_uiIndex;
    _GuiNvmeCtrlTotalNum++;
    __NVME_CTRL_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_NvmeCtrlHandleGetFromPciArg
** 功能描述: 通过 PCI 设备句柄获取一个控制器的句柄
** 输　入  : pvCtrlPciArg      控制器参数
** 输　出  : 设备控制句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
NVME_CTRL_HANDLE  API_NvmeCtrlHandleGetFromPciArg (PVOID  pvCtrlPciArg)
{
    PLW_LIST_LINE       plineTemp = LW_NULL;
    NVME_CTRL_HANDLE    hCtrl     = LW_NULL;

    hCtrl = LW_NULL;
    __NVME_CTRL_LOCK();
    for (plineTemp  = _GplineNvmeCtrlHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hCtrl = _LIST_ENTRY(plineTemp, NVME_CTRL_CB, NVMECTRL_lineCtrlNode);
        if (hCtrl->NVMECTRL_pvArg == pvCtrlPciArg) {
            break;
        }
    }
    __NVME_CTRL_UNLOCK();

    if (plineTemp) {
        return  (hCtrl);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: API_NvmeCtrlHandleGetFromName
** 功能描述: 通过名字获取一个控制器的句柄
** 输　入  : cpcName    控制器名称
**           uiUnit     本类控制器索引
** 输　出  : 设备控制句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
NVME_CTRL_HANDLE  API_NvmeCtrlHandleGetFromName (CPCHAR  cpcName, UINT  uiUnit)
{
    PLW_LIST_LINE       plineTemp = LW_NULL;
    NVME_CTRL_HANDLE    hCtrl     = LW_NULL;

    hCtrl = LW_NULL;
    __NVME_CTRL_LOCK();
    for (plineTemp  = _GplineNvmeCtrlHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hCtrl = _LIST_ENTRY(plineTemp, NVME_CTRL_CB, NVMECTRL_lineCtrlNode);
        if ((lib_strncmp(&hCtrl->NVMECTRL_cCtrlName[0], cpcName, NVME_CTRL_NAME_MAX) == 0) &&
            (hCtrl->NVMECTRL_uiUnitIndex == uiUnit)) {
            break;
        }
    }
    __NVME_CTRL_UNLOCK();

    if (plineTemp) {
        return  (hCtrl);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: API_NvmeCtrlHandleGetFromIndex
** 功能描述: 通过索引获取一个控制器的句柄
** 输　入  : uiIndex     控制器索引
** 输　出  : 设备控制句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
NVME_CTRL_HANDLE  API_NvmeCtrlHandleGetFromIndex (UINT  uiIndex)
{
    PLW_LIST_LINE       plineTemp = LW_NULL;
    NVME_CTRL_HANDLE    hCtrl     = LW_NULL;

    hCtrl = LW_NULL;
    __NVME_CTRL_LOCK();
    for (plineTemp  = _GplineNvmeCtrlHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hCtrl = _LIST_ENTRY(plineTemp, NVME_CTRL_CB, NVMECTRL_lineCtrlNode);
        if (hCtrl->NVMECTRL_uiIndex == uiIndex) {
            break;
        }
    }
    __NVME_CTRL_UNLOCK();

    if (plineTemp) {
        return  (hCtrl);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: API_NvmeCtrlIndexGet
** 功能描述: 获取 NVMe 控制器索引
** 输　入  : NONE
** 输　出  : NVMe 控制器总数
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_NvmeCtrlIndexGet (VOID)
{
    REGISTER INT    i;

    __NVME_CTRL_LOCK();
    for (i = 0; i < NVME_CTRL_MAX; i++) {
        if (!((_GucNvmeCtrlIndexMap >> i) & 0x01)) {
            break;
        }
    }
    __NVME_CTRL_UNLOCK();

    if (i >= NVME_CTRL_MAX) {
        return  (PX_ERROR);
    }

    return  (i);
}
/*********************************************************************************************************
** 函数名称: API_NvmeCtrlCountGet
** 功能描述: 获取 NVMe 控制器总数
** 输　入  : NONE
** 输　出  : NVMe 控制器总数
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
UINT32  API_NvmeCtrlCountGet (VOID)
{
    UINT32      uiCount = 0;

    __NVME_CTRL_LOCK();
    uiCount = _GuiNvmeCtrlTotalNum;
    __NVME_CTRL_UNLOCK();

    return  (uiCount);
}
/*********************************************************************************************************
** 函数名称: API_NvmeCtrlInit
** 功能描述: NVMe 控制器管理初始化
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_NvmeCtrlInit (VOID)
{
    static BOOL     bInitFlag = LW_FALSE;

    if (bInitFlag == LW_TRUE) {
        return  (ERROR_NONE);
    }
    bInitFlag = LW_TRUE;

    _GuiNvmeCtrlTotalNum  = 0;
    _GplineNvmeCtrlHeader = LW_NULL;

    _GulNvmeCtrlLock = API_SemaphoreMCreate("nvme_ctrllock",
                                            LW_PRIO_DEF_CEILING,
                                            LW_OPTION_WAIT_PRIORITY |
                                            LW_OPTION_INHERIT_PRIORITY |
                                            LW_OPTION_DELETE_SAFE |
                                            LW_OPTION_OBJECT_GLOBAL,
                                            LW_NULL);
    if (_GulNvmeCtrlLock == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

    API_TShellKeywordAdd("nvmectrl", __tshellNvmeCtrlCmd);
    API_TShellFormatAdd("nvmectrl", " [add | del] [0:1]");
    API_TShellHelpAdd("nvmectrl", "show, add, del nvme control\n"
                                  "eg. nvmectrl\n"
                                  "    nvmectrl add imx6dq_nvme 0\n"
                                  "    nvmectrl del imx6dq_nvme 0\n");

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellNvmeCtrlCmdShow
** 功能描述: 打印 NVMe 控制器列表
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __tshellNvmeCtrlCmdShow (VOID)
{
    static PCHAR        pcNvmeCtrlShowHdr = \
    "INDEX CTRL UNIT     CTRLNAME           CTRL VER             DRIVE VER\n"
    "----- ---- ---- ---------------- -------------------- --------------------\n";

    REGISTER INT        i;
    NVME_CTRL_HANDLE    hCtrl     = LW_NULL;
    PLW_LIST_LINE       plineTemp = LW_NULL;
    CHAR                cVerNum[NVME_DRV_VER_STR_LEN]  = {0};

    printf("nvme control number total: %d\n", _GuiNvmeCtrlTotalNum);
    printf(pcNvmeCtrlShowHdr);

    __NVME_CTRL_LOCK();
    i = 0;
    for (plineTemp  = _GplineNvmeCtrlHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hCtrl = _LIST_ENTRY(plineTemp, NVME_CTRL_CB, NVMECTRL_lineCtrlNode);
        printf("%5d %4d %4d %-16s",
               i,
               hCtrl->NVMECTRL_uiIndex,
               hCtrl->NVMECTRL_uiUnitIndex,
               hCtrl->NVMECTRL_cCtrlName);

        if ((hCtrl) && (hCtrl->NVMECTRL_uiCoreVer)) {
            lib_bzero(&cVerNum[0], NVME_DRV_VER_STR_LEN);
            snprintf(cVerNum, NVME_DRV_VER_STR_LEN, NVME_DRV_VER_FORMAT(hCtrl->NVMECTRL_uiCoreVer));
            printf(" %-20s", cVerNum);
        } else {
            printf(" %-20s", "*");
        }

        if ((hCtrl) && (hCtrl->NVMECTRL_hDrv) && (hCtrl->NVMECTRL_hDrv->NVMEDRV_uiDrvVer)) {
            lib_bzero(&cVerNum[0], NVME_DRV_VER_STR_LEN);
            snprintf(cVerNum, NVME_DRV_VER_STR_LEN,
                     NVME_DRV_VER_FORMAT(hCtrl->NVMECTRL_hDrv->NVMEDRV_uiDrvVer));
            printf(" %-20s\n", cVerNum);
        } else {
            printf(" %-20s\n", "*");
        }

        i += 1;
    }
    __NVME_CTRL_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: __tshellNvmeCtrlCmd
** 功能描述: NVMe 命令 "nvmectrl"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellNvmeCtrlCmd (INT  iArgC, PCHAR  ppcArgV[])
{
    if (iArgC == 1) {
        __tshellNvmeCtrlCmdShow();
        return  (ERROR_NONE);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_NVME_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/

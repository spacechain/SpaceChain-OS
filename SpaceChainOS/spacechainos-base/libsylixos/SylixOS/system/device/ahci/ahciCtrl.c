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
** 文   件   名: ahciCtrl.c
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2016 年 03 月 29 日
**
** 描        述: AHCI 控制器管理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0)
#include "ahci.h"
#include "ahciLib.h"
#include "ahciDrv.h"
#include "ahciDev.h"
#include "ahciCtrl.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static UINT32                   _GuiAhciCtrlTotalNum  = 0;
static UINT8                    _GucAhciCtrlIndexMap  = 0;
static LW_OBJECT_HANDLE         _GulAhciCtrlLock      = LW_OBJECT_HANDLE_INVALID;
static LW_LIST_LINE_HEADER      _GplineAhciCtrlHeader = LW_NULL;

#define __AHCI_CTRL_LOCK()      API_SemaphoreMPend(_GulAhciCtrlLock, LW_OPTION_WAIT_INFINITE)
#define __AHCI_CTRL_UNLOCK()    API_SemaphoreMPost(_GulAhciCtrlLock)
/*********************************************************************************************************
** 函数名称: __tshellAhciCtrlCmdShow
** 功能描述: 打印 AHCI 控制器列表
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static
VOID  __tshellAhciCtrlCmdShow (VOID)
{
    static PCHAR        pcAhciCtrlShowHdr = \
    "INDEX CTRL UNIT     CTRLNAME           CTRL VER             DRIVE VER\n"
    "----- ---- ---- ---------------- -------------------- --------------------\n";

    REGISTER INT        i;
    AHCI_CTRL_HANDLE    hCtrl     = LW_NULL;
    PLW_LIST_LINE       plineTemp = LW_NULL;
    CHAR                cVerNum[AHCI_DRV_VER_STR_LEN]  = {0};

    printf("\nahci control number total: %d\n", _GuiAhciCtrlTotalNum);
    printf(pcAhciCtrlShowHdr);

    __AHCI_CTRL_LOCK();
    i = 0;
    for (plineTemp  = _GplineAhciCtrlHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hCtrl = _LIST_ENTRY(plineTemp, AHCI_CTRL_CB, AHCICTRL_lineCtrlNode);
        printf("%5d %4d %4d %-16s",
               i,
               hCtrl->AHCICTRL_uiIndex,
               hCtrl->AHCICTRL_uiUnitIndex,
               hCtrl->AHCICTRL_cCtrlName);

        if ((hCtrl) && (hCtrl->AHCICTRL_uiCoreVer)) {
            lib_bzero(&cVerNum[0], AHCI_DRV_VER_STR_LEN);
            snprintf(cVerNum, AHCI_DRV_VER_STR_LEN, AHCI_DRV_VER_FORMAT(hCtrl->AHCICTRL_uiCoreVer));
            printf(" %-20s", cVerNum);
        } else {
            printf(" %-20s", "*");
        }

        if ((hCtrl) && (hCtrl->AHCICTRL_hDrv) && (hCtrl->AHCICTRL_hDrv->AHCIDRV_uiDrvVer)) {
            lib_bzero(&cVerNum[0], AHCI_DRV_VER_STR_LEN);
            snprintf(cVerNum, AHCI_DRV_VER_STR_LEN,
                     AHCI_DRV_VER_FORMAT(hCtrl->AHCICTRL_hDrv->AHCIDRV_uiDrvVer));
            printf(" %-20s\n", cVerNum);
        } else {
            printf(" %-20s\n", "*");
        }

        i += 1;
    }
    __AHCI_CTRL_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: __tshellAhciCtrlCmd
** 功能描述: AHCI 命令 "ahcictrl"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellAhciCtrlCmd (INT  iArgC, PCHAR  ppcArgV[])
{
    if (iArgC == 1) {
        __tshellAhciCtrlCmdShow();
        return  (ERROR_NONE);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_AhciCtrlDrvDel
** 功能描述: 删除一个 AHCI 控制器的驱动
** 输　入  : hCtrlHandle    控制器句柄
**           hDrvHandle     驱动句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_AhciCtrlDrvDel (AHCI_CTRL_HANDLE  hCtrlHandle, AHCI_DRV_HANDLE  hDrvHandle)
{
    if ((!hCtrlHandle) ||
        (!hDrvHandle)) {
        return  (PX_ERROR);
    }

    if (hCtrlHandle->AHCICTRL_hDrv != hDrvHandle) {
        return  (PX_ERROR);
    }

    hCtrlHandle->AHCICTRL_hDrv = LW_NULL;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_AhciCtrlDrvUpdate
** 功能描述: 更新一个 AHCI 设备的驱动
** 输　入  : hCtrlHandle    控制器句柄
**           hDrvHandle     驱动句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_AhciCtrlDrvUpdate (AHCI_CTRL_HANDLE  hCtrlHandle, AHCI_DRV_HANDLE  hDrvHandle)
{
    if ((!hCtrlHandle) ||
        (!hDrvHandle)) {
        return  (PX_ERROR);
    }

    if ((hCtrlHandle->AHCICTRL_hDrv == LW_NULL) ||
        (hCtrlHandle->AHCICTRL_hDrv != hDrvHandle)) {
        hCtrlHandle->AHCICTRL_hDrv = hDrvHandle;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_AhciCtrlDelete
** 功能描述: 删除一个 AHCI 控制器
** 输　入  : hCtrl     设备控制句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_AhciCtrlDelete (AHCI_CTRL_HANDLE  hCtrl)
{
    AHCI_CTRL_HANDLE    hCtrlTmep = LW_NULL;

    hCtrlTmep = API_AhciCtrlHandleGetFromIndex(hCtrl->AHCICTRL_uiIndex);
    if ((!hCtrlTmep) ||
        (hCtrlTmep != hCtrl)) {
        return  (PX_ERROR);
    }

    __AHCI_CTRL_LOCK();
    _List_Line_Del(&hCtrl->AHCICTRL_lineCtrlNode, &_GplineAhciCtrlHeader);
    _GucAhciCtrlIndexMap &= ~(0x01 << hCtrl->AHCICTRL_uiIndex);
    _GuiAhciCtrlTotalNum--;
    __AHCI_CTRL_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_AhciCtrlAdd
** 功能描述: 增加一个控制器
** 输　入  : hCtrl      控制器句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_AhciCtrlAdd (AHCI_CTRL_HANDLE  hCtrl)
{
    AHCI_CTRL_HANDLE    hCtrlTmep = LW_NULL;

    hCtrlTmep = API_AhciCtrlHandleGetFromIndex(hCtrl->AHCICTRL_uiIndex);
    if (hCtrlTmep != LW_NULL) {
        return  (ERROR_NONE);
    }

    __AHCI_CTRL_LOCK();
    _List_Line_Add_Ahead(&hCtrl->AHCICTRL_lineCtrlNode, &_GplineAhciCtrlHeader);
    _GucAhciCtrlIndexMap |= 0x01 << hCtrl->AHCICTRL_uiIndex;
    _GuiAhciCtrlTotalNum++;
    __AHCI_CTRL_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_AhciCtrlHandleGetFromPciArg
** 功能描述: 通过 PCI 设备句柄获取一个控制器的句柄
** 输　入  : pvCtrlPciArg      控制器参数
** 输　出  : 设备控制句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
AHCI_CTRL_HANDLE  API_AhciCtrlHandleGetFromPciArg (PVOID  pvCtrlPciArg)
{
    PLW_LIST_LINE       plineTemp = LW_NULL;
    AHCI_CTRL_HANDLE    hCtrl     = LW_NULL;

    hCtrl = LW_NULL;
    __AHCI_CTRL_LOCK();
    for (plineTemp  = _GplineAhciCtrlHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hCtrl = _LIST_ENTRY(plineTemp, AHCI_CTRL_CB, AHCICTRL_lineCtrlNode);
        if (hCtrl->AHCICTRL_pvPciArg == pvCtrlPciArg) {
            break;
        }
    }
    __AHCI_CTRL_UNLOCK();

    if (plineTemp) {
        return  (hCtrl);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: API_AhciCtrlHandleGetFromName
** 功能描述: 通过名字获取一个控制器的句柄
** 输　入  : cpcName    控制器名称
**           uiUnit     本类控制器索引
** 输　出  : 设备控制句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
AHCI_CTRL_HANDLE  API_AhciCtrlHandleGetFromName (CPCHAR  cpcName, UINT  uiUnit)
{
    PLW_LIST_LINE       plineTemp = LW_NULL;
    AHCI_CTRL_HANDLE    hCtrl     = LW_NULL;

    hCtrl = LW_NULL;
    __AHCI_CTRL_LOCK();
    for (plineTemp  = _GplineAhciCtrlHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hCtrl = _LIST_ENTRY(plineTemp, AHCI_CTRL_CB, AHCICTRL_lineCtrlNode);
        if ((lib_strncmp(&hCtrl->AHCICTRL_cCtrlName[0], cpcName, AHCI_CTRL_NAME_MAX) == 0) &&
            (hCtrl->AHCICTRL_uiUnitIndex == uiUnit)) {
            break;
        }
    }
    __AHCI_CTRL_UNLOCK();

    if (plineTemp) {
        return  (hCtrl);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: API_AhciCtrlHandleGetFromIndex
** 功能描述: 通过索引获取一个控制器的句柄
** 输　入  : uiIndex     控制器索引
** 输　出  : 设备控制句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
AHCI_CTRL_HANDLE  API_AhciCtrlHandleGetFromIndex (UINT  uiIndex)
{
    PLW_LIST_LINE       plineTemp = LW_NULL;
    AHCI_CTRL_HANDLE    hCtrl     = LW_NULL;

    hCtrl = LW_NULL;
    __AHCI_CTRL_LOCK();
    for (plineTemp  = _GplineAhciCtrlHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hCtrl = _LIST_ENTRY(plineTemp, AHCI_CTRL_CB, AHCICTRL_lineCtrlNode);
        if (hCtrl->AHCICTRL_uiIndex == uiIndex) {
            break;
        }
    }
    __AHCI_CTRL_UNLOCK();

    if (plineTemp) {
        return  (hCtrl);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: API_AhciCtrlIndexGet
** 功能描述: 获取 AHCI 控制器索引
** 输　入  : NONE
** 输　出  : AHCI 控制器总数
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_AhciCtrlIndexGet (VOID)
{
    REGISTER INT    i;

    __AHCI_CTRL_LOCK();
    for (i = 0; i < AHCI_CTRL_MAX; i++) {
        if (!((_GucAhciCtrlIndexMap >> i) & 0x01)) {
            break;
        }
    }
    __AHCI_CTRL_UNLOCK();

    if (i >= AHCI_CTRL_MAX) {
        return  (PX_ERROR);
    }

    return  (i);
}
/*********************************************************************************************************
** 函数名称: API_AhciCtrlCountGet
** 功能描述: 获取 AHCI 控制器总数
** 输　入  : NONE
** 输　出  : AHCI 控制器总数
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
UINT32  API_AhciCtrlCountGet (VOID)
{
    UINT32      uiCount = 0;

    __AHCI_CTRL_LOCK();
    uiCount = _GuiAhciCtrlTotalNum;
    __AHCI_CTRL_UNLOCK();

    return  (uiCount);
}
/*********************************************************************************************************
** 函数名称: API_AhciCtrlInit
** 功能描述: AHCI 控制器管理初始化
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_AhciCtrlInit (VOID)
{
    static BOOL     bInitFlag = LW_FALSE;

    if (bInitFlag == LW_TRUE) {
        return  (ERROR_NONE);
    }
    bInitFlag = LW_TRUE;

    _GuiAhciCtrlTotalNum  = 0;
    _GplineAhciCtrlHeader = LW_NULL;

    _GulAhciCtrlLock = API_SemaphoreMCreate("ahci_ctrllock",
                                            LW_PRIO_DEF_CEILING,
                                            LW_OPTION_WAIT_PRIORITY |
                                            LW_OPTION_INHERIT_PRIORITY |
                                            LW_OPTION_DELETE_SAFE |
                                            LW_OPTION_OBJECT_GLOBAL,
                                            LW_NULL);
    if (_GulAhciCtrlLock == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

    API_TShellKeywordAdd("ahcictrl", __tshellAhciCtrlCmd);
    API_TShellFormatAdd("ahcictrl", " [add | del] [0:1]");
    API_TShellHelpAdd("ahcictrl", "show, add, del ahci control\n"
                                  "eg. ahcictrl\n"
                                  "    ahcictrl add imx6dq_ahci 0\n"
                                  "    ahcictrl del imx6dq_ahci 0\n");

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/

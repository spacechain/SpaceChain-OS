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
** 文   件   名: pciShow.h
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2015 年 09 月 17 日
**
** 描        述: PCI 总线信息打印.
*********************************************************************************************************/

#ifndef __PCISHOW_H
#define __PCISHOW_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)

#define PCIUTILS_LSPCI
extern INT      _G_iPciVerbose;

/*********************************************************************************************************
  Useful macros for decoding of bits and bit fields
*********************************************************************************************************/
#define PCI_FLAG(x, y)          ((x & y) ? '+' : '-')
#define PCI_BITS(x, at, width)  (((x) >> (at)) & ((1 << (width)) - 1))
#define PCI_TABLE(tab, x, buf)  ((x) < sizeof(tab) / sizeof((tab)[0]) ? \
                                (tab)[x] : (sprintf((buf), "??%d", (x)), (buf)))

LW_API INT      API_PciBusDeviceShow(INT  iBus);

LW_API INT      API_PciFindDevShow(UINT16  usVendorId, UINT16  usDeviceId, INT  iInstance);
LW_API INT      API_PciFindClassShow(UINT16  usClassCode, INT  iInstance);

LW_API INT      API_PciHeaderShow(INT  iBus, INT  iSlot, INT  iFunc);

LW_API VOID     API_PciInterruptShow(INT  iBus, INT  iSlot, INT  iFunc);
LW_API VOID     API_PciBarShow(INT  iBus, INT  iSlot, INT  iFunc, INT  iBar);
LW_API VOID     API_PciBistShow(INT  iBus, INT  iSlot, INT  iFunc);
LW_API VOID     API_PciStatusShow(INT  iBus, INT  iSlot, INT  iFunc);
LW_API VOID     API_PciCommandShow(INT  iBus, INT  iSlot, INT  iFunc);
LW_API VOID     API_PciIdsShow(INT  iBus, INT  iSlot, INT  iFunc);
LW_API VOID     API_PciReversionShow(INT  iBus, INT  iSlot, INT  iFunc);
LW_API INT      API_PciConfigShow(INT  iBus, INT  iSlot, INT  iFunc);

LW_API VOID     API_PciCapPcixShow(INT  iBus, INT  iSlot, INT  iFunc, UINT  uiOffset);

LW_API VOID     API_PciCapMsiShow(INT  iBus, INT  iSlot, INT  iFunc, UINT  uiOffset);

LW_API VOID     API_PciCapPcieShow(INT  iBus, INT  iSlot, INT  iFunc, UINT  uiOffset);

#define pciBusDeviceShow        API_PciBusDeviceShow

#define pciFindDevShow          API_PciFindDevShow
#define pciFindClassShow        API_PciFindClassShow

#define pciHeaderShow           API_PciHeaderShow

#define pciInterruptShow        API_PciInterruptShow
#define pciBarShow              API_PciBarShow
#define pciBistShow             API_PciBistShow
#define pciStatusShow           API_PciStatusShow
#define pciCommandShow          API_PciCommandShow
#define pciIdsShow              API_PciIdsShow
#define pciReversionShow        API_PciReversionShow
#define pciConfigShow           API_PciConfigShow

#define pciCapPcixShow          API_PciCapPcixShow
#define pciCapMsiShow           API_PciCapMsiShow
#define pciCapPcieShow          API_PciCapPcieShow

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
#endif                                                                  /*  __PCISHOW_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/

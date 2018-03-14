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
** 文   件   名: pciMsi.h
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2015 年 09 月 10 日
**
** 描        述: PCI 总线 MSI(Message Signaled Interrupts) 管理.
*********************************************************************************************************/

#ifndef __PCIMSI_H
#define __PCIMSI_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)

typedef struct {
    UINT32      uiAddressLo;                                            /* low 32 bits of address       */
    UINT32      uiAddressHi;                                            /* high 32 bits of address      */
    UINT32      uiData;                                                 /* 16 bits of msi message data  */
} PCI_MSI_MSG;

typedef struct {
    UINT32          PCIMSI_uiNum;
    ULONG           PCIMSI_ulDevIrqVector;
    PCI_MSI_MSG     PCIMSI_pmmMsg;

    UINT32          PCIMSI_uiMasked;
    UINT32          PCIMSI_uiMaskPos;

    PVOID           PCIMSI_pvPriv;
} PCI_MSI_DESC;
typedef PCI_MSI_DESC       *PCI_MSI_DESC_HANDLE;

LW_API INT      API_PciMsixClearSet(INT iBus, INT iSlot, INT iFunc, UINT32 uiMsixCapOft,
                                    UINT16  usClear, UINT16  usSet);

LW_API INT      API_PciMsiMsgRead(INT  iBus, INT  iSlot, INT iFunc, UINT32 uiMsixCapOft,
                                  UINT8  ucMultiple, PCI_MSI_MSG *ppmmMsg);
LW_API INT      API_PciMsiMsgWrite(INT iBus, INT iSlot, INT iFunc, UINT32 uiMsixCapOft,
                                   UINT8 ucMultiple, PCI_MSI_MSG *ppmmMsg);

LW_API INT      API_PciMsiPendingSet(INT iBus, INT iSlot, INT iFunc, UINT32 uiMsixCapOft,
                                     UINT32 uiPending, UINT32 uiFlag);
LW_API INT      API_PciMsiPendingGet(INT iBus, INT iSlot, INT iFunc, UINT32 uiMsixCapOft,
                                     UINT32 *puiPending);
LW_API INT      API_PciMsiPendingPosGet(INT iBus, INT iSlot, INT iFunc, UINT32 uiMsixCapOft,
                                        INT *piPendingPos);

LW_API INT      API_PciMsiMaskSet(INT iBus, INT iSlot, INT iFunc, UINT32 uiMsixCapOft,
                                  UINT32 uiMask, UINT32 uiFlag);
LW_API INT      API_PciMsiMaskGet(INT iBus, INT iSlot, INT iFunc, UINT32 uiMsixCapOft, UINT32 *puiMask);
LW_API INT      API_PciMsiMaskPosGet(INT iBus, INT iSlot, INT iFunc, UINT32 uiMsixCapOft, INT *piMaskPos);

LW_API UINT32   API_PciMsiMaskConvert(UINT32 uiMask);
LW_API INT      API_PciMsiMultipleGet(INT iBus, INT iSlot, INT iFunc, UINT32 uiMsixCapOft,
                                      INT iNvec, INT *piMultiple);
LW_API INT      API_PciMsiVecCountGet(INT iBus, INT iSlot, INT iFunc,
                                      UINT32 uiMsiCapOft, UINT32 *puiVecCount);
LW_API INT      API_PciMsiMultiCapGet(INT  iBus, INT  iSlot, INT  iFunc, UINT32  uiMsixCapOft,
                                      INT *piMultiCap);
LW_API INT      API_PciMsi64BitGet(INT iBus, INT iSlot, INT iFunc, UINT32 uiMsixCapOft, INT *pi64Bit);
LW_API INT      API_PciMsiMaskBitGet(INT iBus, INT iSlot, INT iFunc, UINT32 uiMsixCapOft, INT *piMaskBit);

LW_API INT      API_PciMsiEnableSet(INT iBus, INT iSlot, INT iFunc, UINT32 uiMsixCapOft, INT iEnable);
LW_API INT      API_PciMsiEnableGet(INT iBus, INT iSlot, INT iFunc, UINT32 uiMsixCapOft, INT *piEnable);

#define pciMsixClearSet         API_PciMsixClearSet

#define pciMsiMsgRead           API_PciMsiMsgRead
#define pciMsiMsgWrite          API_PciMsiMsgWrite

#define pciMsiPendingSet        API_PciMsiPendingSet
#define pciMsiPendingGet        API_PciMsiPendingGet
#define pciMsiPendingPosGet     API_PciMsiPendingPosGet

#define pciMsiMaskSet           API_PciMsiMaskSet
#define pciMsiMaskGet           API_PciMsiMaskGet
#define pciMsiMaskPosGet        API_PciMsiMaskPosGet

#define pciMsiMaskConvert       API_PciMsiMaskConvert
#define pciMsiMultipleGet       API_PciMsiMultipleGet

#define pciMsiVecCountGet       API_PciMsiVecCountGet
#define pciMsiMultiCapGet       API_PciMsiMultiCapGet
#define pciMsi64BitGet          API_PciMsi64BitGet
#define pciMsiMaskBitGet        API_PciMsiMaskBitGet

#define pciMsiEnableSet         API_PciMsiEnableSet
#define pciMsiEnableGet         API_PciMsiEnableGet

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
#endif                                                                  /*  __PCIMSI_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

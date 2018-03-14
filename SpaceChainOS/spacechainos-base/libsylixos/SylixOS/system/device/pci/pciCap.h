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
** 文   件   名: pciCap.h
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2015 年 09 月 17 日
**
** 描        述: PCI 总线扩展功能管理.
*********************************************************************************************************/

#ifndef __PCICAP_H
#define __PCICAP_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)

/*********************************************************************************************************
  API
  必须保证 API_PciConfigInit 被正确的调用.
*********************************************************************************************************/
static LW_INLINE CHAR * __pciCapHtLinkWidth (UINT32  uiWidth)
{
  static CHAR * const pcWidths[8] = {"8bit", "16bit", "[2]", "32bit", "2bit", "4bit", "[6]", "N/C" };

  return  (pcWidths[uiWidth]);
}

static LW_INLINE CHAR * __pciCapHtLinkFreq (UINT32  uiFreq)
{
  static CHAR * const pcFreqs[16] = {"200MHz", "300MHz", "400MHz", "500MHz", "600MHz", "800MHz",
                                     "1.0GHz", "1.2GHz", "1.4GHz", "1.6GHz", "[a]", "[b]", "[c]",
                                     "[d]", "[e]", "Vend" };
  return  (pcFreqs[uiFreq]);
}

LW_API INT      API_PciCapShow(INT  iBus, INT  iSlot, INT  iFunc);
LW_API INT      API_PciCapFind(INT  iBus, INT  iSlot, INT  iFunc, UINT8  ucCapId, UINT32 *puiOffset);
LW_API INT      API_PciCapEn(INT  iBus, INT  iSlot, INT  iFunc);

#define pciCapShow              API_PciCapShow
#define pciCapFind              API_PciCapFind
#define pciCapEn                API_PciCapEn

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
#endif                                                                  /*  __PCICAP_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: pciCap.c
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2015 年 09 月 17 日
**
** 描        述: PCI 总线扩展功能管理.
**
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
#include "pciLib.h"
#include "pciDev.h"
#include "pciScan.h"
#include "pciBus.h"
#include "pciCap.h"
#include "pciShow.h"
#include "pciVpd.h"
#include "pciCapExt.h"
/*********************************************************************************************************
** 函数名称: __pciCapAfShow
** 功能描述: 打印设备的扩展能力 AF 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapAfShow (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    UINT8       reg;

    printf("PCI Advanced Features\n");
    if ((_G_iPciVerbose < 2) ||
        (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + PCI_AF_CAP), 3))) {
        printf("\n");
        return;
    }

    API_PciConfigInByte(iBus, iSlot, iFunc, (iWhere + PCI_AF_CAP), &reg);
    printf("\t\tAFCap: TP%c FLR%c\n", PCI_FLAG(reg, PCI_AF_CAP_TP),
           PCI_FLAG(reg, PCI_AF_CAP_FLR));
    API_PciConfigInByte(iBus, iSlot, iFunc, (iWhere + PCI_AF_CTRL), &reg);
    printf("\t\tAFCtrl: FLR%c\n", PCI_FLAG(reg, PCI_AF_CTRL_FLR));
    API_PciConfigInByte(iBus, iSlot, iFunc, (iWhere + PCI_AF_STATUS), &reg);
    printf("\t\tAFStatus: TP%c\n", PCI_FLAG(reg, PCI_AF_STATUS_TP));
}
/*********************************************************************************************************
** 函数名称: __pciCapSataHbaShow
** 功能描述: 打印设备的扩展能力 SATA HBA 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
**           iCap       扩展信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapSataHbaShow (INT iBus, INT iSlot, INT iFunc, INT iWhere, INT iCap)
{
    UINT32      bars;
    INT         bar;

    printf("SATA HBA v%d.%d", PCI_BITS(iCap, 4, 4), PCI_BITS(iCap, 0, 4));
    if ((_G_iPciVerbose < 2) ||
        (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + PCI_SATA_HBA_BARS), 4))) {
        printf("\n");
        return;
    }

    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_SATA_HBA_BARS), &bars);
    bar = PCI_BITS(bars, 0, 4);
    if (bar >= 4 && bar <= 9) {
        printf(" BAR%d Offset=%08x\n", bar - 4, PCI_BITS(bars, 4, 20));
    } else if (bar == 15) {
        printf(" InCfgSpace\n");
    } else {
        printf(" BAR??%d\n", bar);
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapMsixShow
** 功能描述: 打印设备的扩展能力 MSIX 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
**           iCap       扩展信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapMsixShow (INT iBus, INT iSlot, INT iFunc, INT iWhere, INT iCap)
{
    UINT32      off;

    printf("MSI-X: Enable%c Count=%d Masked%c\n",
           PCI_FLAG(iCap, PCI_MSIX_ENABLE),
           (iCap & PCI_MSIX_TABSIZE) + 1,
           PCI_FLAG(iCap, PCI_MSIX_MASK));
    if ((_G_iPciVerbose < 2) ||
        (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + PCI_MSIX_TABLE), 8))) {
        return;
    }

    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_MSIX_TABLE), &off);
    printf("\t\tVector table: BAR=%d offset=%08x\n",
           off & PCI_MSIX_BIR, off & ~PCI_MSIX_BIR);
    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_MSIX_PBA), &off);
    printf("\t\tPBA: BAR=%d offset=%08x\n",
           off & PCI_MSIX_BIR, off & ~PCI_MSIX_BIR);
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressSlot2
** 功能描述: 设备的扩展能力 Express SLOT 2 处理.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExpressSlot2 (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    /*
     *  No capabilities that require this field in PCIe rev2.0 spec.
     */
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressLink2Transmargin
** 功能描述: 设备的扩展能力 Express DEV 2 连接速度.
** 输　入  : iType      类型信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static const CHAR *__pciCapExpressLink2Transmargin (INT iType)
{
    switch (iType) {
    
    case 0:
        return "Normal Operating Range";

    case 1:
        return "800-1200mV(full-swing)/400-700mV(half-swing)";

    case 2:
    case 3:
    case 4:
    case 5:
        return "200-400mV(full-swing)/100-200mV(half-swing)";

    default:
        return "Unknown";
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressLink2Deemphasis
** 功能描述: 设备的扩展能力 Express DEV 2 连接速度.
** 输　入  : iType      类型信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static const CHAR *__pciCapExpressLink2Deemphasis (INT iType)
{
    switch (iType) {
    
    case 0:
        return "-6dB";

    case 1:
        return "-3.5dB";

    default:
        return "Unknown";
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressLink2Speed
** 功能描述: 设备的扩展能力 Express DEV 2 连接速度.
** 输　入  : iType      类型信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static const CHAR *__pciCapExpressLink2Speed (INT iType)
{
    /*
     *  hardwire to 0 means only the 2.5GT/s is supported
     */
    switch (iType) {
    
    case 0:
    case 1:
        return "2.5GT/s";

    case 2:
        return "5GT/s";

    case 3:
        return "8GT/s";

    default:
        return "Unknown";
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressLink2
** 功能描述: 设备的扩展能力 Express LINK 2 处理.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
**           iType      类型信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExpressLink2 (INT iBus, INT iSlot, INT iFunc, INT iWhere, INT iType)
{
    UINT16      w;

    if (!(((iType == PCI_EXP_TYPE_ENDPOINT) || (iType == PCI_EXP_TYPE_LEG_END)) &&
          (iSlot != 0 || iFunc != 0                                           ))) {
        API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_EXP_LNKCTL2), &w);
        printf("\t\tLnkCtl2: Target Link Speed: %s, EnterCompliance%c SpeedDis%c",
               __pciCapExpressLink2Speed(PCI_EXP_LNKCTL2_SPEED(w)),
               PCI_FLAG(w, PCI_EXP_LNKCTL2_CMPLNC),
               PCI_FLAG(w, PCI_EXP_LNKCTL2_SPEED_DIS));
    if (iType == PCI_EXP_TYPE_DOWNSTREAM) {
        printf(", Selectable De-emphasis: %s",
               __pciCapExpressLink2Deemphasis(PCI_EXP_LNKCTL2_DEEMPHASIS(w)));
    }
    printf("\n"
           "\t\t\t Transmit Margin: %s, EnterModifiedCompliance%c ComplianceSOS%c\n"
           "\t\t\t Compliance De-emphasis: %s\n",
           __pciCapExpressLink2Transmargin(PCI_EXP_LNKCTL2_MARGIN(w)),
           PCI_FLAG(w, PCI_EXP_LNKCTL2_MOD_CMPLNC),
           PCI_FLAG(w, PCI_EXP_LNKCTL2_CMPLNC_SOS),
           __pciCapExpressLink2Deemphasis(PCI_EXP_LNKCTL2_COM_DEEMPHASIS(w)));
    }

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_EXP_LNKSTA2), &w);
    printf("\t\tLnkSta2: Current De-emphasis Level: %s, EqualizationComplete%c, EqualizationPhase1%c\n"
           "\t\t\t EqualizationPhase2%c, EqualizationPhase3%c, LinkEqualizationRequest%c\n",
           __pciCapExpressLink2Deemphasis(PCI_EXP_LINKSTA2_DEEMPHASIS(w)),
           PCI_FLAG(w, PCI_EXP_LINKSTA2_EQU_COMP),
           PCI_FLAG(w, PCI_EXP_LINKSTA2_EQU_PHASE1),
           PCI_FLAG(w, PCI_EXP_LINKSTA2_EQU_PHASE2),
           PCI_FLAG(w, PCI_EXP_LINKSTA2_EQU_PHASE3),
           PCI_FLAG(w, PCI_EXP_LINKSTA2_EQU_REQ));
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressDevctl2Obff
** 功能描述: 设备的扩展能力 Express DEV CTL 2.
** 输　入  : iObff      参数值
** 输　出  : 参数信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static const CHAR *__pciCapExpressDevctl2Obff (INT  iObff)
{
    switch (iObff) {
    
    case 0:
        return "Disabled";

    case 1:
        return "Via message A";

    case 2:
        return "Via message B";

    case 3:
        return "Via WAKE#";

    default:
        return "Unknown";
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressDev2TimeoutValue
** 功能描述: 设备的扩展能力 Express DEV 2 超时时间.
** 输　入  : iType      类型信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static const CHAR *__pciCapExpressDev2TimeoutValue (INT iType)
{
    /*
     *  Decode Completion Timeout Value.
     */
    switch (iType) {
    
    case 0:
        return "50us to 50ms";

    case 1:
        return "50us to 100us";

    case 2:
        return "1ms to 10ms";

    case 5:
        return "16ms to 55ms";

    case 6:
        return "65ms to 210ms";

    case 9:
        return "260ms to 900ms";

    case 10:
        return "1s to 3.5s";

    case 13:
        return "4s to 13s";

    case 14:
        return "17s to 64s";

    default:
        return "Unknown";
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressDevcap2Obff
** 功能描述: 设备的扩展能力 Express DEV CAP 2.
** 输　入  : iObff      参数值
** 输　出  : 参数信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static const CHAR *__pciCapExpressDevcap2Obff (INT  iObff)
{
    switch (iObff) {
    
    case 1:
        return "Via message";

    case 2:
        return "Via WAKE#";

    case 3:
        return "Via message/WAKE#";

    default:
        return "Not Supported";
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressDev2TimeoutRange
** 功能描述: 设备的扩展能力 Express DEV 2 超时范围.
** 输　入  : iType      类型信息
** 输　出  : 参数信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static const CHAR *__pciCapExpressDev2TimeoutRange (INT iType)
{
    /*
     *  Decode Completion Timeout Ranges.
     */
    switch (iType) {
    
    case 0:
        return "Not Supported";

    case 1:
        return "Range A";

    case 2:
        return "Range B";

    case 3:
        return "Range AB";

    case 6:
        return "Range BC";

    case 7:
        return "Range ABC";

    case 14:
        return "Range BCD";

    case 15:
        return "Range ABCD";

    default:
        return "Unknown";
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressDev2
** 功能描述: 设备的扩展能力 Express DEV 2 处理.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
**           iType      类型信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExpressDev2 (INT iBus, INT iSlot, INT iFunc, INT iWhere, INT iType)
{
    UINT32      l;
    UINT16      w;

    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_EXP_DEVCAP2), &l);
    printf("\t\tDevCap2: Completion Timeout: %s, TimeoutDis%c, LTR%c, OBFF %s",
           __pciCapExpressDev2TimeoutRange(PCI_EXP_DEV2_TIMEOUT_RANGE(l)),
           PCI_FLAG(l, PCI_EXP_DEV2_TIMEOUT_DIS),
           PCI_FLAG(l, PCI_EXP_DEVCAP2_LTR),
           __pciCapExpressDevcap2Obff(PCI_EXP_DEVCAP2_OBFF(l)));
    if ((iType == PCI_EXP_TYPE_ROOT_PORT ) ||
        (iType == PCI_EXP_TYPE_DOWNSTREAM)) {
        printf(" ARIFwd%c\n", PCI_FLAG(l, PCI_EXP_DEV2_ARI));
    } else {
        printf("\n");
    }

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_EXP_DEVCTL2), &w);
    printf("\t\tDevCtl2: Completion Timeout: %s, TimeoutDis%c, LTR%c, OBFF %s",
           __pciCapExpressDev2TimeoutValue(PCI_EXP_DEV2_TIMEOUT_VALUE(w)),
           PCI_FLAG(w, PCI_EXP_DEV2_TIMEOUT_DIS),
           PCI_FLAG(w, PCI_EXP_DEV2_LTR),
           __pciCapExpressDevctl2Obff(PCI_EXP_DEV2_OBFF(w)));
    if ((iType == PCI_EXP_TYPE_ROOT_PORT ) ||
        (iType == PCI_EXP_TYPE_DOWNSTREAM)) {
        printf(" ARIFwd%c\n", PCI_FLAG(w, PCI_EXP_DEV2_ARI));
    } else {
        printf("\n");
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressIndicator
** 功能描述: 设备的扩展能力 Express 指示器.
** 输　入  : iCode      参数值
** 输　出  : 参数信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static const CHAR *__pciCapExpressIndicator (INT  iCode)
{
    static const CHAR *names[] = {"Unknown", "On", "Blink", "Off"};

    return  (names[iCode]);
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressAspmEnabled
** 功能描述: 设备的扩展能力 Express 功率模块使能信息.
** 输　入  : iSpeed     参数值
** 输　出  : 参数信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static const CHAR *__pciCapExpressAspmEnabled (INT  iCode)
{
    static const CHAR *desc[] = {"Disabled", "L0s Enabled", "L1 Enabled", "L0s L1 Enabled"};

    return  (desc[iCode]);
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressAspmSupport
** 功能描述: 设备的扩展能力 Express 功率模块支持.
** 输　入  : iSpeed     参数值
** 输　出  : 参数信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static CHAR *__pciCapExpressAspmSupport (INT  iCode)
{
    switch (iCode) {
    
    case 0:
        return "not supported";

    case 1:
        return "L0s";

    case 2:
        return "L1";

    case 3:
        return "L0s L1";

    default:
        return "unknown";
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressLinkSpeed
** 功能描述: 设备的扩展能力 Express 连接速度.
** 输　入  : iSpeed     参数值
** 输　出  : 参数信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static CHAR *__pciCapExpressLinkSpeed (INT  iSpeed)
{
    switch (iSpeed) {
    
    case 1:
        return "2.5GT/s";

    case 2:
        return "5GT/s";

    case 3:
        return "8GT/s";

    default:
        return "unknown";
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressPowerLimit
** 功能描述: 设备的扩展能力 Express 电源限制.
** 输　入  : iValue     参数值
**           iScale     比例
** 输　出  : 参数值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT __pciCapExpressPowerLimit (INT  iValue, INT  iScale)
{
    static const UINT scales[4] = {1000, 100, 10, 1};

    return  (iValue * scales[iScale]);
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressLatencyL1
** 功能描述: 设备的扩展能力 Express 等待时间 L1.
** 输　入  : iValue     参数值
** 输　出  : 参数信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static const CHAR *__pciCapExpressLatencyL1 (INT  iValue)
{
    static const char *latencies[] = {"<1us", "<2us", "<4us", "<8us", "<16us", "<32us", "<64us",
                                      "unlimited"};
    return  (latencies[iValue]);
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressLatencyL0s
** 功能描述: 设备的扩展能力 Express 等待时间 L0S.
** 输　入  : iValue     参数值
** 输　出  : 参数信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static const CHAR *__pciCapExpressLatencyL0s (INT  iValue)
{
    static const CHAR *latencies[] = {"<64ns", "<128ns", "<256ns", "<512ns", "<1us", "<2us", "<4us",
                                      "unlimited"};
    return  (latencies[iValue]);
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressRoot
** 功能描述: 设备的扩展能力 Express ROOT 处理.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExpressRoot (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    UINT16      w;

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_EXP_RTCTL), &w);
    printf("\t\tRootCtl: ErrCorrectable%c ErrNon-Fatal%c ErrFatal%c PMEIntEna%c CRSVisible%c\n",
           PCI_FLAG(w, PCI_EXP_RTCTL_SECEE),
           PCI_FLAG(w, PCI_EXP_RTCTL_SENFEE),
           PCI_FLAG(w, PCI_EXP_RTCTL_SEFEE),
           PCI_FLAG(w, PCI_EXP_RTCTL_PMEIE),
           PCI_FLAG(w, PCI_EXP_RTCTL_CRSVIS));

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_EXP_RTCAP), &w);
    printf("\t\tRootCap: CRSVisible%c\n",
           PCI_FLAG(w, PCI_EXP_RTCAP_CRSVIS));

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_EXP_RTSTA), &w);
    printf("\t\tRootSta: PME ReqID %04x, PMEStatus%c PMEPending%c\n",
           w & PCI_EXP_RTSTA_PME_REQID,
           PCI_FLAG(w, PCI_EXP_RTSTA_PME_STATUS),
           PCI_FLAG(w, PCI_EXP_RTSTA_PME_PENDING));
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressSlot
** 功能描述: 设备的扩展能力 Express SLOT 处理.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExpressSlot (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    UINT32      t;
    UINT16      w;

    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_EXP_SLTCAP), &t);
    printf("\t\tSltCap:\tAttnBtn%c PwrCtrl%c MRL%c AttnInd%c PwrInd%c HotPlug%c Surprise%c\n",
           PCI_FLAG(t, PCI_EXP_SLTCAP_ATNB),
           PCI_FLAG(t, PCI_EXP_SLTCAP_PWRC),
           PCI_FLAG(t, PCI_EXP_SLTCAP_MRL),
           PCI_FLAG(t, PCI_EXP_SLTCAP_ATNI),
           PCI_FLAG(t, PCI_EXP_SLTCAP_PWRI),
           PCI_FLAG(t, PCI_EXP_SLTCAP_HPC),
           PCI_FLAG(t, PCI_EXP_SLTCAP_HPS));
    printf("\t\t\tSlot #%d, PowerLimit %umW; Interlock%c NoCompl%c\n",
           t >> 19,
           __pciCapExpressPowerLimit((t & PCI_EXP_SLTCAP_PWR_VAL) >> 7,
                                     (t & PCI_EXP_SLTCAP_PWR_SCL) >> 15),
           PCI_FLAG(t, PCI_EXP_SLTCAP_INTERLOCK),
           PCI_FLAG(t, PCI_EXP_SLTCAP_NOCMDCOMP));

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_EXP_SLTCTL), &w);
    printf("\t\tSltCtl:\tEnable: AttnBtn%c PwrFlt%c MRL%c PresDet%c CmdCplt%c HPIrq%c LinkChg%c\n",
           PCI_FLAG(w, PCI_EXP_SLTCTL_ATNB),
           PCI_FLAG(w, PCI_EXP_SLTCTL_PWRF),
           PCI_FLAG(w, PCI_EXP_SLTCTL_MRLS),
           PCI_FLAG(w, PCI_EXP_SLTCTL_PRSD),
           PCI_FLAG(w, PCI_EXP_SLTCTL_CMDC),
           PCI_FLAG(w, PCI_EXP_SLTCTL_HPIE),
           PCI_FLAG(w, PCI_EXP_SLTCTL_LLCHG));
    printf("\t\t\tControl: AttnInd %s, PwrInd %s, Power%c Interlock%c\n",
           __pciCapExpressIndicator((w & PCI_EXP_SLTCTL_ATNI) >> 6),
           __pciCapExpressIndicator((w & PCI_EXP_SLTCTL_PWRI) >> 8),
           PCI_FLAG(w, PCI_EXP_SLTCTL_PWRC),
           PCI_FLAG(w, PCI_EXP_SLTCTL_INTERLOCK));

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_EXP_SLTSTA), &w);
    printf("\t\tSltSta:\tStatus: AttnBtn%c PowerFlt%c MRL%c CmdCplt%c PresDet%c Interlock%c\n",
           PCI_FLAG(w, PCI_EXP_SLTSTA_ATNB),
           PCI_FLAG(w, PCI_EXP_SLTSTA_PWRF),
           PCI_FLAG(w, PCI_EXP_SLTSTA_MRL_ST),
           PCI_FLAG(w, PCI_EXP_SLTSTA_CMDC),
           PCI_FLAG(w, PCI_EXP_SLTSTA_PRES),
           PCI_FLAG(w, PCI_EXP_SLTSTA_INTERLOCK));
    printf("\t\t\tChanged: MRL%c PresDet%c LinkState%c\n",
           PCI_FLAG(w, PCI_EXP_SLTSTA_MRLS),
           PCI_FLAG(w, PCI_EXP_SLTSTA_PRSD),
           PCI_FLAG(w, PCI_EXP_SLTSTA_LLCHG));
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressLink
** 功能描述: 设备的扩展能力 Express LINK 处理.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
**           iType      类型信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExpressLink (INT iBus, INT iSlot, INT iFunc, INT iWhere, INT iType)
{
    UINT32      t;
    UINT16      w;

    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_EXP_LNKCAP), &t);
    printf("\t\tLnkCap:\tPort #%d, Speed %s, Width x%d, ASPM %s, Exit Latency L0s %s, L1 %s\n",
           t >> 24,
           __pciCapExpressLinkSpeed(t & PCI_EXP_LNKCAP_SPEED), (t & PCI_EXP_LNKCAP_WIDTH) >> 4,
           __pciCapExpressAspmSupport((t & PCI_EXP_LNKCAP_ASPM) >> 10),
           __pciCapExpressLatencyL0s((t & PCI_EXP_LNKCAP_L0S) >> 12),
           __pciCapExpressLatencyL1((t & PCI_EXP_LNKCAP_L1) >> 15));
    printf("\t\t\tClockPM%c Surprise%c LLActRep%c BwNot%c ASPMOptComp%c\n",
           PCI_FLAG(t, PCI_EXP_LNKCAP_CLOCKPM),
           PCI_FLAG(t, PCI_EXP_LNKCAP_SURPRISE),
           PCI_FLAG(t, PCI_EXP_LNKCAP_DLLA),
           PCI_FLAG(t, PCI_EXP_LNKCAP_LBNC),
           PCI_FLAG(t, PCI_EXP_LNKCAP_AOC));

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_EXP_LNKCTL), &w);
    printf("\t\tLnkCtl:\tASPM %s;", __pciCapExpressAspmEnabled(w & PCI_EXP_LNKCTL_ASPM));
    if ((iType == PCI_EXP_TYPE_ROOT_PORT) ||
        (iType == PCI_EXP_TYPE_ENDPOINT)  ||
        (iType == PCI_EXP_TYPE_LEG_END)   ||
        (iType == PCI_EXP_TYPE_PCI_BRIDGE)) {
        printf(" RCB %d bytes", w & PCI_EXP_LNKCTL_RCB ? 128 : 64);
    }
    printf(" Disabled%c CommClk%c\n\t\t\tExtSynch%c ClockPM%c AutWidDis%c BWInt%c AutBWInt%c\n",
           PCI_FLAG(w, PCI_EXP_LNKCTL_DISABLE),
           PCI_FLAG(w, PCI_EXP_LNKCTL_CLOCK),
           PCI_FLAG(w, PCI_EXP_LNKCTL_XSYNCH),
           PCI_FLAG(w, PCI_EXP_LNKCTL_CLOCKPM),
           PCI_FLAG(w, PCI_EXP_LNKCTL_HWAUTWD),
           PCI_FLAG(w, PCI_EXP_LNKCTL_BWMIE),
           PCI_FLAG(w, PCI_EXP_LNKCTL_AUTBWIE));

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_EXP_LNKSTA), &w);
    printf("\t\tLnkSta:\tSpeed %s, Width x%d, TrErr%c Train%c SlotClk%c DLActive%c BWMgmt%c ABWMgmt%c\n",
           __pciCapExpressLinkSpeed(w & PCI_EXP_LNKSTA_SPEED),
           (w & PCI_EXP_LNKSTA_WIDTH) >> 4,
           PCI_FLAG(w, PCI_EXP_LNKSTA_TR_ERR),
           PCI_FLAG(w, PCI_EXP_LNKSTA_TRAIN),
           PCI_FLAG(w, PCI_EXP_LNKSTA_SL_CLK),
           PCI_FLAG(w, PCI_EXP_LNKSTA_DL_ACT),
           PCI_FLAG(w, PCI_EXP_LNKSTA_BWMGMT),
           PCI_FLAG(w, PCI_EXP_LNKSTA_AUTBW));
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressDev
** 功能描述: 设备的扩展能力 Express DEV 处理.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
**           iType      类型信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExpressDev (INT iBus, INT iSlot, INT iFunc, INT iWhere, INT iType)
{
    UINT32      t;
    UINT16      w;

    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_EXP_DEVCAP), &t);
    printf("\t\tDevCap:\tMaxPayload %d bytes, PhantFunc %d",
           128 << (t & PCI_EXP_DEVCAP_PAYLOAD),
           (1 << ((t & PCI_EXP_DEVCAP_PHANTOM) >> 3)) - 1);
    if ((iType == PCI_EXP_TYPE_ENDPOINT) || (iType == PCI_EXP_TYPE_LEG_END)) {
        printf(", Latency L0s %s, L1 %s",
               __pciCapExpressLatencyL0s((t & PCI_EXP_DEVCAP_L0S) >> 6),
               __pciCapExpressLatencyL1((t & PCI_EXP_DEVCAP_L1) >> 9));
    }
    printf("\n");
    printf("\t\t\tExtTag%c", PCI_FLAG(t, PCI_EXP_DEVCAP_EXT_TAG));
    if ((iType == PCI_EXP_TYPE_ENDPOINT) || (iType == PCI_EXP_TYPE_LEG_END) ||
        (iType == PCI_EXP_TYPE_UPSTREAM) || (iType == PCI_EXP_TYPE_PCI_BRIDGE)) {
        printf(" AttnBtn%c AttnInd%c PwrInd%c",
               PCI_FLAG(t, PCI_EXP_DEVCAP_ATN_BUT),
               PCI_FLAG(t, PCI_EXP_DEVCAP_ATN_IND), PCI_FLAG(t, PCI_EXP_DEVCAP_PWR_IND));
    }
    printf(" RBE%c",
           PCI_FLAG(t, PCI_EXP_DEVCAP_RBE));
    if ((iType == PCI_EXP_TYPE_ENDPOINT) || (iType == PCI_EXP_TYPE_LEG_END)) {
        printf(" FLReset%c",
               PCI_FLAG(t, PCI_EXP_DEVCAP_FLRESET));
    }
    if (iType == PCI_EXP_TYPE_UPSTREAM) {
        printf(" SlotPowerLimit %umW",
               __pciCapExpressPowerLimit((t & PCI_EXP_DEVCAP_PWR_VAL) >> 18,
                                         (t & PCI_EXP_DEVCAP_PWR_SCL) >> 26));
    }
    printf("\n");

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_EXP_DEVCTL), &w);
    printf("\t\tDevCtl:\tReport errors: Correctable%c Non-Fatal%c Fatal%c Unsupported%c\n",
           PCI_FLAG(w, PCI_EXP_DEVCTL_CERE),
           PCI_FLAG(w, PCI_EXP_DEVCTL_NFERE),
           PCI_FLAG(w, PCI_EXP_DEVCTL_FERE),
           PCI_FLAG(w, PCI_EXP_DEVCTL_URRE));
    printf("\t\t\tRlxdOrd%c ExtTag%c PhantFunc%c AuxPwr%c NoSnoop%c",
           PCI_FLAG(w, PCI_EXP_DEVCTL_RELAXED),
           PCI_FLAG(w, PCI_EXP_DEVCTL_EXT_TAG),
           PCI_FLAG(w, PCI_EXP_DEVCTL_PHANTOM),
           PCI_FLAG(w, PCI_EXP_DEVCTL_AUX_PME),
           PCI_FLAG(w, PCI_EXP_DEVCTL_NOSNOOP));
    if (iType == PCI_EXP_TYPE_PCI_BRIDGE) {
        printf(" BrConfRtry%c", PCI_FLAG(w, PCI_EXP_DEVCTL_BCRE));
    }
    if (((iType == PCI_EXP_TYPE_ENDPOINT) || (iType == PCI_EXP_TYPE_LEG_END)) &&
        (t & PCI_EXP_DEVCAP_FLRESET                                         )) {
        printf(" FLReset%c", PCI_FLAG(w, PCI_EXP_DEVCTL_FLRESET));
    }
    printf("\n\t\t\tMaxPayload %d bytes, MaxReadReq %d bytes\n",
           128 << ((w & PCI_EXP_DEVCTL_PAYLOAD) >> 5),
           128 << ((w & PCI_EXP_DEVCTL_READRQ) >> 12));

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_EXP_DEVSTA), &w);
    printf("\t\tDevSta:\tCorrErr%c UncorrErr%c FatalErr%c UnsuppReq%c AuxPwr%c TransPend%c\n",
           PCI_FLAG(w, PCI_EXP_DEVSTA_CED),
           PCI_FLAG(w, PCI_EXP_DEVSTA_NFED),
           PCI_FLAG(w, PCI_EXP_DEVSTA_FED),
           PCI_FLAG(w, PCI_EXP_DEVSTA_URD),
           PCI_FLAG(w, PCI_EXP_DEVSTA_AUXPD),
           PCI_FLAG(w, PCI_EXP_DEVSTA_TRPND));
}
/*********************************************************************************************************
** 函数名称: __pciCapExpressShow
** 功能描述: 打印设备的扩展能力 Express 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
**           iCap       扩展信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExpressShow (INT iBus, INT iSlot, INT iFunc, INT iWhere, INT iCap)
{
    INT     type = (iCap & PCI_EXP_FLAGS_TYPE) >> 4;
    INT     size = 0;
    INT     slot = 0;
    INT     link = 1;

    printf("Express ");
    if (_G_iPciVerbose >= 2) {
        printf("(v%d) ", iCap & PCI_EXP_FLAGS_VERS);
    }

    switch (type) {
    
    case PCI_EXP_TYPE_ENDPOINT:
        printf("Endpoint");
        break;

    case PCI_EXP_TYPE_LEG_END:
        printf("Legacy Endpoint");
        break;

    case PCI_EXP_TYPE_ROOT_PORT:
        slot = iCap & PCI_EXP_FLAGS_SLOT;
        printf("Root Port (Slot%c)", PCI_FLAG(iCap, PCI_EXP_FLAGS_SLOT));
        break;

    case PCI_EXP_TYPE_UPSTREAM:
        printf("Upstream Port");
        break;

    case PCI_EXP_TYPE_DOWNSTREAM:
        slot = iCap & PCI_EXP_FLAGS_SLOT;
        printf("Downstream Port (Slot%c)", PCI_FLAG(iCap, PCI_EXP_FLAGS_SLOT));
        break;

    case PCI_EXP_TYPE_PCI_BRIDGE:
        printf("PCI-Express to PCI/PCI-X Bridge");
        break;

    case PCI_EXP_TYPE_PCIE_BRIDGE:
        printf("PCI/PCI-X to PCI-Express Bridge");
        break;

    case PCI_EXP_TYPE_ROOT_INT_EP:
        link = 0;
        printf("Root Complex Integrated Endpoint");
        break;

    case PCI_EXP_TYPE_ROOT_EC:
        link = 0;
        printf("Root Complex Event Collector");
        break;

    default:
        printf("Unknown type %d", type);
        break;
    }

    printf(", MSI %02x\n", (iCap & PCI_EXP_FLAGS_IRQ) >> 9);
    if (_G_iPciVerbose < 2) {
        return;
    }

    size = 16;
    if (slot) {
        size = 24;
    }
    if (type == PCI_EXP_TYPE_ROOT_PORT) {
        size = 32;
    }
    if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + PCI_EXP_DEVCAP), size)) {
        return;
    }

    __pciCapExpressDev(iBus, iSlot, iFunc, iWhere, type);
    if (link) {
        __pciCapExpressLink(iBus, iSlot, iFunc, iWhere, type);
    }
    if (slot) {
        __pciCapExpressSlot(iBus, iSlot, iFunc, iWhere);
    }
    if (type == PCI_EXP_TYPE_ROOT_PORT) {
        __pciCapExpressRoot(iBus, iSlot, iFunc, iWhere);
    }

    if ((iCap & PCI_EXP_FLAGS_VERS) < 2) {
        return;
    }
    size = 16;
    if (slot) {
        size = 24;
    }
    if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + PCI_EXP_DEVCAP2), size)) {
        return;
    }

    __pciCapExpressDev2(iBus, iSlot, iFunc, iWhere, type);
    if (link) {
        __pciCapExpressLink2(iBus, iSlot, iFunc, iWhere, type);
    }
    if (slot) {
        __pciCapExpressSlot2(iBus, iSlot, iFunc, iWhere);
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapSsvidShow
** 功能描述: 打印设备的扩展能力 SSVID 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapSsvidShow (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    UINT16      subsys_v, subsys_d;

    if (!API_PciConfigFetch(iBus, iSlot, iFunc, iWhere, 8)) {
        return;
    }

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_SSVID_VENDOR), &subsys_v);
    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_SSVID_DEVICE), &subsys_d);
    printf("Subsystem: 0x%4x 0x%4x\n", subsys_v, subsys_d);
}
/*********************************************************************************************************
** 函数名称: __pciCapDebugPortShow
** 功能描述: 打印设备的扩展能力 Debug Port.
** 输　入  : iCap       扩展信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapDebugPortShow (INT iCap)
{
    INT     bar = iCap >> 13;
    INT     pos = iCap & 0x1fff;

    printf("Debug port: BAR=%d offset=%04x\n", bar, pos);
}
/*********************************************************************************************************
** 函数名称: __pciCapVendorVirtio
** 功能描述: 打印设备的扩展能力 VENDOR 处理.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
**           iCap       扩展信息
** 输　出  : 0 or 1
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciCapVendorVirtio (INT iBus, INT iSlot, INT iFunc, INT iWhere, INT iCap)
{
    INT         length = PCI_BITS(iCap, 0, 8);
    INT         type   = PCI_BITS(iCap, 8, 8);
    CHAR       *tname  = LW_NULL;
    UINT8       bar;
    UINT32      offset;
    UINT32      size;
    UINT32      multiplier;

    if (length < 16) {
        return 0;
    }

    if (!API_PciConfigFetch(iBus, iSlot, iFunc, iWhere, length)) {
        return  (0);
    }

    switch (type) {
    
    case 1:
        tname = "CommonCfg";
        break;

    case 2:
        tname = "Notify";
        break;

    case 3:
        tname = "ISR";
        break;

    case 4:
        tname = "DeviceCfg";
        break;

    default:
        tname = "<unknown>";
        break;
    }

    printf("VirtIO: %s\n", tname);

    if (_G_iPciVerbose < 2) {
        return  (1);
    }

    API_PciConfigInByte (iBus, iSlot, iFunc, (iWhere +  4), &bar);
    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere +  8), &offset);
    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + 12), &size);
    printf("\t\tBAR=%d offset=%08x size=%08x", bar, offset, size);

    if (type == 2 && length >= 20) {
        API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + 16), &multiplier);
        printf(" multiplier=%08x", multiplier);
    }

    printf("\n");

    return  (1);
}
/*********************************************************************************************************
** 函数名称: __pciCapVendorDo
** 功能描述: 打印设备的扩展能力 VENDOR 处理.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
**           iCap       扩展信息
** 输　出  : 0 or 1
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciCapVendorDo (INT iBus, INT iSlot, INT iFunc, INT iWhere, INT iCap)
{
    UINT16      usVendor;
    UINT16      usDevice;

    API_PciConfigInWord(iBus, iSlot, iFunc, PCI_VENDOR_ID, &usVendor);
    
    switch (usVendor) {
    
    case 0x1af4:                                                        /*  Red Hat                     */
        API_PciConfigInWord(iBus, iSlot, iFunc, PCI_DEVICE_ID, &usDevice);
        if ((usDevice >= 0x1000) &&
            (usDevice <= 0x107f)) {
            return  (__pciCapVendorVirtio(iBus, iSlot, iFunc, iWhere, iCap));
        }
        break;
    }

    return  (0);
}
/*********************************************************************************************************
** 函数名称: __pciCapVendorShow
** 功能描述: 打印设备的扩展能力 VENDOR 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
**           iCap       扩展信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapVendorShow (INT iBus, INT iSlot, INT iFunc, INT iWhere, INT iCap)
{
    printf("Vendor Specific Information: ");
    if (!__pciCapVendorDo(iBus, iSlot, iFunc, iWhere, iCap)) {
        printf("Len=%02x <?>\n", PCI_BITS(iCap, 0, 8));
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapHtSecShow
** 功能描述: 打印设备的扩展能力 HT SEC 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
**           iCmd       命令信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapHtSecShow (INT iBus, INT iSlot, INT iFunc, INT iWhere, INT iCmd)
{
    UINT16      lctr, lcnf, ftr, eh;
    UINT8       rid, lfrer, lfcap, mbu, mlu;
    CHAR       *fmt;

    printf("HyperTransport: Host or Secondary Interface\n");

    if (_G_iPciVerbose < 2) {
        return;
    }
    if (!API_PciConfigFetch(iBus, iSlot, iFunc,
                            (iWhere + PCI_HT_SEC_LCTR), (PCI_HT_SEC_SIZEOF - PCI_HT_SEC_LCTR))) {
        return;
    }

    API_PciConfigInByte(iBus, iSlot, iFunc, (iWhere + PCI_HT_SEC_RID), &rid);
    if (rid < 0x22 && rid > 0x11) {
        printf("\t\t!!! Possibly incomplete decoding\n");
    }
    if (rid >= 0x22) {
        fmt ="\t\tCommand: WarmRst%c DblEnd%c DevNum=%u ChainSide%c HostHide%c Slave%c <EOCErr%c DUL%c\n";
    } else {
        fmt = "\t\tCommand: WarmRst%c DblEnd%c\n";
    }
    printf(fmt,
           PCI_FLAG(iCmd, PCI_HT_SEC_CMD_WR),
           PCI_FLAG(iCmd, PCI_HT_SEC_CMD_DE),
           (iCmd & PCI_HT_SEC_CMD_DN) >> 2,
           PCI_FLAG(iCmd, PCI_HT_SEC_CMD_CS),
           PCI_FLAG(iCmd, PCI_HT_SEC_CMD_HH),
           PCI_FLAG(iCmd, PCI_HT_SEC_CMD_AS),
           PCI_FLAG(iCmd, PCI_HT_SEC_CMD_HIECE),
           PCI_FLAG(iCmd, PCI_HT_SEC_CMD_DUL));
    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_HT_SEC_LCTR), &lctr);
    if (rid >= 0x22) {
        fmt = "\t\tLink Control: CFlE%c CST%c CFE%c <LkFail%c Init%c EOC%c TXO%c <CRCErr=%x IsocEn%c"
              " LSEn%c ExtCTL%c 64b%c\n";
    } else {
        fmt = "\t\tLink Control: CFlE%c CST%c CFE%c <LkFail%c Init%c EOC%c TXO%c <CRCErr=%x\n";
    }
    printf(fmt,
           PCI_FLAG(lctr, PCI_HT_LCTR_CFLE),
           PCI_FLAG(lctr, PCI_HT_LCTR_CST),
           PCI_FLAG(lctr, PCI_HT_LCTR_CFE),
           PCI_FLAG(lctr, PCI_HT_LCTR_LKFAIL),
           PCI_FLAG(lctr, PCI_HT_LCTR_INIT),
           PCI_FLAG(lctr, PCI_HT_LCTR_EOC),
           PCI_FLAG(lctr, PCI_HT_LCTR_TXO),
           (lctr & PCI_HT_LCTR_CRCERR) >> 8,
           PCI_FLAG(lctr, PCI_HT_LCTR_ISOCEN),
           PCI_FLAG(lctr, PCI_HT_LCTR_LSEN),
           PCI_FLAG(lctr, PCI_HT_LCTR_EXTCTL),
           PCI_FLAG(lctr, PCI_HT_LCTR_64B));
    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_HT_SEC_LCNF), &lcnf);
    if (rid >= 0x22) {
        fmt = "\t\tLink Config: MLWI=%1$s DwFcIn%5$c MLWO=%2$s DwFcOut%6$c LWI=%3$s DwFcInEn%7$c"
              " LWO=%4$s DwFcOutEn%8$c\n";
    } else {
        fmt = "\t\tLink Config: MLWI=%s MLWO=%s LWI=%s LWO=%s\n";
    }
    printf(fmt,
           __pciCapHtLinkWidth(lcnf & PCI_HT_LCNF_MLWI),
           __pciCapHtLinkWidth((lcnf & PCI_HT_LCNF_MLWO) >> 4),
           __pciCapHtLinkWidth((lcnf & PCI_HT_LCNF_LWI) >> 8),
           __pciCapHtLinkWidth((lcnf & PCI_HT_LCNF_LWO) >> 12),
           PCI_FLAG(lcnf, PCI_HT_LCNF_DFI),
           PCI_FLAG(lcnf, PCI_HT_LCNF_DFO),
           PCI_FLAG(lcnf, PCI_HT_LCNF_DFIE),
           PCI_FLAG(lcnf, PCI_HT_LCNF_DFOE));
    printf("\t\tRevision ID: %u.%02u\n",
           (rid & PCI_HT_RID_MAJ) >> 5, (rid & PCI_HT_RID_MIN));
    if (rid < 0x22) {
        return;
    }
    API_PciConfigInByte(iBus, iSlot, iFunc, (iWhere + PCI_HT_SEC_LFRER), &lfrer);
    printf("\t\tLink Frequency: %s\n", __pciCapHtLinkFreq(lfrer & PCI_HT_LFRER_FREQ));
    printf("\t\tLink Error: <Prot%c <Ovfl%c <EOC%c CTLTm%c\n",
           PCI_FLAG(lfrer, PCI_HT_LFRER_PROT),
           PCI_FLAG(lfrer, PCI_HT_LFRER_OV),
           PCI_FLAG(lfrer, PCI_HT_LFRER_EOC),
           PCI_FLAG(lfrer, PCI_HT_LFRER_CTLT));
    API_PciConfigInByte(iBus, iSlot, iFunc, (iWhere + PCI_HT_SEC_LFCAP), &lfcap);
    printf("\t\tLink Frequency Capability: 200MHz%c 300MHz%c 400MHz%c 500MHz%c 600MHz%c 800MHz%c"
           " 1.0GHz%c 1.2GHz%c 1.4GHz%c 1.6GHz%c Vend%c\n",
           PCI_FLAG(lfcap, PCI_HT_LFCAP_200),
           PCI_FLAG(lfcap, PCI_HT_LFCAP_300),
           PCI_FLAG(lfcap, PCI_HT_LFCAP_400),
           PCI_FLAG(lfcap, PCI_HT_LFCAP_500),
           PCI_FLAG(lfcap, PCI_HT_LFCAP_600),
           PCI_FLAG(lfcap, PCI_HT_LFCAP_800),
           PCI_FLAG(lfcap, PCI_HT_LFCAP_1000),
           PCI_FLAG(lfcap, PCI_HT_LFCAP_1200),
           PCI_FLAG(lfcap, PCI_HT_LFCAP_1400),
           PCI_FLAG(lfcap, PCI_HT_LFCAP_1600),
           PCI_FLAG(lfcap, PCI_HT_LFCAP_VEND));
    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_HT_SEC_FTR), &ftr);
    printf("\t\tFeature Capability: IsocFC%c LDTSTOP%c CRCTM%c ECTLT%c 64bA%c UIDRD%c ExtRS%c UCnfE%c\n",
           PCI_FLAG(ftr, PCI_HT_FTR_ISOCFC),
           PCI_FLAG(ftr, PCI_HT_FTR_LDTSTOP),
           PCI_FLAG(ftr, PCI_HT_FTR_CRCTM),
           PCI_FLAG(ftr, PCI_HT_FTR_ECTLT),
           PCI_FLAG(ftr, PCI_HT_FTR_64BA),
           PCI_FLAG(ftr, PCI_HT_FTR_UIDRD),
           PCI_FLAG(ftr, PCI_HT_SEC_FTR_EXTRS),
           PCI_FLAG(ftr, PCI_HT_SEC_FTR_UCNFE));
    if (ftr & PCI_HT_SEC_FTR_EXTRS) {
        API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_HT_SEC_EH), &eh);
        printf("\t\tError Handling: PFlE%c OFlE%c PFE%c OFE%c EOCFE%c RFE%c CRCFE%c SERRFE%c CF%c"
               " RE%c PNFE%c ONFE%c EOCNFE%c RNFE%c CRCNFE%c SERRNFE%c\n",
               PCI_FLAG(eh, PCI_HT_EH_PFLE),
               PCI_FLAG(eh, PCI_HT_EH_OFLE),
         PCI_FLAG(eh, PCI_HT_EH_PFE),
         PCI_FLAG(eh, PCI_HT_EH_OFE),
         PCI_FLAG(eh, PCI_HT_EH_EOCFE),
         PCI_FLAG(eh, PCI_HT_EH_RFE),
         PCI_FLAG(eh, PCI_HT_EH_CRCFE),
         PCI_FLAG(eh, PCI_HT_EH_SERRFE),
         PCI_FLAG(eh, PCI_HT_EH_CF),
         PCI_FLAG(eh, PCI_HT_EH_RE),
         PCI_FLAG(eh, PCI_HT_EH_PNFE),
         PCI_FLAG(eh, PCI_HT_EH_ONFE),
         PCI_FLAG(eh, PCI_HT_EH_EOCNFE),
         PCI_FLAG(eh, PCI_HT_EH_RNFE),
         PCI_FLAG(eh, PCI_HT_EH_CRCNFE),
         PCI_FLAG(eh, PCI_HT_EH_SERRNFE));
        API_PciConfigInByte(iBus, iSlot, iFunc, (iWhere + PCI_HT_SEC_MBU), &mbu);
        API_PciConfigInByte(iBus, iSlot, iFunc, (iWhere + PCI_HT_SEC_MLU), &mlu);
        printf("\t\tPrefetchable memory behind bridge Upper: %02x-%02x\n", mbu, mlu);
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapHtPriShow
** 功能描述: 打印设备的扩展能力 HT PRI 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
**           iCmd       命令信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapHtPriShow (INT iBus, INT iSlot, INT iFunc, INT iWhere, INT iCmd)
{
    UINT16      lctr0, lcnf0, lctr1, lcnf1, eh;
    UINT8       rid, lfrer0, lfcap0, ftr, lfrer1, lfcap1, mbu, mlu, bn;
    CHAR       *fmt;

    printf("HyperTransport: Slave or Primary Interface\n");

    if (_G_iPciVerbose < 2) {
        return;
    }
    if (!API_PciConfigFetch(iBus, iSlot, iFunc,
                            (iWhere + PCI_HT_PRI_LCTR0), (PCI_HT_PRI_SIZEOF - PCI_HT_PRI_LCTR0))) {
        return;
    }

    API_PciConfigInByte(iBus, iSlot, iFunc, (iWhere + PCI_HT_PRI_RID), &rid);
    if (rid < 0x22 && rid > 0x11) {
        printf("\t\t!!! Possibly incomplete decoding\n");
    }
    if (rid >= 0x22) {
        fmt = "\t\tCommand: BaseUnitID=%u UnitCnt=%u MastHost%c DefDir%c DUL%c\n";
    } else {
        fmt = "\t\tCommand: BaseUnitID=%u UnitCnt=%u MastHost%c DefDir%c\n";
    }
    printf(fmt,
           (iCmd & PCI_HT_PRI_CMD_BUID),
           (iCmd & PCI_HT_PRI_CMD_UC) >> 5,
           PCI_FLAG(iCmd, PCI_HT_PRI_CMD_MH),
           PCI_FLAG(iCmd, PCI_HT_PRI_CMD_DD),
           PCI_FLAG(iCmd, PCI_HT_PRI_CMD_DUL));
    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_HT_PRI_LCTR0), &lctr0);
    if (rid >= 0x22) {
        fmt = "\t\tLink Control 0: CFlE%c CST%c CFE%c <LkFail%c Init%c EOC%c TXO%c <CRCErr=%x IsocEn%c"
              " LSEn%c ExtCTL%c 64b%c\n";
    } else {
        fmt = "\t\tLink Control 0: CFlE%c CST%c CFE%c <LkFail%c Init%c EOC%c TXO%c <CRCErr=%x\n";
    }
    printf(fmt,
           PCI_FLAG(lctr0, PCI_HT_LCTR_CFLE),
           PCI_FLAG(lctr0, PCI_HT_LCTR_CST),
           PCI_FLAG(lctr0, PCI_HT_LCTR_CFE),
           PCI_FLAG(lctr0, PCI_HT_LCTR_LKFAIL),
           PCI_FLAG(lctr0, PCI_HT_LCTR_INIT),
           PCI_FLAG(lctr0, PCI_HT_LCTR_EOC),
           PCI_FLAG(lctr0, PCI_HT_LCTR_TXO),
           (lctr0 & PCI_HT_LCTR_CRCERR) >> 8,
           PCI_FLAG(lctr0, PCI_HT_LCTR_ISOCEN),
           PCI_FLAG(lctr0, PCI_HT_LCTR_LSEN),
           PCI_FLAG(lctr0, PCI_HT_LCTR_EXTCTL),
           PCI_FLAG(lctr0, PCI_HT_LCTR_64B));
    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_HT_PRI_LCNF0), &lcnf0);
    if (rid >= 0x22) {
        fmt = "\t\tLink Config 0: MLWI=%1$s DwFcIn%5$c MLWO=%2$s DwFcOut%6$c LWI=%3$s DwFcInEn%7$c"
              " LWO=%4$s DwFcOutEn%8$c\n";
    } else {
        fmt = "\t\tLink Config 0: MLWI=%s MLWO=%s LWI=%s LWO=%s\n";
    }
    printf(fmt,
           __pciCapHtLinkWidth(lcnf0 & PCI_HT_LCNF_MLWI),
           __pciCapHtLinkWidth((lcnf0 & PCI_HT_LCNF_MLWO) >> 4),
           __pciCapHtLinkWidth((lcnf0 & PCI_HT_LCNF_LWI) >> 8),
           __pciCapHtLinkWidth((lcnf0 & PCI_HT_LCNF_LWO) >> 12),
           PCI_FLAG(lcnf0, PCI_HT_LCNF_DFI),
           PCI_FLAG(lcnf0, PCI_HT_LCNF_DFO),
           PCI_FLAG(lcnf0, PCI_HT_LCNF_DFIE),
           PCI_FLAG(lcnf0, PCI_HT_LCNF_DFOE));
    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_HT_PRI_LCTR1), &lctr1);
    if (rid >= 0x22) {
        fmt = "\t\tLink Control 1: CFlE%c CST%c CFE%c <LkFail%c Init%c EOC%c TXO%c <CRCErr=%x IsocEn%c"
              " LSEn%c ExtCTL%c 64b%c\n";
    } else {
        fmt = "\t\tLink Control 1: CFlE%c CST%c CFE%c <LkFail%c Init%c EOC%c TXO%c <CRCErr=%x\n";
    }
    printf(fmt,
           PCI_FLAG(lctr1, PCI_HT_LCTR_CFLE),
           PCI_FLAG(lctr1, PCI_HT_LCTR_CST),
           PCI_FLAG(lctr1, PCI_HT_LCTR_CFE),
           PCI_FLAG(lctr1, PCI_HT_LCTR_LKFAIL),
           PCI_FLAG(lctr1, PCI_HT_LCTR_INIT),
           PCI_FLAG(lctr1, PCI_HT_LCTR_EOC),
           PCI_FLAG(lctr1, PCI_HT_LCTR_TXO),
           (lctr1 & PCI_HT_LCTR_CRCERR) >> 8,
           PCI_FLAG(lctr1, PCI_HT_LCTR_ISOCEN),
           PCI_FLAG(lctr1, PCI_HT_LCTR_LSEN),
           PCI_FLAG(lctr1, PCI_HT_LCTR_EXTCTL),
           PCI_FLAG(lctr1, PCI_HT_LCTR_64B));
    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_HT_PRI_LCNF1), &lcnf1);
    if (rid >= 0x22) {
        fmt = "\t\tLink Config 1: MLWI=%1$s DwFcIn%5$c MLWO=%2$s DwFcOut%6$c LWI=%3$s DwFcInEn%7$c"
              " LWO=%4$s DwFcOutEn%8$c\n";
    } else {
        fmt = "\t\tLink Config 1: MLWI=%s MLWO=%s LWI=%s LWO=%s\n";
    }
    printf(fmt,
           __pciCapHtLinkWidth(lcnf1 & PCI_HT_LCNF_MLWI),
           __pciCapHtLinkWidth((lcnf1 & PCI_HT_LCNF_MLWO) >> 4),
           __pciCapHtLinkWidth((lcnf1 & PCI_HT_LCNF_LWI) >> 8),
           __pciCapHtLinkWidth((lcnf1 & PCI_HT_LCNF_LWO) >> 12),
           PCI_FLAG(lcnf1, PCI_HT_LCNF_DFI),
           PCI_FLAG(lcnf1, PCI_HT_LCNF_DFO),
           PCI_FLAG(lcnf1, PCI_HT_LCNF_DFIE),
           PCI_FLAG(lcnf1, PCI_HT_LCNF_DFOE));
    printf("\t\tRevision ID: %u.%02u\n",
           (rid & PCI_HT_RID_MAJ) >> 5, (rid & PCI_HT_RID_MIN));
    if (rid < 0x22) {
        return;
    }
    API_PciConfigInByte(iBus, iSlot, iFunc, (iWhere + PCI_HT_PRI_LFRER0), &lfrer0);
    printf("\t\tLink Frequency 0: %s\n", __pciCapHtLinkFreq(lfrer0 & PCI_HT_LFRER_FREQ));
    printf("\t\tLink Error 0: <Prot%c <Ovfl%c <EOC%c CTLTm%c\n",
           PCI_FLAG(lfrer0, PCI_HT_LFRER_PROT),
           PCI_FLAG(lfrer0, PCI_HT_LFRER_OV),
           PCI_FLAG(lfrer0, PCI_HT_LFRER_EOC),
           PCI_FLAG(lfrer0, PCI_HT_LFRER_CTLT));
    API_PciConfigInByte(iBus, iSlot, iFunc, (iWhere + PCI_HT_PRI_LFCAP0), &lfcap0);
    printf("\t\tLink Frequency Capability 0: 200MHz%c 300MHz%c 400MHz%c 500MHz%c 600MHz%c 800MHz%c"
           " 1.0GHz%c 1.2GHz%c 1.4GHz%c 1.6GHz%c Vend%c\n",
           PCI_FLAG(lfcap0, PCI_HT_LFCAP_200),
           PCI_FLAG(lfcap0, PCI_HT_LFCAP_300),
           PCI_FLAG(lfcap0, PCI_HT_LFCAP_400),
           PCI_FLAG(lfcap0, PCI_HT_LFCAP_500),
           PCI_FLAG(lfcap0, PCI_HT_LFCAP_600),
           PCI_FLAG(lfcap0, PCI_HT_LFCAP_800),
           PCI_FLAG(lfcap0, PCI_HT_LFCAP_1000),
           PCI_FLAG(lfcap0, PCI_HT_LFCAP_1200),
           PCI_FLAG(lfcap0, PCI_HT_LFCAP_1400),
           PCI_FLAG(lfcap0, PCI_HT_LFCAP_1600),
           PCI_FLAG(lfcap0, PCI_HT_LFCAP_VEND));
    API_PciConfigInByte(iBus, iSlot, iFunc, (iWhere + PCI_HT_PRI_FTR), &ftr);
    printf("\t\tFeature Capability: IsocFC%c LDTSTOP%c CRCTM%c ECTLT%c 64bA%c UIDRD%c\n",
           PCI_FLAG(ftr, PCI_HT_FTR_ISOCFC),
           PCI_FLAG(ftr, PCI_HT_FTR_LDTSTOP),
           PCI_FLAG(ftr, PCI_HT_FTR_CRCTM),
           PCI_FLAG(ftr, PCI_HT_FTR_ECTLT),
           PCI_FLAG(ftr, PCI_HT_FTR_64BA),
           PCI_FLAG(ftr, PCI_HT_FTR_UIDRD));
    API_PciConfigInByte(iBus, iSlot, iFunc, (iWhere + PCI_HT_PRI_LFRER1), &lfrer1);
    printf("\t\tLink Frequency 1: %s\n", __pciCapHtLinkFreq(lfrer1 & PCI_HT_LFRER_FREQ));
    printf("\t\tLink Error 1: <Prot%c <Ovfl%c <EOC%c CTLTm%c\n",
           PCI_FLAG(lfrer1, PCI_HT_LFRER_PROT),
           PCI_FLAG(lfrer1, PCI_HT_LFRER_OV),
           PCI_FLAG(lfrer1, PCI_HT_LFRER_EOC),
           PCI_FLAG(lfrer1, PCI_HT_LFRER_CTLT));
    API_PciConfigInByte(iBus, iSlot, iFunc, (iWhere + PCI_HT_PRI_LFCAP1), &lfcap1);
    printf("\t\tLink Frequency Capability 1: 200MHz%c 300MHz%c 400MHz%c 500MHz%c 600MHz%c 800MHz%c"
           " 1.0GHz%c 1.2GHz%c 1.4GHz%c 1.6GHz%c Vend%c\n",
           PCI_FLAG(lfcap1, PCI_HT_LFCAP_200),
           PCI_FLAG(lfcap1, PCI_HT_LFCAP_300),
           PCI_FLAG(lfcap1, PCI_HT_LFCAP_400),
           PCI_FLAG(lfcap1, PCI_HT_LFCAP_500),
           PCI_FLAG(lfcap1, PCI_HT_LFCAP_600),
           PCI_FLAG(lfcap1, PCI_HT_LFCAP_800),
           PCI_FLAG(lfcap1, PCI_HT_LFCAP_1000),
           PCI_FLAG(lfcap1, PCI_HT_LFCAP_1200),
           PCI_FLAG(lfcap1, PCI_HT_LFCAP_1400),
           PCI_FLAG(lfcap1, PCI_HT_LFCAP_1600),
           PCI_FLAG(lfcap1, PCI_HT_LFCAP_VEND));
    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_HT_PRI_EH), &eh);
    printf("\t\tError Handling: PFlE%c OFlE%c PFE%c OFE%c EOCFE%c RFE%c CRCFE%c SERRFE%c CF%c RE%c"
           " PNFE%c ONFE%c EOCNFE%c RNFE%c CRCNFE%c SERRNFE%c\n",
           PCI_FLAG(eh, PCI_HT_EH_PFLE),
           PCI_FLAG(eh, PCI_HT_EH_OFLE),
           PCI_FLAG(eh, PCI_HT_EH_PFE),
           PCI_FLAG(eh, PCI_HT_EH_OFE),
           PCI_FLAG(eh, PCI_HT_EH_EOCFE),
           PCI_FLAG(eh, PCI_HT_EH_RFE),
           PCI_FLAG(eh, PCI_HT_EH_CRCFE),
           PCI_FLAG(eh, PCI_HT_EH_SERRFE),
           PCI_FLAG(eh, PCI_HT_EH_CF),
           PCI_FLAG(eh, PCI_HT_EH_RE),
           PCI_FLAG(eh, PCI_HT_EH_PNFE),
           PCI_FLAG(eh, PCI_HT_EH_ONFE),
           PCI_FLAG(eh, PCI_HT_EH_EOCNFE),
           PCI_FLAG(eh, PCI_HT_EH_RNFE),
           PCI_FLAG(eh, PCI_HT_EH_CRCNFE),
           PCI_FLAG(eh, PCI_HT_EH_SERRNFE));
    API_PciConfigInByte(iBus, iSlot, iFunc, (iWhere + PCI_HT_PRI_MBU), &mbu);
    API_PciConfigInByte(iBus, iSlot, iFunc, (iWhere + PCI_HT_PRI_MLU), &mlu);
    printf("\t\tPrefetchable memory behind bridge Upper: %02x-%02x\n", mbu, mlu);
    API_PciConfigInByte(iBus, iSlot, iFunc, (iWhere + PCI_HT_PRI_BN), &bn);
    printf("\t\tBus Number: %02x\n", bn);
}
/*********************************************************************************************************
** 函数名称: __pciCapHtShow
** 功能描述: 打印设备的扩展能力 HT 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
**           iCmd       命令信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapHtShow (INT iBus, INT iSlot, INT iFunc, INT iWhere, INT iCmd)
{
    INT     iType;

    switch (iCmd & PCI_HT_CMD_TYP_HI) {
    
    case PCI_HT_CMD_TYP_HI_PRI:
        __pciCapHtPriShow(iBus, iSlot, iFunc, iWhere, iCmd);
        return;

    case PCI_HT_CMD_TYP_HI_SEC:
        __pciCapHtSecShow(iBus, iSlot, iFunc, iWhere, iCmd);
        return;
    }

    iType = iCmd & PCI_HT_CMD_TYP;
    switch (iType) {
    
    case PCI_HT_CMD_TYP_SW:
        printf("HyperTransport: Switch\n");
        break;

    case PCI_HT_CMD_TYP_IDC:
        printf("HyperTransport: Interrupt Discovery and Configuration\n");
        break;

    case PCI_HT_CMD_TYP_RID:
        printf("HyperTransport: Revision ID: %u.%02u\n",
               (iCmd & PCI_HT_RID_MAJ) >> 5, (iCmd & PCI_HT_RID_MIN));
        break;

    case PCI_HT_CMD_TYP_UIDC:
        printf("HyperTransport: UnitID Clumping\n");
        break;

    case PCI_HT_CMD_TYP_ECSA:
        printf("HyperTransport: Extended Configuration Space Access\n");
        break;

    case PCI_HT_CMD_TYP_AM:
        printf("HyperTransport: Address Mapping\n");
        break;

    case PCI_HT_CMD_TYP_MSIM:
        printf("HyperTransport: MSI Mapping Enable%c Fixed%c\n",
               PCI_FLAG(iCmd, PCI_HT_MSIM_CMD_EN),
               PCI_FLAG(iCmd, PCI_HT_MSIM_CMD_FIXD));
        if ((_G_iPciVerbose >= 2) &&
            (!(iCmd & PCI_HT_MSIM_CMD_FIXD))) {
            UINT32      offl, offh;

            if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + PCI_HT_MSIM_ADDR_LO), 8)) {
                break;
            }

            API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_HT_MSIM_ADDR_LO), &offl);
            API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_HT_MSIM_ADDR_HI), &offh);
            printf("\t\tMapping Address Base: %016llx\n", ((UINT64)offh << 32) | (offl & ~0xfffff));
        }
        break;

    case PCI_HT_CMD_TYP_DR:
        printf("HyperTransport: DirectRoute\n");
        break;

    case PCI_HT_CMD_TYP_VCS:
        printf("HyperTransport: VCSet\n");
        break;

    case PCI_HT_CMD_TYP_RM:
        printf("HyperTransport: Retry Mode\n");
        break;

    case PCI_HT_CMD_TYP_X86:
        printf("HyperTransport: X86 (reserved)\n");
        break;

    default:
        printf("HyperTransport: #%02x\n", iType >> 11);
        break;
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapPcixBridgeShow
** 功能描述: 打印设备的扩展能力 PCIX (Bridge)信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapPcixBridgeShow (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    static const CHAR * const pcSecClockFreq[8] = {"conv", "66MHz", "100MHz", "133MHz",
                                                   "?4", "?5", "?6", "?7" };
    UINT16      usSecStatus;
    UINT32      uiStatus, uiUpStcr, uiDownStcr;

    printf("PCI-X bridge device\n");

    if (_G_iPciVerbose < 2) {
        return;
    }
    if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + PCI_PCIX_BRIDGE_STATUS), 12)) {
        return;
    }

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_PCIX_BRIDGE_SEC_STATUS), &usSecStatus);
    printf("\t\tSecondary uiStatus: 64bit%c 133MHz%c SCD%c USC%c SCO%c SRD%c Freq=%s\n",
           PCI_FLAG(usSecStatus, PCI_PCIX_BRIDGE_SEC_STATUS_64BIT),
           PCI_FLAG(usSecStatus, PCI_PCIX_BRIDGE_SEC_STATUS_133MHZ),
           PCI_FLAG(usSecStatus, PCI_PCIX_BRIDGE_SEC_STATUS_SC_DISCARDED),
           PCI_FLAG(usSecStatus, PCI_PCIX_BRIDGE_SEC_STATUS_UNEXPECTED_SC),
           PCI_FLAG(usSecStatus, PCI_PCIX_BRIDGE_SEC_STATUS_SC_OVERRUN),
           PCI_FLAG(usSecStatus, PCI_PCIX_BRIDGE_SEC_STATUS_SPLIT_REQUEST_DELAYED),
           pcSecClockFreq[(usSecStatus >> 6) & 7]);
    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_PCIX_BRIDGE_STATUS), &uiStatus);
    printf("\t\tStatus: Dev=%02x:%02x.%d 64bit%c 133MHz%c SCD%c USC%c SCO%c SRD%c\n",
           ((uiStatus >> 8) & 0xff),
           ((uiStatus >> 3) & 0x1f),
           (uiStatus & PCI_PCIX_BRIDGE_STATUS_FUNCTION),
           PCI_FLAG(uiStatus, PCI_PCIX_BRIDGE_STATUS_64BIT),
           PCI_FLAG(uiStatus, PCI_PCIX_BRIDGE_STATUS_133MHZ),
           PCI_FLAG(uiStatus, PCI_PCIX_BRIDGE_STATUS_SC_DISCARDED),
           PCI_FLAG(uiStatus, PCI_PCIX_BRIDGE_STATUS_UNEXPECTED_SC),
           PCI_FLAG(uiStatus, PCI_PCIX_BRIDGE_STATUS_SC_OVERRUN),
           PCI_FLAG(uiStatus, PCI_PCIX_BRIDGE_STATUS_SPLIT_REQUEST_DELAYED));
    API_PciConfigInDword(iBus, iSlot, iFunc,
                         (iWhere + PCI_PCIX_BRIDGE_UPSTREAM_SPLIT_TRANS_CTRL), &uiUpStcr);
    printf("\t\tUpstream: Capacity=%u CommitmentLimit=%u\n",
           (uiUpStcr & PCI_PCIX_BRIDGE_STR_CAPACITY),
           (uiUpStcr >> 16) & 0xffff);
    API_PciConfigInDword(iBus, iSlot, iFunc,
                         (iWhere + PCI_PCIX_BRIDGE_DOWNSTREAM_SPLIT_TRANS_CTRL), &uiDownStcr);
    printf("\t\tDownstream: Capacity=%u CommitmentLimit=%u\n",
           (uiDownStcr & PCI_PCIX_BRIDGE_STR_CAPACITY),
           (uiDownStcr >> 16) & 0xffff);
}
/*********************************************************************************************************
** 函数名称: __pciCapPcixNoBridgeShow
** 功能描述: 打印设备的扩展能力 PCIX (No Bridge)信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapPcixNoBridgeShow (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    UINT16                  usCommand;
    UINT32                  uiStatus;
    static const UINT8      ucOutstandingMax[8] = { 1, 2, 3, 4, 8, 12, 16, 32 };

    printf("PCI-X non-bridge device\n");

    if (_G_iPciVerbose < 2) {
        return;
    }
    if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + PCI_PCIX_STATUS), 4)) {
        return;
    }

    API_PciConfigInWord (iBus, iSlot, iFunc, (iWhere + PCI_PCIX_COMMAND), &usCommand);
    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_PCIX_STATUS ), &uiStatus );
    printf("\t\tCommand: DPERE%c ERO%c RBC=%d OST=%d\n",
           PCI_FLAG(usCommand, PCI_PCIX_COMMAND_DPERE),
           PCI_FLAG(usCommand, PCI_PCIX_COMMAND_ERO),
           1 << (9 + ((usCommand & PCI_PCIX_COMMAND_MAX_MEM_READ_BYTE_COUNT) >> 2U)),
           ucOutstandingMax[(usCommand & PCI_PCIX_COMMAND_MAX_OUTSTANDING_SPLIT_TRANS) >> 4U]);
    printf("\t\tStatus: Dev=%02x:%02x.%d 64bit%c 133MHz%c SCD%c USC%c DC=%s DMMRBC=%u DMOST=%u"
           " DMCRS=%u RSCEM%c 266MHz%c 533MHz%c\n",
           ((uiStatus >> 8) & 0xff),
           ((uiStatus >> 3) & 0x1f),
           (uiStatus & PCI_PCIX_STATUS_FUNCTION),
           PCI_FLAG(uiStatus, PCI_PCIX_STATUS_64BIT),
           PCI_FLAG(uiStatus, PCI_PCIX_STATUS_133MHZ),
           PCI_FLAG(uiStatus, PCI_PCIX_STATUS_SC_DISCARDED),
           PCI_FLAG(uiStatus, PCI_PCIX_STATUS_UNEXPECTED_SC),
           ((uiStatus & PCI_PCIX_STATUS_DEVICE_COMPLEXITY) ? "bridge" : "simple"),
           1 << (9 + ((uiStatus >> 21) & 3U)),
           ucOutstandingMax[(uiStatus >> 23) & 7U],
           1 << (3 + ((uiStatus >> 26) & 7U)),
           PCI_FLAG(uiStatus, PCI_PCIX_STATUS_RCVD_SC_ERR_MESS),
           PCI_FLAG(uiStatus, PCI_PCIX_STATUS_266MHZ),
           PCI_FLAG(uiStatus, PCI_PCIX_STATUS_533MHZ));
}
/*********************************************************************************************************
** 函数名称: __pciCapMsiShow
** 功能描述: 打印设备的扩展能力 PCIX 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapPcixShow (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    UINT8       ucType;

    API_PciHeaderTypeGet(iBus, iSlot, iFunc, &ucType);

    switch (ucType) {
    
    case PCI_HEADER_TYPE_NORMAL:
        __pciCapPcixNoBridgeShow(iBus, iSlot, iFunc, iWhere);
        break;

    case PCI_HEADER_TYPE_BRIDGE:
        __pciCapPcixBridgeShow(iBus, iSlot, iFunc, iWhere);
        break;

    default:
        break;
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapMsiShow
** 功能描述: 打印设备的扩展能力 MSI 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
**           iCap       扩展信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapMsiShow (INT iBus, INT iSlot, INT iFunc, INT iWhere, INT iCap)
{
    INT         iIs64;
    UINT32      t;
    UINT16      w;

    printf("MSI: Enable%c Count=%d/%d Maskable%c 64bit%c\n",
           PCI_FLAG(iCap, PCI_MSI_FLAGS_ENABLE),
           1 << ((iCap & PCI_MSI_FLAGS_QSIZE) >> 4),
           1 << ((iCap & PCI_MSI_FLAGS_QMASK) >> 1),
           PCI_FLAG(iCap, PCI_MSI_FLAGS_MASK_BIT),
           PCI_FLAG(iCap, PCI_MSI_FLAGS_64BIT));

    if (_G_iPciVerbose < 2) {
        return;
    }
    iIs64 = iCap & PCI_MSI_FLAGS_64BIT;
    if (!API_PciConfigFetch(iBus, iSlot, iFunc,
                            (iWhere + PCI_MSI_ADDRESS_LO),
                            (iIs64 ? PCI_MSI_DATA_64 : PCI_MSI_DATA_32) + 2 - PCI_MSI_ADDRESS_LO)) {
        return;
    }
    printf("\t\tAddress: ");
    if (iIs64) {
        API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_MSI_ADDRESS_HI), &t);
        API_PciConfigInWord (iBus, iSlot, iFunc, (iWhere + PCI_MSI_DATA_64   ), &w);
        printf("%08x", t);
    } else {
        API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_MSI_DATA_32), &w);
    }
    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_MSI_ADDRESS_LO), &t);
    printf("%08x  Data: %04x\n", t, w);
    if (iCap & PCI_MSI_FLAGS_MASK_BIT) {
      UINT32 mask, pending;

      if (iIs64) {
          if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + PCI_MSI_MASK_BIT_64), 8)) {
              return;
          }
          API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_MSI_MASK_BIT_64), &mask   );
          API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_MSI_PENDING_64 ), &pending);
      } else {
          if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + PCI_MSI_MASK_BIT_32), 8)) {
              return;
          }
          API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_MSI_MASK_BIT_32), &mask   );
          API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_MSI_PENDING_32 ), &pending);
      }
      printf("\t\tMasking: %08x  Pending: %08x\n", mask, pending);
    }
}

/*********************************************************************************************************
** 函数名称: __pciCapSlotIdShow
** 功能描述: 打印设备的扩展能力 SLOTID.
** 输　入  : iCap       扩展信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapSlotIdShow (INT iCap)
{
    INT     iEsr = iCap & 0xff;
    INT     iChs = iCap >> 8;

    printf("Slot ID: %d slots, First%c, chassis %02x\n",
           iEsr & PCI_SID_ESR_NSLOTS,
           PCI_FLAG(iEsr, PCI_SID_ESR_FIC),
           iChs);
}
/*********************************************************************************************************
** 函数名称: __pciCapAgpFormatRate
** 功能描述: 设备的扩展能力 AGP 速率格式化.
** 输　入  : iRate      速率
**           pcBuf      信息缓冲区
**           iAgp3      是否使能 AGP3
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapAgpFormatRate (INT iRate, PCHAR pcBuf, INT iAgp3)
{
    PCHAR       c = pcBuf;
    INT         i = 0;

    for (i = 0; i <= 2; i++) {
        if (iRate & (1 << i)) {
            if (c != pcBuf) {
                *c++ = ',';
            }
            c += sprintf(c, "x%d", 1 << (i + (2 * iAgp3)));
        }
    }
    if (c != pcBuf) {
        *c = 0;
    } else {
        lib_strcpy(pcBuf, "<none>");
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapAgpShow
** 功能描述: 打印设备的扩展能力 AGP 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
**           iCap       扩展信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapAgpShow (INT iBus, INT iSlot, INT iFunc, INT iWhere, INT iCap)
{
    UINT32      t;
    CHAR        cRate[16];
    INT         iVer, iRev;
    INT         iAgp3 = 0;

    iVer = (iCap >> 4) & 0x0f;
    iRev = iCap & 0x0f;
    printf("AGP version %x.%x\n", iVer, iRev);

    if (_G_iPciVerbose < 2) {
        return;
    }
    if (!API_PciConfigFetch(iBus, iSlot, iFunc,
                            (iWhere + PCI_AGP_STATUS), PCI_AGP_SIZEOF - PCI_AGP_STATUS)) {
        return;
    }

    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_AGP_STATUS), &t);
    if (iVer >= 3 && (t & PCI_AGP_STATUS_AGP3)) {
        iAgp3 = 1;
    }
    __pciCapAgpFormatRate(t & 7, cRate, iAgp3);
    printf("\t\tStatus: RQ=%d Iso%c ArqSz=%d Cal=%d SBA%c ITACoh%c GART64%c HTrans%c 64bit%c FW%c"
           " AGP3%c Rate=%s\n",
           ((t & PCI_AGP_STATUS_RQ_MASK) >> 24U) + 1,
           PCI_FLAG(t, PCI_AGP_STATUS_ISOCH),
           ((t & PCI_AGP_STATUS_ARQSZ_MASK) >> 13),
           ((t & PCI_AGP_STATUS_CAL_MASK) >> 10),
           PCI_FLAG(t, PCI_AGP_STATUS_SBA),
           PCI_FLAG(t, PCI_AGP_STATUS_ITA_COH),
           PCI_FLAG(t, PCI_AGP_STATUS_GART64),
           PCI_FLAG(t, PCI_AGP_STATUS_HTRANS),
           PCI_FLAG(t, PCI_AGP_STATUS_64BIT),
           PCI_FLAG(t, PCI_AGP_STATUS_FW),
           PCI_FLAG(t, PCI_AGP_STATUS_AGP3),
           cRate);
    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_AGP_COMMAND), &t);
    __pciCapAgpFormatRate(t & 7, cRate, iAgp3);
    printf("\t\tCommand: RQ=%d ArqSz=%d Cal=%d SBA%c AGP%c GART64%c 64bit%c FW%c Rate=%s\n",
           ((t & PCI_AGP_COMMAND_RQ_MASK) >> 24U) + 1,
           ((t & PCI_AGP_COMMAND_ARQSZ_MASK) >> 13),
           ((t & PCI_AGP_COMMAND_CAL_MASK) >> 10),
           PCI_FLAG(t, PCI_AGP_COMMAND_SBA),
           PCI_FLAG(t, PCI_AGP_COMMAND_AGP),
           PCI_FLAG(t, PCI_AGP_COMMAND_GART64),
           PCI_FLAG(t, PCI_AGP_COMMAND_64BIT),
           PCI_FLAG(t, PCI_AGP_COMMAND_FW),
           cRate);
}
/*********************************************************************************************************
** 函数名称: __pciCapPmShow
** 功能描述: 打印设备的扩展能力 PM 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
**           iCap       扩展信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapPmShow (INT iBus, INT iSlot, INT iFunc, INT iWhere, INT iCap)
{
    INT             t;
    INT             b;
    static INT      iPmAuxCurrent[8] = { 0, 55, 100, 160, 220, 270, 320, 375 };

    printf("Power Management version %d\n", iCap & PCI_PM_CAP_VER_MASK);

    if (_G_iPciVerbose < 2) {
        return;
    }

    printf("\t\tFlags: PMEClk%c DSI%c D1%c D2%c AuxCurrent=%dmA PME(D0%c,D1%c,D2%c,D3hot%c,D3cold%c)\n",
           PCI_FLAG(iCap, PCI_PM_CAP_PME_CLOCK),
           PCI_FLAG(iCap, PCI_PM_CAP_DSI),
           PCI_FLAG(iCap, PCI_PM_CAP_D1),
           PCI_FLAG(iCap, PCI_PM_CAP_D2),
           iPmAuxCurrent[(iCap >> 6) & 7],
           PCI_FLAG(iCap, PCI_PM_CAP_PME_D0),
           PCI_FLAG(iCap, PCI_PM_CAP_PME_D1),
           PCI_FLAG(iCap, PCI_PM_CAP_PME_D2),
           PCI_FLAG(iCap, PCI_PM_CAP_PME_D3_HOT),
           PCI_FLAG(iCap, PCI_PM_CAP_PME_D3_COLD));
    if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + PCI_PM_CTRL), PCI_PM_SIZEOF - PCI_PM_CTRL)) {
        return;
    }
    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_PM_CTRL), (UINT16 *)&t);
    printf("\t\tStatus: D%d NoSoftRst%c PME-Enable%c DSel=%d DScale=%d PME%c\n",
           t & PCI_PM_CTRL_STATE_MASK,
           PCI_FLAG(t, PCI_PM_CTRL_NO_SOFT_RESET),
           PCI_FLAG(t, PCI_PM_CTRL_PME_ENABLE),
           (t & PCI_PM_CTRL_DATA_SEL_MASK) >> 9,
           (t & PCI_PM_CTRL_DATA_SCALE_MASK) >> 13,
           PCI_FLAG(t, PCI_PM_CTRL_PME_STATUS));

    API_PciConfigInByte(iBus, iSlot, iFunc, (iWhere + PCI_PM_PPB_EXTENSIONS), (UINT8 *)&b);
    if (b) {
        printf("\t\tBridge: PM%c B3%c\n",
               PCI_FLAG(t, PCI_PM_BPCC_ENABLE),
               PCI_FLAG(~t, PCI_PM_PPB_B2_B3));
    }
}
/*********************************************************************************************************
** 函数名称: API_PciCapShow
** 功能描述: 打印设备的扩展能力信息.
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciCapShow (INT iBus, INT iSlot, INT iFunc)
{
#define __PCI_CAP_LIST_BITS     256

    UINT8       ucBeenThere[__PCI_CAP_LIST_BITS];
    INT         iRet         = PX_ERROR;
    UINT8       ucHeaderType = 0;
    UINT16      usCapList    = 0;
    UINT8       ucWhere      = 0;
    UINT8       ucId         = 0;
    UINT8       ucNext       = 0;
    UINT16      usCap        = 0;
    INT         iExtCaps     = 0;

    API_PciConfigInWord(iBus, iSlot, iFunc, PCI_STATUS, &usCapList);
    usCapList &= PCI_STATUS_CAP_LIST;
    if (!usCapList) {
        return  (PX_ERROR);
    }

    iRet = API_PciHeaderTypeGet(iBus, iSlot, iFunc, &ucHeaderType);
    if (iRet == PX_ERROR) {
        return  (PX_ERROR);
    }

    switch (ucHeaderType) {
    
    case PCI_HEADER_TYPE_NORMAL:
    case PCI_HEADER_TYPE_BRIDGE:
        ucWhere = PCI_CAPABILITY_LIST;
        break;

    case PCI_HEADER_TYPE_CARDBUS:
        ucWhere = PCI_CB_CAPABILITY_LIST;
        break;

    default:
        return  (PX_ERROR);
    }

    API_PciConfigInByte(iBus, iSlot, iFunc, ucWhere, &ucWhere);
    lib_memset(&ucBeenThere[0], 0, __PCI_CAP_LIST_BITS);
    while (ucWhere) {
        printf("\tCapabilities: ");

        if (!API_PciConfigFetch(iBus, iSlot, iFunc, ucWhere, 4)) {
            puts("<access denied>");
            break;
        }

        API_PciConfigInByte(iBus, iSlot, iFunc, (ucWhere + PCI_CAP_LIST_ID  ), &ucId  );
        API_PciConfigInByte(iBus, iSlot, iFunc, (ucWhere + PCI_CAP_LIST_NEXT), &ucNext);
        ucNext &= ~0x03;
        API_PciConfigInWord(iBus, iSlot, iFunc, (ucWhere + PCI_CAP_FLAGS    ), &usCap );
        printf("[%02x] ", ucWhere);
        if (ucBeenThere[ucWhere]++) {
            printf("<chain looped>\n");
            break;
        }
        if (ucId == 0xff) {
            printf("<chain broken>\n");
            break;
        }

        switch (ucId) {
        
        case PCI_CAP_ID_PM:
            __pciCapPmShow(iBus, iSlot, iFunc, ucWhere, usCap);
            break;

        case PCI_CAP_ID_AGP:
            __pciCapAgpShow(iBus, iSlot, iFunc, ucWhere, usCap);
            break;

        case PCI_CAP_ID_VPD:
            API_PciCapVpdShow(iBus, iSlot, iFunc);
            break;

        case PCI_CAP_ID_SLOTID:
            __pciCapSlotIdShow(usCap);
            break;

        case PCI_CAP_ID_MSI:
            __pciCapMsiShow(iBus, iSlot, iFunc, ucWhere, usCap);
            break;

        case PCI_CAP_ID_CHSWP:
            printf("CompactPCI hot-swap <?>\n");
            break;

        case PCI_CAP_ID_PCIX:
            __pciCapPcixShow(iBus, iSlot, iFunc, ucWhere);
            iExtCaps = 1;
            break;

        case PCI_CAP_ID_HT:
            __pciCapHtShow(iBus, iSlot, iFunc, ucWhere, usCap);
            break;

        case PCI_CAP_ID_VNDR:
            __pciCapVendorShow(iBus, iSlot, iFunc, ucWhere, usCap);
            break;

        case PCI_CAP_ID_DBG:
            __pciCapDebugPortShow(usCap);
            break;

        case PCI_CAP_ID_CCRC:
            printf("CompactPCI central resource control <?>\n");
            break;

        case PCI_CAP_ID_SHPC:
            printf("Hot-plug capable\n");
            break;

        case PCI_CAP_ID_SSVID:
            __pciCapSsvidShow(iBus, iSlot, iFunc, ucWhere);
            break;

        case PCI_CAP_ID_AGP3:
            printf("AGP3 <?>\n");
            break;

        case PCI_CAP_ID_SECDEV:
            printf("Secure device <?>\n");
            break;

        case PCI_CAP_ID_EXP:
            __pciCapExpressShow(iBus, iSlot, iFunc, ucWhere, usCap);
            iExtCaps = 1;
            break;

        case PCI_CAP_ID_MSIX:
            __pciCapMsixShow(iBus, iSlot, iFunc, ucWhere, usCap);
            break;

        case PCI_CAP_ID_SATA:
            __pciCapSataHbaShow(iBus, iSlot, iFunc, ucWhere, usCap);
            break;

        case PCI_CAP_ID_AF:
            __pciCapAfShow(iBus, iSlot, iFunc, ucWhere);
            break;

        default:
            printf("#%02x [%04x]\n", ucId, usCap);
            break;
        }
        ucWhere = ucNext;
    }

    if (iExtCaps) {
        API_PciCapExtShow(iBus, iSlot, iFunc);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciCapFind
** 功能描述: 发现设备的扩展能力, 如电源管理, MSI(Message Signaled Interrupts) 等.
**           只适用于 Header type 0 (normal devices) 设备
** 输　入  : iBus        总线号
**           iSlot       插槽
**           iFunc       功能
**           ucCapId     扩展功能 ID (PCI_CAP_ID_PM, PCI_CAP_ID_MSI 等等)
**           puiOffset   扩展功能保存的地址偏移
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciCapFind (INT iBus, INT iSlot, INT iFunc, UINT8  ucCapId, UINT32 *puiOffset)
{
    INT         iRet         = PX_ERROR;
    UINT16      usTempWord   = 0x0000;
    UINT8       ucTempByte   = 0x00;
    UINT32      uiCapOffset  = 0x00;
    UINT8       ucCapOffset  = 0x00;
    UINT8       ucHeaderType = 0x00;
    INT         iCapTtl      = PCI_FIND_CAP_TTL;

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, PCI_STATUS, &usTempWord);
    if ((iRet == PX_ERROR) ||
        ((usTempWord & PCI_STATUS_CAP_LIST) == 0)) {
        return  (PX_ERROR);
    }

    iRet = API_PciHeaderTypeGet(iBus, iSlot, iFunc, &ucHeaderType);
    if (iRet == PX_ERROR) {
        return  (PX_ERROR);
    }

    switch (ucHeaderType) {
    
    case PCI_HEADER_TYPE_NORMAL:
    case PCI_HEADER_TYPE_BRIDGE:
        uiCapOffset = PCI_CAPABILITY_LIST;
        break;

    case PCI_HEADER_TYPE_CARDBUS:
        uiCapOffset = PCI_CB_CAPABILITY_LIST;
        break;

    default:
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInByte(iBus, iSlot, iFunc, uiCapOffset, &ucCapOffset);
    if (iRet == PX_ERROR) {
        return  (PX_ERROR);
    }

    uiCapOffset = ucCapOffset;
    while (iCapTtl--) {
        if (uiCapOffset < 0x40) {
            break;
        }
        uiCapOffset &= ~3;
        iRet = API_PciConfigInWord(iBus, iSlot, iFunc, uiCapOffset, &usTempWord);
        if (iRet == PX_ERROR) {
            return  (PX_ERROR);
        }
        ucTempByte = usTempWord & 0xff;
        if (ucTempByte == 0xff) {
            break;
        }
        if (ucTempByte == ucCapId) {
            if (puiOffset) {
                *puiOffset = uiCapOffset;
            }
            return  (ERROR_NONE);
        }
        uiCapOffset = (usTempWord >> 8);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_PciCapEn
** 功能描述: 发现设备是否具有扩展能力.
** 输　入  : iBus        总线号
**           iSlot       插槽
**           iFunc       功能
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciCapEn (INT iBus, INT iSlot, INT iFunc)
{
    UINT16      usTempStat = 0x0000;

    API_PciConfigInWord(iBus, iSlot, iFunc, PCI_STATUS, &usTempStat);
    if ((usTempStat & PCI_STATUS_CAP_LIST) == 0) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

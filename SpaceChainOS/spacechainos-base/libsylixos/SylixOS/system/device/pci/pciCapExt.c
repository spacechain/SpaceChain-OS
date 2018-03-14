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
** 文   件   名: pciCapExt.c
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2015 年 10 月 23 日
**
** 描        述: PCI 总线扩展功能管理的延伸功能.
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
/*********************************************************************************************************
** 函数名称: __pciCapExtl1pmShow
** 功能描述: 打印设备的扩展能力延伸 L1PM 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExtl1pmShow (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    UINT32      l1_cap;
    INT         power_on_scale;

    printf("L1 PM Substates\n");

    if (_G_iPciVerbose < 2) {
        return;
    }

    if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + 4), 4)) {
        printf("\t\t<unreadable>\n");
        return;
    }

    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + 4), &l1_cap);
    printf("\t\tL1SubCap: ");
    printf("PCI-PM_L1.2%c PCI-PM_L1.1%c ASPM_L1.2%c ASPM_L1.1%c L1_PM_Substates%c\n",
           PCI_FLAG(l1_cap, 1),
           PCI_FLAG(l1_cap, 2),
           PCI_FLAG(l1_cap, 4),
           PCI_FLAG(l1_cap, 8),
           PCI_FLAG(l1_cap, 16));

    if ((PCI_BITS(l1_cap, 0, 1)) ||
        (PCI_BITS(l1_cap, 2, 1))) {
        printf("\t\t\t  PortCommonModeRestoreTime=%dus ",
               PCI_BITS(l1_cap, 8,8));

        power_on_scale = PCI_BITS(l1_cap, 16, 2);

        printf("PortTPowerOnTime=");
        switch (power_on_scale) {
        
        case 0:
            printf("%dus\n", PCI_BITS(l1_cap, 19, 5) * 2);
            break;

        case 1:
            printf("%dus\n", PCI_BITS(l1_cap, 19, 5) * 10);
            break;

        case 2:
            printf("%dus\n", PCI_BITS(l1_cap, 19, 5) * 100);
            break;

        default:
            printf("<error>\n");
            break;
        }
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapExtPasidShow
** 功能描述: 打印设备的扩展能力延伸 PASID 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExtPasidShow (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    UINT16      w;

    printf("Process Address Space ID (PASID)\n");
    if (_G_iPciVerbose < 2) {
        return;
    }

    if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + PCI_PASID_CAP), 4)) {
        return;
    }

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_PASID_CAP), &w);
    printf("\t\tPASIDCap: Exec%c Priv%c, Max PASID Width: %02x\n",
           PCI_FLAG(w, PCI_PASID_CAP_EXEC), PCI_FLAG(w, PCI_PASID_CAP_PRIV),
           PCI_PASID_CAP_WIDTH(w));

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_PASID_CTRL), &w);
    printf("\t\tPASIDCtl: Enable%c Exec%c Priv%c\n",
           PCI_FLAG(w, PCI_PASID_CTRL_ENABLE), PCI_FLAG(w, PCI_PASID_CTRL_EXEC),
           PCI_FLAG(w, PCI_PASID_CTRL_PRIV));
}
/*********************************************************************************************************
** 函数名称: __pciCapExtLtrScale
** 功能描述: 打印设备的扩展能力延伸 LTR 比例.
** 输　入  : ucScale        比例值
** 输　出  : 换算后的值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT32  __pciCapExtLtrScale (UINT8  ucScale)
{
    return  (1 << (ucScale * 5));
}
/*********************************************************************************************************
** 函数名称: __pciCapExtLtrShow
** 功能描述: 打印设备的扩展能力延伸 LTR 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExtLtrShow (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    UINT32      scale;
    UINT16      snoop, nosnoop;

    printf("Latency Tolerance Reporting\n");
    if (_G_iPciVerbose < 2) {
        return;
    }

    if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + PCI_LTR_MAX_SNOOP), 4)) {
        return;
    }

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_LTR_MAX_SNOOP), &snoop);
    scale = __pciCapExtLtrScale((snoop >> PCI_LTR_SCALE_SHIFT) & PCI_LTR_SCALE_MASK);
    printf("\t\tMax snoop latency: %lldns\n",
           ((UINT64)snoop & PCI_LTR_VALUE_MASK) * scale);

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_LTR_MAX_NOSNOOP), &nosnoop);
    scale = __pciCapExtLtrScale((nosnoop >> PCI_LTR_SCALE_SHIFT) & PCI_LTR_SCALE_MASK);
    printf("\t\tMax no snoop latency: %lldns\n",
           ((UINT64)nosnoop & PCI_LTR_VALUE_MASK) * scale);
}
/*********************************************************************************************************
** 函数名称: __pciCapExtTphShow
** 功能描述: 打印设备的扩展能力延伸 TPH 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExtTphShow (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    UINT32      tph_cap;

    printf("Transaction Processing Hints\n");
    if (_G_iPciVerbose < 2) {
        return;
    }

    if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + PCI_TPH_CAPABILITIES), 4)) {
        return;
    }

    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_TPH_CAPABILITIES), &tph_cap);
    if (tph_cap & PCI_TPH_INTVEC_SUP) {
        printf("\t\tInterrupt vector mode supported\n");
    }
    if (tph_cap & PCI_TPH_DEV_SUP) {
        printf("\t\tDevice specific mode supported\n");
    }
    if (tph_cap & PCI_TPH_EXT_REQ_SUP) {
        printf("\t\tExtended requester support\n");
    }

    switch (tph_cap & PCI_TPH_ST_LOC_MASK) {
    
    case PCI_TPH_ST_NONE:
        printf("\t\tNo steering table available\n");
        break;

    case PCI_TPH_ST_CAP:
        printf("\t\tSteering table in TPH capability structure\n");
        break;

    case PCI_TPH_ST_MSIX:
        printf("\t\tSteering table in MSI-X table\n");
        break;

    default:
        printf("\t\tReserved steering table location\n");
        break;
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapExtPriShow
** 功能描述: 打印设备的扩展能力延伸 PRI 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExtPriShow (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    UINT16      w;
    UINT32      l;

    printf("Page Request Interface (PRI)\n");
    if (_G_iPciVerbose < 2) {
      return;
    }

    if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + PCI_PRI_CTRL), 0xc)) {
        return;
    }

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_PRI_CTRL), &w);
    printf("\t\tPRICtl: Enable%c Reset%c\n",
           PCI_FLAG(w, PCI_PRI_CTRL_ENABLE), PCI_FLAG(w, PCI_PRI_CTRL_RESET));

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_PRI_STATUS), &w);
    printf("\t\tPRISta: RF%c UPRGI%c Stopped%c\n",
           PCI_FLAG(w, PCI_PRI_STATUS_RF), PCI_FLAG(w, PCI_PRI_STATUS_UPRGI),
           PCI_FLAG(w, PCI_PRI_STATUS_STOPPED));

    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_PRI_MAX_REQ), &l);
    printf("\t\tPage Request Capacity: %08x, ", l);

    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_PRI_ALLOC_REQ), &l);
    printf("Page Request Allocation: %08x\n", l);
}
/*********************************************************************************************************
** 函数名称: __pciCapExtSriovShow
** 功能描述: 打印设备的扩展能力延伸 SRIOV 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExtSriovShow (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    UINT8               b;
    UINT16              w;
    UINT32              l;
    REGISTER INT        i;

    printf("Single Root I/O Virtualization (SR-IOV)\n");
    if (_G_iPciVerbose < 2) {
        return;
    }

    if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + PCI_IOV_CAP), 0x3c)) {
        return;
    }

    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_IOV_CAP), &l);
    printf("\t\tIOVCap:\tMigration%c, Interrupt Message Number: %03x\n",
           PCI_FLAG(l, PCI_IOV_CAP_VFM), PCI_IOV_CAP_IMN(l));
    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_IOV_CTRL), &w);
    printf("\t\tIOVCtl:\tEnable%c Migration%c Interrupt%c MSE%c ARIHierarchy%c\n",
           PCI_FLAG(w, PCI_IOV_CTRL_VFE), PCI_FLAG(w, PCI_IOV_CTRL_VFME),
           PCI_FLAG(w, PCI_IOV_CTRL_VFMIE), PCI_FLAG(w, PCI_IOV_CTRL_MSE),
           PCI_FLAG(w, PCI_IOV_CTRL_ARI));
    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_IOV_STATUS), &w);
    printf("\t\tIOVSta:\tMigration%c\n", PCI_FLAG(w, PCI_IOV_STATUS_MS));
    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_IOV_INITIALVF), &w);
    printf("\t\tInitial VFs: %d, ", w);
    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_IOV_TOTALVF), &w);
    printf("Total VFs: %d, ", w);
    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_IOV_NUMVF), &w);
    printf("Number of VFs: %d, ", w);
    API_PciConfigInByte(iBus, iSlot, iFunc, (iWhere + PCI_IOV_FDL), &b);
    printf("Function Dependency Link: %02x\n", b);
    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_IOV_OFFSET), &w);
    printf("\t\tVF offset: %d, ", w);
    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_IOV_STRIDE), &w);
    printf("stride: %d, ", w);
    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_IOV_DID), &w);
    printf("Device ID: %04x\n", w);
    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_IOV_SUPPS), &l);
    printf("\t\tSupported Page Size: %08x, ", l);
    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_IOV_SYSPS), &l);
    printf("System Page Size: %08x\n", l);

    for (i = 0; i < PCI_IOV_NUM_BAR; i++) {
        UINT32      addr;
        INT         type;
        UINT32      h;

        API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_IOV_BAR_BASE + (4 * i)), &l);
        if (l == 0xffffffff) {
            l = 0;
        }
        if (!l) {
            continue;
        }

        printf("\t\tRegion %d: Memory at ", i);
        addr = l & PCI_BASE_ADDRESS_MEM_MASK;
        type = l & PCI_BASE_ADDRESS_MEM_TYPE_MASK;
        if (type == PCI_BASE_ADDRESS_MEM_TYPE_64) {
            i++;

            API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_IOV_BAR_BASE), &h);
            printf("%08x", h);
        }

        printf("%08x (%s-bit, %sprefetchable)\n",
               addr,
               (type == PCI_BASE_ADDRESS_MEM_TYPE_32) ? "32" : "64",
               (l & PCI_BASE_ADDRESS_MEM_PREFETCH) ? "" : "non-");
    }

    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_IOV_MSAO), &l);
    printf("\t\tVF Migration: offset: %08x, BIR: %x\n", PCI_IOV_MSA_OFFSET(l),
           PCI_IOV_MSA_BIR(l));
}
/*********************************************************************************************************
** 函数名称: __pciCapExtAtsShow
** 功能描述: 打印设备的扩展能力延伸 ATS 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExtAtsShow (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    UINT16      w;

    printf("Address Translation Service (ATS)\n");
    if (_G_iPciVerbose < 2) {
        return;
    }

    if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + PCI_ATS_CAP), 4)) {
        return;
    }

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_ATS_CAP), &w);
    printf("\t\tATSCap:\tInvalidate Queue Depth: %02x\n", PCI_ATS_CAP_IQD(w));
    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_ATS_CTRL), &w);
    printf("\t\tATSCtl:\tEnable%c, Smallest Translation Unit: %02x\n",
           PCI_FLAG(w, PCI_ATS_CTRL_ENABLE), PCI_ATS_CTRL_STU(w));
}
/*********************************************************************************************************
** 函数名称: __pciCapExtAriShow
** 功能描述: 打印设备的扩展能力延伸 ARI 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExtAriShow (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    UINT16      w;

    printf("Alternative Routing-ID Interpretation (ARI)\n");
    if (_G_iPciVerbose < 2) {
        return;
    }

    if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + PCI_ARI_CAP), 4)) {
        return;
    }

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_ARI_CAP), &w);
    printf("\t\tARICap:\tMFVC%c ACS%c, Next Function: %d\n",
           PCI_FLAG(w, PCI_ARI_CAP_MFVC), PCI_FLAG(w, PCI_ARI_CAP_ACS),
           PCI_ARI_CAP_NFN(w));
    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_ARI_CTRL), &w);
    printf("\t\tARICtl:\tMFVC%c ACS%c, Function Group: %d\n",
           PCI_FLAG(w, PCI_ARI_CTRL_MFVC), PCI_FLAG(w, PCI_ARI_CTRL_ACS),
           PCI_ARI_CTRL_FG(w));
}
/*********************************************************************************************************
** 函数名称: __pciCapExtAcsShow
** 功能描述: 打印设备的扩展能力延伸 ACS 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExtAcsShow (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    UINT16      w;

    printf("Access Control Services\n");
    if (_G_iPciVerbose < 2) {
        return;
    }

    if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + PCI_ACS_CAP), 4)) {
        return;
    }

    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_ACS_CAP), &w);
    printf("\t\tACSCap:\tSrcValid%c TransBlk%c ReqRedir%c CmpltRedir%c UpstreamFwd%c EgressCtrl%c "
           "DirectTrans%c\n",
           PCI_FLAG(w, PCI_ACS_CAP_VALID), PCI_FLAG(w, PCI_ACS_CAP_BLOCK),
           PCI_FLAG(w, PCI_ACS_CAP_REQ_RED),
           PCI_FLAG(w, PCI_ACS_CAP_CMPLT_RED), PCI_FLAG(w, PCI_ACS_CAP_FORWARD),
           PCI_FLAG(w, PCI_ACS_CAP_EGRESS),
           PCI_FLAG(w, PCI_ACS_CAP_TRANS));
    API_PciConfigInWord(iBus, iSlot, iFunc, (iWhere + PCI_ACS_CTRL), &w);
    printf("\t\tACSCtl:\tSrcValid%c TransBlk%c ReqRedir%c CmpltRedir%c UpstreamFwd%c EgressCtrl%c "
           "DirectTrans%c\n",
           PCI_FLAG(w, PCI_ACS_CTRL_VALID), PCI_FLAG(w, PCI_ACS_CTRL_BLOCK),
           PCI_FLAG(w, PCI_ACS_CTRL_REQ_RED),
           PCI_FLAG(w, PCI_ACS_CTRL_CMPLT_RED), PCI_FLAG(w, PCI_ACS_CTRL_FORWARD),
           PCI_FLAG(w, PCI_ACS_CTRL_EGRESS),
           PCI_FLAG(w, PCI_ACS_CTRL_TRANS));
}
/*********************************************************************************************************
** 函数名称: __pciCapExtEvendorShow
** 功能描述: 打印设备的扩展能力延伸 EVENDOR 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExtEvendorShow (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    UINT32      hdr;

    printf("Vendor Specific Information: ");

    if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + PCI_EVNDR_HEADER), 4)) {
        printf("<unreadable>\n");
        return;
    }

    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_EVNDR_HEADER), &hdr);
    printf("ID=%04x Rev=%d Len=%03x <?>\n",
           PCI_BITS(hdr, 0, 16),
           PCI_BITS(hdr, 16, 4),
           PCI_BITS(hdr, 20, 12));
}
/*********************************************************************************************************
** 函数名称: __pciCapExtRclinkShow
** 功能描述: 打印设备的扩展能力延伸 RCLINK 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExtRclinkShow (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    UINT32                  esd;
    INT                     num_links;
    REGISTER INT            i;
    static const CHAR       elt_types[][9] = {"Config", "Egress", "Internal"};
    CHAR                    buf[8];

    printf("Root Complex Link\n");
    if (_G_iPciVerbose < 2) {
        return;
    }

    if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + 4), PCI_RCLINK_LINK1 - 4)) {
        return;
    }

    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_RCLINK_ESD), &esd);
    num_links = PCI_BITS(esd, 8, 8);
    printf("\t\tDesc:\tPortNumber=%02x ComponentID=%02x EltType=%s\n",
            PCI_BITS(esd, 24, 8),
            PCI_BITS(esd, 16, 8),
            PCI_TABLE(elt_types, PCI_BITS(esd, 0, 8), buf));

    for (i = 0; i < num_links; i++) {
        INT         pos = iWhere + PCI_RCLINK_LINK1 + i*PCI_RCLINK_LINK_SIZE;
        UINT32      desc;
        UINT32      addr_lo, addr_hi;

        printf("\t\tLink%d:\t", i);

        if (!API_PciConfigFetch(iBus, iSlot, iFunc, pos, PCI_RCLINK_LINK_SIZE)) {
            printf("<unreadable>\n");
            return;
        }

        API_PciConfigInDword(iBus, iSlot, iFunc, (pos + PCI_RCLINK_LINK_DESC    ), &desc   );
        API_PciConfigInDword(iBus, iSlot, iFunc, (pos + PCI_RCLINK_LINK_ADDR    ), &addr_lo);
        API_PciConfigInDword(iBus, iSlot, iFunc, (pos + PCI_RCLINK_LINK_ADDR + 4), &addr_hi);
        printf("Desc:\tTargetPort=%02x TargetComponent=%02x AssocRCRB%c LinkType=%s LinkValid%c\n",
               PCI_BITS(desc, 24, 8),
               PCI_BITS(desc, 16, 8),
               PCI_FLAG(desc, 4),
               ((desc & 2) ? "Config" : "MemMapped"),
               PCI_FLAG(desc, 1));

        if (desc & 2) {
            INT     n = addr_lo & 7;

            if (!n) {
                n = 8;
            }

            printf("\t\t\tAddr:\t%02x:%02x.%d  CfgSpace=%08x%08x\n",
                   PCI_BITS(addr_lo, 20, n),
                   PCI_BITS(addr_lo, 15, 5),
                   PCI_BITS(addr_lo, 12, 3),
                   addr_hi, addr_lo);
        } else {
            printf("\t\t\tAddr:\t%08x%08x\n", addr_hi, addr_lo);
        }
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapExtDsnShow
** 功能描述: 打印设备的扩展能力延伸 DSN 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExtDsnShow (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    UINT32      t1, t2;

    if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + 4), 8)) {
        return;
    }

    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + 4), &t1);
    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + 8), &t2);
    printf("Device Serial Number %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",
           t2 >> 24, (t2 >> 16) & 0xff, (t2 >> 8) & 0xff, t2 & 0xff,
           t1 >> 24, (t1 >> 16) & 0xff, (t1 >> 8) & 0xff, t1 & 0xff);
}
/*********************************************************************************************************
** 函数名称: __pciCapExtVcShow
** 功能描述: 打印设备的扩展能力延伸 VC 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExtVcShow (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    UINT32                  cr1, cr2;
    UINT16                  ctrl, status;
    INT                     evc_cnt;
    INT                     arb_table_pos;
    INT                     i, j;
    static const CHAR       ref_clocks[][6]      = {"100ns" };
    static const CHAR       arb_selects[8][7]    = {"Fixed", "WRR32", "WRR64", "WRR128", "??4", "??5",
                                                    "??6", "??7" };
    static const CHAR       vc_arb_selects[8][8] = {"Fixed", "WRR32", "WRR64", "WRR128", "TWRR128",
                                                    "WRR256", "??6", "??7" };
    CHAR                    buf[8];

    printf("Virtual Channel\n");
    if (_G_iPciVerbose < 2) {
        return;
    }

    if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + 4), 0x1c - 4)) {
        return;
    }

    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_VC_PORT_REG1  ), &cr1   );
    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_VC_PORT_REG2  ), &cr2   );
    API_PciConfigInWord (iBus, iSlot, iFunc, (iWhere + PCI_VC_PORT_CTRL  ), &ctrl  );
    API_PciConfigInWord (iBus, iSlot, iFunc, (iWhere + PCI_VC_PORT_STATUS), &status);
    evc_cnt = PCI_BITS(cr1, 0, 3);
    printf("\t\tCaps:\tLPEVC=%d RefClk=%s PATEntryBits=%d\n",
           PCI_BITS(cr1, 4, 3),
           PCI_TABLE(ref_clocks, PCI_BITS(cr1, 8, 2), buf),
           1 << PCI_BITS(cr1, 10, 2));

    printf("\t\tArb:");
    for (i = 0; i < 8; i++) {
        if ((arb_selects[i][0] != '?') ||
            (cr2 & (1 << i)          )) {
            printf("%c%s%c", (i ? ' ' : '\t'), arb_selects[i], PCI_FLAG(cr2, 1 << i));
        }
    }
    arb_table_pos = PCI_BITS(cr2, 24, 8);

    printf("\n\t\tCtrl:\tArbSelect=%s\n", PCI_TABLE(arb_selects, PCI_BITS(ctrl, 1, 3), buf));
    printf("\t\tStatus:\tInProgress%c\n", PCI_FLAG(status, 1));

    if (arb_table_pos) {
        arb_table_pos = iWhere + (16 * arb_table_pos);
        printf("\t\tPort Arbitration Table [%x] <?>\n", arb_table_pos);
    }

    for (i = 0; i <= evc_cnt; i++) {
        INT         pos = iWhere + PCI_VC_RES_CAP + 12*i;
        UINT32      rcap, rctrl;
        UINT16      rstatus;
        INT         pat_pos;

        printf("\t\tVC%d:\t", i);
        if (!API_PciConfigFetch(iBus, iSlot, iFunc, pos, 12)) {
            printf("<unreadable>\n");
            continue;
        }
        API_PciConfigInDword(iBus, iSlot, iFunc, (pos     ), &rcap   );
        API_PciConfigInDword(iBus, iSlot, iFunc, (pos +  4), &rctrl  );
        API_PciConfigInWord (iBus, iSlot, iFunc, (pos + 10), &rstatus);

        pat_pos = PCI_BITS(rcap, 24, 8);
        printf("Caps:\tPATOffset=%02x MaxTimeSlots=%d RejSnoopTrans%c\n",
               pat_pos,
               PCI_BITS(rcap, 16, 6) + 1,
               PCI_FLAG(rcap, 1 << 15));

        printf("\t\t\tArb:");
        for (j = 0; j < 8; j++) {
            if ((vc_arb_selects[j][0] != '?') ||
                (rcap & (1 << j)            )) {
                printf("%c%s%c", (j ? ' ' : '\t'), vc_arb_selects[j], PCI_FLAG(rcap, 1 << j));
            }
        }

        printf("\n\t\t\tCtrl:\tEnable%c ID=%d ArbSelect=%s TC/VC=%02x\n",
               PCI_FLAG(rctrl, 1 << 31),
               PCI_BITS(rctrl, 24, 3),
               PCI_TABLE(vc_arb_selects, PCI_BITS(rctrl, 17, 3), buf),
               PCI_BITS(rctrl, 0, 8));

        printf("\t\t\tStatus:\tNegoPending%c InProgress%c\n",
               PCI_FLAG(rstatus, 2),
               PCI_FLAG(rstatus, 1));

        if (pat_pos) {
            printf("\t\t\tPort Arbitration Table <?>\n");
        }
    }
}
/*********************************************************************************************************
** 函数名称: __pciCapExtAerShow
** 功能描述: 打印设备的扩展能力延伸 AER 信息.
** 输　入  : iBus       总线号
**           iSlot      插槽
**           iFunc      功能
**           iWhere     扩展功能保存的地址偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciCapExtAerShow (INT iBus, INT iSlot, INT iFunc, INT iWhere)
{
    UINT32      l;

    printf("Advanced Error Reporting\n");
    if (_G_iPciVerbose < 2) {
        return;
    }

    if (!API_PciConfigFetch(iBus, iSlot, iFunc, (iWhere + PCI_ERR_UNCOR_STATUS), 24)) {
        return;
    }

    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_ERR_UNCOR_STATUS), &l);
    printf("\t\tUESta:\tDLP%c SDES%c TLP%c FCP%c CmpltTO%c CmpltAbrt%c UnxCmplt%c RxOF%c "
           "MalfTLP%c ECRC%c UnsupReq%c ACSViol%c\n",
           PCI_FLAG(l, PCI_ERR_UNC_DLP), PCI_FLAG(l, PCI_ERR_UNC_SDES),
           PCI_FLAG(l, PCI_ERR_UNC_POISON_TLP),
           PCI_FLAG(l, PCI_ERR_UNC_FCP), PCI_FLAG(l, PCI_ERR_UNC_COMP_TIME),
           PCI_FLAG(l, PCI_ERR_UNC_COMP_ABORT),
           PCI_FLAG(l, PCI_ERR_UNC_UNX_COMP), PCI_FLAG(l, PCI_ERR_UNC_RX_OVER),
           PCI_FLAG(l, PCI_ERR_UNC_MALF_TLP),
           PCI_FLAG(l, PCI_ERR_UNC_ECRC), PCI_FLAG(l, PCI_ERR_UNC_UNSUP),
           PCI_FLAG(l, PCI_ERR_UNC_ACS_VIOL));
    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_ERR_UNCOR_MASK), &l);
    printf("\t\tUEMsk:\tDLP%c SDES%c TLP%c FCP%c CmpltTO%c CmpltAbrt%c UnxCmplt%c RxOF%c "
           "MalfTLP%c ECRC%c UnsupReq%c ACSViol%c\n",
           PCI_FLAG(l, PCI_ERR_UNC_DLP), PCI_FLAG(l, PCI_ERR_UNC_SDES),
           PCI_FLAG(l, PCI_ERR_UNC_POISON_TLP),
           PCI_FLAG(l, PCI_ERR_UNC_FCP), PCI_FLAG(l, PCI_ERR_UNC_COMP_TIME),
           PCI_FLAG(l, PCI_ERR_UNC_COMP_ABORT),
           PCI_FLAG(l, PCI_ERR_UNC_UNX_COMP), PCI_FLAG(l, PCI_ERR_UNC_RX_OVER),
           PCI_FLAG(l, PCI_ERR_UNC_MALF_TLP),
           PCI_FLAG(l, PCI_ERR_UNC_ECRC), PCI_FLAG(l, PCI_ERR_UNC_UNSUP),
           PCI_FLAG(l, PCI_ERR_UNC_ACS_VIOL));
    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_ERR_UNCOR_SEVER), &l);
    printf("\t\tUESvrt:\tDLP%c SDES%c TLP%c FCP%c CmpltTO%c CmpltAbrt%c UnxCmplt%c RxOF%c "
           "MalfTLP%c ECRC%c UnsupReq%c ACSViol%c\n",
           PCI_FLAG(l, PCI_ERR_UNC_DLP), PCI_FLAG(l, PCI_ERR_UNC_SDES),
           PCI_FLAG(l, PCI_ERR_UNC_POISON_TLP),
           PCI_FLAG(l, PCI_ERR_UNC_FCP), PCI_FLAG(l, PCI_ERR_UNC_COMP_TIME),
           PCI_FLAG(l, PCI_ERR_UNC_COMP_ABORT),
           PCI_FLAG(l, PCI_ERR_UNC_UNX_COMP), PCI_FLAG(l, PCI_ERR_UNC_RX_OVER),
           PCI_FLAG(l, PCI_ERR_UNC_MALF_TLP),
           PCI_FLAG(l, PCI_ERR_UNC_ECRC), PCI_FLAG(l, PCI_ERR_UNC_UNSUP),
           PCI_FLAG(l, PCI_ERR_UNC_ACS_VIOL));
    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_ERR_COR_STATUS), &l);
    printf("\t\tCESta:\tRxErr%c BadTLP%c BadDLLP%c Rollover%c Timeout%c NonFatalErr%c\n",
           PCI_FLAG(l, PCI_ERR_COR_RCVR), PCI_FLAG(l, PCI_ERR_COR_BAD_TLP),
           PCI_FLAG(l, PCI_ERR_COR_BAD_DLLP),
           PCI_FLAG(l, PCI_ERR_COR_REP_ROLL), PCI_FLAG(l, PCI_ERR_COR_REP_TIMER),
           PCI_FLAG(l, PCI_ERR_COR_REP_ANFE));
    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_ERR_COR_MASK), &l);
    printf("\t\tCEMsk:\tRxErr%c BadTLP%c BadDLLP%c Rollover%c Timeout%c NonFatalErr%c\n",
           PCI_FLAG(l, PCI_ERR_COR_RCVR), PCI_FLAG(l, PCI_ERR_COR_BAD_TLP),
           PCI_FLAG(l, PCI_ERR_COR_BAD_DLLP),
           PCI_FLAG(l, PCI_ERR_COR_REP_ROLL), PCI_FLAG(l, PCI_ERR_COR_REP_TIMER),
           PCI_FLAG(l, PCI_ERR_COR_REP_ANFE));
    API_PciConfigInDword(iBus, iSlot, iFunc, (iWhere + PCI_ERR_CAP), &l);
    printf("\t\tAERCap:\tFirst Error Pointer: %02x, GenCap%c CGenEn%c ChkCap%c ChkEn%c\n",
           PCI_ERR_CAP_FEP(l), PCI_FLAG(l, PCI_ERR_CAP_ECRC_GENC), PCI_FLAG(l, PCI_ERR_CAP_ECRC_GENE),
           PCI_FLAG(l, PCI_ERR_CAP_ECRC_CHKC), PCI_FLAG(l, PCI_ERR_CAP_ECRC_CHKE));

}
/*********************************************************************************************************
** 函数名称: API_PciCapExtShow
** 功能描述: 打印设备的扩展能力的延伸信息.
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PciCapExtShow (INT iBus, INT iSlot, INT iFunc)
{
#define __PCI_CAP_EXT_BITS                  0x1000
    INT         iWhere = 0x100;
    CHAR        been_there[__PCI_CAP_EXT_BITS];

    lib_memset(been_there, 0, __PCI_CAP_EXT_BITS);
    do {
        UINT32      header;
        INT         id, version;

        if (!API_PciConfigFetch(iBus, iSlot, iFunc, iWhere, 4)) {
            return;
        }

        API_PciConfigInDword(iBus, iSlot, iFunc, iWhere, &header);
        if (!header) {
            break;
        }
        id = header & 0xffff;
        version = (header >> 16) & 0xf;
        printf("\tCapabilities: [%03x", iWhere);
        if (_G_iPciVerbose > 1) {
            printf(" v%d", version);
        }
        printf("] ");
        if (been_there[iWhere]++) {
            printf("<chain looped>\n");
            break;
        }
        switch (id) {
        
        case PCI_EXT_CAP_ID_AER:
            __pciCapExtAerShow(iBus, iSlot, iFunc, iWhere);
            break;

        case PCI_EXT_CAP_ID_VC:
        case PCI_EXT_CAP_ID_VC2:
            __pciCapExtVcShow(iBus, iSlot, iFunc, iWhere);
            break;

        case PCI_EXT_CAP_ID_DSN:
            __pciCapExtDsnShow(iBus, iSlot, iFunc, iWhere);
            break;

        case PCI_EXT_CAP_ID_PB:
            printf("Power Budgeting <?>\n");
            break;

        case PCI_EXT_CAP_ID_RCLINK:
            __pciCapExtRclinkShow(iBus, iSlot, iFunc, iWhere);
            break;

        case PCI_EXT_CAP_ID_RCILINK:
            printf("Root Complex Internal Link <?>\n");
            break;

        case PCI_EXT_CAP_ID_RCECOLL:
            printf("Root Complex Event Collector <?>\n");
            break;

        case PCI_EXT_CAP_ID_MFVC:
            printf("Multi-Function Virtual Channel <?>\n");
            break;

        case PCI_EXT_CAP_ID_RBCB:
            printf("Root Bridge Control Block <?>\n");
            break;

        case PCI_EXT_CAP_ID_VNDR:
            __pciCapExtEvendorShow(iBus, iSlot, iFunc, iWhere);
            break;

        case PCI_EXT_CAP_ID_ACS:
            __pciCapExtAcsShow(iBus, iSlot, iFunc, iWhere);
            break;

        case PCI_EXT_CAP_ID_ARI:
            __pciCapExtAriShow(iBus, iSlot, iFunc, iWhere);
            break;

        case PCI_EXT_CAP_ID_ATS:
            __pciCapExtAtsShow(iBus, iSlot, iFunc, iWhere);
            break;

        case PCI_EXT_CAP_ID_SRIOV:
            __pciCapExtSriovShow(iBus, iSlot, iFunc, iWhere);
            break;

        case PCI_EXT_CAP_ID_PRI:
            __pciCapExtPriShow(iBus, iSlot, iFunc, iWhere);
            break;

        case PCI_EXT_CAP_ID_TPH:
            __pciCapExtTphShow(iBus, iSlot, iFunc, iWhere);
            break;

        case PCI_EXT_CAP_ID_LTR:
            __pciCapExtLtrShow(iBus, iSlot, iFunc, iWhere);
            break;

        case PCI_EXT_CAP_ID_PASID:
            __pciCapExtPasidShow(iBus, iSlot, iFunc, iWhere);
            break;

        case PCI_EXT_CAP_ID_L1PM:
            __pciCapExtl1pmShow(iBus, iSlot, iFunc, iWhere);
            break;

        default:
            printf("#%02x\n", id);
            break;
        }

        iWhere = (header >> 20) & ~3;
    } while (iWhere);
}
/*********************************************************************************************************
** 函数名称: API_PciCapExtFind
** 功能描述: 发现设备的延伸扩展能力
** 输　入  : iBus           总线号
**           iSlot          设备槽号
**           iFunc          功能
**           ucExtCapId     延伸扩展功能
**           puiOffset      延伸扩展功能保存的地址偏移
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciCapExtFind (INT iBus, INT iSlot, INT iFunc, UINT8  ucExtCapId, UINT32 *puiOffset)
{
    INT         iRet     = PX_ERROR;
    INT         iStart   = 0;
    UINT32      uiHeader = 0;
    INT         iTtl     = (PCI_CFG_SPACE_EXP_SIZE - PCI_CFG_SPACE_SIZE) / 8;
    INT         iPos     = PCI_CFG_SPACE_SIZE;

    if (iStart) {
        iPos = iStart;
    }

    iRet = API_PciConfigInDword(iBus, iSlot, iFunc, iPos, &uiHeader);
    if ((iRet     == PX_ERROR) ||
        (uiHeader == 0       )) {
        return  (PX_ERROR);
    }

    while (iTtl-- > 0) {
        if ((PCI_EXT_CAP_ID(uiHeader) == ucExtCapId) &&
            (iPos != iStart                        )) {
            if (puiOffset) {
                *puiOffset = iPos;
            }
            return  (ERROR_NONE);
        }

        iPos = PCI_EXT_CAP_NEXT(uiHeader);
        if (iPos < PCI_CFG_SPACE_SIZE) {
            break;
        }

        iRet = API_PciConfigInDword(iBus, iSlot, iFunc, iPos, &uiHeader);
        if (iRet == PX_ERROR) {
            break;
        }
    }

    return  (PX_ERROR);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

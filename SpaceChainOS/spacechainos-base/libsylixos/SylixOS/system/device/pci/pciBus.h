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
** 文   件   名: pciBus.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 09 月 28 日
**
** 描        述: PCI 总线驱动模型.
*********************************************************************************************************/

#ifndef __PCIBUS_H
#define __PCIBUS_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
/*********************************************************************************************************
  Under PCI, each device has 256 bytes of configuration address space,
  of which the first 64 bytes are standardized as follows:
*********************************************************************************************************/
#define PCI_STD_HEADER_SIZEOF       64
#define PCI_VENDOR_ID               0x00        /* 16 bits                                              */
#define PCI_DEVICE_ID               0x02        /* 16 bits                                              */

#define PCI_COMMAND                 0x04        /* 16 bits                                              */
#define PCI_COMMAND_IO              0x0001      /* Enable response in I/O space                         */
#define PCI_COMMAND_MEMORY          0x0002      /* Enable response in Memory space                      */
#define PCI_COMMAND_MASTER          0x0004      /* Enable bus mastering                                 */
#define PCI_COMMAND_SPECIAL         0x0008      /* Enable response to special cycles                    */
#define PCI_COMMAND_INVALIDATE      0x0010      /* Use memory write and invalidate                      */
#define PCI_COMMAND_VGA_PALETTE     0x0020      /* Enable palette snooping                              */
#define PCI_COMMAND_PARITY          0x0040      /* Enable parity checking                               */
#define PCI_COMMAND_WAIT            0x0080      /* Enable address/data stepping                         */
#define PCI_COMMAND_SERR            0x0100      /* Enable SERR                                          */
#define PCI_COMMAND_FAST_BACK       0x0200      /* Enable back-to-back writes                           */
#define PCI_COMMAND_INTX_DISABLE    0x0400      /* INTx Emulation Disable                               */
#define PCI_COMMAND_MASK            0xffff0000

#define PCI_STATUS                  0x06        /* 16 bits                                              */
#define PCI_STATUS_INTERRUPT        0x0008      /* Interrupt status                                     */
#define PCI_STATUS_CAP_LIST         0x0010      /* Support Capability List                              */
#define PCI_STATUS_66MHZ            0x0020      /* Support 66 MHz PCI 2.1 bus                           */
#define PCI_STATUS_UDF              0x0040      /* Support User Definable Features [obsolete]           */
#define PCI_STATUS_FAST_BACK        0x0080      /* Accept fast-back to back                             */
#define PCI_STATUS_PARITY           0x0100      /* Detected parity error                                */
#define PCI_STATUS_DEVSEL_MASK      0x0600      /* DEVSEL timing                                        */
#define PCI_STATUS_DEVSEL_FAST      0x0000
#define PCI_STATUS_DEVSEL_MEDIUM    0x0200
#define PCI_STATUS_DEVSEL_SLOW      0x0400
#define PCI_STATUS_SIG_TARGET_ABORT 0x0800      /* Set on target abort                                  */
#define PCI_STATUS_REC_TARGET_ABORT 0x1000      /* Master ack of "                                      */
#define PCI_STATUS_REC_MASTER_ABORT 0x2000      /* Set on master abort                                  */
#define PCI_STATUS_SIG_SYSTEM_ERROR 0x4000      /* Set when we drive SERR                               */
#define PCI_STATUS_DETECTED_PARITY  0x8000      /* Set on parity error                                  */

#define PCI_CLASS_REVISION          0x08        /* High 24 bits are class, low 8 revision               */
#define PCI_REVISION_ID             0x08        /* Revision ID                                          */
#define PCI_CLASS_PROG              0x09        /* Reg. Level Programming Interface                     */
#define PCI_CLASS_DEVICE            0x0a        /* Device class                                         */
#define PCI_CLASS                   0x0b        /* class high 8 bits (class code)                       */

#define PCI_CACHE_LINE_SIZE         0x0c        /* 8 bits                                               */
#define PCI_LATENCY_TIMER           0x0d        /* 8 bits                                               */
#define PCI_HEADER_TYPE             0x0e        /* 8 bits                                               */
#define PCI_HEADER_TYPE_NORMAL      0
#define PCI_HEADER_TYPE_BRIDGE      1
#define PCI_HEADER_TYPE_CARDBUS     2

#define PCI_HEADER_TYPE_MASK        0x7f        /* mask for header type                                 */
#define PCI_HEADER_PCI_PCI          0x01        /* PCI to PCI bridge                                    */
#define PCI_HEADER_MULTI_FUNC       0x80        /* multi function device                                */
#define PCI_HEADER_PCI_CARDBUS      0x02        /* PCI to CardBus bridge                                */
#define PCI_HEADER_TYPE0            0x00        /* normal device header                                 */

#define PCI_BIST                    0x0f        /* 8 bits                                               */
#define PCI_BIST_CODE_MASK          0x0f        /* Return result                                        */
#define PCI_BIST_START              0x40        /* 1 to start BIST, 2 secs or less                      */
#define PCI_BIST_CAPABLE            0x80        /* 1 if BIST capable                                    */

/*********************************************************************************************************
  Base addresses specify locations in memory or I/O space.
  Decoded size can be determined by writing a value of
  0xffffffff to the register, and reading it back.  Only
  1 bits are decoded.
*********************************************************************************************************/
#define PCI_BASE_ADDRESS_0              0x10    /* 32 bits                                              */
#define PCI_BASE_ADDRESS_1              0x14    /* 32 bits [htype 0,1 only]                             */
#define PCI_BASE_ADDRESS_2              0x18    /* 32 bits [htype 0 only]                               */
#define PCI_BASE_ADDRESS_3              0x1c    /* 32 bits                                              */
#define PCI_BASE_ADDRESS_4              0x20    /* 32 bits                                              */
#define PCI_BASE_ADDRESS_5              0x24    /* 32 bits                                              */
#define PCI_BASE_ADDRESS_SPACE          0x01    /* 0 = memory, 1 = I/O                                  */
#define PCI_BASE_ADDRESS_SPACE_IO       0x01
#define PCI_BASE_ADDRESS_SPACE_MEMORY   0x00
#define PCI_BASE_ADDRESS_MEM_TYPE_MASK  0x06
#define PCI_BASE_ADDRESS_MEM_TYPE_32    0x00    /* 32 bit address                                       */
#define PCI_BASE_ADDRESS_MEM_TYPE_1M    0x02    /* Below 1M [obsolete]                                  */
#define PCI_BASE_ADDRESS_MEM_TYPE_64    0x04    /* 64 bit address                                       */
#define PCI_BASE_ADDRESS_MEM_PREFETCH   0x08    /* prefetchable?                                        */
#define PCI_BASE_ADDRESS_MEM_MASK       (~(pci_addr_t)0x0fUL)
#define PCI_BASE_ADDRESS_IO_MASK        (~(pci_addr_t)0x03UL)
                                                /* bit 1 is reserved if address_space = 1               */
/*********************************************************************************************************
  Header type 0 (normal devices)
*********************************************************************************************************/
#define PCI_CARDBUS_CIS             0x28
#define PCI_SUBSYSTEM_VENDOR_ID     0x2c
#define PCI_SUBSYSTEM_ID            0x2e
#define PCI_ROM_ADDRESS             0x30        /* Bits 31..11 are address, 10..1 reserved              */
#define PCI_ROM_ADDRESS_ENABLE      0x01
#define PCI_ROM_ADDRESS_MASK        (~(pci_addr_t)0x7ffUL)

#define PCI_CAPABILITY_LIST         0x34        /* Offset of first capability list entry                */
                                                /* 0x35-0x3b are reserved                               */
#define PCI_INTERRUPT_LINE          0x3c        /* 8 bits                                               */
#define PCI_INTERRUPT_PIN           0x3d        /* 8 bits                                               */
#define PCI_MIN_GNT                 0x3e        /* 8 bits                                               */
#define PCI_MAX_LAT                 0x3f        /* 8 bits                                               */

/*********************************************************************************************************
  Header type 1 (PCI-to-PCI bridges)
*********************************************************************************************************/
#define PCI_PRIMARY_BUS             0x18        /* Primary bus number                                   */
#define PCI_SECONDARY_BUS           0x19        /* Secondary bus number                                 */
#define PCI_SUBORDINATE_BUS         0x1a        /* Highest bus number behind the bridge                 */
#define PCI_SEC_LATENCY_TIMER       0x1b        /* Latency timer for secondary interface                */
#define PCI_IO_BASE                 0x1c        /* I/O range behind the bridge                          */
#define PCI_IO_LIMIT                0x1d
#define PCI_IO_RANGE_TYPE_MASK      0x0fUL      /* I/O bridging type                                    */
#define PCI_IO_RANGE_TYPE_16        0x00
#define PCI_IO_RANGE_TYPE_32        0x01
#define PCI_IO_RANGE_MASK           (~0x0fUL)   /* Standard 4K I/O windows                              */
#define PCI_IO_1K_RANGE_MASK        (~0x03UL)   /* Intel 1K I/O windows                                 */
#define PCI_SEC_STATUS              0x1e        /* Secondary status register, only bit 14 used          */
#define PCI_MEMORY_BASE             0x20        /* Memory range behind                                  */
#define PCI_MEMORY_LIMIT            0x22
#define PCI_MEMORY_RANGE_TYPE_MASK  0x0fUL
#define PCI_MEMORY_RANGE_MASK       (~0x0fUL)
#define PCI_PREF_MEMORY_BASE        0x24        /* Prefetchable memory range behind                     */
#define PCI_PREF_MEMORY_LIMIT       0x26
#define PCI_PREF_RANGE_TYPE_MASK    0x0fUL
#define PCI_PREF_RANGE_TYPE_32      0x00
#define PCI_PREF_RANGE_TYPE_64      0x01
#define PCI_PREF_RANGE_MASK         (~0x0fUL)
#define PCI_PREF_BASE_UPPER32       0x28        /* Upper half of prefetchable memory range              */
#define PCI_PREF_LIMIT_UPPER32      0x2c
#define PCI_IO_BASE_UPPER16         0x30        /* Upper half of I/O addresses                          */
#define PCI_IO_LIMIT_UPPER16        0x32
                                                /* 0x34 same as for htype 0                             */
                                                /* 0x35-0x3b is reserved                                */
#define PCI_ROM_ADDRESS1            0x38        /* Same as PCI_ROM_ADDRESS, but for htype 1             */
                                                /* 0x3c-0x3d are same as for htype 0                    */
#define PCI_BRIDGE_CONTROL          0x3e
#define PCI_BRIDGE_CTL_PARITY       0x01        /* Enable parity detection on secondary interface       */
#define PCI_BRIDGE_CTL_SERR         0x02        /* The same for SERR forwarding                         */
#define PCI_BRIDGE_CTL_NO_ISA       0x04        /* Disable bridging of ISA ports                        */
#define PCI_BRIDGE_CTL_VGA          0x08        /* Forward VGA addresses                                */
#define PCI_BRIDGE_CTL_MASTER_ABORT 0x20        /* Report master aborts                                 */
#define PCI_BRIDGE_CTL_BUS_RESET    0x40        /* Secondary bus reset                                  */
#define PCI_BRIDGE_CTL_FAST_BACK    0x80        /* Fast Back2Back enabled on secondary interface        */
#define PCI_BRIDGE_CTL_PRI_DISCARD_TIMER        0x100
#define PCI_BRIDGE_CTL_SEC_DISCARD_TIMER        0x200
#define PCI_BRIDGE_CTL_DISCARD_TIMER_STATUS     0x400
#define PCI_BRIDGE_CTL_DISCARD_TIMER_SERR_EN    0x800

/*********************************************************************************************************
  Header type 2 (CardBus bridges)
*********************************************************************************************************/
#define PCI_CB_CAPABILITY_LIST          0x14
                                                /* 0x15 reserved                                        */
#define PCI_CB_SEC_STATUS               0x16    /* Secondary status                                     */
#define PCI_CB_PRIMARY_BUS              0x18    /* PCI bus number                                       */
#define PCI_CB_CARD_BUS                 0x19    /* CardBus bus number                                   */
#define PCI_CB_SUBORDINATE_BUS          0x1a    /* Subordinate bus number                               */
#define PCI_CB_LATENCY_TIMER            0x1b    /* CardBus latency timer                                */
#define PCI_CB_MEMORY_BASE_0            0x1c
#define PCI_CB_MEMORY_LIMIT_0           0x20
#define PCI_CB_MEMORY_BASE_1            0x24
#define PCI_CB_MEMORY_LIMIT_1           0x28
#define PCI_CB_IO_BASE_0                0x2c
#define PCI_CB_IO_BASE_0_HI             0x2e
#define PCI_CB_IO_LIMIT_0               0x30
#define PCI_CB_IO_LIMIT_0_HI            0x32
#define PCI_CB_IO_BASE_1                0x34
#define PCI_CB_IO_BASE_1_HI             0x36
#define PCI_CB_IO_LIMIT_1               0x38
#define PCI_CB_IO_LIMIT_1_HI            0x3a
#define PCI_CB_IO_RANGE_MASK            (~0x03UL)
                                                /* 0x3c-0x3d are same as for htype 0                    */
#define PCI_CB_BRIDGE_CONTROL           0x3e
#define PCI_CB_BRIDGE_CTL_PARITY        0x01    /* Similar to standard bridge control register          */
#define PCI_CB_BRIDGE_CTL_SERR          0x02
#define PCI_CB_BRIDGE_CTL_ISA           0x04
#define PCI_CB_BRIDGE_CTL_VGA           0x08
#define PCI_CB_BRIDGE_CTL_MASTER_ABORT  0x20
#define PCI_CB_BRIDGE_CTL_CB_RESET      0x40    /* CardBus reset                                        */
#define PCI_CB_BRIDGE_CTL_16BIT_INT     0x80    /* Enable interrupt for 16-bit cards                    */
#define PCI_CB_BRIDGE_CTL_PREFETCH_MEM0 0x100   /* Prefetch enable for both memory regions              */
#define PCI_CB_BRIDGE_CTL_PREFETCH_MEM1 0x200
#define PCI_CB_BRIDGE_CTL_POST_WRITES   0x400

#define PCI_CB_SUBSYSTEM_VENDOR_ID      0x40
#define PCI_CB_SUBSYSTEM_ID             0x42
#define PCI_CB_LEGACY_MODE_BASE         0x44    /* 16-bit PC Card legacy mode base address (ExCa)       */
                                                /* 0x48-0x7f reserved                                   */
/*********************************************************************************************************
  Capability lists
*********************************************************************************************************/
#define PCI_CAP_LIST_ID             0           /* Capability ID                                        */
#define PCI_CAP_ID_PM               0x01        /* Power Management                                     */
#define PCI_CAP_ID_AGP              0x02        /* Accelerated Graphics Port                            */
#define PCI_CAP_ID_VPD              0x03        /* Vital Product Data                                   */
#define PCI_CAP_ID_SLOTID           0x04        /* Slot Identification                                  */
#define PCI_CAP_ID_MSI              0x05        /* Message Signaled Interrupts                          */
#define PCI_CAP_ID_CHSWP            0x06        /* CompactPCI HotSwap                                   */
#define PCI_CAP_ID_PCIX             0x07        /* PCI-X                                                */
#define PCI_CAP_ID_HT               0x08        /* HyperTransport                                       */
#define PCI_CAP_ID_VNDR             0x09        /* Vendor Specific                                      */
#define PCI_CAP_ID_DBG              0x0A        /* Debug port                                           */
#define PCI_CAP_ID_CCRC             0x0B        /* CompactPCI Central Resource Control                  */
#define PCI_CAP_ID_SHPC             0x0C        /* PCI Standard Hot-Plug Controller                     */
#define PCI_CAP_ID_SSVID            0x0D        /* Bridge subsystem vendor/device ID                    */
#define PCI_CAP_ID_AGP3             0x0E        /* AGP Target PCI-PCI bridge                            */
#define PCI_CAP_ID_SECDEV           0x0F        /* Secure Device                                        */
#define PCI_CAP_ID_EXP              0x10        /* PCI Express                                          */
#define PCI_CAP_ID_MSIX             0x11        /* MSI-X                                                */
#define PCI_CAP_ID_SATA             0x12        /* SATA Data/Index Conf                                 */
#define PCI_CAP_ID_AF               0x13        /* PCI Advanced Features                                */
#define PCI_CAP_ID_EA               0x14        /* PCI Enhanced Allocation                              */
#define PCI_CAP_ID_MAX              PCI_CAP_ID_EA
#define PCI_CAP_LIST_NEXT           1           /* Next capability in the list                          */
#define PCI_CAP_FLAGS               2           /* Capability defined flags (16 bits)                   */
#define PCI_CAP_SIZEOF              4

/*********************************************************************************************************
  Capabilities residing in the PCI Express extended configuration space
*********************************************************************************************************/
#define PCI_EXT_CAP_ID(header)      (header & 0x0000ffff)
#define PCI_EXT_CAP_VER(header)     ((header >> 16) & 0xf)
#define PCI_EXT_CAP_NEXT(header)    ((header >> 20) & 0xffc)

#define PCI_EXT_CAP_ID_AER          0x01        /* Advanced Error Reporting                             */
#define PCI_EXT_CAP_ID_VC           0x02        /* Virtual Channel                                      */
#define PCI_EXT_CAP_ID_DSN          0x03        /* Device Serial Number                                 */
#define PCI_EXT_CAP_ID_PB           0x04        /* Power Budgeting                                      */
#define PCI_EXT_CAP_ID_RCLINK       0x05        /* Root Complex Link Declaration                        */
#define PCI_EXT_CAP_ID_RCILINK      0x06        /* Root Complex Internal Link Declaration               */
#define PCI_EXT_CAP_ID_RCECOLL      0x07        /* Root Complex Event Collector                         */
#define PCI_EXT_CAP_ID_MFVC         0x08        /* Multi-Function Virtual Channel                       */
#define PCI_EXT_CAP_ID_VC2          0x09        /* Virtual Channel (2nd ID)                             */
#define PCI_EXT_CAP_ID_RBCB         0x0a        /* Root Bridge Control Block                            */
#define PCI_EXT_CAP_ID_VNDR         0x0b        /* Vendor specific                                      */
#define PCI_EXT_CAP_ID_ACS          0x0d        /* Access Controls                                      */
#define PCI_EXT_CAP_ID_ARI          0x0e        /* Alternative Routing-ID Interpretation                */
#define PCI_EXT_CAP_ID_ATS          0x0f        /* Address Translation Service                          */
#define PCI_EXT_CAP_ID_SRIOV        0x10        /* Single Root I/O Virtualization                       */
#define PCI_EXT_CAP_ID_PRI          0x13        /* Page Request Interface                               */
#define PCI_EXT_CAP_ID_TPH          0x17        /* Transaction processing hints                         */
#define PCI_EXT_CAP_ID_LTR          0x18        /* Latency Tolerance Reporting                          */
#define PCI_EXT_CAP_ID_PASID        0x1b        /* Process Address Space ID                             */
#define PCI_EXT_CAP_ID_L1PM         0x1e        /* L1 PM Substates                                      */

/*********************************************************************************************************
  Definitions of capabilities
*********************************************************************************************************/
/*********************************************************************************************************
  Power Management Registers
*********************************************************************************************************/
#define PCI_PM_PMC                  2           /* PM Capabilities Register                             */
#define PCI_PM_CAP_VER_MASK         0x0007      /* Version                                              */
#define PCI_PM_CAP_PME_CLOCK        0x0008      /* PME clock required                                   */
#define PCI_PM_CAP_RESERVED         0x0010      /* Reserved field                                       */
#define PCI_PM_CAP_DSI              0x0020      /* Device specific initialization                       */
#define PCI_PM_CAP_AUX_POWER        0x01C0      /* Auxiliary power support mask                         */
#define PCI_PM_CAP_D1               0x0200      /* D1 power state support                               */
#define PCI_PM_CAP_D2               0x0400      /* D2 power state support                               */
#define PCI_PM_CAP_PME              0x0800      /* PME pin supported                                    */
#define PCI_PM_CAP_PME_MASK         0xF800      /* PME Mask of all supported states                     */
#define PCI_PM_CAP_PME_D0           0x0800      /* PME# from D0                                         */
#define PCI_PM_CAP_PME_D1           0x1000      /* PME# from D1                                         */
#define PCI_PM_CAP_PME_D2           0x2000      /* PME# from D2                                         */
#define PCI_PM_CAP_PME_D3_HOT       0x4000      /* PME# from D3 (hot)                                   */
#define PCI_PM_CAP_PME_D3_COLD      0x8000      /* PME# from D3 (cold)                                  */
#define PCI_PM_CAP_PME_SHIFT        11          /* Start of the PME Mask in PMC */
#define PCI_PM_CTRL                 4           /* PM control and status register                       */
#define PCI_PM_CTRL_STATE_MASK      0x0003      /* Current power state (D0 to D3)                       */
#define PCI_PM_CTRL_NO_SOFT_RESET   0x0008      /* No reset for D3hot->D0                               */
#define PCI_PM_CTRL_PME_ENABLE      0x0100      /* PME pin enable                                       */
#define PCI_PM_CTRL_DATA_SEL_MASK   0x1e00      /* Data select (??)                                     */
#define PCI_PM_CTRL_DATA_SCALE_MASK 0x6000      /* Data scale (??)                                      */
#define PCI_PM_CTRL_PME_STATUS      0x8000      /* PME pin status                                       */
#define PCI_PM_PPB_EXTENSIONS       6           /* PPB support extensions (??)                          */
#define PCI_PM_PPB_B2_B3            0x40        /* Stop clock when in D3hot (??)                        */
#define PCI_PM_BPCC_ENABLE          0x80        /* Bus power/clock control enable (??)                  */
#define PCI_PM_DATA_REGISTER        7           /* (??)                                                 */
#define PCI_PM_SIZEOF               8

/*********************************************************************************************************
  AGP registers
*********************************************************************************************************/
#define PCI_AGP_VERSION             2           /* BCD version number                                   */
#define PCI_AGP_RFU                 3           /* Rest of capability flags                             */
#define PCI_AGP_STATUS              4           /* Status register                                      */
#define PCI_AGP_STATUS_RQ_MASK      0xff000000  /* Maximum number of requests - 1                       */
#define PCI_AGP_STATUS_ISOCH        0x10000     /* Isochronous transactions supported                   */
#define PCI_AGP_STATUS_ARQSZ_MASK   0xe000      /* log2(optimum async req size in bytes) - 4            */
#define PCI_AGP_STATUS_CAL_MASK     0x1c00      /* Calibration cycle timing                             */
#define PCI_AGP_STATUS_SBA          0x0200      /* Sideband addressing supported                        */
#define PCI_AGP_STATUS_ITA_COH      0x0100      /* In-aperture accesses always coherent                 */
#define PCI_AGP_STATUS_GART64       0x0080      /* 64-bit GART entries supported                        */
#define PCI_AGP_STATUS_HTRANS       0x0040      /* If 0, host CPU accesses thru aperture                */
#define PCI_AGP_STATUS_64BIT        0x0020      /* 64-bit addressing supported                          */
#define PCI_AGP_STATUS_FW           0x0010      /* FW transfers supported                               */
#define PCI_AGP_STATUS_AGP3         0x0008      /* AGP3 mode supported                                  */
#define PCI_AGP_STATUS_RATE4        0x0004      /* 4x transfer rate supported                           */
#define PCI_AGP_STATUS_RATE2        0x0002      /* 2x transfer rate supported                           */
#define PCI_AGP_STATUS_RATE1        0x0001      /* 1x transfer rate supported                           */
#define PCI_AGP_COMMAND             8           /* Control register                                     */
#define PCI_AGP_COMMAND_RQ_MASK     0xff000000  /* Master: Maximum number of requests                   */
#define PCI_AGP_COMMAND_ARQSZ_MASK  0xe000      /* log2(optimum async req size in bytes) - 4            */
#define PCI_AGP_COMMAND_CAL_MASK    0x1c00      /* Calibration cycle timing                             */
#define PCI_AGP_COMMAND_SBA         0x0200      /* Sideband addressing enabled                          */
#define PCI_AGP_COMMAND_AGP         0x0100      /* Allow processing of AGP transactions                 */
#define PCI_AGP_COMMAND_GART64      0x0080      /* 64-bit GART entries enabled                          */
#define PCI_AGP_COMMAND_64BIT       0x0020      /* Allow processing of 64-bit addresses                 */
#define PCI_AGP_COMMAND_FW          0x0010      /* Force FW transfers                                   */
#define PCI_AGP_COMMAND_RATE4       0x0004      /* Use 4x rate (RFU in AGP3 mode)                       */
#define PCI_AGP_COMMAND_RATE2       0x0002      /* Use 2x rate (8x in AGP3 mode)                        */
#define PCI_AGP_COMMAND_RATE1       0x0001      /* Use 1x rate (4x in AGP3 mode)                        */
#define PCI_AGP_SIZEOF              12

/*********************************************************************************************************
  Vital Product Data
*********************************************************************************************************/
#define PCI_VPD_ADDR                2           /* Address to access (15 bits!)                         */
#define PCI_VPD_ADDR_MASK           0x7fff      /* Address mask                                         */
#define PCI_VPD_ADDR_F              0x8000      /* Write 0, 1 indicates completion                      */
#define PCI_VPD_DATA                4           /* 32-bits of data returned here                        */

/*********************************************************************************************************
  Slot Identification
*********************************************************************************************************/
#define PCI_SID_ESR                 2           /* Expansion Slot Register                              */
#define PCI_SID_ESR_NSLOTS          0x1f        /* Number of expansion slots available                  */
#define PCI_SID_ESR_FIC             0x20        /* First In Chassis Flag                                */
#define PCI_SID_CHASSIS_NR          3           /* Chassis Number                                       */

/*********************************************************************************************************
  Message Signaled Interrupts registers
*********************************************************************************************************/
#define PCI_MSI_FLAGS               2           /* Various flags                                        */
#define PCI_MSI_FLAGS_MASK_BIT      0x100       /* interrupt masking & reporting supported              */
#define PCI_MSI_FLAGS_64BIT         0x080       /* 64-bit addresses allowed                             */
#define PCI_MSI_FLAGS_QSIZE         0x070       /* Message queue size configured                        */
#define PCI_MSI_FLAGS_QMASK         0x00e       /* Maximum queue size available                         */
#define PCI_MSI_FLAGS_ENABLE        0x001       /* MSI feature enabled                                  */
#define PCI_MSI_RFU                 3           /* Rest of capability flags                             */
#define PCI_MSI_ADDRESS_LO          4           /* Lower 32 bits                                        */
#define PCI_MSI_ADDRESS_HI          8           /* Upper 32 bits (if PCI_MSI_FLAGS_64BIT set)           */
#define PCI_MSI_DATA_32             8           /* 16 bits of data for 32-bit devices                   */
#define PCI_MSI_DATA_64             12          /* 16 bits of data for 64-bit devices                   */
#define PCI_MSI_MASK_BIT_32         12          /* per-vector masking for 32-bit devices                */
#define PCI_MSI_MASK_BIT_64         16          /* per-vector masking for 64-bit devices                */
#define PCI_MSI_PENDING_32          16          /* per-vector interrupt pending for 32-bit devices      */
#define PCI_MSI_PENDING_64          20          /* per-vector interrupt pending for 64-bit devices      */

/*********************************************************************************************************
  MSI-X registers
*********************************************************************************************************/
#define PCI_MSIX_FLAGS              2           /* Message Control                                      */
#define PCI_MSIX_FLAGS_QSIZE        0x07FF      /* Table size                                           */
#define PCI_MSIX_FLAGS_MASKALL      0x4000      /* Mask all vectors for this function                   */
#define PCI_MSIX_FLAGS_ENABLE       0x8000      /* MSI-X enable                                         */
#define PCI_MSIX_TABLE              4           /* Table offset                                         */
#define PCI_MSIX_TABLE_BIR          0x00000007  /* BAR index                                            */
#define PCI_MSIX_TABLE_OFFSET       0xfffffff8  /* Offset into specified BAR                            */
#define PCI_MSIX_PBA                8           /* Pending Bit Array offset                             */
#define PCI_MSIX_PBA_BIR            0x00000007  /* BAR index                                            */
#define PCI_MSIX_PBA_OFFSET         0xfffffff8  /* Offset into specified BAR                            */
#define PCI_MSIX_FLAGS_BIRMASK      PCI_MSIX_PBA_BIR
#define PCI_CAP_MSIX_SIZEOF         12          /* size of MSIX registers                               */

/*********************************************************************************************************
  MSI-X Table entry format
*********************************************************************************************************/
#define PCI_MSIX_ENTRY_SIZE             16
#define PCI_MSIX_ENTRY_LOWER_ADDR       0
#define PCI_MSIX_ENTRY_UPPER_ADDR       4
#define PCI_MSIX_ENTRY_DATA             8
#define PCI_MSIX_ENTRY_VECTOR_CTRL      12
#define PCI_MSIX_ENTRY_CTRL_MASKBIT     1

/*********************************************************************************************************
  PCI-X
*********************************************************************************************************/
#define PCI_PCIX_COMMAND            2           /* Command register offset                              */
#define PCI_PCIX_COMMAND_DPERE      0x0001      /* Data Parity Error Recover Enable                     */
#define PCI_PCIX_COMMAND_ERO        0x0002      /* Enable Relaxed Ordering                              */
                                                /* Maximum Memory Read Byte Count                       */
#define PCI_PCIX_COMMAND_MAX_MEM_READ_BYTE_COUNT                0x000c
#define PCI_PCIX_COMMAND_MAX_OUTSTANDING_SPLIT_TRANS            0x0070
#define PCI_PCIX_COMMAND_RESERVED                               0xf80
#define PCI_PCIX_STATUS             4           /* Status register offset                               */
#define PCI_PCIX_STATUS_FUNCTION    0x00000007
#define PCI_PCIX_STATUS_DEVICE      0x000000f8
#define PCI_PCIX_STATUS_BUS         0x0000ff00
#define PCI_PCIX_STATUS_64BIT       0x00010000
#define PCI_PCIX_STATUS_133MHZ      0x00020000
                                                /* Split Completion Discarded                           */
#define PCI_PCIX_STATUS_SC_DISCARDED                            0x00040000
                                                /* Unexpected Split Completion                          */
#define PCI_PCIX_STATUS_UNEXPECTED_SC                           0x00080000
                                                /* 0 = simple device, 1 = bridge device                 */
#define PCI_PCIX_STATUS_DEVICE_COMPLEXITY                       0x00100000
                                                /* 0 = 512 bytes, 1 = 1024, 2 = 2048, 3 = 4096          */
#define PCI_PCIX_STATUS_DESIGNED_MAX_MEM_READ_BYTE_COUNT        0x00600000
#define PCI_PCIX_STATUS_DESIGNED_MAX_OUTSTANDING_SPLIT_TRANS    0x03800000
#define PCI_PCIX_STATUS_DESIGNED_MAX_CUMULATIVE_READ_SIZE       0x1c000000
                                                /* Received Split Completion Error Message              */
#define PCI_PCIX_STATUS_RCVD_SC_ERR_MESS                        0x20000000
#define PCI_PCIX_STATUS_266MHZ      0x40000000  /* 266 MHz capable                                      */
#define PCI_PCIX_STATUS_533MHZ      0x80000000  /* 533 MHz capable                                      */
#define PCI_PCIX_SIZEOF             4

/*********************************************************************************************************
  PCI-X Bridges
*********************************************************************************************************/
                                                /* Secondary bus status register offset                 */
#define PCI_PCIX_BRIDGE_SEC_STATUS                          2
#define PCI_PCIX_BRIDGE_SEC_STATUS_64BIT                    0x0001
#define PCI_PCIX_BRIDGE_SEC_STATUS_133MHZ                   0x0002
                                                /* Split Completion Discarded on secondary bus          */
#define PCI_PCIX_BRIDGE_SEC_STATUS_SC_DISCARDED             0x0004
                                                /* Unexpected Split Completion on secondary bus         */
#define PCI_PCIX_BRIDGE_SEC_STATUS_UNEXPECTED_SC            0x0008
                                                /* Split Completion Overrun on secondary bus            */
#define PCI_PCIX_BRIDGE_SEC_STATUS_SC_OVERRUN               0x0010
#define PCI_PCIX_BRIDGE_SEC_STATUS_SPLIT_REQUEST_DELAYED    0x0020
#define PCI_PCIX_BRIDGE_SEC_STATUS_CLOCK_FREQ               0x01c0
#define PCI_PCIX_BRIDGE_SEC_STATUS_RESERVED                 0xfe00
                                                /* Primary bus status register offset                   */
#define PCI_PCIX_BRIDGE_STATUS                              4
#define PCI_PCIX_BRIDGE_STATUS_FUNCTION                     0x00000007
#define PCI_PCIX_BRIDGE_STATUS_DEVICE                       0x000000f8
#define PCI_PCIX_BRIDGE_STATUS_BUS                          0x0000ff00
#define PCI_PCIX_BRIDGE_STATUS_64BIT                        0x00010000
#define PCI_PCIX_BRIDGE_STATUS_133MHZ                       0x00020000
                                                /* Split Completion Discarded                           */
#define PCI_PCIX_BRIDGE_STATUS_SC_DISCARDED                 0x00040000
                                                /* Unexpected Split Completion                          */
#define PCI_PCIX_BRIDGE_STATUS_UNEXPECTED_SC                0x00080000
                                                /* Split Completion Overrun                             */
#define PCI_PCIX_BRIDGE_STATUS_SC_OVERRUN                   0x00100000
#define PCI_PCIX_BRIDGE_STATUS_SPLIT_REQUEST_DELAYED        0x00200000
#define PCI_PCIX_BRIDGE_STATUS_RESERVED                     0xffc00000
                                                /* Upstream Split Transaction Register offset           */
#define PCI_PCIX_BRIDGE_UPSTREAM_SPLIT_TRANS_CTRL           8
                                                /* Downstream Split Transaction Register offset         */
#define PCI_PCIX_BRIDGE_DOWNSTREAM_SPLIT_TRANS_CTRL         12
#define PCI_PCIX_BRIDGE_STR_CAPACITY                        0x0000ffff
#define PCI_PCIX_BRIDGE_STR_COMMITMENT_LIMIT                0xffff0000
#define PCI_PCIX_BRIDGE_SIZEOF                              12

/*********************************************************************************************************
  HyperTransport (as of spec rev. 2.00)
*********************************************************************************************************/
#define PCI_HT_CMD                  2           /* Command Register                                     */
#define PCI_HT_CMD_TYP_HI           0xe000      /* Capability Type high part                            */
#define PCI_HT_CMD_TYP_HI_PRI       0x0000      /* Slave or Primary Interface                           */
#define PCI_HT_CMD_TYP_HI_SEC       0x2000      /* Host or Secondary Interface                          */
#define PCI_HT_CMD_TYP              0xf800      /* Capability Type                                      */
#define PCI_HT_CMD_TYP_SW           0x4000      /* Switch                                               */
#define PCI_HT_CMD_TYP_IDC          0x8000      /* Interrupt Discovery and Configuration                */
#define PCI_HT_CMD_TYP_RID          0x8800      /* Revision ID                                          */
#define PCI_HT_CMD_TYP_UIDC         0x9000      /* UnitID Clumping                                      */
#define PCI_HT_CMD_TYP_ECSA         0x9800      /* Extended Configuration Space Access                  */
#define PCI_HT_CMD_TYP_AM           0xa000      /* Address Mapping                                      */
#define PCI_HT_CMD_TYP_MSIM         0xa800      /* MSI Mapping                                          */
#define PCI_HT_CMD_TYP_DR           0xb000      /* DirectRoute                                          */
#define PCI_HT_CMD_TYP_VCS          0xb800      /* VCSet                                                */
#define PCI_HT_CMD_TYP_RM           0xc000      /* Retry Mode                                           */
#define PCI_HT_CMD_TYP_X86          0xc800      /* X86 (reserved)                                       */

/*********************************************************************************************************
  Link Control Register
*********************************************************************************************************/
#define PCI_HT_LCTR_CFLE            0x0002      /* CRC Flood Enable                                     */
#define PCI_HT_LCTR_CST             0x0004      /* CRC Start Test                                       */
#define PCI_HT_LCTR_CFE             0x0008      /* CRC Force Error                                      */
#define PCI_HT_LCTR_LKFAIL          0x0010      /* Link Failure                                         */
#define PCI_HT_LCTR_INIT            0x0020      /* Initialization Complete                              */
#define PCI_HT_LCTR_EOC             0x0040      /* End of Chain                                         */
#define PCI_HT_LCTR_TXO             0x0080      /* Transmitter Off                                      */
#define PCI_HT_LCTR_CRCERR          0x0f00      /* CRC Error                                            */
#define PCI_HT_LCTR_ISOCEN          0x1000      /* Isochronous Flow Control Enable                      */
#define PCI_HT_LCTR_LSEN            0x2000      /* LDTSTOP# Tristate Enable                             */
#define PCI_HT_LCTR_EXTCTL          0x4000      /* Extended CTL Time                                    */
#define PCI_HT_LCTR_64B             0x8000      /* 64-bit Addressing Enable                             */

/*********************************************************************************************************
  Link Configuration Register
*********************************************************************************************************/
#define PCI_HT_LCNF_MLWI            0x0007      /* Max Link Width In                                    */
#define PCI_HT_LCNF_LW_8B           0x0         /* Link Width 8 bits                                    */
#define PCI_HT_LCNF_LW_16B          0x1         /* Link Width 16 bits                                   */
#define PCI_HT_LCNF_LW_32B          0x3         /* Link Width 32 bits                                   */
#define PCI_HT_LCNF_LW_2B           0x4         /* Link Width 2 bits                                    */
#define PCI_HT_LCNF_LW_4B           0x5         /* Link Width 4 bits                                    */
#define PCI_HT_LCNF_LW_NC           0x7         /* Link physically not connected                        */
#define PCI_HT_LCNF_DFI             0x0008      /* Doubleword Flow Control In                           */
#define PCI_HT_LCNF_MLWO            0x0070      /* Max Link Width Out                                   */
#define PCI_HT_LCNF_DFO             0x0080      /* Doubleword Flow Control Out                          */
#define PCI_HT_LCNF_LWI             0x0700      /* Link Width In                                        */
#define PCI_HT_LCNF_DFIE            0x0800      /* Doubleword Flow Control In Enable                    */
#define PCI_HT_LCNF_LWO             0x7000      /* Link Width Out                                       */
#define PCI_HT_LCNF_DFOE            0x8000      /* Doubleword Flow Control Out Enable                   */

/*********************************************************************************************************
  Revision ID Register
*********************************************************************************************************/
#define PCI_HT_RID_MIN              0x1f        /* Minor Revision                                       */
#define PCI_HT_RID_MAJ              0xe0        /* Major Revision                                       */

/*********************************************************************************************************
  Link Frequency/Error Register
*********************************************************************************************************/
#define PCI_HT_LFRER_FREQ           0x0f        /* Transmitter Clock Frequency                          */
#define PCI_HT_LFRER_200            0x00        /* 200MHz                                               */
#define PCI_HT_LFRER_300            0x01        /* 300MHz                                               */
#define PCI_HT_LFRER_400            0x02        /* 400MHz                                               */
#define PCI_HT_LFRER_500            0x03        /* 500MHz                                               */
#define PCI_HT_LFRER_600            0x04        /* 600MHz                                               */
#define PCI_HT_LFRER_800            0x05        /* 800MHz                                               */
#define PCI_HT_LFRER_1000           0x06        /* 1.0GHz                                               */
#define PCI_HT_LFRER_1200           0x07        /* 1.2GHz                                               */
#define PCI_HT_LFRER_1400           0x08        /* 1.4GHz                                               */
#define PCI_HT_LFRER_1600           0x09        /* 1.6GHz                                               */
#define PCI_HT_LFRER_VEND           0x0f        /* Vendor-Specific                                      */
#define PCI_HT_LFRER_ERR            0xf0        /* Link Error                                           */
#define PCI_HT_LFRER_PROT           0x10        /* Protocol Error                                       */
#define PCI_HT_LFRER_OV             0x20        /* Overflow Error                                       */
#define PCI_HT_LFRER_EOC            0x40        /* End of Chain Error                                   */
#define PCI_HT_LFRER_CTLT           0x80        /* CTL Timeout                                          */

/*********************************************************************************************************
  Link Frequency Capability Register
*********************************************************************************************************/
#define PCI_HT_LFCAP_200            0x0001      /* 200MHz                                               */
#define PCI_HT_LFCAP_300            0x0002      /* 300MHz                                               */
#define PCI_HT_LFCAP_400            0x0004      /* 400MHz                                               */
#define PCI_HT_LFCAP_500            0x0008      /* 500MHz                                               */
#define PCI_HT_LFCAP_600            0x0010      /* 600MHz                                               */
#define PCI_HT_LFCAP_800            0x0020      /* 800MHz                                               */
#define PCI_HT_LFCAP_1000           0x0040      /* 1.0GHz                                               */
#define PCI_HT_LFCAP_1200           0x0080      /* 1.2GHz                                               */
#define PCI_HT_LFCAP_1400           0x0100      /* 1.4GHz                                               */
#define PCI_HT_LFCAP_1600           0x0200      /* 1.6GHz                                               */
#define PCI_HT_LFCAP_VEND           0x8000      /* Vendor-Specific                                      */

/*********************************************************************************************************
  Feature Register
*********************************************************************************************************/
#define PCI_HT_FTR_ISOCFC           0x0001      /* Isochronous Flow Control Mode                        */
#define PCI_HT_FTR_LDTSTOP          0x0002      /* LDTSTOP# Supported                                   */
#define PCI_HT_FTR_CRCTM            0x0004      /* CRC Test Mode                                        */
#define PCI_HT_FTR_ECTLT            0x0008      /* Extended CTL Time Required                           */
#define PCI_HT_FTR_64BA             0x0010      /* 64-bit Addressing                                    */
#define PCI_HT_FTR_UIDRD            0x0020      /* UnitID Reorder Disable                               */

/*********************************************************************************************************
  Error Handling Register
*********************************************************************************************************/
#define PCI_HT_EH_PFLE              0x0001      /* Protocol Error Flood Enable                          */
#define PCI_HT_EH_OFLE              0x0002      /* Overflow Error Flood Enable                          */
#define PCI_HT_EH_PFE               0x0004      /* Protocol Error Fatal Enable                          */
#define PCI_HT_EH_OFE               0x0008      /* Overflow Error Fatal Enable                          */
#define PCI_HT_EH_EOCFE             0x0010      /* End of Chain Error Fatal Enable                      */
#define PCI_HT_EH_RFE               0x0020      /* Response Error Fatal Enable                          */
#define PCI_HT_EH_CRCFE             0x0040      /* CRC Error Fatal Enable                               */
#define PCI_HT_EH_SERRFE            0x0080      /* System Error Fatal Enable                            */
#define PCI_HT_EH_CF                0x0100      /* Chain Fail                                           */
#define PCI_HT_EH_RE                0x0200      /* Response Error                                       */
#define PCI_HT_EH_PNFE              0x0400      /* Protocol Error Nonfatal Enable                       */
#define PCI_HT_EH_ONFE              0x0800      /* Overflow Error Nonfatal Enable                       */
#define PCI_HT_EH_EOCNFE            0x1000      /* End of Chain Error Nonfatal Enable                   */
#define PCI_HT_EH_RNFE              0x2000      /* Response Error Nonfatal Enable                       */
#define PCI_HT_EH_CRCNFE            0x4000      /* CRC Error Nonfatal Enable                            */
#define PCI_HT_EH_SERRNFE           0x8000      /* System Error Nonfatal Enable                         */

/*********************************************************************************************************
  HyperTransport: Slave or Primary Interface
*********************************************************************************************************/
#define PCI_HT_PRI_CMD              2           /* Command Register                                     */
#define PCI_HT_PRI_CMD_BUID         0x001f      /* Base UnitID                                          */
#define PCI_HT_PRI_CMD_UC           0x03e0      /* Unit Count                                           */
#define PCI_HT_PRI_CMD_MH           0x0400      /* Master Host                                          */
#define PCI_HT_PRI_CMD_DD           0x0800      /* Default Direction                                    */
#define PCI_HT_PRI_CMD_DUL          0x1000      /* Drop on Uninitialized Link                           */

#define PCI_HT_PRI_LCTR0            4           /* Link Control 0 Register                              */
#define PCI_HT_PRI_LCNF0            6           /* Link Config 0 Register                               */
#define PCI_HT_PRI_LCTR1            8           /* Link Control 1 Register                              */
#define PCI_HT_PRI_LCNF1            10          /* Link Config 1 Register                               */
#define PCI_HT_PRI_RID              12          /* Revision ID Register                                 */
#define PCI_HT_PRI_LFRER0           13          /* Link Frequency/Error 0 Register                      */
#define PCI_HT_PRI_LFCAP0           14          /* Link Frequency Capability 0 Register                 */
#define PCI_HT_PRI_FTR              16          /* Feature Register                                     */
#define PCI_HT_PRI_LFRER1           17          /* Link Frequency/Error 1 Register                      */
#define PCI_HT_PRI_LFCAP1           18          /* Link Frequency Capability 1 Register                 */
#define PCI_HT_PRI_ES               20          /* Enumeration Scratchpad Register                      */
#define PCI_HT_PRI_EH               22          /* Error Handling Register                              */
#define PCI_HT_PRI_MBU              24          /* Memory Base Upper Register                           */
#define PCI_HT_PRI_MLU              25          /* Memory Limit Upper Register                          */
#define PCI_HT_PRI_BN               26          /* Bus Number Register                                  */
#define PCI_HT_PRI_SIZEOF           28

/*********************************************************************************************************
  HyperTransport: Host or Secondary Interface
*********************************************************************************************************/
#define PCI_HT_SEC_CMD              2           /* Command Register                                     */
#define PCI_HT_SEC_CMD_WR           0x0001      /* Warm Reset                                           */
#define PCI_HT_SEC_CMD_DE           0x0002      /* Double-Ended                                         */
#define PCI_HT_SEC_CMD_DN           0x0076      /* Device Number                                        */
#define PCI_HT_SEC_CMD_CS           0x0080      /* Chain Side                                           */
#define PCI_HT_SEC_CMD_HH           0x0100      /* Host Hide                                            */
#define PCI_HT_SEC_CMD_AS           0x0400      /* Act as Slave                                         */
#define PCI_HT_SEC_CMD_HIECE        0x0800      /* Host Inbound End of Chain Error                      */
#define PCI_HT_SEC_CMD_DUL          0x1000      /* Drop on Uninitialized Link                           */

#define PCI_HT_SEC_LCTR             4           /* Link Control Register                                */
#define PCI_HT_SEC_LCNF             6           /* Link Config Register                                 */
#define PCI_HT_SEC_RID              8           /* Revision ID Register                                 */
#define PCI_HT_SEC_LFRER            9           /* Link Frequency/Error Register                        */
#define PCI_HT_SEC_LFCAP            10          /* Link Frequency Capability Register                   */
#define PCI_HT_SEC_FTR              12          /* Feature Register                                     */
#define PCI_HT_SEC_FTR_EXTRS        0x0100      /* Extended Register Set                                */
#define PCI_HT_SEC_FTR_UCNFE        0x0200      /* Upstream Configuration Enable                        */
#define PCI_HT_SEC_ES               16          /* Enumeration Scratchpad Register                      */
#define PCI_HT_SEC_EH               18          /* Error Handling Register                              */
#define PCI_HT_SEC_MBU              20          /* Memory Base Upper Register                           */
#define PCI_HT_SEC_MLU              21          /* Memory Limit Upper Register                          */
#define PCI_HT_SEC_SIZEOF           24

/*********************************************************************************************************
  HyperTransport: Switch
*********************************************************************************************************/
#define PCI_HT_SW_CMD               2           /* Switch Command Register                              */
#define PCI_HT_SW_CMD_VIBERR        0x0080      /* VIB Error                                            */
#define PCI_HT_SW_CMD_VIBFL         0x0100      /* VIB Flood                                            */
#define PCI_HT_SW_CMD_VIBFT         0x0200      /* VIB Fatal                                            */
#define PCI_HT_SW_CMD_VIBNFT        0x0400      /* VIB Nonfatal                                         */
#define PCI_HT_SW_PMASK             4           /* Partition Mask Register                              */
#define PCI_HT_SW_SWINF             8           /* Switch Info Register                                 */
#define PCI_HT_SW_SWINF_DP          0x0000001f  /* Default Port                                         */
#define PCI_HT_SW_SWINF_EN          0x00000020  /* Enable Decode                                        */
#define PCI_HT_SW_SWINF_CR          0x00000040  /* Cold Reset                                           */
#define PCI_HT_SW_SWINF_PCIDX       0x00000f00  /* Performance Counter Index                            */
#define PCI_HT_SW_SWINF_BLRIDX      0x0003f000  /* Base/Limit Range Index                               */
#define PCI_HT_SW_SWINF_SBIDX       0x00002000  /* Secondary Base Range Index                           */
#define PCI_HT_SW_SWINF_HP          0x00040000  /* Hot Plug                                             */

#define PCI_HT_SW_SWINF_HIDE        0x00080000  /*  Hide Port                                           */
#define PCI_HT_SW_PCD               12          /* Performance Counter Data Register                    */
#define PCI_HT_SW_BLRD              16          /* Base/Limit Range Data Register                       */
#define PCI_HT_SW_SBD               20          /* Secondary Base Data Register                         */
#define PCI_HT_SW_SIZEOF            24

/*********************************************************************************************************
  Counter indices
*********************************************************************************************************/
#define PCI_HT_SW_PC_PCR            0x0         /* Posted Command Receive                               */
#define PCI_HT_SW_PC_NPCR           0x1         /* Nonposted Command Receive                            */
#define PCI_HT_SW_PC_RCR            0x2         /* Response Command Receive                             */
#define PCI_HT_SW_PC_PDWR           0x3         /* Posted DW Receive                                    */
#define PCI_HT_SW_PC_NPDWR          0x4         /* Nonposted DW Receive                                 */
#define PCI_HT_SW_PC_RDWR           0x5         /* Response DW Receive                                  */
#define PCI_HT_SW_PC_PCT            0x6         /* Posted Command Transmit                              */
#define PCI_HT_SW_PC_NPCT           0x7         /* Nonposted Command Transmit                           */
#define PCI_HT_SW_PC_RCT            0x8         /* Response Command Transmit                            */
#define PCI_HT_SW_PC_PDWT           0x9         /* Posted DW Transmit                                   */
#define PCI_HT_SW_PC_NPDWT          0xa         /* Nonposted DW Transmit                                */
#define PCI_HT_SW_PC_RDWT           0xb         /* Response DW Transmit                                 */

/*********************************************************************************************************
  Base/Limit Range indices
*********************************************************************************************************/
#define PCI_HT_SW_BLR_BASE0_LO      0x0         /* Base 0[31:1], Enable                                 */
#define PCI_HT_SW_BLR_BASE0_HI      0x1         /* Base 0 Upper                                         */
#define PCI_HT_SW_BLR_LIM0_LO       0x2         /* Limit 0 Lower                                        */
#define PCI_HT_SW_BLR_LIM0_HI       0x3         /* Limit 0 Upper                                        */

/*********************************************************************************************************
  Secondary Base indices
*********************************************************************************************************/
#define PCI_HT_SW_SB_LO             0x0         /* Secondary Base[31:1], Enable                         */
#define PCI_HT_SW_S0_HI             0x1         /* Secondary Base Upper                                 */

/*********************************************************************************************************
  HyperTransport: Interrupt Discovery and Configuration
*********************************************************************************************************/
#define PCI_HT_IDC_IDX              2           /* Index Register                                       */
#define PCI_HT_IDC_DATA             4           /* Data Register                                        */
#define PCI_HT_IDC_SIZEOF           8

/*********************************************************************************************************
  Register indices
*********************************************************************************************************/
#define PCI_HT_IDC_IDX_LINT         0x01        /* Last Interrupt Register                              */
#define PCI_HT_IDC_LINT             0x00ff0000  /* Last interrupt definition                            */
#define PCI_HT_IDC_IDX_IDR          0x10        /* Interrupt Definition Registers                       */

/*********************************************************************************************************
  Low part (at index)
*********************************************************************************************************/
#define PCI_HT_IDC_IDR_MASK         0x10000001  /* Mask                                                 */
#define PCI_HT_IDC_IDR_POL          0x10000002  /* Polarity                                             */
#define PCI_HT_IDC_IDR_II_2         0x1000001c  /* IntrInfo[4:2]: Message Type                          */
#define PCI_HT_IDC_IDR_II_5         0x10000020  /* IntrInfo[5]: Request EOI                             */
#define PCI_HT_IDC_IDR_II_6         0x00ffffc0  /* IntrInfo[23:6]                                       */
#define PCI_HT_IDC_IDR_II_24        0xff000000  /* IntrInfo[31:24]                                      */

/*********************************************************************************************************
  High part (at index + 1)
*********************************************************************************************************/
#define PCI_HT_IDC_IDR_II_32        0x00ffffff  /* IntrInfo[55:32]                                      */
#define PCI_HT_IDC_IDR_PASSPW       0x40000000  /* PassPW setting for messages                          */
#define PCI_HT_IDC_IDR_WEOI         0x80000000  /* Waiting for EOI                                      */

/*********************************************************************************************************
  HyperTransport: Revision ID
*********************************************************************************************************/
#define PCI_HT_RID_RID              2           /* Revision Register                                    */
#define PCI_HT_RID_SIZEOF           4

/*********************************************************************************************************
  HyperTransport: UnitID Clumping
*********************************************************************************************************/
#define PCI_HT_UIDC_CS              4           /* Clumping Support Register                            */
#define PCI_HT_UIDC_CE              8           /* Clumping Enable Register                             */
#define PCI_HT_UIDC_SIZEOF          12

/*********************************************************************************************************
  HyperTransport: Extended Configuration Space Access
*********************************************************************************************************/
#define PCI_HT_ECSA_ADDR            4           /* Configuration Address Register                       */
#define PCI_HT_ECSA_ADDR_REG        0x00000ffc  /* Register                                             */
#define PCI_HT_ECSA_ADDR_FUN        0x00007000  /* Function                                             */
#define PCI_HT_ECSA_ADDR_DEV        0x000f1000  /* Device                                               */
#define PCI_HT_ECSA_ADDR_BUS        0x0ff00000  /* Bus Number                                           */
#define PCI_HT_ECSA_ADDR_TYPE       0x10000000  /* Access Type                                          */
#define PCI_HT_ECSA_DATA            8           /* Configuration Data Register                          */
#define PCI_HT_ECSA_SIZEOF          12

/*********************************************************************************************************
  HyperTransport: Address Mapping
*********************************************************************************************************/
#define PCI_HT_AM_CMD               2           /* Command Register                                     */
#define PCI_HT_AM_CMD_NDMA          0x000f      /* Number of DMA Mappings                               */
#define PCI_HT_AM_CMD_IOSIZ         0x01f0      /* I/O Size                                             */
#define PCI_HT_AM_CMD_MT            0x0600      /* Map Type                                             */
#define PCI_HT_AM_CMD_MT_40B        0x0000      /* 40-bit                                               */
#define PCI_HT_AM_CMD_MT_64B        0x0200      /* 64-bit                                               */

/*********************************************************************************************************
  Window Control Register bits
*********************************************************************************************************/
#define PCI_HT_AM_SBW_CTR_COMP      0x1         /* Compat                                               */
#define PCI_HT_AM_SBW_CTR_NCOH      0x2         /* NonCoherent                                          */
#define PCI_HT_AM_SBW_CTR_ISOC      0x4         /* Isochronous                                          */
#define PCI_HT_AM_SBW_CTR_EN        0x8         /* Enable                                               */

/*********************************************************************************************************
  HyperTransport: 40-bit Address Mapping
*********************************************************************************************************/
#define PCI_HT_AM40_SBNPW           4           /* Secondary Bus Non-Prefetchable Window Register       */
#define PCI_HT_AM40_SBW_BASE        0x000fffff  /* Window Base                                          */
#define PCI_HT_AM40_SBW_CTR         0xf0000000  /* Window Control                                       */
#define PCI_HT_AM40_SBPW            8           /* Secondary Bus Prefetchable Window Register           */
#define PCI_HT_AM40_DMA_PBASE0      12          /* DMA Window Primary Base 0 Register                   */
#define PCI_HT_AM40_DMA_CTR0        15          /* DMA Window Control 0 Register                        */
#define PCI_HT_AM40_DMA_CTR_CTR     0xf0        /* Window Control                                       */
#define PCI_HT_AM40_DMA_SLIM0       16          /* DMA Window Secondary Limit 0 Register                */
#define PCI_HT_AM40_DMA_SBASE0      18          /* DMA Window Secondary Base 0 Register                 */
#define PCI_HT_AM40_SIZEOF          12          /* size is variable: 12 + 8 * NDMA                      */

/*********************************************************************************************************
  HyperTransport: 64-bit Address Mapping
*********************************************************************************************************/
#define PCI_HT_AM64_IDX             4           /* Index Register                                       */
#define PCI_HT_AM64_DATA_LO         8           /* Data Lower Register                                  */
#define PCI_HT_AM64_DATA_HI         12          /* Data Upper Register                                  */
#define PCI_HT_AM64_SIZEOF          16

/*********************************************************************************************************
  Register indices
*********************************************************************************************************/
#define PCI_HT_AM64_IDX_SBNPW       0x00        /* Secondary Bus Non-Prefetchable Window Register       */
#define PCI_HT_AM64_W_BASE_LO       0xfff00000  /* Window Base Lower                                    */
#define PCI_HT_AM64_W_CTR           0x0000000f  /* Window Control                                       */
#define PCI_HT_AM64_IDX_SBPW        0x01        /* Secondary Bus Prefetchable Window Register           */
#define PCI_HT_AM64_IDX_PBNPW       0x02        /* Primary Bus Non-Prefetchable Window Register         */
#define PCI_HT_AM64_IDX_DMAPB0      0x04        /* DMA Window Primary Base 0 Register                   */
#define PCI_HT_AM64_IDX_DMASB0      0x05        /* DMA Window Secondary Base 0 Register                 */
#define PCI_HT_AM64_IDX_DMASL0      0x06        /* DMA Window Secondary Limit 0 Register                */

/*********************************************************************************************************
  HyperTransport: MSI Mapping
*********************************************************************************************************/
#define PCI_HT_MSIM_CMD             2           /* Command Register                                     */
#define PCI_HT_MSIM_CMD_EN          0x0001      /* Mapping Active                                       */
#define PCI_HT_MSIM_CMD_FIXD        0x0002      /* MSI Mapping Address Fixed                            */
#define PCI_HT_MSIM_ADDR_LO         4           /* MSI Mapping Address Lower Register                   */
#define PCI_HT_MSIM_ADDR_HI         8           /* MSI Mapping Address Upper Register                   */
#define PCI_HT_MSIM_SIZEOF          12

/*********************************************************************************************************
  HyperTransport: DirectRoute
*********************************************************************************************************/
#define PCI_HT_DR_CMD               2           /* Command Register                                     */
#define PCI_HT_DR_CMD_NDRS          0x000f      /* Number of DirectRoute Spaces                         */
#define PCI_HT_DR_CMD_IDX           0x01f0      /* Index                                                */
#define PCI_HT_DR_EN                4           /* Enable Vector Register                               */
#define PCI_HT_DR_DATA              8           /* Data Register                                        */
#define PCI_HT_DR_SIZEOF            12

/*********************************************************************************************************
  Register indices
*********************************************************************************************************/
#define PCI_HT_DR_IDX_BASE_LO       0x00        /* DirectRoute Base Lower Register                      */
#define PCI_HT_DR_OTNRD             0x00000001  /* Opposite to Normal Request Direction                 */
#define PCI_HT_DR_BL_LO             0xffffff00  /* Base/Limit Lower                                     */
#define PCI_HT_DR_IDX_BASE_HI       0x01        /* DirectRoute Base Upper Register                      */
#define PCI_HT_DR_IDX_LIMIT_LO      0x02        /* DirectRoute Limit Lower Register                     */
#define PCI_HT_DR_IDX_LIMIT_HI      0x03        /* DirectRoute Limit Upper Register                     */

/*********************************************************************************************************
  HyperTransport: VCSet
*********************************************************************************************************/
#define PCI_HT_VCS_SUP              4           /* VCSets Supported Register                            */
#define PCI_HT_VCS_L1EN             5           /* Link 1 VCSets Enabled Register                       */
#define PCI_HT_VCS_L0EN             6           /* Link 0 VCSets Enabled Register                       */
#define PCI_HT_VCS_SBD              8           /* Stream Bucket Depth Register                         */
#define PCI_HT_VCS_SINT             9           /* Stream Interval Register                             */
#define PCI_HT_VCS_SSUP             10          /* Number of Streaming VCs Supported Register           */
#define PCI_HT_VCS_SSUP_0           0x00        /* Streaming VC 0                                       */
#define PCI_HT_VCS_SSUP_3           0x01        /* Streaming VCs 0-3                                    */
#define PCI_HT_VCS_SSUP_15          0x02        /* Streaming VCs 0-15                                   */
#define PCI_HT_VCS_NFCBD            12          /* Non-FC Bucket Depth Register                         */
#define PCI_HT_VCS_NFCINT           13          /* Non-FC Bucket Interval Register                      */
#define PCI_HT_VCS_SIZEOF           16

/*********************************************************************************************************
  HyperTransport: Retry Mode
*********************************************************************************************************/
#define PCI_HT_RM_CTR0              4           /* Control 0 Register                                   */
#define PCI_HT_RM_CTR_LRETEN        0x01        /* Link Retry Enable                                    */
#define PCI_HT_RM_CTR_FSER          0x02        /* Force Single Error                                   */
#define PCI_HT_RM_CTR_ROLNEN        0x04        /* Rollover Nonfatal Enable                             */
#define PCI_HT_RM_CTR_FSS           0x08        /* Force Single Stomp                                   */
#define PCI_HT_RM_CTR_RETNEN        0x10        /* Retry Nonfatal Enable                                */
#define PCI_HT_RM_CTR_RETFEN        0x20        /* Retry Fatal Enable                                   */
#define PCI_HT_RM_CTR_AA            0xc0        /* Allowed Attempts                                     */
#define PCI_HT_RM_STS0              5           /* Status 0 Register                                    */
#define PCI_HT_RM_STS_RETSNT        0x01        /* Retry Sent                                           */
#define PCI_HT_RM_STS_CNTROL        0x02        /* Count Rollover                                       */
#define PCI_HT_RM_STS_SRCV          0x04        /* Stomp Received                                       */
#define PCI_HT_RM_CTR1              6           /* Control 1 Register                                   */
#define PCI_HT_RM_STS1              7           /* Status 1 Register                                    */
#define PCI_HT_RM_CNT0              8           /* Retry Count 0 Register                               */
#define PCI_HT_RM_CNT1              10          /* Retry Count 1 Register                               */
#define PCI_HT_RM_SIZEOF            12

/*********************************************************************************************************
  Vendor-Specific Capability (see PCI_EVNDR_xxx for the PCIe version)
*********************************************************************************************************/
#define PCI_VNDR_LENGTH             2           /* Length byte                                          */

/*********************************************************************************************************
  PCI Express
*********************************************************************************************************/
#define PCI_EXP_FLAGS               0x2         /* Capabilities register                                */
#define PCI_EXP_FLAGS_VERS          0x000f      /* Capability version                                   */
#define PCI_EXP_FLAGS_TYPE          0x00f0      /* Device/Port type                                     */
#define PCI_EXP_TYPE_ENDPOINT       0x0         /* Express Endpoint                                     */
#define PCI_EXP_TYPE_LEG_END        0x1         /* Legacy Endpoint                                      */
#define PCI_EXP_TYPE_ROOT_PORT      0x4         /* Root Port                                            */
#define PCI_EXP_TYPE_UPSTREAM       0x5         /* Upstream Port                                        */
#define PCI_EXP_TYPE_DOWNSTREAM     0x6         /* Downstream Port                                      */
#define PCI_EXP_TYPE_PCI_BRIDGE     0x7         /* PCIe to PCI/PCI-X Bridge                             */
#define PCI_EXP_TYPE_PCIE_BRIDGE    0x8         /* PCI/PCI-X to PCIe Bridge                             */
#define PCI_EXP_TYPE_ROOT_INT_EP    0x9         /* Root Complex Integrated Endpoint                     */
#define PCI_EXP_TYPE_ROOT_EC        0xa         /* Root Complex Event Collector                         */
#define PCI_EXP_FLAGS_SLOT          0x0100      /* Slot implemented                                     */
#define PCI_EXP_FLAGS_IRQ           0x3e00      /* Interrupt message number                             */
#define PCI_EXP_DEVCAP              0x4         /* Device capabilities                                  */
#define PCI_EXP_DEVCAP_PAYLOAD      0x07        /* Max_Payload_Size                                     */
#define PCI_EXP_DEVCAP_PHANTOM      0x18        /* Phantom functions                                    */
#define PCI_EXP_DEVCAP_EXT_TAG      0x20        /* Extended tags                                        */
#define PCI_EXP_DEVCAP_L0S          0x1c0       /* L0s Acceptable Latency                               */
#define PCI_EXP_DEVCAP_L1           0xe00       /* L1 Acceptable Latency                                */
#define PCI_EXP_DEVCAP_ATN_BUT      0x1000      /* Attention Button Present                             */
#define PCI_EXP_DEVCAP_ATN_IND      0x2000      /* Attention Indicator Present                          */
#define PCI_EXP_DEVCAP_PWR_IND      0x4000      /* Power Indicator Present                              */
#define PCI_EXP_DEVCAP_RBE          0x8000      /* Role-Based Error Reporting                           */
#define PCI_EXP_DEVCAP_PWR_VAL      0x3fc0000   /* Slot Power Limit Value                               */
#define PCI_EXP_DEVCAP_PWR_SCL      0xc000000   /* Slot Power Limit Scale                               */
#define PCI_EXP_DEVCAP_FLRESET      0x10000000  /* Function-Level Reset                                 */
#define PCI_EXP_DEVCTL              0x8         /* Device Control                                       */
#define PCI_EXP_DEVCTL_CERE         0x0001      /* Correctable Error Reporting Enabel                   */
#define PCI_EXP_DEVCTL_NFERE        0x0002      /* Non-Fatal Error Reporting Enable                     */
#define PCI_EXP_DEVCTL_FERE         0x0004      /* Fatal Error Reporting Enable                         */
#define PCI_EXP_DEVCTL_URRE         0x0008      /* Unsupported Request Reporting Enable                 */
#define PCI_EXP_DEVCTL_RELAXED      0x0010      /* Enable Relaxed Ordering                              */
#define PCI_EXP_DEVCTL_PAYLOAD      0x00e0      /* Max_Payload_Size                                     */
#define PCI_EXP_DEVCTL_EXT_TAG      0x0100      /* Extended Tag Field Enable                            */
#define PCI_EXP_DEVCTL_PHANTOM      0x0200      /* Phantom Functions Enable                             */
#define PCI_EXP_DEVCTL_AUX_PME      0x0400      /* Auxiliary Power PM Enable                            */
#define PCI_EXP_DEVCTL_NOSNOOP      0x0800      /* Enable No Snoop                                      */
#define PCI_EXP_DEVCTL_READRQ       0x7000      /* Max_Read_Request_Size                                */
#define PCI_EXP_DEVCTL_BCRE         0x8000      /* Bridge Configuration Retry Enable                    */
#define PCI_EXP_DEVCTL_FLRESET      0x8000      /* Function-Level Reset [bit shared with BCRE]          */
#define PCI_EXP_DEVSTA              0xa         /* Device Status                                        */
#define PCI_EXP_DEVSTA_CED          0x01        /* Correctable Error Detected                           */
#define PCI_EXP_DEVSTA_NFED         0x02        /* Non-Fatal Error Detected                             */
#define PCI_EXP_DEVSTA_FED          0x04        /* Fatal Error Detected                                 */
#define PCI_EXP_DEVSTA_URD          0x08        /* Unsupported Request Detected                         */
#define PCI_EXP_DEVSTA_AUXPD        0x10        /* AUX Power Detected                                   */
#define PCI_EXP_DEVSTA_TRPND        0x20        /* Transactions Pending                                 */
#define PCI_EXP_LNKCAP              0xc         /* Link Capabilities                                    */
#define PCI_EXP_LNKCAP_SPEED        0x0000f     /* Maximum Link Speed                                   */
#define PCI_EXP_LNKCAP_WIDTH        0x003f0     /* Maximum Link Width                                   */
#define PCI_EXP_LNKCAP_ASPM         0x00c00     /* Active State Power Management                        */
#define PCI_EXP_LNKCAP_L0S          0x07000     /* L0s Acceptable Latency                               */
#define PCI_EXP_LNKCAP_L1           0x38000     /* L1 Acceptable Latency                                */
#define PCI_EXP_LNKCAP_CLOCKPM      0x40000     /* Clock Power Management                               */
#define PCI_EXP_LNKCAP_SURPRISE     0x80000     /* Surprise Down Error Reporting                        */
#define PCI_EXP_LNKCAP_DLLA         0x100000    /* Data Link Layer Active Reporting                     */
#define PCI_EXP_LNKCAP_LBNC         0x200000    /* Link Bandwidth Notification Capability               */
#define PCI_EXP_LNKCAP_AOC          0x400000    /* ASPM Optionality Compliance                          */
#define PCI_EXP_LNKCAP_PORT         0xff000000  /* Port Number                                          */
#define PCI_EXP_LNKCTL              0x10        /* Link Control                                         */
#define PCI_EXP_LNKCTL_ASPM         0x0003      /* ASPM Control                                         */
#define PCI_EXP_LNKCTL_RCB          0x0008      /* Read Completion Boundary                             */
#define PCI_EXP_LNKCTL_DISABLE      0x0010      /* Link Disable                                         */
#define PCI_EXP_LNKCTL_RETRAIN      0x0020      /* Retrain Link                                         */
#define PCI_EXP_LNKCTL_CLOCK        0x0040      /* Common Clock Configuration                           */
#define PCI_EXP_LNKCTL_XSYNCH       0x0080      /* Extended Synch                                       */
#define PCI_EXP_LNKCTL_CLOCKPM      0x0100      /* Clock Power Management                               */
#define PCI_EXP_LNKCTL_HWAUTWD      0x0200      /* Hardware Autonomous Width Disable                    */
#define PCI_EXP_LNKCTL_BWMIE        0x0400      /* Bandwidth Mgmt Interrupt Enable                      */
#define PCI_EXP_LNKCTL_AUTBWIE      0x0800      /* Autonomous Bandwidth Mgmt Interrupt Enable           */
#define PCI_EXP_LNKSTA              0x12        /* Link Status                                          */
#define PCI_EXP_LNKSTA_SPEED        0x000f      /* Negotiated Link Speed                                */
#define PCI_EXP_LNKSTA_WIDTH        0x03f0      /* Negotiated Link Width                                */
#define PCI_EXP_LNKSTA_TR_ERR       0x0400      /* Training Error (obsolete)                            */
#define PCI_EXP_LNKSTA_TRAIN        0x0800      /* Link Training                                        */
#define PCI_EXP_LNKSTA_SL_CLK       0x1000      /* Slot Clock Configuration                             */
#define PCI_EXP_LNKSTA_DL_ACT       0x2000      /* Data Link Layer in DL_Active State                   */
#define PCI_EXP_LNKSTA_BWMGMT       0x4000      /* Bandwidth Mgmt Status                                */
#define PCI_EXP_LNKSTA_AUTBW        0x8000      /* Autonomous Bandwidth Mgmt Status                     */
#define PCI_EXP_SLTCAP              0x14        /* Slot Capabilities                                    */
#define PCI_EXP_SLTCAP_ATNB         0x0001      /* Attention Button Present                             */
#define PCI_EXP_SLTCAP_PWRC         0x0002      /* Power Controller Present                             */
#define PCI_EXP_SLTCAP_MRL          0x0004      /* MRL Sensor Present                                   */
#define PCI_EXP_SLTCAP_ATNI         0x0008      /* Attention Indicator Present                          */
#define PCI_EXP_SLTCAP_PWRI         0x0010      /* Power Indicator Present                              */
#define PCI_EXP_SLTCAP_HPS          0x0020      /* Hot-Plug Surprise                                    */
#define PCI_EXP_SLTCAP_HPC          0x0040      /* Hot-Plug Capable                                     */
#define PCI_EXP_SLTCAP_PWR_VAL      0x00007f80  /* Slot Power Limit Value                               */
#define PCI_EXP_SLTCAP_PWR_SCL      0x00018000  /* Slot Power Limit Scale                               */
#define PCI_EXP_SLTCAP_INTERLOCK    0x020000    /* Electromechanical Interlock Present                  */
#define PCI_EXP_SLTCAP_NOCMDCOMP    0x040000    /* No Command Completed Support                         */
#define PCI_EXP_SLTCAP_PSN          0xfff80000  /* Physical Slot Number                                 */
#define PCI_EXP_SLTCTL              0x18        /* Slot Control                                         */
#define PCI_EXP_SLTCTL_ATNB         0x0001      /* Attention Button Pressed Enable                      */
#define PCI_EXP_SLTCTL_PWRF         0x0002      /* Power Fault Detected Enable                          */
#define PCI_EXP_SLTCTL_MRLS         0x0004      /* MRL Sensor Changed Enable                            */
#define PCI_EXP_SLTCTL_PRSD         0x0008      /* Presence Detect Changed Enable                       */
#define PCI_EXP_SLTCTL_CMDC         0x0010      /* Command Completed Interrupt Enable                   */
#define PCI_EXP_SLTCTL_HPIE         0x0020      /* Hot-Plug Interrupt Enable                            */
#define PCI_EXP_SLTCTL_ATNI         0x00c0      /* Attention Indicator Control                          */
#define PCI_EXP_SLTCTL_PWRI         0x0300      /* Power Indicator Control                              */
#define PCI_EXP_SLTCTL_PWRC         0x0400      /* Power Controller Control                             */
#define PCI_EXP_SLTCTL_INTERLOCK    0x0800      /* Electromechanical Interlock Control                  */
#define PCI_EXP_SLTCTL_LLCHG        0x1000      /* Data Link Layer State Changed Enable                 */
#define PCI_EXP_SLTSTA              0x1a        /* Slot Status                                          */
#define PCI_EXP_SLTSTA_ATNB         0x0001      /* Attention Button Pressed                             */
#define PCI_EXP_SLTSTA_PWRF         0x0002      /* Power Fault Detected                                 */
#define PCI_EXP_SLTSTA_MRLS         0x0004      /* MRL Sensor Changed                                   */
#define PCI_EXP_SLTSTA_PRSD         0x0008      /* Presence Detect Changed                              */
#define PCI_EXP_SLTSTA_CMDC         0x0010      /* Command Completed                                    */
#define PCI_EXP_SLTSTA_MRL_ST       0x0020      /* MRL Sensor State                                     */
#define PCI_EXP_SLTSTA_PRES         0x0040      /* Presence Detect State                                */
#define PCI_EXP_SLTSTA_INTERLOCK    0x0080      /* Electromechanical Interlock Status                   */
#define PCI_EXP_SLTSTA_LLCHG        0x0100      /* Data Link Layer State Changed                        */
#define PCI_EXP_RTCTL               0x1c        /* Root Control                                         */
#define PCI_EXP_RTCTL_SECEE         0x0001      /* System Error on Correctable Error                    */
#define PCI_EXP_RTCTL_SENFEE        0x0002      /* System Error on Non-Fatal Error                      */
#define PCI_EXP_RTCTL_SEFEE         0x0004      /* System Error on Fatal Error                          */
#define PCI_EXP_RTCTL_PMEIE         0x0008      /* PME Interrupt Enable                                 */
#define PCI_EXP_RTCTL_CRSVIS        0x0010      /* Configuration Request Retry Status Visible to SW     */
#define PCI_EXP_RTCAP               0x1e        /* Root Capabilities                                    */
#define PCI_EXP_RTCAP_CRSVIS        0x0001      /* Configuration Request Retry Status Visible to SW     */
#define PCI_EXP_RTSTA               0x20        /* Root Status                                          */
#define PCI_EXP_RTSTA_PME_REQID     0x0000ffff  /* PME Requester ID                                     */
#define PCI_EXP_RTSTA_PME_STATUS    0x00010000  /* PME Status                                           */
#define PCI_EXP_RTSTA_PME_PENDING   0x00020000  /* PME is Pending                                       */
#define PCI_EXP_DEVCAP2             0x24        /* Device capabilities 2                                */
#define PCI_EXP_DEVCAP2_LTR         0x0800      /* LTR supported                                        */
                                                /* OBFF supported                                       */
#define PCI_EXP_DEVCAP2_OBFF(x)                 (((x) >> 18) & 3)
#define PCI_EXP_DEVCTL2             0x28        /* Device Control                                       */
                                                /* Completion Timeout Ranges Supported                  */
#define PCI_EXP_DEV2_TIMEOUT_RANGE(x)           ((x) & 0xf)
                                                /* Completion Timeout Value                             */
#define PCI_EXP_DEV2_TIMEOUT_VALUE(x)           ((x) & 0xf)
#define PCI_EXP_DEV2_TIMEOUT_DIS    0x0010      /* Completion Timeout Disable Supported                 */
#define PCI_EXP_DEV2_ARI            0x0020      /* ARI Forwarding                                       */
#define PCI_EXP_DEV2_LTR            0x0400      /* LTR enabled                                          */
                                                /* OBFF enabled                                         */
#define PCI_EXP_DEV2_OBFF(x)                    (((x) >> 13) & 3)
#define PCI_EXP_DEVSTA2             0x2a        /* Device Status                                        */
#define PCI_EXP_LNKCAP2             0x2c        /* Link Capabilities                                    */
#define PCI_EXP_LNKCTL2             0x30        /* Link Control                                         */
                                                /* Target Link Speed                                    */
#define PCI_EXP_LNKCTL2_SPEED(x)                ((x) & 0xf)
#define PCI_EXP_LNKCTL2_CMPLNC      0x0010      /* Enter Compliance                                     */
#define PCI_EXP_LNKCTL2_SPEED_DIS   0x0020      /* Hardware Autonomous Speed Disable                    */
                                                /* Selectable De-emphasis                               */
#define PCI_EXP_LNKCTL2_DEEMPHASIS(x)           (((x) >> 6) & 1)
                                                /* Transmit Margin                                      */
#define PCI_EXP_LNKCTL2_MARGIN(x)               (((x) >> 7) & 7)
#define PCI_EXP_LNKCTL2_MOD_CMPLNC  0x0400      /* Enter Modified Compliance                            */
#define PCI_EXP_LNKCTL2_CMPLNC_SOS  0x0800      /* Compliance SOS                                       */
                                                /* Compliance De-emphasis                               */
#define PCI_EXP_LNKCTL2_COM_DEEMPHASIS(x)       (((x) >> 12) & 0xf)
#define PCI_EXP_LNKSTA2             0x32        /* Link Status                                          */
                                                /* Current De-emphasis Level                            */
#define PCI_EXP_LINKSTA2_DEEMPHASIS(x)          ((x) & 1)
#define PCI_EXP_LINKSTA2_EQU_COMP   0x02        /* Equalization Complete                                */
#define PCI_EXP_LINKSTA2_EQU_PHASE1 0x04        /* Equalization Phase 1 Successful                      */
#define PCI_EXP_LINKSTA2_EQU_PHASE2 0x08        /* Equalization Phase 2 Successful                      */
#define PCI_EXP_LINKSTA2_EQU_PHASE3 0x10        /* Equalization Phase 3 Successful                      */
#define PCI_EXP_LINKSTA2_EQU_REQ    0x20        /* Link Equalization Request                            */
#define PCI_EXP_SLTCAP2             0x34        /* Slot Capabilities                                    */
#define PCI_EXP_SLTCTL2             0x38        /* Slot Control                                         */
#define PCI_EXP_SLTSTA2             0x3a        /* Slot Status                                          */

/*********************************************************************************************************
  MSI-X
*********************************************************************************************************/
#define PCI_MSIX_ENABLE             0x8000
#define PCI_MSIX_MASK               0x4000
#define PCI_MSIX_TABSIZE            0x07ff
#define PCI_MSIX_TABLE              4
#define PCI_MSIX_PBA                8
#define PCI_MSIX_BIR                0x7

/*********************************************************************************************************
  Subsystem vendor/device ID for PCI bridges
*********************************************************************************************************/
#define PCI_SSVID_VENDOR            4
#define PCI_SSVID_DEVICE            6

/*********************************************************************************************************
  PCI Advanced Features
*********************************************************************************************************/
#define PCI_AF_CAP                  3
#define PCI_AF_CAP_TP               0x01
#define PCI_AF_CAP_FLR              0x02
#define PCI_AF_CTRL                 4
#define PCI_AF_CTRL_FLR             0x01
#define PCI_AF_STATUS               5
#define PCI_AF_STATUS_TP            0x01

/*********************************************************************************************************
  SATA Host Bus Adapter
*********************************************************************************************************/
#define PCI_SATA_HBA_BARS           4
#define PCI_SATA_HBA_REG0           8

/*********************************************************************************************************
  Enhanced Allocation (EA)
*********************************************************************************************************/
#define PCI_EA_CAP_TYPE1_SECONDARY              4
#define PCI_EA_CAP_TYPE1_SUBORDINATE            5
/*********************************************************************************************************
  EA Entry header
*********************************************************************************************************/
#define PCI_EA_CAP_ENT_WRITABLE     0x40000000  /* Writable: 1 = RW, 0 = HwInit                         */
#define PCI_EA_CAP_ENT_ENABLE       0x80000000  /* Enable for this entry                                */

/*********************************************************************************************************
  Definitions of extended capabilities
*********************************************************************************************************/
/*********************************************************************************************************
  Advanced Error Reporting
*********************************************************************************************************/
#define PCI_ERR_UNCOR_STATUS        4           /* Uncorrectable Error Status                           */
#define PCI_ERR_UNC_TRAIN           0x00000001  /* Undefined in PCIe rev1.1 & 2.0 spec                  */
#define PCI_ERR_UNC_DLP             0x00000010  /* Data Link Protocol                                   */
#define PCI_ERR_UNC_SDES            0x00000020  /* Surprise Down Error                                  */
#define PCI_ERR_UNC_POISON_TLP      0x00001000  /* Poisoned TLP                                         */
#define PCI_ERR_UNC_FCP             0x00002000  /* Flow Control Protocol                                */
#define PCI_ERR_UNC_COMP_TIME       0x00004000  /* Completion Timeout                                   */
#define PCI_ERR_UNC_COMP_ABORT      0x00008000  /* Completer Abort                                      */
#define PCI_ERR_UNC_UNX_COMP        0x00010000  /* Unexpected Completion                                */
#define PCI_ERR_UNC_RX_OVER         0x00020000  /* Receiver Overflow                                    */
#define PCI_ERR_UNC_MALF_TLP        0x00040000  /* Malformed TLP                                        */
#define PCI_ERR_UNC_ECRC            0x00080000  /* ECRC Error Status                                    */
#define PCI_ERR_UNC_UNSUP           0x00100000  /* Unsupported Request                                  */
#define PCI_ERR_UNC_ACS_VIOL        0x00200000  /* ACS Violation                                        */
#define PCI_ERR_UNCOR_MASK          8           /* Uncorrectable Error Mask                             */

/*********************************************************************************************************
  Same bits as above
*********************************************************************************************************/
#define PCI_ERR_UNCOR_SEVER         12          /* Uncorrectable Error Severity                         */

/*********************************************************************************************************
  Same bits as above
*********************************************************************************************************/
#define PCI_ERR_COR_STATUS          16          /* Correctable Error Status                             */
#define PCI_ERR_COR_RCVR            0x00000001  /* Receiver Error Status                                */
#define PCI_ERR_COR_BAD_TLP         0x00000040  /* Bad TLP Status                                       */
#define PCI_ERR_COR_BAD_DLLP        0x00000080  /* Bad DLLP Status                                      */
#define PCI_ERR_COR_REP_ROLL        0x00000100  /* REPLAY_NUM Rollover                                  */
#define PCI_ERR_COR_REP_TIMER       0x00001000  /* Replay Timer Timeout                                 */
#define PCI_ERR_COR_REP_ANFE        0x00002000  /* Advisory Non-Fatal Error                             */
#define PCI_ERR_COR_MASK            20          /* Correctable Error Mask                               */

/*********************************************************************************************************
  Same bits as above
*********************************************************************************************************/
#define PCI_ERR_CAP                 24          /* Advanced Error Capabilities                          */
#define PCI_ERR_CAP_FEP(x)          ((x) & 31)  /* First Error Pointer                                  */
#define PCI_ERR_CAP_ECRC_GENC       0x00000020  /* ECRC Generation Capable                              */
#define PCI_ERR_CAP_ECRC_GENE       0x00000040  /* ECRC Generation Enable                               */
#define PCI_ERR_CAP_ECRC_CHKC       0x00000080  /* ECRC Check Capable                                   */
#define PCI_ERR_CAP_ECRC_CHKE       0x00000100  /* ECRC Check Enable                                    */
#define PCI_ERR_HEADER_LOG          28          /* Header Log Register (16 bytes)                       */
#define PCI_ERR_ROOT_COMMAND        44          /* Root Error Command                                   */
#define PCI_ERR_ROOT_STATUS         48
#define PCI_ERR_ROOT_COR_SRC        52
#define PCI_ERR_ROOT_SRC            54

/*********************************************************************************************************
  Virtual Channel
*********************************************************************************************************/
#define PCI_VC_PORT_REG1            4
#define PCI_VC_PORT_REG2            8
#define PCI_VC_PORT_CTRL            12
#define PCI_VC_PORT_STATUS          14
#define PCI_VC_RES_CAP              16
#define PCI_VC_RES_CTRL             20
#define PCI_VC_RES_STATUS           26

/*********************************************************************************************************
  Power Budgeting
*********************************************************************************************************/
#define PCI_PWR_DSR                 4           /* Data Select Register                                 */
#define PCI_PWR_DATA                8           /* Data Register                                        */
#define PCI_PWR_DATA_BASE(x)        ((x) & 0xff)/* Base Power                                           */
                                                /* Data Scale                                           */
#define PCI_PWR_DATA_SCALE(x)       (((x) >>  8) & 3)
                                                /* PM Sub State                                         */
#define PCI_PWR_DATA_PM_SUB(x)      (((x) >> 10) & 7)
                                                /* PM State                                             */
#define PCI_PWR_DATA_PM_STATE(x)    (((x) >> 13) & 3)
                                                /* Type                                                 */
#define PCI_PWR_DATA_TYPE(x)        (((x) >> 15) & 7)
                                                /* Power Rail                                           */
#define PCI_PWR_DATA_RAIL(x)        (((x) >> 18) & 7)
#define PCI_PWR_CAP                 12          /* Capability                                           */
#define PCI_PWR_CAP_BUDGET(x)       ((x) & 1)   /* Included in system budget                            */

/*********************************************************************************************************
  Root Complex Link
*********************************************************************************************************/
#define PCI_RCLINK_ESD              4           /* Element Self Description                             */
#define PCI_RCLINK_LINK1            16          /* First Link Entry                                     */
#define PCI_RCLINK_LINK_DESC        0           /* Link Entry: Description                              */
#define PCI_RCLINK_LINK_ADDR        8           /* Link Entry: Address (64-bit)                         */
#define PCI_RCLINK_LINK_SIZE        16          /* Link Entry: sizeof                                   */

/*********************************************************************************************************
  PCIe Vendor-Specific Capability
*********************************************************************************************************/
#define PCI_EVNDR_HEADER            4           /* Vendor-Specific Header                               */
#define PCI_EVNDR_REGISTERS         8           /* Vendor-Specific Registers                            */

/*********************************************************************************************************
  Access Control Services
*********************************************************************************************************/
#define PCI_ACS_CAP                 0x04        /* ACS Capability Register                              */
#define PCI_ACS_CAP_VALID           0x0001      /* ACS Source Validation                                */
#define PCI_ACS_CAP_BLOCK           0x0002      /* ACS Translation Blocking                             */
#define PCI_ACS_CAP_REQ_RED         0x0004      /* ACS P2P Request Redirect                             */
#define PCI_ACS_CAP_CMPLT_RED       0x0008      /* ACS P2P Completion Redirect                          */
#define PCI_ACS_CAP_FORWARD         0x0010      /* ACS Upstream Forwarding                              */
#define PCI_ACS_CAP_EGRESS          0x0020      /* ACS P2P Egress Control                               */
#define PCI_ACS_CAP_TRANS           0x0040      /* ACS Direct Translated P2P                            */
                                                /* Egress Control Vector Size                           */
#define PCI_ACS_CAP_VECTOR(x)       (((x) >> 8) & 0xff)
#define PCI_ACS_CTRL                0x06        /* ACS Control Register                                 */
#define PCI_ACS_CTRL_VALID          0x0001      /* ACS Source Validation Enable                         */
#define PCI_ACS_CTRL_BLOCK          0x0002      /* ACS Translation Blocking Enable                      */
#define PCI_ACS_CTRL_REQ_RED        0x0004      /* ACS P2P Request Redirect Enable                      */
#define PCI_ACS_CTRL_CMPLT_RED      0x0008      /* ACS P2P Completion Redirect Enable                   */
#define PCI_ACS_CTRL_FORWARD        0x0010      /* ACS Upstream Forwarding Enable                       */
#define PCI_ACS_CTRL_EGRESS         0x0020      /* ACS P2P Egress Control Enable                        */
#define PCI_ACS_CTRL_TRANS          0x0040      /* ACS Direct Translated P2P Enable                     */
#define PCI_ACS_EGRESS_CTRL         0x08        /* Egress Control Vector                                */

/*********************************************************************************************************
  Alternative Routing-ID Interpretation
*********************************************************************************************************/
#define PCI_ARI_CAP                 0x04        /* ARI Capability Register                              */
#define PCI_ARI_CAP_MFVC            0x0001      /* MFVC Function Groups Capability                      */
#define PCI_ARI_CAP_ACS             0x0002      /* ACS Function Groups Capability                       */
                                                /* Next Function Number                                 */
#define PCI_ARI_CAP_NFN(x)          (((x) >> 8) & 0xff)
#define PCI_ARI_CTRL                0x06        /* ARI Control Register                                 */
#define PCI_ARI_CTRL_MFVC           0x0001      /* MFVC Function Groups Enable                          */
#define PCI_ARI_CTRL_ACS            0x0002      /* ACS Function Groups Enable                           */
                                                /* Function Group */
#define PCI_ARI_CTRL_FG(x)          (((x) >> 4) & 7)

/*********************************************************************************************************
  Address Translation Service
*********************************************************************************************************/
#define PCI_ATS_CAP                 0x04        /* ATS Capability Register                              */
#define PCI_ATS_CAP_IQD(x)          ((x) & 0x1f)/* Invalidate Queue Depth                               */
#define PCI_ATS_CTRL                0x06        /* ATS Control Register                                 */
#define PCI_ATS_CTRL_STU(x)         ((x) & 0x1f)/* Smallest Translation Unit                            */
#define PCI_ATS_CTRL_ENABLE         0x8000      /* ATS Enable                                           */

/*********************************************************************************************************
  Single Root I/O Virtualization
*********************************************************************************************************/
#define PCI_IOV_CAP                 0x04        /* SR-IOV Capability Register                           */
#define PCI_IOV_CAP_VFM             0x00000001  /* VF Migration Capable                                 */
#define PCI_IOV_CAP_IMN(x)          ((x) >> 21) /* VF Migration Interrupt Message Number                */
#define PCI_IOV_CTRL                0x08        /* SR-IOV Control Register                              */
#define PCI_IOV_CTRL_VFE            0x0001      /* VF Enable                                            */
#define PCI_IOV_CTRL_VFME           0x0002      /* VF Migration Enable                                  */
#define PCI_IOV_CTRL_VFMIE          0x0004      /* VF Migration Interrupt Enable                        */
#define PCI_IOV_CTRL_MSE            0x0008      /* VF MSE                                               */
#define PCI_IOV_CTRL_ARI            0x0010      /* ARI Capable Hierarchy                                */
#define PCI_IOV_STATUS              0x0a        /* SR-IOV Status Register                               */
#define PCI_IOV_STATUS_MS           0x0001      /* VF Migration Status                                  */
#define PCI_IOV_INITIALVF           0x0c        /* Number of VFs that are initially associated          */
#define PCI_IOV_TOTALVF             0x0e        /* Maximum number of VFs that could be associated       */
#define PCI_IOV_NUMVF               0x10        /* Number of VFs that are available                     */
#define PCI_IOV_FDL                 0x12        /* Function Dependency Link                             */
#define PCI_IOV_OFFSET              0x14        /* First VF Offset                                      */
#define PCI_IOV_STRIDE              0x16        /* Routing ID offset from one VF to the next one        */
#define PCI_IOV_DID                 0x1a        /* VF Device ID                                         */
#define PCI_IOV_SUPPS               0x1c        /* Supported Page Sizes                                 */
#define PCI_IOV_SYSPS               0x20        /* System Page Size                                     */
#define PCI_IOV_BAR_BASE            0x24        /* VF BAR0, VF BAR1, ... VF BAR5                        */
#define PCI_IOV_NUM_BAR             6           /* Number of VF BARs                                    */
#define PCI_IOV_MSAO                0x3c        /* VF Migration State Array Offset                      */
#define PCI_IOV_MSA_BIR(x)          ((x) & 7)   /* VF Migration State BIR                               */
                                                /* VF Migration State Offset                            */
#define PCI_IOV_MSA_OFFSET(x)       ((x) & 0xfffffff8)

/*********************************************************************************************************
  Page Request Interface
*********************************************************************************************************/
#define PCI_PRI_CTRL                0x04        /* PRI Control Register                                 */
#define PCI_PRI_CTRL_ENABLE         0x01        /* Enable                                               */
#define PCI_PRI_CTRL_RESET          0x02        /* Reset                                                */
#define PCI_PRI_STATUS              0x06        /* PRI status register                                  */
#define PCI_PRI_STATUS_RF           0x001       /* Response Failure                                     */
#define PCI_PRI_STATUS_UPRGI        0x002       /* Unexpected PRG index                                 */
#define PCI_PRI_STATUS_STOPPED      0x100       /* PRI Stopped                                          */
#define PCI_PRI_MAX_REQ             0x08        /* PRI max reqs supported                               */
#define PCI_PRI_ALLOC_REQ           0x0c        /* PRI max reqs allowed                                 */

/*********************************************************************************************************
  Transaction Processing Hints
*********************************************************************************************************/
#define PCI_TPH_CAPABILITIES        4
#define PCI_TPH_INTVEC_SUP          (1<<1)      /* Supports interrupt vector mode                       */
#define PCI_TPH_DEV_SUP             (1<<2)      /* Device specific mode supported                       */
#define PCI_TPH_EXT_REQ_SUP         (1<<8)      /* Supports extended requests                           */
#define PCI_TPH_ST_LOC_MASK         (3<<9)      /* Steering table location bits                         */
#define PCI_TPH_ST_NONE             (0<<9)      /* No steering table                                    */
#define PCI_TPH_ST_CAP              (1<<9)      /* Steering table in TPH cap                            */
#define PCI_TPH_ST_MSIX             (2<<9)      /* Steering table in MSI-X table                        */
#define PCI_TPH_ST_SIZE_SHIFT       (16)        /* Encoded as size - 1                                  */

/*********************************************************************************************************
  Latency Tolerance Reporting
*********************************************************************************************************/
#define PCI_LTR_MAX_SNOOP           4           /* 16 bit value                                         */
#define PCI_LTR_VALUE_MASK          (0x3ff)
#define PCI_LTR_SCALE_SHIFT         (10)
#define PCI_LTR_SCALE_MASK          (7)
#define PCI_LTR_MAX_NOSNOOP         6           /* 16 bit value                                         */

/*********************************************************************************************************
  Process Address Space ID
*********************************************************************************************************/
#define PCI_PASID_CAP               0x04        /* PASID feature register                               */
#define PCI_PASID_CAP_EXEC          0x02        /* Exec permissions Supported                           */
#define PCI_PASID_CAP_PRIV          0x04        /* Privilege Mode Supported                             */
                                                /* Max PASID Width                                      */
#define PCI_PASID_CAP_WIDTH(x)      (((x) >> 8) & 0x1f)
#define PCI_PASID_CTRL              0x06        /* PASID control register                               */
#define PCI_PASID_CTRL_ENABLE       0x01        /* Enable bit                                           */
#define PCI_PASID_CTRL_EXEC         0x02        /* Exec permissions Enable                              */
#define PCI_PASID_CTRL_PRIV         0x04        /* Privilege Mode Enable                                */

#define PCI_CFG_SPACE_SIZE          256
#define PCI_CFG_SPACE_EXP_SIZE      4096

#define PCI_FIND_CAP_TTL            48

/*********************************************************************************************************
  PCI-to-PCI bridge Type 1 configuration register offsets
  Note that only modulo-4 addresses are written to the address register
*********************************************************************************************************/
#define PCI_CFG_PRIMARY_BUS         0x18
#define PCI_CFG_SECONDARY_BUS       0x19
#define PCI_CFG_SUBORDINATE_BUS     0x1a
#define PCI_CFG_SEC_LATENCY         0x1b
#define PCI_CFG_IO_BASE             0x1c
#define PCI_CFG_IO_LIMIT            0x1d
#define PCI_CFG_SEC_STATUS          0x1e
#define PCI_CFG_MEM_BASE            0x20
#define PCI_CFG_MEM_LIMIT           0x22
#define PCI_CFG_PRE_MEM_BASE        0x24
#define PCI_CFG_PRE_MEM_LIMIT       0x26
#define PCI_CFG_PRE_MEM_BASE_U      0x28
#define PCI_CFG_PRE_MEM_LIMIT_U     0x2c
#define PCI_CFG_IO_BASE_U           0x30
#define PCI_CFG_IO_LIMIT_U          0x32
#define PCI_CFG_ROM_BASE            0x38
#define PCI_CFG_BRG_INT_LINE        0x3c
#define PCI_CFG_BRG_INT_PIN         0x3d
#define PCI_CFG_BRIDGE_CONTROL      0x3e

/*********************************************************************************************************
  Standard device Type 0 configuration register offsets
  Note that only modulo-4 addresses are written to the address register
*********************************************************************************************************/
#define PCI_CFG_VENDOR_ID           0x00
#define PCI_CFG_DEVICE_ID           0x02
#define PCI_CFG_COMMAND             0x04
#define PCI_CFG_STATUS              0x06
#define PCI_CFG_REVISION            0x08
#define PCI_CFG_PROGRAMMING_IF      0x09
#define PCI_CFG_SUBCLASS            0x0a
#define PCI_CFG_CLASS               0x0b
#define PCI_CFG_CACHE_LINE_SIZE     0x0c
#define PCI_CFG_LATENCY_TIMER       0x0d
#define PCI_CFG_HEADER_TYPE         0x0e
#define PCI_CFG_BIST                0x0f
#define PCI_CFG_BASE_ADDRESS_0      0x10
#define PCI_CFG_BASE_ADDRESS_1      0x14
#define PCI_CFG_BASE_ADDRESS_2      0x18
#define PCI_CFG_BASE_ADDRESS_3      0x1c
#define PCI_CFG_BASE_ADDRESS_4      0x20
#define PCI_CFG_BASE_ADDRESS_5      0x24
#define PCI_CFG_CIS                 0x28
#define PCI_CFG_SUB_VENDER_ID       0x2c
#define PCI_CFG_SUB_SYSTEM_ID       0x2e
#define PCI_CFG_EXPANSION_ROM       0x30
#define PCI_CFG_CAP_PTR             0x34
#define PCI_CFG_RESERVED_0          0x35
#define PCI_CFG_RESERVED_1          0x38
#define PCI_CFG_DEV_INT_LINE        0x3c
#define PCI_CFG_DEV_INT_PIN         0x3d
#define PCI_CFG_MIN_GRANT           0x3e
#define PCI_CFG_MAX_LATENCY         0x3f
#define PCI_CFG_SPECIAL_USE         0x41
#define PCI_CFG_MODE                0x43

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
#endif                                                                  /*  __PCIBUS_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: pciDb.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 09 月 30 日
**
** 描        述: PCI 总线信息数据库.

** BUG:
2015.10.15  更新类与子类以及编程接口数据库信息.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
#include "pciIds.h"
/*********************************************************************************************************
  总线信息结构
*********************************************************************************************************/
typedef struct {
    UINT32              CP_uiProgIf;
    PCHAR               CS_pcProgIfName;
} PCI_CLASS_PROGIF;

typedef struct {
    UINT16              CS_ucSubClass;                                  /*  high 8 bit is class code    */
    PCHAR               CS_pcSubName;
    PCI_CLASS_PROGIF   *CS_pcpTable;
    UINT16              CS_usTableNum;
} PCI_CLASS_SUB;

typedef struct {
    UINT8               CC_ucClassCode;
    PCHAR               CC_pcClassName;
    PCI_CLASS_SUB      *CC_pcsTable;
    UINT16              CC_usTableNum;
} PCI_CLASS_CODE;

typedef struct {
    UINT16              SI_usSubVendorId;
    UINT16              SI_usSubDeviceId;
    PCHAR               SI_pcSubSystemName;
} PCI_SUB_IDS;

typedef struct {
    UINT16              DI_usDeviceId;
    PCHAR               DI_pcDeviceName;
    PCI_SUB_IDS        *DI_psiSubTable;
    UINT16              DI_usSubTableNum;
} PCI_DEVICE_IDS;

typedef struct {
    UINT16              VI_usVendorId;
    PCHAR               VI_pcVendorName;
    PCI_DEVICE_IDS     *VI_pdiDeviceTable;
    UINT16              VI_usDeviceTableNum;
} PCI_VENDOR_IDS;
/*********************************************************************************************************
  总线 class 信息
*********************************************************************************************************/
static PCI_CLASS_CODE   _G_pccPciClassTable[] = {
    {PCI_BASE_CLASS_NOT_DEFINED,            "Unclassified device",                      LW_NULL,    2},
    {PCI_BASE_CLASS_STORAGE,                "Mass storage controller",                  LW_NULL,   10},
    {PCI_BASE_CLASS_NETWORK,                "Network controller",                       LW_NULL,   10},
    {PCI_BASE_CLASS_DISPLAY,                "Display controller",                       LW_NULL,    4},
    {PCI_BASE_CLASS_MULTIMEDIA,             "Multimedia controller",                    LW_NULL,    5},
    {PCI_BASE_CLASS_MEMORY,                 "Memory controller",                        LW_NULL,    3},
    {PCI_BASE_CLASS_BRIDGE,                 "Bridge",                                   LW_NULL,   12},
    {PCI_BASE_CLASS_COMMUNICATION,          "Communication controller",                 LW_NULL,    7},
    {PCI_BASE_CLASS_SYSTEM,                 "Generic system peripheral",                LW_NULL,    8},
    {PCI_BASE_CLASS_INPUT,                  "Input device controller",                  LW_NULL,    6},
    {PCI_BASE_CLASS_DOCKING,                "Docking station",                          LW_NULL,    2},
    {PCI_BASE_CLASS_PROCESSOR,              "Processor",                                LW_NULL,    7},
    {PCI_BASE_CLASS_SERIAL,                 "Serial bus controller",                    LW_NULL,   10},
    {PCI_BASE_CLASS_WIRELESS,               "Wireless controller",                      LW_NULL,    8},
    {PCI_BASE_CLASS_INTELLIGENT,            "Intelligent controller",                   LW_NULL,    1},
    {PCI_BASE_CLASS_SATELLITE,              "Satellite communications controller",      LW_NULL,    4},
    {PCI_BASE_CLASS_CRYPT,                  "Encryption controller",                    LW_NULL,    3},
    {PCI_BASE_CLASS_SIGNAL_PROCESSING,      "Signal processing controller",             LW_NULL,    5},
    {PCI_BASE_CLASS_PROCESS_ACCELER,        "Processing accelerators",                  LW_NULL,    1},
    {PCI_BASE_CLASS_NON_ESS_INS,            "Non-Essential Instrumentation",            LW_NULL,    0},
    {PCI_BASE_CLASS_COPROCESS,              "Coprocessor",                              LW_NULL,    0},
    {PCI_CLASS_OTHERS,                      "Unassigned class",                         LW_NULL,    0},
};
/*********************************************************************************************************
  总线 sub class 信息
*********************************************************************************************************/
static PCI_CLASS_SUB    _G_pcsPciSubClassTable[] = {
    /*
     *  Unclassified device
     */
    {PCI_CLASS_NOT_DEFINED_NON_VGA,         "Non-VGA unclassified device",              LW_NULL,    0},
    {PCI_CLASS_NOT_DEFINED_VGA,             "VGA compatible unclassified device",       LW_NULL,    0},
    /*
     *  Mass storage controller
     */
    {PCI_CLASS_STORAGE_SCSI,                "SCSI storage controller",                  LW_NULL,    0},
    {PCI_CLASS_STORAGE_IDE,                 "IDE interface",                            LW_NULL,    5},
    {PCI_CLASS_STORAGE_FLOPPY,              "Floppy disk controller",                   LW_NULL,    0},
    {PCI_CLASS_STORAGE_IPI,                 "IPI bus controller",                       LW_NULL,    0},
    {PCI_CLASS_STORAGE_RAID,                "RAID bus controller",                      LW_NULL,    0},
    {PCI_CLASS_STORAGE_ATA,                 "ATA controller",                           LW_NULL,    2},
    {PCI_CLASS_STORAGE_SATA,                "SATA controller",                          LW_NULL,    3},
    {PCI_CLASS_STORAGE_SAS,                 "Serial Attached SCSI controller",          LW_NULL,    1},
    {PCI_CLASS_STORAGE_NVM,                 "Non-Volatile memory controller",           LW_NULL,    2},
    {PCI_CLASS_STORAGE_OTHER,               "Mass storage controller",                  LW_NULL,    0},
    /*
     *  Network controller
     */
    {PCI_CLASS_NETWORK_ETHERNET,            "Ethernet controller",                      LW_NULL,    0},
    {PCI_CLASS_NETWORK_TOKEN_RING,          "Token ring network controller",            LW_NULL,    0},
    {PCI_CLASS_NETWORK_FDDI,                "FDDI network controller",                  LW_NULL,    0},
    {PCI_CLASS_NETWORK_ATM,                 "ATM network controller",                   LW_NULL,    0},
    {PCI_CLASS_NETWORK_ISDN,                "ISDN controller",                          LW_NULL,    0},
    {PCI_CLASS_NETWORK_WORLDFIP,            "WorldFip controller",                      LW_NULL,    0},
    {PCI_CLASS_NETWORK_PICMG,               "PICMG controller",                         LW_NULL,    0},
    {PCI_CLASS_NETWORK_INFINIBAND,          "Infiniband controller",                    LW_NULL,    0},
    {PCI_CLASS_NETWORK_FABRIC,              "Fabric controller",                        LW_NULL,    0},
    {PCI_CLASS_NETWORK_OTHER,               "Network controller",                       LW_NULL,    0},
    /*
     *  Display controller
     */
    {PCI_CLASS_DISPLAY_VGA,                 "VGA compatible controller",                LW_NULL,    2},
    {PCI_CLASS_DISPLAY_XGA,                 "XGA compatible controller",                LW_NULL,    0},
    {PCI_CLASS_DISPLAY_3D,                  "3D controller",                            LW_NULL,    0},
    {PCI_CLASS_DISPLAY_OTHER,               "Display controller",                       LW_NULL,    0},
    /*
     *  Multimedia controller
     */
    {PCI_CLASS_MULTIMEDIA_VIDEO,            "Multimedia video controller",              LW_NULL,    0},
    {PCI_CLASS_MULTIMEDIA_AUDIO,            "Multimedia audio controller",              LW_NULL,    0},
    {PCI_CLASS_MULTIMEDIA_PHONE,            "Computer telephony device",                LW_NULL,    0},
    {PCI_CLASS_MULTIMEDIA_AUDIO_DEV,        "Audio device",                             LW_NULL,    0},
    {PCI_CLASS_MULTIMEDIA_OTHER,            "Multimedia controller",                    LW_NULL,    0},
    /*
     *  Memory controller
     */
    {PCI_CLASS_MEMORY_RAM,                  "RAM memory",                               LW_NULL,    0},
    {PCI_CLASS_MEMORY_FLASH,                "FLASH memory",                             LW_NULL,    0},
    {PCI_CLASS_MEMORY_OTHER,                "Memory controller",                        LW_NULL,    0},
    /*
     *  Bridge
     */
    {PCI_CLASS_BRIDGE_HOST,                 "Host bridge",                              LW_NULL,    0},
    {PCI_CLASS_BRIDGE_ISA,                  "ISA bridge",                               LW_NULL,    0},
    {PCI_CLASS_BRIDGE_EISA,                 "EISA bridge",                              LW_NULL,    0},
    {PCI_CLASS_BRIDGE_MC,                   "MicroChannel bridge",                      LW_NULL,    0},
    {PCI_CLASS_BRIDGE_PCI,                  "PCI bridge",                               LW_NULL,    2},
    {PCI_CLASS_BRIDGE_PCMCIA,               "PCMCIA bridge",                            LW_NULL,    0},
    {PCI_CLASS_BRIDGE_NUBUS,                "NuBus bridge",                             LW_NULL,    0},
    {PCI_CLASS_BRIDGE_CARDBUS,              "CardBus bridge",                           LW_NULL,    0},
    {PCI_CLASS_BRIDGE_RACEWAY,              "RACEway bridge",                           LW_NULL,    2},
    {PCI_CLASS_BRIDGE_PCI_SEMI,             "Semi-transparent PCI-to-PCI bridge",       LW_NULL,    2},
    {PCI_CLASS_BRIDGE_IB_TO_PCI,            "InfiniBand to PCI host bridge",            LW_NULL,    0},
    {PCI_CLASS_BRIDGE_OTHER,                "Bridge",                                   LW_NULL,    0},
    /*
     *  Communication controller
     */
    {PCI_CLASS_COMMUNICATION_SERIAL,        "Serial controller",                        LW_NULL,    7},
    {PCI_CLASS_COMMUNICATION_PARALLEL,      "Parallel controller",                      LW_NULL,    5},
    {PCI_CLASS_COMMUNICATION_MULTISERIAL,   "Multiport serial controller",              LW_NULL,    0},
    {PCI_CLASS_COMMUNICATION_MODEM,         "Modem",                                    LW_NULL,    5},
    {PCI_CLASS_COMMUNICATION_GPIB,          "GPIB controller",                          LW_NULL,    0},
    {PCI_CLASS_COMMUNICATION_SMARD_CARD,    "Smard Card controller",                    LW_NULL,    0},
    {PCI_CLASS_COMMUNICATION_OTHER,         "Communication controller",                 LW_NULL,    0},
    /*
     *  Generic system peripheral
     */
    {PCI_CLASS_SYSTEM_PIC,                  "PIC",                                      LW_NULL,    5},
    {PCI_CLASS_SYSTEM_DMA,                  "DMA controller",                           LW_NULL,    3},
    {PCI_CLASS_SYSTEM_TIMER,                "Timer",                                    LW_NULL,    4},
    {PCI_CLASS_SYSTEM_RTC,                  "RTC",                                      LW_NULL,    2},
    {PCI_CLASS_SYSTEM_PCI_HOTPLUG,          "PCI Hot-plug controller",                  LW_NULL,    0},
    {PCI_CLASS_SYSTEM_SDHCI,                "SD Host controller",                       LW_NULL,    0},
    {PCI_CLASS_SYSTEM_IOMMU,                "IOMMU",                                    LW_NULL,    0},
    {PCI_CLASS_SYSTEM_OTHER,                "System peripheral",                        LW_NULL,    0},
    /*
     *  Input device controller
     */
    {PCI_CLASS_INPUT_KEYBOARD,              "Keyboard controller",                      LW_NULL,    0},
    {PCI_CLASS_INPUT_PEN,                   "Digitizer Pen",                            LW_NULL,    0},
    {PCI_CLASS_INPUT_MOUSE,                 "Mouse controller",                         LW_NULL,    0},
    {PCI_CLASS_INPUT_SCANNER,               "Scanner controller",                       LW_NULL,    0},
    {PCI_CLASS_INPUT_GAMEPORT,              "Gameport controller",                      LW_NULL,    2},
    {PCI_CLASS_INPUT_OTHER,                 "Input device controller",                  LW_NULL,    0},
    /*
     *  Docking station
     */
    {PCI_CLASS_DOCKING_GENERIC,             "Generic Docking Station",                  LW_NULL,    0},
    {PCI_CLASS_DOCKING_OTHER,               "Docking Station",                          LW_NULL,    0},
    /*
     *  Processor
     */
    {PCI_CLASS_PROCESSOR_386,               "386",                                      LW_NULL,    0},
    {PCI_CLASS_PROCESSOR_486,               "486",                                      LW_NULL,    0},
    {PCI_CLASS_PROCESSOR_PENTIUM,           "Pentium",                                  LW_NULL,    0},
    {PCI_CLASS_PROCESSOR_ALPHA,             "Alpha",                                    LW_NULL,    0},
    {PCI_CLASS_PROCESSOR_POWERPC,           "Power PC",                                 LW_NULL,    0},
    {PCI_CLASS_PROCESSOR_MIPS,              "MIPS",                                     LW_NULL,    0},
    {PCI_CLASS_PROCESSOR_CO,                "Co-Processor",                             LW_NULL,    0},
    /*
     *  Serial bus controller
     */
    {PCI_CLASS_SERIAL_FIREWIRE,             "FireWire (IEEE 1394)",                     LW_NULL,    2},
    {PCI_CLASS_SERIAL_ACCESS,               "ACCESS bus",                               LW_NULL,    0},
    {PCI_CLASS_SERIAL_SSA,                  "SSA (Serial Storage Architecture)",        LW_NULL,    0},
    {PCI_CLASS_SERIAL_USB,                  "USB controller",                           LW_NULL,    6},
    {PCI_CLASS_SERIAL_FIBER,                "Fibre Channel",                            LW_NULL,    0},
    {PCI_CLASS_SERIAL_SMBUS,                "SMBus",                                    LW_NULL,    0},
    {PCI_CLASS_SERIAL_INFINIBAND,           "InfiniBand",                               LW_NULL,    0},
    {PCI_CLASS_SERIAL_IPMI,                 "IPMI SMIC interface",                      LW_NULL,    0},
    {PCI_CLASS_SERIAL_SERCOS,               "SERCOS interface",                         LW_NULL,    0},
    {PCI_CLASS_SERIAL_CANBUS,               "CANBUS",                                   LW_NULL,    0},
    /*
     *  Wireless controller
     */
    {PCI_CLASS_WIRELESS_IRDA,               "IRDA controller",                          LW_NULL,    0},
    {PCI_CLASS_WIRELESS_CONSUMER_IR,        "Consumer IR controller",                   LW_NULL,    0},
    {PCI_CLASS_WIRELESS_RF_CONTROLLER,      "RF controller",                            LW_NULL,    1},
    {PCI_CLASS_WIRELESS_BLUETOOTH,          "Bluetooth",                                LW_NULL,    0},
    {PCI_CLASS_WIRELESS_BROADBAND,          "Broadband",                                LW_NULL,    0},
    {PCI_CLASS_WIRELESS_802_1A,             "802.1a controller",                        LW_NULL,    0},
    {PCI_CLASS_WIRELESS_802_1B,             "802.1b controller",                        LW_NULL,    0},
    {PCI_CLASS_WIRELESS_OTHER,              "Wireless controller",                      LW_NULL,    0},
    /*
     *  Intelligent controller
     */
    {PCI_CLASS_INTELLIGENT_I2O,             "I2O",                                      LW_NULL,    0},
    /*
     *  Satellite communications controller
     */
    {PCI_CLASS_SATELLITE_TV,                "Satellite TV controller",                  LW_NULL,    0},
    {PCI_CLASS_SATELLITE_AUDIO,             "Satellite audio communication controller", LW_NULL,    0},
    {PCI_CLASS_SATELLITE_VOICE,             "Satellite voice communication controller", LW_NULL,    0},
    {PCI_CLASS_SATELLITE_DATA,              "Satellite data communication controller",  LW_NULL,    0},
    /*
     *  Encryption controller
     */
    {PCI_CLASS_CRYPT_NETWORK,               "Network and computing encryption device",  LW_NULL,    0},
    {PCI_CLASS_CRYPT_ENTERTAINMENT,         "Entertainment encryption device",          LW_NULL,    0},
    {PCI_CLASS_CRYPT_OTHER,                 "Encryption controller",                    LW_NULL,    0},
    /*
     *  Signal processing controller
     */
    {PCI_CLASS_SP_DPIO,                     "DPIO module",                              LW_NULL,    0},
    {PCI_CLASS_SP_PERF_CTR,                 "Performance counters",                     LW_NULL,    0},
    {PCI_CLASS_SP_SYNCHRONIZER,             "Communication synchronizer",               LW_NULL,    0},
    {PCI_CLASS_SP_MANAGE,                   "Signal processing management",             LW_NULL,    0},
    {PCI_CLASS_SP_OTHER,                    "Signal processing controller",             LW_NULL,    0},
    /*
     *  Processing accelerators
     */
    {PCI_CLASS_PROCESS_ACCELER,             "Processing accelerators",                  LW_NULL,    0},
};
/*********************************************************************************************************
  总线 prog if 信息
*********************************************************************************************************/
static PCI_CLASS_PROGIF _G_pcpPciProgIfTable[] = {
    /*
     *  IDE interface
     */
    {PCI_CLASS_STORAGE_IDE_OPERATING_PRIMARY,       "Operating mode (primary)"          },
    {PCI_CLASS_STORAGE_IDE_PROG_PRIMARY,            "Programmable indicator (primary)"  },
    {PCI_CLASS_STORAGE_IDE_OPERATING_SECONDARY,     "Operating mode (secondary)"        },
    {PCI_CLASS_STORAGE_IDE_PROG_SECONDARY,          "Programmable indicator (secondary)"},
    {PCI_CLASS_STORAGE_IDE_MASTER_IDE_DEVICE,       "Master IDE device"                 },
    /*
     *  ATA controller
     */
    {PCI_CLASS_STORAGE_ATA_ADMA_SINGLE,             "ADMA single stepping"              },
    {PCI_CLASS_STORAGE_ATA_ADMA_CONTINUOUS,         "ADMA continuous operation"         },
    /*
     *  SATA controller
     */
    {PCI_CLASS_STORAGE_SATA_VENDOR_SPECIFIC,        "Vendor specific"                   },
    {PCI_CLASS_STORAGE_SATA_AHCI,                   "AHCI 1.0"                          },
    {PCI_CLASS_STORAGE_SATA_SERIAL_STORAGE_BUS,     "Serial Storage Bus"                },
    /*
     *  Serial Attached SCSI controller
     */
    {PCI_CLASS_STORAGE_SAS_SERIAL_STORAGE_BUS,      "Serial Storage Bus"                },
    /*
     *  Non-Volatile memory controller
     */
    {PCI_CLASS_STORAGE_NVM_HCI,                     "NVMHCI"                            },
    {PCI_CLASS_STORAGE_NVM_EXPRESS,                 "NVM Express"                       },
    /*
     *  VGA compatible controller
     */
    {PCI_CLASS_DISPLAY_VGA_VGA_CONTROLLER,          "VGA controller"                    },
    {PCI_CLASS_DISPLAY_VGA_8514_CONTROLLER,         "8514 controller"                   },
    /*
     *  PCI bridge
     */
    {PCI_CLASS_BRIDGE_PCI_NORMAL,                   "Normal decode"                     },
    {PCI_CLASS_BRIDGE_PCI_SUBTRACTIVE,              "Subtractive decode"                },
    /*
     *  RACEway bridge
     */
    {PCI_CLASS_BRIDGE_RACEWAY_TRANSPARENT,          "Transparent mode"                  },
    {PCI_CLASS_BRIDGE_RACEWAY_ENDPOINT,             "Endpoint mode"                     },
    /*
     *  Semi-transparent PCI-to-PCI bridge
     */
    {PCI_CLASS_BRIDGE_PCI_SEMI_PRIMARY,             "Primary bus towards host CPU"      },
    {PCI_CLASS_BRIDGE_PCI_SEMI_SECONDARY,           "Secondary bus towards host CPU"    },
    /*
     *  Serial controller
     */
    {PCI_CLASS_COMMUNICATION_SERIAL_8250,           "8250"                              },
    {PCI_CLASS_COMMUNICATION_SERIAL_16450,          "16450"                             },
    {PCI_CLASS_COMMUNICATION_SERIAL_16550,          "16550"                             },
    {PCI_CLASS_COMMUNICATION_SERIAL_16650,          "16650"                             },
    {PCI_CLASS_COMMUNICATION_SERIAL_16750,          "16750"                             },
    {PCI_CLASS_COMMUNICATION_SERIAL_16850,          "16850"                             },
    {PCI_CLASS_COMMUNICATION_SERIAL_16950,          "16950"                             },
    /*
     *  Parallel controller
     */
    {PCI_CLASS_COMMUNICATION_PARALLEL_SPP,          "SPP"                               },
    {PCI_CLASS_COMMUNICATION_PARALLEL_BIDIR,        "BiDir"                             },
    {PCI_CLASS_COMMUNICATION_PARALLEL_ECP,          "ECP"                               },
    {PCI_CLASS_COMMUNICATION_PARALLEL_IEEE1284,     "IEEE1284"                          },
    {PCI_CLASS_COMMUNICATION_PARALLEL_IEEE1284_T,   "IEEE1284 Target"                   },
    /*
     *  Modem
     */
    {PCI_CLASS_COMMUNICATION_MODEM_GENERIC,         "Generic"                           },
    {PCI_CLASS_COMMUNICATION_MODEM_HAYES_16450,     "Hayes/16450"                       },
    {PCI_CLASS_COMMUNICATION_MODEM_HAYES_16550,     "Hayes/16550"                       },
    {PCI_CLASS_COMMUNICATION_MODEM_HAYES_16650,     "Hayes/16650"                       },
    {PCI_CLASS_COMMUNICATION_MODEM_HAYES_16750,     "Hayes/16750"                       },
    /*
     *  PIC
     */
    {PCI_CLASS_SYSTEM_PIC_8259,                     "8259"                              },
    {PCI_CLASS_SYSTEM_PIC_ISA_PIC,                  "ISA PIC"                           },
    {PCI_CLASS_SYSTEM_PIC_EISA_PIC,                 "EISA PIC"                          },
    {PCI_CLASS_SYSTEM_PIC_IO_APIC,                  "IO-APIC"                           },
    {PCI_CLASS_SYSTEM_PIC_IOX_APIC,                 "IO(X)-APIC"                        },
    /*
     *  DMA controller
     */
    {PCI_CLASS_SYSTEM_DMA_8237,                     "8237"                              },
    {PCI_CLASS_SYSTEM_DMA_ISA_DAM,                  "ISA DMA"                           },
    {PCI_CLASS_SYSTEM_DMA_EISA_DMA,                 "EISA DMA"                          },
    /*
     *  Timer
     */
    {PCI_CLASS_SYSTEM_TIMER_8254,                   "8254"                              },
    {PCI_CLASS_SYSTEM_TIMER_ISA_TIMER,              "ISA Timer"                         },
    {PCI_CLASS_SYSTEM_TIMER_EISA_TIMER,             "EISA Timer"                        },
    {PCI_CLASS_SYSTEM_TIMER_HPET,                   "HPET"                              },
    /*
     *  RTC
     */
    {PCI_CLASS_SYSTEM_RTC_GENERIC,                  "Generic"                           },
    {PCI_CLASS_SYSTEM_RTC_ISA_RTC,                  "ISA RTC"                           },
    /*
     *  Gameport controller
     */
    {PCI_CLASS_INPUT_GAMEPORT_GENERIC,              "Generic"                           },
    {PCI_CLASS_INPUT_GAMEPORT_EXTENDED,             "Extended"                          },
    /*
     *  FireWire (IEEE 1394)
     */
    {PCI_CLASS_SERIAL_FIREWIRE_GENERIC,             "Generic"                           },
    {PCI_CLASS_SERIAL_FIREWIRE_OHCI,                "OHCI"                              },
    /*
     *  USB controller
     */
    {PCI_CLASS_SERIAL_USB_UHCI,                     "UHCI"                              },
    {PCI_CLASS_SERIAL_USB_OHCI,                     "OHCI"                              },
    {PCI_CLASS_SERIAL_USB_EHCI,                     "EHCI"                              },
    {PCI_CLASS_SERIAL_USB_XHCI,                     "XHCI"                              },
    {PCI_CLASS_SERIAL_USB_UNSPECIFIED,              "Unspecified"                       },
    {PCI_CLASS_SERIAL_USB_DEVICE,                   "USB Device"                        },
    /*
     *  RF controller
     */
    {PCI_CLASS_WIRELESS_WHCI,                       "WHCI"                              },
};
/*********************************************************************************************************
  总线 vendor 信息
*********************************************************************************************************/
static PCI_VENDOR_IDS       _G_pviPciVendorTable[] = {
    {PCI_VENDOR_ID_PEAK,                    "PEAK-System Technik GmbH",                 LW_NULL,    1},
    {PCI_VENDOR_ID_TTTECH,                  "TTTech Computertechnik AG (Wrong ID)",     LW_NULL,    1},
    {PCI_VENDOR_ID_DYNALINK,                "Dynalink",                                 LW_NULL,    1},
};
/*********************************************************************************************************
  总线 device 信息
*********************************************************************************************************/
static PCI_DEVICE_IDS       _G_pdiPciDeviceTable[] = {
    /*
     *  PEAK-System Technik GmbH
     */
    {PCI_DEVICE_ID_PEAK_PCAN,               "PCAN-PCI CAN-Bus controller",              LW_NULL,    2},
    /*
     *  TTTech Computertechnik AG (Wrong ID)
     */
    {PCI_DEVICE_ID_TTTECH_MC322,            "TTP-Monitoring Card V2.0",                 LW_NULL,    0},
};
/*********************************************************************************************************
  总线 subsystem 信息
*********************************************************************************************************/
static PCI_SUB_IDS          _G_psiPciSubTable[] = {
    /*
     *  PCAN-PCI CAN-Bus controller
     */
    {PCI_SUBVENDOR_ID_PEAK_PCAN_SJC1000, PCI_SUBDEVICE_ID_PEAK_PCAN_SJC1000,
     "2 Channel CAN Bus SJC1000"},
    {PCI_SUBVENDOR_ID_PEAK_PCAN_SJC1000_OI, PCI_SUBDEVICE_ID_PEAK_PCAN_SJC1000_OI,
     "2 Channel CAN Bus SJC1000 (Optically Isolated)"},
};
/*********************************************************************************************************
  PCI 事例类型根节点
*********************************************************************************************************/
static INT  _G_iPciClassTableNum;
static INT  _G_iPciIdTableNum;
/*********************************************************************************************************
** 函数名称: API_PciDbInit
** 功能描述: 初始化 pci 总线信息数据库
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PciDbInit (VOID)
{
    INT                 i, j;
    INT                 iClassMax    = sizeof(_G_pccPciClassTable ) / sizeof(PCI_CLASS_CODE);
    INT                 iIdMax       = sizeof(_G_pviPciVendorTable) / sizeof(PCI_VENDOR_IDS);
    
    PCI_CLASS_CODE     *pccClassCode = _G_pccPciClassTable;
    PCI_CLASS_SUB      *pcsClassSub  = _G_pcsPciSubClassTable;
    PCI_CLASS_PROGIF   *pcpifProgIf  = _G_pcpPciProgIfTable;
    
    PCI_VENDOR_IDS     *pviVendorId  = _G_pviPciVendorTable;
    PCI_DEVICE_IDS     *pdiDeviceId  = _G_pdiPciDeviceTable;
    PCI_SUB_IDS        *psiSubId     = _G_psiPciSubTable;

    for (i = 0; i < iClassMax; i++) {
        if (pccClassCode[i].CC_usTableNum) {
            pccClassCode[i].CC_pcsTable = pcsClassSub;
            
            for (j = 0; j < pccClassCode[i].CC_usTableNum; j++) {
                if (pcsClassSub[j].CS_usTableNum) {
                    pcsClassSub[j].CS_pcpTable = pcpifProgIf;
                    
                    pcpifProgIf += pcsClassSub[j].CS_usTableNum;
                }
            }
            pcsClassSub += pccClassCode[i].CC_usTableNum;
        }
    }
    
    _G_iPciClassTableNum = iClassMax;

    for (i = 0; i < iIdMax; i++) {
        if (pviVendorId[i].VI_usDeviceTableNum) {
            pviVendorId[i].VI_pdiDeviceTable = pdiDeviceId;

            for (j = 0; j < pviVendorId[i].VI_usDeviceTableNum; j++) {
                if (pdiDeviceId[j].DI_usSubTableNum) {
                    pdiDeviceId[j].DI_psiSubTable = psiSubId;

                    psiSubId += pdiDeviceId[j].DI_usSubTableNum;
                }
            }
            pdiDeviceId += pviVendorId[i].VI_usDeviceTableNum;
        }
    }

    _G_iPciIdTableNum = iIdMax;
}
/*********************************************************************************************************
** 函数名称: API_PciDbGetClassStr
** 功能描述: 通过类型查询 PCI 事例信息
** 输　入  : ucClass       类型信息
**           ucSubClass    子类型
**           ucProgIf      prog if
**           pcBuffer      输出缓冲
**           stSize        缓冲区大小
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PciDbGetClassStr (UINT8   ucClass,
                            UINT8   ucSubClass,
                            UINT8   ucProgIf,
                            PCHAR   pcBuffer,
                            size_t  stSize)
{
    INT                 i, j, k;
    PCHAR               pcClass = "", pcSubClass = "", pcProgIf = "";
    PCI_CLASS_CODE     *pccClassCode;
    PCI_CLASS_SUB      *pcsClassSub;
    PCI_CLASS_PROGIF   *pcpifProgIf;
    UINT16              usClassSub = (UINT16)((ucClass <<  8) + ucSubClass);
    UINT32              uiProgIf   = (UINT32)((ucClass << 16) + (ucSubClass << 8) + ucProgIf);
    
    for (i = 0; i < _G_iPciClassTableNum; i++) {
        pccClassCode = &_G_pccPciClassTable[i];
        
        if (pccClassCode->CC_ucClassCode == ucClass) {
            pcClass = pccClassCode->CC_pcClassName;
            
            for (j = 0; j < pccClassCode->CC_usTableNum; j++) {
                pcsClassSub = &pccClassCode->CC_pcsTable[j];
                
                if (pcsClassSub->CS_ucSubClass == usClassSub) {
                    pcSubClass = pcsClassSub->CS_pcSubName;
                    
                    for (k = 0; k < pcsClassSub->CS_usTableNum; k++) {
                        pcpifProgIf = &pcsClassSub->CS_pcpTable[k];
                    
                        if (pcpifProgIf->CP_uiProgIf == uiProgIf) {
                            pcProgIf = pcpifProgIf->CS_pcProgIfName;
                            break;
                        }
                    }
                    break;
                }
            }
            break;
        }
    }
    
    bnprintf(pcBuffer, stSize, 0, 
             "Class %d [%s] Sub %d [%s] Prog-if %d [%s]", 
             ucClass, pcClass, ucSubClass, pcSubClass, ucProgIf, pcProgIf);
}
/*********************************************************************************************************
** 函数名称: API_PciDbGetIdStr
** 功能描述: 通过类型查询 PCI 厂商信息
** 输　入  : usVendor           厂商 ID
**           ucDevice           设备 ID
**           ucSubVendor        厂商子 ID
**           ucSubSystem        设备子 ID
**           pcBuffer           输出缓冲
**           stSize             缓冲区大小
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PciDbGetIdStr (UINT16  usVendor,
                         UINT16  ucDevice,
                         UINT16  ucSubVendor,
                         UINT16  ucSubSystem,
                         PCHAR   pcBuffer,
                         size_t  stSize)
{
    INT                 i, j, k;
    PCHAR               pcVendor = "", pcDevice = "", pcSubSystem = "";
    PCI_VENDOR_IDS     *pviVendor;
    PCI_DEVICE_IDS     *pdiDevice;
    PCI_SUB_IDS        *psiSub;

    for (i = 0; i < _G_iPciIdTableNum; i++) {
        pviVendor = &_G_pviPciVendorTable[i];

        if (pviVendor->VI_usVendorId == usVendor) {
            pcVendor = pviVendor->VI_pcVendorName;

            for (j = 0; j < pviVendor->VI_usDeviceTableNum; j++) {
                pdiDevice = &pviVendor->VI_pdiDeviceTable[j];

                if (pdiDevice->DI_usDeviceId == ucDevice) {
                    pcDevice = pdiDevice->DI_pcDeviceName;

                    for (k = 0; k < pdiDevice->DI_usSubTableNum; k++) {
                        psiSub = &pdiDevice->DI_psiSubTable[k];

                        if ((psiSub->SI_usSubVendorId == ucSubVendor) &&
                            (psiSub->SI_usSubDeviceId == ucSubSystem)) {
                            pcSubSystem = psiSub->SI_pcSubSystemName;
                            break;
                        }
                    }
                    break;
                }
            }
            break;
        }
    }

    if (i >= _G_iPciIdTableNum) {
        return;
    }

    bnprintf(pcBuffer, stSize, 0,
             "Vendor ID %04x Vendor Name [%s] Device ID %04x Device Name [%s] "
             "SubVendor ID %04x SubSystem ID %04x SubSystem Name [%s]",
             usVendor, pcVendor, ucDevice, pcDevice, ucSubVendor, ucSubSystem, pcSubSystem);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

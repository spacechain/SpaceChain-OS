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
** 文   件   名: nvme.h
**
** 创   建   人: Hui.Kai (惠凯)
**
** 文件创建日期: 2017 年 7 月 17 日
**
** 描        述: NVMe 驱动.
*********************************************************************************************************/

#ifndef __NVME_H
#define __NVME_H

#include "sys/cdefs.h"
#include "nvmeCfg.h"

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_NVME_EN > 0)

#include "linux/types.h"

/*********************************************************************************************************
  驱动选项操作命令
*********************************************************************************************************/
#define NVME_OPT_CMD_BASE_ADDR_GET          0x100                       /* 获取控制器基地址             */
#define NVME_OPT_CMD_BASE_ADDR_SET          0x101                       /* 设置控制器基地址             */

#define NVME_OPT_CMD_BASE_SIZE_GET          0x102                       /* 获取控制器基地址范围         */
#define NVME_OPT_CMD_BASE_SIZE_SET          0x103                       /* 设置控制器基地址范围         */

#define NVME_OPT_CMD_EXT_ARG_GET            0x108                       /* 获取扩展参数                 */
#define NVME_OPT_CMD_EXT_ARG_SET            0x109                       /* 设置扩展参数                 */

#define NVME_OPT_CMD_SECTOR_SIZE_GET        0x10a                       /* 获取扇区大小                 */
#define NVME_OPT_CMD_SECTOR_SIZE_SET        0x10b                       /* 设置扇区大小                 */

#define NVME_OPT_CMD_CACHE_BURST_RD_GET     0x10e                       /* 获取读猝发扇区数             */
#define NVME_OPT_CMD_CACHE_BURST_RD_SET     0x10f                       /* 设置读猝发扇区数             */

#define NVME_OPT_CMD_CACHE_BURST_WR_GET     0x110                       /* 获取写猝发扇区数             */
#define NVME_OPT_CMD_CACHE_BURST_WR_SET     0x111                       /* 设置写猝发扇区数             */

#define NVME_OPT_CMD_CACHE_SIZE_GET         0x112                       /* 获取扇区缓冲区大小           */
#define NVME_OPT_CMD_CACHE_SIZE_SET         0x113                       /* 设置扇区缓冲区大小           */

#define NVME_OPT_CMD_CACHE_PL_GET           0x114                       /* 获取扇区缓冲区并发操作线程数 */
#define NVME_OPT_CMD_CACHE_PL_SET           0x115                       /* 设置扇区缓冲区并发操作线程数 */

#define NVME_OPT_CMD_DC_MSG_COUNT_GET       0x11e                       /* Disk cache 消息数量          */
#define NVME_OPT_CMD_DC_PARALLEL_EN_GET     0x11f                       /* Disk cache 并行操作是否使能  */
/*********************************************************************************************************
  NVMe 控制器寄存器
*********************************************************************************************************/
#define NVME_REG_CAP                        0x0000                      /* Controller Capabilities      */
#define NVME_REG_VS                         0x0008                      /* Version                      */
#define NVME_REG_INTMS                      0x000c                      /* Interrupt Mask Set           */
#define NVME_REG_INTMC                      0x0010                      /* Interrupt Mask Set           */
#define NVME_REG_CC                         0x0014                      /* Controller Configuration     */
#define NVME_REG_CSTS                       0x001c                      /* Controller Status            */
#define NVME_REG_NSSR                       0x0020                      /* NVM Subsystem Reset          */
#define NVME_REG_AQA                        0x0024                      /* Admin Queue Attributes       */
#define NVME_REG_ASQ                        0x0028                      /* Admin SQ Base Address        */
#define NVME_REG_ACQ                        0x0030                      /* Admin SQ Base Address        */
#define NVME_REG_CMBLOC                     0x0038                      /* Controller Memory Buffer     */
                                                                        /* Location                     */
#define NVME_REG_CMBSZ                      0x003c                      /* Controller Memory Buffer Size*/
#define NVME_REG_SQ0TDBL                    0x1000                      /* Submission Queue 0 Tail DB   */
/*********************************************************************************************************
  NVMe 控制器 Capabilities 获取操作
*********************************************************************************************************/
#define NVME_CAP_MQES(cap)                  ((cap) & 0xffff)            /* Max Queue Entries Supported  */
#define NVME_CAP_TIMEOUT(cap)               (((cap) >> 24) & 0xff)      /* CSTS.RDY Timeout             */
#define NVME_CAP_STRIDE(cap)                (((cap) >> 32) & 0xf)       /* CSTS.RDY Timeout             */
#define NVME_CAP_NSSRC(cap)                 (((cap) >> 36) & 0x1)       /* NVM Subsystem Reset Supported*/
#define NVME_CAP_MPSMIN(cap)                (((cap) >> 48) & 0xf)       /* Memory Page Size Minimum     */
#define NVME_CAP_MPSMAX(cap)                (((cap) >> 52) & 0xf)       /* Memory Page Size Maximum     */
/*********************************************************************************************************
  NVMe 控制器 Memory Buffer 相关操作
*********************************************************************************************************/
#define NVME_CMB_BIR(cmbloc)                ((cmbloc) & 0x7)            /* Base Indicator Register      */
#define NVME_CMB_OFST(cmbloc)               (((cmbloc) >> 12) & 0xfffff)/* offset of the CMB            */
#define NVME_CMB_SZ(cmbsz)                  (((cmbsz) >> 12) & 0xfffff) /* size of the CMB              */
#define NVME_CMB_SZU(cmbsz)                 (((cmbsz) >> 8) & 0xf)      /* Size Units                   */
#define NVME_CMB_WDS(cmbsz)                 ((cmbsz) & 0x10)            /* Write Data Support           */
#define NVME_CMB_RDS(cmbsz)                 ((cmbsz) & 0x8)             /* Read Data Support            */
#define NVME_CMB_LISTS(cmbsz)               ((cmbsz) & 0x4)             /* PRP SGL List Support         */
#define NVME_CMB_CQS(cmbsz)                 ((cmbsz) & 0x2)             /* Completion Queue Support     */
#define NVME_CMB_SQS(cmbsz)                 ((cmbsz) & 0x1)             /* Submission Queue Support     */
/*********************************************************************************************************
  控制器寄存器读写 (PCI: 小端)
*********************************************************************************************************/
#define NVME_CTRL_READ(ctrl, reg)   \
        le32toh(read32((addr_t)((ULONG)(ctrl)->NVMECTRL_pvRegAddr + reg)))
#define NVME_CTRL_WRITE(ctrl, reg, val) \
        write32(htole32(val), (addr_t)((ULONG)(ctrl)->NVMECTRL_pvRegAddr + reg))

#define NVME_CTRL_RAW_READ(base, reg)   \
        le32toh(read32((addr_t)((ULONG)(base + reg))))
#define NVME_CTRL_RAW_WRITE(base, reg, val) \
        write32(htole32(val), (addr_t)((ULONG)(base + reg)))

#define NVME_CTRL_READQ(ctrl, reg)  \
        le64toh(read64((addr_t)((ULONG)(ctrl)->NVMECTRL_pvRegAddr + reg)))
#define NVME_CTRL_WRITEQ(ctrl, reg, val)    \
        write64(htole64(val), (addr_t)((ULONG)(ctrl)->NVMECTRL_pvRegAddr + reg))

#define NVME_CTRL_RAW_READQ(base, reg)  \
        le64toh(read64((addr_t)((ULONG)(base + reg))))
#define NVME_CTRL_RAW_WRITEQ(base, reg, val)    \
        write64(htole64(val), (addr_t)((ULONG)(base + reg)))
/*********************************************************************************************************
  控制器版本
*********************************************************************************************************/
#define NVME_VS(major, minor)               (((major) << 16) | ((minor) << 8))
/*********************************************************************************************************
  NVMe 控制器配置和状态
*********************************************************************************************************/
#define NVME_CC_ENABLE                      (1 << 0)                    /* Enable                       */
#define NVME_CC_CSS_NVM                     (0 << 4)                    /* I/O Command Set NVM          */
#define NVME_CC_MPS_SHIFT                   (7)                         /* Memory Page Size shfit       */
#define NVME_CC_ARB_RR                      (0 << 11)                   /* Round Robin                  */
#define NVME_CC_ARB_WRRU                    (1 << 11)                   /* Weighted Round Robin with    */
                                                                        /* Urgent Priority Class        */
#define NVME_CC_ARB_VS                      (7 << 11)                   /* Vendor Specific              */
#define NVME_CC_SHN_NONE                    (0 << 14)                   /* No notification; no effect   */
#define NVME_CC_SHN_NORMAL                  (1 << 14)                   /* Normal shutdown notification */
#define NVME_CC_SHN_ABRUPT                  (2 << 14)                   /* Abrupt shutdown notification */
#define NVME_CC_SHN_MASK                    (3 << 14)                   /* Shutdown Notification Mask   */
#define NVME_CC_IOSQES                      (6 << 16)                   /* I/O SQ Entry Size            */
#define NVME_CC_IOCQES                      (4 << 20)                   /* I/O CQ Entry Size            */
#define NVME_CSTS_RDY                       (1 << 0)                    /* the controller is ready      */
#define NVME_CSTS_CFS                       (1 << 1)                    /* Controller Fatal Status      */
#define NVME_CSTS_NSSRO                     (1 << 4)                    /* NVM Subsystem Reset Occurred */
#define NVME_CSTS_SHST_NORMAL               (0 << 2)                    /* Normal operation             */
#define NVME_CSTS_SHST_OCCUR                (1 << 2)                    /* Shutdown processing occurring*/
#define NVME_CSTS_SHST_CMPLT                (2 << 2)                    /* Shutdown processing complete */
#define NVME_CSTS_SHST_MASK                 (3 << 2)                    /* Shutdown Status Mask         */
/*********************************************************************************************************
  设备怪异行为
*********************************************************************************************************/
enum {
    /*
     * Prefers I/O aligned to a stripe size specified in a vendor
     * specific Identify field.
     */
    NVME_QUIRK_STRIPE_SIZE  = (1 << 0),

    /*
     * The controller doesn't handle Identify value others than 0 or 1
     * correctly.
     */
    NVME_QUIRK_IDENTIFY_CNS = (1 << 1)
};
/*********************************************************************************************************
  命令异常处理
*********************************************************************************************************/
#define POISON_POINTER_DELTA    0
#define CMD_CTX_BASE            ((PVOID)POISON_POINTER_DELTA)
#define CMD_CTX_CANCELLED       ((PVOID)(0x30c + (addr_t)CMD_CTX_BASE))
#define CMD_CTX_COMPLETED       ((PVOID)(0x310 + (addr_t)CMD_CTX_BASE))
#define CMD_CTX_INVALID         ((PVOID)(0x314 + (addr_t)CMD_CTX_BASE))
#define CMD_CTX_FLUSH           ((PVOID)(0x318 + (addr_t)CMD_CTX_BASE))
#define CMD_CTX_ABORT           ((PVOID)(0x318 + (addr_t)CMD_CTX_BASE))
/*********************************************************************************************************
  Power State Descriptor Data Structure
*********************************************************************************************************/
typedef struct nvme_id_power_state_cb {
    UINT16                    NVMEIDPOWERSTATE_usMaxPower;              /* Maximum Power                */
    UINT8                     NVMEIDPOWERSTATE_ucRsvd2;                 /* Reserved                     */
    UINT8                     NVMEIDPOWERSTATE_ucFlags;                 /* Flags                        */
    UINT32                    NVMEIDPOWERSTATE_uiEntryLat;              /* Entry Latency                */
    UINT32                    NVMEIDPOWERSTATE_uiExitLat;               /* Exit Latency                 */
    UINT8                     NVMEIDPOWERSTATE_ucReadTput;              /* Relative Read Throughput     */
    UINT8                     NVMEIDPOWERSTATE_ucReadLat;               /* Relative Read Latency        */
    UINT8                     NVMEIDPOWERSTATE_ucWriteTput;             /* Relative Write Throughput    */
    UINT8                     NVMEIDPOWERSTATE_ucWriteLat;              /* Relative Write Latency       */
    UINT16                    NVMEIDPOWERSTATE_usIdlePower;             /* Idle Power                   */
    UINT8                     NVMEIDPOWERSTATE_ucIdleScale;             /* Idle Power Scale             */
    UINT8                     NVMEIDPOWERSTATE_ucRsvd19;                /* Reserved                     */
    UINT16                    NVMEIDPOWERSTATE_usActivePower;           /* Active Power                 */
    UINT8                     NVMEIDPOWERSTATE_ucActiveWorkScale;       /* Active Power Workload ,Scale */
    UINT8                     NVMEIDPOWERSTATE_ucRsvd23[9];             /* Reserved                     */
} NVME_ID_POWER_STATE_CB;
typedef NVME_ID_POWER_STATE_CB    *NVME_ID_POWER_STATE_HANDLE;

#define NVME_PS_FLAGS_MAX_POWER_SCALE       (1 << 0)                    /* Max Power Scale              */
#define NVME_PS_FLAGS_NON_OP_STATE          (1 << 1)                    /* Non-Operational State        */
/*********************************************************************************************************
  Identify Controller Data Structure
*********************************************************************************************************/
typedef struct nvme_id_ctrl_cb {
    UINT16                    NVMEIDCTRL_usVid;                         /* PCI Vendor ID                */
    UINT16                    NVMEIDCTRL_usSsvid;                       /* PCI Subsystem Vendor ID      */
    CHAR                      NVMEIDCTRL_ucSn[20];                      /* Serial Number                */
    CHAR                      NVMEIDCTRL_ucMn[40];                      /* Model Number                 */
    CHAR                      NVMEIDCTRL_ucFr[8];                       /* Firmware Revision            */
    UINT8                     NVMEIDCTRL_ucRab;                         /* Recommended Arbitration Burst*/
    UINT8                     NVMEIDCTRL_ucIeee[3];                     /* IEEE OUI Identifier          */
    UINT8                     NVMEIDCTRL_ucMic;                         /* Controller Multi-Path I/O and*/
                                                                        /* Namespace Sharing Capabilitie*/
    UINT8                     NVMEIDCTRL_ucMdts;                        /* Maximum Data Transfer Size   */
    UINT16                    NVMEIDCTRL_usCntlid;                      /* Controller ID                */
    UINT32                    NVMEIDCTRL_uiVer;                         /* Version                      */
    UINT8                     NVMEIDCTRL_ucRsvd84[172];                 /* Reserved                     */
    UINT16                    NVMEIDCTRL_usOacs;                        /* Optional Admin CMD Support   */
    UINT8                     NVMEIDCTRL_ucAcl;                         /* Abort Command Limit          */
    UINT8                     NVMEIDCTRL_ucAerl;                        /* Async Event Request Limit    */
    UINT8                     NVMEIDCTRL_ucFrmw;                        /* Firmware Updates             */
    UINT8                     NVMEIDCTRL_ucLpa;                         /* Log Page Attributes          */
    UINT8                     NVMEIDCTRL_ucElpe;                        /* Error Log Page Entries       */
    UINT8                     NVMEIDCTRL_ucNpss;                        /* Number of Power State Support*/
    UINT8                     NVMEIDCTRL_ucAvscc;                       /* Admin Vendor Spec CMD Config */
    UINT8                     NVMEIDCTRL_ucApsta;                       /* Autonomous Power State       */
                                                                        /* Transition Attributes        */
    UINT16                    NVMEIDCTRL_usWctemp;                      /* Warn Composite Temp Threshold*/
    UINT16                    NVMEIDCTRL_usCctemp;                      /* Critical CompoTemperThreshold*/
    UINT8                     NVMEIDCTRL_ucRsvd270[242];                /* Reserved                     */
    UINT8                     NVMEIDCTRL_ucSqes;                        /* Submission Queue Entry Size  */
    UINT8                     NVMEIDCTRL_ucCqes;                        /* Completion Queue Entry Size  */
    UINT8                     NVMEIDCTRL_ucRsvd514[2];                  /* Reserved                     */
    UINT32                    NVMEIDCTRL_uiNn;                          /* Number of Namespaces         */
    UINT16                    NVMEIDCTRL_usOncs;                        /* Optional NVM Command Support */
    UINT16                    NVMEIDCTRL_usFuses;                       /* Fused Operation Support      */
    UINT8                     NVMEIDCTRL_ucFna;                         /* Format NVM Attributes        */
    UINT8                     NVMEIDCTRL_ucVwc;                         /* Volatile Write Cache         */
    UINT16                    NVMEIDCTRL_usAwun;                        /* Atomic Write Unit Normal     */
    UINT16                    NVMEIDCTRL_usAwupf;                       /* Atomic Write Unit Power Fail */
    UINT8                     NVMEIDCTRL_ucNvscc;                       /* NVM Vendor Specific cmd Cfg  */
    UINT8                     NVMEIDCTRL_ucRsvd531;                     /* Reserved                     */
    UINT16                    NVMEIDCTRL_usAcwu;                        /* Atomic Compare & Write Unit  */
    UINT8                     NVMEIDCTRL_ucRsvd534[2];                  /* Reserved                     */
    UINT32                    NVMEIDCTRL_uiSgls;                        /* SGL Support                  */
    UINT8                     NVMEIDCTRL_ucRsvd540[1508];               /* Reserved                     */
    NVME_ID_POWER_STATE_CB    NVMEIDCTRL_tPsd[32];                      /* Power State Descriptors      */
    UINT8                     NVMEIDCTRL_ucVs[1024];                    /* Vendor Specific              */
} NVME_ID_CTRL_CB;
typedef NVME_ID_CTRL_CB      *NVME_ID_CTRL_HANDLE;

#define NVME_CTRL_ONCS_COMPARE              (1 << 0)                    /* support Compare command      */
#define NVME_CTRL_ONCS_WRITE_UNCORRECTABLE  (1 << 1)                    /* support Uncorrectable command*/
#define NVME_CTRL_ONCS_DSM                  (1 << 2)                    /* spt Dataset Management cmd   */
#define NVME_CTRL_VWC_PRESENT               (1 << 0)                    /* volatile write cache present */
/*********************************************************************************************************
  LBA Format Data Structure, NVM Command Set Specific
*********************************************************************************************************/
typedef struct nvme_lbaf_cb {
    UINT16                    NVMELBAF_Ms;                              /* Metadata Size                */
    UINT8                     NVMELBAF_Ds;                              /* LBA Data Size                */
    UINT8                     NVMELBAF_Rp;                              /* Relative Performance         */
} NVME_LBAF_CB;
typedef NVME_LBAF_CB         *NVME_LBAF_HANDLE;
/*********************************************************************************************************
  Identify Namespace Data Structure, NVM Command Set Specific
*********************************************************************************************************/
typedef struct nvme_id_ns_cb {
    UINT64                    NVMEIDNS_ullNsze;                         /* Namespace Size               */
    UINT64                    NVMEIDNS_ullNcap;                         /* Namespace Capacity           */
    UINT64                    NVMEIDNS_ullNuse;                         /* Namespace Utilization        */
    UINT8                     NVMEIDNS_ucNsfeat;                        /* Namespace Features           */
    UINT8                     NVMEIDNS_ucNlbaf;                         /* Number of LBA Format         */
    UINT8                     NVMEIDNS_ucFlbas;                         /* Formatted LBA Size           */
    UINT8                     NVMEIDNS_ucMc;                            /* Metadata Capabilities        */
    UINT8                     NVMEIDNS_ucDpc;                           /* End-to-end Data Protect Cap  */
    UINT8                     NVMEIDNS_ucDps;                           /* End-to-end Data Protect Set  */
    UINT8                     NVMEIDNS_ucNmic;                          /* Namespace Multi-path I/O and */
                                                                        /* Namespace Sharing Capabilitie*/
    UINT8                     NVMEIDNS_ucRescap;                        /* Reservation Capabilities     */
    UINT8                     NVMEIDNS_ucFpi;                           /* Format Progress Indicator    */
    UINT8                     NVMEIDNS_ucRsvd33;                        /* Reserved                     */
    UINT16                    NVMEIDNS_usNawun;                         /* Ns Atomic Write Unit Normal  */
    UINT16                    NVMEIDNS_usNawupf;                        /* Ns Atomic Wrt Unit Power Fail*/
    UINT16                    NVMEIDNS_usNacwu;                         /* Ns Atomic Compare & Wrt Unit */
    UINT16                    NVMEIDNS_usNabsn;                         /* Ns Atomic Boundary Size Norma*/
    UINT16                    NVMEIDNS_usNabo;                          /* Ns Atomic Boundary Offset    */
    UINT16                    NVMEIDNS_usNabspf;                        /* Ns Atomic Bd Size Power Fail */
    UINT16                    NVMEIDNS_usRsvd46;                        /* Reserved                     */
    UINT64                    NVMEIDNS_ullNvmcap[2];                    /* NVM Capacity                 */
    UINT8                     NVMEIDNS_ucRsvd64[40];                    /* Reserved                     */
    UINT8                     NVMEIDNS_ucNguid[16];                     /* Ns Globally Unique Identifier*/
    UINT8                     NVMEIDNS_ucEui64[8];                      /* IEEE Extended Unique Id      */
    NVME_LBAF_CB              NVMEIDNS_tLbaf[16];                       /* LBA Format Support           */
    UINT8                     NVMEIDNS_ucRsvd192[192];                  /* Reserved                     */
    UINT8                     NVMEIDNS_ucVs[3712];                      /* Vendor Specific              */
} NVME_ID_NS_CB;
typedef NVME_ID_NS_CB        *NVME_ID_NS_HANDLE;

#define NVME_NS_FEAT_THIN                   (1 << 0)                    /* supports thin provisioning   */
#define NVME_NS_FLBAS_LBA_MASK              (0xf)                       /* supported LBA Formats mask   */
#define NVME_NS_FLBAS_META_EXT              (0x10)                      /* creat an extended data LBA   */
#define NVME_LBAF_RP_BEST                   (0)                         /* Best performance             */
#define NVME_LBAF_RP_BETTER                 (1)                         /* Better performance           */
#define NVME_LBAF_RP_GOOD                   (2)                         /* Good performance             */
#define NVME_LBAF_RP_DEGRADED               (3)                         /* Degraded performance         */
#define NVME_NS_DPC_PI_LAST                 (1 << 4)                    /* last eight bytes of metadata */
#define NVME_NS_DPC_PI_FIRST                (1 << 3)                    /* first eight bytes of metadata*/
#define NVME_NS_DPC_PI_TYPE3                (1 << 2)                    /* Protection Information Type 3*/
#define NVME_NS_DPC_PI_TYPE2                (1 << 1)                    /* Protection Information Type 2*/
#define NVME_NS_DPC_PI_TYPE1                (1 << 0)                    /* Protection Information Type 1*/
#define NVME_NS_DPS_PI_FIRST                (1 << 3)                    /* first eight bytes of metadata*/
#define NVME_NS_DPS_PI_MASK                 (0x7)                       /* Protection Information mask  */
#define NVME_NS_DPS_PI_TYPE1                (1)                         /* is enabled, Type 1           */
#define NVME_NS_DPS_PI_TYPE2                (2)                         /* is enabled, Type 2           */
#define NVME_NS_DPS_PI_TYPE3                (3)                         /* is enabled, Type 3           */
/*********************************************************************************************************
  Get Log Page – SMART / Health Information Log
*********************************************************************************************************/
typedef struct nvme_smart_log_cb {
    UINT8                     NVMESMARTLOG_ucCriticalWarning;           /* Critical Warning             */
    UINT8                     NVMESMARTLOG_ucTemperature[2];            /* Composite Temperature        */
    UINT8                     NVMESMARTLOG_ucAvailSpare;                /* Available Spare              */
    UINT8                     NVMESMARTLOG_ucSpareThresh;               /* Available Spare Threshold    */
    UINT8                     NVMESMARTLOG_ucPercentUsed;               /* Percentage Used              */
    UINT8                     NVMESMARTLOG_ucRsvd6[26];                 /* Reserve                      */
    UINT8                     NVMESMARTLOG_ucDataUnitsRead[16];         /* Data Units Read              */
    UINT8                     NVMESMARTLOG_ucDataUnitsWritten[16];      /* Data Units Written           */
    UINT8                     NVMESMARTLOG_ucHostReads[16];             /* Host Read Commands           */
    UINT8                     NVMESMARTLOG_ucHostWrites[16];            /* Host Write Commands          */
    UINT8                     NVMESMARTLOG_ucCtrlBusyTime[16];          /* Controller Busy Time         */
    UINT8                     NVMESMARTLOG_ucPowerCycles[16];           /* Power Cycles                 */
    UINT8                     NVMESMARTLOG_ucPowerOnHours[16];          /* Power On Hours               */
    UINT8                     NVMESMARTLOG_ucUnsafeShutdowns[16];       /* Unsafe Shutdowns             */
    UINT8                     NVMESMARTLOG_ucMediaErrors[16];           /* Media, Data Integrity Errors */
    UINT8                     NVMESMARTLOG_ucNumErrLogEntries[16];      /* Media and Data Integrity Er  */
    UINT32                    NVMESMARTLOG_uiWarningTempTime;           /* Warning Composite Temper Time*/
    UINT32                    NVMESMARTLOG_uiCriticalCompTime;          /* Critical Composite Tempe Time*/
    UINT16                    NVMESMARTLOG_uiTempSensor[8];             /* Temperature Sensor           */
    UINT8                     NVMESMARTLOG_ucRsvd216[296];              /* Reserve                      */
} NVME_SMART_LOG_CB;
typedef NVME_SMART_LOG_CB    *NVME_SMART_LOG_HANDLE;

#define NVME_SMART_CRIT_SPARE               (1 << 0)                    /* Critical Warning: spare space*/
#define NVME_SMART_CRIT_TEMPERATURE         (1 << 1)                    /* Critical Warning: temperature*/
#define NVME_SMART_CRIT_RELIABILITY         (1 << 2)                    /* Critical Warning: reliability*/
#define NVME_SMART_CRIT_MEDIA               (1 << 3)                    /* Critical Warning: media      */
#define NVME_SMART_CRIT_VOLATILE_MEMORY     (1 << 4)                    /* Critical Warning: memory     */

#define NVME_AER_NOTICE_NS_CHANGED          0x0002
/*********************************************************************************************************
  LBA Range Type – Data Structure Entry
*********************************************************************************************************/
typedef struct nvme_lba_range_type_cb {
    UINT8                     NVMELBARANGETYPE_ucType;                  /* Type of the LBA range        */
    UINT8                     NVMELBARANGETYPE_ucAttributes;            /* attributes of the LBA range  */
    UINT8                     NVMELBARANGETYPE_ucRsvd2[14];             /* Reserved                     */
    UINT64                    NVMELBARANGETYPE_ullSlba;                 /* Starting LBA                 */
    UINT64                    NVMELBARANGETYPE_ullNlb;                  /* Number of Logical Blocks     */
    UINT8                     NVMELBARANGETYPE_ucGuid[16];              /* Unique Identifier            */
    UINT8                     NVMELBARANGETYPE_ucRsvd48[16];            /* Reserved                     */
} NVME_LBA_RANGE_TYPE_CB;
typedef NVME_LBA_RANGE_TYPE_CB    *NVME_LBA_RANGE_TYPE_HANDLE;

#define NVME_LBART_TYPE_FS                  (0x01)                      /* Filesystem                   */
#define NVME_LBART_TYPE_RAID                (0x02)                      /* RAID                         */
#define NVME_LBART_TYPE_CACHE               (0x03)                      /* Cache                        */
#define NVME_LBART_TYPE_SWAP                (0x04)                      /* Page / swap file             */
#define NVME_LBART_ATTRIB_TEMP              (1 << 0)                    /* may be overwritten           */
#define NVME_LBART_ATTRIB_HIDE              (1 << 1)                    /* hidden from the OS/EFI/BIOS  */
/*********************************************************************************************************
  Reservation Status Data Structure
*********************************************************************************************************/
struct reg_ctl_ds_cb {
    UINT16                    REGCTLDS_uiCntlid;                        /* Controller ID                */
    UINT8                     REGCTLDS_ucRcsts;                         /* Reservation Status           */
    UINT8                     REGCTLDS_ucResv3[5];                      /* Reserved                     */
    UINT64                    REGCTLDS_ullHostid;                       /* Host Identifier              */
    UINT64                    REGCTLDS_ullRkey;                         /* Reservation Key              */
};

typedef struct nvme_reservation_status_cb {
    UINT32                    NVMERSVSTATUS_uiGen;                      /* Generation                   */
    UINT8                     NVMERSVSTATUS_ucRtype;                    /* Reservation Type             */
    UINT8                     NVMERSVSTATUS_ucRegctl[2];                /* Number of Registered Ctrllor */
    UINT8                     NVMERSVSTATUS_ucResv5[2];                 /* Reserved                     */
    UINT8                     NVMERSVSTATUS_ucPtpls;                    /* Persist Through Power Loss St*/
    UINT8                     NVMERSVSTATUS_ucResv10[13];               /* Reserved                     */
    struct reg_ctl_ds_cb      REG_CTL_DS_CB __flexarr;                  /* Reged Ctroller DataStructure */
} NVME_RSV_STATUS_CB;
typedef NVME_RSV_STATUS_CB   *NVME_RSV_STATUS_HANDLE;
/*********************************************************************************************************
  Opcodes for NVM Commands
*********************************************************************************************************/
#define NVME_CMD_FLUSH                      0x00                        /* Flush                        */
#define NVME_CMD_WRITE                      0x01                        /* Write                        */
#define NVME_CMD_READ                       0x02                        /* Read                         */
#define NVME_CMD_WRITE_UNCOR                0x04                        /* Write Uncorrectable          */
#define NVME_CMD_COMPARE                    0x05                        /* Compare                      */
#define NVME_CMD_WRITE_ZEROES               0x08                        /* Write Zeroes                 */
#define NVME_CMD_DSM                        0x09                        /* Dataset Management           */
#define NVME_CMD_RESV_REGISTER              0x0d                        /* Reservation Register         */
#define NVME_CMD_RESV_REPORT                0x0e                        /* Reservation Report           */
#define NVME_CMD_RESV_ACQUIRE               0x11                        /* Reservation Acquire          */
#define NVME_CMD_RESV_RELEASE               0x15                        /* Reservation Release          */
/*********************************************************************************************************
  Common Command Structure
*********************************************************************************************************/
typedef struct nvme_common_command_cb {
    UINT8                     NVMECOMMONCMD_ucOpcode;                   /* Opcode                       */
    UINT8                     NVMECOMMONCMD_ucFlags;                    /* Flags                        */
    UINT16                    NVMECOMMONCMD_usCmdId;                    /* Command Identifier           */
    UINT32                    NVMECOMMONCMD_uiNsid;                     /* Namespace Identifier         */
    UINT32                    NVMECOMMONCMD_uiCdw2[2];                  /* Command Dword                */
    UINT64                    NVMECOMMONCMD_ullMetadata;                /* Metadata Pointer             */
    UINT64                    NVMECOMMONCMD_ullPrp1;                    /* PRP Entry 1                  */
    UINT64                    NVMECOMMONCMD_ullPrp2;                    /* PRP Entry 2                  */
    UINT32                    NVMECOMMONCMD_uiCdw10[6];                 /* Command Dword                */
} NVME_COMMON_CMD_CB;
typedef NVME_COMMON_CMD_CB   *NVME_COMMON_CMD_HANDLE;
/*********************************************************************************************************
  Read and Write Command Structure
*********************************************************************************************************/
typedef struct nvme_rw_command_cb {
    UINT8                     NVMERWCMD_ucOpcode;                       /* Opcode                       */
    UINT8                     NVMERWCMD_ucFlags;                        /* Flags                        */
    UINT16                    NVMERWCMD_usCmdId;                        /* Command Identifier           */
    UINT32                    NVMERWCMD_uiNsid;                         /* Namespace Identifier         */
    UINT64                    NVMERWCMD_ullRsvd2;                       /* Reserved                     */
    UINT64                    NVMERWCMD_ullMetadata;                    /* Metadata Pointer             */
    UINT64                    NVMERWCMD_ullPrp1;                        /* PRP Entry 1                  */
    UINT64                    NVMERWCMD_ullPrp2;                        /* PRP Entry 2                  */
    UINT64                    NVMERWCMD_ullSlba;                        /* Starting LBA                 */
    UINT16                    NVMERWCMD_usLength;                       /* Number of Logical Blocks     */
    UINT16                    NVMERWCMD_usControl;                      /* Control                      */
    UINT32                    NVMERWCMD_uiDsmgmt;                       /* Dataset Management           */
    UINT32                    NVMERWCMD_uiReftag;                       /* Logical Block Reference Tag  */
    UINT16                    NVMERWCMD_usApptag;                       /* Expected Logical Blk App Tag */
    UINT16                    NVMERWCMD_usAppmask;                      /* Expected Log Blk App Tag Mask*/
} NVME_RW_CMD_CB;
typedef NVME_RW_CMD_CB       *NVME_RW_CMD_HANDLE;

#define NVME_RW_LR                          (1 << 15)                   /* Limited Retry                */
#define NVME_RW_FUA                         (1 << 14)                   /* Force Unit Access            */
#define NVME_RW_DSM_FREQ_UNSPEC             (0)                         /* No frequency info provided   */
#define NVME_RW_DSM_FREQ_TYPICAL            (1)                         /* Typical number of rd and wrt */
#define NVME_RW_DSM_FREQ_RARE               (2)                         /* Infrequent wrt and infre rd  */
#define NVME_RW_DSM_FREQ_READS              (3)                         /* Infrequent wrt and freq reads*/
#define NVME_RW_DSM_FREQ_WRITES             (4)                         /* Frequent wrt and infreq reads*/
#define NVME_RW_DSM_FREQ_RW                 (5)                         /* Frequent wrt and frequent rd */
#define NVME_RW_DSM_FREQ_ONCE               (6)                         /* One time read                */
#define NVME_RW_DSM_FREQ_PREFETCH           (7)                         /* Speculative read             */
#define NVME_RW_DSM_FREQ_TEMP               (8)                         /* going to be overwritten      */
#define NVME_RW_DSM_LATENCY_NONE            (0 << 4)                    /* No latency info provided     */
#define NVME_RW_DSM_LATENCY_IDLE            (1 << 4)                    /* Longer latency acceptable    */
#define NVME_RW_DSM_LATENCY_NORM            (2 << 4)                    /* Typical latency.             */
#define NVME_RW_DSM_LATENCY_LOW             (3 << 4)                    /* Smallest possible latency    */
#define NVME_RW_DSM_SEQ_REQ                 (1 << 6)                    /* be part of a sequential read */
#define NVME_RW_DSM_COMPRESSED              (1 << 7)                    /* not compressible for the blks*/
#define NVME_RW_PRINFO_PRCHK_REF            (1 << 10)                   /* Block Reference Tag field    */
#define NVME_RW_PRINFO_PRCHK_APP            (1 << 11)                   /* the Application Tag field    */
#define NVME_RW_PRINFO_PRCHK_GUARD          (1 << 12)                   /* the Guard field              */
#define NVME_RW_PRINFO_PRACT                (1 << 13)                   /* Protection Information Action*/
/*********************************************************************************************************
  Dataset Management command Structure
*********************************************************************************************************/
typedef struct nvme_dsm_cmd_cb {
    UINT8                     NVMEDSMCMD_ucOpcode;                      /* Opcode                       */
    UINT8                     NVMEDSMCMD_ucFlags;                       /* Flags                        */
    UINT16                    NVMEDSMCMD_usCmdId;                       /* Command Identifier           */
    UINT32                    NVMEDSMCMD_uiNsid;                        /* Namespace Identifier         */
    UINT64                    NVMEDSMCMD_ullRsvd2[2];                   /* Reserved                     */
    UINT64                    NVMEDSMCMD_ullPrp1;                       /* PRP Entry 1                  */
    UINT64                    NVMEDSMCMD_ullPrp2;                       /* PRP Entry 2                  */
    UINT32                    NVMEDSMCMD_uiNr;                          /* Number of Ranges             */
    UINT32                    NVMEDSMCMD_uiAttributes;                  /* Attribute                    */
    UINT32                    NVMEDSMCMD_uiRsvd12[4];                   /* Reserved                     */
} NVME_DSM_CMD_CB;
typedef NVME_DSM_CMD_CB      *NVME_DSM_CMD_HANDLE;

#define NVME_DSMGMT_IDR                     (1 << 0)                    /* Integral Dataset for Read    */
#define NVME_DSMGMT_IDW                     (1 << 1)                    /* Integral Dataset for Write   */
#define NVME_DSMGMT_AD                      (1 << 2)                    /* Attribute – Deallocate       */

typedef struct nvme_dsm_range_cb {
    UINT32          NVMEDSMRANGE_uiCattr;                               /* Context Attributes           */
    UINT32          NVMEDSMRANGE_uiNlb;                                 /* Length in logical blocks     */
    UINT64          NVMEDSMRANGE_uiSlba;                                /* Length in logical blocks     */
} NVME_DSM_RANGE_CB;
typedef NVME_DSM_RANGE_CB    *NVME_DSM_RANGE_HANDLE;
/*********************************************************************************************************
  Opcodes for Admin Commands
*********************************************************************************************************/
#define NVME_ADMIN_DELETE_SQ                0x00                        /* Delete I/O Submission Queue  */
#define NVME_ADMIN_CREATE_SQ                0x01                        /* Create I/O Submission Queue  */
#define NVME_ADMIN_GET_LOG_PAGE             0x02                        /* Get Log Page                 */
#define NVME_ADMIN_DELETE_CQ                0x04                        /* Delete I/O Completion Queue  */
#define NVME_ADMIN_CREATE_CQ                0x05                        /* Create I/O Completion Queue  */
#define NVME_ADMIN_IDENTIFY                 0x06                        /* Identify                     */
#define NVME_ADMIN_ABORT_CMD                0x08                        /* Abort                        */
#define NVME_ADMIN_SET_FEATURES             0x09                        /* Set Features                 */
#define NVME_ADMIN_GET_FEATURES             0x0a                        /* Get Features                 */
#define NVME_ADMIN_ASYNC_EVENT              0x0c                        /* Asynchronous Event Request   */
#define NVME_ADMIN_ACTIVATE_FW              0x10                        /* Firmware Commit              */
#define NVME_ADMIN_DOWNLOAD_FW              0x11                        /* Firmware Image Download      */
#define NVME_ADMIN_FORMAT_NVM               0x80                        /* Format NVM                   */
#define NVME_ADMIN_SECURITY_SEND            0x81                        /* Security Send                */
#define NVME_ADMIN_SECURITY_RECV            0x82                        /* Security Receive             */

typedef struct nvme_identify_cb {
    UINT8                     NVMEIDENTIFY_ucOpcode;                    /* Opcode                       */
    UINT8                     NVMEIDENTIFY_ucFlags;                     /* Flags                        */
    UINT16                    NVMEIDENTIFY_usCmdId;                     /* Command Identifier           */
    UINT32                    NVMEIDENTIFY_uiNsid;                      /* Namespace Identifier         */
    UINT64                    NVMEIDENTIFY_ullRsvd2[2];                 /* Reserved                     */
    UINT64                    NVMEIDENTIFY_ullPrp1;                     /* PRP Entry 1                  */
    UINT64                    NVMEIDENTIFY_ullPrp2;                     /* PRP Entry 2                  */
    UINT32                    NVMEIDENTIFY_uiCns;                       /* Controller or Ns Structure   */
    UINT32                    NVMEIDENTIFY_uiRsvd11[5];                 /* Reserved                     */
} NVME_IDENTIFY_CB;
typedef NVME_IDENTIFY_CB     *NVME_IDENTIFY_HANDLE;

typedef struct nvme_features_cb {
    UINT8                     NVMEFEATURE_ucOpcode;                     /* Opcode                       */
    UINT8                     NVMEFEATURE_ucFlags;                      /* Flags                        */
    UINT16                    NVMEFEATURE_usCmdId;                      /* Command Identifier           */
    UINT32                    NVMEFEATURE_uiSsid;                       /* Namespace Identifier         */
    UINT64                    NVMEFEATURE_ullRsvd2[2];                  /* Reserved                     */
    UINT64                    NVMEFEATURE_ullPrp1;                      /* PRP Entry 1                  */
    UINT64                    NVMEFEATURE_ullPrp2;                      /* PRP Entry 2                  */
    UINT32                    NVMEFEATURE_uiFid;                        /* Feature Identifier           */
    UINT32                    NVMEFEATURE_uiDword11;                    /* Command Dword                */
    UINT32                    NVMEFEATURE_uiRsvd12[4];                  /* Reserved                     */
} NVME_FEATURE_CB;
typedef NVME_FEATURE_CB      *NVME_FEATURE_HANDLE;

typedef struct nvme_create_cq_cb {
    UINT8                     NVMECREATECQ_ucOpcode;                    /* Opcode                       */
    UINT8                     NVMECREATECQ_ucFlags;                     /* Flags                        */
    UINT16                    NVMECREATECQ_usCmdId;                     /* Command Identifier           */
    UINT32                    NVMECREATECQ_uiRsvd1[5];                  /* Command Dword                */
    UINT64                    NVMECREATECQ_ullPrp1;                     /* PRP Entry 1                  */
    UINT64                    NVMECREATECQ_ullRsvd8;                    /* Reserved                     */
    UINT16                    NVMECREATECQ_usCqid;                      /* Queue Identifie              */
    UINT16                    NVMECREATECQ_usQsize;                     /* Queue Size                   */
    UINT16                    NVMECREATECQ_usCqFlags;                   /* Flags                        */
    UINT16                    NVMECREATECQ_usIrqVector;                 /* Interrupt Vector             */
    UINT32                    NVMECREATECQ_uiRsvd12[4];                 /* Reserved                     */
} NVME_CREATE_CQ_CB;
typedef NVME_CREATE_CQ_CB    *NVME_CREATE_CQ_HANDLE;

typedef struct nvme_create_sq_cb {
    UINT8                     NVMECREATESQ_ucOpcode;                    /* Opcode                       */
    UINT8                     NVMECREATESQ_ucFlags;                     /* Flags                        */
    UINT16                    NVMECREATESQ_usCmdId;                     /* Command Identifier           */
    UINT32                    NVMECREATESQ_uiRsvd1[5];                  /* Reserved                     */
    UINT64                    NVMECREATESQ_ullPrp1;                     /* PRP Entry 1                  */
    UINT64                    NVMECREATESQ_ullRsvd8;                    /* Reserved                     */
    UINT16                    NVMECREATESQ_usSqid;                      /* Queue Identifie              */
    UINT16                    NVMECREATESQ_usQsize;                     /* Queue Size                   */
    UINT16                    NVMECREATESQ_usSqFlags;                   /* Flags                        */
    UINT16                    NVMECREATESQ_usCqid;                      /* Completion Queue Identifier  */
    UINT32                    NVMECREATESQ_uiRsvd12[4];                 /* Reserved                     */
} NVME_CREATE_SQ_CB;
typedef NVME_CREATE_SQ_CB    *NVME_CREATE_SQ_HANDLE;

typedef struct nvme_delete_queue_cb {
    UINT8                     NVMEDELETEQUEUE_ucOpcode;                 /* Opcode                       */
    UINT8                     NVMEDELETEQUEUE_ucFlags;                  /* Flags                        */
    UINT16                    NVMEDELETEQUEUE_usCmdId;                  /* Command Identifier           */
    UINT32                    NVMEDELETEQUEUE_uiRsvd1[9];               /* Reserved                     */
    UINT16                    NVMEDELETEQUEUE_usQid;                    /* Queue Identifie              */
    UINT16                    NVMEDELETEQUEUE_usRsvd10;                 /* Reserved                     */
    UINT32                    NVMEDELETEQUEUE_uiRsvd11[5];              /* Reserved                     */
} NVME_DELETE_QUEUE_CB;
typedef NVME_DELETE_QUEUE_CB    *NVME_DELETE_QUEUE_HANDLE;

typedef struct nvme_abort_cmd_cb {
    UINT8                     NVMEABORTCMD_ucOpcode;                    /* Opcode                       */
    UINT8                     NVMEABORTCMD_ucFlags;                     /* Flags                        */
    UINT16                    NVMEABORTCMD_usCmdId;                     /* Command Identifier           */
    UINT32                    NVMEABORTCMD_uiRsvd1[9];                  /* Reserved                     */
    UINT16                    NVMEABORTCMD_usSqid;                      /* Submission Queue Identifier  */
    UINT16                    NVMEABORTCMD_usCid;                       /* Command Identifier           */
    UINT32                    NVMEABORTCMD_uiRsvd11[5];                 /* Reserved                     */
} NVME_ABORT_CMD_CB;
typedef NVME_ABORT_CMD_CB    *NVME_ABORT_CMD_HANDLE;

typedef struct nvme_download_firmware_cb {
    UINT8                     NVMEDOWNLOADFIRMWARE_ucOpcode;            /* Opcode                       */
    UINT8                     NVMEDOWNLOADFIRMWARE_ucFlags;             /* Flags                        */
    UINT16                    NVMEDOWNLOADFIRMWARE_usCmdId;             /* Command Identifier           */
    UINT32                    NVMEDOWNLOADFIRMWARE_uiRsvd1[5];          /* Reserved                     */
    UINT64                    NVMEDOWNLOADFIRMWARE_ullPrp1;             /* PRP Entry 1                  */
    UINT64                    NVMEDOWNLOADFIRMWARE_ullPrp2;             /* PRP Entry 2                  */
    UINT32                    NVMEDOWNLOADFIRMWARE_uiNumd;              /* Number of Dwords             */
    UINT32                    NVMEDOWNLOADFIRMWARE_uiOffset;            /* Offset                       */
    UINT32                    NVMEDOWNLOADFIRMWARE_uiRsvd12[4];         /* Reserved                     */
} NVME_DOWNLOAD_FIRMWARE_CB;
typedef NVME_DOWNLOAD_FIRMWARE_CB    *NVME_DOWNLOAD_FIRMWARE_HANDLE;

typedef struct nvme_format_cmd_cb {
    UINT8                     NVMEFORMATCMD_ucOpcode;                   /* Opcode                       */
    UINT8                     NVMEFORMATCMD_ucFlags;                    /* Flags                        */
    UINT16                    NVMEFORMATCMD_usCmdId;                    /* Command Identifier           */
    UINT32                    NVMEFORMATCMD_uiNsid;                     /* Namespace Identifier         */
    UINT64                    NVMEFORMATCMD_ullRsvd2[4];                /* Reserved                     */
    UINT32                    NVMEFORMATCMD_uiCdw10;                    /* Command Dword                */
    UINT32                    NVMEFORMATCMD_uiRsvd11[5];                /* Reserved                     */
} NVME_FORMAT_CMD_CB;
typedef NVME_FORMAT_CMD_CB   *NVME_FORMAT_CMD_HANDLE;

#define NVME_QUEUE_PHYS_CONTIG              (1 << 0)                    /* Physically Contiguous        */
#define NVME_CQ_IRQ_ENABLED                 (1 << 1)                    /* Interrupts Enabled           */
#define NVME_SQ_PRIO_URGENT                 (0 << 1)                    /* Queue Priority :Urgent       */
#define NVME_SQ_PRIO_HIGH                   (1 << 1)                    /* Queue Priority :High         */
#define NVME_SQ_PRIO_MEDIUM                 (2 << 1)                    /* Queue Priority :Urgent       */
#define NVME_SQ_PRIO_LOW                    (3 << 1)                    /* Queue Priority :Low          */
#define NVME_FEAT_ARBITRATION               0x01                        /* Arbitration                  */
#define NVME_FEAT_POWER_MGMT                0x02                        /* Power Management             */
#define NVME_FEAT_LBA_RANGE                 0x03                        /* LBA Range Type               */
#define NVME_FEAT_TEMP_THRESH               0x04                        /* Temperature Threshold        */
#define NVME_FEAT_ERR_RECOVERY              0x05                        /* Error Recovery               */
#define NVME_FEAT_VOLATILE_WC               0x06                        /* Volatile Write Cache         */
#define NVME_FEAT_NUM_QUEUES                0x07                        /* Number of Queues             */
#define NVME_FEAT_IRQ_COALESCE              0x08                        /* Interrupt Coalescing         */
#define NVME_FEAT_IRQ_CONFIG                0x09                        /* Interrupt Vector Configuratio*/
#define NVME_FEAT_WRITE_ATOMIC              0x0a                        /* Write Atomicity Normal       */
#define NVME_FEAT_ASYNC_EVENT               0x0b                        /* Asynchronous Event Confige   */
#define NVME_FEAT_AUTO_PST                  0x0c                        /* Autonomous Power St Transit  */
#define NVME_FEAT_SW_PROGRESS               0x80                        /* Software Progress Marker     */
#define NVME_FEAT_HOST_ID                   0x81                        /* Host Identifier              */
#define NVME_FEAT_RESV_MASK                 0x82                        /* Reservation Notification Mask*/
#define NVME_FEAT_RESV_PERSIST              0x83                        /* Reservation Persistance      */
#define NVME_LOG_ERROR                      0x01                        /* Error Information            */
#define NVME_LOG_SMART                      0x02                        /* SMART / Health Information   */
#define NVME_LOG_FW_SLOT                    0x03                        /* Firmware Slot Information    */
#define NVME_LOG_RESERVATION                0x80                        /* Reservation Notification     */
#define NVME_FWACT_REPL                     (0 << 3)                    /* replaces the image           */
#define NVME_FWACT_REPL_ACTV                (1 << 3)                    /* replaces and reset activated */
#define NVME_FWACT_ACTV                     (2 << 3)                    /* activated immediately        */
/*********************************************************************************************************
  NVMe Command
*********************************************************************************************************/
typedef struct nvme_command_cb {
    union {
        NVME_COMMON_CMD_CB            utCommonCmd;
        NVME_RW_CMD_CB                utRwCmd;
        NVME_IDENTIFY_CB              utIdentify;
        NVME_FEATURE_CB               utFeatures;
        NVME_CREATE_CQ_CB             utCreateCq;
        NVME_CREATE_SQ_CB             utCreateSq;
        NVME_DELETE_QUEUE_CB          utDeleteQueue;
        NVME_DOWNLOAD_FIRMWARE_CB     utDwnLoadFirmWare;
        NVME_FORMAT_CMD_CB            utFormat;
        NVME_DSM_CMD_CB               utDsm;
        NVME_ABORT_CMD_CB             utAbort;
    } u;
#define tCommonCmd                    u.utCommonCmd
#define tRwCmd                        u.utRwCmd
#define tIdentify                     u.utIdentify
#define tFeatures                     u.utFeatures
#define tCreateCq                     u.utCreateCq
#define tCreateSq                     u.utCreateSq
#define tDeleteQueue                  u.utDeleteQueue
#define tDwnLoadFirmWare              u.utDwnLoadFirmWare
#define tFormat                       u.utFormat
#define tDsm                          u.utDsm
#define tAbort                        u.utAbort
} NVME_COMMAND_CB;
typedef NVME_COMMAND_CB              *NVME_COMMAND_HANDLE;
/*********************************************************************************************************
  Status Code – Generic Command Status Values
*********************************************************************************************************/
#define NVME_SC_SUCCESS                     0x0                         /* Successful Completion        */
#define NVME_SC_INVALID_OPCODE              0x1                         /* Invalid Command Opcode       */
#define NVME_SC_INVALID_FIELD               0x2                         /* Invalid Field in Command     */
#define NVME_SC_CMDID_CONFLICT              0x3                         /* Command ID Conflict          */
#define NVME_SC_DATA_XFER_ERROR             0x4                         /* Data Transfer Error          */
#define NVME_SC_POWER_LOSS                  0x5                         /* Power Loss Notification      */
#define NVME_SC_INTERNAL                    0x6                         /* Internal Error               */
#define NVME_SC_ABORT_REQ                   0x7                         /* Command Abort Requested      */
#define NVME_SC_ABORT_QUEUE                 0x8                         /* Command Aborted: SQ Deletion */
#define NVME_SC_FUSED_FAIL                  0x9                         /* Cmd Aborted: Failed Fused Cmd*/
#define NVME_SC_FUSED_MISSING               0xa                         /* Cmd Aborted:Missing Fused Cmd*/
#define NVME_SC_INVALID_NS                  0xb                         /* Invalid Namespace or Format  */
#define NVME_SC_CMD_SEQ_ERROR               0xc                         /* Command Sequence Error       */
#define NVME_SC_SGL_INVALID_LAST            0xd                         /* Invalid SGL Segment Descript */
#define NVME_SC_SGL_INVALID_COUNT           0xe                         /* Invalid Number of SGL Des    */
#define NVME_SC_SGL_INVALID_DATA            0xf                         /* Data SGL Length Invalid      */
#define NVME_SC_SGL_INVALID_METADATA        0x10                        /* Metadata SGL Length Invalid  */
#define NVME_SC_SGL_INVALID_TYPE            0x11                        /* SGL Descriptor Type Invalid  */
#define NVME_SC_LBA_RANGE                   0x80                        /* LBA Out of Range             */
#define NVME_SC_CAP_EXCEEDED                0x81                        /* Capacity Exceeded            */
#define NVME_SC_NS_NOT_READY                0x82                        /* Namespace Not Ready          */
#define NVME_SC_RESERVATION_CONFLICT        0x83                        /* Reservation Conflict         */
#define NVME_SC_CQ_INVALID                  0x100                       /* Completion Queue Invalid     */
#define NVME_SC_QID_INVALID                 0x101                       /* Invalid Queue Identifier     */
#define NVME_SC_QUEUE_SIZE                  0x102                       /* Invalid Queue Size           */
#define NVME_SC_ABORT_LIMIT                 0x103                       /* Abort Command Limit Exceeded */
#define NVME_SC_ABORT_MISSING               0x104
#define NVME_SC_ASYNC_LIMIT                 0x105                       /* Asyn Event Request Limit Exc */
#define NVME_SC_FIRMWARE_SLOT               0x106                       /* Invalid Firmware Slot        */
#define NVME_SC_FIRMWARE_IMAGE              0x107                       /* Invalid Firmware Image       */
#define NVME_SC_INVALID_VECTOR              0x108                       /* Invalid Interrupt Vector     */
#define NVME_SC_INVALID_LOG_PAGE            0x109                       /* Invalid Log Page             */
#define NVME_SC_INVALID_FORMAT              0x10a                       /* Invalid Format               */
#define NVME_SC_FIRMWARE_NEEDS_RESET        0x10b                       /* FW Activation Needs Reset    */
#define NVME_SC_INVALID_QUEUE               0x10c                       /* Invalid Queue Deletion       */
#define NVME_SC_FEATURE_NOT_SAVEABLE        0x10d                       /* Feature ID Not Saveable      */
#define NVME_SC_FEATURE_NOT_CHANGEABLE      0x10e                       /* Feature Not Changeable       */
#define NVME_SC_FEATURE_NOT_PER_NS          0x10f                       /* Feature Not NS Specific      */
#define NVME_SC_FW_NEEDS_RESET_SUBSYS       0x110                       /* FW  Needs Subsys Reset       */
#define NVME_SC_BAD_ATTRIBUTES              0x180                       /* Conflicting Attributes       */
#define NVME_SC_INVALID_PI                  0x181                       /* Invalid Protection Info      */
#define NVME_SC_READ_ONLY                   0x182                       /* Write to Read Only Range     */
#define NVME_SC_WRITE_FAULT                 0x280                       /* Write Fault                  */
#define NVME_SC_READ_ERROR                  0x281                       /* Unrecovered Read Error       */
#define NVME_SC_GUARD_CHECK                 0x282                       /* End-to-end Guard Check Error */
#define NVME_SC_APPTAG_CHECK                0x283                       /* End-to-end App Tag Check Err */
#define NVME_SC_REFTAG_CHECK                0x284                       /* End-to-end Ref Tag Check Err */
#define NVME_SC_COMPARE_FAILED              0x285                       /* Compare Failure              */
#define NVME_SC_ACCESS_DENIED               0x286                       /* Access Denied                */
#define NVME_SC_DNR                         0x4000                      /* Do Not Retry                 */
/*********************************************************************************************************
  NVMe 完成队列条目
*********************************************************************************************************/
typedef struct nvme_completion_cb {
    UINT32   NVMECOMPLETION_uiResult;                                   /* Return data by admin commands*/
    UINT32   NVMECOMPLETION_uiRsvd;
    UINT16   NVMECOMPLETION_usSqHead;                                   /* SQ Head Pointer              */
    UINT16   NVMECOMPLETION_usSqid;                                     /* SQ Identifier                */
    UINT16   NVMECOMPLETION_usCmdId;                                    /* Command Identifier           */
    UINT16   NVMECOMPLETION_usStatus;                                   /* Status Field                 */
} NVME_COMPLETION_CB;
typedef NVME_COMPLETION_CB    *NVME_COMPLETION_HANDLE;
/*********************************************************************************************************
  NVMe 队列控制块(每个设备至少拥有两个队列，一个用于发送Amin命令，一个用于发送I/O命令.)
*********************************************************************************************************/
typedef struct nvme_queue_cb {
    struct nvme_ctrl_cb    *NVMEQUEUE_hCtrl;                            /* 控制器句柄                   */
    CHAR                    NVMEQUEUE_cIrqName[NVME_CTRL_IRQ_NAME_MAX]; /* 中断名称                     */
    spinlock_t              NVMEQUEUE_QueueLock;                        /* 队列锁                       */
    NVME_COMMAND_HANDLE     NVMEQUEUE_hSubmissionQueue;                 /* 命令发送队列                 */
    NVME_COMPLETION_HANDLE  NVMEQUEUE_hCompletionQueue;                 /* 命令完成队列                 */

    UINT32                 *NVMEQUEUE_puiDoorBell;                      /* Doorbell 寄存器地址          */
    UINT16                  NVMEQUEUE_usDepth;                          /* 队列深度                     */
    UINT16                  NVMEQUEUE_usCqVector;                       /* 中断号                       */
    UINT16                  NVMEQUEUE_usSqHead;                         /* 发送队列头部                 */
    UINT16                  NVMEQUEUE_usSqTail;                         /* 发送队列尾部                 */
    UINT16                  NVMEQUEUE_usCqHead;                         /* 完成队列头部                 */
    UINT16                  NVMEQUEUE_usQueueId;                        /* 队列ID                       */
    UINT8                   NVMEQUEUE_ucCqPhase;                        /* 完成队列相位                 */
    UINT8                   NVMEQUEUE_ucCqSeen;                         /* 属于本队列中断               */
    UINT8                   NVMEQUEUE_ucSusPended;                      /* 队列处于SUSPEND              */

    LW_OBJECT_HANDLE        NVMEQUEUE_hSyncBSem[NVME_CMD_DEPTH_MAX];    /* 命令同步锁                   */
    LW_OBJECT_HANDLE        NVMEQUEUE_hTagMuteSem;                      /* 标签锁                       */
    UINT32                  NVMEQUEUE_uiNextTag;                        /* 下一个可用标签               */
    PVOID                   NVMEQUEUE_pvPrpBuf;                         /* PRP                          */
    ULONG                   NVMEQUEUE_ulCmdIdData __flexarr;            /* 命令返回数据                 */
} NVME_QUEUE_CB;
typedef NVME_QUEUE_CB      *NVME_QUEUE_HANDLE;
/*********************************************************************************************************
  控制器控制块
*********************************************************************************************************/
typedef struct nvme_ctrl_cb {
    LW_LIST_LINE            NVMECTRL_lineCtrlNode;                      /* 控制器管理节点               */
    NVME_QUEUE_HANDLE      *NVMECTRL_hQueues;                           /* NVMe 队列                    */
                                                                        /* 0 号为管理队列               */
    CHAR                    NVMECTRL_cCtrlName[NVME_CTRL_NAME_MAX];     /* 控制器类型名称               */
    BOOL                    NVMECTRL_bInstalled;

    UINT32                  NVMECTRL_uiCoreVer;                         /* 驱动版本                     */
    UINT                    NVMECTRL_uiIndex;                           /* 控制器全局索引               */
    PVOID                   NVMECTRL_pvArg;                             /* 控制器扩展参数               */
    UINT                    NVMECTRL_uiUnitIndex;                       /* 本类控制器索引               */
    PVOID                   NVMECTRL_pvRegAddr;
    size_t                  NVMECTRL_stRegSize;
    ULONG                   NVMECTRL_ulIrqVector;
    PVOID                   NVMECTRL_pvIrqArg;                          /* 中断扩展参数                 */

    BOOL                    NVMECTRL_bMsix;                             /* MSI-X 中断是否使能           */
    UINT32                  NVMECTRL_uiIntNum;                          /* 中断数量                     */
    PVOID                   NVMECTRL_pvIntHandle;                       /* 中断描述                     */
    ULONG                   NVMECTRL_ulSemTimeout;                      /* 信号量超时时间               */
    LW_OBJECT_HANDLE        NVMECTRL_hLockMSem;                         /* nvme_lock 锁                 */
    CHAR                    NVMECTRL_cFlags;                            /* 控制器标志                   */
#define NVME_CTRL_RESETTING        1
#define NVME_CTRL_REMOVING         2

    UINT32                  NVMECTRL_uiCtrlConfig;                      /* 控制器配置                   */
    UINT32                  NVMECTRL_uiPageSize;                        /* 内存页大小                   */
    UINT32                  NVMECTRL_uiMaxHwSize;                       /* 最大数据传输大小             */
    UINT32                  NVMECTRL_uiStripeSize;
    UINT16                  NVMECTRL_usOncs;                            /* 可选的 NVMe 命令支持         */
    UINT8                   NVMECTRL_ucVwc;                             /* Volatile Write Cache         */
    CHAR                    NVMECTRL_cSerial[20];
    CHAR                    NVMECTRL_cModel[40];
    CHAR                    NVMECTRL_cFirmWareRev[8];

    UINT32                 *NVMECTRL_puiDoorBells;                      /* Doorbell 寄存器基址          */
    UINT32                  NVMECTRL_uiDBStride;                        /* Doorbell 寄存器间跨度        */
    
    UINT                    NVMECTRL_uiQCmdDepth;                       /* 队列命令深度                 */
    UINT                    NVMECTRL_uiQueueCount;                      /* 申请的队列数量               */
    UINT                    NVMECTRL_uiQueuesOnline;                    /* 激活的队列数量               */

    UINT32                  NVMECTRL_uiVersion;                         /* 控制器版本                   */
    BOOL                    NVMECTRL_bSubSystem;                        /* 是否支持subsystem            */
    ULONG                   NVMECTRL_ulQuirks;                          /* 设备异常行为                 */

    struct nvme_drv_cb     *NVMECTRL_hDrv;
} NVME_CTRL_CB;
typedef NVME_CTRL_CB       *NVME_CTRL_HANDLE;
/*********************************************************************************************************
  设备控制块
*********************************************************************************************************/
typedef struct nvme_dev_cb {
    LW_BLK_DEV              NVMEDEV_tBlkDev;                            /* 块设备控制块, 必须放在首位   */
    LW_LIST_LINE            NVMEDEV_lineDevNode;                        /* 设备管理节点                 */

    NVME_CTRL_HANDLE        NVMEDEV_hCtrl;                              /* 驱动器控制句柄               */
    UINT                    NVMEDEV_uiCtrl;                             /* 控制器号                     */
    UINT                    NVMEDEV_uiNameSpaceId;                      /* 命名空间号                   */

    ULONG                   NVMEDEV_ulBlkOffset;                        /* 扇区偏移                     */
    ULONG                   NVMEDEV_ulBlkCount;                         /* 扇区数量                     */
    UINT                    NVMEDEV_uiSectorShift;                      /* LBA偏移                      */
    UINT                    NVMEDEV_uiMaxHwSectors;                     /* 最大传输扇区数量             */
    PVOID                   NVMEDEV_pvOemdisk;                          /* OEMDISK 控制句柄             */
#if NVME_TRIM_EN > 0
    NVME_DSM_RANGE_HANDLE   NVMEDEV_hDsmRange;                          /* TRIM 操作参数                */
#endif
} NVME_DEV_CB;
typedef NVME_DEV_CB        *NVME_DEV_HANDLE;
/*********************************************************************************************************
  驱动控制块
*********************************************************************************************************/
typedef struct nvme_drv_cb {
    LW_LIST_LINE            NVMEDRV_lineDrvNode;                        /* 驱动管理节点                 */
    CHAR                    NVMEDRV_cDrvName[NVME_DRV_NAME_MAX];        /* 驱动名称                     */
    UINT32                  NVMEDRV_uiDrvVer;                           /* 驱动版本                     */

    NVME_CTRL_HANDLE        NVMEDRV_hCtrl;                              /* 控制器器句柄                 */

    INT                   (*NVMEDRV_pfuncOptCtrl)(NVME_CTRL_HANDLE  hCtrl, UINT  uiDrive,
                                                    INT  iCmd, LONG  lArg);
    INT                   (*NVMEDRV_pfuncVendorCtrlIntEnable)(NVME_CTRL_HANDLE   hCtrl,
                                                              NVME_QUEUE_HANDLE  hQueue,
                                                              PINT_SVR_ROUTINE   pfuncIsr,
                                                              CPCHAR             cpcName);
    INT                   (*NVMEDRV_pfuncVendorCtrlIntConnect)(NVME_CTRL_HANDLE   hCtrl,
                                                               NVME_QUEUE_HANDLE  hQueue,
                                                               PINT_SVR_ROUTINE   pfuncIsr,
                                                               CPCHAR             cpcName);
    INT                   (*NVMEDRV_pfuncVendorCtrlIntDisConnect)(NVME_CTRL_HANDLE   hCtrl,
                                                                  NVME_QUEUE_HANDLE  hQueue,
                                                                  PINT_SVR_ROUTINE   pfuncIsr);
    INT                   (*NVMEDRV_pfuncVendorCtrlReadyWork)(NVME_CTRL_HANDLE  hCtrl, UINT uiIrqNum);
    INT                   (*NVMEDRV_pfuncVendorDrvReadyWork)(struct nvme_drv_cb  *hDrv);
} NVME_DRV_CB;
typedef NVME_DRV_CB        *NVME_DRV_HANDLE;
/*********************************************************************************************************
  驱动控制块
*********************************************************************************************************/
typedef void (*nvme_completion_fn)(NVME_QUEUE_HANDLE,  PVOID, NVME_COMPLETION_HANDLE);
/*********************************************************************************************************
  Admin 命令返回数据
*********************************************************************************************************/
typedef struct nvme_cmd_info_cb {
    nvme_completion_fn      NVMECMDINFO_pfCompletion;
    PVOID                   NVMECMDINFO_pvCtx;
#if NVME_ID_DELAYED_RECOVERY > 0
    INT64                   NVMECMDINFO_i64Timeout;
#endif
}NVME_CMD_INFO_CB;
typedef NVME_CMD_INFO_CB   *NVME_CMD_INFO_HANDLE;
/*********************************************************************************************************
  命令同步信息
*********************************************************************************************************/
typedef struct sync_cmd_info_cb {
    UINT32                  SYNCCMDINFO_uiResult;
    INT                     SYNCCMDINFO_iStatus;
}SYNC_CMD_INFO_CB;
typedef SYNC_CMD_INFO_CB   *SYNC_CMD_INFO_HANDLE;
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
LW_API NVME_DEV_HANDLE      API_NvmeDevHandleGet(UINT  uiCtrl, UINT  uiDrive);
LW_API INT                  API_NvmeCtrlFree(NVME_CTRL_HANDLE  hCtrl);
LW_API NVME_CTRL_HANDLE     API_NvmeCtrlCreate(CPCHAR  pcName,
                                               UINT    uiUnit,
                                               PVOID   pvArg,
                                               ULONG   ulData);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_NVME_EN > 0)        */
#endif                                                                  /*  __NVME_H                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: ahci.h
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2016 年 01 月 04 日
**
** 描        述: AHCI 驱动.
*********************************************************************************************************/

#ifndef __AHCI_H
#define __AHCI_H

#include "ahciCfg.h"
#include "ahciPort.h"

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_AHCI_EN > 0)
/*********************************************************************************************************
  驱动选项操作命令
*********************************************************************************************************/
#define AHCI_OPT_CMD_BASE_ADDR_GET          0x100                       /* 获取控制器基地址             */
#define AHCI_OPT_CMD_BASE_ADDR_SET          0x101                       /* 设置控制器基地址             */

#define AHCI_OPT_CMD_BASE_SIZE_GET          0x102                       /* 获取控制器基地址范围         */
#define AHCI_OPT_CMD_BASE_SIZE_SET          0x103                       /* 设置控制器基地址范围         */

#define AHCI_OPT_CMD_VECTOR_NUM_GET         0x104                       /* 获取中断向量号               */
#define AHCI_OPT_CMD_VECTOR_NUM_SET         0x105                       /* 设置中断向量号               */

#define AHCI_OPT_CMD_SECTOR_START_GET       0x106                       /* 获取起始扇区                 */
#define AHCI_OPT_CMD_SECTOR_START_SET       0x107                       /* 设置起始扇区                 */

#define AHCI_OPT_CMD_EXT_ARG_GET            0x108                       /* 获取扩展参数                 */
#define AHCI_OPT_CMD_EXT_ARG_SET            0x109                       /* 设置扩展参数                 */

#define AHCI_OPT_CMD_SECTOR_SIZE_GET        0x10a                       /* 获取扇区大小                 */
#define AHCI_OPT_CMD_SECTOR_SIZE_SET        0x10b                       /* 设置扇区大小                 */

#define AHCI_OPT_CMD_CACHE_BURST_RD_GET     0x10e                       /* 获取读猝发扇区数             */
#define AHCI_OPT_CMD_CACHE_BURST_RD_SET     0x10f                       /* 设置读猝发扇区数             */

#define AHCI_OPT_CMD_CACHE_BURST_WR_GET     0x110                       /* 获取写猝发扇区数             */
#define AHCI_OPT_CMD_CACHE_BURST_WR_SET     0x111                       /* 设置写猝发扇区数             */

#define AHCI_OPT_CMD_CACHE_SIZE_GET         0x112                       /* 获取扇区缓冲区大小           */
#define AHCI_OPT_CMD_CACHE_SIZE_SET         0x113                       /* 设置扇区缓冲区大小           */

#define AHCI_OPT_CMD_CACHE_PL_GET           0x114                       /* 获取扇区缓冲区并发操作线程数 */
#define AHCI_OPT_CMD_CACHE_PL_SET           0x115                       /* 设置扇区缓冲区并发操作线程数 */

#define AHCI_OPT_CMD_PROB_TIME_UNIT_GET     0x116                       /* 获取驱动器单次探测时间 (ms)  */
#define AHCI_OPT_CMD_PROB_TIME_UNIT_SET     0x117                       /* 设置驱动器单次探测时间 (ms)  */

#define AHCI_OPT_CMD_PROB_TIME_COUNT_GET    0x118                       /* 获取驱动器单次探测数量       */
#define AHCI_OPT_CMD_PROB_TIME_COUNT_SET    0x119                       /* 设置驱动器单次探测数量       */

#define AHCI_OPT_CMD_RSTON_INTER_TIME_GET   0x11a                       /* 获取 RST ON 命令间隔时间     */
#define AHCI_OPT_CMD_RSTON_INTER_COUNT_GET  0x11b                       /* 获取 RST ON 命令间隔计数     */

#define AHCI_OPT_CMD_RSTOFF_INTER_TIME_GET  0x11c                       /* 获取 RST OFF 命令间隔时间    */
#define AHCI_OPT_CMD_RSTOFF_INTER_COUNT_GET 0x11d                       /* 获取 RST OFF 命令间隔计数    */

#define AHCI_OPT_CMD_DC_MSG_COUNT_GET       0x11e                       /* Disk cache 消息数量          */
#define AHCI_OPT_CMD_DC_PARALLEL_EN_GET     0x11f                       /* Disk cache 并行操作是否使能  */

#define AHCI_OPT_CMD_CTRL_ENDIAN_TYPE_GET   0x120                       /* 获取控制器大小端属性         */
#define AHCI_OPT_CMD_PORT_ENDIAN_TYPE_GET   0x121                       /* 获取端口大小端属性           */
#define AHCI_OPT_CMD_PARAM_ENDIAN_TYPE_GET  0x122                       /* 获取参数大小端属性           */
/*********************************************************************************************************
  大小端类型
*********************************************************************************************************/
#define AHCI_ENDIAN_TYPE_BIG                0x4321                      /* 大端类型                     */
#define AHCI_ENDIAN_TYPE_LITTEL             0x1234                      /* 小端类型                     */
/*********************************************************************************************************
  设备操作命令
*********************************************************************************************************/
#define AHCI_DEV_IOCTL_CACHE_FLUSH_GET      0x200                       /* 获取设备 cache 回写使能      */
#define AHCI_DEV_IOCTL_CACHE_FLUSH_SET      0x201                       /* 设置设备 cache 回写使能      */
/*********************************************************************************************************
  设备类型
*********************************************************************************************************/
#define AHCI_TYPE_NONE                      0x00                        /* 设备不存在或错误             */
#define AHCI_TYPE_ATA                       0x01                        /* ATA 设备                     */
#define AHCI_TYPE_ATAPI                     0x02                        /* ATAPI 设备                   */
#define AHCI_TYPE_INIT                      255                         /* 需要识别设备                 */
/*********************************************************************************************************
  设备状态
*********************************************************************************************************/
#define AHCI_DEV_OK                         0                           /* Device is OK                 */
#define AHCI_DEV_NONE                       1                           /* Device absent or not respond */
#define AHCI_DEV_DIAG_F                     2                           /* Device diagnostic failed     */
#define AHCI_DEV_PREAD_F                    3                           /* Read device parameters failed*/
#define AHCI_DEV_MED_CH                     4                           /* Medium have been changed     */
#define AHCI_DEV_NO_BLKDEV                  5                           /* No block device available    */
#define AHCI_DEV_INIT                       255                         /* Uninitialized device         */
/*********************************************************************************************************
  PRDT
*********************************************************************************************************/
#define AHCI_PRDT_MAX                       16                          /* PRDT 最大值                  */
#define AHCI_PRDT_I                         0x80000000                  /* Interrupt on Completion      */
#define AHCI_PRDT_BYTE_MAX                  0x400000                    /* Data Byte Count              */
/*********************************************************************************************************
  HBA Memory Registers
  +-------+-------+-------------------------------------+
  | Start |  End  |            Description              |
  +-------+-------+-------------------------------------+
  |   00h |   2Bh | Generic Host Control                |
  +-------+-------+-------------------------------------+
  |   2Ch |   5Fh | Reserved                            |
  +-------+-------+-------------------------------------+
  |   60h |   9Fh | Reserved for NVMHCI                 |
  +-------+-------+-------------------------------------+
  |   A0h |   FFh | Vendor Specific registers           |
  +-------+-------+-------------------------------------+
  |  100h |  17Fh | Port 0 port control registers       |
  +-------+-------+-------------------------------------+
  |  180h |  1FFh | Port 1 port control registers       |
  +-------+-------+-------------------------------------+
  |  200h |  FFFh | Ports 2 - port 29 control registers |
  +-------+-------+-------------------------------------+
  | 1000h | 107Fh | Port 30 port control registers      |
  +-------+-------+-------------------------------------+
  | 1080h | 10FFh | Port 31 port control registers      |
  +-------+-------+-------------------------------------+
*********************************************************************************************************/

/*********************************************************************************************************
  Generic Host Control
  +-------+-----+-----------+---------------------------------------+
  | Start | End |   Symbol  |             Description               |
  +-------+-----+-----------+---------------------------------------+
  |  00h  | 03h | CAP       | Host Capabilities                     |
  +-------+-----+-----------+---------------------------------------+
  |  04h  | 07h | GHC       | Global Host Control                   |
  +-------+-----+-----------+---------------------------------------+
  |  08h  | 0Bh | IS        | Interrupt Status                      |
  +-------+-----+-----------+---------------------------------------+
  |  0Ch  | 0Fh | PI        | Ports Implemented                     |
  +-------+-----+-----------+---------------------------------------+
  |  10h  | 13h | VS        | Version                               |
  +-------+-----+-----------+---------------------------------------+
  |  14h  | 17h | CCC_CTL   | Command Completion Coalescing Control |
  +-------+-----+-----------+---------------------------------------+
  |  18h  | 1Bh | CCC_PORTS | Command Completion Coalsecing Ports   |
  +-------+-----+-----------+---------------------------------------+
  |  1Ch  | 1Fh | EM_LOC    | Enclosure Management Location         |
  +-------+-----+-----------+---------------------------------------+
  |  20h  | 23h | EM_CTL    | Enclosure Management Control          |
  +-------+-----+-----------+---------------------------------------+
  |  24h  | 27h | CAP2      | Host Capabilities Extended            |
  +-------+-----+-----------+---------------------------------------+
  |  28h  | 2Bh | BOHC      | BIOS/OS Handoff Control and Status    |
  +-------+-----+-----------+---------------------------------------+
*********************************************************************************************************/
#define AHCI_CAP                            0x00
#define AHCI_GHC                            0x04
#define AHCI_IS                             0x08
#define AHCI_PI                             0x0c
#define AHCI_VS                             0x10
#define AHCI_CCC_CTL                        0x14
#define AHCI_CCC_PORTS                      0x18
#define AHCI_EM_LOC                         0x1c
#define AHCI_EM_CTL                         0x20
#define AHCI_CAP2                           0x24
#define AHCI_BOHC                           0x28
#define AHCI_VSR                            0xa0
#define AHCI_PI_WIDTH                       0x20                        /* PI 寄存器宽度                */
/*********************************************************************************************************
  CAP (Host Capabilities) 掩码
*********************************************************************************************************/
#define AHCI_CAP_S64A                       0x80000000                  /* 64-bit Addressing            */
#define AHCI_CAP_SNCQ                       0x40000000                  /* Native Command Queuing       */
#define AHCI_CAP_SSNTF                      0x20000000                  /* SNotification Register       */
#define AHCI_CAP_SMPS                       0x10000000                  /* Mechanical Presence Switch   */
#define AHCI_CAP_SSS                        0x08000000                  /* Staggered Spin-up            */
#define AHCI_CAP_SALP                       0x04000000                  /* Aggressive Link Power Manage */
#define AHCI_CAP_SAL                        0x02000000                  /* Activity LED                 */
#define AHCI_CAP_SCLO                       0x01000000                  /* Command List Override        */
#define AHCI_CAP_ISS                        0x00F00000                  /* Interface Speed Support      */
#define AHCI_CAP_ISS_SHFT                   20
#define AHCI_CAP_SNZO                       0x00080000                  /* Non-Zero DMA Offsets         */
#define AHCI_CAP_SAM                        0x00040000                  /* AHCI mode only               */
#define AHCI_CAP_SPM                        0x00020000                  /* Port Multiplier              */
#define AHCI_CAP_FBSS                       0x00010000                  /* FIS-based Switching Supported*/
#define AHCI_CAP_PMD                        0x00008000                  /* PIO Multiple DRQ Block       */
#define AHCI_CAP_SSC                        0x00004000                  /* Slumber State Capable        */
#define AHCI_CAP_PSC                        0x00002000                  /* Partial State Capable        */
#define AHCI_CAP_NCS                        0x00001F00                  /* Number of Command Slots      */
#define AHCI_CAP_NCS_SHFT                   8
#define AHCI_CAP_CCCS                       0x00000080                  /* Command Completion Coalescing*/
#define AHCI_CAP_EMS                        0x00000040                  /* Enclosure Management         */
#define AHCI_CAP_SXS                        0x00000020                  /* External SATA                */
#define AHCI_CAP_NP                         0x0000001F                  /* Number of Ports              */
/*********************************************************************************************************
  GHC (Global HBA Control)
*********************************************************************************************************/
#define AHCI_GHC_AE                         0x80000000                  /* AHCI Enable                  */
#define AHCI_GHC_MRSM                       0x00000004                  /* MSI Revert to Single Message */
#define AHCI_GHC_IE                         0x00000002                  /* Interrupt Enable             */
#define AHCI_GHC_HR                         0x00000001                  /* HBA Reset                    */
/*********************************************************************************************************
  VS (AHCI Version HBA Control)
  +-------+-------------+-------------+
  |  版本 | AHCI_VS_MJR | AHCI_VS_MNR |
  +-------+-------------+-------------+
  | 0.95  |    0000h    |    0905h    |
  +-------+-------------+-------------+
  | 1.0   |    0001h    |    0000h    |
  +-------+-------------+-------------+
  | 1.1   |    0001h    |    0100h    |
  +-------+-------------+-------------+
*********************************************************************************************************/
#define AHCI_VS_MJR                         0xffff0000                  /* Major Version Number         */
#define AHCI_VS_MJR_SHFT                    16
#define AHCI_VS_MNR                         0x0000ffff                  /* Minor Version Number         */
/*********************************************************************************************************
  CAP2 (Host Capabilities Extended) 掩码
*********************************************************************************************************/
#define AHCI_CAP2_DESO                      0x00000020                  /* DevSleep Entrance Slumber Oly*/
#define AHCI_CAP2_SADM                      0x00000010                  /* Aggressive Dev Sleep Manage  */
#define AHCI_CAP2_SDS                       0x00000008                  /* Supports Device Sleep        */
#define AHCI_CAP2_APST                      0x00000004                  /* Auto Partial to Slumber Trans*/
#define AHCI_CAP2_NVMP                      0x00000002                  /* NVMHCI Present               */
#define AHCI_CAP2_BOH                       0x00000001                  /* BIOS/OS Handoff              */
/*********************************************************************************************************
  控制器寄存器读写
*********************************************************************************************************/
#define AHCI_CTRL_READ(ctrl, reg)           (ctrl)->AHCICTRL_pfuncCtrlRead(ctrl, (addr_t)reg)
#define AHCI_CTRL_WRITE(ctrl, reg, val)     (ctrl)->AHCICTRL_pfuncCtrlWrite(ctrl, (addr_t)reg, val)

#define AHCI_CTRL_RAW_READ(base, reg)       read32((addr_t)((ULONG)(base + reg)))
#define AHCI_CTRL_RAW_WRITE(base, reg, val) write32(val, (addr_t)((ULONG)(base + reg)))
/*********************************************************************************************************
  Port Registers (one set per port)
  +-------+-----+----------+------------------------------------------------------+
  | Start | End |  Symbol  |                    Description                       |
  +-------+-----+----------+------------------------------------------------------+
  |  00h  | 03h | PxCLB    | Port x Command List Base Address                     |
  +-------+-----+----------+------------------------------------------------------+
  |  04h  | 07h | PxCLBU   | Port x Command List Base Address Upper 32-Bits       |
  +-------+-----+----------+------------------------------------------------------+
  |  08h  | 0Bh | PxFB     | Port x FIS Base Address                              |
  +-------+-----+----------+------------------------------------------------------+
  |  0Ch  | 0Fh | PxFBU    | Port x FIS Base Address Upper 32-Bits                |
  +-------+-----+----------+------------------------------------------------------+
  |  10h  | 13h | PxIS     | Port x Interrupt Status                              |
  +-------+-----+----------+------------------------------------------------------+
  |  14h  | 17h | PxIE     | Port x Interrupt Enable                              |
  +-------+-----+----------+------------------------------------------------------+
  |  18h  | 1Bh | PxCMD    | Port x Command and Status                            |
  +-------+-----+----------+------------------------------------------------------+
  |  1Ch  | 1Fh | Reserved | Reserved                                             |
  +-------+-----+----------+------------------------------------------------------+
  |  20h  | 23h | PxTFD    | Port x Task File Data                                |
  +-------+-----+----------+------------------------------------------------------+
  |  24h  | 27h | PxSIG    | Port x Signature                                     |
  +-------+-----+----------+------------------------------------------------------+
  |  28h  | 2Bh | PxSSTS   | Port x Serial ATA Status (SCR0: SStatus)             |
  +-------+-----+----------+------------------------------------------------------+
  |  2Ch  | 2Fh | PxSCTL   | Port x Serial ATA Control (SCR2: SControl)           |
  +-------+-----+----------+------------------------------------------------------+
  |  30h  | 33h | PxSERR   | Port x Serial ATA Error (SCR1: SError)               |
  +-------+-----+----------+------------------------------------------------------+
  |  34h  | 37h | PxSACT   | Port x Serial ATA Active (SCR3: SActive)             |
  +-------+-----+----------+------------------------------------------------------+
  |  38h  | 3Bh | PxCI     | Port x Command Issue                                 |
  +-------+-----+----------+------------------------------------------------------+
  |  3Ch  | 3Fh | PxSNTF   | Port x Serial ATA Notification (SCR4: SNotification) |
  +-------+-----+----------+------------------------------------------------------+
  |  40h  | 43h | PxFBS    | Port x FIS-based Switching Control                   |
  +-------+-----+----------+------------------------------------------------------+
  |  44h  | 47h | PxDEVSLP | Port x Device Sleep                                  |
  +-------+-----+----------+------------------------------------------------------+
  |  48h  | 6Fh | Reserved | Reserved                                             |
  +-------+-----+----------+------------------------------------------------------+
  |  70h  | 7Fh | PxVS     | Port x Vendor Specific                               |
  +-------+-----+----------+------------------------------------------------------+
*********************************************************************************************************/
#define AHCI_PxCLB                          0x00
#define AHCI_PxCLBU                         0x04
#define AHCI_PxFB                           0x08
#define AHCI_PxFBU                          0x0C
#define AHCI_PxIS                           0x10
#define AHCI_PxIE                           0x14
#define AHCI_PxCMD                          0x18
#define AHCI_PxTFD                          0x20
#define AHCI_PxSIG                          0x24
#define AHCI_PxSSTS                         0x28
#define AHCI_PxSCTL                         0x2C
#define AHCI_PxSERR                         0x30
#define AHCI_PxSACT                         0x34
#define AHCI_PxCI                           0x38
#define AHCI_PxSNTF                         0x3C
#define AHCI_PxFBS                          0x40
#define AHCI_PxDEVSLP                       0x44
#define AHCI_PxVS                           0x70
/*********************************************************************************************************
  PxIS (Port x Interrupt Status) 掩码
*********************************************************************************************************/
#define AHCI_PIS_CPDS                       0x80000000                  /* Cold Port Detect Status      */
#define AHCI_PIS_TFES                       0x40000000                  /* Task File Error Status       */
#define AHCI_PIS_HBFS                       0x20000000                  /* Host Bus Fatal Error Status  */
#define AHCI_PIS_HBDS                       0x10000000                  /* Host Bus Data Error Status   */
#define AHCI_PIS_IFS                        0x08000000                  /* Interface Fatal Error Status */
#define AHCI_PIS_INFS                       0x04000000                  /* Interface Non-fatal Error    */
#define AHCI_PIS_OFS                        0x01000000                  /* Overflow Status              */
#define AHCI_PIS_IPMS                       0x00800000                  /* Incorrect Port Multiplier    */
#define AHCI_PIS_PRCS                       0x00400000                  /* PhyRdy Change Status         */
#define AHCI_PIS_DMPS                       0x00000080                  /* Device Mechanical Presence   */
#define AHCI_PIS_PCS                        0x00000040                  /* Port Connect Change Status   */
#define AHCI_PIS_DPS                        0x00000020                  /* Descriptor Processed         */
#define AHCI_PIS_UFS                        0x00000010                  /* Unknown FIS Interrupt        */
#define AHCI_PIS_SDBS                       0x00000008                  /* Set Device Bits Interrupt    */
#define AHCI_PIS_DSS                        0x00000004                  /* DMA Setup FIS Interrupt      */
#define AHCI_PIS_PSS                        0x00000002                  /* PIO Setup FIS Interrupt      */
#define AHCI_PIS_DHRS                       0x00000001                  /* Dev to Host Register FIS Int */
/*********************************************************************************************************
  PxIE (Port x Interrupt Enable) 掩码
*********************************************************************************************************/
#define AHCI_PIE_CPDE                       0x80000000                  /* Cold Presence Detect Enable  */
#define AHCI_PIE_TFEE                       0x40000000                  /* Task File Error Enable       */
#define AHCI_PIE_HBFE                       0x20000000                  /* Host Bus Fatal Error Enable  */
#define AHCI_PIE_HBDE                       0x10000000                  /* Host Bus Data Error Enable   */
#define AHCI_PIE_IFE                        0x08000000                  /* Interface Fatal Error Enable */
#define AHCI_PIE_INFE                       0x04000000                  /* Interface Non-fatal Error    */
#define AHCI_PIE_OFE                        0x01000000                  /* Overflow Enable              */
#define AHCI_PIE_IPME                       0x00800000                  /* Incorrect Port Multiplier    */
#define AHCI_PIE_PRCE                       0x00400000                  /* PhyRdy Change Interrupt      */
#define AHCI_PIE_DMPE                       0x00000080                  /* Device Mechanical Presence   */
#define AHCI_PIE_PCE                        0x00000040                  /* Port Change Interrupt Enable */
#define AHCI_PIE_DPE                        0x00000020                  /* Descriptor Processed Int     */
#define AHCI_PIE_UFE                        0x00000010                  /* Unknown FIS Interrupt Enable */
#define AHCI_PIE_SDBE                       0x00000008                  /* Set Device Bits FIS Interrupt*/
#define AHCI_PIE_DSE                        0x00000004                  /* DMA Setup FIS Interrupt      */
#define AHCI_PIE_PSE                        0x00000002                  /* PIO Setup FIS Interrupt      */
#define AHCI_PIE_DHRE                       0x00000001                  /* Dev to Host Register FIS Int */
/*********************************************************************************************************
  PxCMD (Port x Command and Status) 掩码
*********************************************************************************************************/
#define AHCI_PCMD_ICC                       0xF0000000                  /* Interface Communication Ctrl */
#define AHCI_PCMD_ICC_SLUMBER               0x60000000                  /* Slumber                      */
#define AHCI_PCMD_ICC_PARTIAL               0x20000000                  /* Partial                      */
#define AHCI_PCMD_ICC_ACTIVE                0x10000000                  /* Active                       */
#define AHCI_PCMD_ICC_IDLE                  0x00000000                  /* No-Op / Idle                 */
#define AHCI_PCMD_ICC_SHFT                  28
#define AHCI_PCMD_ASP                       0x08000000                  /* Aggressive Slumber / Partial */
#define AHCI_PCMD_ALPE                      0x04000000                  /* Aggressive Link Power Manage */
#define AHCI_PCMD_DLAE                      0x02000000                  /* Drive LED on ATAPI Enable    */
#define AHCI_PCMD_ATAPI                     0x01000000                  /* Device is ATAPI              */
#define AHCI_PCMD_APSTE                     0x00800000                  /* Auto Partial to Slumber Trans*/
#define AHCI_PCMD_FBSCP                     0x00400000                  /* FIS-based Switch Capable Port*/
#define AHCI_PCMD_ESP                       0x00200000                  /* External SATA Port           */
#define AHCI_PCMD_CPD                       0x00100000                  /* Cold Presence Detection      */
#define AHCI_PCMD_MPSP                      0x00080000                  /* Mechanical Presence Switch   */
#define AHCI_PCMD_HPCP                      0x00040000                  /* Hot Plug Capable Port        */
#define AHCI_PCMD_PMA                       0x00020000                  /* Port Multiplier Attached     */
#define AHCI_PCMD_CPS                       0x00010000                  /* Cold Presence State          */
#define AHCI_PCMD_CR                        0x00008000                  /* Command List Running         */
#define AHCI_PCMD_FR                        0x00004000                  /* FIS Receive Running          */
#define AHCI_PCMD_MPSS                      0x00002000                  /* Mechanical Presence Switch   */
#define AHCI_PCMD_CCS                       0x00001F00                  /* Current Command Slot         */
#define AHCI_PCMD_CCS_SHFT                  8
#define AHCI_PCMD_FRE                       0x00000010                  /* FIS Receive Enable           */
#define AHCI_PCMD_CLO                       0x00000008                  /* Command List Override        */
#define AHCI_PCMD_POD                       0x00000004                  /* Power On Device              */
#define AHCI_PCMD_SUD                       0x00000002                  /* Spin-Up Device               */
#define AHCI_PCMD_ST                        0x00000001                  /* Start                        */
/*********************************************************************************************************
  AHCI_PxSIG (Port x Signature) 掩码
*********************************************************************************************************/
#define AHCI_PSIG_ATA                       0x00000101                  /* SATA drive                   */
#define AHCI_PSIG_ATAPI                     0xEB140101                  /* SATAPI drive                 */
#define AHCI_PSIG_SEMB                      0xC33C0101                  /* Enclosure management bridge  */
#define AHCI_PSIG_PM                        0x96690101                  /* Port multiplier              */
/*********************************************************************************************************
  PxSSTS (Port x Serial ATA Status (SCR0: SStatus)) 掩码
*********************************************************************************************************/
#define AHCI_PSSTS_IPM_MSK                  0x00000F00                  /* Interface Power Management   */
#define AHCI_PSSTS_IPM_DEVICE_NONE          0x00000000                  /* Not present or not establish */
#define AHCI_PSSTS_IPM_ACTIVE               0x00000100                  /* Interface in active state    */
#define AHCI_PSSTS_IPM_PARTIAL              0x00000200                  /* Partial power management     */
#define AHCI_PSSTS_IPM_SLUMBER              0x00000600                  /* Slumber power management     */
#define AHCI_PSSTS_SPD_MSK                  0x000000F0                  /* Current Interface Speed      */
#define AHCI_PSSTS_SPD_DEVICE_NONE          0x00000000                  /* Not present or not establish */
#define AHCI_PSSTS_SPD_GEN1                 0x00000010                  /* Generation 1 communication   */
#define AHCI_PSSTS_SPD_GEN2                 0x00000020                  /* Generation 2 communication   */
#define AHCI_PSSTS_SPD_GEN3                 0x00000030                  /* Generation 3 communication   */
#define AHCI_PSSTS_DET_MSK                  0x0000000F                  /* Device Detection             */
#define AHCI_PSSTS_DET_DEVICE_NONE          0x00000000                  /* Not present or not establish */
#define AHCI_PSSTS_DET_PHY_NONE             0x00000001                  /* Phy not established          */
#define AHCI_PSSTS_DET_PHY                  0x00000003                  /* Phy communication established*/
#define AHCI_PSSTS_DET_PHY_OFFLINE          0x00000004                  /* Phy in offline mode          */
/*********************************************************************************************************
  PxSCTL (Port x Serial ATA Control (SCR2: SControl)) 掩码
*********************************************************************************************************/
#define AHCI_PSCTL_IPM_MSK                  0x00000F00                  /* Interface Power Management   */
#define AHCI_PSCTL_IPM_RESTRICT_NONE        0x00000000                  /* No interface restrictions    */
#define AHCI_PSCTL_IPM_PARTIAL_DISABLED     0x00000100                  /* Partial state disabled       */
#define AHCI_PSCTL_IPM_SLUMBER_DISABLED     0x00000200                  /* Slumber state disabled       */
#define AHCI_PSCTL_IPM_PARSLUM_DISABLED     0x00000300                  /* Partial and Slumber disabled */
#define AHCI_PSCTL_SPD_MSK                  0x000000F0                  /* Speed Allowed                */
#define AHCI_PSCTL_SPD_RESTRICT_NONE        0x00000000                  /* No speed restrictions        */
#define AHCI_PSCTL_SPD_LIMIT_GEN1           0x00000010                  /* Limit speed Generation 1 rate*/
#define AHCI_PSCTL_SPD_LIMIT_GEN2           0x00000020                  /* Limit speed Generation 2 rate*/
#define AHCI_PSCTL_SPD_LIMIT_GEN3           0x00000030                  /* Limit speed Generation 3 rate*/
#define AHCI_PSCTL_DET_MSK                  0x0000000F                  /* Device Detection Init        */
#define AHCI_PSCTL_DET_ACTION_NONE          0x00000000                  /* No device or action requested*/
#define AHCI_PSCTL_DET_RESET                0x00000001                  /* COMRESET is transmitted      */
#define AHCI_PSCTL_DET_DISABLE              0x00000004                  /* Disable interface and offline*/
/*********************************************************************************************************
  PxSERR (Port x Serial ATA Error (SCR1: SError)) 掩码
*********************************************************************************************************/
#define AHCI_PSERR_DIAG_X                   0x04000000                  /* Exchanged                    */
#define AHCI_PSERR_DIAG_F                   0x02000000                  /* Unknown FIS Type             */
#define AHCI_PSERR_DIAG_T                   0x01000000                  /* Transport state transition   */
#define AHCI_PSERR_DIAG_S                   0x00800000                  /* Link Sequence Error          */
#define AHCI_PSERR_DIAG_H                   0x00400000                  /* Handshake Error              */
#define AHCI_PSERR_DIAG_C                   0x00200000                  /* CRC Error                    */
#define AHCI_PSERR_DIAG_D                   0x00100000                  /* Disparity Error              */
#define AHCI_PSERR_DIAG_B                   0x00080000                  /* 10B to 8B Decode Erro        */
#define AHCI_PSERR_DIAG_W                   0x00040000                  /* Comm Wake                    */
#define AHCI_PSERR_DIAG_I                   0x00020000                  /* Phy Internal Error           */
#define AHCI_PSERR_DIAG_N                   0x00010000                  /* PhyRdy Change                */
#define AHCI_PSERR_ERR_E                    0x00000800                  /* Internal Error               */
#define AHCI_PSERR_ERR_P                    0x00000400                  /* Protocol Error               */
#define AHCI_PSERR_ERR_C                    0x00000200                  /* Persistent or Data Integrity */
#define AHCI_PSERR_ERR_T                    0x00000100                  /* Transient Data Integrity Err */
#define AHCI_PSERR_ERR_M                    0x00000002                  /* Recovered Communications Err */
#define AHCI_PSERR_ERR_I                    0x00000001                  /* Recovered Data Integrity Err */
/*********************************************************************************************************
  端口寄存器读写
*********************************************************************************************************/
#define AHCI_PORT_READ(port, reg)           (port)->AHCIDRIVE_pfuncDriveRead(port, (addr_t)reg)
#define AHCI_PORT_WRITE(port, reg, val)     (port)->AHCIDRIVE_pfuncDriveWrite(port, (addr_t)reg, val)

#define AHCI_PORT_RAW_READ(base, reg)       read32((addr_t)((ULONG)(base + reg)))
#define AHCI_PORT_RAW_WRITE(base, reg, val) write32(val, (addr_t)((ULONG)(base + reg)))
/*********************************************************************************************************
  Command List Structure DW 0
*********************************************************************************************************/
#define AHCI_CMD_LIST_PRDTL                 0xFFFF0000                  /* Physical Descriptor Table Len*/
#define AHCI_CMD_LIST_PRDTL_SHFT            16
#define AHCI_CMD_LIST_PMP                   0x0000F000                  /* Port Multiplier Port         */
#define AHCI_CMD_LIST_PMP_SHFT              12
#define AHCI_CMD_LIST_C                     0x00000400                  /* Clear Busy upon R_OK         */
#define AHCI_CMD_LIST_B                     0x00000200                  /* BIST                         */
#define AHCI_CMD_LIST_R                     0x00000100                  /* Reset                        */
#define AHCI_CMD_LIST_P                     0x00000080                  /* Prefetchable                 */
#define AHCI_CMD_LIST_W                     0x00000040                  /* Write                        */
#define AHCI_CMD_LIST_A                     0x00000020                  /* ATAPI                        */
#define AHCI_CMD_LIST_CFL                   0x0000001F                  /* Command FIS Length           */
/*********************************************************************************************************
  工作模式
*********************************************************************************************************/
#define AHCI_NCQ_MODE                       0x1000                      /* Native Command Queueing Mode */
#define AHCI_DMA_ULTRA                      0x0c00                      /* RW DMA ultra                 */
#define AHCI_DMA_AUTO                       0x0017                      /* DMA max supported mode       */
#define AHCI_MODE_ALL                       (AHCI_DMA_AUTO | AHCI_DMA_ULTRA | AHCI_NCQ_MODE)
/*********************************************************************************************************
  消息类型
*********************************************************************************************************/
#define AHCI_MSG_ATTACH                     'A'                         /* 探测到设备                   */
#define AHCI_MSG_REMOVE                     'R'                         /* 移除设备                     */
#define AHCI_MSG_ERROR                      'E'                         /* 端口错误                     */
#define AHCI_MSG_TIMEOUT                    'T'                         /* 超时错误                     */
/*********************************************************************************************************
  诊断参数
*********************************************************************************************************/
#define AHCI_DIAG_OK                        0x01
/*********************************************************************************************************
  控制寄存器
*********************************************************************************************************/
#define AHCI_CTL_4BIT                       0x8                         /* use 4 head bits              */
#define AHCI_CTL_RST                        0x4                         /* reset controller             */
#define AHCI_CTL_IDS                        0x2                         /* disable interrupts           */
/*********************************************************************************************************
  状态寄存器
*********************************************************************************************************/
#define AHCI_STAT_ACCESS                    (AHCI_STAT_BUSY | AHCI_STAT_DRQ)
#define AHCI_STAT_BUSY                      0x80                        /* controller busy              */
#define AHCI_STAT_READY                     0x40                        /* selected drive ready         */
#define AHCI_STAT_WRTFLT                    0x20                        /* write fault                  */
#define AHCI_STAT_SEEKCMPLT                 0x10                        /* seek complete                */
#define AHCI_STAT_DRQ                       0x08                        /* data request                 */
#define AHCI_STAT_ECCCOR                    0x04                        /* ECC correction made in data  */
#define AHCI_STAT_INDEX                     0x02                        /* index pulse from selected drv*/
#define AHCI_STAT_ERR                       0x01                        /* error detect                 */
/*********************************************************************************************************
  地址模式
*********************************************************************************************************/
#define AHCI_SDH_IBM                        0xa0                        /* chs, 512 bytes sector, ecc   */
#define AHCI_SDH_LBA                        0xe0                        /* lba, 512 bytes sector, ecc   */
/*********************************************************************************************************
  ATA 命令
*********************************************************************************************************/
#define AHCI_CMD_DSM                        0x06                        /* DATA SET MANAGEMENT          */
#define AHCI_CMD_RECALIB                    0x10                        /* recalibrate                  */
#define AHCI_CMD_SEEK                       0x70                        /* seek                         */
#define AHCI_CMD_READ                       0x20                        /* read sectors with retries    */
#define AHCI_CMD_READ_EXT                   0x24                        /* 48-bit LBA                   */
#define AHCI_CMD_WRITE                      0x30                        /* write sectors with retries   */
#define AHCI_CMD_WRITE_EXT                  0x34                        /* 48-bit LBA                   */
#define AHCI_CMD_FORMAT                     0x50                        /* format track                 */
#define AHCI_CMD_READ_FPDMA_QUEUED          0x60                        /* read sector NCQ              */
#define AHCI_CMD_WRITE_FPDMA_QUEUED         0x61                        /* write sector NCQ             */
#define AHCI_CMD_DIAGNOSE                   0x90                        /* execute controller diagnostic*/
#define AHCI_CMD_INITP                      0x91                        /* initialize drive parameters  */
#define AHCI_CMD_READP                      0xEC                        /* identify                     */
#define AHCI_CMD_SET_FEATURE                0xEF                        /* set features                 */
#define AHCI_CMD_SET_MULTI                  0xC6                        /* set multi                    */
#define AHCI_CMD_READ_MULTI                 0xC4                        /* read multi                   */
#define AHCI_CMD_READ_MULTI_EXT             0x29                        /* read multi (48-bit LBA)      */
#define AHCI_CMD_READ_DMA                   0xC8                        /* read dma                     */
#define AHCI_CMD_READ_DMA_EXT               0x25                        /* read dma (48-bit LBA)        */
#define AHCI_CMD_WRITE_MULTI                0xC5                        /* write multi                  */
#define AHCI_CMD_WRITE_MULTI_EXT            0x39                        /* write multi (48-bit LBA)     */
#define AHCI_CMD_WRITE_DMA                  0xCA                        /* write dma                    */
#define AHCI_CMD_WRITE_DMA_EXT              0x35                        /* write dma (48-bit LBA)       */
#define AHCI_CMD_CHECK_POWER_LEVEL          0xE5                        /* Check Power Level            */
#define AHCI_CMD_IDLE_IMMEDIATE             0xE1                        /* Go to Idle State             */
#define AHCI_CMD_STANDBY_IMMEDIATE          0xE0                        /* Go to Standby State          */
#define AHCI_CMD_SLEEP                      0xE6                        /* Go to Sleep State            */
#define AHCI_CMD_DEVICE_RESET               0x08                        /* Go to Sleep State            */
#define AHCI_CMD_SMART                      0xB0                        /* SMART                        */
#define AHCI_CMD_FLUSH_CACHE                0xE7                        /* Flush Cache                  */
#define AHCI_CMD_FLUSH_CACHE_EXT            0xEA                        /* Flush Cache EXT              */
#define AHCI_CMD_SFQ_DSM                    0x64                        /* SFQ DATA SET MANAGEMENT      */
/*********************************************************************************************************
  SMART 命令
*********************************************************************************************************/
#define AHCI_SMART_CMD_READ_DATA            0xD0
#define AHCI_SMART_CMD_READ_THRESHOLDS      0xD1
#define AHCI_SMART_CMD_EN_DIS_AUTOSAVE      0xD2
#define AHCI_SMART_CMD_SAVE_ATTRIBUTES      0xD3
#define AHCI_SMART_CMD_OFFLINE_DIAGS        0xD4
#define AHCI_SMART_CMD_ENABLE_SMART         0xD8
#define AHCI_SMART_CMD_DISABLE_SMART        0xD9
#define AHCI_SMART_CMD_RETURN_STATUS        0xDA

#define AHCI_SMART_OK                       0
#define AHCI_SMART_EXCEEDED                 1
/*********************************************************************************************************
  ATAPI 命令
*********************************************************************************************************/
#define AHCI_PI_CMD_SRST                    0x08                        /* ATAPI Soft Reset             */
#define AHCI_PI_CMD_PKTCMD                  0xA0                        /* ATAPI Pakcet Command         */
#define AHCI_PI_CMD_IDENTD                  0xA1                        /* ATAPI Identify Device        */
#define AHCI_PI_CMD_SERVICE                 0xA2                        /* Service                      */
/*********************************************************************************************************
  ATAPI 命令最长长度
*********************************************************************************************************/
#define AHCI_ATAPI_CMD_LEN_MAX              16                          /* ATAPI 命令最大字节长度       */
/*********************************************************************************************************
  CDROM 命令
*********************************************************************************************************/
#define AHCI_CDROM_CMD_TEST_UNIT_READY      0x00                        /* CDROM Test Unit Ready        */
#define AHCI_CDROM_CMD_REQUEST_SENSE        0x03                        /* CDROM Request Sense          */
#define AHCI_CDROM_CMD_INQUIRY              0x12                        /* CDROM Inquiry                */
#define AHCI_CDROM_CMD_READ_CDROM_CAP       0x25                        /* CDROM Read CD-ROM Capacity   */
#define AHCI_CDROM_CMD_READ_12              0xA8                        /* CDROM Read (12)              */
/*********************************************************************************************************
  AHCI_CMD_SET_FEATURE 命令
*********************************************************************************************************/
#define AHCI_SUB_ENABLE_8BIT                0x01                        /* enable 8bit data transfer    */
#define AHCI_SUB_ENABLE_WCACHE              0x02                        /* enable write cache           */
#define AHCI_SUB_SET_RWMODE                 0x03                        /* set transfer mode            */
#define AHCI_SUB_ENABLE_APM                 0x05                        /* en advanced power management */
#define AHCI_SUB_DISABLE_RETRY              0x33                        /* disable retry                */
#define AHCI_SUB_SET_LENGTH                 0x44                        /* length vendor specific bytes */
#define AHCI_SUB_SET_CACHE                  0x54                        /* set cache segments           */
#define AHCI_SUB_DISABLE_LOOK               0x55                        /* dis read look-ahead feature  */
#define AHCI_SUB_DISABLE_REVE               0x66                        /* dis reverting to power on def*/
#define AHCI_SUB_DISABLE_ECC                0x77                        /* disable ECC                  */
#define AHCI_SUB_DISABLE_8BIT               0x81                        /* disable 8bit data transfer   */
#define AHCI_SUB_DISABLE_WCACHE             0x82                        /* disable write cache          */
#define AHCI_SUB_DISABLE_APM                0x85                        /* dis advanced power management*/
#define AHCI_SUB_ENABLE_ECC                 0x88                        /* enable ECC                   */
#define AHCI_SUB_ENABLE_RETRY               0x99                        /* enable retries               */
#define AHCI_SUB_ENABLE_LOOK                0xaa                        /* en read look-ahead feature   */
#define AHCI_SUB_SET_PREFETCH               0xab                        /* set maximum prefetch         */
#define AHCI_SUB_SET_4BYTES                 0xbb                        /* 4 bytes vendor specific bytes*/
#define AHCI_SUB_ENABLE_REVE                0xcc                        /* en reverting to power on def */
/*********************************************************************************************************
  AHCI_CMD_SET_FEATURE 支持参数位 (82 字)
*********************************************************************************************************/
#define AHCI_WCACHE_SUPPORTED               0x0020                      /* Write Cache Supported        */
#define AHCI_LOOK_SUPPORTED                 0x0040                      /* Read Look Ahead Supported    */
#define AHCI_APM_SUPPORT_APM                0x0008                      /* Advanced Power Management    */
#define AHCI_SMART_SUPPORTED                0x0001                      /* SMART Supported              */
/*********************************************************************************************************
  AHCI_SUB_SET_RWMODE 传输模式
*********************************************************************************************************/
#define AHCI_PIO_DEF_W                      0x00                        /* PIO default mode             */
#define AHCI_PIO_DEF_WO                     0x01                        /* PIO default no IORDY         */
#define AHCI_PIO_W_0                        0x08                        /* PIO flow control mode 0      */
#define AHCI_PIO_W_1                        0x09                        /* PIO flow control mode 1      */
#define AHCI_PIO_W_2                        0x0a                        /* PIO flow control mode 2      */
#define AHCI_PIO_W_3                        0x0b                        /* PIO flow control mode 3      */
#define AHCI_PIO_W_4                        0x0c                        /* PIO flow control mode 4      */
#define AHCI_DMA_SINGLE_0                   0x10                        /* singleword DMA mode 0        */
#define AHCI_DMA_SINGLE_1                   0x11                        /* singleword DMA mode 1        */
#define AHCI_DMA_SINGLE_2                   0x12                        /* singleword DMA mode 2        */
#define AHCI_DMA_MULTI_0                    0x20                        /* multiword DMA mode 0         */
#define AHCI_DMA_MULTI_1                    0x21                        /* multiword DMA mode 1         */
#define AHCI_DMA_MULTI_2                    0x22                        /* multiword DMA mode 2         */
#define AHCI_DMA_ULTRA_0                    0x40                        /* ultra DMA mode 0             */
#define AHCI_DMA_ULTRA_1                    0x41                        /* ultra DMA mode 1             */
#define AHCI_DMA_ULTRA_2                    0x42                        /* ultra DMA mode 2 (UDMA/33)   */
#define AHCI_DMA_ULTRA_3                    0x43                        /* ultra DMA mode 3             */
#define AHCI_DMA_ULTRA_4                    0x44                        /* ultra DMA mode 4 (UDMA/66)   */
#define AHCI_DMA_ULTRA_5                    0x45                        /* ultra DMA mode 5 (UDMA/100)  */
#define AHCI_DMA_ULTRA_6                    0x46                        /* ultra DMA mode 5 (UDMA/133)  */
/*********************************************************************************************************
  读写扇区数
*********************************************************************************************************/
#define AHCI_MAX_RW_SECTORS                 0x100                       /* max sectors per transfer     */
#define AHCI_MAX_RW_48LBA_SECTORS           0x10000                     /* max sectors 48-bit LBA mode  */
/*********************************************************************************************************
  配置参数
*********************************************************************************************************/
#define AHCI_PIO_DEF_0                      AHCI_PIO_DEF_W              /* PIO default mode             */
#define AHCI_PIO_DEF_1                      AHCI_PIO_DEF_WO             /* PIO default mode, no IORDY   */
#define AHCI_PIO_0                          AHCI_PIO_W_0                /* PIO mode 0                   */
#define AHCI_PIO_1                          AHCI_PIO_W_1                /* PIO mode 1                   */
#define AHCI_PIO_2                          AHCI_PIO_W_2                /* PIO mode 2                   */
#define AHCI_PIO_3                          AHCI_PIO_W_3                /* PIO mode 3                   */
#define AHCI_PIO_4                          AHCI_PIO_W_4                /* PIO mode 4                   */
#define AHCI_PIO_AUTO                       0x000d                      /* PIO max supported mode       */
#define AHCI_DMA_0                          0x0010                      /* DMA mode 0                   */
#define AHCI_DMA_1                          0x0011                      /* DMA mode 1                   */
#define AHCI_DMA_2                          0x0012                      /* DMA mode 2                   */
#define AHCI_DMA_3                          0x0013                      /* DMA mode 3                   */
#define AHCI_DMA_4                          0x0014                      /* DMA mode 4                   */
#define AHCI_DMA_5                          0x0015                      /* DMA mode 5                   */
#define AHCI_DMA_6                          0x0016                      /* DMA mode 6                   */
#define AHCI_MODE_MASK                      0x00FF                      /* transfer mode mask           */

#define AHCI_PIO_SINGLE                     0x0100                      /* RW PIO single sector         */
#define AHCI_PIO_MULTI                      0x0200                      /* RW PIO multi sector          */
#define AHCI_PIO_MASK                       0x0300                      /* RW PIO mask                  */

#define AHCI_DMA_SINGLE                     0x0400                      /* RW DMA single word           */
#define AHCI_DMA_MULTI                      0x0800                      /* RW DMA multi word            */
#define AHCI_DMA_MASK                       0x0c00                      /* RW DMA mask                  */

#define AHCI_NCQ_MASK                       0x1000                      /* Native Command Queueing Mask */
/*********************************************************************************************************
  ATAPI
*********************************************************************************************************/
#define AHCI_CONFIG_PROT_TYPE               0xc000                      /* Protocol Type                */
#define AHCI_CONFIG_PROT_TYPE_ATAPI         0x8000                      /* ATAPI                        */

#define AHCI_CONFIG_DEV_TYPE                0x1f00                      /* Device Type                  */
#define AHCI_CONFIG_DEV_TYPE_CD_ROM         0x0500

#define AHCI_CONFIG_REMOVABLE               0x0080                      /* Removable                    */

#define AHCI_CONFIG_PKT_TYPE                0x0060                      /* CMD DRQ Type                 */
#define AHCI_CONFIG_PKT_TYPE_MICRO          0x0000                      /* Microprocessor DRQ           */
#define AHCI_CONFIG_PKT_TYPE_INTER          0x0020                      /* Interrupt DRQ                */
#define AHCI_CONFIG_PKT_TYPE_ACCEL          0x0040                      /* Accelerated DRQ              */

#define AHCI_CONFIG_PKT_SIZE                0x0003                      /* Command Packet Size          */
#define AHCI_CONFIG_PKT_SIZE_12             0x0000                      /* 12 bytes                     */
/*********************************************************************************************************
  能力参数
*********************************************************************************************************/
#define AHCI_CAPABIL_DMA                    0x0100                      /* DMA Supported                */
#define AHCI_CAPABIL_LBA                    0x0200                      /* LBA Supported                */
#define AHCI_CAPABIL_IORDY_CTRL             0x0400                      /* IORDY can be disabled        */
#define AHCI_CAPABIL_IORDY                  0x0800                      /* IORDY Supported              */
#define AHCI_CAPABIL_OVERLAP                0x2000                      /* Overlap Operation Supported  */
/*********************************************************************************************************
  TRIM 参数
*********************************************************************************************************/
#define AHCI_TRIM_BLOCK_MAX                 0xffff                      /* 单次扇区数量                 */
#define AHCI_TRIM_TCB_SIZE                  8                           /* 控制块大小                   */
#define AHCI_TRIM_TCB_MAX                   64                          /* 控制块最大数量               */
#define AHCI_TRIM_CMD_BLOCK_MAX             (AHCI_TRIM_BLOCK_MAX * AHCI_TRIM_TCB_MAX)
/*********************************************************************************************************
  有效
*********************************************************************************************************/
#define AHCI_FIELDS_VALID                   0x0002
/*********************************************************************************************************
  单 DMA
*********************************************************************************************************/
#define AHCI_SINGLEDMA_MODE                 0xff00                      /* 15-8: mode active            */
#define AHCI_SINGLEDMA_SUPPORT              0x00ff                      /* 7-0: modes supported         */
/*********************************************************************************************************
  多 DMA
*********************************************************************************************************/
#define AHCI_MULTIDMA_MODE                  0xff00                      /* 15-8: mode active            */
#define AHCI_MULTIDMA_SUPPORT               0x00ff                      /* 7-0: modes supported         */
/*********************************************************************************************************
  高级 PIO
*********************************************************************************************************/
#define AHCI_ADVPIO_MODE3                   0x0001                      /* The Dev supports PIO Mode 3  */
/*********************************************************************************************************
  ATAPI 寄存器
*********************************************************************************************************/
#define AHCI_ATAPI_DATA                     hCtrl->data                 /* AHCI_DATA (RW) data (16 bits)*/
#define AHCI_ATAPI_ERROR                    hCtrl->error                /* AHCI_ERROR (R) error         */
#define AHCI_ATAPI_FEATURE                  hCtrl->feature              /* AHCI_FEATURE (W) feature reg */
#define AHCI_ATAPI_INTREASON                hCtrl->seccnt               /* AHCI_SECCNT (R) int reason   */
#define AHCI_ATAPI_BCOUNT_LO                hCtrl->cylLo                /* AHCI_CYL_LO (RW) byte count  */
#define AHCI_ATAPI_BCOUNT_HI                hCtrl->cylHi                /* AHCI_CYL_HI (RW) byte count  */
#define AHCI_ATAPI_D_SELECT                 hCtrl->sdh                  /* AHCI_SDH (RW) drv select reg */
#define AHCI_ATAPI_STATUS                   hCtrl->status               /* AHCI_STATUS (R) status reg   */
#define AHCI_ATAPI_COMMAND                  hCtrl->command              /* AHCI_COMMAND (W) command reg */
#define AHCI_ATAPI_D_CONTROL                hCtrl->control              /* AHCI_D_CONTROL (W) dev ctrl  */
/*********************************************************************************************************
  错误寄存器
*********************************************************************************************************/
#define AHCI_ERR_SENSE_KEY                  0xf0                        /* Sense Key mask               */
#define AHCI_SENSE_NO_SENSE                 0x00                        /* no sense sense key           */
#define AHCI_SENSE_RECOVERED_ERROR          0x10                        /* recovered error sense key    */
#define AHCI_SENSE_NOT_READY                0x20                        /* not ready sense key          */
#define AHCI_SENSE_MEDIUM_ERROR             0x30                        /* medium error sense key       */
#define AHCI_SENSE_HARDWARE_ERROR           0x40                        /* hardware error sense key     */
#define AHCI_SENSE_ILLEGAL_REQUEST          0x50                        /* illegal request sense key    */
#define AHCI_SENSE_UNIT_ATTENTION           0x60                        /* unit attention sense key     */
#define AHCI_SENSE_DATA_PROTECT             0x70                        /* data protect sense key       */
#define AHCI_SENSE_ABBORTED_COMMAND         0xb0                        /* aborted command sense key    */
#define AHCI_SENSE_MISCOMPARE               0xe0                        /* miscompare sense key         */
#define AHCI_ERR_MCR                        0x08                        /* Media Change Requested       */
#define AHCI_ERR_ABRT                       0x04                        /* Aborted command              */
#define AHCI_ERR_EOM                        0x02                        /* End Of Media                 */
#define AHCI_ERR_ILI                        0x01                        /* Illegal Length Indication    */
/*********************************************************************************************************
  特性寄存器
*********************************************************************************************************/
#define AHCI_FEAT_OVERLAP                   0x02                        /* command may be overlapped    */
#define AHCI_FEAT_DMA                       0x01                        /* data be transferred via DMA  */
/*********************************************************************************************************
  中断源
*********************************************************************************************************/
#define AHCI_INTR_RELEASE                   0x04                        /* Bus released                 */
#define AHCI_INTR_IO                        0x02                        /* 1-In to Host 0-Out todevice  */
#define AHCI_INTR_COD                       0x01                        /* 1-Command 0-user Data        */
/*********************************************************************************************************
  选择寄存器
*********************************************************************************************************/
#define AHCI_DSEL_FILLER                    0xa0                        /* to fill static fields        */
#define AHCI_DSEL_DRV                       0x10                        /* Device 0 (DRV=0) or 1 (DRV=1)*/
/*********************************************************************************************************
  状态寄存器
*********************************************************************************************************/
#define AHCI_STAT_BUSY                      0x80                        /* controller busy              */
#define AHCI_STAT_READY                     0x40                        /* selected drive ready         */

#define AHCI_STAT_DMA_READY                 0x20                        /* ready to a DMA data transfer */

#define AHCI_STAT_WRTFLT                    0x20                        /* write fault                  */

#define AHCI_STAT_SERVICE                   0x10                        /* service or interrupt request */

#define AHCI_STAT_SEEKCMPLT                 0x10                        /* seek complete                */
#define AHCI_STAT_DRQ                       0x08                        /* data request                 */
#define AHCI_STAT_ECCCOR                    0x04                        /* ECC correction made in data  */
#define AHCI_STAT_ERR                       0x01                        /* error detect                 */
/*********************************************************************************************************
  设备控制寄存器
*********************************************************************************************************/
#define AHCI_CTL_FILLER                     0x8                         /* bit 3 must be always set     */
#define AHCI_CTL_RST                        0x4                         /* reset controller             */
#define AHCI_CTL_IDS                        0x2                         /* disable interrupts           */
/*********************************************************************************************************
  电源管理状态
*********************************************************************************************************/
#define AHCI_PM_ACTIVE_IDLE                 0
#define AHCI_PM_STANDBY                     1
#define AHCI_PM_SLEEP                       2
/*********************************************************************************************************
  ioctl 命令
*********************************************************************************************************/
#define AHCI_IOCTL_APM_ENABLE               0x100
#define AHCI_IOCTL_APM_DISABLE              0x101
#define AHCI_IOCTL_APM_GET                  0x102
#define AHCI_IOCTL_CHECK_POWER_LEVEL        0x103
#define AHCI_IOCTL_PM_IDLE                  0x104
#define AHCI_IOCTL_PM_STANDBY               0x105
#define AHCI_IOCTL_PM_SLEEP                 0x106
#define AHCI_IOCTL_PM_WAKE                  0x107
#define AHCI_IOCTL_SMART_ENABLE             0x108
#define AHCI_IOCTL_SMART_DISABLE            0x109
#define AHCI_IOCTL_SMART_SAVE_ATTR          0x10A
#define AHCI_IOCTL_SMART_IS_ENABLED         0x10B
#define AHCI_IOCTL_SMART_AUTO_SAVE_ENABLE   0x10C
#define AHCI_IOCTL_SMART_AUTO_SAVEDISABLE   0x10D
#define AHCI_IOCTL_SMART_OFFLINE_DIAG       0x10E
#define AHCI_IOCTL_SMART_RETURN_STATUS      0x10F
#define AHCI_IOCTL_SMART_READ_DATA          0x110
#define AHCI_IOCTL_SMART_READ_THRESHOLDS    0x111
#define AHCI_IOCTL_PARMS_GET                0x112
#define AHCI_IOCTL_MAX_SECTORXFER_SET       0x113
#define AHCI_IOCTL_MAX_SECTORXFER_GET       0x114
#define AHCI_IOCTL_DRIVE_HALT               0x115
#define AHCI_IOCTL_DRIVE_STOP               0x116
#define AHCI_IOCTL_DRIVE_START              0x117
/*********************************************************************************************************
  存取控制块

  CHS (Cylinder/Head/Sector)
    一共 24 个 bit 位, 其中前 10 位表示 cylinder, 中间 8 位表示 head, 后面 6 位表示 sector
    柱面 : 0 到 Cylinders - 1
    磁头 : 0 到 Heads - 1
    扇区 : 1 到 Sectors per track (注意是从 1 开始)
    磁盘最大容量为: (1024 * 256 * 63 * 512) / 1048576 = 8064 MB (1 M = 1048576 Bytes)

  LBA (Logical Block Addressing)
    LBA 是一个整数, 通过转换成 CHS 格式完成磁盘具体寻址, 48 个 bit 位寻址, 最大寻址空间 128 PB
  CHS -> LBA
    LBA = (((C * HPC) + H) * SPT) + S - 1
  LBA -> CHS
    C = LAB / (SPT * HPC)
    H = (LBA / SPT) % HPC
    S = (LBA % SPT) + 1
*********************************************************************************************************/
#define AHCI_DIR_READ                       0                           /* 读操作                       */
#define AHCI_DIR_WRITE                      1                           /* 写操作                       */
/*********************************************************************************************************
  AHCI 参数控制块
*********************************************************************************************************/
typedef struct ahci_param_cb {
    UINT16          AHCIPARAM_usConfig;                 /* [000] general configuration                  */
    UINT16          AHCIPARAM_usCylinders;              /* [001] number of cylinders                    */
    UINT16          AHCIPARAM_usRemovcyl;               /* [002] number of removable cylinders          */
    UINT16          AHCIPARAM_usHeads;                  /* [003] number of heads                        */
    UINT16          AHCIPARAM_usBytesTrack;             /* [004] number of unformatted bytes/track      */
    UINT16          AHCIPARAM_usBytesSec;               /* [005] number of unformatted bytes/sector     */
    UINT16          AHCIPARAM_usSectors;                /* [006] number of sectors/track                */
    UINT16          AHCIPARAM_usBytesGap;               /* [007] minimum bytes in intersector gap       */
    UINT16          AHCIPARAM_usBytesSync;              /* [008] minimum bytes in sync field            */
    UINT16          AHCIPARAM_usVendstat;               /* [009] number of words of vendor status       */
    UINT8           AHCIPARAM_ucSerial[20];             /* [010] controller serial number               */
    UINT16          AHCIPARAM_usType;                   /* [020] controller type                        */
    UINT16          AHCIPARAM_usSize;                   /* [021] sector buffer size, in sectors         */
    UINT16          AHCIPARAM_usBytesEcc;               /* [022] ecc bytes appended                     */
    UINT8           AHCIPARAM_ucFwRev[8];               /* [023] firmware revision                      */
    UINT8           AHCIPARAM_ucModel[40];              /* [027] model name                             */
    UINT16          AHCIPARAM_usMultiSecs;              /* [047] RW multiple support bits 7-0 max secs  */
    UINT16          AHCIPARAM_usDwordIo;                /* [048] Dword IO                               */
    UINT16          AHCIPARAM_usCapabilities;           /* [049] capabilities                           */
    UINT16          AHCIPARAM_usCapabilities1;          /* [050] capabilities 1                         */
    UINT16          AHCIPARAM_usPioMode;                /* [051] PIO data transfer cycle timing mode    */
    UINT16          AHCIPARAM_usDmaMode;                /* [052] single word DMA transfer cycle timing  */
    UINT16          AHCIPARAM_usValid;                  /* [053] field validity                         */
    UINT16          AHCIPARAM_usCurrentCylinders;       /* [054] number of current logical cylinders    */
    UINT16          AHCIPARAM_usCurrentHeads;           /* [055] number of current logical heads        */
    UINT16          AHCIPARAM_usCurrentSectors;         /* [056] number of current logical sectors/track*/
    UINT16          AHCIPARAM_usCapacity0;              /* [057] current capacity in sectors            */
    UINT16          AHCIPARAM_usCapacity1;              /* [058] current capacity in sectors            */
    UINT16          AHCIPARAM_usMultiSet;               /* [059] multiple sector setting                */
    UINT16          AHCIPARAM_usSectors0;               /* [060] total number of user addressable sector*/
    UINT16          AHCIPARAM_usSectors1;               /* [061] total number of user addressable sector*/
    UINT16          AHCIPARAM_usSingleDma;              /* [062] single word DMA transfer               */
    UINT16          AHCIPARAM_usMultiDma;               /* [063] multi word DMA transfer                */
    UINT16          AHCIPARAM_usAdvancedPio;            /* [064] flow control PIO modes supported       */
    UINT16          AHCIPARAM_usCycletimeDma;           /* [065] minimum multi DMA transfer cycle time  */
    UINT16          AHCIPARAM_usCycletimeMulti;         /* [066] recommended multiword DMA cycle time   */
    UINT16          AHCIPARAM_usCycletimePioNoIordy;    /* [067] min PIO transfer cycle time wo flow ctl*/
    UINT16          AHCIPARAM_usCycletimePioIordy;      /* [068] min PIO transfer cycle time w IORDY    */
    UINT16          AHCIPARAM_usAdditionalSupported;    /* [069] Additional Supported                   */
    UINT16          AHCIPARAM_usReserved70;             /* [070] reserved                               */
    UINT16          AHCIPARAM_usPktCmdRelTime;          /* [071] Typical Time for Release after Packet  */
    UINT16          AHCIPARAM_usServCmdRelTime;         /* [072] Typical Time for Release after SERVICE */
    UINT16          AHCIPARAM_usReservedPacketID[2];    /* [073] reserved for packet id                 */
    UINT16          AHCIPARAM_usQueueDepth;             /* [075] maximum queue depth - 1                */
    UINT16          AHCIPARAM_usSataCapabilities;       /* [076] SATA capabilities                      */
    UINT16          AHCIPARAM_usSataAddCapabilities;    /* [077] SATA Additional Capabilities           */
    UINT16          AHCIPARAM_usSataFeaturesSupported;  /* [078] SATA features Supported                */
    UINT16          AHCIPARAM_usSataFeaturesEnabled;    /* [079] SATA features Enabled                  */
    UINT16          AHCIPARAM_usMajorRevNum;            /* [080] Major ata version number               */
    UINT16          AHCIPARAM_usMinorVersNum;           /* [081] minor version number                   */
    UINT16          AHCIPARAM_usFeaturesSupported0;     /* [082] Features Supported word 0              */
    UINT16          AHCIPARAM_usFeaturesSupported1;     /* [083] Features Supported word 1              */
    UINT16          AHCIPARAM_usFeaturesSupported2;     /* [084] Features Supported word 2              */
    UINT16          AHCIPARAM_usFeaturesEnabled0;       /* [085] Features Enabled word 0                */
    UINT16          AHCIPARAM_usFeaturesEnabled1;       /* [086] Features Enabled word 1                */
    UINT16          AHCIPARAM_usFeaturesEnabled2;       /* [087] Features Enabled word 2                */
    UINT16          AHCIPARAM_usUltraDma;               /* [088] ultra DMA transfer                     */
    UINT16          AHCIPARAM_usTimeErase;              /* [089] time to perform security erase         */
    UINT16          AHCIPARAM_usTimeEnhancedErase;      /* [090] time to perform enhanced security erase*/
    UINT16          AHCIPARAM_usCurrentAPM;             /* [091] current power management value         */
    UINT16          AHCIPARAM_usPasswordRevisionCode;   /* [092] master password revision code          */
    UINT16          AHCIPARAM_usResetResult;            /* [093] result of last reset                   */
    UINT16          AHCIPARAM_usAcousticManagement;     /* [094] recommended acoustic management        */
    UINT16          AHCIPARAM_usReserved95[5];          /* [095] reserved                               */
    UINT16          AHCIPARAM_usLba48Size[4];           /* [100] 48-bit LBA size (stored little endian) */
    UINT16          AHCIPARAM_usStreamingTimePio;       /* [104] Streaming Transfer Time PIO            */
    UINT16          AHCIPARAM_usTrimBlockNumMax;        /* [105] Max number blocks DATA SET MANAGEMENT  */
    UINT16          AHCIPARAM_usPhysicalLogicalSector;  /* [106] Physical / logical sector size         */
    UINT16          AHCIPARAM_usReserved104[20];        /* [107] reserved                               */
    UINT16          AHCIPARAM_usRemovableFeatureSupport;/* removable media status notification support  */
    UINT16          AHCIPARAM_usSecurityStatus;         /* security status                              */
    UINT16          AHCIPARAM_usVendor[31];             /* vendor specific                              */
    UINT16          AHCIPARAM_usCfaPowerMode;           /* [160]                                        */
    UINT16          AHCIPARAM_usReserved161[8];         /* [161] reserved assignment by Compact Flash   */
    UINT16          AHCIPARAM_usDataSetManagement;      /* [169] DATA SET MANAGEMENT command support    */
    UINT16          AHCIPARAM_usReserved170[6];         /* [170] reserved                               */
    UINT16          AHCIPARAM_usCurrentMediaSN[30];     /* [176] Current Media Serial Number            */
    UINT16          AHCIPARAM_usReserved206[49];        /* reserved                                     */
    UINT16          AHCIPARAM_usChecksum;               /* integrity word                               */
} AHCI_PARAM_CB;
typedef AHCI_PARAM_CB     *AHCI_PARAM_HANDLE;
/*********************************************************************************************************
  Physical Region Descriptor Table (PRDT)
*********************************************************************************************************/
typedef struct ahci_prdt_cb {
    UINT32          AHCIPRDT_uiDataAddrLow;
    UINT32          AHCIPRDT_uiDataAddrHigh;
    UINT32          AHCIPRDT_uiReserved0;
    UINT32          AHCIPRDT_uiDataByteCount;
} AHCI_PRDT_CB;
typedef AHCI_PRDT_CB   *AHCI_PRDT_HANDLE;
/*********************************************************************************************************
  Command Table
*********************************************************************************************************/
#define AHCI_CMD_TABLE_SIZE                 384

typedef struct ahci_cmd_table_cb {
    UINT8           AHCICMDTABL_ucCommandFis[64];
    UINT8           AHCICMDTABL_ucAtapiCommand[16];
    UINT8           AHCICMDTABL_ucReserved0[48];
    AHCI_PRDT_CB    AHCICMDTABL_tPrdt[AHCI_PRDT_MAX];
} AHCI_CMD_TABLE_CB;
typedef AHCI_CMD_TABLE_CB     *AHCI_CMD_TABLE_HANDLE;
/*********************************************************************************************************
  Command List Structure
*********************************************************************************************************/
#define AHCI_CMD_LIST_ALIGN                 1024
#define AHCI_CMD_LIST_SIZE                  1024

typedef struct ahci_cmd_list_cb {
    UINT32          AHCICMDLIST_uiPrdtFlags;
    UINT32          AHCICMDLIST_uiByteCount;
    UINT32          AHCICMDLIST_uiCTAddrLow;
    UINT32          AHCICMDLIST_uiCTAddrHigh;
    UINT32          AHCICMDLIST_uiReserved0;
    UINT32          AHCICMDLIST_uiReserved1;
    UINT32          AHCICMDLIST_uiReserved2;
    UINT32          AHCICMDLIST_uiReserved3;
} AHCI_CMD_LIST_CB;
typedef AHCI_CMD_LIST_CB      *AHCI_CMD_LIST_HANDLE;
/*********************************************************************************************************
  Received FIS Structure
*********************************************************************************************************/
#define AHCI_RECV_FIS_SIZE                  256

typedef struct ahci_recv_fis_cb {
    UINT8           AHCIRECVFIS_ucDmaSetupFis[28];
    UINT8           AHCIRECVFIS_ucReserved0[4];
    UINT8           AHCIRECVFIS_ucPioSetupFis[20];
    UINT8           AHCIRECVFIS_ucReserved1[12];
    UINT8           AHCIRECVFIS_ucD2hRegisterFis[20];
    UINT8           AHCIRECVFIS_ucReserved2[4];
    UINT8           AHCIRECVFIS_ucSetDeviceBitsFis[8];
    UINT8           AHCIRECVFIS_ucUnknownFis[64];
    UINT8           AHCIRECVFIS_ucReserved3[96];
} AHCI_RECV_FIS_CB;
typedef AHCI_RECV_FIS_CB      *AHCI_RECV_FIS_HANDLE;
/*********************************************************************************************************
  数据方向
*********************************************************************************************************/
typedef enum {
    AHCI_DATA_DIR_NONE,
    AHCI_DATA_DIR_IN,
    AHCI_DATA_DIR_OUT
} AHCI_DATA_DIR;
/*********************************************************************************************************
  命令标记
*********************************************************************************************************/
typedef enum {
    AHCI_CMD_FLAG_SRST_ON      = 0x0001,
    AHCI_CMD_FLAG_SRST_OFF     = 0x0002,
    AHCI_CMD_FLAG_NON_SEC_DATA = 0x0004,
    AHCI_CMD_FLAG_ATAPI        = 0x0008,
    AHCI_CMD_FLAG_NCQ          = 0x0010,
    AHCI_CMD_FLAG_WAIT_SPINUP  = 0x0020,
    AHCI_CMD_FLAG_TRIM         = 0x0040,
    AHCI_CMD_FLAG_CACHE        = 0x0080
} AHCI_CMD_FLAG;
/*********************************************************************************************************
  AHCI 命令控制块
*********************************************************************************************************/
typedef struct ahci_cmd_cb {
    union {
        struct {
            UINT8           AHCICMDATA_ucAtaCommand;
            UINT32          AHCICMDATA_uiAtaFeature;
            UINT32          AHCICMDATA_uiAtaCount;
            UINT64          AHCICMDATA_ullAtaLba;
        } ata;

        struct {
            UINT8           AHCICMDATAPI_ucAtapiCommand;
            UINT8           AHCICMDATAPI_ucAtapiFeature;
            UINT8           AHCICMDATAPI_ucAtapiCmdPkt[AHCI_ATAPI_CMD_LEN_MAX];
        } atapi;
    } ahci_cmd;

#define AHCI_CMD_ATA        ahci_cmd.ata
#define AHCI_CMD_ATAPI      ahci_cmd.atapi

    INT                     AHCICMD_iFlags;
    UINT8                  *AHCICMD_pucDataBuf;
    ULONG                   AHCICMD_ulDataLen;
    AHCI_DATA_DIR           AHCICMD_iDirection;
} AHCI_CMD_CB;
typedef AHCI_CMD_CB        *AHCI_CMD_HANDLE;
/*********************************************************************************************************
  驱动器控制块
*********************************************************************************************************/
typedef struct ahci_drive_cb {
    UINT8                   AHCIDRIVE_ucType;
    UINT8                   AHCIDRIVE_ucState;
    CHAR                    AHCIDRIVE_cDevName[AHCI_DEV_NAME_MAX];
    spinlock_t              AHCIDRIVE_slLock;

    PVOID                   AHCIDRIVE_pvRegAddr;
    ULONG                   AHCIDRIVE_ulStartSector;

    UINT                    AHCIDRIVE_uiPort;
    struct ahci_ctrl_cb    *AHCIDRIVE_hCtrl;
    
    AHCI_CMD_LIST_HANDLE    AHCIDRIVE_hCmdList;                         /* 命令资源表                   */
    AHCI_RECV_FIS_HANDLE    AHCIDRIVE_hRecvFis;
    AHCI_CMD_TABLE_HANDLE   AHCIDRIVE_hCmdTable;
    
    LW_OBJECT_HANDLE        AHCIDRIVE_hSyncBSem[AHCI_CMD_SLOT_MAX];     /* ahci_sync 锁                 */
    LW_OBJECT_HANDLE        AHCIDRIVE_hLockMSem;                        /* ahci_dlock 锁                */
    LW_OBJECT_HANDLE        AHCIDRIVE_hQueueSlotCSem;                   /* ahci_slot 锁                 */
    UINT32                  AHCIDRIVE_uiCmdMask;
    AHCI_PARAM_CB           AHCIDRIVE_tParam;
    
    BOOL                    AHCIDRIVE_bMulti;
    BOOL                    AHCIDRIVE_bIordy;
    BOOL                    AHCIDRIVE_bDma;
    BOOL                    AHCIDRIVE_bLba;
    BOOL                    AHCIDRIVE_bLba48;
    BOOL                    AHCIDRIVE_bNcq;
    
    UINT16                  AHCIDRIVE_usMultiSector;
    UINT16                  AHCIDRIVE_usPioMode;
    UINT16                  AHCIDRIVE_usSingleDmaMode;
    UINT16                  AHCIDRIVE_usMultiDmaMode;
    UINT16                  AHCIDRIVE_usUltraDmaMode;
    UINT16                  AHCIDRIVE_usRwMode;         /* RW mode: PIO[0,1,2,3,4] or DMA[0,1,2]        */
    UINT16                  AHCIDRIVE_usRwPio;          /* RW PIO unit: single or multi sector          */
    UINT16                  AHCIDRIVE_usRwDma;          /* RW DMA unit: single, multi word, or ultra    */
    UINT32                  AHCIDRIVE_uiCylinder;
    UINT32                  AHCIDRIVE_uiHead;
    UINT32                  AHCIDRIVE_uiSector;
    ULONG                   AHCIDRIVE_ulByteSector;
    UINT32                  AHCIDRIVE_uiIntCount;
    UINT32                  AHCIDRIVE_uiIntStatus;
    UINT32                  AHCIDRIVE_uiIntError;
    UINT32                  AHCIDRIVE_uiTaskFileErrorCount;
    UINT32                  AHCIDRIVE_uiTimeoutErrorCount;
    BOOL                    AHCIDRIVE_bPortError;
    UINT32                  AHCIDRIVE_uiNextTag;
    UINT32                  AHCIDRIVE_uiBuffNextTag;
    BOOL                    AHCIDRIVE_bQueued;
    UINT32                  AHCIDRIVE_uiQueueDepth;
    ULONG                   AHCIDRIVE_ulSectorMax;
    UINT8                  *AHCIDRIVE_pucAlignDmaBuf;
    UINT32                  AHCIDRIVE_uiAlignSize;
    INT                     AHCIDRIVE_iInitActive;
    INT                     AHCIDRIVE_iPwmState;

    BOOL                    AHCIDRIVE_bTrim;
    UINT16                  AHCIDRIVE_usTrimBlockNumMax;
    BOOL                    AHCIDRIVE_bDrat;
    BOOL                    AHCIDRIVE_bRzat;

    INT                   (*AHCIDRIVE_pfuncReset)(struct ahci_ctrl_cb *hCtrl, UINT uiDrive);
    struct ahci_dev_cb     *AHCIDRIVE_hDev;

    UINT32                  AHCIDRIVE_uiProbTimeUnit;
    UINT32                  AHCIDRIVE_uiProbTimeCount;

    UINT32                  AHCIDRIVE_uiAttachNum;
    UINT32                  AHCIDRIVE_uiRemoveNum;

    INT                     AHCIDRIVE_iPortEndianType;
    INT                     AHCIDRIVE_iParamEndianType;
    UINT32                (*AHCIDRIVE_pfuncDriveRead)(struct ahci_drive_cb *hDrive, addr_t  ulAddr);
    VOID                  (*AHCIDRIVE_pfuncDriveWrite)(struct ahci_drive_cb *hDrive,
                                                       addr_t  ulAddr, UINT32  uiData);
} AHCI_DRIVE_CB;
typedef AHCI_DRIVE_CB      *AHCI_DRIVE_HANDLE;
/*********************************************************************************************************
  控制器控制块
*********************************************************************************************************/
typedef struct ahci_ctrl_cb {
    LW_LIST_LINE            AHCICTRL_lineCtrlNode;                      /* 控制器管理节点               */

    CHAR                    AHCICTRL_cCtrlName[AHCI_CTRL_NAME_MAX];     /* 控制器类型名称               */
    BOOL                    AHCICTRL_bInstalled;

    UINT32                  AHCICTRL_uiCoreVer;                         /* 驱动版本                     */

    UINT                    AHCICTRL_uiIndex;                           /* 控制器全局索引               */
    BOOL                    AHCICTRL_bIntConnect;
    PVOID                   AHCICTRL_pvPciArg;                          /* 控制器扩展参数               */

    BOOL                    AHCICTRL_bDrvInstalled;
    LW_OBJECT_HANDLE        AHCICTRL_hMsgQueue;                         /* ahci_msg 消息队列            */
    LW_OBJECT_HANDLE        AHCICTRL_hMonitorThread;                    /* t_ahcimsg 线程               */
    BOOL                    AHCICTRL_bMonitorStarted;

    UINT                    AHCICTRL_uiUnitIndex;                       /* 本类控制器索引               */
    PVOID                   AHCICTRL_pvRegAddr;
    size_t                  AHCICTRL_stRegSize;
    ULONG                   AHCICTRL_ulIrqVector;
    PVOID                   AHCICTRL_pvIrqArg;                          /* 中断扩展参数                 */
    PINT_SVR_ROUTINE        AHCICTRL_pfuncIrq;
    CHAR                    AHCICTRL_cIrqName[AHCI_CTRL_IRQ_NAME_MAX];

    INT                    *AHCICTRL_piTypes;
    INT                     AHCICTRL_iUseBlockWrapper;

    UINT32                  AHCICTRL_uiCap;
    BOOL                    AHCICTRL_bAddr64;
    BOOL                    AHCICTRL_bNcq;
    BOOL                    AHCICTRL_bSntf;
    BOOL                    AHCICTRL_bPmp;
    BOOL                    AHCICTRL_bEms;
    UINT32                  AHCICTRL_uiCmdSlotNum;

    UINT32                  AHCICTRL_uiPortNum;
    UINT32                  AHCICTRL_uiImpPortMap;
    UINT32                  AHCICTRL_uiImpPortNum;                      /* 有效端口数目                 */

    AHCI_DRIVE_HANDLE       AHCICTRL_hDrive;
    ULONG                   AHCICTRL_ulSemTimeout;
    INT                    *AHCICTRL_piConfigType;
    UINT32                  AHCICTRL_uiLbaTotalSecs[AHCI_DRIVE_MAX];
    UINT64                  AHCICTRL_ullLba48TotalSecs[AHCI_DRIVE_MAX];

    INT                     AHCICTRL_iEndianType;
    UINT32                (*AHCICTRL_pfuncCtrlRead)(struct ahci_ctrl_cb *hCtrl, addr_t  ulAddr);
    VOID                  (*AHCICTRL_pfuncCtrlWrite)(struct ahci_ctrl_cb *hCtrl,
                                                     addr_t  ulAddr, UINT32  uiData);

    struct ahci_drv_cb     *AHCICTRL_hDrv;
} AHCI_CTRL_CB;
typedef AHCI_CTRL_CB       *AHCI_CTRL_HANDLE;
/*********************************************************************************************************
  消息控制块
*********************************************************************************************************/
typedef struct ahci_msg_cb {
    AHCI_CTRL_HANDLE    AHCIMSG_hCtrl;                                  /* 控制器句柄                   */
    UINT                AHCIMSG_uiDrive;                                /* 驱动器号                     */
    UINT                AHCIMSG_uiMsgId;                                /* 消息 ID                      */
} AHCI_MSG_CB;
typedef AHCI_MSG_CB    *AHCI_MSG_HANDLE;
/*********************************************************************************************************
  设备控制块
*********************************************************************************************************/
typedef struct ahci_dev_cb {
    LW_BLK_DEV          AHCIDEV_tBlkDev;                                /* 块设备控制块, 必须放在首位   */
    LW_LIST_LINE        AHCIDEV_lineDevNode;                            /* 设备管理节点                 */

    AHCI_CTRL_HANDLE    AHCIDEV_hCtrl;                                  /* 驱动器控制句柄               */
    AHCI_DRIVE_HANDLE   AHCIDEV_hDrive;                                 /* 驱动器控制句柄               */
    UINT                AHCIDEV_uiCtrl;                                 /* 控制器号                     */
    UINT                AHCIDEV_uiDrive;                                /* 驱动器号                     */

    ULONG               AHCIDEV_ulBlkOffset;                            /* 扇区偏移                     */
    ULONG               AHCIDEV_ulBlkCount;                             /* 扇区数量                     */

    PVOID               AHCIDEV_pvOemdisk;                              /* OEMDISK 控制句柄             */
    AHCI_DATA_DIR       AHCIDEV_iDirection;                             /* 数据传输方向                 */
    UINT32              AHCIDEV_uiTransCount;                           /* 传输数据单元个数             */
    INT                 AHCIDEV_iErrNum;                                /* 错误号                       */

    UINT8               AHCIDEV_ucIntReason;                            /* 中断原由寄存器               */
    UINT8               AHCIDEV_ucStatus;                               /* 状态寄存器                   */
    UINT16              AHCIDEV_usTransSize;                            /* 字节数量                     */

    INT                 AHCIDEV_iCacheFlush;                            /* cache 回写操作               */
} AHCI_DEV_CB;
typedef AHCI_DEV_CB    *AHCI_DEV_HANDLE;
/*********************************************************************************************************
  驱动管理控制器控制块
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE        AHCIDCB_lineDrvCtrlNode;                        /* 控制器节点管理               */
    AHCI_CTRL_HANDLE    AHCIDCB_hDrvCtrlHandle;                         /* 控制器句柄                   */
} AHCI_DRV_CTRL_CB;
typedef AHCI_DRV_CTRL_CB       *AHCI_DRV_CTRL_HANDLE;
/*********************************************************************************************************
  驱动控制块
*********************************************************************************************************/
typedef struct ahci_drv_cb {
    LW_LIST_LINE            AHCIDRV_lineDrvNode;                        /* 驱动管理节点                 */

    CHAR                    AHCIDRV_cDrvName[AHCI_DRV_NAME_MAX];        /* 驱动名称                     */
    UINT32                  AHCIDRV_uiDrvVer;                           /* 驱动版本                     */

    AHCI_CTRL_HANDLE        AHCIDRV_hCtrl;                              /* 控制器器句柄                 */

    INT                   (*AHCIDRV_pfuncOptCtrl)(AHCI_CTRL_HANDLE hCtrl, UINT uiDrive,
                                                  INT iCmd, LONG lArg);
    INT                   (*AHCIDRV_pfuncVendorDriveInfoShow)(AHCI_CTRL_HANDLE hCtrl, UINT uiDrive,
                                                                AHCI_PARAM_HANDLE hParam);
    PCHAR                 (*AHCIDRV_pfuncVendorDriveRegNameGet)(AHCI_DRIVE_HANDLE hDrive,
                                                                UINT32 uiOffset);
    INT                   (*AHCIDRV_pfuncVendorDriveInit)(AHCI_DRIVE_HANDLE hDrive);
    INT                   (*AHCIDRV_pfuncVendorCtrlInfoShow)(AHCI_CTRL_HANDLE hCtrl);
    PCHAR                 (*AHCIDRV_pfuncVendorCtrlRegNameGet)(AHCI_CTRL_HANDLE hCtrl, UINT32 uiOffset);
    PCHAR                 (*AHCIDRV_pfuncVendorCtrlTypeNameGet)(AHCI_CTRL_HANDLE hCtrl);
    INT                   (*AHCIDRV_pfuncVendorCtrlIntEnable)(AHCI_CTRL_HANDLE hCtrl);
    INT                   (*AHCIDRV_pfuncVendorCtrlIntConnect)(AHCI_CTRL_HANDLE hCtrl,
                                                               PINT_SVR_ROUTINE pfuncIsr,
                                                               CPCHAR cpcName);
    INT                   (*AHCIDRV_pfuncVendorCtrlInit)(AHCI_CTRL_HANDLE hCtrl);
    INT                   (*AHCIDRV_pfuncVendorCtrlReadyWork)(AHCI_CTRL_HANDLE hCtrl);
    INT                   (*AHCIDRV_pfuncVendorPlatformInit)(AHCI_CTRL_HANDLE hCtrl);
    INT                   (*AHCIDRV_pfuncVendorDrvReadyWork)(struct ahci_drv_cb *hDrv);

    INT                   (*AHCIDRV_pfuncCtrlProbe)(AHCI_CTRL_HANDLE hCtrl);
    VOID                  (*AHCIDRV_pfuncCtrlRemove)(AHCI_CTRL_HANDLE hCtrl);

#define AHCI_DRV_FLAG_MASK      0xffff                                  /*  掩码                        */
#define AHCI_DRV_FLAG_ACTIVE    0x01                                    /*  是否激活                    */
    INT                     AHCIDRV_iDrvFlag;                           /* 驱动标志                     */
    UINT32                  AHCIDRV_uiDrvCtrlNum;                       /* 关联控制器数                 */
    LW_LIST_LINE_HEADER     AHCIDRV_plineDrvCtrlHeader;                 /* 控制器管理链表头             */
} AHCI_DRV_CB;
typedef AHCI_DRV_CB    *AHCI_DRV_HANDLE;
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
LW_API INT                  API_AhciDiskTrimSet(AHCI_CTRL_HANDLE hCtrl,
                                                UINT             uiDrive,
                                                ULONG            ulStartSector,
                                                ULONG            ulEndSector);

LW_API INT                  API_AhciNoDataCommandSend(AHCI_CTRL_HANDLE hCtrl,
                                                      UINT             uiDrive,
                                                      UINT8            ucCmd,
                                                      UINT32           uiFeature,
                                                      UINT16           usSector,
                                                      UINT8            ucLbaLow,
                                                      UINT8            ucLbaMid,
                                                      UINT8            ucLbaHigh,
                                                      INT              iFlags);

LW_API INT                  API_AhciDevIoctl(AHCI_DEV_HANDLE hDev, INT iCmd, LONG lArg);
LW_API AHCI_DEV_HANDLE      API_AhciDevHandleGet(UINT uiCtrl, UINT uiDrive);

LW_API INT                  API_AhciCtrlFree(AHCI_CTRL_HANDLE hCtrl);
LW_API AHCI_CTRL_HANDLE     API_AhciCtrlCreate(CPCHAR pcName, UINT uiUnit, PVOID pvArg);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_AHCI_EN > 0)        */
#endif                                                                  /*  __AHCI_H                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/

/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: x86MpApic.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 07 月 29 日
**
** 描        述: x86 体系构架 MP configuration table.
*********************************************************************************************************/

#ifndef __X86_MPAPIC_H
#define __X86_MPAPIC_H

/*********************************************************************************************************
  头文件
*********************************************************************************************************/
#include "arch/x86/common/x86Topology.h"
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
/*********************************************************************************************************
  Memory location where MP data stored
*********************************************************************************************************/
#define MPAPIC_DATA_START       0x00100000                      /*  MPAPIC DATA start address           */
#define MPAPIC_DATA_MAX_SIZE    8192                            /*  Max MP data buf size                */
/*********************************************************************************************************
  ACPI 堆大小
*********************************************************************************************************/
#define ACPI_HEAP_SIZE          (4 * LW_CFG_MB_SIZE)            /*  Max ACPI heap size                  */
/*********************************************************************************************************
  Identify startup structure used for initialization: NONE/MP/ACPI/USR
*********************************************************************************************************/
#define NO_MP_STRUCT            0x0
#define MP_MP_STRUCT            0x1
#define ACPI_MP_STRUCT          0x2
#define USR_MP_STRUCT           0x3
/*********************************************************************************************************
  MP Configuration Table Entries
*********************************************************************************************************/
#define MP_ENTRY_CPU            0                              /*  Entry Type: CPU                      */
#define MP_ENTRY_BUS            1                              /*  Entry Type: BUS                      */
#define MP_ENTRY_IOAPIC         2                              /*  Entry Type: IO APIC                  */
#define MP_ENTRY_IOINTERRUPT    3                              /*  Entry Type: IO INT                   */
#define MP_ENTRY_LOINTERRUPT    4                              /*  Entry Type: LO INT                   */
/*********************************************************************************************************
  Extended MP Configuration Table Entries
*********************************************************************************************************/
#define EXT_MP_ENTRY_SASM       128                            /*  Entry Type: System Address Space Map */
#define EXT_MP_ENTRY_BHD        129                            /*  Entry Type: Bus Hierarchy Descriptor */
#define EXT_MP_ENTRY_CBASM      130                            /*  Entry Type: Comp Addr Space Modifier */
/*********************************************************************************************************
  MP Apic Table Phys-To-Virt translation
*********************************************************************************************************/
#if LW_CFG_CPU_WORD_LENGHT == 64
#define X86_MPAPIC_PHYS_TO_VIRT(mpapicdata, x)      (VOID *)((addr_t)mpapicdata + \
                                                    (mpapicdata->x - mpapicdata->MPAPIC_uiDataLoc))

#else
#define X86_MPAPIC_PHYS_TO_VIRT(mpapicdata, x)      (VOID *)(mpapicdata->x)

#endif                                                          /*  LW_CFG_CPU_WORD_LENGHT == 64        */
/*********************************************************************************************************
  数据类型定义
*********************************************************************************************************/
/*********************************************************************************************************
  MultiProcessor Specification version 1.4

  URL: http://download.intel.com/design/archives/processors/pro/docs/24201606.pdf
*********************************************************************************************************/
typedef struct {                                        /*  MP Floating Pointer Structure               */
    CHAR    FPS_acSignature[4];                         /*  "_MP_"                                      */
    UINT32  FPS_uiConfigTableAddr;                      /*  Address of the MP configuration table       */
    UINT8   FPS_ucLength;                               /*  Length of the floating pointer structure    */
    UINT8   FPS_ucSpecRev;                              /*  Version number of the MP specification      */
    UINT8   FPS_ucChecksum;                             /*  Checksum of the pointer structure           */
    UINT8   FPS_aucFeatureByte[5];                      /*  MP feature bytes 1 - 5                      */
} X86_MP_FPS;

typedef struct {                                        /*  MP Configuration Table Header               */
    CHAR    HDR_acSignature[4];                         /*  "PCMP"                                      */
    UINT16  HDR_usTableLength;                          /*  Length of the base configuration table      */
    UINT8   HDR_ucSpecRevision;                         /*  Version number of the MP specification      */
    UINT8   HDR_ucCheckSum;                             /*  Checksum of the base configuration table    */
    CHAR    HDR_acOemId[8];                             /*  String that identifies the manufacturer     */
    CHAR    HDR_acProdId[12];                           /*  String that identifies the product          */
    UINT32  HDR_uiOemTablePtr;                          /*  Address to an OEM configuration table       */
    UINT16  HDR_usOemTableSize;                         /*  Size of the OEM configuration table         */
    UINT16  HDR_usEntryCount;                           /*  Number of entries in the variable table     */
    UINT32  HDR_uiLocalApicBaseAddr;                    /*  Address of the Local APIC                   */
    UINT16  HDR_usExtendedTableLength;                  /*  Length of the extended entries              */
    UINT8   HDR_ucExtendedTableCheckSum;                /*  Checksum of the extended table entries      */
    UINT8   HDR_ucReserved;
} X86_MP_HEADER;

typedef struct {                                        /*  MP Config Table Entry for CPU's             */
    UINT8   CPU_ucEntryType;                            /*  0 identifies a processor entry              */
    UINT8   CPU_ucLocalApicId;                          /*  Local APIC ID number                        */
    UINT8   CPU_ucLocalApicVersion;                     /*  Local APIC's version number                 */
#define CPU_EN          0x01
#define CPU_BP          0x02
    UINT8   CPU_ucCpuFlags;                             /*  EN: usable, BP: boot-processor              */
    UINT32  CPU_ucCpuSig;                               /*  Stepping, model, family of the CPU          */
    UINT32  CPU_uiFeatureFlags;                         /*  Feature definition flag                     */
    UINT32  CPU_uiReserved[2];
} X86_MP_CPU;

typedef struct {                                        /*  MP Config Table Entry for IO APIC's         */
    UINT8   IOAPIC_ucEntryType;                         /*  2 identifies an IO APIC entry               */
    UINT8   IOAPIC_ucIoApicId;                          /*  ID of this IO APIC                          */
    UINT8   IOAPIC_ucIoApicVersion;                     /*  Version of this IO APIC                     */
    UINT8   IOAPIC_ucIoApicFlags;                       /*  Usable or not                               */
    UINT32  IOAPIC_uiIoApicBaseAddress;                 /*  Address of this IO APIC                     */
} X86_MP_IOAPIC;

typedef struct {
    UINT8   BUS_ucEntryType;
    UINT8   BUS_ucBusId;
    UINT8   BUS_aucBusType[6];
} X86_MP_BUS;

typedef struct {
    UINT8   INT_ucEntryType;
    UINT8   INT_ucInterruptType;
    UINT16  INT_usInterruptFlags;
    UINT8   INT_ucSourceBusId;
    UINT8   INT_ucSourceBusIrq;
    UINT8   INT_ucDestApicId;
    UINT8   INT_ucDestApicIntIn;
} X86_MP_INTERRUPT;
/*********************************************************************************************************
 * X86_MP_APIC_DATA structure - Structure holding MP APIC details retrieved from
 *                              BIOS/ACPI/USER defined methods.
 *
 * The dynamic tables/structures are found contiguosly in memory following
 * the X86_MP_APIC_DATA structure.
 *
 *
 *         +----------------------+ <--- MPAPIC_DATA_START
 *         |                      |
 *         | X86_MP_APIC_DATA.    |
 *         |                      |
 *         |                      | <--- sizeof(X86_MP_APIC_DATA)
 *   +-----| ptr to Local Apic    |
 *   |     |    Index Table       |
 *   |     |                      |
 *   |+----| ptr to Bus Routing   |
 *   ||    |    Table             |
 *   ||    |                      |
 *   ||+---| ptr to Interrupt     |
 *   |||   |    Routing Table     |
 *   |||   |                      |
 *   |||+--| ptr to Logical Table |
 *   ||||  |                      |
 *   ||||+-| ptr to Address Table |
 *   ||||| |                      |
 *   ||||| |                      |
 *   ||||| +----------------------+
 *   ||||+>|                      |
 *   ||||  | MPAPIC_puiAddrTable  | <--- (MPAPIC_uiIoApicNr * sizeof(UINT32))
 *   ||||  |                      |
 *   ||||  +----------------------+
 *   |||+->|                      |
 *   |||   |MPAPIC_pucLogicalTable| <--- (MPAPIC_uiIoApicNr * sizeof(UINT8))
 *   |||   |                      |
 *   |||   +----------------------+
 *   ||+-->|                      |
 *   |||   +----------------------+
 *   ||+-->|                      |
 *   ||    |MPAPIC_pInterruptTable| <--- ((MPAPIC_uiIoIntNr + MPAPIC_uiLoIntNr) *
 *   ||    |                      |       sizeof(X86_MP_INTERRUPT))
 *   ||    |                      |
 *   ||    +----------------------+
 *   |+--->|                      |
 *   |     |   MPAPIC_pBusTable   | <--- (MPAPIC_uiBusNr * sizeof(X86_MP_BUS))
 *   |     |                      |
 *   |     +----------------------+
 *   +---->|                      |
 *         | pucLoApicIndexTable  | <--- (MPAPIC_uiCpuCount * sizeof(UINT8))
 *         |                      |
 *         +----------------------+
 *
*********************************************************************************************************/
typedef struct {
     UINT32         MPAPIC_uiDataLoc;                           /*  Mem location of X86_MP_APIC_DATA    */
     UINT32         MPAPIC_uiDataSize;                          /*  Size of X86_MP_APIC_DATA            */

     UINT32         MPAPIC_uiBootOpt;                           /*  NONE/MP/ACPI/USR boot structure used*/

     X86_MP_FPS     MPAPIC_Fps;                                 /*  MP Floating Pointer Structure       */

     X86_MP_HEADER  MPAPIC_Header;                              /*  MP Configuration Table Header       */

     UINT32         MPAPIC_uiLocalApicBase;                     /*  Default Local APIC addr             */

     UINT32         MPAPIC_uiIoApicNr;                          /*  Number of IO APICs (MP Table)       */

     /*
      * Logical IO APIC Id to address mapping
      */
#if LW_CFG_CPU_WORD_LENGHT == 64
     UINT32         MPAPIC_uiReserved1;                         /*  (Not used in 64BIT)                 */
#else
     UINT32        *MPAPIC_puiAddrTable;
#endif	                                                        /*  LW_CFG_CPU_WORD_LENGHT == 64        */
     UINT32         MPAPIC_uiAtLoc;                             /*  Mem location of MPAPIC_puiAddrTable */
     UINT32         MPAPIC_uiAtSize;                            /*  Size of MPAPIC_puiAddrTable         */

     /*
      * Recorded Id mapping
      */
#if LW_CFG_CPU_WORD_LENGHT == 64
     UINT32         MPAPIC_uiReserved2;                         /*  (Not used in 64BIT)                 */
#else
     UINT8         *MPAPIC_pucLogicalTable;
#endif	                                                        /*  LW_CFG_CPU_WORD_LENGHT == 64        */
     UINT32         MPAPIC_uiLtLoc;                             /*  Mem location of MPAPIC_LogicalTable */
     UINT32         MPAPIC_uiLtSize;                            /*  Size of MPAPIC_LogicalTable         */

     /*
      * Interrupt routing table
      */
     UINT32         MPAPIC_uiIoIntNr;                           /*  Number of IO interrupts (MP Table)  */
     UINT32         MPAPIC_uiLoIntNr;                           /*  Number of Local ints (MP Table)     */
#if LW_CFG_CPU_WORD_LENGHT == 64
     UINT32         MPAPIC_uiReserved3;                         /*  (Not used in 64BIT)                 */
#else
     X86_MP_INTERRUPT  *MPAPIC_pInterruptTable;
#endif                                                          /*  LW_CFG_CPU_WORD_LENGHT == 64        */
     UINT32         MPAPIC_uiItLoc;                             /*  Mem location of pInterruptTable     */
     UINT32         MPAPIC_uiItSize;                            /*  Size of pInterruptTable             */

     /*
      * Bus routing table
      */
     UINT32         MPAPIC_uiBusNr;                             /*  Number of buses (MP Table)          */
#if LW_CFG_CPU_WORD_LENGHT == 64
     UINT32         MPAPIC_uiReserved4;                         /*  (Not used in 64BIT)                 */
#else
     X86_MP_BUS    *MPAPIC_pBusTable;
#endif                                                          /*  LW_CFG_CPU_WORD_LENGHT == 64        */
     UINT32         MPAPIC_uiBtLoc;                             /*  Mem location of MPAPIC_pBusTable    */
     UINT32         MPAPIC_uiBtSize;                            /*  Size of MPAPIC_pBusTable            */

     /*
      * Local Apic Id translation
      */
#if LW_CFG_CPU_WORD_LENGHT == 64
     UINT32         MPAPIC_uiReserved5;                         /*  (Not used in 64BIT)                 */
#else
     UINT8         *MPAPIC_pucLoApicIndexTable;
#endif                                                          /*  LW_CFG_CPU_WORD_LENGHT == 64        */
     UINT32         MPAPIC_uiLaLoc;                             /*  Mem location of pucLoApicIndexTabl  */
     UINT32         MPAPIC_uiLaSize;                            /*  Size of pucLoApicIndexTable         */

     /*
      * CPU Id translation
      */
     UINT8          MPAPIC_aucCpuIndexTable[X86_CPUID_MAX_NUM_CPUS];

     UINT32         MPAPIC_uiCpuCount;                          /*  CPU counter                         */
     UINT8          MPAPIC_ucCpuBSP;                            /*  BSP Local Apic Id                   */
} X86_MP_APIC_DATA;
/*********************************************************************************************************
  全局变量声明
*********************************************************************************************************/
extern UINT32               _G_uiX86CpuCount;                           /*  逻辑处理器数目              */

extern UINT32               _G_uiX86MpApicIoIntNr;                      /*  IO 中断数目                 */
extern UINT32               _G_uiX86MpApicLoIntNr;                      /*  Local 中断数目              */
extern UINT8                _G_ucX86MpApicIoBaseId;                     /*  Base IOAPIC Id              */

extern X86_MP_INTERRUPT    *_G_pX86MpApicInterruptTable;                /*  中断表                      */

/*
 * Local APIC ID -> 逻辑处理器 ID 转换表
 */
extern UINT8                _G_aucX86CpuIndexTable[X86_CPUID_MAX_NUM_CPUS];
extern UINT                 _G_uiX86BaseCpuPhysIndex;                   /*  Base CPU Phy index          */

/*
 * 在调用 x86MpApicInstInit 函数前，可以初始化以下三个变量实现定制
 */
extern UINT                 _G_uiX86MpApicBootOpt;                      /*  启动选项                    */
extern addr_t               _G_ulX86MpApicDataAddr;                     /*  MPAPIC 数据开始地址         */
/*
 * 用户自定义的 MPAPIC 数据获取函数
 */
extern VOID               (*_G_pfuncX86MpApicDataGet)(X86_MP_APIC_DATA *);
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
extern INT   x86MpApicInstInit(VOID);
extern INT   x86MpApicLoBaseGet(CHAR  **ppcLocalApicBase);

extern INT   x86MpApicIoApicNrGet(UINT32  *puiIoApicNr);
extern INT   x86MpApicIoIntNrGet(UINT32   *puiIoIntNr);
extern INT   x86MpApicLoIntNrGet(UINT32   *puiLoIntNr);
extern INT   x86MpApicBusNrGet(UINT32     *puiBusNr);
extern INT   x86MpApicCpuNrGet(UINT32     *puiCpuNr);

extern INT   x86MpApicInterruptTableGet(X86_MP_INTERRUPT  **ppInterruptTable);
extern INT   x86MpApicBusTableGet(X86_MP_BUS              **ppBusTable);
extern INT   x86MpApicFpsGet(X86_MP_FPS                   **ppFps);

extern INT   x86MpApicAddrTableGet(UINT32     **ppuiAddrTable);
extern INT   x86MpApicLogicalTableGet(UINT8   **ppucLogicalTable);
extern INT   x86MpApicLoIndexTableGet(UINT8   **ppucLoApicIndexTable);
extern INT   x86MpApicCpuIndexTableGet(UINT8  **ppucCpuIndexTable);
extern INT   x86MpApicDataShow(VOID);

extern VOID  x86MpBiosShow(VOID);
extern VOID  x86MpBiosIoIntMapShow(VOID);
extern VOID  x86MpBiosLocalIntMapShow(VOID);

#endif                                                                  /*  __X86_MPAPIC_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: x86LocalApic.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 07 月 29 日
**
** 描        述: x86 体系构架 Local APIC 相关源文件.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/x86/pentium/x86Pentium.h"
#include "arch/x86/mpconfig/x86MpApic.h"
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
/*********************************************************************************************************
  Local APIC Register Offset
*********************************************************************************************************/
#define LOAPIC_ID                       0x020                       /*  Local APIC ID Reg               */
#define LOAPIC_VER                      0x030                       /*  Local APIC Version Reg          */
#define LOAPIC_TPR                      0x080                       /*  Task Priority Reg               */
#define LOAPIC_APR                      0x090                       /*  Arbitration Priority Reg        */
#define LOAPIC_PPR                      0x0a0                       /*  Processor Priority Reg          */
#define LOAPIC_EOI                      0x0b0                       /*  EOI Register                    */
#define LOAPIC_LDR                      0x0d0                       /*  Logical Destination Reg         */
#define LOAPIC_DFR                      0x0e0                       /*  Destination Format Reg          */
#define LOAPIC_SVR                      0x0f0                       /*  Spurious Interrupt Reg          */
#define LOAPIC_ISR                      0x100                       /*  In-service Reg                  */
#define LOAPIC_TMR                      0x180                       /*  Trigger Mode Reg                */
#define LOAPIC_IRR                      0x200                       /*  Interrupt Request Reg           */
#define LOAPIC_ESR                      0x280                       /*  Error Status Reg                */
#define LOAPIC_ICRLO                    0x300                       /*  Interrupt Command Reg           */
#define LOAPIC_ICRHI                    0x310                       /*  Interrupt Command Reg           */
#define LOAPIC_TIMER                    0x320                       /*  LVT (Timer)                     */
#define LOAPIC_THERMAL                  0x330                       /*  LVT (Thermal)                   */
#define LOAPIC_PMC                      0x340                       /*  LVT (PMC)                       */
#define LOAPIC_LINT0                    0x350                       /*  LVT (LINT0)                     */
#define LOAPIC_LINT1                    0x360                       /*  LVT (LINT1)                     */
#define LOAPIC_ERROR                    0x370                       /*  LVT (ERROR)                     */
#define LOAPIC_TIMER_ICR                0x380                       /*  Timer Initial Count Reg         */
#define LOAPIC_TIMER_CCR                0x390                       /*  Timer Current Count Reg         */
#define LOAPIC_TIMER_CONFIG             0x3e0                       /*  Timer Divide Config Reg         */
/*********************************************************************************************************
  MSR_APICBASE MSR Bits
*********************************************************************************************************/
#define LOAPIC_BASE_MASK                ~0xfffUL                    /*  Local APIC Base Addr mask       */
#define LOAPIC_GLOBAL_ENABLE            0x800UL                     /*  Local APIC Global Enable        */
#define LOAPIC_BSP                      0x100UL                     /*  Local APIC BSP                  */
/*********************************************************************************************************
  Local APIC ID Register Bits
*********************************************************************************************************/
#define LOAPIC_ID_MASK                  0x1f000000                  /*  Id [28-24] mask                 */
#define LOAPIC_ID_SHIFT                 24                          /*  Id [28-24] shift                */
#define LOAPIC_CLUSTER_ID_MASK          0x18000000                  /*  Cluster ID [28-27] mask         */
#define LOAPIC_PHY_PRC_ID_MASK          0x06000000                  /*  Phy processor id [26-25] mask   */
#define LOAPIC_LOG_PRC_ID_MASK          0x01000000                  /*  Logical processor id [24] mask  */
/*********************************************************************************************************
  Local APIC Version Register Bits
*********************************************************************************************************/
#define LOAPIC_VERSION_MASK             0x000000ff                  /*  Local APIC Version mask         */
#define LOAPIC_MAXLVT_MASK              0x00ff0000                  /*  Local APIC Max LVT mask         */
#define LOAPIC_MAXLVT_SHIFT             16                          /*  Local APIC Max LVT shift        */
#define LOAPIC_PENTIUM4                 0x00000014                  /*  Local APIC in Pentium4          */
#define LOAPIC_LVT_PENTIUM4             5                           /*  Local APIC LVT - Pentium4       */
#define LOAPIC_LVT_P6                   4                           /*  Local APIC LVT - P6             */
#define LOAPIC_LVT_P5                   3                           /*  Local APIC LVT - P5             */
/*********************************************************************************************************
  Local APIC Vector Table Bits
*********************************************************************************************************/
#define LOAPIC_VECTOR                   0x000000ff                  /*  VectorNo                        */
#define LOAPIC_MODE                     0x00000700                  /*  Delivery mode                   */
#define LOAPIC_FIXED                    0x00000000                  /*  Delivery mode: FIXED            */
#define LOAPIC_SMI                      0x00000200                  /*  Delivery mode: SMI              */
#define LOAPIC_NMI                      0x00000400                  /*  Delivery mode: NMI              */
#define LOAPIC_EXT                      0x00000700                  /*  Delivery mode: ExtINT           */
#define LOAPIC_IDLE                     0x00000000                  /*  Delivery status: Idle           */
#define LOAPIC_PEND                     0x00001000                  /*  Delivery status: Pend           */
#define LOAPIC_HIGH                     0x00000000                  /*  Polarity: High                  */
#define LOAPIC_LOW                      0x00002000                  /*  Polarity: Low                   */
#define LOAPIC_REMOTE                   0x00004000                  /*  Remote IRR                      */
#define LOAPIC_EDGE                     0x00000000                  /*  Trigger mode: Edge              */
#define LOAPIC_LEVEL                    0x00008000                  /*  Trigger mode: Level             */
#define LOAPIC_MASK                     0x00010000                  /*  Mask                            */
/*********************************************************************************************************
  Local APIC Spurious-Interrupt Register Bits
*********************************************************************************************************/
#define LOAPIC_ENABLE                   0x100                       /*  APIC Enabled                    */
#define LOAPIC_FOCUS_DISABLE            0x200                       /*  Focus Processor Checking        */

#define INT_NUMLOAPIC_SPURIOUS          0xe0                        /*  Local APIC SPURIOUS vector no   */
/*********************************************************************************************************
  Local APIC Timer Bits
*********************************************************************************************************/
#define LOAPIC_TIMER_DIVBY_2            0x0                         /*  Divide by 2                     */
#define LOAPIC_TIMER_DIVBY_4            0x1                         /*  Divide by 4                     */
#define LOAPIC_TIMER_DIVBY_8            0x2                         /*  Divide by 8                     */
#define LOAPIC_TIMER_DIVBY_16           0x3                         /*  Divide by 16                    */
#define LOAPIC_TIMER_DIVBY_32           0x8                         /*  Divide by 32                    */
#define LOAPIC_TIMER_DIVBY_64           0x9                         /*  Divide by 64                    */
#define LOAPIC_TIMER_DIVBY_128          0xa                         /*  Divide by 128                   */
#define LOAPIC_TIMER_DIVBY_1            0xb                         /*  Divide by 1                     */
#define LOAPIC_TIMER_DIVBY_MASK         0xf                         /*  Mask bits                       */
#define LOAPIC_TIMER_PERIODIC           0x00020000                  /*  Timer Mode: Periodic            */
/*********************************************************************************************************
  Local APIC LDR Bits
*********************************************************************************************************/
#define LOAPIC_LDR_LOGICID_SHIFT        24
/*********************************************************************************************************
  Local Vector's lock-unlock macro used in x86LocalApicIrqDisable/x86LocalApicIrqEnable
*********************************************************************************************************/
#define LOCKED_TIMER                    0x01
#define LOCKED_PMC                      0x02
#define LOCKED_LINT0                    0x04
#define LOCKED_LINT1                    0x08
#define LOCKED_ERROR                    0x10
#define LOCKED_THERMAL                  0x20
/*********************************************************************************************************
  Interrupt Command Register: delivery mode and status
*********************************************************************************************************/
#define MODE_FIXED                      0x0                         /*  Delivery mode: Fixed            */
#define MODE_LOWEST                     0x1                         /*  Delivery mode: Lowest           */
#define MODE_SMI                        0x2                         /*  Delivery mode: SMI              */
#define MODE_NMI                        0x4                         /*  Delivery mode: NMI              */
#define MODE_INIT                       0x5                         /*  Delivery mode: INIT             */
#define MODE_STARTUP                    0x6                         /*  Delivery mode: StartUp          */
#define STATUS_PEND                     0x1000                      /*  Delivery status: Pend           */
/*********************************************************************************************************
  数据类型定义
*********************************************************************************************************/
typedef struct {
    CHAR       *LOAPIC_pcBase;                                      /*  Local APIC addr                 */

    UINT32      LOAPIC_uiId;                                        /*  Local APIC Id                   */
    UINT32      LOAPIC_uiVersion;                                   /*  Local APIC Version              */
    UINT32      LOAPIC_uiMaxLvt;                                    /*  Local APIC Max LVT              */

    UINT32      LOAPIC_uiIntMask;                                   /*  Interrupt mask                  */

    UINT32      LOAPIC_uiOldSvr;                                    /*  Original SVR                    */
    UINT32      LOAPIC_uiOldLint0;                                  /*  Original LINT0                  */
    UINT32      LOAPIC_uiOldLint1;                                  /*  Original LINT1                  */
} X86_LOAPIC_INTR, *PX86_LOAPIC_INTR;                               /*  Local APIC 中断控制器           */
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static X86_LOAPIC_INTR  _G_x86LocalApicIntrs[LW_CFG_MAX_PROCESSORS];/*  Local APIC 中断控制器           */
/*********************************************************************************************************
  Local APIC 寄存器操作函数与宏
*********************************************************************************************************/
static LW_INLINE UINT32  __x86LocalApicRead (UINT32  *puiAddr)
{
    UINT32  uiValue;

    __asm__ volatile
    (
            "movl    %1, %0"
            : "=r" (uiValue)
            : "m" (*(puiAddr))
    );
    return uiValue;
}

static LW_INLINE VOID  __x86LocalApicWrite (UINT32  *puiAddr, UINT32  uiValue)
{
    __asm__ volatile
    (
            "movl    %1, %0"
            : "=m" (*(puiAddr))
            : "ir" (uiValue)
    );
}

static LW_INLINE VOID  __x86LocalApicOr (UINT32  *puiAddr, UINT32  uiValue)
{
    UINT32  uiDummy;

    __asm__ volatile
    (
            "movl    %0, %1;"
            "orl     %2, %1;"
            "movl    %1, %0"
            : "+m" (*(puiAddr)), "=&r" (uiDummy)
            : "g" (uiValue)
    );
}

static LW_INLINE VOID  __x86LocalApicAnd (UINT32  *puiAddr, UINT32  uiValue)
{
    UINT32  uiDummy;

    __asm__ volatile
    (
            "movl    %0, %1;"
            "andl    %2, %1;"
            "movl    %1, %0"
            : "+m" (*(puiAddr)), "=&r" (uiDummy)
            : "g" (uiValue)
    );
}

#define LOAPIC_READ(uiOffset) \
        __x86LocalApicRead((UINT32 *)((pLoApicIntr->LOAPIC_pcBase) + uiOffset))

#define LOAPIC_WRITE(uiOffset, uiValue) \
        __x86LocalApicWrite((UINT32 *)((pLoApicIntr->LOAPIC_pcBase) + uiOffset), uiValue)

#define LOAPIC_OR(uiOffset, uiValue) \
        __x86LocalApicOr((UINT32 *)((pLoApicIntr->LOAPIC_pcBase) + uiOffset), uiValue)

#define LOAPIC_AND(uiOffset, uiValue) \
        __x86LocalApicAnd((UINT32 *)((pLoApicIntr->LOAPIC_pcBase) + uiOffset), uiValue)
/*********************************************************************************************************
** 函数名称: __x86LocalApicEnable
** 功能描述: 使能 Local APIC
** 输　入  : pLoApicIntr       Local APIC 中断控制器
**           bEnable           是否使能
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __x86LocalApicEnable (PX86_LOAPIC_INTR  pLoApicIntr, BOOL  bEnable)
{
    INTREG          iregInterLevel;
    BOOL            bHasImcr = LW_FALSE;
    X86_MP_FPS     *pFps;


    x86MpApicFpsGet(&pFps);

    /*
     * Check if the IMCR exists or not
     */
    bHasImcr = (pFps->FPS_aucFeatureByte[1] & 0x80) ? LW_TRUE : LW_FALSE;

    iregInterLevel = KN_INT_DISABLE();

    /*
     * Enable or disable the Local APIC
     */
    if (bEnable) {
        /*
         * If the original mode is PIC mode, enable the Local APIC
         */
        if ((pLoApicIntr->LOAPIC_uiOldSvr & LOAPIC_ENABLE) == 0) {
            LOAPIC_OR(LOAPIC_SVR, LOAPIC_ENABLE);
        }

        if (bspIntModeGet() == X86_INT_MODE_SYMMETRIC_IO) {
            /*
             * Switch to SYMMETRIC_IO mode from PIC/VIRTUAL_WIRE mode
             */
            if (bHasImcr) {
                out8(IMCR_REG_SEL,   IMCR_ADRS);
                out8(IMCR_IOAPIC_ON, IMCR_DATA);
            }
        } else {
            /*
             * Switch from SYMMETRIC_IO mode to PIC/VIRTUAL_WIRE mode
             */
            if (bHasImcr) {
                out8(IMCR_REG_SEL,    IMCR_ADRS);
                out8(IMCR_IOAPIC_OFF, IMCR_DATA);
            }
        }

    } else {
        if (bspIntModeGet() == X86_INT_MODE_SYMMETRIC_IO) {
            /*
             * Switch from SYMMETRIC_IO mode to PIC/VIRTUAL_WIRE mode
             */
            if (bHasImcr) {
                out8(IMCR_REG_SEL,    IMCR_ADRS);
                out8(IMCR_IOAPIC_OFF, IMCR_DATA);
            }
        }

        /*
         * If the original mode is PIC mode, disable the Local APIC
         */
        if ((pLoApicIntr->LOAPIC_uiOldSvr & LOAPIC_ENABLE) == 0) {
            LOAPIC_AND(LOAPIC_SVR, ~LOAPIC_ENABLE);

        } else {
            /*
             * Restore the original value
             */
            LOAPIC_WRITE(LOAPIC_SVR,   pLoApicIntr->LOAPIC_uiOldSvr);
            LOAPIC_WRITE(LOAPIC_LINT0, pLoApicIntr->LOAPIC_uiOldLint0);
            LOAPIC_WRITE(LOAPIC_LINT1, pLoApicIntr->LOAPIC_uiOldLint1);
        }
    }

    KN_INT_ENABLE(iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: x86LocalApicInit
** 功能描述: 初始化 Local APIC
** 输　入  : NONE
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86LocalApicInit (UINT  *puiLocalApicIntNr)
{
    PX86_LOAPIC_INTR   pLoApicIntr = &_G_x86LocalApicIntrs[LW_CPU_GET_CUR_ID()];
    UINT64             ui64ApicBase;
    INT                iCoreNum;

    pLoApicIntr->LOAPIC_pcBase    = (CHAR *)LOCAL_APIC_BASE;
    pLoApicIntr->LOAPIC_uiIntMask = 0;

    if (X86_FEATURE_HAS_APIC && (X86_FEATURE_PROCESSOR_FAMILY != X86_FAMILY_PENTIUM)) {
        CHAR  *pcMpApicLoBase;
        INT    i;

        x86PentiumMsrGet(X86_MSR_APICBASE, (UINT64 *)&ui64ApicBase);
        pcMpApicLoBase = (CHAR *)((ULONG)ui64ApicBase & LOAPIC_BASE_MASK);

        if (pLoApicIntr->LOAPIC_pcBase != pcMpApicLoBase) {
            ui64ApicBase &= ~LOAPIC_BASE_MASK;
            ui64ApicBase |= LOCAL_APIC_BASE;
        }

        /*
         * Enable the Local APIC explicitly by making sure
         * it is disabled first. A toggle will make sure
         * interrupts are cleared.
         */
        ui64ApicBase &= ~(LOAPIC_GLOBAL_ENABLE);                        /*  Clear Enable bit            */
        x86PentiumMsrSet(X86_MSR_APICBASE, &ui64ApicBase);

        /*
         * Delays 50usec
         */
        for (i = 0; i < 70; i++) {                                      /*  70*720 ~= 50.4usec          */
            bspDelay720Ns();                                            /*  720ns                       */
        }

        ui64ApicBase |= LOAPIC_GLOBAL_ENABLE;                           /*  Set Enable bit              */
        x86PentiumMsrSet(X86_MSR_APICBASE, &ui64ApicBase);
    }

    /*
     * Remember the original state: SVR, LINT0, LINT1 for now
     */
    pLoApicIntr->LOAPIC_uiOldSvr   = LOAPIC_READ(LOAPIC_SVR);
    pLoApicIntr->LOAPIC_uiOldLint0 = LOAPIC_READ(LOAPIC_LINT0);
    pLoApicIntr->LOAPIC_uiOldLint1 = LOAPIC_READ(LOAPIC_LINT1);

    /*
     * Enable the Local APIC
     */
    __x86LocalApicEnable(pLoApicIntr, LW_TRUE);

    LOAPIC_OR(LOAPIC_SVR, LOAPIC_FOCUS_DISABLE | INT_NUMLOAPIC_SPURIOUS);

    /*
     * Get the Local APIC ID from Local APIC ID register
     */
    pLoApicIntr->LOAPIC_uiId = (LOAPIC_READ(LOAPIC_ID) & LOAPIC_ID_MASK) >> LOAPIC_ID_SHIFT;

    /*
     * Get the Local APIC Version from Local APIC Version register
     */
    pLoApicIntr->LOAPIC_uiVersion =  LOAPIC_READ(LOAPIC_VER) & LOAPIC_VERSION_MASK;
    pLoApicIntr->LOAPIC_uiMaxLvt  = (LOAPIC_READ(LOAPIC_VER) & LOAPIC_MAXLVT_MASK) >> LOAPIC_MAXLVT_SHIFT;

    /*
     * Reset the DFR, TPR, TIMER_CONFIG, and TIMER_ICR
     */
    LOAPIC_WRITE(LOAPIC_DFR,           0xffffffff);
    LOAPIC_WRITE(LOAPIC_TPR,           0x0);
    LOAPIC_WRITE(LOAPIC_TIMER_CONFIG,  0x0);
    LOAPIC_WRITE(LOAPIC_TIMER_ICR,     0x0);

    if (bspIntModeGet() == X86_INT_MODE_VIRTUAL_WIRE) {
        /*
         * Program Local Vector Table for the Virtual Wire Mode
         */
        if (LW_CPU_GET_CUR_ID() == 0) {                                 /*  BSP                         */
            /*
             * Set LINT0: ExtInt, high-polarity, edge-trigger, not-masked
             */
            LOAPIC_WRITE(LOAPIC_LINT0,
                    (LOAPIC_READ(LOAPIC_LINT0) &
                     ~(LOAPIC_MODE | LOAPIC_LOW  | LOAPIC_LEVEL | LOAPIC_MASK))
                    | (LOAPIC_EXT  | LOAPIC_HIGH | LOAPIC_EDGE));

            /*
             * Set LINT1: NMI, high-polarity, edge-trigger, not-masked
             */
            LOAPIC_WRITE(LOAPIC_LINT1,
                    (LOAPIC_READ(LOAPIC_LINT1) &
                     ~(LOAPIC_MODE | LOAPIC_LOW  | LOAPIC_LEVEL | LOAPIC_MASK))
                    | (LOAPIC_NMI  | LOAPIC_HIGH | LOAPIC_EDGE));
        } else {
            LOAPIC_WRITE(LOAPIC_LINT0, LOAPIC_MASK);
            LOAPIC_WRITE(LOAPIC_LINT1, LOAPIC_MASK);
        }

    } else if (bspIntModeGet() == X86_INT_MODE_SYMMETRIC_IO) {
        /*
         * Program Local Vector Table for the Symmetric IO Mode
         */
        LOAPIC_WRITE(LOAPIC_LINT0, LOAPIC_MASK);
        LOAPIC_WRITE(LOAPIC_LINT1, LOAPIC_MASK);
    }

    /*
     * Lock the Local APIC interrupts
     */
    LOAPIC_WRITE(LOAPIC_TIMER, LOAPIC_MASK);
    LOAPIC_WRITE(LOAPIC_ERROR, LOAPIC_MASK);

    if (pLoApicIntr->LOAPIC_uiMaxLvt >= LOAPIC_LVT_P6) {
        LOAPIC_WRITE(LOAPIC_PMC, LOAPIC_MASK);
    }

    if (pLoApicIntr->LOAPIC_uiMaxLvt >= LOAPIC_LVT_PENTIUM4) {
        LOAPIC_WRITE(LOAPIC_THERMAL, LOAPIC_MASK);
    }

    /*
     * Discard a pending interrupt if any
     */
    LOAPIC_WRITE(LOAPIC_EOI, 0);

    /*
     * Establish BSP Logical Destination Register to support broadcasts...
     */
    iCoreNum = x86CpuPhysIndexGet();
    LOAPIC_WRITE(LOAPIC_LDR, 1 << (iCoreNum + LOAPIC_LDR_LOGICID_SHIFT));

    if (puiLocalApicIntNr) {
        *puiLocalApicIntNr = 6;                                         /*  最多 6 个                   */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86LocalApicSecondaryInit
** 功能描述: Secondary CPU 初始化 Local APIC
** 输　入  : NONE
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86LocalApicSecondaryInit (VOID)
{
    return  (x86LocalApicInit(LW_NULL));
}
/*********************************************************************************************************
** 函数名称: x86LocalApicIrqDisable
** 功能描述: Local APIC 禁能 IRQ
** 输　入  : ucVector   中断向量号
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86LocalApicIrqDisable (UINT8  ucVector)
{
    PX86_LOAPIC_INTR  pLoApicIntr = &_G_x86LocalApicIntrs[LW_CPU_GET_CUR_ID()];

    if (!(LOAPIC_READ(LOAPIC_TIMER) & LOAPIC_MASK)) {
        pLoApicIntr->LOAPIC_uiIntMask |= LOCKED_TIMER;
        LOAPIC_OR(LOAPIC_TIMER, LOAPIC_MASK);
    }

    if (!(LOAPIC_READ(LOAPIC_LINT0) & LOAPIC_MASK)) {
        pLoApicIntr->LOAPIC_uiIntMask |= LOCKED_LINT0;
        LOAPIC_OR(LOAPIC_LINT0, LOAPIC_MASK);
    }

    if (!(LOAPIC_READ(LOAPIC_LINT1) & LOAPIC_MASK)) {
        pLoApicIntr->LOAPIC_uiIntMask |= LOCKED_LINT1;
        LOAPIC_OR(LOAPIC_LINT1, LOAPIC_MASK);
    }

    if (!(LOAPIC_READ(LOAPIC_ERROR) & LOAPIC_MASK)) {
        pLoApicIntr->LOAPIC_uiIntMask |= LOCKED_ERROR;
        LOAPIC_OR(LOAPIC_ERROR, LOAPIC_MASK);
    }

    if ((pLoApicIntr->LOAPIC_uiMaxLvt >= LOAPIC_LVT_P6) &&
        (!(LOAPIC_READ(LOAPIC_PMC) & LOAPIC_MASK))) {
        pLoApicIntr->LOAPIC_uiIntMask |= LOCKED_PMC;
        LOAPIC_OR(LOAPIC_PMC, LOAPIC_MASK);
    }

    if ((pLoApicIntr->LOAPIC_uiMaxLvt >= LOAPIC_LVT_PENTIUM4) &&
        (!(LOAPIC_READ(LOAPIC_THERMAL) & LOAPIC_MASK))) {
        pLoApicIntr->LOAPIC_uiIntMask |= LOCKED_THERMAL;
        LOAPIC_OR(LOAPIC_THERMAL, LOAPIC_MASK);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86LocalApicIrqEnable
** 功能描述: Local APIC 使能 IRQ
** 输　入  : ucVector   中断向量号
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86LocalApicIrqEnable (UINT8  ucVector)
{
    PX86_LOAPIC_INTR  pLoApicIntr = &_G_x86LocalApicIntrs[LW_CPU_GET_CUR_ID()];

    if (pLoApicIntr->LOAPIC_uiIntMask & LOCKED_TIMER) {
        LOAPIC_AND(LOAPIC_TIMER, ~LOAPIC_MASK);
    }

    if (pLoApicIntr->LOAPIC_uiIntMask & LOCKED_LINT0) {
        LOAPIC_AND(LOAPIC_LINT0, ~LOAPIC_MASK);
    }

    if (pLoApicIntr->LOAPIC_uiIntMask & LOCKED_LINT1) {
        LOAPIC_AND(LOAPIC_LINT1, ~LOAPIC_MASK);
    }

    if (pLoApicIntr->LOAPIC_uiIntMask & LOCKED_ERROR) {
        LOAPIC_AND(LOAPIC_ERROR, ~LOAPIC_MASK);
    }

    if ((pLoApicIntr->LOAPIC_uiMaxLvt >= LOAPIC_LVT_P6) &&
        (pLoApicIntr->LOAPIC_uiIntMask & LOCKED_PMC)) {
        LOAPIC_AND(LOAPIC_PMC, ~LOAPIC_MASK);
    }

    if ((pLoApicIntr->LOAPIC_uiMaxLvt >= LOAPIC_LVT_PENTIUM4) &&
        (pLoApicIntr->LOAPIC_uiIntMask & LOCKED_THERMAL)) {
        LOAPIC_AND(LOAPIC_THERMAL, ~LOAPIC_MASK);
    }

    pLoApicIntr->LOAPIC_uiIntMask = 0;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86LocalApicIrqIsEnable
** 功能描述: 判断 Local APIC 是否使能 IRQ
** 输　入  : ucVector   中断向量号
** 输　出  : LW_TRUE OR LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL x86LocalApicIrqIsEnable (UINT8  ucVector)
{
    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: x86LocalApicEoi
** 功能描述: Local APIC 结束中断
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  x86LocalApicEoi (VOID)
{
    PX86_LOAPIC_INTR  pLoApicIntr = &_G_x86LocalApicIntrs[LW_CPU_GET_CUR_ID()];

    LOAPIC_WRITE(LOAPIC_EOI, 0);
}
/*********************************************************************************************************
** 函数名称: x86LocalApicSendIpiRaw
** 功能描述: Local APIC 发送 IPI
** 输　入  : uiIpiInfo     IPI 信息
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86LocalApicSendIpiRaw (UINT32  uiIpiInfo)
{
    PX86_LOAPIC_INTR    pLoApicIntr = &_G_x86LocalApicIntrs[LW_CPU_GET_CUR_ID()];
    INTREG              iregInterLevel;
    UINT32              uiIcrHi;
    UINT32              uiIcrLo;
    LOAPIC_IPI_INFO     ipiInfo;

    ipiInfo.IPI_uiAll = uiIpiInfo;

    iregInterLevel = KN_INT_DISABLE();
    uiIcrHi = LOAPIC_READ(LOAPIC_ICRHI) & 0x00ffffff;
    uiIcrLo = LOAPIC_READ(LOAPIC_ICRLO) & 0xfff32000;
    KN_INT_ENABLE(iregInterLevel);

    /*
     * Destination short hand
     * Trigger mode. 0: edge or 1: level
     * Level. 0: de-assert or 1: assert
     * Destination mode. 0: physical or 1: logical
     * Delivery mode. 000: fixed ... 110: startup
     * Vector number
     */
    uiIcrLo |= (ipiInfo.IPI_uiAll & 0xccfff);

    /*
     * Destination Apic Id
     * APIC ID number to send the interrupt
     */
    uiIcrHi |= (ipiInfo.IPI_uiAll & 0xff000000);

    /*
     * Wait if the previous IPI hasn't been sent
     */
    while (LOAPIC_READ(LOAPIC_ICRLO) & STATUS_PEND) {
        if (LOAPIC_READ(LOAPIC_ESR) != 0) {
            _PrintFormat("failed to send IPI from CPU#0 to CPU#%d!\r\n",
                    LW_CPU_GET_CUR_ID(), X86_APICID_TO_LOGICID(ipiInfo.ICR.ICR_uiApicId));
            return  (PX_ERROR);
        }
    }

    /*
     * Reset the error status register
     */
    LOAPIC_WRITE(LOAPIC_ESR, 0);
    LOAPIC_WRITE(LOAPIC_ESR, 0);

    if (LOAPIC_READ(LOAPIC_ESR) != 0) {
        _PrintFormat("failed to send IPI from CPU#0 to CPU#%d!\r\n",
                LW_CPU_GET_CUR_ID(), X86_APICID_TO_LOGICID(ipiInfo.ICR.ICR_uiApicId));
        return  (PX_ERROR);
    }

    /*
     * Can send shorthand message by performing a single
     * 32-bit memory write to the low half of the ICR.
     */
    if ((ipiInfo.ICR.ICR_uiShortHand == 0x00) || (ipiInfo.ICR.ICR_uiDestMode == 0x01)) {
        iregInterLevel = KN_INT_DISABLE();
        LOAPIC_WRITE(LOAPIC_ICRHI, uiIcrHi);
        LOAPIC_WRITE(LOAPIC_ICRLO, uiIcrLo);
        KN_INT_ENABLE(iregInterLevel);

    } else {
        LOAPIC_WRITE(LOAPIC_ICRLO, uiIcrLo);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86LocalApicSendIpi
** 功能描述: Local APIC 发送 IPI
** 输　入  : ucLocalApicID     Local APIC ID
**           ulVector          SylixOS 中断向量号
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86LocalApicSendIpi (UINT8  ucLocalApicId, ULONG  ulVector)
{
    LOAPIC_IPI_INFO     ipiInfo;

    ipiInfo.IPI_uiAll = (((UINT32)ucLocalApicId << 24) | (1 << 14) |
                        (X86_IRQ_BASE + ulVector));                     /*  SylixOS 中断向量号转换成 x86*/
                                                                        /*  IDTE 号                     */

    return  (x86LocalApicSendIpiRaw(ipiInfo.IPI_uiAll));
}
/*********************************************************************************************************
 END
*********************************************************************************************************/

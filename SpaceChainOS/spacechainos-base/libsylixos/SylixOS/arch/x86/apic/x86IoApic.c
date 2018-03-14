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
** 文   件   名: x86IoApic.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 07 月 29 日
**
** 描        述: x86 体系构架 IOAPIC 相关源文件.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/x86/mpconfig/x86MpApic.h"
/*********************************************************************************************************
  The I/O APIC manages hardware interrupts for an SMP system.

  http://www.intel.com/design/chipsets/datashts/29056601.pdf
*********************************************************************************************************/
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
/*********************************************************************************************************
  IO APIC direct register offset
*********************************************************************************************************/
#define IOAPIC_IND              0x00                                /*  Index Register                  */
#define IOAPIC_DATA             0x10                                /*  IO window (data)                */
#define IOAPIC_IRQPA            0x20                                /*  IRQ Pin Assertion Register      */
#define IOAPIC_EOI              0x40                                /*  EOI Register                    */
/*********************************************************************************************************
  IO APIC indirect register offset
*********************************************************************************************************/
#define IOAPIC_ID               0x00                                /*  IOAPIC ID                       */
#define IOAPIC_VERS             0x01                                /*  IOAPIC Version                  */
#define IOAPIC_ARB              0x02                                /*  IOAPIC Arbitration ID           */
#define IOAPIC_BOOT             0x03                                /*  IOAPIC Boot Configuration       */
#define IOAPIC_REDTBL           0x10                                /*  Redirection Table (24 * 64bit)  */
/*********************************************************************************************************
  ID register bits
*********************************************************************************************************/
#define IOAPIC_ID_SHIFT         24
/*********************************************************************************************************
  Version register bits
*********************************************************************************************************/
#define IOAPIC_MRE_MASK         0x00ff0000                          /*  Max Red. entry mask             */
#define IOAPIC_MRE_SHIFT        16                                  /*  Max Red. entry shift            */
#define IOAPIC_PRQ              0x00008000                          /*  This has IRQ reg                */
#define IOAPIC_VERSION_MASK     0x000000ff                          /*  Version number                  */
/*********************************************************************************************************
  Interrupt delivery type
*********************************************************************************************************/
#define IOAPIC_DT_APIC          0x0                                 /*  APIC serial bus                 */
#define IOAPIC_DT_FS            0x1                                 /*  Front side bus message          */
/*********************************************************************************************************
  类型定义
*********************************************************************************************************/
typedef struct {
    addr_t          IOAPIC_ulBase;                                  /*  IOAPIC 基地址                   */
    UINT            IOAPIC_uiIdteBase;                              /*  IDTE 向量号开始                 */
    UINT8           IOAPIC_ucId;                                    /*  IOAPIC ID                       */
    UINT32          IOAPIC_uiVersion;                               /*  Version 寄存器值                */
} X86_IOAPIC_INTR, *PX86_IOAPIC_INTR;                               /*  IOAPIC 中断控制器               */
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
static LW_SPINLOCK_DEFINE_CACHE_ALIGN(_G_slX86IoApic);              /*  访问寄存器用自旋锁              */

static X86_IOAPIC_INTR  _G_x86IoApicIntrs[IOAPIC_MAX_NR];           /*  IOAPIC 中断控制器               */
static UINT             _G_uiX86IoApicNr           = 1;             /*  IOAPIC 中断控制器数目           */
static UINT             _G_uiX86IoApicRedEntriesNr = 24;            /*  Redirection 条目数目            */
/*********************************************************************************************************
** 函数名称: __x86IoApicRegGet
** 功能描述: IOAPIC 读寄存器
** 输　入  : pIoApicIntr       IOAPIC 中断控制器
**           ucRegIndex        寄存器索引
** 输　出  : 值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT32  __x86IoApicRegGet (PX86_IOAPIC_INTR  pIoApicIntr, UINT8  ucRegIndex)
{
    UINT32  uiValue;
    INTREG  iregInterLevel;

    LW_SPIN_LOCK_QUICK(&_G_slX86IoApic, &iregInterLevel);

    write32(ucRegIndex, pIoApicIntr->IOAPIC_ulBase + IOAPIC_IND);
    uiValue = read32(pIoApicIntr->IOAPIC_ulBase    + IOAPIC_DATA);

    LW_SPIN_UNLOCK_QUICK(&_G_slX86IoApic, iregInterLevel);

    return  (uiValue);
}
/*********************************************************************************************************
** 函数名称: __x86IoApicRegSet
** 功能描述: IOAPIC 写寄存器
** 输　入  : pIoApicIntr       IOAPIC 中断控制器
**           ucRegIndex        寄存器索引
**           uiValue           值
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __x86IoApicRegSet (PX86_IOAPIC_INTR  pIoApicIntr, UINT8  ucRegIndex, UINT32  uiValue)
{
    INTREG  iregInterLevel;

    LW_SPIN_LOCK_QUICK(&_G_slX86IoApic, &iregInterLevel);

    write32(ucRegIndex, pIoApicIntr->IOAPIC_ulBase + IOAPIC_IND);
    write32(uiValue,    pIoApicIntr->IOAPIC_ulBase + IOAPIC_DATA);

    LW_SPIN_UNLOCK_QUICK(&_G_slX86IoApic, iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: __x86IoApicRedGetLo
** 功能描述: 获得 Red 表条目的低 32 位
** 输　入  : pIoApicIntr       IOAPIC 中断控制器
**           ucRedNum          Redirection 条目号
** 输　出  : 值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT32  __x86IoApicRedGetLo (PX86_IOAPIC_INTR  pIoApicIntr, UINT8  ucRedNum)
{
    UINT8  ucRegIndex = IOAPIC_REDTBL + (ucRedNum << 1);

    return  (__x86IoApicRegGet(pIoApicIntr, ucRegIndex));
}
/*********************************************************************************************************
** 函数名称: __x86IoApicRedGetHi
** 功能描述: 获得 Red 表条目的高 32 位
** 输　入  : pIoApicIntr       IOAPIC 中断控制器
**           ucRedNum          Redirection 条目号
** 输　出  : 值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT32  __x86IoApicRedGetHi (PX86_IOAPIC_INTR  pIoApicIntr, UINT8  ucRedNum)
{
    UINT8  ucRegIndex = IOAPIC_REDTBL + (ucRedNum << 1) + 1;

    return  (__x86IoApicRegGet(pIoApicIntr, ucRegIndex));
}
/*********************************************************************************************************
** 函数名称: __x86IoApicRedSetLo
** 功能描述: 设置 Red 表条目的低 32 位
** 输　入  : pIoApicIntr       IOAPIC 中断控制器
**           ucRedNum          Redirection 条目号
**           uiValue           值
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __x86IoApicRedSetLo (PX86_IOAPIC_INTR  pIoApicIntr, UINT8  ucRedNum, UINT32  uiValue)
{
    UINT8  ucRegIndex = IOAPIC_REDTBL + (ucRedNum << 1);

    __x86IoApicRegSet(pIoApicIntr, ucRegIndex, uiValue);
}
/*********************************************************************************************************
** 函数名称: __x86IoApicRedSetHi
** 功能描述: 设置 Red 表条目的高 32 位
** 输　入  : pIoApicIntr       IOAPIC 中断控制器
**           ucRedNum          Redirection 条目号
**           uiValue           值
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __x86IoApicRedSetHi (PX86_IOAPIC_INTR  pIoApicIntr, UINT8  ucRedNum, UINT32  uiValue)
{
    UINT8  ucRegIndex = IOAPIC_REDTBL + (ucRedNum << 1) + 1;

    __x86IoApicRegSet(pIoApicIntr, ucRegIndex, uiValue);
}
/*********************************************************************************************************
** 函数名称: __x86IoApicRedUpdateLo
** 功能描述: 更新 Red 表条目的低 32 位
** 输　入  : pIoApicIntr       IOAPIC 中断控制器
**           ucRedNum          Redirection 条目号
**           uiValue           值
**           uiMask            掩码
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __x86IoApicRedUpdateLo (PX86_IOAPIC_INTR  pIoApicIntr,
                                     UINT8             ucRedNum,
                                     UINT32            uiValue,
                                     UINT32            uiMask)
{
    __x86IoApicRedSetLo(pIoApicIntr, ucRedNum,
            (__x86IoApicRedGetLo(pIoApicIntr, ucRedNum) & ~uiMask) | (uiValue & uiMask));
}
/*********************************************************************************************************
** 函数名称: __x86IoApicIntrLookup
** 功能描述: 通过 IRQ 号查找 IOAPIC 中断控制器和 Redirection 条目号
** 输　入  : ucIrq             IRQ 号
**           pucRedNum         Redirection 条目号
** 输　出  : IOAPIC 中断控制器 或 LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PX86_IOAPIC_INTR  __x86IoApicIntrLookup (UINT8  ucIrq, UINT8  *pucRedNum)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             uiIoApicIdOffset;
    INT               i;

    for (i = 0; i < _G_uiX86IoApicNr; i++) {
        pIoApicIntr = &_G_x86IoApicIntrs[i];

        uiIoApicIdOffset = _G_x86IoApicIntrs->IOAPIC_ucId - _G_ucX86MpApicIoBaseId;

        if ((ucIrq >= (uiIoApicIdOffset * _G_uiX86IoApicRedEntriesNr)) &&
            (ucIrq < ((uiIoApicIdOffset + 1) * _G_uiX86IoApicRedEntriesNr))) {
            *pucRedNum = ucIrq - (uiIoApicIdOffset * _G_uiX86IoApicRedEntriesNr);
            return  (pIoApicIntr);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: x86IoApicRedGetLo
** 功能描述: 获得 Red 表条目的低 32 位
** 输　入  : ucIrq             IRQ 号
** 输　出  : 值 OR ~0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT32  x86IoApicRedGetLo (UINT8  ucIrq)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        return  (__x86IoApicRedGetLo(pIoApicIntr, ucRedNum));
    } else {
        return  ((UINT32)~0);
    }
}
/*********************************************************************************************************
** 函数名称: x86IoApicRedGetHi
** 功能描述: 获得 Red 表条目的高 32 位
** 输　入  : ucIrq             IRQ 号
** 输　出  : 值 OR ~0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT32  x86IoApicRedGetHi (UINT8  ucIrq)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        return  (__x86IoApicRedGetHi(pIoApicIntr, ucRedNum));
    } else {
        return  ((UINT32)~0);
    }
}
/*********************************************************************************************************
** 函数名称: x86IoApicRedSetLo
** 功能描述: 设置 Red 表条目的低 32 位
** 输　入  : ucIrq             IRQ 号
**           uiValue           值
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86IoApicRedSetLo (UINT8  ucIrq, UINT32  uiValue)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        __x86IoApicRedSetLo(pIoApicIntr, ucRedNum, uiValue);
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: x86IoApicRedSetLo
** 功能描述: 设置 Red 表条目的高 32 位
** 输　入  : ucIrq             IRQ 号
**           uiValue           值
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86IoApicRedSetHi (UINT8  ucIrq, UINT32  uiValue)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        __x86IoApicRedSetHi(pIoApicIntr, ucRedNum, uiValue);
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: x86IoApicRedUpdateLo
** 功能描述: 更新 Red 表条目的低 32 位
** 输　入  : ucIrq             IRQ 号
**           uiValue           值
**           uiMask            掩码
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86IoApicRedUpdateLo (UINT8  ucIrq, UINT32  uiValue, UINT32  uiMask)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        __x86IoApicRedUpdateLo(pIoApicIntr, ucRedNum, uiValue, uiMask);
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: x86IoApicIrqEnable
** 功能描述: IOAPIC 使能 IRQ
** 输　入  : ucIrq             IRQ 号
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86IoApicIrqEnable (UINT8  ucIrq)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        __x86IoApicRedUpdateLo(pIoApicIntr, ucRedNum, 0, IOAPIC_INT_MASK);
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: x86IoApicIrqDisable
** 功能描述: IOAPIC 禁能 IRQ
** 输　入  : ucIrq             IRQ 号
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86IoApicIrqDisable (UINT8  ucIrq)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        UINT32  uiLo = __x86IoApicRedGetLo(pIoApicIntr, ucRedNum);
        UINT32  uiIsLevel;

        /*
         * We need to set the trigger mode to edge first, to workaround following
         * situation: when the mask bit is set after the interrupt message has
         * been accepted by a local APIC unit but before the interrupt is
         * dispensed to the processor. In that case, the I/O APIC will always
         * waiting for EOI even re-enable the interrupt(Remote IRR is always set).
         * We switch to edge first, and then the Remote IRR will be cleared, and
         * then we switch back to level if original mode is level.
         */
        uiIsLevel = uiLo & IOAPIC_LEVEL;
        uiLo     &= ~IOAPIC_LEVEL;                                      /*  Set to edge first           */

        __x86IoApicRedSetLo(pIoApicIntr, ucRedNum, uiLo | IOAPIC_INT_MASK);

        if (uiIsLevel) {
            uiLo |= uiIsLevel;
            __x86IoApicRedSetLo(pIoApicIntr, ucRedNum, uiLo | IOAPIC_INT_MASK);
        }

        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: x86IoApicIrqIsEnable
** 功能描述: IOAPIC 是否使能指定的 IRQ
** 输　入  : ucIrq             IRQ 号
** 输　出  : LW_TRUE OR LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL  x86IoApicIrqIsEnable (UINT8  ucIrq)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        return  ((__x86IoApicRedGetLo(pIoApicIntr, ucRedNum) & IOAPIC_INT_MASK) ? (LW_FALSE) : (LW_TRUE));

    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** 函数名称: x86IoApicIrqSetTarget
** 功能描述: IOAPIC 设置 IRQ 中断目标 CPU
** 输　入  : ucIrq                 IRQ 号
**           ucTargetLocalApicId   目标 CPU Local APIC ID
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86IoApicIrqSetTarget (UINT8  ucIrq, UINT8  ucTargetLocalApicId)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        __x86IoApicRedSetHi(pIoApicIntr, ucRedNum,                      /*  分发到该指定的 CPU          */
                            ucTargetLocalApicId << IOAPIC_DESTINATION_SHIFT);
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: x86IoApicIrqGetTarget
** 功能描述: IOAPIC 获得 IRQ 中断目标 CPU
** 输　入  : ucIrq                 IRQ 号
**           pucTargetLocalApicId  目标 CPU Local APIC ID
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86IoApicIrqGetTarget (UINT8  ucIrq, UINT8  *pucTargetLocalApicId)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        *pucTargetLocalApicId = __x86IoApicRedGetHi(pIoApicIntr, ucRedNum) >> IOAPIC_DESTINATION_SHIFT;
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __x86IoApicInit
** 功能描述: 初始化 IOAPIC
** 输　入  : pIoApicIntr       IOAPIC 中断控制器
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __x86IoApicInit (PX86_IOAPIC_INTR  pIoApicIntr)
{
    UINT32  uiRteValue;
    UINT8   ucRedNum;

    /*
     * Set the IOAPIC bus arbitration ID
     */
    __x86IoApicRegSet(pIoApicIntr, IOAPIC_ID, (UINT32)pIoApicIntr->IOAPIC_ucId << IOAPIC_ID_SHIFT);

    /*
     * Boot Config register does not exist in I/O APIC (version 0x1X).
     * It may or may not exist in I/O xAPIC (version 0x2X). Some Intel
     * chipsets with I/O xAPIC don't have Boot Config reg.
     *
     * Attempt to set Interrupt Delivery Mechanism to Front Side Bus
     * (ie. MSI capable), or APIC Serial Bus. Some I/O xAPIC allow this
     * bit to take effect. Some I/O xAPIC allow the bit to be written
     * (for software compat reason), but it has no effect as the mode is
     * hardwired to FSB delivery.
     */
    if (((pIoApicIntr->IOAPIC_uiVersion & IOAPIC_VERSION_MASK) >= 0x20)) {
        if (X86_FEATURE_PROCESSOR_FAMILY >= X86_FAMILY_PENTIUM4) {
            /*
             * Pentium4 and later use FSB for interrupt delivery
             */
            __x86IoApicRegSet(pIoApicIntr, IOAPIC_BOOT, IOAPIC_DT_FS);

        } else {
            /*
             * Pentium up to and including P6 use APIC bus
             */
            __x86IoApicRegSet(pIoApicIntr, IOAPIC_BOOT, IOAPIC_DT_APIC);
        }
    }

    uiRteValue = IOAPIC_EDGE     |                                      /*  边沿信号触发                */
                 IOAPIC_HIGH     |                                      /*  高电平有效(PCI为低电平)     */
                 IOAPIC_FIXED    |                                      /*  固定分发到 DEST 域的所有 CPU*/
                 IOAPIC_INT_MASK |                                      /*  屏蔽中断                    */
                 IOAPIC_PHYSICAL;                                       /*  物理模式, DEST = APIC ID    */

    for (ucRedNum = 0; ucRedNum < _G_uiX86IoApicRedEntriesNr; ucRedNum++) {
        __x86IoApicRedSetLo(pIoApicIntr, ucRedNum,
                            uiRteValue |
                            (pIoApicIntr->IOAPIC_uiIdteBase + ucRedNum));   /*  低 8 位为 x86 IDTE 号   */

        __x86IoApicRedSetHi(pIoApicIntr,
                            ucRedNum, 0 << IOAPIC_DESTINATION_SHIFT);   /*  DEST 域为 0(默认分发到 BSP) */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86IoApicInit
** 功能描述: 初始化 IOAPIC
** 输　入  : puiIoIntNr        IO 中断数目
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86IoApicInitAll (UINT  *puiIoIntNr)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8            *pucLogicalTable;
    UINT32           *puiAddrTable;
    UINT32            uiIoApicNr;
    INT               i;

    LW_SPIN_INIT(&_G_slX86IoApic);
    
    x86MpApicIoApicNrGet(&uiIoApicNr);
    _G_uiX86IoApicNr = uiIoApicNr;

    x86MpApicAddrTableGet(&puiAddrTable);

    x86MpApicLogicalTableGet(&pucLogicalTable);

    for (i = 0; i < uiIoApicNr; i++) {
        pIoApicIntr = &_G_x86IoApicIntrs[i];

        /*
         * Typically assume that the IOAPIC_ID register has been
         * set by the BIOS, however in some cases this isn't what
         * actually happens. If the register has not been initialized,
         * then we must do it ourselves based on system configuration
         * data, either from MPS or ACPI.
         */
        pIoApicIntr->IOAPIC_ucId            = pucLogicalTable[i];
        pIoApicIntr->IOAPIC_ulBase          = puiAddrTable[i];

        pIoApicIntr->IOAPIC_uiVersion       = __x86IoApicRegGet(pIoApicIntr, IOAPIC_VERS);
        _G_uiX86IoApicRedEntriesNr          = ((pIoApicIntr->IOAPIC_uiVersion &
                                                IOAPIC_MRE_MASK) >> IOAPIC_MRE_SHIFT) + 1;

        pIoApicIntr->IOAPIC_uiIdteBase      = X86_IRQ_BASE + \
                 ((pIoApicIntr->IOAPIC_ucId - _G_ucX86MpApicIoBaseId) * _G_uiX86IoApicRedEntriesNr);

        __x86IoApicInit(pIoApicIntr);
    }

    if (puiIoIntNr) {
        *puiIoIntNr = _G_uiX86IoApicNr * _G_uiX86IoApicRedEntriesNr;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86IoApicIrqEoi
** 功能描述: IOAPIC 结束中断
** 输　入  : ucIrq             IRQ 号
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86IoApicIrqEoi (UINT8  ucIrq)
{
    PX86_IOAPIC_INTR  pIoApicIntr;
    UINT8             ucRedNum;

    pIoApicIntr = __x86IoApicIntrLookup(ucIrq, &ucRedNum);
    if (pIoApicIntr) {
        if (((pIoApicIntr->IOAPIC_uiVersion & IOAPIC_VERSION_MASK) >= 0x20)) {
            UINT8  ucX86Vector = __x86IoApicRedGetLo(pIoApicIntr, ucIrq) & IOAPIC_VEC_MASK;
            write32(ucX86Vector, pIoApicIntr->IOAPIC_ulBase + IOAPIC_EOI);
        }
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: x86IoApicPciIrqGet
** 功能描述: 通过 Dest APID ID 和 IRQ 号获得真正的 IRQ 号
** 输　入  : ucApicId          IOAPIC ID
**           ucIrq             IRQ 号
** 输　出  : 真正的 IRQ 号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT8  x86IoApicPciIrqGet (UINT8  ucDestIoApicId, UINT8  ucIrq)
{
    if ((ucDestIoApicId > _G_ucX86MpApicIoBaseId) &&
        (ucDestIoApicId < (_G_ucX86MpApicIoBaseId + _G_uiX86IoApicNr))) {
        ucIrq += ((ucDestIoApicId - _G_ucX86MpApicIoBaseId) * _G_uiX86IoApicRedEntriesNr);
    }

    return  (ucIrq);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

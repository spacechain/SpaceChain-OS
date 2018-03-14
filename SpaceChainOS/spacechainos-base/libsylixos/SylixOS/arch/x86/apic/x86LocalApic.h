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
** 文   件   名: x86LocalApic.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 07 月 29 日
**
** 描        述: x86 体系构架 Local APIC 相关头文件.
*********************************************************************************************************/

#ifndef __ARCH_X86LOCALAPIC_H
#define __ARCH_X86LOCALAPIC_H

#include "arch/x86/common/x86Idt.h"

/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
/*********************************************************************************************************
  Local APIC default base address
*********************************************************************************************************/
#define LOCAL_APIC_BASE                 0xfee00000
/*********************************************************************************************************
  MSI address fields
*********************************************************************************************************/
#define LOAPIC_MSI_BASE                 LOCAL_APIC_BASE
#define LOAPIC_MSI_DESTID_MASK          0x000ff000
#define LOAPIC_MSI_DESTID_SHIFT         12
#define LOAPIC_MSI_REDIRHINT            0x00000008
#define LOAPIC_MSI_DEST_MASK            0x00000004
#define LOAPIC_MSI_DEST_LOG             0x00000004
#define LOAPIC_MSI_DEST_PHYS            0x00000000

#define LOAPIC_MSI_ADDR_HI              ((UINT64)LOAPIC_MSI_BASE >> 32)
#define LOAPIC_MSI_ADDR_LO(cpu)         ((LOAPIC_MSI_BASE & 0xffffffff) | \
                                        (((cpu) << LOAPIC_MSI_DESTID_SHIFT) & LOAPIC_MSI_DESTID_MASK) | \
                                        LOAPIC_MSI_DEST_PHYS)
/*********************************************************************************************************
  MSI data fields
*********************************************************************************************************/
#define LOAPIC_MSI_TRG_MASK             0x00008000
#define LOAPIC_MSI_TRG_LEVEL            0x00008000
#define LOAPIC_MSI_TRG_EDGE             0x00000000
#define LOAPIC_MSI_DELYSTAT             0x00004000
#define LOAPIC_MSI_DESTMODE_MASK        0x00000800
#define LOAPIC_MSI_DESTMODE_LOGICAL     0x00000800
#define LOAPIC_MSI_DESTMODE_PHYS        0x00000000
#define LOAPIC_MSI_DELY_MASK            0x00000700
#define LOAPIC_MSI_DELY_FIXED           0x00000000
#define LOAPIC_MSI_IRQ_MASK             0x000000ff

#define LOAPIC_MSI_DATA(irq)            (((X86_IRQ_BASE + irq) & LOAPIC_MSI_IRQ_MASK) | \
                                        LOAPIC_MSI_TRG_EDGE   | \
                                        LOAPIC_MSI_DELY_FIXED | \
                                        LOAPIC_MSI_DESTMODE_PHYS)
/*********************************************************************************************************
  IPI 中断向量
*********************************************************************************************************/
extern UINT  _G_uiX86IpiVectorBase;                                     /*  IPI 中断开始                */

#define X86_IPI_VECTOR(cpuid)   (_G_uiX86IpiVectorBase + (cpuid))
/*********************************************************************************************************
  数据类型定义
*********************************************************************************************************/
typedef union {
    UINT32      IPI_uiAll;
    struct {
        UINT32  ICR_uiVectorNo  : 8;
        UINT32  ICR_uiDeliMode  : 3;
        UINT32  ICR_uiDestMode  : 1;
        UINT32  ICR_uiReserve2  : 2;
        UINT32  ICR_uiLevel     : 1;
        UINT32  ICR_uiTrigger   : 1;
        UINT32  ICR_uiReserve1  : 2;
        UINT32  ICR_uiShortHand : 2;
        UINT32  ICR_uiReserve0  : 4;
        UINT32  ICR_uiApicId    : 8;
    } ICR;
} LOAPIC_IPI_INFO;                                                      /*  IPI 信息                    */
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
INT     x86LocalApicInit(UINT  *puiLocalApicIntNr);
INT     x86LocalApicSecondaryInit(VOID);

VOID    x86LocalApicEoi(VOID);
UINT8   x86LocalApicId(VOID);

INT     x86LocalApicIrqEnable(UINT8  ucVector);
INT     x86LocalApicIrqDisable(UINT8  ucVector);
BOOL    x86LocalApicIrqIsEnable(UINT8  ucVector);

INT     x86LocalApicSendIpi(UINT8  ucLocalApicId,  ULONG  ulVector);
INT     x86LocalApicSendIpiRaw(UINT32  uiIpiInfo);

#endif                                                                  /*  __ARCH_X86LOCALAPIC_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

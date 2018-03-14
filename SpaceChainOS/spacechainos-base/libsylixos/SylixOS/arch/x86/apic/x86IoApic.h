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
** 文   件   名: x86IoApic.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 07 月 29 日
**
** 描        述: x86 体系构架 IOAPIC 相关头文件.
*********************************************************************************************************/

#ifndef __ARCH_X86IOAPIC_H
#define __ARCH_X86IOAPIC_H

#include "arch/x86/common/x86Idt.h"

/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
/*********************************************************************************************************
  Maximum Number of IOAPICs in the system
*********************************************************************************************************/
#define IOAPIC_MAX_NR                           16
/*********************************************************************************************************
  Redirection table entry bits: upper 32 bit
*********************************************************************************************************/
#define IOAPIC_DESTINATION_MASK                 0xff000000
#define IOAPIC_DESTINATION_SHIFT                24
/*********************************************************************************************************
  Redirection table entry bits: lower 32 bit
*********************************************************************************************************/
#define IOAPIC_INT_MASK                         0x00010000
#define IOAPIC_TRIGGER_MASK                     0x00008000
#define IOAPIC_LEVEL                            0x00008000
#define IOAPIC_EDGE                             0x00000000
#define IOAPIC_REMOTE                           0x00004000
#define IOAPIC_LOW                              0x00002000
#define IOAPIC_HIGH                             0x00000000
#define IOAPIC_LOGICAL                          0x00000800
#define IOAPIC_PHYSICAL                         0x00000000
#define IOAPIC_FIXED                            0x00000000
#define IOAPIC_LOWEST                           0x00000100
#define IOAPIC_SMI                              0x00000200
#define IOAPIC_NMI                              0x00000400
#define IOAPIC_INIT                             0x00000500
#define IOAPIC_EXTINT                           0x00000700
#define IOAPIC_VEC_MASK                         0x000000ff
/*********************************************************************************************************
  PCI 设备 IOAPIC Redirection table entry 低 32 位值
*********************************************************************************************************/
#define PCI_DEV_IOAPIC_RED_LO(irq)                                            \
    (IOAPIC_LEVEL    |           /*  电平信号触发                         */  \
     IOAPIC_LOW      |           /*  低电平有效                           */  \
     IOAPIC_FIXED    |           /*  固定分发到 DEST 域的所有 CPU         */  \
     IOAPIC_INT_MASK |           /*  屏蔽中断                             */  \
     IOAPIC_PHYSICAL |           /*  物理模式, DEST = APIC ID             */  \
     (X86_IRQ_BASE + (irq)))     /*  SylixOS 中断向量号转换成 x86 IDTE 号 */
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
INT     x86IoApicInitAll(UINT  *puiIoIntNr);

UINT32  x86IoApicRedGetLo(UINT8     ucIrq);
UINT32  x86IoApicRedGetHi(UINT8     ucIrq);
INT     x86IoApicRedSetLo(UINT8     ucIrq, UINT32  uiValue);
INT     x86IoApicRedSetHi(UINT8     ucIrq, UINT32  uiValue);
INT     x86IoApicRedUpdateLo(UINT8  ucIrq, UINT32  uiValue, UINT32  uiMask);

INT     x86IoApicIrqEoi(UINT8       ucIrq);
INT     x86IoApicIrqEnable(UINT8    ucIrq);
INT     x86IoApicIrqDisable(UINT8   ucIrq);
BOOL    x86IoApicIrqIsEnable(UINT8  ucIrq);
INT     x86IoApicIrqSetTarget(UINT8 ucIrq,  UINT8  ucTargetLocalApicId);
INT     x86IoApicIrqGetTarget(UINT8 ucIrq,  UINT8 *pucTargetLocalApicId);
UINT8   x86IoApicPciIrqGet(UINT8    ucDestIoApicId, UINT8  ucIrq);

#endif                                                                  /*  __ARCH_X86IOAPIC_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/

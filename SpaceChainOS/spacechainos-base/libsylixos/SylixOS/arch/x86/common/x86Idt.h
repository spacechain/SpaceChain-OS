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
** 文   件   名: x86Idt.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 07 月 05 日
**
** 描        述: x86 体系构架 IDT.
*********************************************************************************************************/

#ifndef __ARCH_X86IDT_H
#define __ARCH_X86IDT_H

/*********************************************************************************************************
  Manage the x86 Interrupt Descriptor Table, the table which maps the
  hardware interrupt lines, hardware exceptions, and software
  interrupts, to software routines. We only define "interrupt gate" IDT entries.

  @see Intel x86 doc, Vol 3, chapter 5
*********************************************************************************************************/
/*********************************************************************************************************
  Number of IDT entries.
*********************************************************************************************************/

#define X86_IDTE_NUM                            256

/*********************************************************************************************************
  Mapping of the CPU exceptions in the IDT (imposed by Intel standards)
*********************************************************************************************************/

#define X86_EXCEPT_BASE                         0
#define X86_EXCEPT_MAX                          31
#define X86_EXCEPT_NUM                          32

/*********************************************************************************************************
  Mapping of the IRQ lines in the IDT
*********************************************************************************************************/

#define X86_IRQ_BASE                            32
#define X86_IRQ_MAX                             255
#define X86_IRQ_NUM                             224

/*********************************************************************************************************
  Standard Intel x86 exceptions.

  @see Intel x86 doc vol 3, section 5.12.
*********************************************************************************************************/

#define X86_EXCEPT_DIVIDE_ERROR                  0                      /*  No error code               */
#define X86_EXCEPT_DEBUG                         1                      /*  No error code               */
#define X86_EXCEPT_NMI_INTERRUPT                 2                      /*  No error code               */
#define X86_EXCEPT_BREAKPOINT                    3                      /*  No error code               */
#define X86_EXCEPT_OVERFLOW                      4                      /*  No error code               */
#define X86_EXCEPT_BOUND_RANGE_EXCEDEED          5                      /*  No error code               */
#define X86_EXCEPT_INVALID_OPCODE                6                      /*  No error code               */
#define X86_EXCEPT_DEVICE_NOT_AVAILABLE          7                      /*  No error code               */
#define X86_EXCEPT_DOUBLE_FAULT                  8                      /*  Yes (Zero)                  */
#define X86_EXCEPT_COPROCESSOR_SEGMENT_OVERRUN   9                      /*  No error code               */
#define X86_EXCEPT_INVALID_TSS                  10                      /*  Yes                         */
#define X86_EXCEPT_SEGMENT_NOT_PRESENT          11                      /*  Yes                         */
#define X86_EXCEPT_STACK_SEGMENT_FAULT          12                      /*  Yes                         */
#define X86_EXCEPT_GENERAL_PROTECTION           13                      /*  Yes                         */
#define X86_EXCEPT_PAGE_FAULT                   14                      /*  Yes                         */
#define X86_EXCEPT_INTEL_RESERVED_1             15                      /*  No                          */
#define X86_EXCEPT_FLOATING_POINT_ERROR         16                      /*  No                          */
#define X86_EXCEPT_ALIGNEMENT_CHECK             17                      /*  Yes (Zero)                  */
#define X86_EXCEPT_MACHINE_CHECK                18                      /*  No                          */
#define X86_EXCEPT_INTEL_RESERVED_2             19                      /*  No                          */
#define X86_EXCEPT_INTEL_RESERVED_3             20                      /*  No                          */
#define X86_EXCEPT_INTEL_RESERVED_4             21                      /*  No                          */
#define X86_EXCEPT_INTEL_RESERVED_5             22                      /*  No                          */
#define X86_EXCEPT_INTEL_RESERVED_6             23                      /*  No                          */
#define X86_EXCEPT_INTEL_RESERVED_7             24                      /*  No                          */
#define X86_EXCEPT_INTEL_RESERVED_8             25                      /*  No                          */
#define X86_EXCEPT_INTEL_RESERVED_9             26                      /*  No                          */
#define X86_EXCEPT_INTEL_RESERVED_10            27                      /*  No                          */
#define X86_EXCEPT_INTEL_RESERVED_11            28                      /*  No                          */
#define X86_EXCEPT_INTEL_RESERVED_12            29                      /*  No                          */
#define X86_EXCEPT_INTEL_RESERVED_13            30                      /*  No                          */
#define X86_EXCEPT_INTEL_RESERVED_14            31                      /*  No                          */

/*********************************************************************************************************
  Usual IRQ levels
*********************************************************************************************************/

#define X86_IRQ_TIMER                           0
#define X86_IRQ_KEYBOARD                        1
#define X86_IRQ_SLAVE_PIC                       2
#define X86_IRQ_COM2                            3
#define X86_IRQ_COM1                            4
#define X86_IRQ_LPT2                            5
#define X86_IRQ_FLOPPY                          6
#define X86_IRQ_LPT1                            7
#define X86_IRQ_8_NOT_DEFINED                   8
#define X86_IRQ_RESERVED_1                      9
#define X86_IRQ_RESERVED_2                      10
#define X86_IRQ_RESERVED_3                      11
#define X86_IRQ_RESERVED_4                      12
#define X86_IRQ_COPROCESSOR                     13
#define X86_IRQ_HARDDISK                        14
#define X86_IRQ_RESERVED_5                      15

#define X86_IRQ_ERROR                           19
#define X86_IRQ_SPURIOUS                        31

#endif                                                                  /*  __ARCH_X86IDT_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/

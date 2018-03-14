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
** 文   件   名: x86Idt.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 07 月 05 日
**
** 描        述: x86 体系构架 IDT.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "x86Segment.h"
#include "x86Idt.h"
/*********************************************************************************************************
  类型定义
*********************************************************************************************************/
/*********************************************************************************************************
  An entry in the IDT, or "IDTE" in the following, ie a reference to
  a interrupt/trap routine or a task gate to handle the sw/hw interrupts and exceptions.

  @see figure 5-2, intel x86 doc, vol 3
*********************************************************************************************************/
struct x86_idt_entry {
    /*
     * Low dword
     */
    UINT16      offset_low;                             /*  15..0, offset of the routine in the segment */
    UINT16      seg_sel;                                /*  31..16, the ID of the segment               */

    /*
     * High dword
     */
    UINT8       reserved:5;                             /*  4..0                                        */
    UINT8       flags:3;                                /*  7..5                                        */
    UINT8       type:3;                                 /*  10..8 (interrupt gate, trap gate...)        */
    UINT8       op_size:1;                              /*  11 (0=16bits instructions, 1=32bits instr.) */
    UINT8       zero:1;                                 /*  12                                          */
    UINT8       dpl:2;                                  /*  14..13                                      */
    UINT8       present:1;                              /*  15                                          */
    UINT16      offset_high;                            /*  31..16                                      */

#if LW_CFG_CPU_WORD_LENGHT == 64
    UINT32      offset_high_63_32;
    UINT32      reserved0;
#endif                                                  /*  LW_CFG_CPU_WORD_LENGHT == 64                */
} __attribute__ ((packed, aligned(8 * (LW_CFG_CPU_WORD_LENGHT / 32))));

typedef struct x86_idt_entry        X86_IDT_ENTRY, *PX86_IDT_ENTRY;
/*********************************************************************************************************
  The IDT register, which stores the address and size of the IDT.

  @see Intel x86 doc vol 3, section 2.4, figure 2-4
*********************************************************************************************************/
struct x86_idt_register {
    UINT16      limit;
    addr_t      base_addr;
} __attribute__ ((packed, aligned(8)));

typedef struct x86_idt_register     X86_IDT_REG;
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static X86_IDT_ENTRY     _G_x86IDT[X86_IDTE_NUM];                       /*  中断描述符表                */
/*********************************************************************************************************
** 函数名称: x86IdtInit
** 功能描述: 初始化 IDT
** 输　入  : NONE
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86IdtInit (VOID)
{
    X86_IDT_REG  idtr;
    INT          i;

    for (i = 0; i < X86_IDTE_NUM; i++) {
        PX86_IDT_ENTRY  pidte = _G_x86IDT + i;

        /*
         * Setup an empty IDTE interrupt gate, see figure 5-2 in Intel x86 doc, vol 3
         */
        pidte->seg_sel  = X86_CS_KERNEL;
        pidte->reserved = 0;
        pidte->flags    = 0;
        pidte->type     = 0x6;                                          /*  Interrupt gate (110b)       */
        pidte->op_size  = 1;                                            /*  32bits instructions         */
        pidte->zero     = 0;

        /*
         * Disable this IDT entry for the moment
         */
        x86IdtSetHandler(i, (addr_t)LW_NULL, 0);
    }

    /*
     * Setup the IDT register, see Intel x86 doc vol 3, section 5.8.
     */

    /*
     * Address of the IDT
     */
    idtr.base_addr = (addr_t)_G_x86IDT;

    /*
     * The limit is the maximum offset in bytes from the base address of the IDT
     */
    idtr.limit = sizeof(_G_x86IDT) - 1;

    /*
     * Commit the IDT into the CPU
     */
    __asm__ __volatile__ ("lidt %0" :: "m"(idtr) : "memory");

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86IdtSecondaryInit
** 功能描述: Secondary CPU 初始化 IDT
** 输　入  : NONE
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86IdtSecondaryInit (VOID)
{
    X86_IDT_REG  idtr;

    /*
     * Setup the IDT register, see Intel x86 doc vol 3, section 5.8.
     */

    /*
     * Address of the IDT
     */
    idtr.base_addr = (addr_t)_G_x86IDT;

    /*
     * The limit is the maximum offset in bytes from the base address of the IDT
     */
    idtr.limit = sizeof(_G_x86IDT) - 1;

    /*
     * Commit the IDT into the CPU
     */
    __asm__ __volatile__ ("lidt %0" :: "m"(idtr) : "memory");

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: x86IdtSetHandler
** 功能描述: 设备 IDTE 的处理函数
** 输　入  : ucX86Vector       x86 异常向量
**           ulHandlerAddr     处理函数
**           iLowestPriviledge 最低特权级  0..3
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  x86IdtSetHandler (UINT8    ucX86Vector,
                       addr_t   ulHandlerAddr,
                       INT      iLowestPriviledge)
{
    PX86_IDT_ENTRY  pidte;

    if ((iLowestPriviledge < 0) || (iLowestPriviledge > 3)) {
        return  (PX_ERROR);
    }

    pidte = _G_x86IDT + ucX86Vector;

    if (ulHandlerAddr != (addr_t)LW_NULL) {
        pidte->offset_low  = ulHandlerAddr & 0xffff;
        pidte->offset_high = (ulHandlerAddr >> 16) & 0xffff;
#if LW_CFG_CPU_WORD_LENGHT == 64
        pidte->offset_high_63_32 = (ulHandlerAddr >> 32) & 0xffffffff;
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 64*/
        pidte->dpl         = iLowestPriviledge;
        pidte->present     = 1;                                         /*  Yes, there is a handler     */

    } else {                                                            /*  Disable this IDT entry      */
        pidte->offset_low  = 0;
        pidte->offset_high = 0;
#if LW_CFG_CPU_WORD_LENGHT == 64
        pidte->offset_high_63_32 = 0;
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 64*/
        pidte->dpl         = 0;
        pidte->present     = 0;                                         /*  No, there is no handler     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

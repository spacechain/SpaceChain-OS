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
** 文   件   名: arch_regs.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 11 月 26 日
**
** 描        述: PowerPC 寄存器相关.
*********************************************************************************************************/

#ifndef __PPC_ARCH_REGS_H
#define __PPC_ARCH_REGS_H

/*********************************************************************************************************
  寄存器在栈帧中的偏移量
*********************************************************************************************************/

#define XR1         (0)
#define XBLK        (XR1   + 4)
#define XR2         (XBLK  + 4)
#define XR3         (XR2   + 4)
#define XR4         (XR3   + 4)
#define XR5         (XR4   + 4)
#define XR6         (XR5   + 4)
#define XR7         (XR6   + 4)
#define XR8         (XR7   + 4)
#define XR9         (XR8   + 4)
#define XR10        (XR9   + 4)
#define XR11        (XR10  + 4)
#define XR12        (XR11  + 4)
#define XR13        (XR12  + 4)
#define XR14        (XR13  + 4)
#define XR15        (XR14  + 4)
#define XR16        (XR15  + 4)
#define XR17        (XR16  + 4)
#define XR18        (XR17  + 4)
#define XR19        (XR18  + 4)
#define XR20        (XR19  + 4)
#define XR21        (XR20  + 4)
#define XR22        (XR21  + 4)
#define XR23        (XR22  + 4)
#define XR24        (XR23  + 4)
#define XR25        (XR24  + 4)
#define XR26        (XR25  + 4)
#define XR27        (XR26  + 4)
#define XR28        (XR27  + 4)
#define XR29        (XR28  + 4)
#define XR30        (XR29  + 4)
#define XR31        (XR30  + 4)
#define XR0         (XR31  + 4)
#define XSRR0       (XR0   + 4)
#define XSRR1       (XSRR0 + 4)
#define XCTR        (XSRR1 + 4)
#define XXER        (XCTR  + 4)
#define XCR         (XXER  + 4)
#define XLR         (XCR   + 4)
#define XPAD2       (XLR   + 4)
#define XPAD3       (XPAD2 + 4)
#define XMSR        (XPAD3 + 4)

/*********************************************************************************************************
  栈帧的大小
*********************************************************************************************************/

#define STACK_FRAME_SIZE   (XMSR + 4)

/*********************************************************************************************************
  汇编代码不包含以下内容
*********************************************************************************************************/

#if (!defined(__ASSEMBLY__)) && (!defined(ASSEMBLY))

/*********************************************************************************************************
  寄存器表
*********************************************************************************************************/

typedef UINT32      ARCH_REG_T;

typedef struct {
    ARCH_REG_T      REG_uiSp;
    ARCH_REG_T      REG_uiBlank;
    ARCH_REG_T      REG_uiR2;
    ARCH_REG_T      REG_uiR3;
    ARCH_REG_T      REG_uiR4;
    ARCH_REG_T      REG_uiR5;
    ARCH_REG_T      REG_uiR6;
    ARCH_REG_T      REG_uiR7;
    ARCH_REG_T      REG_uiR8;
    ARCH_REG_T      REG_uiR9;
    ARCH_REG_T      REG_uiR10;
    ARCH_REG_T      REG_uiR11;
    ARCH_REG_T      REG_uiR12;
    ARCH_REG_T      REG_uiR13;
    ARCH_REG_T      REG_uiR14;
    ARCH_REG_T      REG_uiR15;
    ARCH_REG_T      REG_uiR16;
    ARCH_REG_T      REG_uiR17;
    ARCH_REG_T      REG_uiR18;
    ARCH_REG_T      REG_uiR19;
    ARCH_REG_T      REG_uiR20;
    ARCH_REG_T      REG_uiR21;
    ARCH_REG_T      REG_uiR22;
    ARCH_REG_T      REG_uiR23;
    ARCH_REG_T      REG_uiR24;
    ARCH_REG_T      REG_uiR25;
    ARCH_REG_T      REG_uiR26;
    ARCH_REG_T      REG_uiR27;
    ARCH_REG_T      REG_uiR28;
    ARCH_REG_T      REG_uiR29;
    ARCH_REG_T      REG_uiR30;
    ARCH_REG_T      REG_uiFp;
    ARCH_REG_T      REG_uiR0;
    ARCH_REG_T      REG_uiSrr0;
    ARCH_REG_T      REG_uiSrr1;
    ARCH_REG_T      REG_uiCtr;
    ARCH_REG_T      REG_uiXer;
    ARCH_REG_T      REG_uiCr;
    ARCH_REG_T      REG_uiLr;
    ARCH_REG_T      REG_uiPad2;
    ARCH_REG_T      REG_uiPad3;
    ARCH_REG_T      REG_uiMsr;
} ARCH_REG_CTX;

/*********************************************************************************************************
  EABI 标准调用回溯堆栈表
*********************************************************************************************************/

typedef struct {
    ARCH_REG_T      FP_uiFp;
    ARCH_REG_T      FP_uiLr;
} ARCH_FP_CTX;

/*********************************************************************************************************
  从上下文中获取信息
*********************************************************************************************************/

#define ARCH_REG_CTX_GET_PC(ctx)    ((void *)(ctx).REG_uiSrr0)
#define ARCH_REG_CTX_GET_FRAME(ctx) ((void *)(ctx).REG_uiFp)
#define ARCH_REG_CTX_GET_STACK(ctx) ((void *)&(ctx))                    /*  不准确, 仅为了兼容性设计    */

/*********************************************************************************************************
  堆栈中的寄存器信息所占大小
*********************************************************************************************************/

#define ARCH_REG_CTX_WORD_SIZE  ((sizeof(ARCH_REG_CTX) + sizeof(ARCH_FP_CTX)) / sizeof(ARCH_REG_T))
#define ARCH_STK_MIN_WORD_SIZE  256

/*********************************************************************************************************
  堆栈对齐要求
*********************************************************************************************************/

#define ARCH_STK_ALIGN_SIZE     8

#endif

#endif                                                                  /*  __PPC_ARCH_REGS_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/

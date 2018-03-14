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
** 文件创建日期: 2017 年 02 月 28 日
**
** 描        述: TI C6X DSP 寄存器相关.
*********************************************************************************************************/

#ifndef __C6X_ARCH_REGS_H
#define __C6X_ARCH_REGS_H

#ifndef ASSEMBLY

/*********************************************************************************************************
  寄存器表
*********************************************************************************************************/

typedef UINT32      ARCH_REG_T;

#if LW_CFG_CPU_ENDIAN == 1
#define ARCH_REG_PAIR(a,b) UINT32 a; UINT32 b

#elif LW_CFG_CPU_ENDIAN == 0
#define ARCH_REG_PAIR(a,b) UINT32 b; UINT32 a

#else
#error TI Compiler does not specify endianness

#endif

typedef struct __ARCH_REG_IRQ_CTX {
    UINT32      REG_uiReserved1;
    UINT32      REG_uiReserved2;
    ARCH_REG_PAIR(REG_uiA13,  REG_uiA12);
    ARCH_REG_PAIR(REG_uiB13,  REG_uiB12);
    ARCH_REG_PAIR(REG_uiA11_, REG_uiA10_);
    ARCH_REG_PAIR(REG_uiB11_, REG_uiB10);
    ARCH_REG_PAIR(REG_uiA15,  REG_uiA14);
    UINT32      REG_uiB14;
    UINT32      REG_uiCsr;
    UINT32      REG_uiAmr;
    UINT32      REG_uiGfpgfr;
    UINT32      REG_uiIrp;
    UINT32      REG_uiReserved3;
    UINT32      REG_uiFmcr;
    UINT32      REG_uiFaucr;
    UINT32      REG_uiFadcr;
    UINT32      REG_uiSsr;
    UINT32      REG_uiRilc;
    UINT32      REG_uiItsr;
    UINT32      REG_uiGplyb;
    UINT32      REG_uiGplya;
    UINT32      REG_uiIlc;
    UINT32      REG_uiReserved4;
    ARCH_REG_PAIR(REG_uiA31, REG_uiA30);
    ARCH_REG_PAIR(REG_uiB31, REG_uiB30);
    ARCH_REG_PAIR(REG_uiA29, REG_uiA28);
    ARCH_REG_PAIR(REG_uiB29, REG_uiB28);
    ARCH_REG_PAIR(REG_uiA27, REG_uiA26);
    ARCH_REG_PAIR(REG_uiB27, REG_uiB26);
    ARCH_REG_PAIR(REG_uiA25, REG_uiA24);
    ARCH_REG_PAIR(REG_uiB25, REG_uiB24);
    ARCH_REG_PAIR(REG_uiA23, REG_uiA22);
    ARCH_REG_PAIR(REG_uiB23, REG_uiB22);
    ARCH_REG_PAIR(REG_uiA21, REG_uiA20);
    ARCH_REG_PAIR(REG_uiB21, REG_uiB20);
    ARCH_REG_PAIR(REG_uiA19, REG_uiA18);
    ARCH_REG_PAIR(REG_uiB19, REG_uiB18);
    ARCH_REG_PAIR(REG_uiA17, REG_uiA16);
    ARCH_REG_PAIR(REG_uiB17, REG_uiB16);
    ARCH_REG_PAIR(REG_uiA9,  REG_uiA8);
    ARCH_REG_PAIR(REG_uiB9,  REG_uiB8);
    ARCH_REG_PAIR(REG_uiA7,  REG_uiA6);
    ARCH_REG_PAIR(REG_uiB7,  REG_uiB6);
    ARCH_REG_PAIR(REG_uiA5,  REG_uiA4);
    ARCH_REG_PAIR(REG_uiB5,  REG_uiB4);
    ARCH_REG_PAIR(REG_uiA3,  REG_uiA2);
    ARCH_REG_PAIR(REG_uiB3,  REG_uiB2);
    ARCH_REG_PAIR(REG_uiA1,  REG_uiA0);
    ARCH_REG_PAIR(REG_uiB1,  REG_uiB0);
    UINT32      REG_uiA10;
    UINT32      REG_uiA11;
    UINT32      REG_uiB11;
} ARCH_REG_IRQ_CTX;

typedef struct __ARCH_REG_CTX {
    UINT32      REG_uiReserved0;
    UINT32      REG_uiCsr;
    UINT32      REG_uiAmr;
    UINT32      REG_uiGfpgfr;
    UINT32      REG_uiB3;
    UINT32      REG_uiReserved1;
    UINT32      REG_uiFmcr;
    UINT32      REG_uiFaucr;
    UINT32      REG_uiFadcr;
    UINT32      REG_uiSsr;
    UINT32      REG_uiRilc;
    UINT32      REG_uiItsr;
    UINT32      REG_uiGplyb;
    UINT32      REG_uiGplya;
    UINT32      REG_uiIlc;
    UINT32      REG_uiReserved2;
    ARCH_REG_PAIR(REG_uiA13, REG_uiA12);
    ARCH_REG_PAIR(REG_uiB13, REG_uiB12);
    ARCH_REG_PAIR(REG_uiA11, REG_uiA10);
    ARCH_REG_PAIR(REG_uiB11, REG_uiB10);
    ARCH_REG_PAIR(REG_uiA15, REG_uiA14);
    UINT32      REG_uiB14;
    UINT32      REG_uiA4;
} ARCH_REG_CTX;

/*********************************************************************************************************
  调用回溯堆栈表
*********************************************************************************************************/

typedef struct {
    UINT32          FP_uiRetAddr;
    UINT32          FP_uiArg;
} ARCH_FP_CTX;

/*********************************************************************************************************
  从上下文中获取信息
*********************************************************************************************************/

#define ARCH_REG_CTX_GET_PC(ctx)    ((void *)(ctx).REG_uiIrp)
#define ARCH_REG_CTX_GET_FRAME(ctx) ((void *)(ctx).REG_uiA15)
#define ARCH_REG_CTX_GET_STACK(ctx) ((void *)&(ctx))                    /*  不准确, 仅为了兼容性设计    */

#endif

/*********************************************************************************************************
  堆栈中的寄存器信息所占大小
*********************************************************************************************************/

#define ARCH_REG_CTX_WORD_SIZE  (sizeof(ARCH_REG_CTX) / sizeof(UINT32))
#define ARCH_STK_MIN_WORD_SIZE  (256)

/*********************************************************************************************************
  堆栈对齐要求
*********************************************************************************************************/

#define ARCH_STK_ALIGN_SIZE     8

#endif                                                                  /*  __C6X_ARCH_REGS_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/

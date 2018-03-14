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
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 09 月 01 日
**
** 描        述: MIPS 寄存器相关.
*********************************************************************************************************/

#ifndef __MIPS_ARCH_REGS_H
#define __MIPS_ARCH_REGS_H

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

/*********************************************************************************************************
  MIPS 的寄存器列表
*********************************************************************************************************/

typedef UINT32      ARCH_REG_T;

typedef struct {
#define MIPS_REG_NR             32
    ARCH_REG_T  REG_uiReg[MIPS_REG_NR];                                 /*  32 个通用目的寄存器         */
    ARCH_REG_T  REG_uiCP0DataLO;                                        /*  除数低位寄存器              */
    ARCH_REG_T  REG_uiCP0DataHI;                                        /*  除数高位寄存器              */
    ARCH_REG_T  REG_uiCP0Cause;                                         /*  产生中断或者异常查看的寄存器*/
    ARCH_REG_T  REG_uiCP0Status;                                        /*  CP0 协处理器状态寄存器      */
    ARCH_REG_T  REG_uiCP0EPC;                                           /*  程序计数器寄存器			*/
    ARCH_REG_T  REG_uiCP0BadVAddr;                                      /*  出错地址寄存器              */
} ARCH_REG_CTX;

/*********************************************************************************************************
  MIPS 调用回溯堆栈表
*********************************************************************************************************/

typedef struct {
#define MIPS_ARG_REG_NR         4
    ARCH_REG_T  FP_uiArg[MIPS_ARG_REG_NR];                              /*  4 个参数寄存器              */
} ARCH_FP_CTX;

#endif

/*********************************************************************************************************
  从上下文中获取信息
*********************************************************************************************************/

#define ARCH_REG_CTX_GET_PC(ctx)    ((void *)(ctx).REG_uiCP0EPC)

/*********************************************************************************************************
  堆栈中的寄存器信息所占大小
*********************************************************************************************************/

#define ARCH_REG_CTX_WORD_SIZE  38
#define ARCH_STK_MIN_WORD_SIZE  256

/*********************************************************************************************************
  堆栈对齐要求
*********************************************************************************************************/

#define ARCH_STK_ALIGN_SIZE     8

/*********************************************************************************************************
  MIPS 的寄存器索引
*********************************************************************************************************/

#define REG_ZERO                0                                       /*  wired zero                  */
#define REG_AT                  1                                       /*  assembler temp              */
#define REG_V0                  2                                       /*  return reg 0                */
#define REG_V1                  3                                       /*  return reg 1                */
#define REG_A0                  4                                       /*  arg reg 0                   */
#define REG_A1                  5                                       /*  arg reg 1                   */
#define REG_A2                  6                                       /*  arg reg 2                   */
#define REG_A3                  7                                       /*  arg reg 3                   */
#define REG_T0                  8                                       /*  caller saved 0              */
#define REG_T1                  9                                       /*  caller saved 1              */
#define REG_T2                  10                                      /*  caller saved 2              */
#define REG_T3                  11                                      /*  caller saved 3              */
#define REG_T4                  12                                      /*  caller saved 4              */
#define REG_T5                  13                                      /*  caller saved 5              */
#define REG_T6                  14                                      /*  caller saved 6              */
#define REG_T7                  15                                      /*  caller saved 7              */
#define REG_S0                  16                                      /*  callee saved 0              */
#define REG_S1                  17                                      /*  callee saved 1              */
#define REG_S2                  18                                      /*  callee saved 2              */
#define REG_S3                  19                                      /*  callee saved 3              */
#define REG_S4                  20                                      /*  callee saved 4              */
#define REG_S5                  21                                      /*  callee saved 5              */
#define REG_S6                  22                                      /*  callee saved 6              */
#define REG_S7                  23                                      /*  callee saved 7              */
#define REG_T8                  24                                      /*  caller saved 8              */
#define REG_T9                  25                                      /*  caller saved 9              */
#define REG_K0                  26                                      /*  kernel temp 0               */
#define REG_K1                  27                                      /*  kernel temp 1               */
#define REG_GP                  28                                      /*  global pointer              */
#define REG_SP                  29                                      /*  stack pointer               */
#define REG_S8                  30                                      /*  callee saved 8              */
#define REG_FP                  REG_S8                                  /*  callee saved 8              */
#define REG_RA                  31                                      /*  return address              */

/*********************************************************************************************************
  MIPS 的寄存器栈偏移
*********************************************************************************************************/

#define STK_CTX_SIZE            (ARCH_REG_CTX_WORD_SIZE * 4)

#define STK_OFFSET_LO           ((32 + 0) * 4)
#define STK_OFFSET_HI           ((32 + 1) * 4)
#define STK_OFFSET_CAUSE        ((32 + 2) * 4)
#define STK_OFFSET_SR           ((32 + 3) * 4)
#define STK_OFFSET_EPC          ((32 + 4) * 4)
#define STK_OFFSET_BADVADDR     ((32 + 5) * 4)

#define STK_OFFSET_LAST         ((ARCH_REG_CTX_WORD_SIZE - 1) * 4)

#endif                                                                  /*  __MIPS_ARCH_REGS_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/

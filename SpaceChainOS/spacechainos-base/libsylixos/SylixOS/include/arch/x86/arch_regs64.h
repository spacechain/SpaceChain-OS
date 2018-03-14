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
** 文   件   名: arch_regs64.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 06 月 05 日
**
** 描        述: x86-64 寄存器相关.
*********************************************************************************************************/

#ifndef __X86_ARCH_REGS64_H
#define __X86_ARCH_REGS64_H

#ifndef ASSEMBLY

/*********************************************************************************************************
  寄存器表
*********************************************************************************************************/

typedef UINT64      ARCH_REG_T;

struct __ARCH_REG_CTX {
    UINT16          REG_usGS;                                           /*  4 个段寄存器                */
    UINT16          REG_usFS;
    UINT16          REG_usES;
    UINT16          REG_usDS;

    UINT64          REG_ulRAX;                                          /*  4 个数据寄存器              */
    UINT64          REG_ulRBX;
    UINT64          REG_ulRCX;
    UINT64          REG_ulRDX;

    UINT64          REG_ulRSI;                                          /*  2 个变址和指针寄存器        */
    UINT64          REG_ulRDI;
    UINT64          REG_ulRBP;                                          /*  指针寄存器(RBP)             */

    UINT64          REG_ulR8;                                           /*  8 个数据寄存器              */
    UINT64          REG_ulR9;
    UINT64          REG_ulR10;
    UINT64          REG_ulR11;
    UINT64          REG_ulR12;
    UINT64          REG_ulR13;
    UINT64          REG_ulR14;
    UINT64          REG_ulR15;

#define REG_ERROR   REG_ulError
#define REG_XIP     REG_ulRIP
#define REG_XFLAGS  REG_ulRFLAGS
    UINT64          REG_ulError;                                        /*  ERROR CODE                  */
    UINT64          REG_ulRIP;                                          /*  指令指针寄存器(RIP)         */
    UINT64          REG_ulCS;                                           /*  代码段寄存器(CS)            */
    UINT64          REG_ulRFLAGS;                                       /*  标志寄存器(RFLAGS)          */
    UINT64          REG_ulRSP;                                          /*  原栈指针寄存器(RSP)         */
    UINT64          REG_ulSS;                                           /*  栈段寄存器(SS)              */
} __attribute__((packed));

typedef struct __ARCH_REG_CTX   ARCH_REG_CTX;

/*********************************************************************************************************
  调用回溯堆栈表
*********************************************************************************************************/

typedef struct {
    UINT64          FP_ulRetAddr;
    UINT64          FP_ulSavedRBP;
} ARCH_FP_CTX;

/*********************************************************************************************************
  从上下文中获取信息
*********************************************************************************************************/

#define ARCH_REG_CTX_GET_PC(ctx)    ((PVOID)(ctx).REG_ulRIP)
#define ARCH_REG_CTX_GET_FRAME(ctx) ((PVOID)(ctx).REG_ulRBP)
#define ARCH_REG_CTX_GET_STACK(ctx) ((PVOID)&(ctx))                    /*  不准确, 仅为了兼容性设计    */

#endif                                                                  /*  !defined(ASSEMBLY)          */

/*********************************************************************************************************
  堆栈中的寄存器信息所占大小
*********************************************************************************************************/

#define ARCH_REG_CTX_WORD_SIZE  22
#define ARCH_STK_MIN_WORD_SIZE  512

/*********************************************************************************************************
  堆栈对齐要求
*********************************************************************************************************/

#define ARCH_STK_ALIGN_SIZE     16

/*********************************************************************************************************
  寄存器在任务上下文中的偏移量
*********************************************************************************************************/

#define ARCH_REG_OFFSET_GS      0
#define ARCH_REG_OFFSET_FS      2
#define ARCH_REG_OFFSET_ES      4
#define ARCH_REG_OFFSET_DS      6
#define ARCH_REG_OFFSET_RAX     8
#define ARCH_REG_OFFSET_RBX     16
#define ARCH_REG_OFFSET_RCX     24
#define ARCH_REG_OFFSET_RDX     32
#define ARCH_REG_OFFSET_RSI     40
#define ARCH_REG_OFFSET_RDI     48
#define ARCH_REG_OFFSET_RBP     56
#define ARCH_REG_OFFSET_R8      64
#define ARCH_REG_OFFSET_R9      72
#define ARCH_REG_OFFSET_R10     80
#define ARCH_REG_OFFSET_R11     88
#define ARCH_REG_OFFSET_R12     96
#define ARCH_REG_OFFSET_R13     104
#define ARCH_REG_OFFSET_R14     112
#define ARCH_REG_OFFSET_R15     120
#define ARCH_REG_OFFSET_ERROR   128
#define ARCH_REG_OFFSET_RIP     136
#define ARCH_REG_OFFSET_CS      144
#define ARCH_REG_OFFSET_RFLAGS  152
#define ARCH_REG_OFFSET_RSP     160
#define ARCH_REG_OFFSET_SS      168

#endif                                                                  /*  __X86_ARCH_REGS64_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: assembler.h
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 09 月 01 日
**
** 描        述: MIPS 汇编相关.
*********************************************************************************************************/

#ifndef __ASMMIPS_ASSEMBLER_H
#define __ASMMIPS_ASSEMBLER_H

#include "archprob.h"

/*********************************************************************************************************
  包含定义
*********************************************************************************************************/

#include "arch/mips/arch_def.h"

/*********************************************************************************************************
  mips architecture assembly special code
*********************************************************************************************************/

#if defined(__ASSEMBLY__) || defined(ASSEMBLY)

#ifndef __MP_CFG_H
#include "../SylixOS/config/mp/mp_cfg.h"
#endif

/*********************************************************************************************************
  assembler define
*********************************************************************************************************/

#define EXPORT_LABEL(label)       .global label

#define IMPORT_LABEL(label)       .extern label

#define FUNC_LABEL(func)          func:
#define LINE_LABEL(line)          line:

#define FUNC_DEF(name)                  \
        .text;                          \
        .balign     4;                  \
        .type       symbol, @function;  \
        .ent        name;               \
name:

#define FUNC_END(name)                  \
        .size       name,.-name;        \
        .end        name

#define MACRO_DEF(mfunc...)             \
        .macro      mfunc

#define MACRO_END()                     \
        .endm

#define FILE_BEGIN()                    \
        .set        noreorder;          \
        .balign     4;

#define FILE_END()

#define SECTION(sec)                    \
        .section    sec

#define WEAK(name)                      \
        .weakext    name;               \
        .balign     4;

/*********************************************************************************************************
  MIPS Use the following macros in assembler code
*********************************************************************************************************/

#define LA              la
#define LI              li
#define SW              sw
#define LW              lw
#define ADD             add
#define ADDU            addu
#define ADDI            addi
#define ADDIU           addiu
#define SUB             sub
#define SUBU            subu
#define SLL             sll
#define SLLV            sllv
#define SRL             srl
#define SRLV            srlv
#define SRA             sra
#define SRAV            srav
#define AND             and
#define OR              or
#define ORI             ori
#define XOR             xor
#define XORI            xori
#define JR              jr
#define MOV             move
#define JALR            jalr
#define JAL             jal
#define BNE             bne
#define BAL             bal
#define BLT             blt
#define BLEZ            blez
#define BEQ             beq

#define LONG            .word
#define LONGSIZE        4
#define LONGMASK        3
#define LONGLOG         2

#define PTR             .word
#define PTR_SCALESHIFT  2
#define PTRSIZE         4
#define PTRLOG          2

#define SSNOP           SLL ZER0, ZER0, 1
#define NOPS            SSNOP; SSNOP; SSNOP; SSNOP
#define NOP             nop
#define ERET            eret

#define MTC0(src, dst)                  \
    MTC0    src , dst;                  \
    EHB

#define MFC0(dst, src)                  \
    MFC0    dst , src;                  \
    EHB

#define CTC1(src, dst)                  \
    CTC1    src , dst;                  \
    EHB

#define CFC1(dst, src)                  \
    CFC1    dst , src;                  \
    EHB

#endif                                                                  /*  __ASSEMBLY__                */

#endif                                                                  /*  __ASMMIPS_ASSEMBLER_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

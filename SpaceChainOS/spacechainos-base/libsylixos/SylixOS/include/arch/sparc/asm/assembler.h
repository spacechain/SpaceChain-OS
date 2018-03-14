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
** 创   建   人: Xu.Guizhou (徐贵洲)
**
** 文件创建日期: 2017 年 05 月 15 日
**
** 描        述: SPARC 汇编相关.
*********************************************************************************************************/

#ifndef __ASMSPARC_ASSEMBLER_H
#define __ASMSPARC_ASSEMBLER_H

#include "archprob.h"
#include "arch/sparc/arch_def.h"
#include "arch/sparc/arch_regs.h"

/*********************************************************************************************************
  sparc architecture assembly special code
*********************************************************************************************************/

#if defined(__ASSEMBLY__) || defined(ASSEMBLY)

#ifndef __MP_CFG_H
#include "../SylixOS/config/mp/mp_cfg.h"
#endif

/*********************************************************************************************************
  assembler define
*********************************************************************************************************/

#ifdef __GNUC__
#  define EXPORT_LABEL(label)       .global label
#  define IMPORT_LABEL(label)       .extern label

#  define FUNC_LABEL(func)          func:
#  define LINE_LABEL(line)          line:

#define __ALIGN                     .align 4,0x90

#  define FUNC_DEF(func)    \
        .globl func ;       \
        __ALIGN ;           \
        func:

#  define FUNC_END(func)    \
        .type func, @function ; \
        .size func, .-func
        
#  define MACRO_DEF(mfunc...)   \
        .macro  mfunc
        
#  define MACRO_END()   \
        .endm

#  define FILE_BEGIN()  \
        .text;  \
        .balign 4;

#  define FILE_END()    \
        .end
        
#  define SECTION(sec)  \
        .section sec

#  define WEAK(sym)     \
        .weak sym
#else                                                                   /*  __GNUC__                    */

#endif                                                                  /*  !__GNUC__                   */

/*********************************************************************************************************
  Entry for traps which jump to a programmer-specified trap handler
*********************************************************************************************************/

#define TRAP(_vector, _handler)     \
    MOV     %psr , %l0;             \
    SETHI   %hi(_handler) , %l4;    \
    JMP     %l4 + %lo(_handler);    \
    MOV     _vector, %l3

/*********************************************************************************************************
  Used for the reset trap to avoid a supervisor instruction
*********************************************************************************************************/

#define RTRAP(_vector, _handler)    \
    MOV     %g0 , %l0;              \
    SETHI   %hi(_handler) , %l4;    \
    JMP     %l4 + %lo(_handler);    \
    MOV     _vector , %l3

/*********************************************************************************************************
  Unexpected trap will halt the processor by forcing it to error state
*********************************************************************************************************/

#define BAD_TRAP    \
    TA      0;      \
    NOP;            \
    NOP;            \
    NOP

/*********************************************************************************************************
  Software trap. Treat as BAD_TRAP for the time being...
*********************************************************************************************************/

#define SOFT_TRAP       BAD_TRAP

/*********************************************************************************************************
  Size definitions
*********************************************************************************************************/

#ifndef LW_CFG_KB_SIZE
#define LW_CFG_KB_SIZE  (1024)
#define LW_CFG_MB_SIZE  (1024 * LW_CFG_KB_SIZE)
#define LW_CFG_GB_SIZE  (1024 * LW_CFG_MB_SIZE)
#endif

/*********************************************************************************************************
  Read CPU ID (Now only support LEON3, LEON4!)
*********************************************************************************************************/

#if LW_CFG_SMP_EN > 0
#define READ_CPUID(reg) LEON3_READ_CPUID(reg)
#else
#define READ_CPUID(reg) MOV     %g0 , reg
#endif

/*********************************************************************************************************
  B2BST NOP (Now only support LEON3FT, LEON4FT!)
*********************************************************************************************************/

#define SPARC_B2BST_NOP LEON3FT_B2BST_NOP

#endif                                                                  /*  __ASSEMBLY__                */
/*********************************************************************************************************
  清窗口系统调用
*********************************************************************************************************/

#define ST_FLUSH_WINDOWS    3

#if defined(__ASSEMBLY__) || defined(ASSEMBLY)
#define SPARC_FLUSH_REG_WINDOWS() \
    TA      ST_FLUSH_WINDOWS
#else
#define SPARC_FLUSH_REG_WINDOWS() \
    asm volatile ("ta %0" : : "i" (ST_FLUSH_WINDOWS))
#endif                                                                  /*  __ASSEMBLY__                */

#endif                                                                  /*  __ASMSPARC_ASSEMBLER_H      */
/*********************************************************************************************************
  END
*********************************************************************************************************/

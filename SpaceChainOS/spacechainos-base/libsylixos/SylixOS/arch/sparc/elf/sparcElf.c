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
** 文   件   名: sparcElf.c
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2017 年 10 月 13 日
**
** 描        述: 实现 SPARC 体系结构的 ELF 文件重定位.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#ifdef LW_CFG_CPU_ARCH_SPARC                                            /*  SPARC 体系结构              */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/elf/elf_type.h"
#include "../SylixOS/loader/elf/elf_arch.h"
#include "../SylixOS/loader/include/loader_lib.h"
/*********************************************************************************************************
  SPARC relocs
*********************************************************************************************************/
#define R_SPARC_NONE        0                                   /*  No reloc                            */
#define R_SPARC_8           1                                   /*  Direct 8 bit                        */
#define R_SPARC_16          2                                   /*  Direct 16 bit                       */
#define R_SPARC_32          3                                   /*  Direct 32 bit                       */
#define R_SPARC_DISP8       4                                   /*  PC relative 8 bit                   */
#define R_SPARC_DISP16      5                                   /*  PC relative 16 bit                  */
#define R_SPARC_DISP32      6                                   /*  PC relative 32 bit                  */
#define R_SPARC_WDISP30     7                                   /*  PC relative 30 bit shifted          */
#define R_SPARC_WDISP22     8                                   /*  PC relative 22 bit shifted          */
#define R_SPARC_HI22        9                                   /*  High 22 bit                         */
#define R_SPARC_22          10                                  /*  Direct 22 bit                       */
#define R_SPARC_13          11                                  /*  Direct 13 bit                       */
#define R_SPARC_LO10        12                                  /*  Truncated 10 bit                    */
#define R_SPARC_GOT10       13                                  /*  Truncated 10 bit GOT entry          */
#define R_SPARC_GOT13       14                                  /*  13 bit GOT entry                    */
#define R_SPARC_GOT22       15                                  /*  22 bit GOT entry shifted            */
#define R_SPARC_PC10        16                                  /*  PC relative 10 bit truncated        */
#define R_SPARC_PC22        17                                  /*  PC relative 22 bit shifted          */
#define R_SPARC_WPLT30      18                                  /*  30 bit PC relative PLT address      */
#define R_SPARC_COPY        19                                  /*  Copy symbol at runtime              */
#define R_SPARC_GLOB_DAT    20                                  /*  Create GOT entry                    */
#define R_SPARC_JMP_SLOT    21                                  /*  Create PLT entry                    */
#define R_SPARC_RELATIVE    22                                  /*  Adjust by program base              */
#define R_SPARC_UA32        23                                  /*  Direct 32 bit unaligned             */
/*********************************************************************************************************
  Some SPARC opcodes we need to use for self-modifying code
*********************************************************************************************************/
#define OPCODE_NOP          0x01000000                          /*  nop                                 */
#define OPCODE_CALL         0x40000000                          /*  call ?; add PC-rel word address     */
#define OPCODE_SETHI_G1     0x03000000                          /*  sethi ?, %g1; add value>>10         */
#define OPCODE_JMP_G1       0x81c06000                          /*  jmp %g1+?; add lo 10 bits of value  */
#define OPCODE_SAVE_SP      0x9de3bfa8                          /*  save %sp, -(16+6)*4, %sp            */
#define OPCODE_BA           0x30800000                          /*  b,a ?; add PC-rel word address      */
/*********************************************************************************************************
** 函数名称: archElfRelocateRela
** 功能描述: 重定位 RELA 类型的重定位项
** 输  入  : pmodule      模块
**           prela        RELA 表项
**           psym         符号表项
**           addrSymVal   重定位符号的值
**           pcTargetSec  重定位目目标节区
**           pcBuffer     跳转表起始地址
**           stBuffLen    跳转表长度
** 输  出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archElfRelocateRela (PVOID       pmodule,
                          Elf_Rela   *prela,
                          Elf_Sym    *psym,
                          Elf_Addr    addrSymVal,
                          PCHAR       pcTargetSec,
                          PCHAR       pcBuffer,
                          size_t      stBuffLen)
{
    LW_LD_EXEC_MODULE   *pmod = (LW_LD_EXEC_MODULE *)pmodule;
    Elf_Addr            *paddrWhere;
    Elf_Addr             addrValue;

    paddrWhere = (Elf_Addr *)((size_t)pcTargetSec + prela->r_offset);   /*  计算重定位目标地址          */
    addrValue  = addrSymVal + prela->r_addend;

    switch (ELF_R_TYPE(prela->r_info)) {

    case R_SPARC_DISP32:
        *paddrWhere = addrValue - (Elf_Addr)paddrWhere;
        break;

    case R_SPARC_GLOB_DAT:
    case R_SPARC_32:
    case R_SPARC_UA32:
        *paddrWhere = addrValue;
        break;

    case R_SPARC_WDISP30:
        addrValue  -= (Elf_Addr)paddrWhere;
        *paddrWhere = (*paddrWhere & ~0x3fffffff) | ((addrValue >> 2) & 0x3fffffff);
        break;

    case R_SPARC_WDISP22:
        addrValue  -= (Elf_Addr)paddrWhere;
        *paddrWhere = (*paddrWhere & ~0x3fffff) | ((addrValue >> 2) & 0x3fffff);
        break;

    case R_SPARC_LO10:
        *paddrWhere = (*paddrWhere & ~0x3ff) | (addrValue & 0x3ff);
        break;

    case R_SPARC_HI22:
        *paddrWhere = (*paddrWhere & ~0x3fffff) | ((addrValue >> 10) & 0x3fffff);
        break;

    case R_SPARC_JMP_SLOT:
        paddrWhere[1] = OPCODE_SETHI_G1 | ((addrValue >> 10) & 0x3fffff);
        paddrWhere[2] = OPCODE_JMP_G1 | (addrValue & 0x3ff);
        break;

    case R_SPARC_RELATIVE:
        *paddrWhere += (Elf_Addr)pmod->EMOD_pvBaseAddr + prela->r_addend;
        break;

    case R_SPARC_COPY:
        if (addrSymVal) {
            lib_memcpy((PCHAR)paddrWhere, (CPCHAR)addrSymVal, psym->st_size);
        }
        break;

    default:
        _DebugFormat(__ERRORMESSAGE_LEVEL, "unknown relocate type %d.\r\n", ELF_R_TYPE(prela->r_info));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archElfRelocateRel
** 功能描述: 重定位 REL 类型的重定位项
** 输  入  : pmodule      模块
**           prel         REL 表项
**           addrSymVal   重定位符号的值
**           pcTargetSec  重定位目目标节区
**           pcBuffer     跳转表起始地址
**           stBuffLen    跳转表长度
** 输  出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archElfRelocateRel (PVOID        pmodule,
                         Elf_Rel     *prel,
                         Elf_Addr     addrSymVal,
                         PCHAR        pcTargetSec,
                         PCHAR        pcBuffer,
                         size_t       stBuffLen)
{
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: archElfRGetJmpBuffItemLen
** 功能描述: 返回跳转表项长度
** 输  入  : pmodule       模块
** 输  出  : 跳转表项长度
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archElfRGetJmpBuffItemLen (PVOID  pmodule)
{
    return  (0);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  LW_CFG_CPU_ARCH_SPARC       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

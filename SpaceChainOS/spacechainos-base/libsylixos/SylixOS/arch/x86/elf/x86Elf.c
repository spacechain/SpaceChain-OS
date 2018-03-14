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
** 文   件   名: x86Elf.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 07 月 04 日
**
** 描        述: 实现 x86 体系结构的 ELF 文件重定位.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#ifdef   LW_CFG_CPU_ARCH_X86                                            /*  x86 体系结构                */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/elf/elf_type.h"
#include "../SylixOS/loader/elf/elf_arch.h"
#include "../SylixOS/loader/include/loader_lib.h"
/*********************************************************************************************************
  重定位类型定义
*********************************************************************************************************/
#define R_386_NONE          0                               /*  No reloc                                */
#define R_386_32            1                               /*  Direct 32 bit                           */
#define R_386_PC32          2                               /*  PC relative 32 bit                      */
#define R_386_GOT32         3                               /*  32 bit GOT entry                        */
#define R_386_PLT32         4                               /*  32 bit PLT address                      */
#define R_386_COPY          5                               /*  Copy symbol at runtime                  */
#define R_386_GLOB_DAT      6                               /*  Create GOT entry                        */
#define R_386_JMP_SLOT      7                               /*  Create PLT entry                        */
#define R_386_RELATIVE      8                               /*  Adjust by program base                  */
#define R_386_GOTOFF        9                               /*  32 bit offset to GOT                    */
#define R_386_GOTPC         10                              /*  32 bit signed pc relative offset to GOT */
/*********************************************************************************************************
** 函数名称: archElfRelocateRela
** 功能描述: 重定位 RELA 类型的重定位项
** 输  入  : pmodule      模块
**           prela        RELA 表项
**           addrSymVal   重定位符号的值
**           pcTargetSec  重定位目目标节区
**           pcBuffer     跳转表起始地址
**           stBuffLen    跳转表长度
** 输  出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  archElfRelocateRela (PVOID       pmodule,
                          Elf_Rela   *prela,
                          Elf_Addr    addrSymVal,
                          PCHAR       pcTargetSec,
                          PCHAR       pcBuffer,
                          size_t      stBuffLen)
{
    _DebugFormat(__ERRORMESSAGE_LEVEL, "unknown relocate type %d.\r\n", ELF_R_TYPE(prela->r_info));
    return  (PX_ERROR);
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
** 输  出  : ERROR CODE
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
    Elf_Addr  *paddrWhere;
    Elf_Addr   valueRaw;

    paddrWhere = (Elf_Addr *)((size_t)pcTargetSec + prel->r_offset);    /*  计算重定位目标地址          */

    switch (ELF_R_TYPE(prel->r_info)) {

    case R_386_JMP_SLOT:
        valueRaw    = *paddrWhere;
        *paddrWhere = addrSymVal;
        LD_DEBUG_MSG(("R_386_JMP_SLOT: %lx %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, *paddrWhere));
        break;

    case R_386_RELATIVE:
        valueRaw    = *paddrWhere;
        *paddrWhere = valueRaw + addrSymVal + (Elf_Addr)((LW_LD_EXEC_MODULE *)pmodule)->EMOD_pvBaseAddr;
        LD_DEBUG_MSG(("R_386_RELATIVE: %lx %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, *paddrWhere));
        break;

    case R_386_GLOB_DAT:
        valueRaw    = *paddrWhere;
        *paddrWhere = valueRaw + addrSymVal;
        LD_DEBUG_MSG(("R_386_GLOB_DAT: %lx %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, *paddrWhere));
        break;

    case R_386_32:
        valueRaw    = *paddrWhere;
        *paddrWhere = valueRaw + addrSymVal;
        LD_DEBUG_MSG(("R_386_32: %lx %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, *paddrWhere));
        break;

    case R_386_PC32:
        valueRaw    = *paddrWhere;
        *paddrWhere = valueRaw + addrSymVal - (Elf_Addr)paddrWhere;
        LD_DEBUG_MSG(("R_386_PC32: %lx %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, *paddrWhere));
        break;

    case R_386_NONE:
        break;

    default:
        _DebugFormat(__ERRORMESSAGE_LEVEL, "unknown relocate type %d.\r\n", ELF_R_TYPE(prel->r_info));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
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
    return  (0);                                                        /*  不需要跳转表项              */
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  LW_CFG_CPU_ARCH_X86         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

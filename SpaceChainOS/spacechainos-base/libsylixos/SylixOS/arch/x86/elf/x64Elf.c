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
** 文   件   名: x64Elf.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 06 月 05 日
**
** 描        述: 实现 x86-64 体系结构的 ELF 文件重定位.
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
#define R_X86_64_NONE       0                           /*  No reloc                                    */
#define R_X86_64_64         1                           /*  Direct 64 bit                               */
#define R_X86_64_PC32       2                           /*  PC relative 32 bit signed                   */
#define R_X86_64_GOT32      3                           /*  32 bit GOT entry                            */
#define R_X86_64_PLT32      4                           /*  32 bit PLT address                          */
#define R_X86_64_COPY       5                           /*  Copy symbol at runtime                      */
#define R_X86_64_GLOB_DAT   6                           /*  Create GOT entry                            */
#define R_X86_64_JUMP_SLOT  7                           /*  Create PLT entry                            */
#define R_X86_64_RELATIVE   8                           /*  Adjust by program base                      */
#define R_X86_64_GOTPCREL   9                           /*  32 bit signed pc relative offset to GOT     */
#define R_X86_64_32         10                          /*  Direct 32 bit zero extended                 */
#define R_X86_64_32S        11                          /*  Direct 32 bit sign extended                 */
#define R_X86_64_16         12                          /*  Direct 16 bit zero extended                 */
#define R_X86_64_PC16       13                          /*  16 bit sign extended pc relative            */
#define R_X86_64_8          14                          /*  Direct 8 bit sign extended                  */
#define R_X86_64_PC8        15                          /*  8 bit sign extended pc relative             */
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
    Elf_Addr  *paddrWhere;

    paddrWhere = (Elf_Addr *)((size_t)pcTargetSec + prela->r_offset);   /*  计算重定位目标地址          */

    addrSymVal = addrSymVal + prela->r_addend;                          /*  所有都得加上 ADD            */

    switch (ELF_R_TYPE(prela->r_info)) {

    case R_X86_64_NONE:
        break;

    case R_X86_64_JUMP_SLOT:
    {
#ifdef  __SYLIXOS_DEBUG
        Elf_Addr  valueRaw;
        valueRaw = *paddrWhere;
#endif
        *paddrWhere = addrSymVal;
        LD_DEBUG_MSG(("R_X86_64_JUMP_SLOT: where %lx raw %lx val %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, addrSymVal, *paddrWhere));
    }
    break;

    case R_X86_64_GLOB_DAT:
    {
        Elf_Addr  valueRaw;
        valueRaw = *paddrWhere;
        *paddrWhere = valueRaw + addrSymVal;
        LD_DEBUG_MSG(("R_X86_64_GLOB_DAT: where %lx raw %lx val %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, addrSymVal, *paddrWhere));
    }
    break;

    case R_X86_64_RELATIVE:
    {
#ifdef  __SYLIXOS_DEBUG
        Elf_Addr  valueRaw;
        valueRaw = *paddrWhere;
#endif
        *paddrWhere = addrSymVal + (Elf_Addr)((LW_LD_EXEC_MODULE *)pmodule)->EMOD_pvBaseAddr;
        LD_DEBUG_MSG(("R_X86_64_RELATIVE: where %lx raw %lx val %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, addrSymVal, *paddrWhere));
    }
    break;

    case R_X86_64_64:
    {
        UINT64  valueRaw;
        valueRaw = *(UINT64 *)paddrWhere;
        *(UINT64 *)paddrWhere = valueRaw + addrSymVal;
        LD_DEBUG_MSG(("R_X86_64_64: where %lx raw %llx val %lx -> %llx\r\n",
                     (ULONG)paddrWhere, valueRaw, addrSymVal, *(UINT64 *)paddrWhere));
    }
    break;

    case R_X86_64_32:
    {
        UINT32  valueRaw;
        valueRaw = *(UINT32 *)paddrWhere;
        *(UINT32 *)paddrWhere = valueRaw + addrSymVal;
        LD_DEBUG_MSG(("R_X86_64_32: where %lx raw %x val %lx -> %x\r\n",
                     (ULONG)paddrWhere, valueRaw, addrSymVal, *(UINT32 *)paddrWhere));
    }
    break;

    case R_X86_64_32S:
    {
        INT32  valueRaw;
        valueRaw  = *(INT32 *)paddrWhere;
        *(INT32 *)paddrWhere = valueRaw + addrSymVal;
        LD_DEBUG_MSG(("R_X86_64_32S: where %lx raw %x val %lx -> %x\r\n",
                     (ULONG)paddrWhere, valueRaw, addrSymVal, *(INT32 *)paddrWhere));
    }
    break;

    case R_X86_64_PC32:
    {
        UINT32  valueRaw;
        valueRaw = *(UINT32 *)paddrWhere;
        *(UINT32 *)paddrWhere = valueRaw + addrSymVal - (Elf_Addr)paddrWhere;
        LD_DEBUG_MSG(("R_X86_64_PC32: where %lx raw %x val %lx -> %x\r\n",
                     (ULONG)paddrWhere, valueRaw, addrSymVal, *(UINT32 *)paddrWhere));
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
    _DebugFormat(__ERRORMESSAGE_LEVEL, "unknown relocate type %d.\r\n", ELF_R_TYPE(prel->r_info));
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
    return  (0);                                                        /*  不需要跳转表项              */
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  LW_CFG_CPU_ARCH_X86         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

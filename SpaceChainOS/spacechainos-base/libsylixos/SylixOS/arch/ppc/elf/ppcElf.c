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
** 文   件   名: ppcElf.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 12 月 15 日
**
** 描        述: 实现 PowerPC 体系结构的 ELF 文件重定位.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#ifdef   LW_CFG_CPU_ARCH_PPC                                            /*  PowerPC 体系结构            */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/elf/elf_type.h"
#include "../SylixOS/loader/elf/elf_arch.h"
#include "../SylixOS/loader/include/loader_lib.h"
/*********************************************************************************************************
  PowerPC Relocation types
*********************************************************************************************************/
#define R_PPC_NONE              0
#define R_PPC_ADDR32            1
#define R_PPC_ADDR24            2
#define R_PPC_ADDR16            3
#define R_PPC_ADDR16_LO         4
#define R_PPC_ADDR16_HI         5
#define R_PPC_ADDR16_HA         6
#define R_PPC_ADDR14            7
#define R_PPC_ADDR14_BRTAKEN    8
#define R_PPC_ADDR14_BRNTAKEN   9
#define R_PPC_REL24             10
#define R_PPC_REL14             11
#define R_PPC_REL14_BRTAKEN     12
#define R_PPC_REL14_BRNTAKEN    13
#define R_PPC_GOT16             14
#define R_PPC_GOT16_LO          15
#define R_PPC_GOT16_HI          16
#define R_PPC_GOT16_HA          17
#define R_PPC_PLTREL24          18
#define R_PPC_COPY              19
#define R_PPC_JMP_SLOT          21
#define R_PPC_RELATIVE          22
#define R_PPC_LOCAL24PC         23
#define R_PPC_UADDR32           24
#define R_PPC_UADDR16           25
#define R_PPC_REL32             26
#define R_PPC_PLT32             27
#define R_PPC_PLTREL32          28
#define R_PPC_PLT16_LO          29
#define R_PPC_PLT16_HI          30
#define R_PPC_PLT16_HA          31
#define R_PPC_SDAREL16          32
/*********************************************************************************************************
  Stuff for the PLT
*********************************************************************************************************/
#define PLT_INITIAL_ENTRY_WORDS     18

#define PLT_LONGBRANCH_ENTRY_WORDS  0

#define PLT_TRAMPOLINE_ENTRY_WORDS  6

#define PLT_DOUBLE_SIZE             (1 << 13)

#define PLT_ENTRY_START_WORDS(entry_number)         \
    (PLT_INITIAL_ENTRY_WORDS + (entry_number) * 2   \
      + ((entry_number) > PLT_DOUBLE_SIZE           \
      ? ((entry_number) - PLT_DOUBLE_SIZE) * 2      \
      : 0))

#define PLT_DATA_START_WORDS(num_entries) PLT_ENTRY_START_WORDS(num_entries)
/*********************************************************************************************************
  Macros to build PowerPC opcode words.
*********************************************************************************************************/
#define OPCODE_ADDI(rd, ra, simm) \
    (0x38000000 | (rd) << 21 | (ra) << 16 | ((simm) & 0xffff))

#define OPCODE_ADDIS(rd, ra, simm) \
    (0x3c000000 | (rd) << 21 | (ra) << 16 | ((simm) & 0xffff))

#define OPCODE_ADD(rd, ra, rb) \
    (0x7c000214 | (rd) << 21 | (ra) << 16 | (rb) << 11)

#define OPCODE_B(target) \
    (0x48000000 | ((target) & 0x03fffffc))

#define OPCODE_BA(target) \
    (0x48000002 | ((target) & 0x03fffffc))

#define OPCODE_BCTR() \
    0x4e800420

#define OPCODE_LWZ(rd, d, ra) \
    (0x80000000 | (rd) << 21 | (ra) << 16 | ((d) & 0xffff))

#define OPCODE_LWZU(rd, d, ra) \
    (0x84000000 | (rd) << 21 | (ra) << 16 | ((d) & 0xffff))

#define OPCODE_MTCTR(rd) \
    (0x7C0903A6 | (rd) << 21)

#define OPCODE_RLWINM(ra, rs, sh, mb, me) \
    (0x54000000 | (rs) << 21 | (ra) << 16 | (sh) << 11 | (mb) << 6 | (me) << 1)

#define OPCODE_LI(rd, simm)             OPCODE_ADDI(rd, 0, simm)
#define OPCODE_ADDIS_HI(rd, ra, value)  OPCODE_ADDIS(rd, ra, ((value) + 0x8000) >> 16)
#define OPCODE_LIS_HI(rd, value)        OPCODE_ADDIS_HI(rd, 0, value)
#define OPCODE_SLWI(ra, rs, sh)         OPCODE_RLWINM(ra, rs, sh, 0, 31 - sh)
/*********************************************************************************************************
  跳转表项类型定义
*********************************************************************************************************/
typedef struct  {
    UINT32      JMP_auiInst[4];
} LONG_JMP_ITEM;
/*********************************************************************************************************
** 函数名称: jmpItemFind
** 功能描述: 查找跳转表项，如果没有，则新建一跳转表项
** 输　入  : addrSymVal    要查找的符号的值
**           pcBuffer      跳转表起始地址
**           stBuffLen     跳转表长度
** 输　出  : 返回跳转表项跳转指令的地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static Elf_Addr jmpItemFind (Elf_Addr addrSymVal,
                             PCHAR    pcBuffer,
                             size_t   stBuffLen)
{
    LONG_JMP_ITEM *pJmpItem     = (LONG_JMP_ITEM *)pcBuffer;
    UINT           uiJmpItemCnt = stBuffLen / sizeof(LONG_JMP_ITEM);
    UINT           i;

    for (i = 0; i < uiJmpItemCnt; pJmpItem++, i++) {
        if (pJmpItem->JMP_auiInst[3] == OPCODE_BCTR()) {
            if ((pJmpItem->JMP_auiInst[0] == OPCODE_LIS_HI(12, addrSymVal)) &&
                (pJmpItem->JMP_auiInst[1] == OPCODE_ADDI(12, 12, addrSymVal))) {
                return  (Elf_Addr)(pJmpItem);
            }
        } else {
            break;
        }
    }

    if (i >= uiJmpItemCnt) {
        return  (0);
    }

    pJmpItem->JMP_auiInst[0] = OPCODE_LIS_HI(12, addrSymVal);
    pJmpItem->JMP_auiInst[1] = OPCODE_ADDI(12, 12, addrSymVal);
    pJmpItem->JMP_auiInst[2] = OPCODE_MTCTR(12);
    pJmpItem->JMP_auiInst[3] = OPCODE_BCTR();

    return  (Elf_Addr)(pJmpItem);
}
/*********************************************************************************************************
** 函数名称: archElfRelocateRela
** 功能描述: 重定位 RELA 类型的重定位项
** 输  入  : pmodule      模块
**           prela        RELA 表项
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
                          Elf_Addr    addrSymVal,
                          PCHAR       pcTargetSec,
                          PCHAR       pcBuffer,
                          size_t      stBuffLen)
{
    Elf_Addr  *paddrWhere;
    Elf_Addr   valueRaw;

    paddrWhere = (Elf_Addr *)((size_t)pcTargetSec + prela->r_offset);   /*  计算重定位目标地址          */

    addrSymVal = addrSymVal + prela->r_addend;                          /*  所有都得加上 ADD            */

    switch (ELF32_R_TYPE(prela->r_info)) {

    case R_PPC_NONE:
        break;

    case R_PPC_ADDR16_HA:
        valueRaw = *(Elf32_Half *)paddrWhere;
        *(Elf32_Half *)paddrWhere = ((addrSymVal + 0x8000) >> 16);
        LD_DEBUG_MSG(("R_PPC_ADDR16_HA: %lx %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, (ULONG)*(UINT16 *)paddrWhere));
        break;

    case R_PPC_ADDR16_LO:
        valueRaw = *(Elf32_Half *)paddrWhere;
        *(Elf32_Half *)paddrWhere = (addrSymVal & 0xFFFF);
        LD_DEBUG_MSG(("R_PPC_ADDR16_LO: %lx %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, (ULONG)*(UINT16 *)paddrWhere));
        break;

    case R_PPC_ADDR16_HI:
        valueRaw = *(Elf32_Half *)paddrWhere;
        *(Elf32_Half *)paddrWhere = (addrSymVal >> 16);
        LD_DEBUG_MSG(("R_PPC_ADDR16_HI: %lx %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, (ULONG)*(UINT16 *)paddrWhere));
        break;

    case R_PPC_REL24:
        valueRaw = *paddrWhere;

        if ((Elf32_Sword)(addrSymVal - (Elf_Addr)paddrWhere) < -0x02000000 ||
            (Elf32_Sword)(addrSymVal - (Elf_Addr)paddrWhere) >= 0x02000000) {
            addrSymVal = jmpItemFind(addrSymVal, pcBuffer, stBuffLen);
            if (0 == addrSymVal) {
                return  (PX_ERROR);
            }
        }

        *paddrWhere = (valueRaw & (~0x03fffffc)) |
                      ((addrSymVal - (Elf_Addr)paddrWhere) & 0x03fffffc);

        LD_DEBUG_MSG(("R_PPC_REL24: %lx %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, *paddrWhere));
        break;

    case R_PPC_REL32:
        valueRaw    = *paddrWhere;
        *paddrWhere = addrSymVal - (Elf_Addr)paddrWhere;
        LD_DEBUG_MSG(("R_PPC_REL32: %lx %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, *paddrWhere));
        break;

    case R_PPC_ADDR32:
        valueRaw    = *paddrWhere;
        *paddrWhere = addrSymVal;
        LD_DEBUG_MSG(("R_PPC_ADDR32: %lx %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, *paddrWhere));
        break;

    case R_PPC_RELATIVE:
        valueRaw    = *paddrWhere;
        *paddrWhere = addrSymVal + (Elf_Addr)((LW_LD_EXEC_MODULE *)pmodule)->EMOD_pvBaseAddr;
        LD_DEBUG_MSG(("R_PPC_RELATIVE: %lx %lx -> %lx\r\n",
                     (ULONG)paddrWhere, valueRaw, *paddrWhere));
        break;

    case R_PPC_JMP_SLOT:
        {
            ELF_DYN_DIR  *pdyndir = (ELF_DYN_DIR *)(((LW_LD_EXEC_MODULE *)pmodule)->EMOD_pvFormatInfo);
            Elf32_Sword   delta   = addrSymVal - (Elf32_Word)paddrWhere;

            valueRaw = *paddrWhere;

            if (delta << 6 >> 6 == delta) {
                *paddrWhere = OPCODE_B(delta);

            } else if (addrSymVal <= 0x01fffffc || addrSymVal >= 0xfe000000) {
                *paddrWhere = OPCODE_BA(addrSymVal);

            } else {
                Elf32_Word *pplt, *pdataWords;
                Elf32_Word  index, offset, numPltEntries;

                pplt   = (Elf32_Word *)pdyndir->ulPltGotAddr;
                offset = (Elf32_Word *)paddrWhere - pplt;

                if (offset < PLT_DOUBLE_SIZE * 2 + PLT_INITIAL_ENTRY_WORDS) {
                    index             = (offset - PLT_INITIAL_ENTRY_WORDS) / 2;
                    numPltEntries     = pdyndir->ulJmpRSize / sizeof(Elf32_Rela);
                    pdataWords        = pplt + PLT_DATA_START_WORDS(numPltEntries);
                    pdataWords[index] = addrSymVal;

                    paddrWhere[0] = OPCODE_LI(11, index * 4);
                    paddrWhere[1] = OPCODE_B((PLT_LONGBRANCH_ENTRY_WORDS - (offset + 1)) * 4);

                } else {
                    paddrWhere[0] = OPCODE_LIS_HI(12, addrSymVal);
                    paddrWhere[1] = OPCODE_ADDI(12, 12, addrSymVal);
                    paddrWhere[2] = OPCODE_MTCTR(12);
                    paddrWhere[3] = OPCODE_BCTR();
                }
            }

            LD_DEBUG_MSG(("R_PPC_JMP_SLOT: %lx %lx -> %lx\r\n",
                         (ULONG)paddrWhere, valueRaw, *paddrWhere));
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
    return  (sizeof(LONG_JMP_ITEM));
}
/*********************************************************************************************************
** 函数名称: archElfGotInit
** 功能描述: 初始化 GOT 表
** 输  入  : pmodule       模块
** 输  出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT archElfGotInit (PVOID  pmodule)
{
    Elf32_Word   *pplt, *pdataWords;
    Elf32_Word    numPltEntries;
    ELF_DYN_DIR  *pdyndir = (ELF_DYN_DIR *)(((LW_LD_EXEC_MODULE *)pmodule)->EMOD_pvFormatInfo);

	pplt          = (Elf32_Word *)pdyndir->ulPltGotAddr;
    numPltEntries = pdyndir->ulJmpRSize / sizeof(Elf32_Rela);
    pdataWords    = pplt + PLT_DATA_START_WORDS(numPltEntries);

    pplt[PLT_LONGBRANCH_ENTRY_WORDS]     = OPCODE_ADDIS_HI(11, 11, ((UINT)pdataWords));
    pplt[PLT_LONGBRANCH_ENTRY_WORDS + 1] = OPCODE_LWZ(11, ((UINT)pdataWords), 11);

    pplt[PLT_LONGBRANCH_ENTRY_WORDS + 2] = OPCODE_MTCTR(11);
    pplt[PLT_LONGBRANCH_ENTRY_WORDS + 3] = OPCODE_BCTR();

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
#endif                                                                  /*  LW_CFG_CPU_ARCH_PPC         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

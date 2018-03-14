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
** 文   件   名: elf_arch.h
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2010 年 04 月 17 日
**
** 描        述: 与体系结构相关的 elf 文件重定位接口
*********************************************************************************************************/

#ifndef __ELF_ARCH_H
#define __ELF_ARCH_H

/*********************************************************************************************************
  体系结构相关的重定位函数接口声明
*********************************************************************************************************/

INT archElfRGetJmpBuffItemLen(PVOID  pmodule);                          /*  获取跳转表项大小            */

#if !defined(LW_CFG_CPU_ARCH_C6X) && !defined(LW_CFG_CPU_ARCH_SPARC)
INT archElfRelocateRela(PVOID       pmodule,
                        Elf_Rela   *prela,
                        Elf_Addr    addrSymVal,
                        PCHAR       pcTargetSec,
                        PCHAR       pcBuffer,
                        size_t      stBuffLen);                         /*  RELA 项重定位               */
#else
INT archElfRelocateRela(PVOID       pmodule,
                        Elf_Rela   *prela,
                        Elf_Sym    *psym,
                        Elf_Addr    addrSymVal,
                        PCHAR       pcTargetSec,
                        PCHAR       pcBuffer,
                        size_t      stBuffLen);                         /*  RELA 项重定位               */
#endif

INT archElfRelocateRel(PVOID        pmodule,
                       Elf_Rel     *prel,
                       Elf_Addr     addrSymVal,
                       PCHAR        pcTargetSec,
                       PCHAR        pcBuffer,
                       size_t       stBuffLen);                         /*  REL 项重定位                */

#if defined(LW_CFG_CPU_ARCH_PPC) || defined(LW_CFG_CPU_ARCH_MIPS) || defined(LW_CFG_CPU_ARCH_C6X)
INT archElfGotInit(PVOID  pmodule);                                     /*  初始化GOT表                 */
#endif

#if defined(LW_CFG_CPU_ARCH_C6X)
INT archElfDSBTRemove(PVOID  pmodule);
#endif                                                                  /*  LW_CFG_CPU_ARCH_C6X         */

#endif                                                                  /*  __ELF_ARCH_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/

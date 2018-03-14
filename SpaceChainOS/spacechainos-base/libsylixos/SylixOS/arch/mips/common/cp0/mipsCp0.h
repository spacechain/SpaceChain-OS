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
** 文   件   名: mipsCp0.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 10 月 13 日
**
** 描        述: MIPS CP0 函数库.
*********************************************************************************************************/

#ifndef __MIPSCP0_H
#define __MIPSCP0_H

#include "arch/mips/arch_def.h"

/*********************************************************************************************************
  CP0 register read
*********************************************************************************************************/

#define mipsCp0RegRead(name) \
extern UINT32  mipsCp0##name##Read (VOID);

/*********************************************************************************************************
  CP0 register write
*********************************************************************************************************/

#define mipsCp0RegWrite(name) \
extern VOID  mipsCp0##name##Write (UINT32  uiValue);

/*********************************************************************************************************
  CP0 register read write
*********************************************************************************************************/

#define mipsCp0RegReadWrite(name) \
        mipsCp0RegRead(name) \
        mipsCp0RegWrite(name)

/*********************************************************************************************************
  CP0 register op
*********************************************************************************************************/

mipsCp0RegReadWrite(Status)
mipsCp0RegReadWrite(Cause)
mipsCp0RegReadWrite(EPC)
mipsCp0RegReadWrite(BadVAddr)
mipsCp0RegReadWrite(Count)
mipsCp0RegReadWrite(Compare)
mipsCp0RegReadWrite(PRId)
mipsCp0RegReadWrite(Config)
mipsCp0RegReadWrite(Config1)
mipsCp0RegReadWrite(Config2)
mipsCp0RegReadWrite(Config3)
mipsCp0RegReadWrite(EBase)
mipsCp0RegReadWrite(IntCtl)
mipsCp0RegReadWrite(LLAddr)
mipsCp0RegReadWrite(ECC)
mipsCp0RegReadWrite(CacheErr)
mipsCp0RegReadWrite(ERRPC)

mipsCp0RegReadWrite(Index)
mipsCp0RegReadWrite(Wired)
mipsCp0RegReadWrite(Random)
mipsCp0RegReadWrite(EntryLo0)
mipsCp0RegReadWrite(EntryLo1)
mipsCp0RegReadWrite(Context)
mipsCp0RegReadWrite(PageMask)
mipsCp0RegReadWrite(EntryHi)

mipsCp0RegReadWrite(TagLo)
mipsCp0RegReadWrite(TagHi)

/*********************************************************************************************************
  Loongson2/3x CP0 register op
*********************************************************************************************************/

mipsCp0RegReadWrite(Diag)
mipsCp0RegReadWrite(PageGrain)
mipsCp0RegReadWrite(GSConfig)

#endif                                                                  /*  __MIPSCP0_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/

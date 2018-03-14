/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: ppcMmuE500.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 05 月 05 日
**
** 描        述: PowerPC E500 体系构架 MMU 驱动.
*********************************************************************************************************/

#ifndef __ARCH_PPCMMUE500_H
#define __ARCH_PPCMMUE500_H

VOID   ppcE500MmuInit(LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName);
ULONG  ppcE500MmuDataStorageAbortType(addr_t  ulAbortAddr, BOOL  bIsWrite);
ULONG  ppcE500MmuInstStorageAbortType(addr_t  ulAbortAddr);
INT    ppcE500MmuDataTlbPreLoad(addr_t  ulAddr);

#endif                                                                  /*  __ARCH_PPCMMUE500_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

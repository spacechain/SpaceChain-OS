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
** 文   件   名: mipsCacheLs3x.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 11 月 02 日
**
** 描        述: Loongson3x 体系构架 CACHE 驱动.
*********************************************************************************************************/

#ifndef __MIPS_LS3X_CACHE_H
#define __MIPS_LS3X_CACHE_H

VOID  ls3xCacheFlushAll(VOID);
VOID  ls3xCacheEnableHw(VOID);

VOID  mipsCacheLs3xInit(LW_CACHE_OP *pcacheop,
                        CACHE_MODE   uiInstruction,
                        CACHE_MODE   uiData,
                        CPCHAR       pcMachineName);
VOID  mipsCacheLs3xReset(CPCHAR  pcMachineName);

#endif                                                                  /*  __MIPS_LS3X_CACHE_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

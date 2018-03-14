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
** 文   件   名: sparcCache.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 10 月 10 日
**
** 描        述: SPARC 体系构架 CACHE 驱动.
*********************************************************************************************************/

#ifndef __ARCH_SPARC_CACHE_H
#define __ARCH_SPARC_CACHE_H

VOID  sparcCacheInit(LW_CACHE_OP *pcacheop,
                     CACHE_MODE   uiInstruction,
                     CACHE_MODE   uiData,
                     CPCHAR       pcMachineName);

VOID  sparcCacheReset(CPCHAR  pcMachineName);

INT   sparcCacheFlush(LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes);

#endif                                                                  /*  __ARCH_SPARC_CACHE_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/

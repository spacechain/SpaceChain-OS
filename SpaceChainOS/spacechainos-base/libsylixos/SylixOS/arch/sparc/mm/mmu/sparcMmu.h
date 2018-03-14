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
** 文   件   名: sparcMmu.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 10 月 10 日
**
** 描        述: SPARC 体系构架 MMU 驱动.
*********************************************************************************************************/

#ifndef __ARCH_SPARC_MMU_H
#define __ARCH_SPARC_MMU_H

/*********************************************************************************************************
  错误状态相关宏定义
*********************************************************************************************************/

#define FSTAT_EBE_SHIFT     10
#define FSTAT_EBE_MASK      (0xff << FSTAT_EBE_SHIFT)

#define FSTAT_L_SHIFT       8
#define FSTAT_L_MASK        (0x3 << FSTAT_L_SHIFT)

#define FSTAT_AT_SHIFT      5
#define FSTAT_AT_MASK       (0x7 << FSTAT_AT_SHIFT)

#define FSTAT_AT_LUD        0                       /*  Load from User Data Space                       */
#define FSTAT_AT_LSD        1                       /*  Load from Supervisor Data Space                 */
#define FSTAT_AT_LXUI       2                       /*  Load/Execute from User Instruction Space        */
#define FSTAT_AT_LXSI       3                       /*  Load/Execute from Supervisor Instruction Space  */
#define FSTAT_AT_SUD        4                       /*  Store to User Data Space                        */
#define FSTAT_AT_SSD        5                       /*  Store to Supervisor Data Space                  */
#define FSTAT_AT_SUI        6                       /*  Store to User Instruction Space                 */
#define FSTAT_AT_SSI        7                       /*  Store to Supervisor Instruction Space           */

#define FSTAT_FT_SHIFT      2
#define FSTAT_FT_MASK       (0x7 << FSTAT_FT_SHIFT)

#define FSTAT_FT_NONE       0                       /*  None                                            */
#define FSTAT_FT_INVADDR    1                       /*  Invalid address error                           */
#define FSTAT_FT_PROT       2                       /*  Protection error                                */
#define FSTAT_FT_PRIV       3                       /*  Privilege violation error                       */
#define FSTAT_FT_TRANS      4                       /*  Translation error                               */
#define FSTAT_FT_ACCESSBUS  5                       /*  Access bus error                                */
#define FSTAT_FT_INTERNAL   6                       /*  Internal error                                  */
#define FSTAT_FT_RESV       7                       /*  Reserved                                        */

/*********************************************************************************************************
  FAV The Fault Address Valid bit is set to one if the contents of the Fault
  Address Register are valid. The Fault Address Register need not be valid
  for instruction faults. The Fault Address Register must be valid for data
  faults and translation errors.
*********************************************************************************************************/

#define FSTAT_FAV_SHIFT     1
#define FSTAT_FAV_MASK      (0x1 << FSTAT_FAV_SHIFT)

/*********************************************************************************************************
  OW The Overwrite bit is set to one if the Fault Status Register has been writ-
  ten more than once by faults of the same class since the last time it was
  read. If an instruction access fault occurs and the OW bit is set, system
  software must determine the cause by probing the MMU and/or memory.
*********************************************************************************************************/

#define FSTAT_OW_SHIFT      0
#define FSTAT_OW_MASK       (0x1 << FSTAT_OW_SHIFT)

/*********************************************************************************************************
  函数声明
*********************************************************************************************************/

VOID    sparcMmuInit(LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName);
addr_t  sparcMmuGetFaultAddr(VOID);
UINT32  sparcMmuGetFaultStatus(VOID);

#endif                                                                  /*  __ARCH_SPARC_MMU_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/

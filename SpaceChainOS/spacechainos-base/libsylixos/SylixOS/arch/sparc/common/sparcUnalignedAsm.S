;/*********************************************************************************************************
;**
;**                                    中国软件开源组织
;**
;**                                   嵌入式实时操作系统
;**
;**                                       SylixOS(TM)
;**
;**                               Copyright  All Rights Reserved
;**
;**--------------文件信息--------------------------------------------------------------------------------
;**
;** 文   件   名: sparcUnalignedAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2017 年 10 月 13 日
;**
;** 描        述: SPARC 体系构架非对齐异常处理.
;*********************************************************************************************************/

/* una_asm.S: Kernel unaligned trap assembler helpers.
 *
 * Copyright (C) 1996,2005,2008 David S. Miller (davem@davemloft.net)
 * Copyright (C) 1996,1997 Jakub Jelinek (jj@sunsite.mff.cuni.cz)
 */

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#define EFAULT          14                                              /* Bad address                  */

    FILE_BEGIN()

    EXPORT_LABEL(__do_int_store)
    EXPORT_LABEL(do_int_load)

;/*********************************************************************************************************
;  int __do_int_store(unsigned long *dst_addr, int size,
;                     unsigned long *src_val)
;
;  %o0 = dest_addr
;  %o1 = size
;  %o2 = src_val
;
;  Return '0' on success, -EFAULT on failure.
;*********************************************************************************************************/

FUNC_DEF(__do_int_store)
    LD      [%o2] , %g1
    CMP     %o1 , 2
    BE      2f
    CMP     %o1 , 4
    BE      1f
    SRL     %g1 , 24 , %g2
    SRL     %g1 , 16 , %g7
4:  STB     %g2 , [%o0]
    SRL     %g1 , 8 , %g2
5:  STB     %g7 , [%o0 + 1]
    LD      [%o2 + 4] , %g7
6:  STB     %g2 , [%o0 + 2]
    SRL     %g7 , 24 , %g2
7:  STB     %g1 , [%o0 + 3]
    SRL     %g7 , 16 , %g1
8:  STB     %g2 , [%o0 + 4]
    SRL     %g7 , 8 , %g2
9:  STB     %g1 , [%o0 + 5]
10: STB     %g2 , [%o0 + 6]
    B       0f
11: STB     %g7 , [%o0 + 7]
1:  SRL     %g1 , 16 , %g7
12: STB     %g2 , [%o0]
    SRL     %g1 , 8 , %g2
13: STB     %g7 , [%o0 + 1]
14: STB     %g2 , [%o0 + 2]
    B       0f
15: STB     %g1 , [%o0 + 3]
2:  SRL     %g1 , 8 , %g2
16: STB     %g2 , [%o0]
17: STB     %g1 , [%o0 + 1]
0:  RETL
    MOV     0 , %o0
    FUNC_END(__do_int_store)

;/*********************************************************************************************************
;  int do_int_load(unsigned long *dest_reg, int size,
;                  unsigned long *saddr, int is_signed)
;
;  %o0 = dest_reg
;  %o1 = size
;  %o2 = saddr
;  %o3 = is_signed
;
;  Return '0' on success, -EFAULT on failure.
;*********************************************************************************************************/

FUNC_DEF(do_int_load)
    CMP     %o1 , 8
    BE      9f
    CMP     %o1 , 4
    BE      6f
4:  LDUB    [%o2] , %g1
5:  LDUB    [%o2 + 1] , %g2
    SLL     %g1 , 8 , %g1
    TST     %o3
    BE      3f
    OR      %g1 , %g2 , %g1
    SLL     %g1 , 16 , %g1
    SRA     %g1 , 16 , %g1
3:  B       0f
    ST      %g1 , [%o0]
6:  LDUB    [%o2 + 1] , %g2
    SLL     %g1 , 24 , %g1
7:  LDUB    [%o2 + 2] , %g7
    SLL     %g2 , 16 , %g2
8:  LDUB    [%o2 + 3] , %g3
    SLL     %g7 , 8 , %g7
    OR      %g3 , %g2 , %g3
    OR      %g7 , %g3 , %g7
    OR      %g1 , %g7 , %g1
    B       0f
    ST      %g1 , [%o0]
9:  LDUB    [%o2] , %g1
10: LDUB    [%o2 + 1] , %g2
    SLL     %g1 , 24 , %g1
11: LDUB    [%o2 + 2] , %g7
    SLL     %g2 , 16 , %g2
12: LDUB    [%o2 + 3] , %g3
    SLL     %g7 , 8 , %g7
    OR      %g1 , %g2 , %g1
    OR      %g7 , %g3 , %g7
    OR      %g1 , %g7 , %g7
13: LDUB    [%o2 + 4] , %g1
    ST      %g7 , [%o0]
14: LDUB    [%o2 + 5] , %g2
    SLL     %g1 , 24 , %g1
15: LDUB    [%o2 + 6] , %g7
    SLL     %g2 , 16 , %g2
16: LDUB    [%o2 + 7] , %g3
    SLL     %g7 , 8 , %g7
    OR      %g1 , %g2 , %g1
    OR      %g7 , %g3 , %g7
    OR      %g1 , %g7 , %g7
    ST      %g7 , [%o0 + 4]
0:  RETL
    MOV     0 , %o0
    FUNC_END(do_int_load)

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

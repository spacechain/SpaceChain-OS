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
;** 文   件   名: sparcMpCoreAsm.S
;**
;** 创   建   人: Xu.Guizhou (徐贵洲)
;**
;** 文件创建日期: 2017 年 7 月 26 日
;**
;** 描        述: SPARC 体系构架多核接口驱动.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if LW_CFG_SMP_EN > 0

    FILE_BEGIN()
    
    IMPORT_LABEL(bspMpInt)
    
    EXPORT_LABEL(sparcSpinLock)
    EXPORT_LABEL(sparcSpinTryLock)
    EXPORT_LABEL(sparcSpinUnlock)
    EXPORT_LABEL(archMpCur)
    EXPORT_LABEL(archMpInt)
    
    WEAK(sparcSpinLock)
    WEAK(sparcSpinTryLock)
    WEAK(sparcSpinUnlock)
    WEAK(archMpCur)
    WEAK(archMpInt)

;/*********************************************************************************************************
;  自旋锁
;*********************************************************************************************************/

FUNC_DEF(sparcSpinLock)
LINE_LABEL(sl_retry)
    LDSTUB  [%o0] , %g1                                                 ;/*  原子加载原值并写 1          */
    TST     %g1
    BE      sl_out
    NOP
LINE_LABEL(sl_loop)
    LDUB    [%o0] , %g1                                                 ;/*  避免写太多次                */
    TST     %g1
    BNE     sl_loop
    NOP
    BA,a    sl_retry
    NOP
LINE_LABEL(sl_out)
    RETL
    NOP
    FUNC_END(sparcSpinLock)

FUNC_DEF(sparcSpinTryLock)
    LDSTUB  [%o0] , %o0                                                 ;/*  原子加载原值并写 1          */
    RETL
    NOP
    FUNC_END(sparcSpinTryLock)

FUNC_DEF(sparcSpinUnlock)
    STUB    %g0 , [%o0]                                                 ;/*  写 0                        */
    RETL
    NOP
    FUNC_END(sparcSpinUnlock)

;/*********************************************************************************************************
;  获得当前核 ID
;*********************************************************************************************************/

FUNC_DEF(archMpCur)
    READ_CPUID(%o0)
    RETL
    NOP
    FUNC_END(archMpCur)

;/*********************************************************************************************************
;  产生一次核间中断
;*********************************************************************************************************/

FUNC_DEF(archMpInt)
    BA      bspMpInt
    NOP
    FUNC_END(archMpInt)
    
    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

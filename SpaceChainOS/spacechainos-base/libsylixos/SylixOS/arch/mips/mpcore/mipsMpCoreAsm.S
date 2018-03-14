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
;** 文   件   名: mipsMpCoreAsm.S
;**
;** 创   建   人: Ryan.Xin (信金龙)
;**
;** 文件创建日期: 2015 年 09 月 01 日
;**
;** 描        述: MIPS 体系构架多核接口驱动.
;**
;** 参        考: http://www.ibm.com/developerworks/cn/linux/l-cn-spinlock_mips/index.html
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

    FILE_BEGIN()

#if LW_CFG_SMP_EN > 0

    IMPORT_LABEL(bspMpInt)

    EXPORT_LABEL(mipsSpinLock)
    EXPORT_LABEL(mipsSpinTryLock)
    EXPORT_LABEL(mipsSpinUnlock)
    EXPORT_LABEL(archMpCur)
    EXPORT_LABEL(archMpInt)

    WEAK(mipsSpinLock)
    WEAK(mipsSpinTryLock)
    WEAK(mipsSpinUnlock)
    WEAK(archMpCur)
    WEAK(archMpInt)

;/*********************************************************************************************************
;  自旋锁
;*********************************************************************************************************/

FUNC_DEF(mipsSpinLock)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

1:
    LL      T0 , 0(A0)
    BNEZ    T0 , 1b
    NOP

    LI      T0 , 1
    SC      T0 , 0(A0)
    BEQZ    T0 , 1b
    SYNC

    JR      RA
    NOP

    .set    pop
    FUNC_END(mipsSpinLock)

FUNC_DEF(mipsSpinTryLock)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    LL      T0 , 0(A0)
    BNEZ    T0 , 1f
    NOP

    LI      T0 , 1
    SC      T0 , 0(A0)
    BEQZ    T0 , 1f
    SYNC

    JR      RA
    MOV     V0 , ZERO
1:
    JR      RA
    LI      V0 , 1

    .set    pop
    FUNC_END(mipsSpinTryLock)

FUNC_DEF(mipsSpinUnlock)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    SW      ZERO , 0(A0)
    JR      RA
    NOP

    .set    pop
    FUNC_END(mipsSpinUnlock)

;/*********************************************************************************************************
;  获得当前核 ID
;*********************************************************************************************************/

FUNC_DEF(archMpCur)
    JR      RA
    MOV     V0 , ZERO
    FUNC_END(archMpCur)

;/*********************************************************************************************************
;  产生一次核间中断
;*********************************************************************************************************/

FUNC_DEF(archMpInt)
    J       bspMpInt
    NOP
    FUNC_END(archMpInt)

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

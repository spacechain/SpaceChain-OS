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
;** 文   件   名: ppcMpCoreAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2015 年 12 月 15 日
;**
;** 描        述: PowerPC 体系构架多核接口驱动.
;**
;** 参        考: http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.47.4033&rep=rep1&type=pdf
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if LW_CFG_SMP_EN > 0

    FILE_BEGIN()
    
    IMPORT_LABEL(bspMpInt)
    
    EXPORT_LABEL(ppcSpinLock)
    EXPORT_LABEL(ppcSpinTryLock)
    EXPORT_LABEL(ppcSpinUnlock)
    EXPORT_LABEL(archMpCur)
    EXPORT_LABEL(archMpInt)
    
    WEAK(ppcSpinLock)
    WEAK(ppcSpinTryLock)
    WEAK(ppcSpinUnlock)
    WEAK(archMpCur)
    WEAK(archMpInt)

;/*********************************************************************************************************
;  自旋锁
;*********************************************************************************************************/

FUNC_DEF(ppcSpinLock)
    LI      R0 , 1
1:
    LWARX   R4 , 0 , R3
    CMPWI   CR0, R4 , 0
    BNE     CR0, 1b
    STWCX.  R0 , 0 , R3
    BNE     1b
    ISYNC
    BLR
    FUNC_END()

FUNC_DEF(ppcSpinTryLock)
    LI      R0 , 1
    LWARX   R4 , 0 , R3
    CMPWI   CR0, R4 , 0
    BNE     CR0, 1f
    STWCX.  R0 , 0 , R3
    BNE     1f
    ISYNC
    LI      R3 , 0
    BLR
1:
    LI      R3 , 1
    BLR
    FUNC_END()
    
FUNC_DEF(ppcSpinUnlock)
    LI      R0 , 0
    STW     R0 , 0(R3)
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  获得当前核 ID
;*********************************************************************************************************/

FUNC_DEF(archMpCur)
    LI     R3 , 0
    BLR
    FUNC_END()
    
;/*********************************************************************************************************
;  产生一次核间中断
;*********************************************************************************************************/

FUNC_DEF(archMpInt)
    B       bspMpInt
    FUNC_END()
    
    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

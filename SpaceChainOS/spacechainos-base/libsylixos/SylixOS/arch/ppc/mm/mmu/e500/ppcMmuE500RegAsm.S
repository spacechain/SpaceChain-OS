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
;** 文   件   名: ppcMmuE500RegAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 05 月 04 日
;**
;** 描        述: PowerPC E500 体系构架 MMU 寄存器函数库.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/vmm_cfg.h>

#if LW_CFG_VMM_EN > 0

#include "arch/ppc/arch_e500.h"

    FILE_BEGIN()

    EXPORT_LABEL(ppcE500MmuGetMMUCSR0)
    EXPORT_LABEL(ppcE500MmuSetMMUCSR0)

    EXPORT_LABEL(ppcE500MmuGetMMUCFG)

    EXPORT_LABEL(ppcE500MmuGetTLB0CFG)
    EXPORT_LABEL(ppcE500MmuGetTLB1CFG)

    EXPORT_LABEL(ppcE500MmuGetMAS0)
    EXPORT_LABEL(ppcE500MmuSetMAS0)

    EXPORT_LABEL(ppcE500MmuGetMAS1)
    EXPORT_LABEL(ppcE500MmuSetMAS1)

    EXPORT_LABEL(ppcE500MmuGetMAS2)
    EXPORT_LABEL(ppcE500MmuSetMAS2)

    EXPORT_LABEL(ppcE500MmuGetMAS3)
    EXPORT_LABEL(ppcE500MmuSetMAS3)

    EXPORT_LABEL(ppcE500MmuGetMAS4)
    EXPORT_LABEL(ppcE500MmuSetMAS4)

    EXPORT_LABEL(ppcE500MmuGetMAS6)
    EXPORT_LABEL(ppcE500MmuSetMAS6)

    EXPORT_LABEL(ppcE500MmuGetMAS7)
    EXPORT_LABEL(ppcE500MmuSetMAS7)

    EXPORT_LABEL(ppcE500MmuSetPID0)
    EXPORT_LABEL(ppcE500MmuSetPID1)
    EXPORT_LABEL(ppcE500MmuSetPID2)

;/*********************************************************************************************************
;  SPR GET SET 函数
;*********************************************************************************************************/

MACRO_DEF(SET_SPR, spr)
    MTSPR       \spr , R3
    ISYNC
    BLR
    MACRO_END()

MACRO_DEF(GET_SPR, spr)
    MFSPR       R3 , \spr
    ISYNC
    BLR
    MACRO_END()

FUNC_DEF(ppcE500MmuGetMMUCSR0)
    GET_SPR     MMUCSR0
    FUNC_END()

FUNC_DEF(ppcE500MmuSetMMUCSR0)
    SET_SPR     MMUCSR0
    FUNC_END()

FUNC_DEF(ppcE500MmuGetMMUCFG)
    GET_SPR     MMUCFG
    FUNC_END()

FUNC_DEF(ppcE500MmuGetTLB0CFG)
    GET_SPR     TLB0CFG
    FUNC_END()

FUNC_DEF(ppcE500MmuGetTLB1CFG)
    GET_SPR     TLB1CFG
    FUNC_END()

FUNC_DEF(ppcE500MmuGetMAS0)
    GET_SPR     MAS0
    FUNC_END()

FUNC_DEF(ppcE500MmuSetMAS0)
    SET_SPR     MAS0
    FUNC_END()

FUNC_DEF(ppcE500MmuGetMAS1)
    GET_SPR     MAS1
    FUNC_END()

FUNC_DEF(ppcE500MmuSetMAS1)
    SET_SPR     MAS1
    FUNC_END()

FUNC_DEF(ppcE500MmuGetMAS2)
    GET_SPR     MAS2
    FUNC_END()

FUNC_DEF(ppcE500MmuSetMAS2)
    SET_SPR     MAS2
    FUNC_END()

FUNC_DEF(ppcE500MmuGetMAS3)
    GET_SPR     MAS3
    FUNC_END()

FUNC_DEF(ppcE500MmuSetMAS3)
    SET_SPR     MAS3
    FUNC_END()

FUNC_DEF(ppcE500MmuGetMAS4)
    GET_SPR     MAS4
    FUNC_END()

FUNC_DEF(ppcE500MmuSetMAS4)
    SET_SPR     MAS4
    FUNC_END()

FUNC_DEF(ppcE500MmuGetMAS6)
    GET_SPR     MAS6
    FUNC_END()

FUNC_DEF(ppcE500MmuSetMAS6)
    SET_SPR     MAS6
    FUNC_END()

FUNC_DEF(ppcE500MmuGetMAS7)
    GET_SPR     MAS7
    FUNC_END()

FUNC_DEF(ppcE500MmuSetMAS7)
    SET_SPR     MAS7
    FUNC_END()

FUNC_DEF(ppcE500MmuSetPID0)
    SET_SPR     PID0
    FUNC_END()

FUNC_DEF(ppcE500MmuSetPID1)
    SET_SPR     PID1
    FUNC_END()

FUNC_DEF(ppcE500MmuSetPID2)
    SET_SPR     PID2
    FUNC_END()

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

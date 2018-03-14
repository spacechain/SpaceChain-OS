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
;** 文   件   名: x86MpCoreAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 07 月 04 日
;**
;** 描        述: x86 体系构架多核接口驱动.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if LW_CFG_SMP_EN > 0

    FILE_BEGIN()
    
    EXPORT_LABEL(x86SpinLock)
    EXPORT_LABEL(x86SpinTryLock)
    EXPORT_LABEL(x86SpinUnlock)

    WEAK(x86SpinLock)
    WEAK(x86SpinTryLock)
    WEAK(x86SpinUnlock)

;/*********************************************************************************************************
;  自旋锁
;*********************************************************************************************************/

FUNC_DEF(x86SpinLock)
    MOVL    X86_SP_ARG1(%ESP) , %EDX

1:
    LOCK    BTS  $0, (%EDX)
    JC      2f
    RET

2:
    BT      $0 , (%EDX)
    JNC     1b
    PAUSE                                                               ;/*  适合超线程                  */
    JMP     2b
    FUNC_END(x86SpinLock)

FUNC_DEF(x86SpinTryLock)
    MOVL    X86_SP_ARG1(%ESP) , %EDX

1:
    LOCK    BTS  $0, (%EDX)
    JC      2f

    XORL    %EAX , %EAX
    RET

2:
    MOVL    $1 , %EAX
    RET
    FUNC_END(x86SpinTryLock)
    
FUNC_DEF(x86SpinUnlock)
    MOVL    X86_SP_ARG1(%ESP) , %EDX
    MOVL    $0 , (%EDX)
    RET
    FUNC_END(x86SpinUnlock)

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

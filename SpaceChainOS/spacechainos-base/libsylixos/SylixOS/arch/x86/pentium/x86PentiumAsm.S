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
;** 文   件   名: x86PentiumAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2017 年 04 月 12 日
;**
;** 描        述: x86 体系构架 Pentium 处理器 MSR 相关接口.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

    FILE_BEGIN()

    EXPORT_LABEL(x86PentiumMsrGet)
    EXPORT_LABEL(x86PentiumMsrSet)

;/*********************************************************************************************************
;  获取 Pentium MSR 寄存器值
;*********************************************************************************************************/

FUNC_DEF(x86PentiumMsrGet)
    MOVL    X86_SP_ARG1(%ESP) , %ECX                                    ;/*  Specify MSR to read         */
    RDMSR                                                               ;/*  Read the MSR to %EDX:%EAX   */
    MOVL    X86_SP_ARG2(%ESP) , %ECX
    MOVL    %EAX , 0(%ECX)                                              ;/*  Save low-order 32 bits      */
    MOVL    %EDX , 4(%ECX)                                              ;/*  Save high-order 32 bits     */
    RET
    FUNC_END(x86PentiumMsrGet)

;/*********************************************************************************************************
;  设置 Pentium MSR 寄存器值
;*********************************************************************************************************/

FUNC_DEF(x86PentiumMsrSet)
    MOVL    X86_SP_ARG2(%ESP) , %ECX
    MOVL    0(%ECX) , %EAX                                              ;/*  Low-order 32 bits           */
    MOVL    4(%ECX) , %EDX                                              ;/*  High-order 32 bits          */
    MOVL    X86_SP_ARG1(%ESP) , %ECX                                    ;/*  Specify MSR to write        */
    WRMSR                                                               ;/*  Write %EDX:%EAX to the MSR  */
    RET
    FUNC_END(x86PentiumMsrSet)

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

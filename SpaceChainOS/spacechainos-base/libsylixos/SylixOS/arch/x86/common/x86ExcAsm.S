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
;** 文   件   名: x86ExcAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 07 月 04 日
;**
;** 描        述: x86 体系构架异常/中断处理.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include "x86ContextAsm.h"
#include "x86Idt.h"

    FILE_BEGIN()
    
    IMPORT_LABEL(API_ThreadTcbInter)
    IMPORT_LABEL(API_InterStackBaseGet)
    IMPORT_LABEL(API_InterEnter)
    IMPORT_LABEL(API_InterExit)
    IMPORT_LABEL(_G_pfuncX86IntHandleArray)

    EXPORT_LABEL(_G_ulX86IntEntryArray)

;/*********************************************************************************************************
;  异常/中断入口函数
;*********************************************************************************************************/

    .irp        Id,                                   \
                                                      \
    X86_EXCEPT_DIVIDE_ERROR,                          \
    X86_EXCEPT_DEBUG,                                 \
    X86_EXCEPT_NMI_INTERRUPT,                         \
    X86_EXCEPT_BREAKPOINT,                            \
    X86_EXCEPT_OVERFLOW,                              \
    X86_EXCEPT_BOUND_RANGE_EXCEDEED,                  \
    X86_EXCEPT_INVALID_OPCODE,                        \
    X86_EXCEPT_DEVICE_NOT_AVAILABLE,                  \
    X86_EXCEPT_COPROCESSOR_SEGMENT_OVERRUN,           \
    X86_EXCEPT_INTEL_RESERVED_1,                      \
    X86_EXCEPT_FLOATING_POINT_ERROR,                  \
    X86_EXCEPT_MACHINE_CHECK,                         \
    X86_EXCEPT_INTEL_RESERVED_2,                      \
    X86_EXCEPT_INTEL_RESERVED_3,                      \
    X86_EXCEPT_INTEL_RESERVED_4,                      \
    X86_EXCEPT_INTEL_RESERVED_5,                      \
    X86_EXCEPT_INTEL_RESERVED_6,                      \
    X86_EXCEPT_INTEL_RESERVED_7,                      \
    X86_EXCEPT_INTEL_RESERVED_8,                      \
    X86_EXCEPT_INTEL_RESERVED_9,                      \
    X86_EXCEPT_INTEL_RESERVED_10,                     \
    X86_EXCEPT_INTEL_RESERVED_11,                     \
    X86_EXCEPT_INTEL_RESERVED_12,                     \
    X86_EXCEPT_INTEL_RESERVED_13,                     \
    X86_EXCEPT_INTEL_RESERVED_14,                     \
               32,  33,  34,  35,  36,  37,  38,  39, \
     40,  41,  42,  43,  44,  45,  46,  47,  48,  49, \
     50,  51,  52,  53,  54,  55,  56,  57,  58,  59, \
     60,  61,  62,  63,  64,  65,  66,  67,  68,  69, \
     70,  71,  72,  73,  74,  75,  76,  77,  78,  79, \
     80,  81,  82,  83,  84,  85,  86,  87,  88,  89, \
     90,  91,  92,  93,  94,  95,  96,  97,  98,  99, \
    100, 101, 102, 103, 104, 105, 106, 107, 108, 109, \
    110, 111, 112, 113, 114, 115, 116, 117, 118, 119, \
    120, 121, 122, 123, 124, 125, 126, 127, 128, 129, \
    130, 131, 132, 133, 134, 135, 136, 137, 138, 139, \
    140, 141, 142, 143, 144, 145, 146, 147, 148, 149, \
    150, 151, 152, 153, 154, 155, 156, 157, 158, 159, \
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, \
    170, 171, 172, 173, 174, 175, 176, 177, 178, 179, \
    180, 181, 182, 183, 184, 185, 186, 187, 188, 189, \
    190, 191, 192, 193, 194, 195, 196, 197, 198, 199, \
    200, 201, 202, 203, 204, 205, 206, 207, 208, 209, \
    210, 211, 212, 213, 214, 215, 216, 217, 218, 219, \
    220, 221, 222, 223, 224, 225, 226, 227, 228, 229, \
    230, 231, 232, 233, 234, 235, 236, 237, 238, 239, \
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, \
    250, 251, 252, 253, 254, 255

FUNC_DEF(x86IntEntry\Id)
    INT_SAVE_REGS_FAKE_ERRNO                                            ;/*  所有寄存器入栈              */

    MOVL    %ESP , %ESI                                                 ;/*  使用 %ESI 来记录 %ESP       */

    ;/*
    ; *  if (API_InterEnter() == 1) {
    ; *      current stack = SP;
    ; *      SP = interrupt stack;
    ; *  }
    ; */
    CALL    API_InterEnter
    CMPL    $1 , %EAX
    JNE     1f

    CALL    API_ThreadTcbInter
    MOVL    %ESP , 0(%EAX)

    CALL    API_InterStackBaseGet
    MOVL    %EAX , %ESP

1:
    PUSHL   %ESI
    PUSHL   $\Id
    LEAL    _G_pfuncX86IntHandleArray , %EDI
    CALL    *(\Id * 4)(%EDI)
    ADDL    $8 , %ESP

    CALL    API_InterExit

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */

    FUNC_END(x86IntEntry\Id)

    .endr

;/*********************************************************************************************************
;  异常/中断入口函数
;*********************************************************************************************************/

    .irp        Id,                         \
                                            \
    X86_EXCEPT_DOUBLE_FAULT,                \
    X86_EXCEPT_INVALID_TSS,                 \
    X86_EXCEPT_SEGMENT_NOT_PRESENT,         \
    X86_EXCEPT_STACK_SEGMENT_FAULT,         \
    X86_EXCEPT_GENERAL_PROTECTION,          \
    X86_EXCEPT_PAGE_FAULT,                  \
    X86_EXCEPT_ALIGNEMENT_CHECK

FUNC_DEF(x86IntEntry\Id)
    INT_SAVE_REGS_HW_ERRNO                                              ;/*  所有寄存器入栈              */

    MOVL    %ESP , %ESI                                                 ;/*  使用 %ESI 来记录 %ESP       */

    ;/*
    ; *  if (API_InterEnter() == 1) {
    ; *      current stack = SP;
    ; *      SP = interrupt stack;
    ; *  }
    ; */
    CALL    API_InterEnter
    CMPL    $1 , %EAX
    JNE     1f

    CALL    API_ThreadTcbInter
    MOVL    %ESP , 0(%EAX)

    CALL    API_InterStackBaseGet
    MOVL    %EAX , %ESP

1:
    PUSHL   %ESI
    PUSHL   $\Id
    LEAL    _G_pfuncX86IntHandleArray , %EDI
    CALL    *(\Id * 4)(%EDI)
    ADDL    $8 , %ESP

    CALL    API_InterExit

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */

    FUNC_END(x86IntEntry\Id)

    .endr

;/*********************************************************************************************************
;  异常/中断入口函数数组
;*********************************************************************************************************/

    SECTION(.rodata)
    .p2align    2                                                       ;/*  2^2 = 4 字节对齐            */

LINE_LABEL(_G_ulX86IntEntryArray)
    .irp        Id,                                   \
                                                      \
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9, \
     10,  11,  12,  13,  14,  15,  16,  17,  18,  19, \
     20,  21,  22,  23,  24,  25,  26,  27,  28,  29, \
     30,  31,  32,  33,  34,  35,  36,  37,  38,  39, \
     40,  41,  42,  43,  44,  45,  46,  47,  48,  49, \
     50,  51,  52,  53,  54,  55,  56,  57,  58,  59, \
     60,  61,  62,  63,  64,  65,  66,  67,  68,  69, \
     70,  71,  72,  73,  74,  75,  76,  77,  78,  79, \
     80,  81,  82,  83,  84,  85,  86,  87,  88,  89, \
     90,  91,  92,  93,  94,  95,  96,  97,  98,  99, \
    100, 101, 102, 103, 104, 105, 106, 107, 108, 109, \
    110, 111, 112, 113, 114, 115, 116, 117, 118, 119, \
    120, 121, 122, 123, 124, 125, 126, 127, 128, 129, \
    130, 131, 132, 133, 134, 135, 136, 137, 138, 139, \
    140, 141, 142, 143, 144, 145, 146, 147, 148, 149, \
    150, 151, 152, 153, 154, 155, 156, 157, 158, 159, \
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, \
    170, 171, 172, 173, 174, 175, 176, 177, 178, 179, \
    180, 181, 182, 183, 184, 185, 186, 187, 188, 189, \
    190, 191, 192, 193, 194, 195, 196, 197, 198, 199, \
    200, 201, 202, 203, 204, 205, 206, 207, 208, 209, \
    210, 211, 212, 213, 214, 215, 216, 217, 218, 219, \
    220, 221, 222, 223, 224, 225, 226, 227, 228, 229, \
    230, 231, 232, 233, 234, 235, 236, 237, 238, 239, \
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, \
    250, 251, 252, 253, 254, 255

    .LONG       (x86IntEntry\Id)                                        ;/*  LONG 型是 32 位宽度         */

    .endr

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

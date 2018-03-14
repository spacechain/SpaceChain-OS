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
;** 文   件   名: x86LibAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 07 月 04 日
;**
;** 描        述: x86 体系构架内部库.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/cache_cfg.h>

    FILE_BEGIN()

    IMPORT_LABEL(__setjmpSetup)
    IMPORT_LABEL(__sigsetjmpSetup)
    IMPORT_LABEL(__longjmpSetup)
    IMPORT_LABEL(__siglongjmpSetup)
    IMPORT_LABEL(bspReboot)
    
    EXPORT_LABEL(setjmp)
    EXPORT_LABEL(sigsetjmp)
    EXPORT_LABEL(longjmp)
    EXPORT_LABEL(siglongjmp)
    
    EXPORT_LABEL(archFindLsb)
    EXPORT_LABEL(archFindMsb)

    EXPORT_LABEL(archIntDisable)
    EXPORT_LABEL(archIntEnable)
    EXPORT_LABEL(archIntEnableForce)
    EXPORT_LABEL(archReboot)
    EXPORT_LABEL(archBogoMipsLoop)

    WEAK(archIntDisable)
    WEAK(archIntEnable)
    WEAK(archIntEnableForce)

;/*********************************************************************************************************
;  x86 关闭总中断
;*********************************************************************************************************/

FUNC_DEF(archIntDisable)
    PUSHFL
    CLI
    POPL    %EAX
    RET
    FUNC_END(archIntDisable)
    
FUNC_DEF(archIntEnable)
    MOVL    X86_SP_ARG1(%ESP) , %EAX
    TESTL   $X86_EFLAGS_IF , %EAX
    JZ      __archIntEnable
    STI
LINE_LABEL(__archIntEnable)
    RET
    FUNC_END(archIntEnable)
    
FUNC_DEF(archIntEnableForce)
    STI
    RET
    FUNC_END(archIntEnableForce)
    
;/*********************************************************************************************************
;  x86 前导零运算
;*********************************************************************************************************/

FUNC_DEF(archFindLsb)
    MOVL    X86_SP_ARG1(%ESP) , %EDX                                    ;/*  %EDX = i                    */

    BSFL    %EDX , %EAX                                                 ;/*  从 0 位开始搜索             */
    JNE     __archFindLsb                                               ;/*  0 意味着没有位被设置        */

    XORL    %EAX , %EAX                                                 ;/*  return 0                    */
    RET

LINE_LABEL(__archFindLsb)                                               ;/*  找到了                      */
    INCL    %EAX                                                        ;/*  +1                          */
    RET
    FUNC_END(archFindLsb)


FUNC_DEF(archFindMsb)
    MOVL    X86_SP_ARG1(%ESP) , %EDX                                    ;/*  %EDX = i                    */

    BSRL    %EDX , %EAX                                                 ;/*  从 31 位开始搜索            */
    JNE     __archFindMsb                                               ;/*  0 意味着没有位被设置        */

    XORL    %EAX , %EAX                                                 ;/*  return 0                    */
    RET

LINE_LABEL(__archFindMsb)                                               ;/*  找到了                      */
    INCL    %EAX                                                        ;/*  +1                          */
    RET
    FUNC_END(archFindMsb)

;/*********************************************************************************************************
;  注意: setjmp 与 longjmp 上下文结构与线程上下文结构不同
;*********************************************************************************************************/

#define _X86_EIP_OFFSET        0x00
#define _X86_EFLAGS_OFFSET     0x04
#define _X86_EDI_OFFSET        0x08
#define _X86_ESI_OFFSET        0x0c
#define _X86_EBP_OFFSET        0x10
#define _X86_EBX_OFFSET        0x14
#define _X86_ECX_OFFSET        0x18
#define _X86_CS_OFFSET         0x1c
#define _X86_ESP_OFFSET        0x20

;/*********************************************************************************************************
;  保存寄存器宏
;*********************************************************************************************************/

MACRO_DEF(SAVE_NON_VOLATILE_REG)
    MOVL    X86_SP_ARG1(%ESP) , %EDX

    MOVL    0(%ESP) , %EAX                                              ;/*  栈顶内容是 EIP              */
    MOVL    %EAX , _X86_EIP_OFFSET(%EDX)                                ;/*  保存 EIP                    */

    PUSHFL
    POPL    _X86_EFLAGS_OFFSET(%EDX)                                    ;/*  保存 EFLAGS                 */

    MOVL    %EDI , _X86_EDI_OFFSET(%EDX)                                ;/*  保存 nonvolatile 寄存器     */
    MOVL    %ESI , _X86_ESI_OFFSET(%EDX)

    MOVL    %EBP , _X86_EBP_OFFSET(%EDX)

    MOVL    %EBX , _X86_EBX_OFFSET(%EDX)
    MOVL    %ECX , _X86_ECX_OFFSET(%EDX)

    MOVL    %CS  , %EAX
    MOVL    %EAX , _X86_CS_OFFSET(%EDX)                                 ;/*  保存 CS                     */

    MOVL    %ESP , %EAX
    ADDL    $4   , %EAX                                                 ;/*  EAX = EIP 出栈后的 ESP      */
    MOVL    %EAX , _X86_ESP_OFFSET(%EDX)                                ;/*  保存 ESP                    */
    MACRO_END()

;/*********************************************************************************************************
;  恢复寄存器宏
;*********************************************************************************************************/

MACRO_DEF(RESTORE_NON_VOLATILE_REG)
    CLI                                                                 ;/*  关闭中断(后面会恢复 EFLAGS) */

    MOVL    X86_SP_ARG1(%ESP) , %EDX
    MOVL    X86_SP_ARG2(%ESP) , %EAX

    MOVL    _X86_EDI_OFFSET(%EDX) , %EDI                                ;/*  恢复 nonvolatile 寄存器     */
    MOVL    _X86_ESI_OFFSET(%EDX) , %ESI

    MOVL    _X86_EBP_OFFSET(%EDX) , %EBP

    MOVL    _X86_EBX_OFFSET(%EDX) , %EBX
    MOVL    _X86_ECX_OFFSET(%EDX) , %ECX

    MOVL    _X86_ESP_OFFSET(%EDX) , %ESP                                ;/*  恢复 ESP                    */

    PUSHL   _X86_EFLAGS_OFFSET(%EDX)                                    ;/*  压栈保存的 EFLAGS           */
    PUSHL   _X86_CS_OFFSET(%EDX)                                        ;/*  压栈保存的 CS               */
    PUSHL   _X86_EIP_OFFSET(%EDX)                                       ;/*  压栈保存的 EIP(后面会 IRET) */
    MACRO_END()

;/*********************************************************************************************************
;  sigsetjmp (参数为 jmp_buf, mask_saved)
;*********************************************************************************************************/

FUNC_DEF(sigsetjmp)
    ;/*
    ; * VOID  __sigsetjmpSetup (sigjmp_buf sigjmpEnv, INT iSaveSigs)
    ; */
    PUSHL   X86_SP_ARG2(%ESP)
    PUSHL   (X86_SP_ARG1 + 4)(%ESP)
    CALL    __sigsetjmpSetup
    ADDL    $8 , %ESP

    SAVE_NON_VOLATILE_REG

    XORL    %EAX , %EAX                                                 ;/*  return 0                    */
    RET
    FUNC_END(sigsetjmp)

;/*********************************************************************************************************
;  siglongjmp (参数为 jmp_buf, retval)
;*********************************************************************************************************/

FUNC_DEF(siglongjmp)
    ;/*
    ; * VOID  __siglongjmpSetup (sigjmp_buf sigjmpEnv, INT iVal)
    ; */
    PUSHL   X86_SP_ARG2(%ESP)
    PUSHL   (X86_SP_ARG1 + 4)(%ESP)
    CALL    __siglongjmpSetup
    ADDL    $8 , %ESP

    RESTORE_NON_VOLATILE_REG

    CMPL    $0 , %EAX
    JNE     siglongjmpnotzero
    MOVL    $1 , %EAX

LINE_LABEL(siglongjmpnotzero)
    IRET                                                                ;/*  IRET 等于弹出 CS EIP EFLAGS */
    FUNC_END(siglongjmp)

;/*********************************************************************************************************
;  setjmp (参数为 jmp_buf)
;*********************************************************************************************************/

FUNC_DEF(setjmp)
    ;/*
    ; * void __setjmpSetup (jmp_buf jmpbuf)
    ; */
    PUSHL   X86_SP_ARG1(%ESP)
    CALL    __setjmpSetup
    ADDL    $4 , %ESP

    SAVE_NON_VOLATILE_REG

    XORL    %EAX , %EAX                                                 ;/*  return 0                    */
    RET
    FUNC_END(setjmp)

;/*********************************************************************************************************
;  longjmp (参数为 jmp_buf, retval)
;*********************************************************************************************************/

FUNC_DEF(longjmp)
    ;/*
    ; * void __longjmpSetup (jmp_buf jmpbuf, int val)
    ; */
    PUSHL   X86_SP_ARG2(%ESP)
    PUSHL   (X86_SP_ARG1 + 4)(%ESP)
    CALL    __longjmpSetup
    ADDL    $8 , %ESP

    RESTORE_NON_VOLATILE_REG

    CMPL    $0 , %EAX
    JNE     longjmpnotzero
    MOVL    $1 , %EAX

LINE_LABEL(longjmpnotzero)
    IRET                                                                ;/*  IRET 等于弹出 CS EIP EFLAGS */
    FUNC_END(longjmp)
    
;/*********************************************************************************************************
;  系统重启
;*********************************************************************************************************/

FUNC_DEF(archReboot)
    JMP     bspReboot
    FUNC_END(archReboot)
    
;/*********************************************************************************************************
;  Bogo 循环
;*********************************************************************************************************/

FUNC_DEF(archBogoMipsLoop)
    MOVL    X86_SP_ARG1(%ESP) , %EAX
1:
    DEC     %EAX
    JNS     1b
    RET
    FUNC_END(archBogoMipsLoop)

    FILE_END()
   
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

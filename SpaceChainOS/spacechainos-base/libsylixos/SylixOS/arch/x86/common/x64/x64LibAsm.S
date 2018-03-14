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
;** 文   件   名: x64LibAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2017 年 06 月 05 日
;**
;** 描        述: x86-64 体系构架内部库.
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

    EXPORT_LABEL(archPageCopy)

    WEAK(archIntDisable)
    WEAK(archIntEnable)
    WEAK(archIntEnableForce)

;/*********************************************************************************************************
;  x86-64 关闭总中断
;*********************************************************************************************************/

FUNC_DEF(archIntDisable)
    PUSHFQ
    CLI
    POP     X86_64_RETREG
    RET
    FUNC_END(archIntDisable)
    
FUNC_DEF(archIntEnable)
    TESTL   $X86_EFLAGS_IF , X86_64_ARG0DW
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
;  x86-64 前导零运算
;*********************************************************************************************************/

FUNC_DEF(archFindLsb)
    BSFL    X86_64_ARG0DW , %EAX                                        ;/*  从 0 位开始搜索             */
    JNE     __archFindLsb                                               ;/*  0 意味着没有位被设置        */

    XORL    %EAX , %EAX                                                 ;/*  return 0                    */
    RET

LINE_LABEL(__archFindLsb)                                               ;/*  找到了                      */
    INCL    %EAX                                                        ;/*  +1                          */
    RET
    FUNC_END(archFindLsb)


FUNC_DEF(archFindMsb)
    BSRL    X86_64_ARG0DW , %EAX                                        ;/*  从 31 位开始搜索            */
    JNE     __archFindMsb                                               ;/*  0 意味着没有位被设置        */

    XORL    %EAX , %EAX                                                 ;/*  return 0                    */
    RET

LINE_LABEL(__archFindMsb)                                               ;/*  找到了                      */
    INCL    %EAX                                                        ;/*  +1                          */
    RET
    FUNC_END(archFindMsb)

;/*********************************************************************************************************
;  x86-64 页拷贝
;*********************************************************************************************************/

FUNC_DEF(archPageCopy)
    MOV      $512 /* 4096/8 */, %RCX
    REP
    MOVSQ
    RET
    FUNC_END(archPageCopy)

;/*********************************************************************************************************
;  注意: setjmp 与 longjmp 上下文结构与线程上下文结构不同
;*********************************************************************************************************/

#define _X86_64_RIP_OFFSET      0x00
#define _X86_64_RFLAGS_OFFSET   0x08
#define _X86_64_CS_OFFSET       0x10
#define _X86_64_SS_OFFSET       0x18
#define _X86_64_RSP_OFFSET      0x20
#define _X86_64_RBP_OFFSET      0x28
#define _X86_64_RBX_OFFSET      0x30
#define _X86_64_R12_OFFSET      0x38
#define _X86_64_R13_OFFSET      0x40
#define _X86_64_R14_OFFSET      0x48
#define _X86_64_R15_OFFSET      0x50

;/*********************************************************************************************************
;  保存寄存器宏
;*********************************************************************************************************/

MACRO_DEF(SAVE_NON_VOLATILE_REG)
    MOV     0(%RSP) , %RAX                                              ;/*  栈顶内容是 RIP              */
    MOV     %RAX , _X86_64_RIP_OFFSET(%RDI)                             ;/*  保存 RIP                    */

    PUSHFQ
    POP     _X86_64_RFLAGS_OFFSET(%RDI)                                 ;/*  保存 RFLAGS                 */

    MOV     %RBP , _X86_64_RBP_OFFSET(%RDI)                             ;/*  保存 nonvolatile 寄存器     */

    MOV     %RBX , _X86_64_RBX_OFFSET(%RDI)

    MOV     %R12 , _X86_64_R12_OFFSET(%RDI)
    MOV     %R13 , _X86_64_R13_OFFSET(%RDI)
    MOV     %R14 , _X86_64_R14_OFFSET(%RDI)
    MOV     %R15 , _X86_64_R15_OFFSET(%RDI)

    MOV     %CS  , %RAX
    MOV     %RAX , _X86_64_CS_OFFSET(%RDI)                              ;/*  保存 CS                     */

    MOV     %SS  , %RAX
    MOV     %RAX , _X86_64_SS_OFFSET(%RDI)                              ;/*  保存 SS                     */

    MOV     %RSP , %RAX
    ADD     $8   , %RAX                                                 ;/*  RAX = RIP 出栈后的 RSP      */
    MOV     %RAX , _X86_64_RSP_OFFSET(%RDI)                             ;/*  保存 RSP                    */
    MACRO_END()

;/*********************************************************************************************************
;  恢复寄存器宏
;*********************************************************************************************************/

MACRO_DEF(RESTORE_NON_VOLATILE_REG)
    CLI                                                                 ;/*  关闭中断(后面会恢复 RFLAGS) */

    MOV     X86_64_ARG0 , %RDX
    MOV     X86_64_ARG1 , %RAX

    MOV     _X86_64_RBP_OFFSET(%RDX) , %RBP                             ;/*  恢复 nonvolatile 寄存器     */

    MOV     _X86_64_RBX_OFFSET(%RDX) , %RBX

    MOV     _X86_64_R12_OFFSET(%RDX) , %R12
    MOV     _X86_64_R13_OFFSET(%RDX) , %R13
    MOV     _X86_64_R14_OFFSET(%RDX) , %R14
    MOV     _X86_64_R15_OFFSET(%RDX) , %R15

    AND     $~15 , %RSP                                                 ;/*  RSP 向下 16 字节对齐        */

    PUSH    _X86_64_SS_OFFSET(%RDX)                                     ;/*  压栈保存的 SS               */
    PUSH    _X86_64_RSP_OFFSET(%RDX)                                    ;/*  压栈保存的 RSP              */
    PUSH    _X86_64_RFLAGS_OFFSET(%RDX)                                 ;/*  压栈保存的 RFLAGS           */
    PUSH    _X86_64_CS_OFFSET(%RDX)                                     ;/*  压栈保存的 CS               */
    PUSH    _X86_64_RIP_OFFSET(%RDX)                                    ;/*  压栈保存的 RIP(后面会 IRETQ)*/
    MACRO_END()

;/*********************************************************************************************************
;  sigsetjmp (参数为 jmp_buf, mask_saved)
;*********************************************************************************************************/

FUNC_DEF(sigsetjmp)
    ;/*
    ; * VOID  __sigsetjmpSetup (sigjmp_buf sigjmpEnv, INT iSaveSigs)
    ; */
    PUSH    X86_64_ARG0
    PUSH    X86_64_ARG1
    CALL    __sigsetjmpSetup
    POP     X86_64_ARG1
    POP     X86_64_ARG0

    SAVE_NON_VOLATILE_REG

    MOVL    $0 , %EAX                                                   ;/*  return 0                    */
    RET
    FUNC_END(sigsetjmp)

;/*********************************************************************************************************
;  siglongjmp (参数为 jmp_buf, retval)
;*********************************************************************************************************/

FUNC_DEF(siglongjmp)
    ;/*
    ; * VOID  __siglongjmpSetup (sigjmp_buf sigjmpEnv, INT iVal)
    ; */
    PUSH    X86_64_ARG0
    PUSH    X86_64_ARG1
    CALL    __siglongjmpSetup
    POP     X86_64_ARG1
    POP     X86_64_ARG0

    RESTORE_NON_VOLATILE_REG

    CMPL    $0 , %EAX
    JNE     siglongjmpnotzero
    MOVL    $1 , %EAX

LINE_LABEL(siglongjmpnotzero)
    IRETQ                                                               ;/*  IRETQ 等于弹出 CS RIP RFLAGS*/
    FUNC_END(siglongjmp)

;/*********************************************************************************************************
;  setjmp (参数为 jmp_buf)
;*********************************************************************************************************/

FUNC_DEF(setjmp)
    ;/*
    ; * void __setjmpSetup (jmp_buf jmpbuf)
    ; */
    PUSH    X86_64_ARG0
    CALL    __setjmpSetup
    POP     X86_64_ARG0

    SAVE_NON_VOLATILE_REG

    MOVL    $0 , %EAX                                                   ;/*  return 0                    */
    RET
    FUNC_END(setjmp)

;/*********************************************************************************************************
;  longjmp (参数为 jmp_buf, retval)
;*********************************************************************************************************/

FUNC_DEF(longjmp)
    ;/*
    ; * void __longjmpSetup (jmp_buf jmpbuf, int val)
    ; */
    PUSH    X86_64_ARG0
    PUSH    X86_64_ARG1
    CALL    __longjmpSetup
    POP     X86_64_ARG1
    POP     X86_64_ARG0

    RESTORE_NON_VOLATILE_REG

    CMPL    $0 , %EAX
    JNE     longjmpnotzero
    MOVL    $1 , %EAX

LINE_LABEL(longjmpnotzero)
    IRETQ                                                               ;/*  IRETQ 等于弹出 CS RIP RFLAGS*/
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
1:
    DEC     X86_64_ARG0
    JNS     1b
    RET
    FUNC_END(archBogoMipsLoop)

    FILE_END()
   
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

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
;** 文   件   名: ppcLibAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2015 年 12 月 15 日
;**
;** 描        述: PowerPC 体系构架内部库.
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
;  PowerPC 关闭总中断
;  注意: 这里没有关闭机器检查异常和临界输入异常(E500 才有)
;*********************************************************************************************************/

FUNC_DEF(archIntDisable)
    MFMSR   R3
    ISYNC
    OR      R4 , R3 , R3
    ARCH_PPC_INT_MASK(R4, R4)
    ISYNC
    MTMSR   R4
    ISYNC
    BLR
    FUNC_END()
    
FUNC_DEF(archIntEnable)
    SYNC
    RLWINM  R4 , R3 , 0 , ARCH_PPC_MSR_BIT_EE , ARCH_PPC_MSR_BIT_EE
    CMPWI   CR0, R4 , 0
    BEQ     CR0, __archIntIsDisable
    MFMSR   R3
    ISYNC
    ORI     R3 , R3 , ARCH_PPC_MSR_EE
    ISYNC
    MTMSR   R3
    ISYNC
LINE_LABEL(__archIntIsDisable)
    BLR
    FUNC_END()
    
FUNC_DEF(archIntEnableForce)
    MFMSR   R3
    ISYNC
    ORI     R3 , R3 , ARCH_PPC_MSR_EE
    ISYNC
    MTMSR   R3
    ISYNC
    BLR
    FUNC_END()
    
;/*********************************************************************************************************
;  PowerPC 前导零运算
;*********************************************************************************************************/

FUNC_DEF(archFindLsb)
    NEG     R4 , R3
    AND     R3 , R3 , R4
    CNTLZW  R3 , R3                                                     ;/*  R3 = # leading zeros        */
    SUBFIC  R3 , R3 , 32                                                ;/*  R3 = 32 - R3                */
    BLR
    FUNC_END()

FUNC_DEF(archFindMsb)
    CNTLZW  R3 , R3                                                     ;/*  R3 = # leading zeros        */
    SUBFIC  R3 , R3 , 32                                                ;/*  R3 = 32 - R3                */
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  注意: setjmp 与 longjmp 上下文结构与线程上下文结构不同
;
;  根据文档 <PowerPC-eabi_app.pdf>, 一次调用过程, Nonvolatile(非易失性)的寄存器有如下:
;  CR的CR2-CR4域  R14-R31  R1(SP)  F14-F31
;
;  setjmp 与 longjmp 上下文结构设计如下:
;
;  高地址
;  +---------------+
;  |    CR         | (1 regs)
;  +---------------+
;  |    R14 - R31  | (18 regs)
;  +---------------+
;  |    SP(R1)     | (1 regs)
;  +---------------+
;  |    LR         | (1 regs)
;  +---------------+
;  低地址
;
;  F14-F31 不保存, 共 21 个寄存器
;
;*********************************************************************************************************/

;/*********************************************************************************************************
;  调用设置函数宏
;*********************************************************************************************************/

MACRO_DEF(CALL_SETUP, setup)
    SUBI    SP , SP , 32

    STW     R3 , 28(SP)
    STW     R4 , 24(SP)

    MFLR    R5
    ISYNC
    STW     R5 , 20(SP)
    SYNC

    BL      \setup

    LWZ     R5 , 20(SP)
    SYNC
    MTLR    R5
    ISYNC

    LWZ     R4 , 24(SP)
    LWZ     R3 , 28(SP)

    ADDI    SP , SP , 32
    MACRO_END()

;/*********************************************************************************************************
;  保存寄存器宏
;*********************************************************************************************************/

MACRO_DEF(SAVE_NON_VOLATILE_REG)
    MFLR    R5
    ISYNC
    STW     R5 , 0(R3)
    SYNC

    STW     SP , 4(R3)

    STMW    R14, 8(R3)

    MFCR    R5
    ISYNC
    STW     R5 , 80(R3)
    SYNC
    MACRO_END()

;/*********************************************************************************************************
;  恢复寄存器宏
;*********************************************************************************************************/

MACRO_DEF(RESTORE_NON_VOLATILE_REG)
    LWZ     R5 , 0(R3)
    SYNC
    MTLR    R5
    ISYNC

    LWZ     SP , 4(R3)

    LMW     R14, 8(R3)

    LWZ     R5 , 80(R3)
    SYNC
    MTCR    R5
    ISYNC
    MACRO_END()

;/*********************************************************************************************************
;  sigsetjmp (参数为 jmp_buf, mask_saved)
;*********************************************************************************************************/

FUNC_DEF(sigsetjmp)
    CALL_SETUP  __sigsetjmpSetup

    SAVE_NON_VOLATILE_REG

    XOR     R3 , R3 , R3
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  setjmp (参数为 jmp_buf)
;*********************************************************************************************************/

FUNC_DEF(setjmp)
    CALL_SETUP  __setjmpSetup

    SAVE_NON_VOLATILE_REG

    XOR     R3 , R3 , R3
    BLR
    FUNC_END()
    
;/*********************************************************************************************************
;  siglongjmp (参数为 jmp_buf, retval)
;*********************************************************************************************************/

FUNC_DEF(siglongjmp)
    CALL_SETUP  __siglongjmpSetup

    RESTORE_NON_VOLATILE_REG

    OR      R3 , R4 , R4
    CMPWI   CR0, R4 , 0
    BNE     CR0, siglongjmpnotzero
    LI      R3 , 1
LINE_LABEL(siglongjmpnotzero)
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  longjmp (参数为 jmp_buf, retval)
;*********************************************************************************************************/

FUNC_DEF(longjmp)
    CALL_SETUP  __longjmpSetup

    RESTORE_NON_VOLATILE_REG

    OR      R3 , R4 , R4
    CMPWI   CR0, R4 , 0
    BNE     CR0, longjmpnotzero
    LI      R3 , 1
LINE_LABEL(longjmpnotzero)
    BLR
    FUNC_END()
    
;/*********************************************************************************************************
;  系统重启
;*********************************************************************************************************/

FUNC_DEF(archReboot)
    B       bspReboot
    FUNC_END()
    
;/*********************************************************************************************************
;  Bogo 循环
;*********************************************************************************************************/

FUNC_DEF(archBogoMipsLoop)
1:
    SUBI    R3 , R3 , 1
    CMPWI   CR0, R3 , 0
    BNE     CR0, 1b
    BLR
    FUNC_END()

    FILE_END()
   
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

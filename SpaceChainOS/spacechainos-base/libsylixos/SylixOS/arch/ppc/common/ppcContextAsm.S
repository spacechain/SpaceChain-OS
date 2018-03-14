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
;** 文   件   名: ppcContextAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2015 年 12 月 15 日
;**
;** 描        述: PowerPC 体系构架上下文切换.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/kernel_cfg.h>
#include <config/mp/mp_cfg.h>
#include "ppcContextAsm.h"

    FILE_BEGIN()

    IMPORT_LABEL(_SchedSwp)
#if LW_CFG_SMP_EN > 0
    IMPORT_LABEL(_SchedSafeStack)
#endif
#if LW_CFG_COROUTINE_EN > 0
    IMPORT_LABEL(_SchedCrSwp)
#endif
    
    EXPORT_LABEL(archTaskCtxStart)
    EXPORT_LABEL(archTaskCtxSwitch)
#if LW_CFG_COROUTINE_EN > 0
    EXPORT_LABEL(archCrtCtxSwitch)
#endif
    EXPORT_LABEL(archIntCtxLoad)
    EXPORT_LABEL(archSigCtxLoad)

;/*********************************************************************************************************
;  当前 CPU 开始运行多任务执行
;  参数为当前 CPU 控制块, 即 R3 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxStart)
    LWZ     R4 , 0(R3)                                                  ;/*  获取该任务堆栈栈顶地址      */
    LWZ     SP , 0(R4)                                                  ;/*  堆栈指针赋值                */

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    FUNC_END()

;/*********************************************************************************************************
;  线程切换 
;  参数为当前 CPU 控制块, 即 R3 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxSwitch)
    SAVE_REGS                                                           ;/*  所有寄存器入栈              */

    LWZ     R4 , 0(R3)                                                  ;/*  stack current = SP          */
    STW     SP , 0(R4)

    OR      R14, R3 , R3                                                ;/*  保存当前 CPU 信息           */

#if LW_CFG_SMP_EN > 0
    SUBI    SP , SP , 16                                                ;/*  开辟 C 函数栈帧             */
    BL      _SchedSafeStack                                             ;/*  _SchedSafeStack();          */
    OR      SP , R3 , R3
    OR      R3 , R14, R14
#endif

    SUBI    SP , SP , 16                                                ;/*  开辟 C 函数栈帧             */
    BL      _SchedSwp                                                   ;/*  _SchedSwp();                */
    OR      R3 , R14, R14

    LWZ     R4 , 0(R3)                                                  ;/*  SP = stack current          */
    LWZ     SP , 0(R4)

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    FUNC_END()

;/*********************************************************************************************************
;  协程切换 
;  参数为当前 CPU 控制块, 即 R3 为当前 CPU 控制块指针
;*********************************************************************************************************/

#if LW_CFG_COROUTINE_EN > 0
FUNC_DEF(archCrtCtxSwitch)
    SAVE_REGS                                                           ;/*  所有寄存器入栈              */

    LWZ     R4 , 8(R3)                                                  ;/*  stack current = SP          */
    STW     SP , 0(R4)

    OR      R14, R3 , R3                                                ;/*  保存当前 CPU 信息           */
    SUBI    SP , SP , 16                                                ;/*  开辟 C 函数栈帧             */
    BL      _SchedCrSwp                                                 ;/*  _SchedCrSwp();              */
    OR      R3 , R14, R14

    LWZ     R4 , 8(R3)                                                  ;/*  SP = stack current          */
    LWZ     SP , 0(R4)

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    FUNC_END()
#endif

;/*********************************************************************************************************
;  中断返回时, 线程装载 
;  参数为当前 CPU 控制块, 即 R3 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archIntCtxLoad)
    LWZ     R4 , 0(R3)                                                  ;/*  SP = stack current          */
    LWZ     SP , 0(R4)

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    FUNC_END()

;/*********************************************************************************************************
;  信号上下文返回
;  参数为需要返回的堆栈栈顶位置
;*********************************************************************************************************/

FUNC_DEF(archSigCtxLoad)
    OR      SP , R3 , R3                                                ;/*  返回的堆栈指针              */

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    FUNC_END()

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

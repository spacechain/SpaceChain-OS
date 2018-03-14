/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: x86Fpu.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 07 月 04 日
**
** 描        述: x86 体系架构硬件浮点运算器 (FPU).
*********************************************************************************************************/

#ifndef __ARCH_X86FPU_H
#define __ARCH_X86FPU_H

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

/*********************************************************************************************************
  x86 fpu 操作函数
*********************************************************************************************************/

typedef struct {
    VOIDFUNCPTR     PFPU_pfuncEnable;
    VOIDFUNCPTR     PFPU_pfuncDisable;
    BOOLFUNCPTR     PFPU_pfuncIsEnable;
    VOIDFUNCPTR     PFPU_pfuncSave;
    VOIDFUNCPTR     PFPU_pfuncRestore;
    VOIDFUNCPTR     PFPU_pfuncCtxShow;
    VOIDFUNCPTR     PFPU_pfuncEnableTask;
} X86_FPU_OP;
typedef X86_FPU_OP *PX86_FPU_OP;

/*********************************************************************************************************
  x86 fpu 基本操作
*********************************************************************************************************/

#define X86_FPU_ENABLE(op)              op->PFPU_pfuncEnable()
#define X86_FPU_DISABLE(op)             op->PFPU_pfuncDisable()
#define X86_FPU_ISENABLE(op)            op->PFPU_pfuncIsEnable()
#define X86_FPU_SAVE(op, ctx)           op->PFPU_pfuncSave((ctx))
#define X86_FPU_RESTORE(op, ctx)        op->PFPU_pfuncRestore((ctx))
#define X86_FPU_CTXSHOW(op, fd, ctx)    op->PFPU_pfuncCtxShow((fd), (ctx))
#define X86_FPU_ENABLE_TASK(op, tcb)    op->PFPU_pfuncEnableTask((tcb))

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
#endif                                                                  /*  __ARCH_X86FPU_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/

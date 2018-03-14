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
** 文   件   名: ppcFpu.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 12 月 15 日
**
** 描        述: PowerPC 体系架构硬件浮点运算器 (VFP).
*********************************************************************************************************/

#ifndef __ARCH_PPCFPU_H
#define __ARCH_PPCFPU_H

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

/*********************************************************************************************************
  PowerPC fpu 操作函数
*********************************************************************************************************/

typedef struct {
    VOIDFUNCPTR     PFPU_pfuncEnable;
    VOIDFUNCPTR     PFPU_pfuncDisable;
    BOOLFUNCPTR     PFPU_pfuncIsEnable;
    VOIDFUNCPTR     PFPU_pfuncSave;
    VOIDFUNCPTR     PFPU_pfuncRestore;
    VOIDFUNCPTR     PFPU_pfuncCtxShow;
    VOIDFUNCPTR     PFPU_pfuncEnableTask;
} PPC_FPU_OP;
typedef PPC_FPU_OP *PPPC_FPU_OP;

/*********************************************************************************************************
  PowerPC fpu 基本操作
*********************************************************************************************************/

#define PPC_VFP_ENABLE(op)              op->PFPU_pfuncEnable()
#define PPC_VFP_DISABLE(op)             op->PFPU_pfuncDisable()
#define PPC_VFP_ISENABLE(op)            op->PFPU_pfuncIsEnable()
#define PPC_VFP_SAVE(op, ctx)           op->PFPU_pfuncSave((ctx))
#define PPC_VFP_RESTORE(op, ctx)        op->PFPU_pfuncRestore((ctx))
#define PPC_VFP_CTXSHOW(op, fd, ctx)    op->PFPU_pfuncCtxShow((fd), (ctx))
#define PPC_VFP_ENABLE_TASK(op, tcb)    op->PFPU_pfuncEnableTask((tcb))

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
#endif                                                                  /*  __ARCH_PPCFPU_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/

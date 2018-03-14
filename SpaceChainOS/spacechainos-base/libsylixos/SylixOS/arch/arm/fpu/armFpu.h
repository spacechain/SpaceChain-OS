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
** 文   件   名: armFpu.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 09 日
**
** 描        述: ARM 体系架构硬件浮点运算器 (VFP).
*********************************************************************************************************/

#ifndef __ARMFPU_H
#define __ARMFPU_H

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

/*********************************************************************************************************
  ARM fpu 操作函数
*********************************************************************************************************/

typedef struct {
#if !defined(__SYLIXOS_ARM_ARCH_M__)
    ULONGFUNCPTR    AFPU_pfuncHwSid;
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
    VOIDFUNCPTR     AFPU_pfuncEnable;
    VOIDFUNCPTR     AFPU_pfuncDisable;
    BOOLFUNCPTR     AFPU_pfuncIsEnable;
    VOIDFUNCPTR     AFPU_pfuncSave;
    VOIDFUNCPTR     AFPU_pfuncRestore;
    VOIDFUNCPTR     AFPU_pfuncCtxShow;
} ARM_FPU_OP;
typedef ARM_FPU_OP *PARM_FPU_OP;

/*********************************************************************************************************
  ARM fpu 基本操作
*********************************************************************************************************/

#if !defined(__SYLIXOS_ARM_ARCH_M__)
#define ARM_VFP_HW_SID(op)              op->AFPU_pfuncHwSid()
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
#define ARM_VFP_ENABLE(op)              op->AFPU_pfuncEnable()
#define ARM_VFP_DISABLE(op)             op->AFPU_pfuncDisable()
#define ARM_VFP_ISENABLE(op)            op->AFPU_pfuncIsEnable()
#define ARM_VFP_SAVE(op, ctx)           op->AFPU_pfuncSave((ctx))
#define ARM_VFP_RESTORE(op, ctx)        op->AFPU_pfuncRestore((ctx))
#define ARM_VFP_CTXSHOW(op, fd, ctx)    op->AFPU_pfuncCtxShow((fd), (ctx))

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
#endif                                                                  /*  __ARMFPU_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

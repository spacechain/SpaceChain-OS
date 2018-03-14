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
** 文   件   名: ppcVfp.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 12 月 21 日
**
** 描        述: PowerPC 体系架构 VPU 支持.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "../ppcFpu.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static PPC_FPU_OP   _G_fpuopVfp;
/*********************************************************************************************************
  实现函数
*********************************************************************************************************/
extern VOID     ppcVfpEnable(VOID);
extern VOID     ppcVfpDisable(VOID);
extern BOOL     ppcVfpIsEnable(VOID);
extern VOID     ppcVfpSave(PVOID  pvFpuCtx);
extern VOID     ppcVfpRestore(PVOID  pvFpuCtx);
/*********************************************************************************************************
** 函数名称: ppcVfpCtxShow
** 功能描述: 显示 VFP 上下文
** 输　入  : iFd       输出文件描述符
**           pvFpuCtx  VFP 上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcVfpCtxShow (INT  iFd, PVOID  pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    LW_FPU_CONTEXT *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX   *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;
    INT             i;

    fdprintf(iFd, "FPSCR   = 0x%08x\n", pcpufpuCtx->FPUCTX_uiFpscr);

    for (i = 0; i < FP_DREG_NR; i += 2) {
        fdprintf(iFd, "FPR%02d = %lf, FPR%02d = %lf\n",
                 i,     pcpufpuCtx->FPUCTX_dfDreg[i],
                 i + 1, pcpufpuCtx->FPUCTX_dfDreg[i + 1]);
    }
#endif
}
/*********************************************************************************************************
** 函数名称: ppcVfpEnableTask
** 功能描述: 系统发生 undef 异常时, 使能任务的 VFP
** 输　入  : ptcbCur    当前任务控制块
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcVfpEnableTask (PLW_CLASS_TCB  ptcbCur)
{
    ARCH_REG_CTX  *pregctx;
    ARCH_FPU_CTX  *pfpuctx;
    ARCH_REG_T     regSp;

    pregctx = archTaskRegsGet(ptcbCur->TCB_pstkStackNow, &regSp);
    pregctx->REG_uiSrr1 |= ARCH_PPC_MSR_FP;

    pfpuctx = &ptcbCur->TCB_fpuctxContext.FPUCTX_fpuctxContext;
    pfpuctx->FPUCTX_uiFpscr     = 0ul;
    pfpuctx->FPUCTX_uiFpscrCopy = 0ul;
}
/*********************************************************************************************************
** 函数名称: ppcVfpPrimaryInit
** 功能描述: 初始化并获取 VFP 控制器操作函数集
** 输　入  : pcMachineName 机器名
**           pcFpuName     浮点运算器名
** 输　出  : 操作函数集
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PPPC_FPU_OP  ppcVfpPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    _G_fpuopVfp.PFPU_pfuncSave       = ppcVfpSave;
    _G_fpuopVfp.PFPU_pfuncRestore    = ppcVfpRestore;
    _G_fpuopVfp.PFPU_pfuncEnable     = ppcVfpEnable;
    _G_fpuopVfp.PFPU_pfuncDisable    = ppcVfpDisable;
    _G_fpuopVfp.PFPU_pfuncIsEnable   = ppcVfpIsEnable;
    _G_fpuopVfp.PFPU_pfuncCtxShow    = ppcVfpCtxShow;
    _G_fpuopVfp.PFPU_pfuncEnableTask = ppcVfpEnableTask;

    return  (&_G_fpuopVfp);
}
/*********************************************************************************************************
** 函数名称: ppcVfpSecondaryInit
** 功能描述: 初始化 VFP 控制器
** 输　入  : pcMachineName 机器名
**           pcFpuName     浮点运算器名
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  ppcVfpSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

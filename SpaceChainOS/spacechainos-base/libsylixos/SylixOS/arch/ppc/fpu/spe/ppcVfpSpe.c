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
** 文   件   名: ppcVfpSpe.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 05 月 02 日
**
** 描        述: PowerPC 体系架构 SPE 支持.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "../ppcFpu.h"
#include "arch/ppc/arch_e500.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static PPC_FPU_OP   _G_fpuopVfpSpe;
/*********************************************************************************************************
  实现函数
*********************************************************************************************************/
extern VOID     ppcVfpSpeEnable(VOID);
extern VOID     ppcVfpSpeDisable(VOID);
extern BOOL     ppcVfpSpeIsEnable(VOID);
extern VOID     ppcVfpSpeSave(PVOID  pvFpuCtx);
extern VOID     ppcVfpSpeRestore(PVOID  pvFpuCtx);
/*********************************************************************************************************
** 函数名称: ppcVfpSpeCtxShow
** 功能描述: 显示 VFP 上下文
** 输　入  : iFd       输出文件描述符
**           pvFpuCtx  VFP 上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  ppcVfpSpeCtxShow (INT  iFd, PVOID  pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    LW_FPU_CONTEXT *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX   *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;
    INT             i;

    fdprintf(iFd, "Upper 32 bits of GPR\n");

    for (i = 0; i < SPE_GPR_NR; i += 4) {
        fdprintf(iFd, "R%02d 0x%08x R%02d 0x%08x R%02d 0x%08x R%02d 0x%08x\n",
                 i,     pcpufpuCtx->SPECTX_uiGpr[i],
                 i + 1, pcpufpuCtx->SPECTX_uiGpr[i + 1],
                 i + 2, pcpufpuCtx->SPECTX_uiGpr[i + 2],
                 i + 3, pcpufpuCtx->SPECTX_uiGpr[i + 3]);
    }

    fdprintf(iFd, "\nAccL = 0x%08x, AccH = 0x%08x\n",
             pcpufpuCtx->SPECTX_uiAcc[0],
             pcpufpuCtx->SPECTX_uiAcc[1]);
#endif
}
/*********************************************************************************************************
** 函数名称: ppcVfpSpeEnableTask
** 功能描述: 系统发生 undef 异常时, 使能任务的 VFP
** 输　入  : ptcbCur    当前任务控制块
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcVfpSpeEnableTask (PLW_CLASS_TCB  ptcbCur)
{
    ARCH_REG_CTX  *pregctx;
    ARCH_FPU_CTX  *pfpuctx;
    ARCH_REG_T     regSp;

    pregctx = archTaskRegsGet(ptcbCur->TCB_pstkStackNow, &regSp);
    pregctx->REG_uiSrr1 |= ARCH_PPC_MSR_SPE;

    pfpuctx = &ptcbCur->TCB_fpuctxContext.FPUCTX_fpuctxContext;
    pfpuctx->SPECTX_uiSpefscr = 0ul;
}
/*********************************************************************************************************
** 函数名称: ppcVfpSpePrimaryInit
** 功能描述: 获取 VFP 控制器操作函数集
** 输　入  : pcMachineName 机器名
** 输　出  : 操作函数集
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PPPC_FPU_OP  ppcVfpSpePrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    _G_fpuopVfpSpe.PFPU_pfuncEnable     = ppcVfpSpeEnable;
    _G_fpuopVfpSpe.PFPU_pfuncDisable    = ppcVfpSpeDisable;
    _G_fpuopVfpSpe.PFPU_pfuncIsEnable   = ppcVfpSpeIsEnable;
    _G_fpuopVfpSpe.PFPU_pfuncSave       = ppcVfpSpeSave;
    _G_fpuopVfpSpe.PFPU_pfuncRestore    = ppcVfpSpeRestore;
    _G_fpuopVfpSpe.PFPU_pfuncCtxShow    = ppcVfpSpeCtxShow;
    _G_fpuopVfpSpe.PFPU_pfuncEnableTask = ppcVfpSpeEnableTask;

    return  (&_G_fpuopVfpSpe);
}
/*********************************************************************************************************
** 函数名称: ppcVfpSpeSecondaryInit
** 功能描述: 初始化 VFP 控制器
** 输　入  : pcMachineName 机器名
**           pcFpuName     浮点运算器名
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  ppcVfpSpeSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

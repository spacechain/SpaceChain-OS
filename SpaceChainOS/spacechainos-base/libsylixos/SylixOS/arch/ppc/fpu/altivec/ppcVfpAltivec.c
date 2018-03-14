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
** 文   件   名: ppcVfpAltivec.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 05 月 04 日
**
** 描        述: PowerPC 体系架构 ALTIVEC 支持.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "../ppcFpu.h"
#define  __SYLIXOS_PPC_HAVE_ALTIVEC 1
#include "arch/ppc/arch_604.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static PPC_FPU_OP   _G_fpuopVfpAltivec;
/*********************************************************************************************************
  实现函数
*********************************************************************************************************/
extern VOID     ppcVfpAltivecEnable(VOID);
extern VOID     ppcVfpAltivecDisable(VOID);
extern BOOL     ppcVfpAltivecIsEnable(VOID);
extern VOID     ppcVfpAltivecSave(PVOID  pvFpuCtx);
extern VOID     ppcVfpAltivecRestore(PVOID  pvFpuCtx);
/*********************************************************************************************************
  VSCR 相关定义
*********************************************************************************************************/
#define ALTIVEC_VSCR_CONFIG_WORD    3                                   /*  VSCR word with below bits   */
#define ALTIVEC_VSCR_NJ             0x00010000                          /*  Non-Java mode               */
#define ALTIVEC_VSCR_SAT            0x00000001                          /*  Vector Saturation           */
/*********************************************************************************************************
** 函数名称: ppcVfpAltivecCtxShow
** 功能描述: 显示 VFP 上下文
** 输　入  : iFd       输出文件描述符
**           pvFpuCtx  VFP 上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcVfpAltivecCtxShow (INT  iFd, PVOID  pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    LW_FPU_CONTEXT *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX   *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;
    UINT32         *puiValue;
    INT             i;

    fdprintf(iFd, "VRSAVE = 0x%08x VSCR = 0x%08x\n",
             pcpufpuCtx->VECCTX_uiVrsave,
             pcpufpuCtx->VECCTX_uiVscr[ALTIVEC_VSCR_CONFIG_WORD]);

    for (i = 0; i < ALTIVEC_REG_NR; i++) {
        puiValue = (UINT32 *)&pcpufpuCtx->VECCTX_regs[i];
        fdprintf(iFd, "AR%02d: 0x%08x_0x%08x_0x%08x_0x%08x\n",
                 i,
                 puiValue[0], puiValue[1], puiValue[2], puiValue[3]);
    }
#endif
}
/*********************************************************************************************************
** 函数名称: ppcVfpAltivecEnableTask
** 功能描述: 系统发生 undef 异常时, 使能任务的 VFP
** 输　入  : ptcbCur    当前任务控制块
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcVfpAltivecEnableTask (PLW_CLASS_TCB  ptcbCur)
{
    ARCH_REG_CTX  *pregctx;
    ARCH_FPU_CTX  *pfpuctx;
    ARCH_REG_T     regSp;

    pregctx = archTaskRegsGet(ptcbCur->TCB_pstkStackNow, &regSp);
    pregctx->REG_uiSrr1 |= ARCH_PPC_MSR_VEC;

    pfpuctx = &ptcbCur->TCB_fpuctxContext.FPUCTX_fpuctxContext;
    pfpuctx->VECCTX_uiVrsave = 0ul;
    pfpuctx->VECCTX_uiVscr[ALTIVEC_VSCR_CONFIG_WORD] = 0ul;
}
/*********************************************************************************************************
** 函数名称: ppcVfpAltivecPrimaryInit
** 功能描述: 获取 VFP 控制器操作函数集
** 输　入  : pcMachineName 机器名
** 输　出  : 操作函数集
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PPPC_FPU_OP  ppcVfpAltivecPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    _G_fpuopVfpAltivec.PFPU_pfuncEnable     = ppcVfpAltivecEnable;
    _G_fpuopVfpAltivec.PFPU_pfuncDisable    = ppcVfpAltivecDisable;
    _G_fpuopVfpAltivec.PFPU_pfuncIsEnable   = ppcVfpAltivecIsEnable;
    _G_fpuopVfpAltivec.PFPU_pfuncSave       = ppcVfpAltivecSave;
    _G_fpuopVfpAltivec.PFPU_pfuncRestore    = ppcVfpAltivecRestore;
    _G_fpuopVfpAltivec.PFPU_pfuncCtxShow    = ppcVfpAltivecCtxShow;
    _G_fpuopVfpAltivec.PFPU_pfuncEnableTask = ppcVfpAltivecEnableTask;

    return  (&_G_fpuopVfpAltivec);
}
/*********************************************************************************************************
** 函数名称: ppcVfpAltivecSecondaryInit
** 功能描述: 初始化 VFP 控制器
** 输　入  : pcMachineName 机器名
**           pcFpuName     浮点运算器名
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  ppcVfpAltivecSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

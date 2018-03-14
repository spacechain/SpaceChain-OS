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
** 文   件   名: armVfpV7M.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 11 月 08 日
**
** 描        述: ARM 体系架构 Cortex-Mx VFPv4/5 支持.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv7M 体系构架
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__)
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "../armFpu.h"
/*********************************************************************************************************
  Coprocessor Access Control Register: enable CP10 & CP11 for VFP
*********************************************************************************************************/
#define ARMV7M_CPACR            0xe000ed88
#define ARMV7M_CPACR_CP_FA(x)   (3 << (2 * (x)))                /* 0b11 : Full access for this cp       */
/*********************************************************************************************************
  Floating-Point Context Control Register, FPCCR
*********************************************************************************************************/
#define ARMV7M_FPCCR            0xe000ef34
#define ARMV7M_FPCCR_ASPEN      (1 << 31)                       /* enable auto FP context stacking      */
#define ARMV7M_FPCCR_LSPEN      (1 << 30)                       /* enable lazy FP context stacking      */
#define ARMV7M_FPCCR_MONRDY     (1 << 8)
#define ARMV7M_FPCCR_BFRDY      (1 << 6)
#define ARMV7M_FPCCR_MMRDY      (1 << 5)
#define ARMV7M_FPCCR_HFRDY      (1 << 4)
#define ARMV7M_FPCCR_THREAD     (1 << 3)                        /* allocated FP stack in thread         */
#define ARMV7M_FPCCR_USER       (1 << 1)                        /* allocated FP stack in priviledged    */
#define ARMV7M_FPCCR_LSPACT     (1 << 0)                        /* lazy stacking is active              */
/*********************************************************************************************************
  Floating-Point Context Address Register, FPCAR
  this register contains the FP context address in the exception stack
*********************************************************************************************************/
#define ARMV7M_FPCAR            (0xe000ef38)
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static ARM_FPU_OP   _G_fpuopVfpV7M;
/*********************************************************************************************************
  汇编实现函数
*********************************************************************************************************/
extern VOID    armVfpV7MSave(PVOID pvFpuCtx);
extern VOID    armVfpV7MRestore(PVOID pvFpuCtx);
/*********************************************************************************************************
** 函数名称: armVfpV7MEnable
** 功能描述: 使能 VFP
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armVfpV7MEnable (VOID)
{
    volatile UINT32 *uiCpacr = (volatile UINT32 *)ARMV7M_CPACR;

    *uiCpacr |= ARMV7M_CPACR_CP_FA(10) | ARMV7M_CPACR_CP_FA(11);
}
/*********************************************************************************************************
** 函数名称: armVfpV7MDisable
** 功能描述: 禁能 VFP
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  armVfpV7MDisable (VOID)
{
    volatile UINT32 *uiCpacr = (volatile UINT32 *)ARMV7M_CPACR;

    *uiCpacr &= ~(ARMV7M_CPACR_CP_FA(10) | ARMV7M_CPACR_CP_FA(11));
}
/*********************************************************************************************************
** 函数名称: armVfpV7MIsEnable
** 功能描述: 是否使能 VFP
** 输　入  : NONE
** 输　出  : 是否使能
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  armVfpV7MIsEnable (VOID)
{
    UINT32  uiVal = *(volatile UINT32 *)ARMV7M_CPACR;

    return  (uiVal & ((ARMV7M_CPACR_CP_FA(10) | ARMV7M_CPACR_CP_FA(11)))) ? (LW_TRUE) : (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: armVfpV7MCtxShow
** 功能描述: 显示 VFP 上下文
** 输　入  : iFd       输出文件描述符
**           pvFpuCtx  VFP 上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  armVfpV7MCtxShow (INT iFd, PVOID pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    INT i;

    LW_FPU_CONTEXT  *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX    *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;

    fdprintf(iFd, "FPSCR   = 0x%08x\n", pcpufpuCtx->FPUCTX_uiFpscr);

    for (i = 0; i < 16; i += 2) {
        fdprintf(iFd, "FPS[%02d] = 0x%08x  ", i,     pcpufpuCtx->FPUCTX_uiDreg[i]);
        fdprintf(iFd, "FPS[%02d] = 0x%08x\n", i + 1, pcpufpuCtx->FPUCTX_uiDreg[i + 1]);
    }
#endif
}
/*********************************************************************************************************
** 函数名称: armVfpV7MPrimaryInit
** 功能描述: 初始化并获取 VFP 控制器操作函数集
** 输　入  : pcMachineName 机器名
**           pcFpuName     浮点运算器名
** 输　出  : 操作函数集
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PARM_FPU_OP  armVfpV7MPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
             UINT32  uiVal;
    volatile UINT32 *uiCpacr = (volatile UINT32 *)ARMV7M_CPACR;
    volatile UINT32 *uiFpccr = (volatile UINT32 *)ARMV7M_FPCCR;

    uiVal = *uiCpacr;
    if (uiVal & ((ARMV7M_CPACR_CP_FA(10) | ARMV7M_CPACR_CP_FA(11)))) {
        uiVal &= ~(ARMV7M_CPACR_CP_FA(10) | ARMV7M_CPACR_CP_FA(11));
        *uiCpacr = uiVal;                                       /* disable                              */
    }

    *uiFpccr &= ~(ARMV7M_FPCCR_ASPEN | ARMV7M_FPCCR_LSPEN);     /* disable automaically FP ctx saving.  */

    _G_fpuopVfpV7M.AFPU_pfuncEnable   = armVfpV7MEnable;
    _G_fpuopVfpV7M.AFPU_pfuncDisable  = armVfpV7MDisable;
    _G_fpuopVfpV7M.AFPU_pfuncIsEnable = armVfpV7MIsEnable;
    _G_fpuopVfpV7M.AFPU_pfuncSave     = armVfpV7MSave;
    _G_fpuopVfpV7M.AFPU_pfuncRestore  = armVfpV7MRestore;
    _G_fpuopVfpV7M.AFPU_pfuncCtxShow  = armVfpV7MCtxShow;

    return  (&_G_fpuopVfpV7M);
}
/*********************************************************************************************************
** 函数名称: armVfpV7MSecondaryInit
** 功能描述: 初始化 VFP 控制器
** 输　入  : pcMachineName 机器名
**           pcFpuName     浮点运算器名
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armVfpV7MSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
#endif                                                                  /*  __SYLIXOS_ARM_ARCH_M__      */
/*********************************************************************************************************
  END
*********************************************************************************************************/

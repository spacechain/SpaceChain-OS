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
** 文   件   名: mipsVfp32.c
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 11 月 17 日
**
** 描        述: MIPS 体系架构 VPU32 支持.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "../mipsFpu.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static MIPS_FPU_OP  _G_fpuopVfp32;
/*********************************************************************************************************
  实现函数
*********************************************************************************************************/
extern VOID    mipsVfp32Init(VOID);
extern UINT32  mipsVfp32GetFIR(VOID);
extern VOID    mipsVfp32Enable(VOID);
extern VOID    mipsVfp32Disable(VOID);
extern BOOL    mipsVfp32IsEnable(VOID);
extern VOID    mipsVfp32Save(PVOID  pvFpuCtx);
extern VOID    mipsVfp32Restore(PVOID  pvFpuCtx);
/*********************************************************************************************************
** 函数名称: mipsVfp32CtxShow
** 功能描述: 显示 VFP 上下文
** 输　入  : iFd       输出文件描述符
**           pvFpuCtx  VFP 上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  mipsVfp32CtxShow (INT  iFd, PVOID  pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    LW_FPU_CONTEXT  *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX    *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;
    INT              i;

    fdprintf(iFd, "FCSR = 0x%08x\n", pcpufpuCtx->FPUCTX_uiFpcsr);

    for (i = 0; i < FP_NUM_DREGS; i++) {
        fdprintf(iFd, "FP%02d = 0x%08x%08x\n", i,
                 pcpufpuCtx->FPUCTX_reg[i].val32[0],
                 pcpufpuCtx->FPUCTX_reg[i].val32[1]);
    }
#endif
}
/*********************************************************************************************************
** 函数名称: mipsVfp32PrimaryInit
** 功能描述: 初始化并获取 VFP 控制器操作函数集
** 输　入  : pcMachineName 机器名
**           pcFpuName     浮点运算器名
** 输　出  : 操作函数集
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PMIPS_FPU_OP  mipsVfp32PrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;

    mipsVfp32Init();

    _G_fpuopVfp32.MFPU_pfuncEnable   = mipsVfp32Enable;
    _G_fpuopVfp32.MFPU_pfuncDisable  = mipsVfp32Disable;
    _G_fpuopVfp32.MFPU_pfuncIsEnable = mipsVfp32IsEnable;
    _G_fpuopVfp32.MFPU_pfuncCtxShow  = mipsVfp32CtxShow;
    _G_fpuopVfp32.MFPU_pfuncSave     = mipsVfp32Save;
    _G_fpuopVfp32.MFPU_pfuncRestore  = mipsVfp32Restore;

    return  (&_G_fpuopVfp32);
}
/*********************************************************************************************************
** 函数名称: mipsVfp32SecondaryInit
** 功能描述: 初始化 VFP 控制器
** 输　入  : pcMachineName 机器名
**           pcFpuName     浮点运算器名
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  mipsVfp32SecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;

    mipsVfp32Init();
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

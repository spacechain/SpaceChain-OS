/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: mipsContext.c
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 09 月 01 日
**
** 描        述: MIPS 体系构架上下文处理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/mips/common/cp0/mipsCp0.h"
/*********************************************************************************************************
  外部函数声明
*********************************************************************************************************/
extern ARCH_REG_T  archGetGP(VOID);
/*********************************************************************************************************
** 函数名称: archTaskCtxCreate
** 功能描述: 创建任务上下文
** 输　入  : pfuncTask      任务入口
**           pvArg          入口参数
**           pstkTop        初始化堆栈起点
**           ulOpt          任务创建选项
** 输　出  : 初始化堆栈结束点
** 全局变量:
** 调用模块:
** 注  意  : 堆栈从高地址向低地址增长.
*********************************************************************************************************/
PLW_STACK  archTaskCtxCreate (PTHREAD_START_ROUTINE  pfuncTask,
                              PVOID                  pvArg,
                              PLW_STACK              pstkTop,
                              ULONG                  ulOpt)
{
    ARCH_REG_CTX       *pregctx;
    ARCH_FP_CTX        *pfpctx;
    UINT32              uiCP0Status;
    INT                 i;

    uiCP0Status  = mipsCp0StatusRead();                                 /*  获得当前的 CP0 STATUS 寄存器*/
    uiCP0Status |= bspIntInitEnableStatus() | M_StatusIE;               /*  使能中断                    */
    uiCP0Status &= ~M_StatusCU1;                                        /*  禁能 FPU                    */

    if ((addr_t)pstkTop & 0x7) {                                        /*  保证出栈后 CPU SP 8 字节对齐*/
        pstkTop = (PLW_STACK)((addr_t)pstkTop - 4);                     /*  向低地址推进 4 字节         */
    }

    pfpctx  = (ARCH_FP_CTX  *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX));
    pregctx = (ARCH_REG_CTX *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX) - sizeof(ARCH_REG_CTX));

    /*
     * 初始化栈帧
     */
    for (i = 0; i < MIPS_ARG_REG_NR; i++) {
        pfpctx->FP_uiArg[i] = i;
    }

    /*
     * 初始化寄存器上下文
     */
    for (i = 0; i < MIPS_REG_NR; i++) {
        pregctx->REG_uiReg[i] = i;
    }

    pregctx->REG_uiReg[REG_A0] = (ARCH_REG_T)pvArg;
    pregctx->REG_uiReg[REG_GP] = (ARCH_REG_T)archGetGP();               /*  获得 GP 寄存器的值          */
    pregctx->REG_uiReg[REG_FP] = (ARCH_REG_T)pfpctx;
    pregctx->REG_uiReg[REG_RA] = (ARCH_REG_T)0x0;

    pregctx->REG_uiCP0Status   = (ARCH_REG_T)uiCP0Status;
    pregctx->REG_uiCP0EPC      = (ARCH_REG_T)pfuncTask;
    pregctx->REG_uiCP0Cause    = (ARCH_REG_T)0x0;
    pregctx->REG_uiCP0DataLO   = (ARCH_REG_T)0x0;
    pregctx->REG_uiCP0DataHI   = (ARCH_REG_T)0x0;
    pregctx->REG_uiCP0BadVAddr = (ARCH_REG_T)0x0;

    return  ((PLW_STACK)pregctx);
}
/*********************************************************************************************************
** 函数名称: archTaskCtxSetFp
** 功能描述: 设置任务上下文栈帧 (用于 backtrace 回溯, 详情请见 backtrace 相关文件)
** 输　入  : pstkDest  目的 stack frame
**           pstkSrc   源端 stack frame
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTaskCtxSetFp (PLW_STACK  pstkDest, PLW_STACK  pstkSrc)
{
    ARCH_REG_CTX      *pregctxDest = (ARCH_REG_CTX *)pstkDest;
    ARCH_REG_CTX      *pregctxSrc  = (ARCH_REG_CTX *)pstkSrc;

    pregctxDest->REG_uiReg[REG_FP] = (ARCH_REG_T)pstkSrc + sizeof(ARCH_REG_CTX);
    pregctxDest->REG_uiReg[REG_RA] = (ARCH_REG_T)pregctxSrc->REG_uiCP0EPC;
}
/*********************************************************************************************************
** 函数名称: archTaskRegsGet
** 功能描述: 通过栈顶指针获取寄存器表 (满栈结构)
** 输　入  : pstkTop        堆栈顶点
**           pregSp         SP 指针
** 输　出  : 寄存器结构
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ARCH_REG_CTX  *archTaskRegsGet (PLW_STACK  pstkTop, ARCH_REG_T *pregSp)
{
    ARCH_REG_T  regSp = (ARCH_REG_T)pstkTop;

#if CPU_STK_GROWTH == 0
    regSp -= sizeof(ARCH_REG_CTX);
#else
    regSp += sizeof(ARCH_REG_CTX);
#endif

    *pregSp = regSp;

    return  ((ARCH_REG_CTX *)pstkTop);
}
/*********************************************************************************************************
** 函数名称: archTaskRegsSet
** 功能描述: 通过栈顶指针获取寄存器表 (满栈结构)
** 输　入  : pstkTop        堆栈顶点
**           pregctx        寄存器表
** 输　出  : 寄存器结构
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTaskRegsSet (PLW_STACK  pstkTop, const ARCH_REG_CTX  *pregctx)
{
    *(ARCH_REG_CTX *)pstkTop = *pregctx;
}
/*********************************************************************************************************
** 函数名称: archTaskCtxShow
** 功能描述: 打印任务上下文
** 输　入  : iFd        文件描述符
             pstkTop    堆栈栈顶
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

VOID  archTaskCtxShow (INT  iFd, PLW_STACK  pstkTop)
{
    UINT32              uiCP0Status;
    ARCH_REG_CTX       *pregctx = (ARCH_REG_CTX *)pstkTop;

    if (iFd >= 0) {
        fdprintf(iFd, "\n");

        fdprintf(iFd, "SP        = 0x%08x\n", (ARCH_REG_T)pstkTop);
        fdprintf(iFd, "EPC       = 0x%08x\n", (ARCH_REG_T)pregctx->REG_uiCP0EPC);
        fdprintf(iFd, "BADVADDR  = 0x%08x\n", (ARCH_REG_T)pregctx->REG_uiCP0BadVAddr);
        fdprintf(iFd, "CAUSE     = 0x%08x\n", (ARCH_REG_T)pregctx->REG_uiCP0Cause);
        fdprintf(iFd, "LO        = 0x%08x\n", (ARCH_REG_T)pregctx->REG_uiCP0DataLO);
        fdprintf(iFd, "HI        = 0x%08x\n", (ARCH_REG_T)pregctx->REG_uiCP0DataHI);

        fdprintf(iFd, "$00(ZERO) = 0x%08x\n", pregctx->REG_uiReg[REG_ZERO]);
        fdprintf(iFd, "$01(AT)   = 0x%08x\n", pregctx->REG_uiReg[REG_AT]);
        fdprintf(iFd, "$02(V0)   = 0x%08x\n", pregctx->REG_uiReg[REG_V0]);
        fdprintf(iFd, "$03(V1)   = 0x%08x\n", pregctx->REG_uiReg[REG_V1]);
        fdprintf(iFd, "$04(A0)   = 0x%08x\n", pregctx->REG_uiReg[REG_A0]);
        fdprintf(iFd, "$05(A1)   = 0x%08x\n", pregctx->REG_uiReg[REG_A1]);
        fdprintf(iFd, "$06(A2)   = 0x%08x\n", pregctx->REG_uiReg[REG_A2]);
        fdprintf(iFd, "$07(A3)   = 0x%08x\n", pregctx->REG_uiReg[REG_A3]);
        fdprintf(iFd, "$08(T0)   = 0x%08x\n", pregctx->REG_uiReg[REG_T0]);
        fdprintf(iFd, "$09(T1)   = 0x%08x\n", pregctx->REG_uiReg[REG_T1]);
        fdprintf(iFd, "$10(T2)   = 0x%08x\n", pregctx->REG_uiReg[REG_T2]);
        fdprintf(iFd, "$11(T3)   = 0x%08x\n", pregctx->REG_uiReg[REG_T3]);
        fdprintf(iFd, "$12(T4)   = 0x%08x\n", pregctx->REG_uiReg[REG_T4]);
        fdprintf(iFd, "$13(T5)   = 0x%08x\n", pregctx->REG_uiReg[REG_T5]);
        fdprintf(iFd, "$14(T6)   = 0x%08x\n", pregctx->REG_uiReg[REG_T6]);
        fdprintf(iFd, "$15(T7)   = 0x%08x\n", pregctx->REG_uiReg[REG_T7]);
        fdprintf(iFd, "$16(S0)   = 0x%08x\n", pregctx->REG_uiReg[REG_S0]);
        fdprintf(iFd, "$17(S1)   = 0x%08x\n", pregctx->REG_uiReg[REG_S1]);
        fdprintf(iFd, "$18(S2)   = 0x%08x\n", pregctx->REG_uiReg[REG_S2]);
        fdprintf(iFd, "$19(S3)   = 0x%08x\n", pregctx->REG_uiReg[REG_S3]);
        fdprintf(iFd, "$20(S4)   = 0x%08x\n", pregctx->REG_uiReg[REG_S4]);
        fdprintf(iFd, "$21(S5)   = 0x%08x\n", pregctx->REG_uiReg[REG_S5]);
        fdprintf(iFd, "$22(S6)   = 0x%08x\n", pregctx->REG_uiReg[REG_S6]);
        fdprintf(iFd, "$23(S7)   = 0x%08x\n", pregctx->REG_uiReg[REG_S7]);
        fdprintf(iFd, "$24(T8)   = 0x%08x\n", pregctx->REG_uiReg[REG_T8]);
        fdprintf(iFd, "$25(T9)   = 0x%08x\n", pregctx->REG_uiReg[REG_T9]);
        fdprintf(iFd, "$28(GP)   = 0x%08x\n", pregctx->REG_uiReg[REG_GP]);
        fdprintf(iFd, "$29(SP)   = 0x%08x\n", pregctx->REG_uiReg[REG_SP]);
        fdprintf(iFd, "$30(FP)   = 0x%08x\n", pregctx->REG_uiReg[REG_FP]);
        fdprintf(iFd, "$31(RA)   = 0x%08x\n", pregctx->REG_uiReg[REG_RA]);

        uiCP0Status = pregctx->REG_uiCP0Status;

        fdprintf(iFd, "CP0 Status Register:\n");
        fdprintf(iFd, "CU3 = %d  ", (uiCP0Status & M_StatusCU3) >> S_StatusCU3);
        fdprintf(iFd, "CU2 = %d\n", (uiCP0Status & M_StatusCU2) >> S_StatusCU2);
        fdprintf(iFd, "CU1 = %d  ", (uiCP0Status & M_StatusCU1) >> S_StatusCU1);
        fdprintf(iFd, "CU0 = %d\n", (uiCP0Status & M_StatusCU0) >> S_StatusCU0);
        fdprintf(iFd, "RP  = %d  ", (uiCP0Status & M_StatusRP)  >> S_StatusRP);
        fdprintf(iFd, "FR  = %d\n", (uiCP0Status & M_StatusFR)  >> S_StatusFR);
        fdprintf(iFd, "RE  = %d  ", (uiCP0Status & M_StatusRE)  >> S_StatusRE);
        fdprintf(iFd, "MX  = %d\n", (uiCP0Status & M_StatusMX)  >> S_StatusMX);
        fdprintf(iFd, "PX  = %d  ", (uiCP0Status & M_StatusPX)  >> S_StatusPX);
        fdprintf(iFd, "BEV = %d\n", (uiCP0Status & M_StatusBEV) >> S_StatusBEV);
        fdprintf(iFd, "TS  = %d  ", (uiCP0Status & M_StatusTS)  >> S_StatusTS);
        fdprintf(iFd, "SR  = %d\n", (uiCP0Status & M_StatusSR)  >> S_StatusSR);
        fdprintf(iFd, "NMI = %d  ", (uiCP0Status & M_StatusNMI) >> S_StatusNMI);
        fdprintf(iFd, "IM7 = %d\n", (uiCP0Status & M_StatusIM7) >> S_StatusIM7);
        fdprintf(iFd, "IM6 = %d  ", (uiCP0Status & M_StatusIM6) >> S_StatusIM6);
        fdprintf(iFd, "IM5 = %d\n", (uiCP0Status & M_StatusIM5) >> S_StatusIM5);
        fdprintf(iFd, "IM4 = %d  ", (uiCP0Status & M_StatusIM4) >> S_StatusIM4);
        fdprintf(iFd, "IM3 = %d\n", (uiCP0Status & M_StatusIM3) >> S_StatusIM3);
        fdprintf(iFd, "IM2 = %d  ", (uiCP0Status & M_StatusIM2) >> S_StatusIM2);
        fdprintf(iFd, "IM1 = %d\n", (uiCP0Status & M_StatusIM1) >> S_StatusIM1);
        fdprintf(iFd, "IM0 = %d  ", (uiCP0Status & M_StatusIM0) >> S_StatusIM0);
        fdprintf(iFd, "KX  = %d\n", (uiCP0Status & M_StatusKX)  >> S_StatusKX);
        fdprintf(iFd, "SX  = %d  ", (uiCP0Status & M_StatusSX)  >> S_StatusSX);
        fdprintf(iFd, "UX  = %d\n", (uiCP0Status & M_StatusUX)  >> S_StatusUX);
        fdprintf(iFd, "KSU = %d  ", (uiCP0Status & M_StatusKSU) >> S_StatusKSU);
        fdprintf(iFd, "UM  = %d\n", (uiCP0Status & M_StatusUM)  >> S_StatusUM);
        fdprintf(iFd, "SM  = %d  ", (uiCP0Status & M_StatusSM)  >> S_StatusSM);
        fdprintf(iFd, "ERL = %d\n", (uiCP0Status & M_StatusERL) >> S_StatusERL);
        fdprintf(iFd, "EXL = %d  ", (uiCP0Status & M_StatusEXL) >> S_StatusEXL);
        fdprintf(iFd, "IE  = %d\n", (uiCP0Status & M_StatusIE)  >> S_StatusIE);

    } else {
        archTaskCtxPrint(LW_NULL, 0, pstkTop);
    }
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** 函数名称: archTaskCtxPrint
** 功能描述: 通过 _PrintFormat 打印任务上下文
** 输　入  : pvBuffer   内存缓冲区 (NULL, 表示直接打印)
**           stSize     缓冲大小
**           pstkTop    堆栈栈顶
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTaskCtxPrint (PVOID  pvBuffer, size_t  stSize, PLW_STACK  pstkTop)
{
    UINT32              uiCP0Status;
    ARCH_REG_CTX       *pregctx = (ARCH_REG_CTX *)pstkTop;

    if (pvBuffer && stSize) {
        size_t  stOft = 0;

        stOft = bnprintf(pvBuffer, stSize, stOft, "SP        = 0x%08x\n", (ARCH_REG_T)pstkTop);
        stOft = bnprintf(pvBuffer, stSize, stOft, "EPC       = 0x%08x\n", (ARCH_REG_T)pregctx->REG_uiCP0EPC);
        stOft = bnprintf(pvBuffer, stSize, stOft, "BADVADDR  = 0x%08x\n", (ARCH_REG_T)pregctx->REG_uiCP0BadVAddr);
        stOft = bnprintf(pvBuffer, stSize, stOft, "CAUSE     = 0x%08x\n", (ARCH_REG_T)pregctx->REG_uiCP0Cause);
        stOft = bnprintf(pvBuffer, stSize, stOft, "LO        = 0x%08x\n", (ARCH_REG_T)pregctx->REG_uiCP0DataLO);
        stOft = bnprintf(pvBuffer, stSize, stOft, "HI        = 0x%08x\n", (ARCH_REG_T)pregctx->REG_uiCP0DataHI);

        stOft = bnprintf(pvBuffer, stSize, stOft, "$00(ZERO) = 0x%08x\n", pregctx->REG_uiReg[REG_ZERO]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$01(AT)   = 0x%08x\n", pregctx->REG_uiReg[REG_AT]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$02(V0)   = 0x%08x\n", pregctx->REG_uiReg[REG_V0]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$03(V1)   = 0x%08x\n", pregctx->REG_uiReg[REG_V1]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$04(A0)   = 0x%08x\n", pregctx->REG_uiReg[REG_A0]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$05(A1)   = 0x%08x\n", pregctx->REG_uiReg[REG_A1]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$06(A2)   = 0x%08x\n", pregctx->REG_uiReg[REG_A2]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$07(A3)   = 0x%08x\n", pregctx->REG_uiReg[REG_A3]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$08(T0)   = 0x%08x\n", pregctx->REG_uiReg[REG_T0]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$09(T1)   = 0x%08x\n", pregctx->REG_uiReg[REG_T1]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$10(T2)   = 0x%08x\n", pregctx->REG_uiReg[REG_T2]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$11(T3)   = 0x%08x\n", pregctx->REG_uiReg[REG_T3]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$12(T4)   = 0x%08x\n", pregctx->REG_uiReg[REG_T4]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$13(T5)   = 0x%08x\n", pregctx->REG_uiReg[REG_T5]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$14(T6)   = 0x%08x\n", pregctx->REG_uiReg[REG_T6]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$15(T7)   = 0x%08x\n", pregctx->REG_uiReg[REG_T7]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$16(S0)   = 0x%08x\n", pregctx->REG_uiReg[REG_S0]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$17(S1)   = 0x%08x\n", pregctx->REG_uiReg[REG_S1]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$18(S2)   = 0x%08x\n", pregctx->REG_uiReg[REG_S2]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$19(S3)   = 0x%08x\n", pregctx->REG_uiReg[REG_S3]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$20(S4)   = 0x%08x\n", pregctx->REG_uiReg[REG_S4]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$21(S5)   = 0x%08x\n", pregctx->REG_uiReg[REG_S5]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$22(S6)   = 0x%08x\n", pregctx->REG_uiReg[REG_S6]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$23(S7)   = 0x%08x\n", pregctx->REG_uiReg[REG_S7]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$24(T8)   = 0x%08x\n", pregctx->REG_uiReg[REG_T8]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$25(T9)   = 0x%08x\n", pregctx->REG_uiReg[REG_T9]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$28(GP)   = 0x%08x\n", pregctx->REG_uiReg[REG_GP]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$29(SP)   = 0x%08x\n", pregctx->REG_uiReg[REG_SP]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$30(FP)   = 0x%08x\n", pregctx->REG_uiReg[REG_FP]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "$31(RA)   = 0x%08x\n", pregctx->REG_uiReg[REG_RA]);

        uiCP0Status = pregctx->REG_uiCP0Status;

        stOft = bnprintf(pvBuffer, stSize, stOft, "CP0 Status Register:\n");
        stOft = bnprintf(pvBuffer, stSize, stOft, "CU3 = %d  ", (uiCP0Status & M_StatusCU3) >> S_StatusCU3);
        stOft = bnprintf(pvBuffer, stSize, stOft, "CU2 = %d\n", (uiCP0Status & M_StatusCU2) >> S_StatusCU2);
        stOft = bnprintf(pvBuffer, stSize, stOft, "CU1 = %d  ", (uiCP0Status & M_StatusCU1) >> S_StatusCU1);
        stOft = bnprintf(pvBuffer, stSize, stOft, "CU0 = %d\n", (uiCP0Status & M_StatusCU0) >> S_StatusCU0);
        stOft = bnprintf(pvBuffer, stSize, stOft, "RP  = %d  ", (uiCP0Status & M_StatusRP)  >> S_StatusRP);
        stOft = bnprintf(pvBuffer, stSize, stOft, "FR  = %d\n", (uiCP0Status & M_StatusFR)  >> S_StatusFR);
        stOft = bnprintf(pvBuffer, stSize, stOft, "RE  = %d  ", (uiCP0Status & M_StatusRE)  >> S_StatusRE);
        stOft = bnprintf(pvBuffer, stSize, stOft, "MX  = %d\n", (uiCP0Status & M_StatusMX)  >> S_StatusMX);
        stOft = bnprintf(pvBuffer, stSize, stOft, "PX  = %d  ", (uiCP0Status & M_StatusPX)  >> S_StatusPX);
        stOft = bnprintf(pvBuffer, stSize, stOft, "BEV = %d\n", (uiCP0Status & M_StatusBEV) >> S_StatusBEV);
        stOft = bnprintf(pvBuffer, stSize, stOft, "TS  = %d  ", (uiCP0Status & M_StatusTS)  >> S_StatusTS);
        stOft = bnprintf(pvBuffer, stSize, stOft, "SR  = %d\n", (uiCP0Status & M_StatusSR)  >> S_StatusSR);
        stOft = bnprintf(pvBuffer, stSize, stOft, "NMI = %d  ", (uiCP0Status & M_StatusNMI) >> S_StatusNMI);
        stOft = bnprintf(pvBuffer, stSize, stOft, "IM7 = %d\n", (uiCP0Status & M_StatusIM7) >> S_StatusIM7);
        stOft = bnprintf(pvBuffer, stSize, stOft, "IM6 = %d  ", (uiCP0Status & M_StatusIM6) >> S_StatusIM6);
        stOft = bnprintf(pvBuffer, stSize, stOft, "IM5 = %d\n", (uiCP0Status & M_StatusIM5) >> S_StatusIM5);
        stOft = bnprintf(pvBuffer, stSize, stOft, "IM4 = %d  ", (uiCP0Status & M_StatusIM4) >> S_StatusIM4);
        stOft = bnprintf(pvBuffer, stSize, stOft, "IM3 = %d\n", (uiCP0Status & M_StatusIM3) >> S_StatusIM3);
        stOft = bnprintf(pvBuffer, stSize, stOft, "IM2 = %d  ", (uiCP0Status & M_StatusIM2) >> S_StatusIM2);
        stOft = bnprintf(pvBuffer, stSize, stOft, "IM1 = %d\n", (uiCP0Status & M_StatusIM1) >> S_StatusIM1);
        stOft = bnprintf(pvBuffer, stSize, stOft, "IM0 = %d  ", (uiCP0Status & M_StatusIM0) >> S_StatusIM0);
        stOft = bnprintf(pvBuffer, stSize, stOft, "KX  = %d\n", (uiCP0Status & M_StatusKX)  >> S_StatusKX);
        stOft = bnprintf(pvBuffer, stSize, stOft, "SX  = %d  ", (uiCP0Status & M_StatusSX)  >> S_StatusSX);
        stOft = bnprintf(pvBuffer, stSize, stOft, "UX  = %d\n", (uiCP0Status & M_StatusUX)  >> S_StatusUX);
        stOft = bnprintf(pvBuffer, stSize, stOft, "KSU = %d  ", (uiCP0Status & M_StatusKSU) >> S_StatusKSU);
        stOft = bnprintf(pvBuffer, stSize, stOft, "UM  = %d\n", (uiCP0Status & M_StatusUM)  >> S_StatusUM);
        stOft = bnprintf(pvBuffer, stSize, stOft, "SM  = %d  ", (uiCP0Status & M_StatusSM)  >> S_StatusSM);
        stOft = bnprintf(pvBuffer, stSize, stOft, "ERL = %d\n", (uiCP0Status & M_StatusERL) >> S_StatusERL);
        stOft = bnprintf(pvBuffer, stSize, stOft, "EXL = %d  ", (uiCP0Status & M_StatusEXL) >> S_StatusEXL);
        stOft = bnprintf(pvBuffer, stSize, stOft, "IE  = %d\n", (uiCP0Status & M_StatusIE)  >> S_StatusIE);

    } else {
        _PrintFormat("\r\n");

        _PrintFormat("SP        = 0x%08x\r\n", (ARCH_REG_T)pstkTop);
        _PrintFormat("EPC       = 0x%08x\r\n", (ARCH_REG_T)pregctx->REG_uiCP0EPC);
        _PrintFormat("BADVADDR  = 0x%08x\r\n", (ARCH_REG_T)pregctx->REG_uiCP0BadVAddr);
        _PrintFormat("CAUSE     = 0x%08x\r\n", (ARCH_REG_T)pregctx->REG_uiCP0Cause);
        _PrintFormat("LO        = 0x%08x\r\n", (ARCH_REG_T)pregctx->REG_uiCP0DataLO);
        _PrintFormat("HI        = 0x%08x\r\n", (ARCH_REG_T)pregctx->REG_uiCP0DataHI);

        _PrintFormat("$00(ZERO) = 0x%08x\r\n", pregctx->REG_uiReg[REG_ZERO]);
        _PrintFormat("$01(AT)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_AT]);
        _PrintFormat("$02(V0)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_V0]);
        _PrintFormat("$03(V1)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_V1]);
        _PrintFormat("$04(A0)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_A0]);
        _PrintFormat("$05(A1)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_A1]);
        _PrintFormat("$06(A2)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_A2]);
        _PrintFormat("$07(A3)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_A3]);
        _PrintFormat("$08(T0)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_T0]);
        _PrintFormat("$09(T1)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_T1]);
        _PrintFormat("$10(T2)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_T2]);
        _PrintFormat("$11(T3)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_T3]);
        _PrintFormat("$12(T4)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_T4]);
        _PrintFormat("$13(T5)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_T5]);
        _PrintFormat("$14(T6)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_T6]);
        _PrintFormat("$15(T7)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_T7]);
        _PrintFormat("$16(S0)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_S0]);
        _PrintFormat("$17(S1)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_S1]);
        _PrintFormat("$18(S2)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_S2]);
        _PrintFormat("$19(S3)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_S3]);
        _PrintFormat("$20(S4)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_S4]);
        _PrintFormat("$21(S5)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_S5]);
        _PrintFormat("$22(S6)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_S6]);
        _PrintFormat("$23(S7)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_S7]);
        _PrintFormat("$24(T8)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_T8]);
        _PrintFormat("$25(T9)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_T9]);
        _PrintFormat("$28(GP)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_GP]);
        _PrintFormat("$29(SP)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_SP]);
        _PrintFormat("$30(FP)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_FP]);
        _PrintFormat("$31(RA)   = 0x%08x\r\n", pregctx->REG_uiReg[REG_RA]);

        uiCP0Status = pregctx->REG_uiCP0Status;

        _PrintFormat("CP0 Status Register:\r\n");
        _PrintFormat("CU3 = %d  ",   (uiCP0Status & M_StatusCU3) >> S_StatusCU3);
        _PrintFormat("CU2 = %d\r\n", (uiCP0Status & M_StatusCU2) >> S_StatusCU2);
        _PrintFormat("CU1 = %d  ",   (uiCP0Status & M_StatusCU1) >> S_StatusCU1);
        _PrintFormat("CU0 = %d\r\n", (uiCP0Status & M_StatusCU0) >> S_StatusCU0);
        _PrintFormat("RP  = %d  ",   (uiCP0Status & M_StatusRP)  >> S_StatusRP);
        _PrintFormat("FR  = %d\r\n", (uiCP0Status & M_StatusFR)  >> S_StatusFR);
        _PrintFormat("RE  = %d  ",   (uiCP0Status & M_StatusRE)  >> S_StatusRE);
        _PrintFormat("MX  = %d\r\n", (uiCP0Status & M_StatusMX)  >> S_StatusMX);
        _PrintFormat("PX  = %d  ",   (uiCP0Status & M_StatusPX)  >> S_StatusPX);
        _PrintFormat("BEV = %d\r\n", (uiCP0Status & M_StatusBEV) >> S_StatusBEV);
        _PrintFormat("TS  = %d  ",   (uiCP0Status & M_StatusTS)  >> S_StatusTS);
        _PrintFormat("SR  = %d\r\n", (uiCP0Status & M_StatusSR)  >> S_StatusSR);
        _PrintFormat("NMI = %d  ",   (uiCP0Status & M_StatusNMI) >> S_StatusNMI);
        _PrintFormat("IM7 = %d\r\n", (uiCP0Status & M_StatusIM7) >> S_StatusIM7);
        _PrintFormat("IM6 = %d  ",   (uiCP0Status & M_StatusIM6) >> S_StatusIM6);
        _PrintFormat("IM5 = %d\r\n", (uiCP0Status & M_StatusIM5) >> S_StatusIM5);
        _PrintFormat("IM4 = %d  ",   (uiCP0Status & M_StatusIM4) >> S_StatusIM4);
        _PrintFormat("IM3 = %d\r\n", (uiCP0Status & M_StatusIM3) >> S_StatusIM3);
        _PrintFormat("IM2 = %d  ",   (uiCP0Status & M_StatusIM2) >> S_StatusIM2);
        _PrintFormat("IM1 = %d\r\n", (uiCP0Status & M_StatusIM1) >> S_StatusIM1);
        _PrintFormat("IM0 = %d  ",   (uiCP0Status & M_StatusIM0) >> S_StatusIM0);
        _PrintFormat("KX  = %d\r\n", (uiCP0Status & M_StatusKX)  >> S_StatusKX);
        _PrintFormat("SX  = %d  ",   (uiCP0Status & M_StatusSX)  >> S_StatusSX);
        _PrintFormat("UX  = %d\r\n", (uiCP0Status & M_StatusUX)  >> S_StatusUX);
        _PrintFormat("KSU = %d  ",   (uiCP0Status & M_StatusKSU) >> S_StatusKSU);
        _PrintFormat("UM  = %d\r\n", (uiCP0Status & M_StatusUM)  >> S_StatusUM);
        _PrintFormat("SM  = %d  ",   (uiCP0Status & M_StatusSM)  >> S_StatusSM);
        _PrintFormat("ERL = %d\r\n", (uiCP0Status & M_StatusERL) >> S_StatusERL);
        _PrintFormat("EXL = %d  ",   (uiCP0Status & M_StatusEXL) >> S_StatusEXL);
        _PrintFormat("IE  = %d\r\n", (uiCP0Status & M_StatusIE)  >> S_StatusIE);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

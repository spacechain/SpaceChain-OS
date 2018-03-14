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
** 文   件   名: armContextV7M.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 11 月 14 日
**
** 描        述: ARMv7M 体系构架上下文处理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv7M 体系构架
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__)
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
    ARCH_REG_CTX      *pregctx;
    ARCH_FP_CTX       *pfpctx;

    if ((addr_t)pstkTop & 0x7) {                                        /*  保证出栈后 CPU SP 8 字节对齐*/
        pstkTop = (PLW_STACK)((addr_t)pstkTop - 4);                     /*  向低地址推进 4 字节         */
    }

    pfpctx  = (ARCH_FP_CTX  *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX));
    pregctx = (ARCH_REG_CTX *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX) - sizeof(ARCH_REG_CTX));

    pfpctx->FP_uiFp = (ARCH_REG_T)LW_NULL;
    pfpctx->FP_uiLr = (ARCH_REG_T)LW_NULL;

    pregctx->REG_uiCpsr = 0x01000000;
    pregctx->REG_uiR0   = (ARCH_REG_T)pvArg;
    pregctx->REG_uiR1   = 0x01010101;
    pregctx->REG_uiR2   = 0x02020202;
    pregctx->REG_uiR3   = 0x03030303;
    pregctx->REG_uiR4   = 0x04040404;
    pregctx->REG_uiR5   = 0x05050505;
    pregctx->REG_uiR6   = 0x06060606;
    pregctx->REG_uiR7   = 0x07070707;
    pregctx->REG_uiR8   = 0x08080808;
    pregctx->REG_uiR9   = 0x09090909;
    pregctx->REG_uiR10  = 0x10101010;
    pregctx->REG_uiFp   = pfpctx->FP_uiFp;
    pregctx->REG_uiIp   = 0x12121212;
    pregctx->REG_uiLr   = 0xfffffffe;
    pregctx->REG_uiPc   = (ARCH_REG_T)pfuncTask;

    pregctx->REG_uiExcRet  = 0xfffffffd;
    pregctx->REG_uiBASEPRI = 0;

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
    ARCH_FP_CTX       *pfpctx      = (ARCH_FP_CTX *)((PCHAR)pregctxDest + sizeof(ARCH_REG_CTX));

    /*
     *  在 ARCH_FP_CTX 区域内, 模拟了一次
     *  push {fp, lr}
     *  add  fp, sp, #4
     */
    pfpctx->FP_uiFp = pregctxSrc->REG_uiFp;
    pfpctx->FP_uiLr = pregctxSrc->REG_uiLr;

    pregctxDest->REG_uiFp = (ARCH_REG_T)&pfpctx->FP_uiLr;
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
    if (iFd >= 0) {
        fdprintf(iFd, "XPSR    = 0x%08x\n", pstkTop[17]);
        fdprintf(iFd, "BASEPRI = 0x%08x\n", pstkTop[0]);
        fdprintf(iFd, "EXCRET  = 0x%08x\n", pstkTop[9]);

        fdprintf(iFd, "r0  = 0x%08x  ", pstkTop[10]);
        fdprintf(iFd, "r1  = 0x%08x\n", pstkTop[11]);
        fdprintf(iFd, "r2  = 0x%08x  ", pstkTop[12]);
        fdprintf(iFd, "r3  = 0x%08x\n", pstkTop[13]);
        fdprintf(iFd, "r4  = 0x%08x  ", pstkTop[1]);
        fdprintf(iFd, "r5  = 0x%08x\n", pstkTop[2]);
        fdprintf(iFd, "r6  = 0x%08x  ", pstkTop[3]);
        fdprintf(iFd, "r7  = 0x%08x\n", pstkTop[4]);
        fdprintf(iFd, "r8  = 0x%08x  ", pstkTop[5]);
        fdprintf(iFd, "r9  = 0x%08x\n", pstkTop[6]);
        fdprintf(iFd, "r10 = 0x%08x  ", pstkTop[7]);
        fdprintf(iFd, "fp  = 0x%08x\n", pstkTop[8]);
        fdprintf(iFd, "ip  = 0x%08x  ", pstkTop[14]);
        fdprintf(iFd, "sp  = 0x%08x\n", (ARCH_REG_T)pstkTop);
        fdprintf(iFd, "lr  = 0x%08x  ", pstkTop[15]);
        fdprintf(iFd, "pc  = 0x%08x\n", pstkTop[16]);

    } else {
        archTaskCtxPrint(LW_NULL, 0, pstkTop);
    }
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** 函数名称: archTaskCtxPrint
** 功能描述: 直接打印任务上下文
** 输　入  : pvBuffer   内存缓冲区 (NULL, 表示直接打印)
**           stSize     缓冲大小
**           pstkTop    堆栈栈顶
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archTaskCtxPrint (PVOID  pvBuffer, size_t  stSize, PLW_STACK  pstkTop)
{
    if (pvBuffer && stSize) {
        size_t  stOft = 0;

        stOft = bnprintf(pvBuffer, stSize, stOft, "XPSR    = 0x%08x\n", pstkTop[17]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "BASEPRI = 0x%08x\n", pstkTop[0]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "EXCRET  = 0x%08x\n", pstkTop[9]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "r0  = 0x%08x  ", pstkTop[10]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "r1  = 0x%08x\n", pstkTop[11]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "r2  = 0x%08x  ", pstkTop[12]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "r3  = 0x%08x\n", pstkTop[13]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "r4  = 0x%08x  ", pstkTop[1]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "r5  = 0x%08x\n", pstkTop[2]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "r6  = 0x%08x  ", pstkTop[3]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "r7  = 0x%08x\n", pstkTop[4]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "r8  = 0x%08x  ", pstkTop[5]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "r9  = 0x%08x\n", pstkTop[6]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "r10 = 0x%08x  ", pstkTop[7]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "fp  = 0x%08x\n", pstkTop[8]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "ip  = 0x%08x  ", pstkTop[14]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "sp  = 0x%08x\n", (ARCH_REG_T)pstkTop);
        stOft = bnprintf(pvBuffer, stSize, stOft, "lr  = 0x%08x  ", pstkTop[15]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "pc  = 0x%08x\n", pstkTop[16]);

    } else {
        _PrintFormat("XPSR    = 0x%08x\r\n", pstkTop[17]);
        _PrintFormat("BASEPRI = 0x%08x\r\n", pstkTop[0]);
        _PrintFormat("EXCRET  = 0x%08x\r\n", pstkTop[9]);

        _PrintFormat("r0  = 0x%08x  ",   pstkTop[10]);
        _PrintFormat("r1  = 0x%08x\r\n", pstkTop[11]);
        _PrintFormat("r2  = 0x%08x  ",   pstkTop[12]);
        _PrintFormat("r3  = 0x%08x\r\n", pstkTop[13]);
        _PrintFormat("r4  = 0x%08x  ",   pstkTop[1]);
        _PrintFormat("r5  = 0x%08x\r\n", pstkTop[2]);
        _PrintFormat("r6  = 0x%08x  ",   pstkTop[3]);
        _PrintFormat("r7  = 0x%08x\r\n", pstkTop[4]);
        _PrintFormat("r8  = 0x%08x  ",   pstkTop[5]);
        _PrintFormat("r9  = 0x%08x\r\n", pstkTop[6]);
        _PrintFormat("r10 = 0x%08x  ",   pstkTop[7]);
        _PrintFormat("fp  = 0x%08x\r\n", pstkTop[8]);
        _PrintFormat("ip  = 0x%08x  ",   pstkTop[14]);
        _PrintFormat("sp  = 0x%08x\r\n", (ARCH_REG_T)pstkTop);
        _PrintFormat("lr  = 0x%08x  ",   pstkTop[15]);
        _PrintFormat("pc  = 0x%08x\r\n", pstkTop[16]);
    }
}

#endif                                                                  /*  __SYLIXOS_ARM_ARCH_M__      */
/*********************************************************************************************************
  END
*********************************************************************************************************/

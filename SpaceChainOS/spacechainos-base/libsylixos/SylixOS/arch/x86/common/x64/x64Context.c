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
** 文   件   名: x64Context.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 06 月 06 日
**
** 描        述: x86-64 体系构架上下文处理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "../x86Segment.h"
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

    pstkTop = (PLW_STACK)ROUND_DOWN(pstkTop, 16);                       /*  保证出栈后 SP 16 字节对齐   */
    pstkTop--;                                                          /*  GCC PUSH 后转为 16 字节对齐 */

    pfpctx  = (ARCH_FP_CTX  *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX));
    pregctx = (ARCH_REG_CTX *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX) - sizeof(ARCH_REG_CTX));

    pfpctx->FP_ulSavedRBP = (ARCH_REG_T)LW_NULL;
    pfpctx->FP_ulRetAddr  = (ARCH_REG_T)LW_NULL;

    lib_bzero(pregctx, sizeof(ARCH_REG_CTX));

    pregctx->REG_ulR8  = 0x0808080808080808;
    pregctx->REG_ulR9  = 0x0909090909090909;
    pregctx->REG_ulR10 = 0x1010101010101010;
    pregctx->REG_ulR11 = 0x1111111111111111;
    pregctx->REG_ulR12 = 0x1212121212121212;
    pregctx->REG_ulR13 = 0x1313131313131313;
    pregctx->REG_ulR14 = 0x1414141414141414;
    pregctx->REG_ulR15 = 0x1515151515151515;

    pregctx->REG_ulRAX = 0xeaeaeaeaeaeaeaea;
    pregctx->REG_ulRBX = 0xebebebebebebebeb;
    pregctx->REG_ulRCX = 0xecececececececec;
    pregctx->REG_ulRDX = 0xedededededededed;

    pregctx->REG_ulRSI   = 0xe0e0e0e0e0e0e0e0;
    pregctx->REG_ulRDI   = (ARCH_REG_T)pvArg;                           /*  参数                        */

    pregctx->REG_ulRBP   = (ARCH_REG_T)pfpctx;                          /*  RBP 指针寄存器              */
    pregctx->REG_ulRSP   = (ARCH_REG_T)pfpctx;                          /*  RSP 指针寄存器              */
    pregctx->REG_ulError = 0x0000000000000000;                          /*  ERROR CODE                  */
    pregctx->REG_ulRIP   = (ARCH_REG_T)pfuncTask;

    pregctx->REG_ulCS = X86_CS_KERNEL;
    pregctx->REG_usDS = X86_DS_KERNEL;
    pregctx->REG_usES = X86_DS_KERNEL;
    pregctx->REG_ulSS = X86_DS_KERNEL;
    pregctx->REG_usFS = X86_DS_KERNEL;
    pregctx->REG_usGS = X86_DS_KERNEL;

    pregctx->REG_ulRFLAGS = X86_EFLAGS_IF;                              /*  设置中断使能位              */

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
    ARCH_FP_CTX       *pfpctx      = (ARCH_FP_CTX  *)((PCHAR)pstkDest + sizeof(ARCH_REG_CTX));
    ARCH_REG_CTX      *pregctxDest = (ARCH_REG_CTX *)pstkDest;
    ARCH_REG_CTX      *pregctxSrc  = (ARCH_REG_CTX *)pstkSrc;

    pregctxDest->REG_ulRBP = (ARCH_REG_T)pregctxSrc->REG_ulRBP;
    pfpctx->FP_ulRetAddr   = (ARCH_REG_T)pregctxSrc->REG_ulRIP;
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
ARCH_REG_CTX  *archTaskRegsGet (PLW_STACK  pstkTop, ARCH_REG_T  *pregSp)
{
    ARCH_REG_CTX  *pregctx = (ARCH_REG_CTX *)pstkTop;
    
    *pregSp = pregctx->REG_ulRSP;
    
    return  (pregctx);
}
/*********************************************************************************************************
** 函数名称: archTaskRegsSet
** 功能描述: 通过栈顶指针设置寄存器表 (满栈结构)
** 输　入  : pstkTop        堆栈顶点
**           pregctx        寄存器表
** 输　出  : 寄存器结构
** 全局变量: 
** 调用模块: 
** 注  意  : 不修改段寄存器, RBP RSP
*********************************************************************************************************/
VOID  archTaskRegsSet (PLW_STACK  pstkTop, const ARCH_REG_CTX  *pregctx)
{
    ARCH_REG_CTX  *pdestregctx = (ARCH_REG_CTX *)pstkTop;

    pdestregctx->REG_ulR8  = pregctx->REG_ulR8;
    pdestregctx->REG_ulR9  = pregctx->REG_ulR9;
    pdestregctx->REG_ulR10 = pregctx->REG_ulR10;
    pdestregctx->REG_ulR11 = pregctx->REG_ulR11;
    pdestregctx->REG_ulR12 = pregctx->REG_ulR12;
    pdestregctx->REG_ulR13 = pregctx->REG_ulR13;
    pdestregctx->REG_ulR14 = pregctx->REG_ulR14;
    pdestregctx->REG_ulR15 = pregctx->REG_ulR15;

    pdestregctx->REG_ulRAX = pregctx->REG_ulRAX;
    pdestregctx->REG_ulRBX = pregctx->REG_ulRBX;
    pdestregctx->REG_ulRCX = pregctx->REG_ulRCX;
    pdestregctx->REG_ulRDX = pregctx->REG_ulRDX;

    pdestregctx->REG_ulRSI = pregctx->REG_ulRSI;
    pdestregctx->REG_ulRDI = pregctx->REG_ulRDI;
    pdestregctx->REG_ulRIP = pregctx->REG_ulRIP;

    pdestregctx->REG_ulRFLAGS = pregctx->REG_ulRFLAGS;
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
    ARCH_REG_CTX       *pregctx = (ARCH_REG_CTX *)pstkTop;

    if (iFd >= 0) {
        fdprintf(iFd, "\n");
        fdprintf(iFd, "RFLAGS = 0x%016lx\n", pregctx->REG_ulRFLAGS);

        fdprintf(iFd, "RIP = 0x%016lx  ", pregctx->REG_ulRIP);
        fdprintf(iFd, "RBP = 0x%016lx\n", pregctx->REG_ulRBP);
        fdprintf(iFd, "RSI = 0x%016lx  ", pregctx->REG_ulRSI);
        fdprintf(iFd, "RDI = 0x%016lx\n", pregctx->REG_ulRDI);

        fdprintf(iFd, "RAX = 0x%016lx  ", pregctx->REG_ulRAX);
        fdprintf(iFd, "RBX = 0x%016lx\n", pregctx->REG_ulRBX);
        fdprintf(iFd, "RCX = 0x%016lx  ", pregctx->REG_ulRCX);
        fdprintf(iFd, "RDX = 0x%016lx\n", pregctx->REG_ulRDX);

        fdprintf(iFd, "SS  = 0x%016lx  ", pregctx->REG_ulSS);
        fdprintf(iFd, "GS  = 0x%016lx\n", pregctx->REG_usGS);
        fdprintf(iFd, "FS  = 0x%016lx  ", pregctx->REG_usFS);
        fdprintf(iFd, "ES  = 0x%016lx\n", pregctx->REG_usES);
        fdprintf(iFd, "DS  = 0x%016lx  ", pregctx->REG_usDS);
        fdprintf(iFd, "CS  = 0x%016lx\n", pregctx->REG_ulCS);

        fdprintf(iFd, "R8  = 0x%016lx  ", pregctx->REG_ulR8);
        fdprintf(iFd, "R9  = 0x%016lx\n", pregctx->REG_ulR9);
        fdprintf(iFd, "R10 = 0x%016lx  ", pregctx->REG_ulR10);
        fdprintf(iFd, "R11 = 0x%016lx\n", pregctx->REG_ulR11);
        fdprintf(iFd, "R12 = 0x%016lx  ", pregctx->REG_ulR12);
        fdprintf(iFd, "R13 = 0x%016lx\n", pregctx->REG_ulR13);
        fdprintf(iFd, "R14 = 0x%016lx  ", pregctx->REG_ulR14);
        fdprintf(iFd, "R15 = 0x%016lx\n", pregctx->REG_ulR15);

        fdprintf(iFd, "RSP = 0x%016lx  ", pregctx->REG_ulRSP);          /*  异常压栈前的 SP             */
        fdprintf(iFd, "SP  = 0x%016lx\n", (ARCH_REG_T)pstkTop);         /*  异常压栈后的 SP             */

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
    ARCH_REG_CTX       *pregctx = (ARCH_REG_CTX *)pstkTop;

    if (pvBuffer && stSize) {
        size_t  stOft = 0;

        stOft = bnprintf(pvBuffer, stSize, stOft, "RFLAGS = 0x%016qx\n", pregctx->REG_ulRFLAGS);

        stOft = bnprintf(pvBuffer, stSize, stOft, "RIP = 0x%016qx  ", pregctx->REG_ulRIP);
        stOft = bnprintf(pvBuffer, stSize, stOft, "RBP = 0x%016qx\n", pregctx->REG_ulRBP);
        stOft = bnprintf(pvBuffer, stSize, stOft, "RSI = 0x%016qx  ", pregctx->REG_ulRSI);
        stOft = bnprintf(pvBuffer, stSize, stOft, "RDI = 0x%016qx\n", pregctx->REG_ulRDI);

        stOft = bnprintf(pvBuffer, stSize, stOft, "RAX = 0x%016qx  ", pregctx->REG_ulRAX);
        stOft = bnprintf(pvBuffer, stSize, stOft, "RBX = 0x%016qx\n", pregctx->REG_ulRBX);
        stOft = bnprintf(pvBuffer, stSize, stOft, "RCX = 0x%016qx  ", pregctx->REG_ulRCX);
        stOft = bnprintf(pvBuffer, stSize, stOft, "RDX = 0x%016qx\n", pregctx->REG_ulRDX);

        stOft = bnprintf(pvBuffer, stSize, stOft, "SS  = 0x%016qx  ", pregctx->REG_ulSS);
        stOft = bnprintf(pvBuffer, stSize, stOft, "GS  = 0x%016qx\n", pregctx->REG_usGS);
        stOft = bnprintf(pvBuffer, stSize, stOft, "FS  = 0x%016qx  ", pregctx->REG_usFS);
        stOft = bnprintf(pvBuffer, stSize, stOft, "ES  = 0x%016qx\n", pregctx->REG_usES);
        stOft = bnprintf(pvBuffer, stSize, stOft, "DS  = 0x%016qx  ", pregctx->REG_usDS);
        stOft = bnprintf(pvBuffer, stSize, stOft, "CS  = 0x%016qx\n", pregctx->REG_ulCS);

        stOft = bnprintf(pvBuffer, stSize, stOft, "R8  = 0x%016qx  ", pregctx->REG_ulR8);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R9  = 0x%016qx\n", pregctx->REG_ulR9);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R10 = 0x%016qx  ", pregctx->REG_ulR10);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R11 = 0x%016qx\n", pregctx->REG_ulR11);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R12 = 0x%016qx  ", pregctx->REG_ulR12);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R13 = 0x%016qx\n", pregctx->REG_ulR13);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R14 = 0x%016qx  ", pregctx->REG_ulR14);
        stOft = bnprintf(pvBuffer, stSize, stOft, "R15 = 0x%016qx\n", pregctx->REG_ulR15);

        stOft = bnprintf(pvBuffer, stSize, stOft, "RSP = 0x%016qx  ", pregctx->REG_ulRSP);
        stOft = bnprintf(pvBuffer, stSize, stOft, "SP  = 0x%016qx\n", (ARCH_REG_T)pstkTop);

    } else {
        _PrintFormat("\r\n");
        _PrintFormat("RFLAGS = 0x%016qx\r\n", pregctx->REG_ulRFLAGS);

        _PrintFormat("RIP = 0x%016qx  ",   pregctx->REG_ulRIP);
        _PrintFormat("RBP = 0x%016qx\r\n", pregctx->REG_ulRBP);
        _PrintFormat("RSI = 0x%016qx  ",   pregctx->REG_ulRSI);
        _PrintFormat("RDI = 0x%016qx\r\n", pregctx->REG_ulRDI);

        _PrintFormat("RAX = 0x%016qx  ",   pregctx->REG_ulRAX);
        _PrintFormat("RBX = 0x%016qx\r\n", pregctx->REG_ulRBX);
        _PrintFormat("RCX = 0x%016qx  ",   pregctx->REG_ulRCX);
        _PrintFormat("RDX = 0x%016qx\r\n", pregctx->REG_ulRDX);

        _PrintFormat("SS  = 0x%016qx  ",   pregctx->REG_ulSS);
        _PrintFormat("GS  = 0x%016qx\r\n", pregctx->REG_usGS);
        _PrintFormat("FS  = 0x%016qx  ",   pregctx->REG_usFS);
        _PrintFormat("ES  = 0x%016qx\r\n", pregctx->REG_usES);
        _PrintFormat("DS  = 0x%016qx  ",   pregctx->REG_usDS);
        _PrintFormat("CS  = 0x%016qx\r\n", pregctx->REG_ulCS);

        _PrintFormat("R8  = 0x%016qx  ",   pregctx->REG_ulR8);
        _PrintFormat("R9  = 0x%016qx\r\n", pregctx->REG_ulR9);
        _PrintFormat("R10 = 0x%016qx  ",   pregctx->REG_ulR10);
        _PrintFormat("R11 = 0x%016qx\r\n", pregctx->REG_ulR11);
        _PrintFormat("R12 = 0x%016qx  ",   pregctx->REG_ulR12);
        _PrintFormat("R13 = 0x%016qx\r\n", pregctx->REG_ulR13);
        _PrintFormat("R14 = 0x%016qx  ",   pregctx->REG_ulR14);
        _PrintFormat("R15 = 0x%016qx\r\n", pregctx->REG_ulR15);

        _PrintFormat("RSP = 0x%016qx  ",   pregctx->REG_ulRSP);         /*  异常压栈前的 SP             */
        _PrintFormat("SP  = 0x%016qx\r\n", (ARCH_REG_T)pstkTop);        /*  异常压栈后的 SP             */
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

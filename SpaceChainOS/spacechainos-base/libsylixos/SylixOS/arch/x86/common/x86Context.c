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
** 文   件   名: x86Context.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 07 月 04 日
**
** 描        述: x86 体系构架上下文处理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "x86Segment.h"
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

    pstkTop = (PLW_STACK)ROUND_DOWN(pstkTop, 8);                        /*  保证出栈后 SP 8 字节对齐    */

    pfpctx  = (ARCH_FP_CTX  *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX));
    pregctx = (ARCH_REG_CTX *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX) - sizeof(ARCH_REG_CTX));

    pfpctx->FP_uiArg     = (ARCH_REG_T)pvArg;
    pfpctx->FP_uiRetAddr = (ARCH_REG_T)LW_NULL;

    lib_bzero(pregctx, sizeof(ARCH_REG_CTX));

    pregctx->REG_uiEAX = 0xeaeaeaea;                                    /*  4 个数据寄存器              */
    pregctx->REG_uiEBX = 0xebebebeb;
    pregctx->REG_uiECX = 0xecececec;
    pregctx->REG_uiEDX = 0xedededed;

    pregctx->REG_uiESI   = 0xe0e0e0e0;
    pregctx->REG_uiEDI   = 0xe1e1e1e1;                                  /*  2 个变址和指针寄存器        */
    pregctx->REG_uiEBP   = (ARCH_REG_T)pfpctx;                          /*  EBP 指针寄存器              */
    pregctx->REG_uiError = 0x00000000;                                  /*  ERROR CODE                  */
    pregctx->REG_uiEIP   = (ARCH_REG_T)pfuncTask;

    pregctx->REG_uiCS = X86_CS_KERNEL;                                  /*  6 个段寄存器                */
    pregctx->REG_usDS = X86_DS_KERNEL;
    pregctx->REG_usES = X86_DS_KERNEL;
    pregctx->REG_usSS = X86_DS_KERNEL;
    pregctx->REG_usFS = X86_DS_KERNEL;
    pregctx->REG_usGS = X86_DS_KERNEL;

    pregctx->REG_uiEFLAGS = X86_EFLAGS_IF;                              /*  设置中断使能位              */

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

    pregctxDest->REG_uiEBP = (ARCH_REG_T)pregctxSrc->REG_uiEBP;
    pfpctx->FP_uiRetAddr   = (ARCH_REG_T)pregctxSrc->REG_uiEIP;
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
    ARCH_REG_T  regSp = (ARCH_REG_T)pstkTop;
    
#if	CPU_STK_GROWTH == 0
    regSp -= sizeof(ARCH_REG_CTX);
#else
    regSp += sizeof(ARCH_REG_CTX);
#endif

    *pregSp = regSp;
    
    return  ((ARCH_REG_CTX *)pstkTop);
}
/*********************************************************************************************************
** 函数名称: archTaskRegsSet
** 功能描述: 通过栈顶指针设置寄存器表 (满栈结构)
** 输　入  : pstkTop        堆栈顶点
**           pregctx        寄存器表
** 输　出  : 寄存器结构
** 全局变量: 
** 调用模块: 
** 注  意  : 不修改段寄存器, EBP ESP
*********************************************************************************************************/
VOID  archTaskRegsSet (PLW_STACK  pstkTop, const ARCH_REG_CTX  *pregctx)
{
    ARCH_REG_CTX  *pdestregctx = (ARCH_REG_CTX *)pstkTop;

    pdestregctx->REG_uiEAX = pregctx->REG_uiEAX;
    pdestregctx->REG_uiECX = pregctx->REG_uiECX;
    pdestregctx->REG_uiEDX = pregctx->REG_uiEDX;
    pdestregctx->REG_uiEBX = pregctx->REG_uiEBX;

    pdestregctx->REG_uiESI = pregctx->REG_uiESI;
    pdestregctx->REG_uiEDI = pregctx->REG_uiEDI;
    pdestregctx->REG_uiEIP = pregctx->REG_uiEIP;

    pdestregctx->REG_uiEFLAGS = pregctx->REG_uiEFLAGS;
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
        fdprintf(iFd, "EFLAGS = 0x%08x\n", pregctx->REG_uiEFLAGS);

        fdprintf(iFd, "EIP = 0x%08x  ", pregctx->REG_uiEIP);
        fdprintf(iFd, "EBP = 0x%08x\n", pregctx->REG_uiEBP);
        fdprintf(iFd, "ESI = 0x%08x  ", pregctx->REG_uiESI);
        fdprintf(iFd, "EDI = 0x%08x\n", pregctx->REG_uiEDI);

        fdprintf(iFd, "EAX = 0x%08x  ", pregctx->REG_uiEAX);
        fdprintf(iFd, "EBX = 0x%08x\n", pregctx->REG_uiEBX);
        fdprintf(iFd, "ECX = 0x%08x  ", pregctx->REG_uiECX);
        fdprintf(iFd, "EDX = 0x%08x\n", pregctx->REG_uiEDX);

        fdprintf(iFd, "SS  = 0x%08x  ", pregctx->REG_usSS);
        fdprintf(iFd, "GS  = 0x%08x\n", pregctx->REG_usGS);
        fdprintf(iFd, "FS  = 0x%08x  ", pregctx->REG_usFS);
        fdprintf(iFd, "ES  = 0x%08x\n", pregctx->REG_usES);
        fdprintf(iFd, "DS  = 0x%08x  ", pregctx->REG_usDS);
        fdprintf(iFd, "CS  = 0x%08x\n", pregctx->REG_uiCS);

        fdprintf(iFd, "SP  = 0x%08x\n", (ARCH_REG_T)pstkTop);           /*  异常压栈后的 SP             */

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

        stOft = bnprintf(pvBuffer, stSize, stOft, "EFLAGS = 0x%08x\n", pregctx->REG_uiEFLAGS);

        stOft = bnprintf(pvBuffer, stSize, stOft, "EIP = 0x%08x  ", pregctx->REG_uiEIP);
        stOft = bnprintf(pvBuffer, stSize, stOft, "EBP = 0x%08x\n", pregctx->REG_uiEBP);
        stOft = bnprintf(pvBuffer, stSize, stOft, "ESI = 0x%08x  ", pregctx->REG_uiESI);
        stOft = bnprintf(pvBuffer, stSize, stOft, "EDI = 0x%08x\n", pregctx->REG_uiEDI);

        stOft = bnprintf(pvBuffer, stSize, stOft, "EAX = 0x%08x  ", pregctx->REG_uiEAX);
        stOft = bnprintf(pvBuffer, stSize, stOft, "EBX = 0x%08x\n", pregctx->REG_uiEBX);
        stOft = bnprintf(pvBuffer, stSize, stOft, "ECX = 0x%08x  ", pregctx->REG_uiECX);
        stOft = bnprintf(pvBuffer, stSize, stOft, "EDX = 0x%08x\n", pregctx->REG_uiEDX);

        stOft = bnprintf(pvBuffer, stSize, stOft, "SS  = 0x%08x  ", pregctx->REG_usSS);
        stOft = bnprintf(pvBuffer, stSize, stOft, "GS  = 0x%08x\n", pregctx->REG_usGS);
        stOft = bnprintf(pvBuffer, stSize, stOft, "FS  = 0x%08x  ", pregctx->REG_usFS);
        stOft = bnprintf(pvBuffer, stSize, stOft, "ES  = 0x%08x\n", pregctx->REG_usES);
        stOft = bnprintf(pvBuffer, stSize, stOft, "DS  = 0x%08x  ", pregctx->REG_usDS);
        stOft = bnprintf(pvBuffer, stSize, stOft, "CS  = 0x%08x\n", pregctx->REG_uiCS);

        stOft = bnprintf(pvBuffer, stSize, stOft, "SP  = 0x%08x\n", (ARCH_REG_T)pstkTop);

    } else {
        _PrintFormat("\r\n");
        _PrintFormat("EFLAGS = 0x%08x\r\n", pregctx->REG_uiEFLAGS);

        _PrintFormat("EIP = 0x%08x  ",   pregctx->REG_uiEIP);
        _PrintFormat("EBP = 0x%08x\r\n", pregctx->REG_uiEBP);
        _PrintFormat("ESI = 0x%08x  ",   pregctx->REG_uiESI);
        _PrintFormat("EDI = 0x%08x\r\n", pregctx->REG_uiEDI);

        _PrintFormat("EAX = 0x%08x  ",   pregctx->REG_uiEAX);
        _PrintFormat("EBX = 0x%08x\r\n", pregctx->REG_uiEBX);
        _PrintFormat("ECX = 0x%08x  ",   pregctx->REG_uiECX);
        _PrintFormat("EDX = 0x%08x\r\n", pregctx->REG_uiEDX);

        _PrintFormat("SS  = 0x%08x  ",   pregctx->REG_usSS);
        _PrintFormat("GS  = 0x%08x\r\n", pregctx->REG_usGS);
        _PrintFormat("FS  = 0x%08x  ",   pregctx->REG_usFS);
        _PrintFormat("ES  = 0x%08x\r\n", pregctx->REG_usES);
        _PrintFormat("DS  = 0x%08x  ",   pregctx->REG_usDS);
        _PrintFormat("CS  = 0x%08x\r\n", pregctx->REG_uiCS);

        _PrintFormat("SP  = 0x%08x\r\n", (ARCH_REG_T)pstkTop);          /*  异常压栈后的 SP             */
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

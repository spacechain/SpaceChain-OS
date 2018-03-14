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
** 文   件   名: sparcContext.c
**
** 创   建   人: Xu.Guizhou (徐贵洲)
**
** 文件创建日期: 2017 年 05 月 15 日
**
** 描        述: SPARC 体系构架上下文处理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "sparcLib.h"
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
    ARCH_REG_T         uiPsr;

    pstkTop = (PLW_STACK)ROUND_DOWN(pstkTop, 8);                        /*  保证出栈后 SP 8 字节对齐    */

    pfpctx  = (ARCH_FP_CTX  *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX));
    pregctx = (ARCH_REG_CTX *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX) - sizeof(ARCH_REG_CTX));

    pregctx->REG_uiGlobal[0]  = 0x00000000;                             /*  Init global regs            */
    pregctx->REG_uiGlobal[1]  = 0x01010101;
    pregctx->REG_uiGlobal[2]  = 0x02020202;
    pregctx->REG_uiGlobal[3]  = 0x03030303;
    pregctx->REG_uiGlobal[4]  = 0x04040404;
    pregctx->REG_uiGlobal[5]  = 0x05050505;
    pregctx->REG_uiGlobal[6]  = 0x06060606;
    pregctx->REG_uiGlobal[7]  = 0x07070707;

    pregctx->REG_uiOutput[0]  = (ARCH_REG_T)pvArg;                      /*  Init output regs            */
    pregctx->REG_uiOutput[1]  = 0x11111111;
    pregctx->REG_uiOutput[2]  = 0x12121212;
    pregctx->REG_uiOutput[3]  = 0x13131313;
    pregctx->REG_uiOutput[4]  = 0x14141414;
    pregctx->REG_uiOutput[5]  = 0x15151515;
    pregctx->REG_uiSp         = (ARCH_REG_T)pregctx;
    pregctx->REG_uiOutput[7]  = 0x00000000;

    pregctx->REG_uiLocal[0]   = 0x20202020;                             /*  Init local regs             */
    pregctx->REG_uiLocal[1]   = 0x21212121;
    pregctx->REG_uiLocal[2]   = 0x22222222;
    pregctx->REG_uiLocal[3]   = 0x23131313;
    pregctx->REG_uiLocal[4]   = 0x24242424;
    pregctx->REG_uiLocal[5]   = 0x25252525;
    pregctx->REG_uiLocal[6]   = 0x26262626;
    pregctx->REG_uiLocal[7]   = 0x27272727;

    pregctx->REG_uiInput[0]   = 0x30303030;                             /*  Init input regs             */
    pregctx->REG_uiInput[1]   = 0x31313131;
    pregctx->REG_uiInput[2]   = 0x32323232;
    pregctx->REG_uiInput[3]   = 0x33333333;
    pregctx->REG_uiInput[4]   = 0x34343434;
    pregctx->REG_uiInput[5]   = 0x35353535;
    pregctx->REG_uiFp         = 0x00000000;
    pregctx->REG_uiInput[7]   = 0x00000000;

    uiPsr  = archPsrGet();
    uiPsr &= ~PSR_PIL;                                                  /*  使能中断                    */
    uiPsr &= ~PSR_CWP;                                                  /*  使用窗口 0                  */
    uiPsr |= PSR_S | PSR_PS;                                            /*  特权模式                    */
    uiPsr |= PSR_ET;                                                    /*  使能异常                    */
    uiPsr &= ~PSR_EF;                                                   /*  禁能浮点                    */
    uiPsr &= ~PSR_EC;                                                   /*  禁能协处理器                */
    pregctx->REG_uiPsr = uiPsr;
    pregctx->REG_uiWim = 0x2;                                           /*  Window 1  (all empty)       */
    pregctx->REG_uiPc  = (ARCH_REG_T)pfuncTask;                         /*  PC                          */
    pregctx->REG_uiNPc = pregctx->REG_uiPc + 4;                         /*  Next PC                     */
    pregctx->REG_uiY   = 0x00000000;                                    /*  Y 寄存器                    */

    pfpctx->FP_uiFp       = 0x00000000;
    pfpctx->FP_uiRetAddr  = 0x00000000;

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

    pregctxDest->REG_uiFp       = (ARCH_REG_T)&pregctxSrc->REG_uiLocal[0];
    pregctxDest->REG_uiInput[7] = pregctxSrc->REG_uiPc;
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
    ARCH_REG_CTX *pregDest = (ARCH_REG_CTX *)pstkTop;

    pregDest->REG_uiGlobal[0] = pregctx->REG_uiGlobal[0];
    pregDest->REG_uiGlobal[1] = pregctx->REG_uiGlobal[1];
    pregDest->REG_uiGlobal[2] = pregctx->REG_uiGlobal[2];
    pregDest->REG_uiGlobal[3] = pregctx->REG_uiGlobal[3];
    pregDest->REG_uiGlobal[4] = pregctx->REG_uiGlobal[4];
    pregDest->REG_uiGlobal[5] = pregctx->REG_uiGlobal[5];
    pregDest->REG_uiGlobal[6] = pregctx->REG_uiGlobal[6];
    pregDest->REG_uiGlobal[7] = pregctx->REG_uiGlobal[7];

    pregDest->REG_uiOutput[0] = pregctx->REG_uiOutput[0];
    pregDest->REG_uiOutput[1] = pregctx->REG_uiOutput[1];
    pregDest->REG_uiOutput[2] = pregctx->REG_uiOutput[2];
    pregDest->REG_uiOutput[3] = pregctx->REG_uiOutput[3];
    pregDest->REG_uiOutput[4] = pregctx->REG_uiOutput[4];
    pregDest->REG_uiOutput[5] = pregctx->REG_uiOutput[5];
    /*
     * %o6(SP)不设置
     */
    pregDest->REG_uiOutput[7] = pregctx->REG_uiOutput[7];

    pregDest->REG_uiLocal[0]  = pregctx->REG_uiLocal[0];
    pregDest->REG_uiLocal[1]  = pregctx->REG_uiLocal[1];
    pregDest->REG_uiLocal[2]  = pregctx->REG_uiLocal[2];
    pregDest->REG_uiLocal[3]  = pregctx->REG_uiLocal[3];
    pregDest->REG_uiLocal[4]  = pregctx->REG_uiLocal[4];
    pregDest->REG_uiLocal[5]  = pregctx->REG_uiLocal[5];
    pregDest->REG_uiLocal[6]  = pregctx->REG_uiLocal[6];
    pregDest->REG_uiLocal[7]  = pregctx->REG_uiLocal[7];

    pregDest->REG_uiInput[0]  = pregctx->REG_uiInput[0];
    pregDest->REG_uiInput[1]  = pregctx->REG_uiInput[1];
    pregDest->REG_uiInput[2]  = pregctx->REG_uiInput[2];
    pregDest->REG_uiInput[3]  = pregctx->REG_uiInput[3];
    pregDest->REG_uiInput[4]  = pregctx->REG_uiInput[4];
    pregDest->REG_uiInput[5]  = pregctx->REG_uiInput[5];
    /*
     * %i6(FP)不设置
     */
    pregDest->REG_uiInput[7]  = pregctx->REG_uiInput[7];

    pregDest->REG_uiPsr       = pregctx->REG_uiPsr;
    pregDest->REG_uiTbr       = pregctx->REG_uiTbr;
    pregDest->REG_uiPc        = pregctx->REG_uiPc;
    pregDest->REG_uiNPc       = pregctx->REG_uiNPc;
    pregDest->REG_uiWim       = pregctx->REG_uiWim;
    pregDest->REG_uiY         = pregctx->REG_uiY;
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

        fdprintf(iFd, "g0  = 0x%08x  ", pregctx->REG_uiGlobal[0]);
        fdprintf(iFd, "g1  = 0x%08x\n", pregctx->REG_uiGlobal[1]);
        fdprintf(iFd, "g2  = 0x%08x  ", pregctx->REG_uiGlobal[2]);
        fdprintf(iFd, "g3  = 0x%08x\n", pregctx->REG_uiGlobal[3]);
        fdprintf(iFd, "g4  = 0x%08x  ", pregctx->REG_uiGlobal[4]);
        fdprintf(iFd, "g5  = 0x%08x\n", pregctx->REG_uiGlobal[5]);
        fdprintf(iFd, "g6  = 0x%08x  ", pregctx->REG_uiGlobal[6]);
        fdprintf(iFd, "g7  = 0x%08x\n", pregctx->REG_uiGlobal[7]);

        fdprintf(iFd, "o0  = 0x%08x  ", pregctx->REG_uiOutput[0]);
        fdprintf(iFd, "o1  = 0x%08x\n", pregctx->REG_uiOutput[1]);
        fdprintf(iFd, "o2  = 0x%08x  ", pregctx->REG_uiOutput[2]);
        fdprintf(iFd, "o3  = 0x%08x\n", pregctx->REG_uiOutput[3]);
        fdprintf(iFd, "o4  = 0x%08x  ", pregctx->REG_uiOutput[4]);
        fdprintf(iFd, "o5  = 0x%08x\n", pregctx->REG_uiOutput[5]);
        fdprintf(iFd, "o6  = 0x%08x  ", pregctx->REG_uiOutput[6]);      /*  异常压栈前的 SP             */
        fdprintf(iFd, "o7  = 0x%08x\n", pregctx->REG_uiOutput[7]);

        fdprintf(iFd, "l0  = 0x%08x  ", pregctx->REG_uiLocal[0]);
        fdprintf(iFd, "l1  = 0x%08x\n", pregctx->REG_uiLocal[1]);
        fdprintf(iFd, "l2  = 0x%08x  ", pregctx->REG_uiLocal[2]);
        fdprintf(iFd, "l3  = 0x%08x\n", pregctx->REG_uiLocal[3]);
        fdprintf(iFd, "l4  = 0x%08x  ", pregctx->REG_uiLocal[4]);
        fdprintf(iFd, "l5  = 0x%08x\n", pregctx->REG_uiLocal[5]);
        fdprintf(iFd, "l6  = 0x%08x  ", pregctx->REG_uiLocal[6]);
        fdprintf(iFd, "l7  = 0x%08x\n", pregctx->REG_uiLocal[7]);

        fdprintf(iFd, "i0  = 0x%08x  ", pregctx->REG_uiInput[0]);
        fdprintf(iFd, "i1  = 0x%08x\n", pregctx->REG_uiInput[1]);
        fdprintf(iFd, "i2  = 0x%08x  ", pregctx->REG_uiInput[2]);
        fdprintf(iFd, "i3  = 0x%08x\n", pregctx->REG_uiInput[3]);
        fdprintf(iFd, "i4  = 0x%08x  ", pregctx->REG_uiInput[4]);
        fdprintf(iFd, "i5  = 0x%08x\n", pregctx->REG_uiInput[5]);
        fdprintf(iFd, "i6  = 0x%08x  ", pregctx->REG_uiInput[6]);       /*  FP                          */
        fdprintf(iFd, "i7  = 0x%08x\n", pregctx->REG_uiInput[7]);

        fdprintf(iFd, "PSR = 0x%08x  ", pregctx->REG_uiPsr);
        fdprintf(iFd, "Y   = 0x%08x\n", pregctx->REG_uiY);
        fdprintf(iFd, "PC  = 0x%08x  ", pregctx->REG_uiPc);
        fdprintf(iFd, "NPC = 0x%08x\n", pregctx->REG_uiNPc);

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

        stOft = bnprintf(pvBuffer, stSize, stOft, "g0  = 0x%08x  ", pregctx->REG_uiGlobal[0]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "g1  = 0x%08x\n", pregctx->REG_uiGlobal[1]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "g2  = 0x%08x  ", pregctx->REG_uiGlobal[2]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "g3  = 0x%08x\n", pregctx->REG_uiGlobal[3]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "g4  = 0x%08x  ", pregctx->REG_uiGlobal[4]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "g5  = 0x%08x\n", pregctx->REG_uiGlobal[5]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "g6  = 0x%08x  ", pregctx->REG_uiGlobal[6]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "g7  = 0x%08x\n", pregctx->REG_uiGlobal[7]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "o0  = 0x%08x  ", pregctx->REG_uiOutput[0]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "o1  = 0x%08x\n", pregctx->REG_uiOutput[1]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "o2  = 0x%08x  ", pregctx->REG_uiOutput[2]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "o3  = 0x%08x\n", pregctx->REG_uiOutput[3]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "o4  = 0x%08x  ", pregctx->REG_uiOutput[4]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "o5  = 0x%08x\n", pregctx->REG_uiOutput[5]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "o6  = 0x%08x  ", pregctx->REG_uiOutput[6]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "o7  = 0x%08x\n", pregctx->REG_uiOutput[7]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "l0  = 0x%08x  ", pregctx->REG_uiLocal[0]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "l1  = 0x%08x\n", pregctx->REG_uiLocal[1]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "l2  = 0x%08x  ", pregctx->REG_uiLocal[2]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "l3  = 0x%08x\n", pregctx->REG_uiLocal[3]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "l4  = 0x%08x  ", pregctx->REG_uiLocal[4]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "l5  = 0x%08x\n", pregctx->REG_uiLocal[5]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "l6  = 0x%08x  ", pregctx->REG_uiLocal[6]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "l7  = 0x%08x\n", pregctx->REG_uiLocal[7]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "i0  = 0x%08x  ", pregctx->REG_uiInput[0]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "i1  = 0x%08x\n", pregctx->REG_uiInput[1]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "i2  = 0x%08x  ", pregctx->REG_uiInput[2]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "i3  = 0x%08x\n", pregctx->REG_uiInput[3]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "i4  = 0x%08x  ", pregctx->REG_uiInput[4]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "i5  = 0x%08x\n", pregctx->REG_uiInput[5]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "i6  = 0x%08x  ", pregctx->REG_uiInput[6]);
        stOft = bnprintf(pvBuffer, stSize, stOft, "i7  = 0x%08x\n", pregctx->REG_uiInput[7]);

        stOft = bnprintf(pvBuffer, stSize, stOft, "PSR = 0x%08x  ", pregctx->REG_uiPsr);
        stOft = bnprintf(pvBuffer, stSize, stOft, "Y   = 0x%08x\n", pregctx->REG_uiY);
        stOft = bnprintf(pvBuffer, stSize, stOft, "PC  = 0x%08x  ", pregctx->REG_uiPc);
        stOft = bnprintf(pvBuffer, stSize, stOft, "NPC = 0x%08x\n", pregctx->REG_uiNPc);

        stOft = bnprintf(pvBuffer, stSize, stOft, "SP  = 0x%08x\n", (ARCH_REG_T)pstkTop);

    } else {
        _PrintFormat(", pregctx");

        _PrintFormat("g0  = 0x%08x  ",   pregctx->REG_uiGlobal[0]);
        _PrintFormat("g1  = 0x%08x\r\n", pregctx->REG_uiGlobal[1]);
        _PrintFormat("g2  = 0x%08x  ",   pregctx->REG_uiGlobal[2]);
        _PrintFormat("g3  = 0x%08x\r\n", pregctx->REG_uiGlobal[3]);
        _PrintFormat("g4  = 0x%08x  ",   pregctx->REG_uiGlobal[4]);
        _PrintFormat("g5  = 0x%08x\r\n", pregctx->REG_uiGlobal[5]);
        _PrintFormat("g6  = 0x%08x  ",   pregctx->REG_uiGlobal[6]);
        _PrintFormat("g7  = 0x%08x\r\n", pregctx->REG_uiGlobal[7]);

        _PrintFormat("o0  = 0x%08x  ",   pregctx->REG_uiOutput[0]);
        _PrintFormat("o1  = 0x%08x\r\n", pregctx->REG_uiOutput[1]);
        _PrintFormat("o2  = 0x%08x  ",   pregctx->REG_uiOutput[2]);
        _PrintFormat("o3  = 0x%08x\r\n", pregctx->REG_uiOutput[3]);
        _PrintFormat("o4  = 0x%08x  ",   pregctx->REG_uiOutput[4]);
        _PrintFormat("o5  = 0x%08x\r\n", pregctx->REG_uiOutput[5]);
        _PrintFormat("o6  = 0x%08x  ",   pregctx->REG_uiOutput[6]);     /*  异常压栈前的 SP             */
        _PrintFormat("o7  = 0x%08x\r\n", pregctx->REG_uiOutput[7]);

        _PrintFormat("l0  = 0x%08x  ",   pregctx->REG_uiLocal[0]);
        _PrintFormat("l1  = 0x%08x\r\n", pregctx->REG_uiLocal[1]);
        _PrintFormat("l2  = 0x%08x  ",   pregctx->REG_uiLocal[2]);
        _PrintFormat("l3  = 0x%08x\r\n", pregctx->REG_uiLocal[3]);
        _PrintFormat("l4  = 0x%08x  ",   pregctx->REG_uiLocal[4]);
        _PrintFormat("l5  = 0x%08x\r\n", pregctx->REG_uiLocal[5]);
        _PrintFormat("l6  = 0x%08x  ",   pregctx->REG_uiLocal[6]);
        _PrintFormat("l7  = 0x%08x\r\n", pregctx->REG_uiLocal[7]);

        _PrintFormat("i0  = 0x%08x  ",   pregctx->REG_uiInput[0]);
        _PrintFormat("i1  = 0x%08x\r\n", pregctx->REG_uiInput[1]);
        _PrintFormat("i2  = 0x%08x  ",   pregctx->REG_uiInput[2]);
        _PrintFormat("i3  = 0x%08x\r\n", pregctx->REG_uiInput[3]);
        _PrintFormat("i4  = 0x%08x  ",   pregctx->REG_uiInput[4]);
        _PrintFormat("i5  = 0x%08x\r\n", pregctx->REG_uiInput[5]);
        _PrintFormat("i6  = 0x%08x  ",   pregctx->REG_uiInput[6]);      /*  FP                          */
        _PrintFormat("i7  = 0x%08x\r\n", pregctx->REG_uiInput[7]);

        _PrintFormat("PSR = 0x%08x  ",   pregctx->REG_uiPsr);
        _PrintFormat("Y   = 0x%08x\r\n", pregctx->REG_uiY);
        _PrintFormat("PC  = 0x%08x  ",   pregctx->REG_uiPc);
        _PrintFormat("NPC = 0x%08x\r\n", pregctx->REG_uiNPc);

        _PrintFormat("SP  = 0x%08x\r\n", (ARCH_REG_T)pstkTop);          /*  异常压栈后的 SP             */
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

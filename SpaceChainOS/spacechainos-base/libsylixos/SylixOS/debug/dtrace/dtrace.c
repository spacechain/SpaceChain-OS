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
** 文   件   名: dtrace.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 11 月 18 日
**
** 描        述: SylixOS 调试跟踪器, GDB server 可以使用此调试进程.

** BUG:
2012.09.05  今天凌晨, 重新设计 dtrace 的等待与接口机制, 开始为 GDB server 的编写扫平一切障碍.
2014.05.23  内存操作时, 需要首先检测内存访问权限.
2014.08.10  API_DtraceThreadStepSet() 首先对单步断点地址产生一次页面中断.
2014.09.02  增加 API_DtraceDelBreakInfo() 接口, 调试器可删除断点信息.
2015.11.17  修正 SMP 高并发度引起的调试错误.
2015.12.01  加入浮点运算器上下文获取与设置操作.
2016.08.16  支持硬件单步调试.
2016.11.23  在停止所有任务前, 预锁定关键性资源, 仿制被停止线程占用, 导致调试器假死.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
/*********************************************************************************************************
  dtrace 调试
*********************************************************************************************************/
#ifdef __DTRACE_DEBUG
#define __DTRACE_MSG    _PrintFormat
#else
#define __DTRACE_MSG(fmt, ...)
#endif                                                                  /*  __DTRACE_DEBUG              */
/*********************************************************************************************************
  dtrace 结构
*********************************************************************************************************/
typedef struct {
    UINT                DMSG_uiIn;
    UINT                DMSG_uiOut;
    UINT                DMSG_uiNum;
    LW_DTRACE_MSG       DMSG_dmsgBuffer[LW_CFG_MAX_THREADS];
} LW_DTRACE_MSG_POOL;

typedef struct {
    LW_LIST_LINE        DTRACE_lineManage;                              /*  管理链表                    */
    pid_t               DTRACE_pid;                                     /*  进程描述符                  */
    UINT                DTRACE_uiType;                                  /*  调试类型                    */
    UINT                DTRACE_uiFlag;                                  /*  调试选项                    */
    LW_DTRACE_MSG_POOL  DTRACE_dmsgpool;                                /*  调试暂停线程信息            */
    LW_OBJECT_HANDLE    DTRACE_ulDbger;                                 /*  调试器                      */
} LW_DTRACE;
typedef LW_DTRACE      *PLW_DTRACE;

#define DPOOL_IN        pdtrace->DTRACE_dmsgpool.DMSG_uiIn
#define DPOOL_OUT       pdtrace->DTRACE_dmsgpool.DMSG_uiOut
#define DPOOL_NUM       pdtrace->DTRACE_dmsgpool.DMSG_uiNum
#define DPOOL_MSG       pdtrace->DTRACE_dmsgpool.DMSG_dmsgBuffer
/*********************************************************************************************************
  dtrace 结构
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _G_plineDtraceHeader = LW_NULL;
/*********************************************************************************************************
** 函数名称: __dtraceWriteMsg
** 功能描述: 插入一个调试信息
** 输　入  : pdtrace       dtrace 节点
**           dmsg          调试信息
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __dtraceWriteMsg (PLW_DTRACE  pdtrace, const PLW_DTRACE_MSG  pdmsg)
{
    if (DPOOL_NUM == LW_CFG_MAX_THREADS) {
        return  (PX_ERROR);
    }
    
    DPOOL_MSG[DPOOL_IN] = *pdmsg;
    DPOOL_IN++;
    DPOOL_NUM++;
    
    if (DPOOL_IN >= LW_CFG_MAX_THREADS) {
        DPOOL_IN =  0;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __dtraceReadMsg
** 功能描述: 读取一个调试信息
** 输　入  : pdtrace       dtrace 节点
**           dmsg          调试信息
**           bPeek         是否需要读后清理
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __dtraceReadMsg (PLW_DTRACE  pdtrace, PLW_DTRACE_MSG  pdmsg, BOOL  bPeek)
{
    if (DPOOL_NUM == 0) {
        return  (PX_ERROR);
    }
    
    while (DPOOL_MSG[DPOOL_OUT].DTM_ulThread == LW_OBJECT_HANDLE_INVALID) {
        DPOOL_OUT++;
        if (DPOOL_OUT >= LW_CFG_MAX_THREADS) {
            DPOOL_OUT =  0;
        }
    }
    
    *pdmsg = DPOOL_MSG[DPOOL_OUT];
    
    if (bPeek == LW_FALSE) {
        DPOOL_MSG[DPOOL_OUT].DTM_ulThread = LW_OBJECT_HANDLE_INVALID;
        DPOOL_OUT++;
        DPOOL_NUM--;
        
        if (DPOOL_OUT >= LW_CFG_MAX_THREADS) {
            DPOOL_OUT =  0;
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __dtraceReadMsgEx
** 功能描述: 读取一个调试信息
** 输　入  : pdtrace       dtrace 节点
**           dmsg          调试信息
**           bPeek         是否需要读后清理
**           ulThread      指定的线程 ID
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __dtraceReadMsgEx (PLW_DTRACE       pdtrace, 
                               PLW_DTRACE_MSG   pdmsg, 
                               BOOL             bPeek, 
                               LW_OBJECT_HANDLE ulThread)
{
    UINT    uiI = DPOOL_OUT;

    if (DPOOL_NUM == 0) {
        return  (PX_ERROR);
    }
    
    while (DPOOL_MSG[uiI].DTM_ulThread != ulThread) {
        uiI++;
        if (uiI >= LW_CFG_MAX_THREADS) {
            uiI =  0;
        }
        if (uiI == DPOOL_IN) {
            return  (PX_ERROR);                                         /*  没有对应线程信息            */
        }
    }
    
    *pdmsg = DPOOL_MSG[uiI];
    
    if (bPeek == LW_FALSE) {
        DPOOL_MSG[uiI].DTM_ulThread = LW_OBJECT_HANDLE_INVALID;
        DPOOL_NUM--;
        
        if (DPOOL_OUT == uiI) {
            DPOOL_OUT++;
            if (DPOOL_OUT >= LW_CFG_MAX_THREADS) {
                DPOOL_OUT =  0;
            }
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __dtraceMemVerify
** 功能描述: 检测内存属性
** 输　入  : pdtrace       dtrace 节点
**           ulAddr        内存地址
**           bWrite        是否为写, 否则为读
** 输　出  : 是否允许操作
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  __dtraceMemVerify (PVOID  pvDtrace, addr_t  ulAddr, BOOL  bWrite)
{
#if LW_CFG_VMM_EN > 0
    ULONG  ulFlags;
    
    if (API_VmmGetFlag((PVOID)ulAddr, &ulFlags)) {
        return  (LW_FALSE);
    }
    
    if (!(ulFlags & LW_VMM_FLAG_ACCESS)) {
        return  (LW_FALSE);
    }
    
#if (LW_CFG_CACHE_EN > 0) && !defined(LW_CFG_CPU_ARCH_PPC)
    if (!API_VmmVirtualIsInside(ulAddr)) {
        if (!API_CacheAliasProb() &&
            !(ulFlags & LW_VMM_FLAG_CACHEABLE)) {                       /*  暂时不允许读取设备内存      */
            return  (LW_FALSE);
        }
    }
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                                                                        /*  !LW_CFG_CPU_ARCH_PPC        */
    if (bWrite && !(ulFlags & LW_VMM_FLAG_WRITABLE)) {
        return  (LW_FALSE);
    }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    return  (LW_TRUE);
}
/*********************************************************************************************************
** 函数名称: API_DtraceBreakTrap
** 功能描述: 有一个线程触发断点
** 输　入  : ulAddr        触发地址
**           uiBpType      触发类型 (LW_TRAP_INVAL / LW_TRAP_BRKPT / LW_TRAP_ABORT) 
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
** 注  意  : SMP 环境中, 在单步断点被中断时, 立即唤醒 debug 线程, 这时 debug 线程可能在本线程切出之前清除
             单步断点, 所以这里需要提前清除单步断点.
             
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_DtraceBreakTrap (addr_t  ulAddr, UINT  uiBpType)
{
#if LW_CFG_MODULELOADER_EN > 0
    LW_OBJECT_HANDLE    ulDbger;
    PLW_LIST_LINE       plineTemp;
    LW_LD_VPROC        *pvproc;
    PLW_CLASS_TCB       ptcbCur;
    PLW_DTRACE          pdtrace;
    LW_DTRACE_MSG       dtm;
    union sigval        sigvalue;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pvproc = __LW_VP_GET_TCB_PROC(ptcbCur);
    if (!pvproc || (pvproc->VP_pid <= 0) || !_G_plineDtraceHeader) {
        return  (PX_ERROR);
    }
    
#if LW_CFG_SMP_EN > 0
    if (uiBpType == LW_TRAP_RETRY) {
        return  (ERROR_NONE);                                           /*  可能是虚断点, 再尝试一次    */
    }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    dtm.DTM_ulAddr   = ulAddr;
    dtm.DTM_uiType   = uiBpType;                                        /*  获得 trap 类型              */
    dtm.DTM_ulThread = ptcbCur->TCB_ulId;
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    for (plineTemp  = _G_plineDtraceHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  查找对应的调试器            */
        pdtrace = _LIST_ENTRY(plineTemp, LW_DTRACE, DTRACE_lineManage);
        if (pdtrace->DTRACE_pid == pvproc->VP_pid) {
            ulDbger = pdtrace->DTRACE_ulDbger;
            __dtraceWriteMsg(pdtrace, &dtm);
            break;
        }
    }
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    if (plineTemp == LW_NULL) {                                         /*  此进程不存在调试器          */
        return  (PX_ERROR);
    
    } else {
        __DTRACE_MSG("[DTRACE] <KERN> Trap thread 0x%lx @ 0x%08lx CPU %ld.\r\n", 
                     dtm.DTM_ulThread, ulAddr, ptcbCur->TCB_ulCPUId);
        
#if LW_CFG_SMP_EN > 0 && !defined(LW_DTRACE_HW_ISTEP)
        if (ulAddr == ptcbCur->TCB_ulStepAddr) {
        	archDbgBpRemove(ptcbCur->TCB_ulStepAddr, sizeof(addr_t),
        			        ptcbCur->TCB_ulStepInst, LW_FALSE);
            ptcbCur->TCB_bStepClear = LW_TRUE;                          /*  断点被移除                  */
        }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
                                                                        /*  !LW_DTRACE_HW_ISTEP         */
        sigvalue.sival_int = dtm.DTM_uiType;
        sigTrap(ulDbger, sigvalue);                                     /*  通知调试器线程              */
        return  (ERROR_NONE);
    }
#else
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
}
/*********************************************************************************************************
** 函数名称: API_DtraceAbortTrap
** 功能描述: 有一个线程触发非可控异常
** 输　入  : ulAddr        触发地址
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_DtraceAbortTrap (addr_t  ulAddr)
{
#if LW_CFG_MODULELOADER_EN > 0
    ULONG               ulIns;
    PLW_LIST_LINE       plineTemp;
    LW_LD_VPROC        *pvproc;
    PLW_CLASS_TCB       ptcbCur;
    PLW_DTRACE          pdtrace;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pvproc = __LW_VP_GET_TCB_PROC(ptcbCur);
    if (!pvproc || (pvproc->VP_pid <= 0)) {
        return  (PX_ERROR);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    for (plineTemp  = _G_plineDtraceHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  查找对应的调试器            */
        pdtrace = _LIST_ENTRY(plineTemp, LW_DTRACE, DTRACE_lineManage);
        if (pdtrace->DTRACE_pid == pvproc->VP_pid) {
            break;
        }
    }
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    if (plineTemp == LW_NULL) {                                         /*  此进程不存在调试器          */
        return  (PX_ERROR);
    
    } else {
        archDbgAbInsert(ulAddr, &ulIns);                                /*  建立一个断点                */
        return  (ERROR_NONE);
    }
#else
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
}
/*********************************************************************************************************
** 函数名称: API_DtraceChildSig
** 功能描述: 被调试线程通知
** 输　入  : pid           进程号
**           psigevent     信号信息
**           psiginfo      信号信息
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_DtraceChildSig (pid_t pid, struct sigevent *psigevent, struct siginfo *psiginfo)
{
#if LW_CFG_MODULELOADER_EN > 0
    PLW_LIST_LINE       plineTemp;
    PLW_DTRACE          pdtrace;
    LW_OBJECT_HANDLE    ulDbger;
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    for (plineTemp  = _G_plineDtraceHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  查找对应的调试器            */
        pdtrace = _LIST_ENTRY(plineTemp, LW_DTRACE, DTRACE_lineManage);
        if (pdtrace->DTRACE_pid == pid) {
            ulDbger = pdtrace->DTRACE_ulDbger;
            break;
        }
    }
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    if (plineTemp == LW_NULL) {                                         /*  此进程不存在调试器          */
        return  (PX_ERROR);
    
    } else {
        _doSigEventEx(ulDbger, psigevent, psiginfo);                    /*  产生 SIGCHLD 信号           */
        return  (ERROR_NONE);
    }
#else
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
}
/*********************************************************************************************************
** 函数名称: API_DtraceCreate
** 功能描述: 创建一个 dtrace 调试节点
** 输　入  : uiType            调试类型
**           uiFlag            调试选项
**           ulDbger           调试器服务线程
** 输　出  : dtrace 节点
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
PVOID  API_DtraceCreate (UINT  uiType, UINT  uiFlag, LW_OBJECT_HANDLE  ulDbger)
{
    PLW_DTRACE  pdtrace;
    
    pdtrace = (PLW_DTRACE)__SHEAP_ALLOC(sizeof(LW_DTRACE));
    if (pdtrace == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    lib_bzero(pdtrace, sizeof(LW_DTRACE));
    
    pdtrace->DTRACE_pid     = (pid_t)PX_ERROR;
    pdtrace->DTRACE_uiType  = uiType;
    pdtrace->DTRACE_uiFlag  = uiFlag;
    pdtrace->DTRACE_ulDbger = ulDbger;
    
    __KERNEL_ENTER();
    _List_Line_Add_Ahead(&pdtrace->DTRACE_lineManage, &_G_plineDtraceHeader);
    __KERNEL_EXIT();
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "dtrace create.\r\n");
    return  ((PVOID)pdtrace);
}
/*********************************************************************************************************
** 函数名称: API_DtraceDelete
** 功能描述: 删除一个 dtrace 调试节点
** 输　入  : pvDtrace      dtrace 节点
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceDelete (PVOID  pvDtrace)
{
    PLW_DTRACE  pdtrace = (PLW_DTRACE)pvDtrace;
    
    if (!pdtrace) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    __KERNEL_ENTER();
    _List_Line_Del(&pdtrace->DTRACE_lineManage, &_G_plineDtraceHeader);
    __KERNEL_EXIT();
    
    __SHEAP_FREE(pdtrace);
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "dtrace delete.\r\n");
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DtraceIsValid
** 功能描述: 是否存在正在调试的 dtrace 调试节点
** 输　入  : NONE
** 输　出  : 是否存在
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
BOOL  API_DtraceIsValid (VOID)
{
    return  ((_G_plineDtraceHeader) ? (LW_TRUE) : (LW_FALSE));
}
/*********************************************************************************************************
** 函数名称: API_DtraceSetPid
** 功能描述: 设置 dtrace 跟踪进程
** 输　入  : pvDtrace          dtrace 节点
**           pid               被调试进程号
** 输　出  : 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceSetPid (PVOID  pvDtrace, pid_t  pid)
{
    PLW_DTRACE  pdtrace = (PLW_DTRACE)pvDtrace;
    
    if (!pdtrace) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    pdtrace->DTRACE_pid = pid;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DtraceGetRegs
** 功能描述: 获取指定调试线程的寄存器上下文
** 输　入  : pvDtrace      dtrace 节点
**           ulThread      线程句柄
**           pregctx       寄存器表
**           pregSp        堆栈指针
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceGetRegs (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, 
                          ARCH_REG_CTX  *pregctx, ARCH_REG_T *pregSp)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
#if !defined(LW_CFG_CPU_ARCH_C6X)
    REGISTER ARCH_REG_CTX  *pregctxGet;
#endif
    
    PLW_DTRACE  pdtrace = (PLW_DTRACE)pvDtrace;
    
    if (!pdtrace || !pregctx) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    usIndex = _ObjectGetIndex(ulThread);
    
    if (!_ObjectClassOK(ulThread, _OBJECT_THREAD)) {                    /*  检查 ID 类型有效性          */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        return  (ERROR_THREAD_NULL);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
#if !defined(LW_CFG_CPU_ARCH_C6X)
    pregctxGet = archTaskRegsGet(ptcb->TCB_pstkStackNow, pregSp);
    *pregctx   = *pregctxGet;
#else
    archTaskRegsGet(ptcb, (ARCH_REG_IRQ_CTX *)pregctx, pregSp);
#endif
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DtraceSetRegs
** 功能描述: 设置指定调试线程的寄存器上下文
** 输　入  : pvDtrace      dtrace 节点
**           ulThread      线程句柄
**           pregctx       寄存器表
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceSetRegs (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, const ARCH_REG_CTX  *pregctx)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
    
    PLW_DTRACE  pdtrace = (PLW_DTRACE)pvDtrace;
    
    if (!pdtrace || !pregctx) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    usIndex = _ObjectGetIndex(ulThread);
    
    if (!_ObjectClassOK(ulThread, _OBJECT_THREAD)) {                    /*  检查 ID 类型有效性          */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        return  (ERROR_THREAD_NULL);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
#if !defined(LW_CFG_CPU_ARCH_C6X)
    archTaskRegsSet(ptcb->TCB_pstkStackNow, pregctx);
#else
    archTaskRegsSet(ptcb, (const ARCH_REG_IRQ_CTX *)pregctx);
#endif
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DtraceGetFpuRegs
** 功能描述: 获取指定调试线程浮点寄存器上下文
** 输　入  : pvDtrace      dtrace 节点
**           ulThread      线程句柄
**           pfpuctx       浮点寄存器表
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

LW_API 
ULONG  API_DtraceGetFpuRegs (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, ARCH_FPU_CTX  *pfpuctx)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
    
    PLW_DTRACE  pdtrace = (PLW_DTRACE)pvDtrace;
    
    if (!pdtrace || !pfpuctx) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    usIndex = _ObjectGetIndex(ulThread);
    
    if (!_ObjectClassOK(ulThread, _OBJECT_THREAD)) {                    /*  检查 ID 类型有效性          */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        return  (ERROR_THREAD_NULL);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    *pfpuctx = ptcb->TCB_fpuctxContext.FPUCTX_fpuctxContext;
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DtraceSetFpuRegs
** 功能描述: 设置指定调试线程浮点寄存器上下文
** 输　入  : pvDtrace      dtrace 节点
**           ulThread      线程句柄
**           pfpuctx       浮点寄存器表
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceSetFpuRegs (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, const ARCH_FPU_CTX  *pfpuctx)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
    
    PLW_DTRACE  pdtrace = (PLW_DTRACE)pvDtrace;
    
    if (!pdtrace || !pfpuctx) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    usIndex = _ObjectGetIndex(ulThread);
    
    if (!_ObjectClassOK(ulThread, _OBJECT_THREAD)) {                    /*  检查 ID 类型有效性          */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        return  (ERROR_THREAD_NULL);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    ptcb->TCB_fpuctxContext.FPUCTX_fpuctxContext = *pfpuctx;
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
** 函数名称: API_DtraceGetMems
** 功能描述: 拷贝内存
** 输　入  : pvDtrace      dtrace 节点
**           ulAddr        拷贝内存起始地址
**           pvBuffer      接收缓冲
**           stSize        拷贝的大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
** 注  意  : 内存操作长度应在 PAGESIZE 内.
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceGetMems (PVOID  pvDtrace, addr_t  ulAddr, PVOID  pvBuffer, size_t  stSize)
{
    if (!pvBuffer) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    if ((__dtraceMemVerify(pvDtrace, ulAddr,              LW_FALSE) == LW_FALSE) ||
        (__dtraceMemVerify(pvDtrace, ulAddr + stSize - 1, LW_FALSE) == LW_FALSE)) {
        _ErrorHandle(EACCES);
        return  (EACCES);
    }
    
    KN_SMP_MB();
    
    lib_memcpy(pvBuffer, (const PVOID)ulAddr, stSize);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DtraceSetMems
** 功能描述: 写入内存
** 输　入  : pvDtrace      dtrace 节点
**           ulAddr        写入内存起始地址
**           pvBuffer      写入数据
**           stSize        写入的大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
** 注  意  : 内存操作长度应在 PAGESIZE 内. C6x DSP 使用内存断点, 所以需要 cache update

                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceSetMems (PVOID  pvDtrace, addr_t  ulAddr, const PVOID  pvBuffer, size_t  stSize)
{
    if (!pvBuffer) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    if ((__dtraceMemVerify(pvDtrace, ulAddr,              LW_TRUE) == LW_FALSE) ||
        (__dtraceMemVerify(pvDtrace, ulAddr + stSize - 1, LW_TRUE) == LW_FALSE)) {
        _ErrorHandle(EACCES);
        return  (EACCES);
    }
    
    lib_memcpy((PVOID)ulAddr, pvBuffer, stSize);

#if defined(LW_CFG_CPU_ARCH_C6X) && (LW_CFG_CACHE_EN > 0)
    API_CacheTextUpdate((PVOID)ulAddr, stSize);
#endif

    KN_SMP_MB();
    
    __DTRACE_MSG("[DTRACE] <GDB>  Set memory @ 0x%08lx size %zu.\r\n", ulAddr, stSize);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DtraceBreakpointInsert
** 功能描述: 插入一个断点
** 输　入  : pvDtrace      dtrace 节点
**           ulAddr        断点地址
**           stSize        断点长度
**           pulIns        返回断点地址之前的指令
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceBreakpointInsert (PVOID  pvDtrace, addr_t  ulAddr, size_t stSize, ULONG  *pulIns)
{
    PLW_DTRACE    pdtrace = (PLW_DTRACE)pvDtrace;
    
    if (!(pdtrace->DTRACE_uiFlag & LW_DTRACE_F_KBP)) {                  /*  内核断点禁能                */
#if LW_CFG_VMM_EN > 0
        if (!API_VmmVirtualIsInside(ulAddr)) {
            _ErrorHandle(ERROR_KERNEL_MEMORY);
            return  (ERROR_KERNEL_MEMORY);
        }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
    }
    
    archDbgBpInsert(ulAddr, stSize, pulIns, LW_FALSE);
    
    __DTRACE_MSG("[DTRACE] <GDB>  Add Breakpoint @ 0x%08lx.\r\n", ulAddr);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DtraceBreakpointRemove
** 功能描述: 删除一个断点
** 输　入  : pvDtrace      dtrace 节点
**           ulAddr        断点地址
**           stSize        断点长度
**           ulIns         断电之前的指令
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceBreakpointRemove (PVOID  pvDtrace, addr_t  ulAddr, size_t stSize, ULONG  ulIns)
{
    archDbgBpRemove(ulAddr, stSize, ulIns, LW_FALSE);
    
    __DTRACE_MSG("[DTRACE] <GDB>  Remove Breakpoint @ 0x%08lx.\r\n", ulAddr);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DtraceWatchpointInsert
** 功能描述: 创建一个数据观察点
** 输　入  : pvDtrace      dtrace 节点
**           ulAddr        数据地址
**           stSize        长度
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceWatchpointInsert (PVOID  pvDtrace, addr_t  ulAddr, size_t stSize)
{
    _ErrorHandle(ENOSYS);
    return  (ENOSYS);
}
/*********************************************************************************************************
** 函数名称: API_DtraceWatchpointRemove
** 功能描述: 删除一个数据观察点
** 输　入  : pvDtrace      dtrace 节点
**           ulAddr        数据地址
**           stSize        长度
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceWatchpointRemove (PVOID  pvDtrace, addr_t  ulAddr, size_t stSize)
{
    _ErrorHandle(ENOSYS);
    return  (ENOSYS);
}
/*********************************************************************************************************
** 函数名称: API_DtraceStopThread
** 功能描述: 停止一个线程
** 输　入  : pvDtrace      dtrace 节点
**           ulThread      线程句柄
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceStopThread (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread)
{
    return  (API_ThreadStop(ulThread));
}
/*********************************************************************************************************
** 函数名称: API_DtraceContinueThread
** 功能描述: 恢复一个已经被停止的线程
** 输　入  : pvDtrace      dtrace 节点
**           ulThread      线程句柄
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceContinueThread (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread)
{
    return  (API_ThreadContinue(ulThread));
}
/*********************************************************************************************************
** 函数名称: API_DtraceStopProcess
** 功能描述: 停止 dtrace 对应的调试进程
** 输　入  : pvDtrace      dtrace 节点
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceStopProcess (PVOID  pvDtrace)
{
    PLW_DTRACE    pdtrace = (PLW_DTRACE)pvDtrace;
    LW_LD_VPROC  *pvproc  = vprocGet(pdtrace->DTRACE_pid);
    
    if (pvproc) {
        vprocThreadDebugStop(pvproc);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DtraceContinueProcess
** 功能描述: 恢复 dtrace 对应的调试进程
** 输　入  : pvDtrace      dtrace 节点
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceContinueProcess (PVOID  pvDtrace)
{
    PLW_DTRACE    pdtrace = (PLW_DTRACE)pvDtrace;
    LW_LD_VPROC  *pvproc  = vprocGet(pdtrace->DTRACE_pid);
    
    if (pvproc) {
        vprocThreadDebugContinue(pvproc);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DtraceGetBreakInfo
** 功能描述: 获得当前断点线程信息
** 输　入  : pvDtrace      dtrace 节点
**           pdtm          获取的信息
**           ulThread      指定的线程句柄
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceGetBreakInfo (PVOID  pvDtrace, PLW_DTRACE_MSG  pdtm, LW_OBJECT_HANDLE  ulThread)
{
    INT         iError;
    PLW_DTRACE  pdtrace = (PLW_DTRACE)pvDtrace;
    
    if (!pdtrace || !pdtm) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (ulThread == LW_OBJECT_HANDLE_INVALID) {
        iError = __dtraceReadMsg(pdtrace, pdtm, LW_FALSE);
    } else {
        iError = __dtraceReadMsgEx(pdtrace, pdtm, LW_FALSE, ulThread);
    }
    __KERNEL_EXIT();                                                    /*  退出内核                    */

    archDbgBpAdjust(pvDtrace, pdtm);                                    /*  MIPS 需要调整断点位置       */
    
    if (iError) {
        _ErrorHandle(ENOMSG);
        return  (ENOMSG);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DtraceDelBreakInfo
** 功能描述: 删除指定线程的断点信息(仅仅截获下一条断点信息).
** 输　入  : pvDtrace      dtrace 节点
**           ulThread      指定的线程句柄 (必须为有效线程句柄)
**           ulBreakAddr   断点地址 (PX_ERROR 表示删除硬件单步断点)
**           bContinue     移除后是否继续执行线程
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceDelBreakInfo (PVOID             pvDtrace, 
                               LW_OBJECT_HANDLE  ulThread, 
                               addr_t            ulBreakAddr,
                               BOOL              bContinue)
{
    INT             iError  = PX_ERROR;
    PLW_DTRACE      pdtrace = (PLW_DTRACE)pvDtrace;
    LW_DTRACE_MSG   dtm;
    
    if (!pdtrace) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (ulThread) {
        if (__dtraceReadMsgEx(pdtrace, &dtm, LW_TRUE, ulThread) == ERROR_NONE) {
            if ((ulBreakAddr == dtm.DTM_ulAddr) ||
                ((ulBreakAddr == (addr_t)PX_ERROR) && (dtm.DTM_uiType == LW_TRAP_ISTEP))) {
                __dtraceReadMsgEx(pdtrace, &dtm, LW_FALSE, ulThread);
                if (bContinue) {
                    API_ThreadContinue(ulThread);
                }
                iError = ERROR_NONE;
            }
        }
    }
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    if (iError) {
        _ErrorHandle(ENOMSG);
        return  (ENOMSG);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DtraceProcessThread
** 功能描述: 获得进程内所有线程的句柄
** 输　入  : pvDtrace      dtrace 节点
**           ulThread      线程句柄列表缓冲
**           uiTableNum    列表大小
**           puiThreadNum  实际获取的线程数目
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceProcessThread (PVOID  pvDtrace, LW_OBJECT_HANDLE ulThread[], 
                                UINT   uiTableNum, UINT *puiThreadNum)
{
    PLW_DTRACE    pdtrace = (PLW_DTRACE)pvDtrace;
    LW_LD_VPROC  *pvproc  = vprocGet(pdtrace->DTRACE_pid);
    
    if (!ulThread || !uiTableNum || !puiThreadNum) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    if (pvproc) {
        *puiThreadNum = vprocThreadDebugGet(pvproc, ulThread, uiTableNum);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DtraceThreadExtraInfo
** 功能描述: 获得线程额外信息
** 输　入  : pvDtrace      dtrace 节点
**           ulThread      线程句柄
**           pcExtraInfo   额外信息缓存
**           siSize        缓存大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceThreadExtraInfo (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread,
                                  PCHAR  pcExtraInfo, size_t  stSize)
{
    ULONG              ulError;
    LW_CLASS_TCB_DESC  tcbdesc;
    
    PCHAR              pcPendType = LW_NULL;
    PCHAR              pcFpu      = LW_NULL;
    size_t             stFreeByteSize = 0;
    
    if (!pcExtraInfo || !stSize) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    ulError = API_ThreadDesc(ulThread, &tcbdesc);
    if (ulError) {
        return  (ulError);
    }
    
    API_ThreadStackCheck(ulThread, &stFreeByteSize, LW_NULL, LW_NULL);
    
    if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_SEM) {                 /*  等待信号量                  */
        pcPendType = "SEM";
    
    } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_MSGQUEUE) {     /*  等待消息队列                */
        pcPendType = "MSGQ";
    
    } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_JOIN) {         /*  等待其他线程                */
        pcPendType = "JOIN";
        
    } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_SUSPEND) {      /*  挂起                        */
        pcPendType = "SUSP";
    
    } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_EVENTSET) {     /*  等待事件组                  */
        pcPendType = "ENTS";
    
    } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_SIGNAL) {       /*  等待信号                    */
        pcPendType = "WSIG";
    
    } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_INIT) {         /*  初始化中                    */
        pcPendType = "INIT";
    
    } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_WDEATH) {       /*  僵死状态                    */
        pcPendType = "WDEA";
    
    } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_DELAY) {        /*  睡眠                        */
        pcPendType = "SLP";
    
    } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_WSTAT) {        /*  等待状态转换                */
        pcPendType = "WSTAT";
    
    } else {
        pcPendType = "RDY";                                             /*  就绪态                      */
    }
    
    if (tcbdesc.TCBD_ulOption & LW_OPTION_THREAD_USED_FP) {
        pcFpu = "USE";
    } else {
        pcFpu = "NO";
    }
    
    snprintf(pcExtraInfo, stSize, "%s,prio:%d,stat:%s,errno:%ld,wake:%ld,fpu:%s,cpu:%ld,stackfree:%zd",
             tcbdesc.TCBD_cThreadName,
             tcbdesc.TCBD_ucPriority,
             pcPendType,
             tcbdesc.TCBD_ulLastError,
             tcbdesc.TCBD_ulWakeupLeft,
             pcFpu,
             tcbdesc.TCBD_ulCPUId,
             stFreeByteSize);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DtraceThreadStepSet
** 功能描述: 设置线程单步断点地址
** 输　入  : pvDtrace      dtrace 节点
**           ulThread      线程句柄
**           ulAddr        单步断点地址，PX_ERROR 表示禁用单步
** 输　出  : ERROR
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
#ifndef LW_DTRACE_HW_ISTEP

LW_API
ULONG  API_DtraceThreadStepSet (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, addr_t  ulAddr)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
             PLW_DTRACE     pdtrace = (PLW_DTRACE)pvDtrace;

    if (!pdtrace) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }

    if (ulAddr != (addr_t)PX_ERROR) {
        archDbgBpPrefetch(ulAddr);                                      /*  预先产生相关页面中断        */
    }

    usIndex = _ObjectGetIndex(ulThread);

    if (!_ObjectClassOK(ulThread, _OBJECT_THREAD)) {                    /*  检查 ID 类型有效性          */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }

    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        return  (ERROR_THREAD_NULL);
    }

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        return  (ERROR_THREAD_NULL);
    }

    ptcb = _K_ptcbTCBIdTable[usIndex];

#ifdef __DTRACE_DEBUG
    if (ulAddr == (addr_t)PX_ERROR) {
        __DTRACE_MSG("[DTRACE] <GDB>  Pre-Clear thread 0x%lx Step-Breakpoint @ 0x%08lx.\r\n", 
                     ulThread, ptcb->TCB_ulStepAddr);
    
    } else {
        __DTRACE_MSG("[DTRACE] <GDB>  Pre-Set thread 0x%lx Step-Breakpoint @ 0x%08lx.\r\n", 
                     ulThread, ulAddr);
    }
#endif                                                                  /*  __DTRACE_DEBUG              */

    ptcb->TCB_ulStepAddr = ulAddr;                                      /*  设置单步断点地址            */
    ptcb->TCB_bStepClear = LW_TRUE;                                     /*  此断点还未生效              */
    __KERNEL_EXIT();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DtraceThreadStepGet
** 功能描述: 获取线程单步断点地址
** 输　入  : pvDtrace      dtrace 节点
**           ulThread      线程句柄
** 输　出  : pulAddr       单步断点地址
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
ULONG  API_DtraceThreadStepGet (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, addr_t  *pulAddr)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
             PLW_DTRACE     pdtrace = (PLW_DTRACE)pvDtrace;

    if (!pdtrace || !pulAddr) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }

    usIndex = _ObjectGetIndex(ulThread);

    if (!_ObjectClassOK(ulThread, _OBJECT_THREAD)) {                    /*  检查 ID 类型有效性          */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }

    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        return  (ERROR_THREAD_NULL);
    }

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        return  (ERROR_THREAD_NULL);
    }

    ptcb = _K_ptcbTCBIdTable[usIndex];

    *pulAddr = ptcb->TCB_ulStepAddr;                                    /*  返回单步断点地址            */
    __KERNEL_EXIT();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DtraceSchedHook
** 功能描述: 线程切换HOOK函数，用于切换断点信息
**           ulThreadOld      切出线程
**           ulThreadNew      切入线程
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 由于在任务切换 HOOK 中执行, 这里只处理本地 I-CACHE, 其他核通过判断虚断点生效 I-CACHE.

                                           API 函数
*********************************************************************************************************/
LW_API
VOID  API_DtraceSchedHook (LW_OBJECT_HANDLE  ulThreadOld, LW_OBJECT_HANDLE  ulThreadNew)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;

    usIndex = _ObjectGetIndex(ulThreadOld);
    ptcb    = __GET_TCB_FROM_INDEX(usIndex);
    if (ptcb && (ptcb->TCB_ulStepAddr != (addr_t)PX_ERROR) && !ptcb->TCB_bStepClear) {
        archDbgBpRemove(ptcb->TCB_ulStepAddr, sizeof(addr_t), 
                        ptcb->TCB_ulStepInst, (LW_CFG_GDB_SMP_TU_LAZY) ? LW_TRUE : LW_FALSE);
        ptcb->TCB_bStepClear = LW_TRUE;                                 /*  单步断点已经被清除          */
        __DTRACE_MSG("[DTRACE] <HOOK> Clear thread 0x%lx Step-Breakpoint @ 0x%08lx CPU %ld.\r\n", 
                     ulThreadOld, ptcb->TCB_ulStepAddr, LW_CPU_GET_CUR_ID());
    }

    usIndex = _ObjectGetIndex(ulThreadNew);
    ptcb    = __GET_TCB_FROM_INDEX(usIndex);
    if ((ptcb->TCB_ulStepAddr != (addr_t)PX_ERROR) && ptcb->TCB_bStepClear) {
        archDbgBpInsert(ptcb->TCB_ulStepAddr, sizeof(addr_t), 
                        &ptcb->TCB_ulStepInst, LW_TRUE);                /*  仅更新本地 I-CACHE          */
        ptcb->TCB_bStepClear = LW_FALSE;                                /*  单步断点生效                */
        __DTRACE_MSG("[DTRACE] <HOOK> Set thread 0x%lx Step-Breakpoint @ 0x%08lx CPU %ld.\r\n", 
                     ulThreadNew, ptcb->TCB_ulStepAddr, LW_CPU_GET_CUR_ID());
    }
}
/*********************************************************************************************************
** 函数名称: API_DtraceThreadStepSet
** 功能描述: 设置线程单步断点模式
** 输　入  : pvDtrace      dtrace 节点
**           ulThread      线程句柄
**           bEnable       是否使能硬单步断点
** 输　出  : ERROR
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
#else

LW_API
ULONG  API_DtraceThreadStepSet (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, BOOL  bEnable)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
             PLW_DTRACE     pdtrace = (PLW_DTRACE)pvDtrace;
             ARCH_REG_CTX  *pregctx;
             ARCH_REG_T     regSp;

    if (!pdtrace) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }

    usIndex = _ObjectGetIndex(ulThread);

    if (!_ObjectClassOK(ulThread, _OBJECT_THREAD)) {                    /*  检查 ID 类型有效性          */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }

    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        return  (ERROR_THREAD_NULL);
    }

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    pregctx = archTaskRegsGet(ptcb->TCB_pstkStackNow, &regSp);
    archDbgSetStepMode(pregctx, bEnable);
    __KERNEL_EXIT();

    return  (ERROR_NONE);
}

#endif                                                                  /*  !LW_DTRACE_HW_ISTEP         */
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/

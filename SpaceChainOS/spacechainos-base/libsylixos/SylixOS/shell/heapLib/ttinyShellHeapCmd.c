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
** 文   件   名: ttinyShellHeapCmd.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 11 月 23 日
**
** 描        述: 系统堆内部命令定义. (主要用于检查内存泄露)

** BUG:
2010.08.03  锁定内核和关闭中断同时进行.
2010.08.19  加入内存开配大小, 内存开辟时间的信息.
2011.03.06  加入开辟内存用途信息.
            在释放算法中加入处理, 可以正确处理 realloc 信息. (取消 hash 表查找, 释放时, 需要遍历)
2011.03.06  修正 gcc 4.5.1 相关 warning.
2012.03.08  最大分段数通过 start 参数自动分配. 这样可以更灵活的跟踪内存泄露.
2012.12.13  修正 __heapFreePrint 对 realloc 错误跟踪的问题.
2013.04.11  加入跟踪选项, 可以跟踪内核, 跟踪指定进程和全部跟踪.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_SHELL_EN > 0) && (LW_CFG_SHELL_HEAP_TRACE_EN > 0)
/*********************************************************************************************************
  内存 trace 控制块
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE             HTN_plineManage;                           /*  hash 链表                   */
    CHAR                     HTN_cThreadName[LW_CFG_OBJECT_NAME_SIZE];  /*  操作线程名                  */
    CHAR                     HTN_cHeapName[LW_CFG_OBJECT_NAME_SIZE];    /*  内存堆名                    */
    CHAR                     HTN_cPurpose[LW_CFG_OBJECT_NAME_SIZE];     /*  开辟内存的用途              */
    PVOID                    HTN_pvMemAddr;                             /*  内存地址                    */
    size_t                   HTN_stMemLen;                              /*  内存大小                    */
    time_t                   HTN_timeAlloc;                             /*  申请内存时间                */
} __HEAP_TRACE_NODE;
typedef __HEAP_TRACE_NODE   *__PHEAP_TRACE_NODE;
/*********************************************************************************************************
  TRACE 回调
*********************************************************************************************************/
extern VOIDFUNCPTR           _K_pfuncHeapTraceAlloc;
extern VOIDFUNCPTR           _K_pfuncHeapTraceFree;
/*********************************************************************************************************
  TRACE 控制
*********************************************************************************************************/
static atomic_t              _G_atomicHeapTraceEn;
static pid_t                 _G_pidTraceProcess;                        /*  跟踪进程选项                */
/*********************************************************************************************************
  TRACE 缓冲
*********************************************************************************************************/
static __HEAP_TRACE_NODE    *_G_phtnNodeBuffer;
static LW_LIST_LINE_HEADER   _G_plineHeapTraceHeader = LW_NULL;
/*********************************************************************************************************
  TRACE 所需内核对象
*********************************************************************************************************/
static LW_OBJECT_HANDLE      _G_ulHeapTracePart;
static LW_OBJECT_HANDLE      _G_ulHeapTraceLock;
/*********************************************************************************************************
  TRACE 锁操作
*********************************************************************************************************/
#define __HEAP_TRACE_LOCK()     API_SemaphoreMPend(_G_ulHeapTraceLock, LW_OPTION_WAIT_INFINITE)
#define __HEAP_TRACE_UNLOCK()   API_SemaphoreMPost(_G_ulHeapTraceLock)
/*********************************************************************************************************
** 函数名称: __heapAllocPrint
** 功能描述: 处理分配内存信息
** 输　入  : pheap         内存堆控制块
**           pvAddr        分配的内存指针
**           stMemLen      内存大小
**           pcPurpose     内存用途
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __heapAllocPrint (PLW_CLASS_HEAP  pheap, 
                               PVOID           pvAddr, 
                               size_t          stMemLen, 
                               CPCHAR          pcPurpose)
{
    __PHEAP_TRACE_NODE      phtn;
    
    if (!pvAddr) {
        return;
    }
    
    if (_G_pidTraceProcess == 0) {                                      /*  仅跟踪内核内存              */
        if ((pheap != _K_pheapKernel) && (pheap != _K_pheapSystem)) {
            return;
        }
    
    } else if (_G_pidTraceProcess > 0) {                                /*  进跟踪指定进程              */
        if (_G_pidTraceProcess != __PROC_GET_PID_CUR()) {
            return;
        }
        if ((pheap == _K_pheapKernel) || (pheap == _K_pheapSystem)) {   /*  不跟踪                      */
            return;
        }
    }
    
    phtn = (__PHEAP_TRACE_NODE)API_PartitionGet(_G_ulHeapTracePart);
    if (phtn == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "heap trace buffer low.\r\n");
        return;
    }
    
    API_ThreadGetName(API_ThreadIdSelf(), phtn->HTN_cThreadName);
    lib_strlcpy(phtn->HTN_cHeapName, pheap->HEAP_cHeapName, LW_CFG_OBJECT_NAME_SIZE);
    lib_strlcpy(phtn->HTN_cPurpose, pcPurpose, LW_CFG_OBJECT_NAME_SIZE);
    phtn->HTN_pvMemAddr = pvAddr;
    phtn->HTN_stMemLen  = stMemLen;
    phtn->HTN_timeAlloc = lib_time(LW_NULL);
    
    __HEAP_TRACE_LOCK();
    _List_Line_Add_Ahead(&phtn->HTN_plineManage, &_G_plineHeapTraceHeader);
    __HEAP_TRACE_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: __heapFreePrint
** 功能描述: 处理释放内存信息
** 输　入  : pheap         内存堆控制块
**           pvAddr        释放的内存指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __heapFreePrint (PLW_CLASS_HEAP  pheap, PVOID  pvAddr)
{
    BOOL                    bPut = LW_FALSE;
    PLW_LIST_LINE           plineTemp;
    __PHEAP_TRACE_NODE      phtn   = LW_NULL;
    REGISTER PCHAR          pcAddr = (PCHAR)pvAddr;
    
    if (!pvAddr) {
        return;
    }
    
    __HEAP_TRACE_LOCK();
    for (plineTemp  = _G_plineHeapTraceHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        phtn = (__PHEAP_TRACE_NODE)plineTemp;
        if (pcAddr == (PCHAR)phtn->HTN_pvMemAddr) {
            _List_Line_Del(&phtn->HTN_plineManage, 
                           &_G_plineHeapTraceHeader);                   /*  hash 表中删除               */
            bPut = LW_TRUE;
            break;
        
        } else if ((pcAddr >= (PCHAR)phtn->HTN_pvMemAddr) &&
                   ((pcAddr - (PCHAR)phtn->HTN_pvMemAddr) < phtn->HTN_stMemLen)) {
            phtn->HTN_stMemLen = (size_t)(pcAddr - (PCHAR)phtn->HTN_pvMemAddr);
            break;                                                      /*  处理 realloc 新分片情况     */
        }
    }
    __HEAP_TRACE_UNLOCK();
    
    if (bPut) {
        API_PartitionPut(_G_ulHeapTracePart, (PVOID)phtn);              /*  释放跟踪内存块              */
    }
}
/*********************************************************************************************************
** 函数名称: __heapTracePrintResult
** 功能描述: 打印内存堆跟踪结果信息
** 输　入  : bIsNeedDel        是否需要删除
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __heapTracePrintResult (BOOL  bIsNeedDel)
{
#if LW_CFG_CPU_WORD_LENGHT == 64
    static PCHAR            pcHeapTraceHdr = \
    "     HEAP          THREAD               TIME                 ADDR          SIZE           PURPOSE\n"
    "-------------- -------------- ------------------------ ---------------- ---------- ----------------------\n";
#else
    static PCHAR            pcHeapTraceHdr = \
    "     HEAP          THREAD               TIME             ADDR      SIZE           PURPOSE\n"
    "-------------- -------------- ------------------------ -------- ---------- ----------------------\n";
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */

    INT                     iCount  = 0;
    size_t                  stTotal = 0;
    PLW_LIST_LINE           plineTemp;
    __PHEAP_TRACE_NODE      phtn;
    CHAR                    cTimeBuffer[32];
    PCHAR                   pcN;
    
    printf(pcHeapTraceHdr);
    
    __HEAP_TRACE_LOCK();
    plineTemp = _G_plineHeapTraceHeader;
    while (plineTemp) {
        phtn      = (__PHEAP_TRACE_NODE)plineTemp;
        plineTemp = _list_line_get_next(plineTemp);
        
        lib_ctime_r(&phtn->HTN_timeAlloc, cTimeBuffer);
        pcN = lib_index(cTimeBuffer, '\n');
        if (pcN) {
            *pcN = PX_EOS;                                              /*  去掉 \n                     */
        }
        
#if LW_CFG_CPU_WORD_LENGHT == 64
        printf("%-14s %-14s %-24s %16lx %10zd %s\n",
#else
        printf("%-14s %-14s %-24s %08lx %10zd %s\n",
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT adj  */
               phtn->HTN_cHeapName,
               phtn->HTN_cThreadName,
               cTimeBuffer,
               (addr_t)phtn->HTN_pvMemAddr,
               phtn->HTN_stMemLen,
               phtn->HTN_cPurpose);
               
        stTotal += phtn->HTN_stMemLen;
        iCount++;
               
        if (bIsNeedDel) {
            _List_Line_Del(&phtn->HTN_plineManage, 
                           &_G_plineHeapTraceHeader);                   /*  删除                        */
            API_PartitionPut(_G_ulHeapTracePart, (PVOID)phtn);          /*  释放跟踪内存块              */
        }
    }
    __HEAP_TRACE_UNLOCK();
    
    if (iCount == 0) {
        printf("no memory heap leak.\n");                               /*  没有内存泄露                */
    } else {
        printf("\ntotal unfree segment: %d size: %zu\n", iCount, stTotal);
    }
    
    fflush(stdout);                                                     /*  清空输出                    */
}
/*********************************************************************************************************
** 函数名称: __tshellHeapCmdLeakChkStart
** 功能描述: 系统命令 "leakchkstart"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellHeapCmdLeakChkStart (INT  iArgC, PCHAR  ppcArgV[])
{
    INTREG          iregInterLevel;
    INT             iCount;
    
    if (iArgC < 2) {
        fprintf(stderr, "argument error.\n");
        return  (PX_ERROR);
    }
    
    iCount = lib_atoi(ppcArgV[1]);
    if (iCount < 1024) {
        fprintf(stderr, "option error. (min:1024)\n");
        return  (PX_ERROR);
    }
    
    if (iArgC > 2) {
        _G_pidTraceProcess = lib_atoi(ppcArgV[2]);                      /*  设置跟踪选项                */
    } else {
        _G_pidTraceProcess = 0;
    }
    
    if (API_AtomicInc(&_G_atomicHeapTraceEn) == 1) {
        
        if (_G_phtnNodeBuffer) {
            __SHEAP_FREE(_G_phtnNodeBuffer);
        }
        _G_phtnNodeBuffer = 
            (__HEAP_TRACE_NODE *)__SHEAP_ALLOC(sizeof(__HEAP_TRACE_NODE) * (size_t)iCount);
        if (_G_phtnNodeBuffer == LW_NULL) {
            API_AtomicDec(&_G_atomicHeapTraceEn);
            fprintf(stderr, "system low memory.\n");
            return  (PX_ERROR);
        }
        _G_ulHeapTracePart = API_PartitionCreate("heap_trace", (PVOID)_G_phtnNodeBuffer,
                                             (ULONG)iCount,
                                             sizeof(__HEAP_TRACE_NODE),
                                             LW_OPTION_OBJECT_GLOBAL,
                                             LW_NULL);
        if (_G_ulHeapTracePart == LW_OBJECT_HANDLE_INVALID) {
            API_AtomicDec(&_G_atomicHeapTraceEn);
            fprintf(stderr, "can not create heap_trace.\n");
            __SHEAP_FREE(_G_phtnNodeBuffer);
            return  (PX_ERROR);                                         /*  无法创建跟踪缓存            */
        }
        
        /*
         *  安装跟踪回调
         */
        iregInterLevel = __KERNEL_ENTER_IRQ();                          /*  需要锁定其他核调度          */
        _K_pfuncHeapTraceAlloc = __heapAllocPrint;
        _K_pfuncHeapTraceFree  = __heapFreePrint;
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  释放内核                    */
        
        printf("leakcheck start checking...\n");
    
    } else {
        API_AtomicDec(&_G_atomicHeapTraceEn);
        printf("leakcheck already start.\n");
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellHeapCmdLeakChkStop
** 功能描述: 系统命令 "leakchkstop"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellHeapCmdLeakChkStop (INT  iArgC, PCHAR  ppcArgV[])
{
    INTREG          iregInterLevel;
    
    if (API_AtomicGet(&_G_atomicHeapTraceEn) == 1) {
        API_AtomicDec(&_G_atomicHeapTraceEn);
        
        /*
         *  卸载跟踪回调
         */
        iregInterLevel = __KERNEL_ENTER_IRQ();                          /*  需要锁定其他核调度          */
        _K_pfuncHeapTraceAlloc = LW_NULL;
        _K_pfuncHeapTraceFree  = LW_NULL;
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  释放内核                    */
        
        __heapTracePrintResult(LW_TRUE);                                /*  打印跟踪信息, 并删除        */
        
        API_PartitionDelete(&_G_ulHeapTracePart);
        if (_G_phtnNodeBuffer) {
            __SHEAP_FREE(_G_phtnNodeBuffer);
            _G_phtnNodeBuffer = LW_NULL;
        }
        
    } else {
        printf("leakcheck not start.\n");
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellHeapCmdLeakChk
** 功能描述: 系统命令 "leakchk"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellHeapCmdLeakChk (INT  iArgC, PCHAR  ppcArgV[])
{
    if (_K_pfuncHeapTraceAlloc != __heapAllocPrint) {
        printf("leakcheck not start.\n");
        return  (PX_ERROR);
    }

    __heapTracePrintResult(LW_FALSE);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellHeapCmdInit
** 功能描述: 初始化文件系统命令集
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __tshellHeapCmdInit (VOID)
{
    API_AtomicSet(0, &_G_atomicHeapTraceEn);
    
    _G_ulHeapTraceLock = API_SemaphoreMCreate("heap_trace_lock", LW_PRIO_DEF_CEILING, 
                                              LW_OPTION_WAIT_PRIORITY |
                                              LW_OPTION_INHERIT_PRIORITY | 
                                              LW_OPTION_DELETE_SAFE |
                                              LW_OPTION_OBJECT_GLOBAL, LW_NULL);
                                                                        /*  必须为 FIFO 等待            */
    if (_G_ulHeapTraceLock == LW_OBJECT_HANDLE_INVALID) {
        API_PartitionDelete(&_G_ulHeapTracePart);
        return;                                                         /*  无法创建跟踪锁              */
    }

    API_TShellKeywordAdd("leakchkstart", __tshellHeapCmdLeakChkStart);
    API_TShellFormatAdd("leakchkstart", " [max save node number] [pid]");
    API_TShellHelpAdd("leakchkstart", "start heap leak check. (time info is UTC),\n"
                                      "arg1 : max traced node number.\n"
                                      "arg2 : pid info. default zero.\n"
                                      "       <  0 : all.\n"
                                      "       == 0 : kernel.\n"
                                      "       >  0 : whose process ID is equal to the value of pid.\n");
    
    API_TShellKeywordAdd("leakchkstop", __tshellHeapCmdLeakChkStop);
    API_TShellHelpAdd("leakchkstop", "stop heap leak check and print leak message.\n");
    
    API_TShellKeywordAdd("leakchk", __tshellHeapCmdLeakChk);
    API_TShellHelpAdd("leakchk", "print memory leak tracer message.\n");
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
                                                                        /*  LW_CFG_SHELL_HEAP_TRACE_EN  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

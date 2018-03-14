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
** 文   件   名: procKernel.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 03 日
**
** 描        述: proc 文件系统 kernel 信息文件.

** BUG:
2010.01.07  修改一些 errno.
2010.08.11  简化了 read 操作.
2011.02.17  就在 2.14 日, 罗纳尔多的退役了, 非常伤心, 他是我心中真正的一代球王! 世上只有一个罗纳尔多!
            tick 文件中使用 64bit 系统时间.
2011.03.04  proc 的文件为 S_IFREG mode.
2012.08.26  加入 Version 信息.
2013.01.23  查看系统版本时, 需要打印编译时间.
2013.03.31  加入 GCC 版本显示.
2013.11.14  加入内核对象使用信息文件.
2014.11.21  加入 SMP 调度亲和度信息显示.
2016.04.07  加入 cmdline 文件.
2017.02.27  tick 文件加入更详细内容.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/mpi/include/mpi_mpi.h"
#include "../procFs.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_PROCFS_EN > 0) && (LW_CFG_PROCFS_KERNEL_INFO > 0)
/*********************************************************************************************************
  进程相关
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  内核 proc 文件函数声明
*********************************************************************************************************/
static ssize_t  __procFsKernelVersionRead(PLW_PROCFS_NODE  p_pfsn, 
                                          PCHAR            pcBuffer, 
                                          size_t           stMaxBytes,
                                          off_t            oft);
static ssize_t  __procFsKernelCmdlineRead(PLW_PROCFS_NODE  p_pfsn, 
                                          PCHAR            pcBuffer, 
                                          size_t           stMaxBytes,
                                          off_t            oft);
static ssize_t  __procFsKernelTickRead(PLW_PROCFS_NODE  p_pfsn, 
                                       PCHAR            pcBuffer, 
                                       size_t           stMaxBytes,
                                       off_t            oft);
static ssize_t  __procFsKernelObjectsRead(PLW_PROCFS_NODE  p_pfsn, 
                                          PCHAR            pcBuffer, 
                                          size_t           stMaxBytes,
                                          off_t            oft);
                                          
#if LW_CFG_SMP_EN > 0
static ssize_t  __procFsKernelSmpRead(PLW_PROCFS_NODE  p_pfsn, 
                                      PCHAR            pcBuffer, 
                                      size_t           stMaxBytes,
                                      off_t            oft);
static ssize_t  __procFsKernelAffinityRead(PLW_PROCFS_NODE  p_pfsn, 
                                           PCHAR            pcBuffer, 
                                           size_t           stMaxBytes,
                                           off_t            oft);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  内核 proc 文件操作函数组
*********************************************************************************************************/
static LW_PROCFS_NODE_OP        _G_pfsnoKernelVersionFuncs = {
    __procFsKernelVersionRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoKernelCmdlineFuncs = {
    __procFsKernelCmdlineRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoKernelTickFuncs = {
    __procFsKernelTickRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoKernelObjectsFuncs = {
    __procFsKernelObjectsRead, LW_NULL
};
#if LW_CFG_SMP_EN > 0
static LW_PROCFS_NODE_OP        _G_pfsnoKernelSmpFuncs = {
    __procFsKernelSmpRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoKernelAffinityFuncs = {
    __procFsKernelAffinityRead, LW_NULL
};
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  内核 proc 文件目录树
*********************************************************************************************************/
#define __PROCFS_BUFFER_SIZE_VERSION    512
#define __PROCFS_BUFFER_SIZE_CMDLINE    1024
#define __PROCFS_BUFFER_SIZE_TICK       256                             /*  tick 文件需要的缓冲大小     */
#define __PROCFS_BUFFER_SIZE_OBJECTS    1024

static LW_PROCFS_NODE           _G_pfsnKernel[] = 
{
    LW_PROCFS_INIT_NODE("kernel",  
                        (S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH), 
                        LW_NULL, 
                        LW_NULL,  
                        0),
                        
    LW_PROCFS_INIT_NODE("version", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoKernelVersionFuncs, 
                        "V",
                        __PROCFS_BUFFER_SIZE_VERSION),
                        
    LW_PROCFS_INIT_NODE("cmdline", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoKernelCmdlineFuncs, 
                        "P",
                        __PROCFS_BUFFER_SIZE_CMDLINE),
                        
    LW_PROCFS_INIT_NODE("tick", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoKernelTickFuncs, 
                        "T",
                        __PROCFS_BUFFER_SIZE_TICK),
                        
    LW_PROCFS_INIT_NODE("objects", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoKernelObjectsFuncs, 
                        "O",
                        __PROCFS_BUFFER_SIZE_OBJECTS),

#if LW_CFG_SMP_EN > 0
    LW_PROCFS_INIT_NODE("smp", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoKernelSmpFuncs, 
                        "S",
                        0),
                        
    LW_PROCFS_INIT_NODE("affinity", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoKernelAffinityFuncs, 
                        "A",
                        0),
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
};
/*********************************************************************************************************
** 函数名称: __procFsKernelVersionRead
** 功能描述: procfs 读一个内核 version proc 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsKernelVersionRead (PLW_PROCFS_NODE  p_pfsn, 
                                           PCHAR            pcBuffer, 
                                           size_t           stMaxBytes,
                                           off_t            oft)
{
    REGISTER PCHAR      pcFileBuffer;
             size_t     stRealSize;                                     /*  实际的文件内容大小          */
             size_t     stCopeBytes;

    /*
     *  程序运行到这里, 文件缓冲一定已经分配了预置的内存大小(创建节点时预置大小为 64 字节).
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  需要生成文件                */
        stRealSize = bnprintf(pcFileBuffer, 
                              __PROCFS_BUFFER_SIZE_VERSION, 0,
                              "%s %s\n(compile time: %s %s)\n",
                              __SYLIXOS_VERINFO, 
                              bspInfoVersion(),
                              __DATE__, __TIME__);
#ifdef __GNUC__
        stRealSize = bnprintf(pcFileBuffer, 
                              __PROCFS_BUFFER_SIZE_VERSION, stRealSize,
                              "GCC:%d.%d.%d\n",
                              __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif                                                                  /*  __GNUC__                    */
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  计算实际拷贝的字节数        */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** 函数名称: __procFsKernelCmdlineRead
** 功能描述: procfs 读一个内核 cmdline proc 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsKernelCmdlineRead (PLW_PROCFS_NODE  p_pfsn, 
                                           PCHAR            pcBuffer, 
                                           size_t           stMaxBytes,
                                           off_t            oft)
{
    REGISTER PCHAR      pcFileBuffer;
             size_t     stRealSize;                                     /*  实际的文件内容大小          */
             size_t     stCopeBytes;

    /*
     *  程序运行到这里, 文件缓冲一定已经分配了预置的内存大小(创建节点时预置大小为 64 字节).
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  需要生成文件                */
        stRealSize = (size_t)API_KernelStartParamGet(pcFileBuffer, 
                                                     __PROCFS_BUFFER_SIZE_CMDLINE);
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  计算实际拷贝的字节数        */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** 函数名称: __procFsKernelTickRead
** 功能描述: procfs 读一个内核 tick proc 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsKernelTickRead (PLW_PROCFS_NODE  p_pfsn, 
                                        PCHAR            pcBuffer, 
                                        size_t           stMaxBytes,
                                        off_t            oft)
{
    REGISTER PCHAR      pcFileBuffer;
             size_t     stRealSize;                                     /*  实际的文件内容大小          */
             size_t     stCopeBytes;

    /*
     *  程序运行到这里, 文件缓冲一定已经分配了预置的内存大小(创建节点时预置大小为 64 字节).
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
     
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  需要生成文件                */
        struct timespec  tsMono, tsRealtime;
        struct tm        tm;
        CHAR             cRealtimeUtc[32];
        CHAR             cRealtimeLcl[32];
        
        lib_clock_gettime(CLOCK_MONOTONIC, &tsMono);
        lib_clock_gettime(CLOCK_REALTIME,  &tsRealtime);
        
        lib_gmtime_r(&tsRealtime.tv_sec, &tm);
        lib_asctime_r(&tm, cRealtimeUtc);
        
        lib_localtime_r(&tsRealtime.tv_sec, &tm);
        lib_asctime_r(&tm, cRealtimeLcl);
        
        stRealSize = bnprintf(pcFileBuffer, 
                              __PROCFS_BUFFER_SIZE_TICK, 0,
                              "tick rate    : %ld hz\n"
                              "tick         : %lld\n"                   /*  使用 64bit 系统时间         */
                              "monotonic    : %d:%02d:%02d.%09ld\n"
                              "realtime UTC : %s"
                              "realtime LCL : %s",
                              LW_TICK_HZ,
                              API_TimeGet64(),
                              (UINT)tsMono.tv_sec / 3600,
                              (UINT)tsMono.tv_sec / 60 % 60,
                              (UINT)tsMono.tv_sec % 60,
                              tsMono.tv_nsec,
                              cRealtimeUtc,
                              cRealtimeLcl);                            /*  将信息打印到缓冲            */
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  计算实际拷贝的字节数        */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** 函数名称: __procFsKernelObjectsRead
** 功能描述: procfs 读一个内核 objects proc 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsKernelObjectsRead (PLW_PROCFS_NODE  p_pfsn, 
                                           PCHAR            pcBuffer, 
                                           size_t           stMaxBytes,
                                           off_t            oft)
{
    const    CHAR       cObjectsInfoHdr[] = 
    "object      total    used     max-used\n";
    
    REGISTER PCHAR      pcFileBuffer;
             size_t     stRealSize;                                     /*  实际的文件内容大小          */
             size_t     stCopeBytes;

    /*
     *  程序运行到这里, 文件缓冲一定已经分配了预置的内存大小(创建节点时预置大小为 64 字节).
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  需要生成文件                */
        UINT    uiUsed;
        UINT    uiMaxUsed;
        
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, 0, cObjectsInfoHdr);
        
#if (LW_CFG_EVENT_EN > 0) && (LW_CFG_MAX_EVENTS > 0)
        uiUsed     = _K_resrcEvent.RESRC_uiUsed;
        uiMaxUsed  = _K_resrcEvent.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "event", LW_CFG_MAX_EVENTS, uiUsed, uiMaxUsed);
#endif
        
#if (LW_CFG_EVENTSET_EN > 0) && (LW_CFG_MAX_EVENTSETS > 0)
        uiUsed     = _K_resrcEventSet.RESRC_uiUsed;
        uiMaxUsed  = _K_resrcEventSet.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "eventset", LW_CFG_MAX_EVENTSETS, uiUsed, uiMaxUsed);
#endif
        
        uiUsed     = _K_resrcHeap.RESRC_uiUsed;
        uiMaxUsed  = _K_resrcHeap.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "heap", LW_CFG_MAX_REGIONS + 2, uiUsed, uiMaxUsed);
        
#if (LW_CFG_MSGQUEUE_EN > 0) && (LW_CFG_MAX_MSGQUEUES > 0)
        uiUsed     = _K_resrcMsgQueue.RESRC_uiUsed;
        uiMaxUsed  = _K_resrcMsgQueue.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "msgqueue", LW_CFG_MAX_MSGQUEUES, uiUsed, uiMaxUsed);
#endif
        
#if (LW_CFG_PARTITION_EN > 0) && (LW_CFG_MAX_PARTITIONS > 0)
        uiUsed     = _K_resrcPart.RESRC_uiUsed;
        uiMaxUsed  = _K_resrcPart.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "partition", LW_CFG_MAX_PARTITIONS, uiUsed, uiMaxUsed);
#endif
          
#if (LW_CFG_RMS_EN > 0) && (LW_CFG_MAX_RMSS > 0)
        uiUsed     = _K_resrcRms.RESRC_uiUsed;
        uiMaxUsed  = _K_resrcRms.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "rms", LW_CFG_MAX_RMSS, uiUsed, uiMaxUsed);
#endif
        
        uiUsed     = _K_resrcTcb.RESRC_uiUsed;
        uiMaxUsed  = _K_resrcTcb.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "thread", LW_CFG_MAX_THREADS, uiUsed, uiMaxUsed);
        
#if (LW_CFG_SMP_EN == 0) && (LW_CFG_THREAD_PRIVATE_VARS_EN > 0) && (LW_CFG_MAX_THREAD_GLB_VARS > 0)
        uiUsed     = _K_resrcThreadVar.RESRC_uiUsed;
        uiMaxUsed  = _K_resrcThreadVar.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "threadvar", LW_CFG_MAX_THREAD_GLB_VARS, uiUsed, uiMaxUsed);
#endif

#if	((LW_CFG_HTIMER_EN > 0) || (LW_CFG_ITIMER_EN > 0)) && (LW_CFG_MAX_TIMERS)
        uiUsed     = _K_resrcTmr.RESRC_uiUsed;
        uiMaxUsed  = _K_resrcTmr.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "timer", LW_CFG_MAX_TIMERS, uiUsed, uiMaxUsed);
#endif
        
#if LW_CFG_MPI_EN > 0
        uiUsed     = _G_resrcDpma.RESRC_uiUsed;
        uiMaxUsed  = _G_resrcDpma.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "dpma", LW_CFG_MAX_MPDPMAS, uiUsed, uiMaxUsed);
#endif
        
#if (LW_CFG_THREAD_POOL_EN > 0) && (LW_CFG_MAX_THREAD_POOLS > 0)
        uiUsed     = _S_resrcThreadPool.RESRC_uiUsed;
        uiMaxUsed  = _S_resrcThreadPool.RESRC_uiMaxUsed;
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_OBJECTS, stRealSize,
                              "%-12s%-9d%-9d%d\n",
                              "threadpool", LW_CFG_MAX_THREAD_POOLS, uiUsed, uiMaxUsed);
#endif

        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  计算实际拷贝的字节数        */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** 函数名称: __procFsKernelSmpRead
** 功能描述: procfs 读一个内核 smp proc 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

static ssize_t  __procFsKernelSmpRead (PLW_PROCFS_NODE  p_pfsn, 
                                       PCHAR            pcBuffer, 
                                       size_t           stMaxBytes,
                                       off_t            oft)
{
    const CHAR      cSmpInfoHdr[] = 
    "LOGIC CPU PHYSICAL CPU NON IDLE STATUS CURRENT THREAD MAX NESTING IPI VECTOR\n"
    "--------- ------------ -------- ------ -------------- ----------- ----------\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  实际的文件内容大小          */
          size_t    stCopeBytes;
    
    /*
     *  由于预置内存大小为 0 , 所以打开后第一次读取需要手动开辟内存.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  还没有分配内存              */
        size_t          stNeedBufferSize;
        INT             i;
        PLW_CLASS_CPU   pcpu;
        
        stNeedBufferSize  = 96 * LW_NCPUS;
        stNeedBufferSize += sizeof(cSmpInfoHdr);

        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  重新获得文件缓冲区地址      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cSmpInfoHdr); 
                                                                        /*  打印头信息                  */
        __KERNEL_ENTER();
        LW_CPU_FOREACH (i) {
            ULONG               ulMaxNesting;
            ULONG               ulStatus;
            CHAR                cThread[LW_CFG_OBJECT_NAME_SIZE];
            
            pcpu = LW_CPU_GET(i);
            lib_strlcpy(cThread, pcpu->CPU_ptcbTCBCur->TCB_cThreadName, LW_CFG_OBJECT_NAME_SIZE);
            ulMaxNesting = pcpu->CPU_ulInterNestingMax;
            ulStatus     = pcpu->CPU_ulStatus;
            
            __KERNEL_EXIT();
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize,
                                  "%9lu %12lu %8u %-6s %-14s %11lu %10ld\n",
                                  pcpu->CPU_ulCPUId, 
#if LW_CFG_CPU_ARCH_SMT > 0
                                  pcpu->CPU_ulPhyId,
                                  LW_PHYCPU_GET(pcpu->CPU_ulPhyId)->PHYCPU_uiNonIdle,
#else
                                  pcpu->CPU_ulCPUId, 
                                  0,
#endif
                                  (ulStatus & LW_CPU_STATUS_ACTIVE) ? "ACTIVE" : "OFF",
                                  cThread,
                                  ulMaxNesting,
                                  (pcpu->CPU_ulIPIVector == __ARCH_ULONG_MAX) ?
                                  -1 : (LONG)(pcpu->CPU_ulIPIVector));
            __KERNEL_ENTER();
        }
        __KERNEL_EXIT();
    
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    } else {
        stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  计算实际拷贝的字节数        */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** 函数名称: __procFsKernelAffinityRead
** 功能描述: procfs 读一个内核 affinity proc 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsKernelAffinityRead (PLW_PROCFS_NODE  p_pfsn, 
                                            PCHAR            pcBuffer, 
                                            size_t           stMaxBytes,
                                            off_t            oft)
{
    const CHAR      cAffinityInfoHdr[] = 
    "      NAME         TID    PID  CPU\n"
    "---------------- ------- ----- ---\n";
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  实际的文件内容大小          */
          size_t    stCopeBytes;
          
    /*
     *  由于预置内存大小为 0 , 所以打开后第一次读取需要手动开辟内存.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  还没有分配内存              */
        size_t              stNeedBufferSize;
        INT                 i;
        PLW_CLASS_TCB       ptcb;
        LW_OBJECT_HANDLE    ulId;
        CHAR                cName[LW_CFG_OBJECT_NAME_SIZE];
        pid_t               pid;
        INT                 iCpuLock;
        
#if LW_CFG_MODULELOADER_EN > 0
        LW_LD_VPROC    *pvproc;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

        stNeedBufferSize  = 64 * API_KernelGetThreadNum();
        stNeedBufferSize += sizeof(cAffinityInfoHdr);
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  重新获得文件缓冲区地址      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cAffinityInfoHdr); 
                                                                        /*  打印头信息                  */
        __KERNEL_ENTER();
        for (i = 0; i < LW_CFG_MAX_THREADS; i++) {
            ptcb = _K_ptcbTCBIdTable[i];                                /*  获得 TCB 控制块             */
            if (ptcb == LW_NULL) {                                      /*  线程不存在                  */
                continue;
            }
            
            ulId = ptcb->TCB_ulId;
            lib_strlcpy(cName, ptcb->TCB_cThreadName, LW_CFG_OBJECT_NAME_SIZE);
            
#if LW_CFG_MODULELOADER_EN > 0
            pvproc = __LW_VP_GET_TCB_PROC(ptcb);
            if (pvproc) {
                pid = pvproc->VP_pid;
            } else {
                pid = 0;
            }
#else
            pid = 0;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
            
            if (ptcb->TCB_bCPULock) {
                iCpuLock = (INT)ptcb->TCB_ulCPULock;
            } else {
                iCpuLock = PX_ERROR;
            }
            
            __KERNEL_EXIT();
            if (iCpuLock < 0) {
                stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize,
                                      "%-16s %7lx %5d   *\n", 
                                      cName, ulId, pid);
            } else {
                stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize,
                                      "%-16s %7lx %5d %3d\n", 
                                      cName, ulId, pid, iCpuLock);
            }
            __KERNEL_ENTER();
        }
        __KERNEL_EXIT();

        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    } else {
        stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  计算实际拷贝的字节数        */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}

#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** 函数名称: __procFsKernelInfoInit
** 功能描述: procfs 初始化内核 proc 文件
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __procFsKernelInfoInit (VOID)
{
    API_ProcFsMakeNode(&_G_pfsnKernel[0], "/");
    API_ProcFsMakeNode(&_G_pfsnKernel[1], "/");
    API_ProcFsMakeNode(&_G_pfsnKernel[2], "/");
    API_ProcFsMakeNode(&_G_pfsnKernel[3], "/kernel");
    API_ProcFsMakeNode(&_G_pfsnKernel[4], "/kernel");
#if LW_CFG_SMP_EN > 0
    API_ProcFsMakeNode(&_G_pfsnKernel[5], "/");
    API_ProcFsMakeNode(&_G_pfsnKernel[6], "/kernel");
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
}

#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
                                                                        /*  LW_CFG_PROCFS_KERNEL_INFO   */
/*********************************************************************************************************
  END
*********************************************************************************************************/

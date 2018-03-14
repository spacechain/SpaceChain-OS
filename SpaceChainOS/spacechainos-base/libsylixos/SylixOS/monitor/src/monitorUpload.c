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
** 文   件   名: monitorUpload.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 08 月 27 日
**
** 描        述: SylixOS 内核事件监控器, 通过文件上传. 可以是设备文件, 也可以是普通文件.

** BUG:
2013.09.14  可以设置监控的目标进程.
2014.10.21  与远程节点的握手使用 XML 格式.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if (LW_CFG_MONITOR_EN > 0) && (LW_CFG_DEVICE_EN > 0)
#include "monitorBuffer.h"
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  上传任务参数
*********************************************************************************************************/
#define MONITOR_STK_MIN_SZ      (LW_CFG_KB_SIZE * 8)
/*********************************************************************************************************
  上传参数
*********************************************************************************************************/
#define MONITOR_EVENT_VERSION   "0.0.3"
#define MONITOR_EVENT_TIMEOUT   (LW_TICK_HZ * 2)
/*********************************************************************************************************
  上传节点
*********************************************************************************************************/
typedef struct {
    INT                         UPLOAD_iFd;                             /*  文件描述符                  */
    UINT64                      UPLOAD_u64SubEventAllow[MONITOR_EVENT_ID_MAX + 1];
                                                                        /*  事件滤波器                  */
    PVOID                       UPLOAD_pvMonitorBuffer;                 /*  事件缓冲区                  */
    PVOID                       UPLOAD_pvMonitorTrace;                  /*  事件跟踪节点                */
    LW_OBJECT_HANDLE            UPLOAD_hMonitorThread;                  /*  上传任务                    */
    
    ULONG                       UPLOAD_ulOption;                        /*  选项                        */
    BOOL                        UPLOAD_bNeedDelete;                     /*  需要删除                    */
    
    pid_t                       UPLOAD_pid;                             /*  监控进程选择                */
    ULONG                       UPLOAD_ulOverrun;                       /*  消息溢出计数                */
    
    LW_SPINLOCK_DEFINE         (UPLOAD_slLock);                         /*  spinlock                    */
} MONITOR_UPLOAD;
typedef MONITOR_UPLOAD         *PMONITOR_UPLOAD;
/*********************************************************************************************************
** 函数名称: __monitorUploadFilter
** 功能描述: 监控跟踪节点滤波器
** 输　入  : pvMonitorUpload    文件上传监控跟踪节点
**           uiEventId          事件号
**           uiSubEvent         子事件号
** 输　出  : 是否收集事件
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  __monitorUploadFilter (PVOID  pvMonitorUpload, UINT32  uiEventId, UINT32  uiSubEvent)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;
    PLW_CLASS_TCB    ptcbCur;
    
#if LW_CFG_MODULELOADER_EN > 0
    LW_LD_VPROC     *pvproc;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    if (pmu) {
        if (pmu->UPLOAD_u64SubEventAllow[uiEventId] & ((UINT64)1 << uiSubEvent)) {
            if (uiEventId == MONITOR_EVENT_ID_SCHED) {                  /*  调度事件不拦截              */
                return  (LW_TRUE);
            }
            
            if (LW_CPU_GET_CUR_NESTING()) {                             /*  中断状态                    */
                ptcbCur = LW_NULL;

#if LW_CFG_MODULELOADER_EN > 0
                pvproc  = LW_NULL;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
            } else {                                                    /*  多任务状态                  */
                LW_TCB_GET_CUR_SAFE(ptcbCur);

#if LW_CFG_MODULELOADER_EN > 0
                pvproc = __LW_VP_GET_TCB_PROC(ptcbCur);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
                if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_NO_MONITOR) {
                    if (uiEventId != MONITOR_EVENT_ID_SCHED) {
                        return  (LW_FALSE);
                    }
                }
            }
            
#if LW_CFG_MODULELOADER_EN > 0
            if (pmu->UPLOAD_pid == 0) {                                 /*  kernel                      */
                if (pvproc) {
                    return  (LW_FALSE);
                }
            
            } else if (pmu->UPLOAD_pid > 0) {                           /*  process                     */
                if ((pvproc == LW_NULL) ||
                    (pvproc->VP_pid != pmu->UPLOAD_pid)) {
                    return  (LW_FALSE);
                }
            
            } else if (pmu->UPLOAD_pid == -2) {                         /*  all process not kernel      */
                if (pvproc == LW_NULL) {
                    return  (LW_FALSE);
                }
            }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
            return  (LW_TRUE);
        }
    }
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: __monitorUploadCollector
** 功能描述: 监控跟踪节点收集器
** 输　入  : pvMonitorUpload    文件上传监控跟踪节点
**           uiEventId          事件号
**           uiSubEvent         子事件号
**           pvMsg              事件信息
**           stSize             信息长度
**           pcAddtional        附加字串
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __monitorUploadCollector (PVOID           pvMonitorUpload, 
                                       UINT32          uiEventId, 
                                       UINT32          uiSubEvent,
                                       CPVOID          pvMsg,
                                       size_t          stSize,
                                       CPCHAR          pcAddtional)
{
    INTREG           iregInterLevel;
    
    UINT16           usMsgLen;
    size_t           stOffset = 0;
    size_t           stAddStrLen;
    INT64            i64KernelTime;
    
    UCHAR            ucBuffer[MONITOR_EVENT_MAX_SIZE];
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;
    
    if (pcAddtional) {
        stAddStrLen = lib_strlen(pcAddtional);
    } else {
        stAddStrLen = 0;
    }
    
    usMsgLen = (UINT16)(2 + 4 + 4 + 8 + sizeof(LW_OBJECT_HANDLE) 
             + stSize + stAddStrLen);                                   /*  事件信息总长度              */
    if (usMsgLen > MONITOR_EVENT_MAX_SIZE) {
        return;
    }
    
    ucBuffer[0] = (UCHAR)(usMsgLen >> 8);                               /*  前两字节为大端长度          */
    ucBuffer[1] = (UCHAR)(usMsgLen & 0xFF);
    stOffset    = 2;
    
    lib_memcpy(&ucBuffer[stOffset], &uiEventId,  4);                    /*  事件 ID                     */
    stOffset += 4;
    
    lib_memcpy(&ucBuffer[stOffset], &uiSubEvent, 4);                    /*  子事件 ID                   */
    stOffset += 4;
    
    __KERNEL_TIME_GET(i64KernelTime, INT64);
    lib_memcpy(&ucBuffer[stOffset], &i64KernelTime, 8);                 /*  系统 TICK                   */
    stOffset += 8;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        LW_OBJECT_HANDLE   hInval = LW_OBJECT_HANDLE_INVALID;
        lib_memcpy(&ucBuffer[stOffset], &hInval, sizeof(LW_OBJECT_HANDLE));
    
    } else {
        PLW_CLASS_TCB    ptcbCur;
        LW_TCB_GET_CUR_SAFE(ptcbCur);
        lib_memcpy(&ucBuffer[stOffset], &ptcbCur->TCB_ulId, sizeof(LW_OBJECT_HANDLE));
    }
    stOffset += sizeof(LW_OBJECT_HANDLE);
    
    if (stSize) {                                                       /*  事件信息                    */
        lib_memcpy(&ucBuffer[stOffset], pvMsg, stSize);
        stOffset += stSize;
    }
    
    if (stAddStrLen) {
        lib_memcpy(&ucBuffer[stOffset], pcAddtional, stAddStrLen);      /*  事件附加字符串              */
    }
    
    if (__monitorBufferPut(pmu->UPLOAD_pvMonitorBuffer, 
                           ucBuffer) == 0) {                            /*  存入事件缓冲区              */
        LW_SPIN_LOCK_QUICK(&pmu->UPLOAD_slLock, &iregInterLevel);
        pmu->UPLOAD_ulOverrun++;
        LW_SPIN_UNLOCK_QUICK(&pmu->UPLOAD_slLock, iregInterLevel);
    }
}
/*********************************************************************************************************
** 函数名称: __monitorUploadTryProcProto
** 功能描述: 监控跟踪节点尝试处理协议
** 输　入  : pmu    文件上传监控跟踪节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __monitorUploadTryProcProto (PMONITOR_UPLOAD  pmu)
{
    INT     iNRead;
    UCHAR   ucBuffer[12];
    
    while (ioctl(pmu->UPLOAD_iFd, FIONREAD, &iNRead) == 0) {
        if (iNRead >= 12) {
            if (read(pmu->UPLOAD_iFd, ucBuffer, 12) == 12) {
                UINT32  uiEvent;
                UINT64  u64Allow;
                
                lib_memcpy(&uiEvent,  &ucBuffer[0], 4);
                lib_memcpy(&u64Allow, &ucBuffer[4], 8);
                
                if (uiEvent <= MONITOR_EVENT_ID_MAX) {                  /*  设置滤波器                  */
                    API_MonitorUploadSetFilter((PVOID)pmu, uiEvent, u64Allow, 
                                               LW_MONITOR_UPLOAD_SET_EVT_SET);
                
                } else if (uiEvent == MONITOR_EVENT_ID_MAX) {           /*  设置全局开关                */
                    if (u64Allow) {
                        API_MonitorUploadEnable((PVOID)pmu);
                    } else {
                        API_MonitorUploadDisable((PVOID)pmu);
                    }
                
                } else if (uiEvent == (MONITOR_EVENT_ID_MAX + 1)) {     /*  设置进程信息                */
                    pid_t   pid = (pid_t)u64Allow;
                    API_MonitorUploadSetPid((PVOID)pmu, pid);
                }
            }
        } else {
            break;                                                      /*  缓冲区里面没有待处理的数据  */
        }
    }
}
/*********************************************************************************************************
** 函数名称: __monitorUploadHello
** 功能描述: 发送 upload hello 数据包
** 输　入  : iFd       双穿
** 输　出  : 是否发送成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __monitorUploadHello (INT  iFd)
{
    static const CHAR       cMonitor[] = \
    "<?xml version=\"1.0\">"
    "<!-- Copyright (C) 2007-2014 SylixOS Group. -->"
    "<cpu name=\"%s\" wordlen=\"%d\" endian=\"%d\"/>"
    "<os name=\"sylixos\" version=\"%s\" tick=\"%dhz\"/>"
    "<tracer name=\"monitor\" version=\"%s\"/>\n";
                 
    CHAR            cStart[16];
    ssize_t         sstLen;
    struct timeval  tvTo = {5, 0};
                 
    fdprintf(iFd, cMonitor, bspInfoCpu(), LW_CFG_CPU_WORD_LENGHT, LW_CFG_CPU_ENDIAN,
                            __SYLIXOS_VERSTR, LW_TICK_HZ,
                            MONITOR_EVENT_VERSION);
                            
    if (waitread(iFd, &tvTo) < 1) {
        fprintf(stderr, "remote no response.\n");
        return  (PX_ERROR);
    }
    
    sstLen = read(iFd, cStart, 16);
    if ((sstLen < 5) || lib_strncmp(cStart, "start", 5)) {
        fprintf(stderr, "remote response error.\n");
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __monitorUploadThread
** 功能描述: 监控跟踪节点上传线程
** 输　入  : pvArg     监控跟踪节点
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PVOID  __monitorUploadThread (PVOID  pvArg)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvArg;
    UCHAR            ucEvent[MONITOR_EVENT_MAX_SIZE];
    ssize_t          sstLen;
    BOOL             bMustStop = LW_FALSE;
    
    INT64            i64ProcProtoTime;
    INT64            i64Now;
    
    __KERNEL_TIME_GET(i64ProcProtoTime, INT64);
    
    if (__monitorUploadHello(pmu->UPLOAD_iFd) < ERROR_NONE) {           /*  发送 hello 数据头           */
        bMustStop = LW_TRUE;
    }
    
    for (;;) {
        if (bMustStop == LW_FALSE) {
            sstLen = __monitorBufferGet(pmu->UPLOAD_pvMonitorBuffer, 
                                        ucEvent, sizeof(ucEvent), 
                                        MONITOR_EVENT_TIMEOUT);
            if (sstLen < 0) {                                           /*  节点删除                    */
                fprintf(stderr, "monitor event buffer crash.\n"
                                "you must stop monitor manually.\n");
                bMustStop = LW_TRUE;
                
            } else if (sstLen > 0) {                                    /*  接收到事件信息              */
                sstLen = write(pmu->UPLOAD_iFd, ucEvent, (size_t)sstLen);
                if (sstLen <= 0) {
                    fprintf(stderr, "monitor can not update event to server, error: %s\n"
                                    "you must stop monitor manually.\n",
                                    lib_strerror(errno));
                    bMustStop = LW_TRUE;
                }
            }
            
            if (pmu->UPLOAD_ulOption & LW_MONITOR_UPLOAD_PROTO) {
                __KERNEL_TIME_GET(i64Now, INT64);
                if ((i64Now - i64ProcProtoTime) > MONITOR_EVENT_TIMEOUT) {
                    __monitorUploadTryProcProto(pmu);                   /*  处理协议                    */
                    __KERNEL_TIME_GET(i64ProcProtoTime, INT64);
                }
            }
        } else {
            API_TimeSleep(MONITOR_EVENT_TIMEOUT);
        }
        
        if (pmu->UPLOAD_bNeedDelete) {                                  /*  需要删除                    */
            API_MonitorTraceDelete(pmu->UPLOAD_pvMonitorTrace);
            __monitorBufferDelete(pmu->UPLOAD_pvMonitorBuffer);
            __KHEAP_FREE(pmu);
            break;
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_MonitorUploadCreate
** 功能描述: 创建一个监控跟踪上传节点
** 输　入  : iFd           文件描述符
**           stSize        缓冲区大小
**           ulOption      创建选项
**           pthreadattr   监控任务创建选项
** 输　出  : Upload 节点
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
PVOID  API_MonitorUploadCreate (INT                   iFd, 
                                size_t                stSize,
                                ULONG                 ulOption,
                                PLW_CLASS_THREADATTR  pthreadattr)
{
    INT                 i;
    PMONITOR_UPLOAD     pmu;
    LW_CLASS_THREADATTR threadattr;
    
    if (iFd < 0) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (LW_NULL);
    }
    
    pmu = (PMONITOR_UPLOAD)__KHEAP_ALLOC(sizeof(MONITOR_UPLOAD));
    if (!pmu) {
        _ErrorHandle(ERROR_MONITOR_ENOMEM);
        return  (LW_NULL);
    }
    
    pmu->UPLOAD_ulOption    = ulOption;
    pmu->UPLOAD_bNeedDelete = LW_FALSE;
    pmu->UPLOAD_iFd         = iFd;
    pmu->UPLOAD_pid         = -1;                                       /*  kernel and process event    */
    pmu->UPLOAD_ulOverrun   = 0ul;
    
    LW_SPIN_INIT(&pmu->UPLOAD_slLock);
    
    for (i = 0; i <= MONITOR_EVENT_ID_MAX; i++) {                       /*  默认过滤所有事件            */
        pmu->UPLOAD_u64SubEventAllow[i] = 0ull;
    }
    
    pmu->UPLOAD_pvMonitorBuffer = __monitorBufferCreate(stSize);
    if (!pmu->UPLOAD_pvMonitorBuffer) {
        __KHEAP_FREE(pmu);
        return  (LW_NULL);
    }
    
    pmu->UPLOAD_pvMonitorTrace = API_MonitorTraceCreate(__monitorUploadFilter,
                                                        __monitorUploadCollector,
                                                        (PVOID)pmu,
                                                        LW_FALSE);
    if (!pmu->UPLOAD_pvMonitorTrace) {
        __monitorBufferDelete(pmu->UPLOAD_pvMonitorBuffer);
        __KHEAP_FREE(pmu);
        return  (LW_NULL);
    }
    
    if (pthreadattr) {
        threadattr = *pthreadattr;
    } else {
        threadattr = API_ThreadAttrGetDefault();
    }
    
    threadattr.THREADATTR_pvArg     = (PVOID)pmu;
    threadattr.THREADATTR_ulOption |= LW_OPTION_OBJECT_GLOBAL
                                    | LW_OPTION_THREAD_NO_MONITOR;
    
    if (threadattr.THREADATTR_stStackByteSize < MONITOR_STK_MIN_SZ) {
        threadattr.THREADATTR_stStackByteSize = MONITOR_STK_MIN_SZ;
    }
    
    pmu->UPLOAD_hMonitorThread = API_ThreadCreate("t_monitor", __monitorUploadThread,
                                                  &threadattr, LW_NULL);
    if (pmu->UPLOAD_hMonitorThread == LW_OBJECT_HANDLE_INVALID) {
        API_MonitorTraceDelete(pmu->UPLOAD_pvMonitorTrace);
        __monitorBufferDelete(pmu->UPLOAD_pvMonitorBuffer);
        __KHEAP_FREE(pmu);
        return  (LW_NULL);
    }
    
    return  ((PVOID)pmu);
}
/*********************************************************************************************************
** 函数名称: API_MonitorUploadDelete
** 功能描述: 删除一个监控跟踪上传节点
** 输　入  : pvMonitorUpload   监控跟踪节点
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadDelete (PVOID  pvMonitorUpload)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;
    
    if (!pmu) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    pmu->UPLOAD_bNeedDelete = LW_TRUE;
    KN_SMP_WMB();
    
    API_ThreadJoin(pmu->UPLOAD_hMonitorThread, LW_NULL);                /*  等待任务结束                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_MonitorUploadSetPid
** 功能描述: 设置上传监控跟踪节点滤波器进程信息
** 输　入  : pvMonitorUpload   上传监控跟踪节点
**           pid               == 0 (kernel) > 0 (process) == -1 (all) == -2 (all process no kernel)
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 如果滤波器允许收集调度事件, 则 pid 对调度事件不起拦截作用.
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadSetPid (PVOID  pvMonitorUpload, pid_t  pid)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;
    
    if (!pmu) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    pmu->UPLOAD_pid = pid;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_MonitorUploadGetPid
** 功能描述: 获取上传监控跟踪节点滤波器进程信息
** 输　入  : pvMonitorUpload   上传监控跟踪节点
**           pid               当前监控点情况
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadGetPid (PVOID  pvMonitorUpload, pid_t  *pid)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;
    
    if (!pmu) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    if (pid) {
        *pid = pmu->UPLOAD_pid;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_MonitorUploadSetFilter
** 功能描述: 设置上传监控跟踪节点滤波器
** 输　入  : pvMonitorUpload   上传监控跟踪节点
**           uiEventId         事件 ID
**           u64SubEventAllow  允许子事件集
**           iHow              设置方法
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadSetFilter (PVOID    pvMonitorUpload, 
                                   UINT32   uiEventId, 
                                   UINT64   u64SubEventAllow,
                                   INT      iHow)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;

    if (!pmu || uiEventId > MONITOR_EVENT_ID_MAX) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    switch (iHow) {
    
    case LW_MONITOR_UPLOAD_SET_EVT_SET:
        pmu->UPLOAD_u64SubEventAllow[uiEventId] = u64SubEventAllow;
        break;
        
    case LW_MONITOR_UPLOAD_ADD_EVT_SET:
        pmu->UPLOAD_u64SubEventAllow[uiEventId] |= u64SubEventAllow;
        break;
        
    case LW_MONITOR_UPLOAD_SUB_EVT_SET:
        pmu->UPLOAD_u64SubEventAllow[uiEventId] &= ~u64SubEventAllow;
        break;
    
    default:
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_MonitorUploadSetFilter
** 功能描述: 获取上传监控跟踪节点滤波器
** 输　入  : pvMonitorUpload   上传监控跟踪节点
**           uiEventId         事件 ID
**           pu64SubEventAllow 允许子事件集
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadGetFilter (PVOID    pvMonitorUpload, 
                                   UINT32   uiEventId, 
                                   UINT64  *pu64SubEventAllow)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;

    if (!pmu || uiEventId > MONITOR_EVENT_ID_MAX) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    if (pu64SubEventAllow) {
        *pu64SubEventAllow = pmu->UPLOAD_u64SubEventAllow[uiEventId];
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_MonitorUploadEnable
** 功能描述: 使能上传监控跟踪节点
** 输　入  : pvMonitorUpload   上传监控跟踪节点
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadEnable (PVOID  pvMonitorUpload)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;
    
    if (!pmu) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    API_MonitorTraceEnable(pmu->UPLOAD_pvMonitorTrace);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_MonitorUploadDisable
** 功能描述: 禁能上传监控跟踪节点
** 输　入  : pvMonitorUpload   上传监控跟踪节点
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadDisable (PVOID  pvMonitorUpload)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;
    
    if (!pmu) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    API_MonitorTraceDisable(pmu->UPLOAD_pvMonitorTrace);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_MonitorUploadFlush
** 功能描述: 清除上传监控跟踪节点内所有缓存的信息
** 输　入  : pvMonitorUpload   上传监控跟踪节点
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadFlush (PVOID  pvMonitorUpload)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;

    if (!pmu) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    __monitorBufferFlush(pmu->UPLOAD_pvMonitorBuffer);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_MonitorUploadClearOverrun
** 功能描述: 清除上传监控跟踪节点丢失信息的个数计数器
** 输　入  : pvMonitorUpload   上传监控跟踪节点
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadClearOverrun (PVOID  pvMonitorUpload)
{
    INTREG           iregInterLevel;
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;
    
    if (!pmu) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    LW_SPIN_LOCK_QUICK(&pmu->UPLOAD_slLock, &iregInterLevel);
    pmu->UPLOAD_ulOverrun = 0;
    LW_SPIN_UNLOCK_QUICK(&pmu->UPLOAD_slLock, iregInterLevel);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_MonitorUploadGetOverrun
** 功能描述: 获得上传监控跟踪节点丢失信息的个数
** 输　入  : pvMonitorUpload   上传监控跟踪节点
**           pulOverRun        丢失个数
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadGetOverrun (PVOID  pvMonitorUpload, ULONG  *pulOverRun)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;
    
    if (!pmu) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    if (pulOverRun) {
        *pulOverRun = pmu->UPLOAD_ulOverrun;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_MonitorUploadFd
** 功能描述: 监控跟踪节点文件描述符
** 输　入  : pvMonitorUpload   上传监控跟踪节点
**           piFd              文件描述符
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorUploadFd (PVOID  pvMonitorUpload, INT  *piFd)
{
    PMONITOR_UPLOAD  pmu = (PMONITOR_UPLOAD)pvMonitorUpload;
    
    if (!pmu) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (ERROR_MONITOR_EINVAL);
    }
    
    if (piFd) {
        *piFd = pmu->UPLOAD_iFd;
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
                                                                        /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: threadpool.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 03 月 20 日
**
** 描        述: 系统线程池功能接口函数库

** BUG
2007.09.21  加入 _DebugHandle() 功能。
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2008.05.31  使用 __KERNEL_MODE_...().
2008.05.31  只能使用动态内存分配.
2013.09.22  创建线程池 pthreadattr 为 NULL 则使用默认属性.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if (LW_CFG_THREAD_POOL_EN > 0) && (LW_CFG_MAX_THREAD_POOLS > 0)
#include "threadpoolLib.h"
/*********************************************************************************************************
  MACRO
*********************************************************************************************************/
#define __OPT_MUTEX_LOCK   (LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE | LW_OPTION_INHERIT_PRIORITY)
/*********************************************************************************************************
  threadpool lock
*********************************************************************************************************/
#define __THREADPOOL_LOCK(pthreadpool)      \
        API_SemaphoreMPend(pthreadpool->TPCB_hMutexLock, LW_OPTION_WAIT_INFINITE)
#define __THREADPOOL_UNLOCK(pthreadpool)    \
        API_SemaphoreMPost(pthreadpool->TPCB_hMutexLock)
/*********************************************************************************************************
** 函数名称: API_ThreadPoolCreate
** 功能描述: 建立一个 ThreadPool
** 输　入  : pcName                        线程池名字
**           pfuncThread                   代码入口
**           pthreadattr                   线程属性控制块
**           usMaxThreadCounter            最大线程数量
**           pulId                         Id指针
** 输　出  : 新建立的线程池句柄
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
LW_OBJECT_HANDLE  API_ThreadPoolCreate (PCHAR                    pcName,
                                        PTHREAD_START_ROUTINE    pfuncThread,
                                        PLW_CLASS_THREADATTR     pthreadattr,
                                        UINT16                   usMaxThreadCounter,
                                        LW_OBJECT_ID            *pulId)
{
    REGISTER PLW_CLASS_THREADPOOL   pthreadpool;
    REGISTER ULONG                  ulIdTemp;
    REGISTER ULONG                  ulOption;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pfuncThread) {                                                 /*  没有线程执行函数            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pfuncThread invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    if (usMaxThreadCounter >= LW_CFG_MAX_THREADS || 
        usMaxThreadCounter == 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "usMaxThreadCounter invalidate.\r\n");
        _ErrorHandle(ERROR_THREADPOOL_MAX_COUNTER);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
#endif

    if (_Object_Name_Invalid(pcName)) {                                 /*  检查名字有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    __KERNEL_MODE_PROC(
        pthreadpool = _Allocate_ThreadPool_Object();                    /*  获得一个分区控制块          */
    );
    
    if (!pthreadpool) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "threadpool full.\r\n");
        _ErrorHandle(ERROR_THREADPOOL_FULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    pthreadpool->TPCB_hMutexLock = API_SemaphoreMCreate("tp_lock",
                                                        LW_PRIO_DEF_CEILING,
                                                        __OPT_MUTEX_LOCK,
                                                        LW_NULL);       /*  建立互斥锁变量              */
    if (!pthreadpool->TPCB_hMutexLock) {                                /*  建立失败                    */
        __KERNEL_MODE_PROC(
            _Free_ThreadPool_Object(pthreadpool);
        );
        _ErrorHandle(ERROR_EVENT_FULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (pcName) {                                                       /*  拷贝名字                    */
        lib_strcpy(pthreadpool->TPCB_cThreadPoolName, pcName);
    } else {
        pthreadpool->TPCB_cThreadPoolName[0] = PX_EOS;                  /*  清空名字                    */
    }
    
    if (pthreadattr) {
        pthreadpool->TPCB_threakattrAttr = *pthreadattr;                /*  保存属性块                  */
    
    } else {
        pthreadpool->TPCB_threakattrAttr = API_ThreadAttrGetDefault();
    }
    
    pthreadpool->TPCB_threakattrAttr.THREADATTR_pstkLowAddr = LW_NULL;  /*  必须采用动态分配堆栈        */
    
    pthreadpool->TPCB_pthreadStartAddress = pfuncThread;                /*  线程代码入口                */
    pthreadpool->TPCB_pringFirstThread    = LW_NULL;                    /*  没有线程                    */
    pthreadpool->TPCB_usMaxThreadCounter  = usMaxThreadCounter;         /*  最大线程数量                */   
    pthreadpool->TPCB_usThreadCounter     = 0;                          /*  当前线程数量                */
    
    ulOption = pthreadattr->THREADATTR_ulOption;
    
    ulIdTemp = _MakeObjectId(_OBJECT_THREAD_POOL, 
                             LW_CFG_PROCESSOR_NUMBER, 
                             pthreadpool->TPCB_usIndex);                /*  构建对象 id                 */
    
    if (pulId) {
        *pulId = ulIdTemp;
    }
    
    __LW_OBJECT_CREATE_HOOK(ulIdTemp, ulOption);
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "thread pool \"%s\" has been create.\r\n", (pcName ? pcName : ""));
    
    return  (ulIdTemp);
}
/*********************************************************************************************************
** 函数名称: API_ThreadPoolDelete
** 功能描述: 删除一个 ThreadPool
** 输　入  : pulId                         线程池句柄指针
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadPoolDelete (LW_OBJECT_HANDLE   *pulId)
{
    REGISTER PLW_CLASS_THREADPOOL      pthreadpool;
    REGISTER UINT16                    usIndex;
    REGISTER LW_OBJECT_HANDLE          ulId;
    REGISTER ULONG                     ulErrCode;
    
    REGISTER UINT16                    usI;
    REGISTER PLW_CLASS_TCB             ptcb;
    REGISTER PLW_LIST_RING             pringTcb;
             LW_OBJECT_HANDLE          hThread;
    
    ulId = *pulId;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD_POOL)) {                   /*  对象类型检查                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_ThreadPool_Index_Invalid(usIndex)) {                           /*  缓冲区索引检查              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    pthreadpool = &_S_threadpoolBuffer[usIndex];
    
    ulErrCode = __THREADPOOL_LOCK(pthreadpool);                         /*  锁                          */
    if (ulErrCode) {                                                    /*  被删除了？                  */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    for (usI = pthreadpool->TPCB_usThreadCounter;                       /*  开始删除建立的线程          */
         usI > 0;
         usI--) {
         
        pringTcb = pthreadpool->TPCB_pringFirstThread;
        
        ptcb = _LIST_ENTRY(pringTcb, LW_CLASS_TCB, TCB_ringThreadPool);
        
        hThread = ptcb->TCB_ulId;
        
        API_ThreadDelete(&hThread, LW_NULL);
        if (hThread) {                                                  /*  释放信号量                  */
            __THREADPOOL_UNLOCK(pthreadpool);
            ulErrCode = API_GetLastError();                             /*  在 API_ThreadDelete 中有    */
            return  (ulErrCode);                                        /*   _ErrorHandle()             */
        }
        
        pthreadpool->TPCB_usThreadCounter--;
        
        _List_Ring_Del(pringTcb, &pthreadpool->TPCB_pringFirstThread);  /*  从管理链表中删除            */
        
    }
    
    API_SemaphoreMDelete(&pthreadpool->TPCB_hMutexLock);                /*  删除锁                      */
    
    _ObjectCloseId(pulId);

    _DebugFormat(__LOGMESSAGE_LEVEL, "thread pool \"%s\" has been delete.\r\n", 
                 pthreadpool->TPCB_cThreadPoolName);
    
    __KERNEL_MODE_PROC(
        _Free_ThreadPool_Object(pthreadpool);                           /*  释放控制块                  */
    );
    
    __LW_OBJECT_DELETE_HOOK(ulId);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ThreadPoolAddThread
** 功能描述: 指定一个 ThreadPool 增加一个线程
** 输　入  : 
**           ulId                          线程池句柄
**           pcArg                         线程入口参数
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadPoolAddThread (LW_OBJECT_HANDLE   ulId, PVOID  pvArg)
{
    REGISTER PLW_CLASS_THREADPOOL      pthreadpool;
    REGISTER UINT16                    usIndex;
    REGISTER ULONG                     ulErrCode;
    
    REGISTER PLW_CLASS_TCB             ptcb;
    REGISTER PLW_LIST_RING             pringTcb;
             LW_OBJECT_HANDLE          hThread;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD_POOL)) {                   /*  对象类型检查                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_ThreadPool_Index_Invalid(usIndex)) {                           /*  缓冲区索引检查              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    pthreadpool = &_S_threadpoolBuffer[usIndex];
    
    ulErrCode = __THREADPOOL_LOCK(pthreadpool);                         /*  锁                          */
    if (ulErrCode) {                                                    /*  被删除了？                  */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if ((pthreadpool->TPCB_usThreadCounter) == 
        (pthreadpool->TPCB_usMaxThreadCounter)) {                       /*  是否应经满了                */
        __THREADPOOL_UNLOCK(pthreadpool);
        _ErrorHandle(ERROR_THREADPOOL_FULL);
        return  (ERROR_THREADPOOL_FULL);
    }
    
    pthreadpool->TPCB_threakattrAttr.THREADATTR_pvArg = pvArg;          /*  保存新参数                  */
    
    hThread = API_ThreadCreate(pthreadpool->TPCB_cThreadPoolName,
                               pthreadpool->TPCB_pthreadStartAddress,
                               &pthreadpool->TPCB_threakattrAttr,
                               LW_NULL);                                /*  建立线程                    */
                     
    if (!hThread) {
        __THREADPOOL_UNLOCK(pthreadpool);
        ulErrCode = API_GetLastError();
        return  (ulErrCode);
    }
    
    pthreadpool->TPCB_usThreadCounter++;
    
    ptcb = _K_ptcbTCBIdTable[_ObjectGetIndex(hThread)];                 /*  获得线程控制块              */
    
    pringTcb = &ptcb->TCB_ringThreadPool;
    
    _List_Ring_Add_Last(pringTcb, &pthreadpool->TPCB_pringFirstThread); /*  加入管理链表                */
    
    __THREADPOOL_UNLOCK(pthreadpool);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ThreadPoolDeleteThread
** 功能描述: 指定一个 ThreadPool 删除一个线程
** 输　入  : 
**           ulId                          线程池句柄
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadPoolDeleteThread (LW_OBJECT_HANDLE   ulId)
{
    REGISTER PLW_CLASS_THREADPOOL      pthreadpool;
    REGISTER UINT16                    usIndex;
    REGISTER ULONG                     ulErrCode;
    
    REGISTER PLW_CLASS_TCB             ptcb;
    REGISTER PLW_LIST_RING             pringTcb;
             LW_OBJECT_HANDLE          hThread;
             
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD_POOL)) {                   /*  对象类型检查                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_ThreadPool_Index_Invalid(usIndex)) {                           /*  缓冲区索引检查              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    pthreadpool = &_S_threadpoolBuffer[usIndex];
    
    ulErrCode = __THREADPOOL_LOCK(pthreadpool);                         /*  锁                          */
    if (ulErrCode) {                                                    /*  被删除了？                  */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (pthreadpool->TPCB_usThreadCounter == 0) {                       /*  是否已经空了                */
        __THREADPOOL_UNLOCK(pthreadpool);
        _ErrorHandle(ERROR_THREADPOOL_NULL);
        return  (ERROR_THREADPOOL_NULL);
    }
    
    pringTcb = pthreadpool->TPCB_pringFirstThread;
        
    ptcb = _LIST_ENTRY(pringTcb, LW_CLASS_TCB, TCB_ringThreadPool);
        
    hThread = ptcb->TCB_ulId;
    
    API_ThreadDelete(&hThread, LW_NULL);
    if (hThread) {                                                      /*  释放信号量                  */
        __THREADPOOL_UNLOCK(pthreadpool);
        ulErrCode = API_GetLastError();                                 /*  在 API_ThreadDelete 中有    */
        return  (ulErrCode);                                            /*  _ErrorHandle()              */
    }
    
    pthreadpool->TPCB_usThreadCounter--;
        
    _List_Ring_Del(pringTcb, &pthreadpool->TPCB_pringFirstThread);      /*  从管理链表中删除            */
    
    __THREADPOOL_UNLOCK(pthreadpool);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ThreadPoolSetAttr
** 功能描述: 指定一个 ThreadPool 设置线程建立属性块
** 输　入  : ulId                          线程池句柄
**           pthreadattr                   线程属性控制块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadPoolSetAttr (LW_OBJECT_HANDLE  ulId, PLW_CLASS_THREADATTR  pthreadattr)
{
    REGISTER PLW_CLASS_THREADPOOL      pthreadpool;
    REGISTER UINT16                    usIndex;
    REGISTER ULONG                     ulErrCode;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pthreadattr) {                                                 /*  缺少属性块                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pthreadattr invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_ATTR_NULL);
        return  (ERROR_THREAD_ATTR_NULL);
    }
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD_POOL)) {                   /*  对象类型检查                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_ThreadPool_Index_Invalid(usIndex)) {                           /*  缓冲区索引检查              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    pthreadpool = &_S_threadpoolBuffer[usIndex];
    
    ulErrCode = __THREADPOOL_LOCK(pthreadpool);                         /*  锁                          */
    if (ulErrCode) {                                                    /*  被删除了？                  */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    pthreadpool->TPCB_threakattrAttr = *pthreadattr;                    /*  保存属性块                  */
    
    __THREADPOOL_UNLOCK(pthreadpool);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ThreadPoolGetAttr
** 功能描述: 指定一个 ThreadPool 获得线程建立属性块
** 输　入  : 
**           ulId                          线程池句柄
**           pthreadattr                   线程属性控制块
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadPoolGetAttr (LW_OBJECT_HANDLE  ulId, PLW_CLASS_THREADATTR  pthreadattr)
{
    REGISTER PLW_CLASS_THREADPOOL      pthreadpool;
    REGISTER UINT16                    usIndex;
    REGISTER ULONG                     ulErrCode;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pthreadattr) {                                                 /*  缺少属性块                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "pthreadattr invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_ATTR_NULL);
        return  (ERROR_THREAD_ATTR_NULL);
    }
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD_POOL)) {                   /*  对象类型检查                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_ThreadPool_Index_Invalid(usIndex)) {                           /*  缓冲区索引检查              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    pthreadpool = &_S_threadpoolBuffer[usIndex];
    
    ulErrCode = __THREADPOOL_LOCK(pthreadpool);                         /*  锁                          */
    if (ulErrCode) {                                                    /*  被删除了？                  */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    *pthreadattr = pthreadpool->TPCB_threakattrAttr;
    
    __THREADPOOL_UNLOCK(pthreadpool);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ThreadPoolStatus
** 功能描述: 获得 ThreadPool 状态
** 输　入  : 
**           ulId                          线程池句柄
**           ppthreadStartAddr             线程入口地址    可以为 NULL
**           pusMaxThreadCounter           线程数量最大值  可以为 NULL
**           pusThreadCounter              当前线程数量    可以为 NULL
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadPoolStatus (LW_OBJECT_HANDLE         ulId,
                             PTHREAD_START_ROUTINE   *ppthreadStartAddr,
                             UINT16                  *pusMaxThreadCounter,
                             UINT16                  *pusThreadCounter)
{
    REGISTER PLW_CLASS_THREADPOOL      pthreadpool;
    REGISTER UINT16                    usIndex;
    REGISTER ULONG                     ulErrCode;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD_POOL)) {                   /*  对象类型检查                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_ThreadPool_Index_Invalid(usIndex)) {                           /*  缓冲区索引检查              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif

    pthreadpool = &_S_threadpoolBuffer[usIndex];
    
    ulErrCode = __THREADPOOL_LOCK(pthreadpool);                         /*  锁                          */
    if (ulErrCode) {                                                    /*  被删除了？                  */
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (ppthreadStartAddr) {
        *ppthreadStartAddr = pthreadpool->TPCB_pthreadStartAddress;
    }
    
    if (pusMaxThreadCounter) {
        *pusMaxThreadCounter = pthreadpool->TPCB_usMaxThreadCounter;
    }
    
    if (pusThreadCounter) {
        *pusThreadCounter = pthreadpool->TPCB_usThreadCounter;
    }
    
    __THREADPOOL_UNLOCK(pthreadpool);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_THREAD_POOL_EN > 0   */
                                                                        /*  LW_CFG_MAX_THREAD_POOLS > 0 */
/*********************************************************************************************************
  END
*********************************************************************************************************/

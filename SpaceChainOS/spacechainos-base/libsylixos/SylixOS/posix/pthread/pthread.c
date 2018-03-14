/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: pthread.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 30 日
**
** 描        述: pthread 兼容库.

** BUG:
2010.01.13  创建线程使用默认属性时, 优先级应使用 POSIX 默认优先级.
2010.05.07  pthread_create() 应在调用 API_ThreadStart() 之前将线程 ID 写入参数地址.
2010.10.04  修正 pthread_create() 处理 attr 为 NULL 时的线程启动参数错误.
            加入线程并行化相关设置.
2010.10.06  加入 pthread_set_name_np() 函数.
2011.06.24  pthread_create() 当没有 attr 时, 使用 POSIX 默认堆栈大小.
2012.06.18  对 posix 线程的优先级操作, 必须要转换优先级为 sylixos 标准. 
            posix 优先级数字越大, 优先级越高, sylixos 刚好相反.
2013.05.01  If successful, the pthread_*() function shall return zero; 
            otherwise, an error number shall be returned to indicate the error.
2013.05.03  pthread_create() 如果存在 attr 并且 attr 指定的堆栈大小为 0, 则继承创建者的堆栈大小.
2013.05.07  加入 pthread_getname_np() 操作.
2013.09.18  pthread_create() 加入对堆栈警戒区的处理.
2013.12.02  加入 pthread_yield().
2014.01.01  加入 pthread_safe_np() 与 pthread_unsafe_np().
2014.07.04  加入 pthread_setaffinity_np 与 pthread_getaffinity_np();
2016.04.12  加入 GJB7714 相关 API 支持.
2017.03.10  加入 pthread_null_attr_method_np() 函数, 可指定线程创建属性为 NULL 时的方法.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_POSIX
#include "../include/px_pthread.h"                                      /*  已包含操作系统头文件        */
#if LW_CFG_POSIXEX_EN > 0
#include "../include/px_pthread_np.h"
#endif
#include "../include/posixLib.h"                                        /*  posix 内部公共库            */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
#if (LW_CFG_GJB7714_EN > 0) && (LW_CFG_MODULELOADER_EN > 0)
#include "unistd.h"
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif
/*********************************************************************************************************
** 函数名称: pthread_atfork
** 功能描述: 设置线程在 fork() 时需要执行的函数.
** 输　入  : prepare       prepare 函数指针
**           parent        父进程执行函数指针
**           child         子进程执行函数指针
**           arg           线程入口参数
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_atfork (void (*prepare)(void), void (*parent)(void), void (*child)(void))
{
    errno = ENOSYS;
    return  (ENOSYS);
}
/*********************************************************************************************************
** 函数名称: pthread_create
** 功能描述: 创建一个 posix 线程.
** 输　入  : pthread       线程 id (返回).
**           pattr         创建属性
**           start_routine 线程入口
**           arg           线程入口参数
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_create (pthread_t              *pthread, 
                     const pthread_attr_t   *pattr, 
                     void                   *(*start_routine)(void *),
                     void                   *arg)
{
    LW_OBJECT_HANDLE        ulId;
    LW_CLASS_THREADATTR     lwattr;
    PLW_CLASS_TCB           ptcbCur;
    PCHAR                   pcName = "pthread";
    INT                     iErrNo;
    
#if LW_CFG_POSIXEX_EN > 0
    __PX_VPROC_CONTEXT     *pvpCtx = _posixVprocCtxGet();
#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */

    if (start_routine == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    lwattr = API_ThreadAttrGetDefault();                                /*  获得默认线程属性            */
    
    if (pattr) {
        pcName = pattr->PTHREADATTR_pcName;                             /*  使用 attr 作为创建线程名    */
        
        if (pattr->PTHREADATTR_stStackGuard > pattr->PTHREADATTR_stStackByteSize) {
            lwattr.THREADATTR_stGuardSize = LW_CFG_THREAD_DEFAULT_GUARD_SIZE;
        } else {
            lwattr.THREADATTR_stGuardSize = pattr->PTHREADATTR_stStackGuard;
        }
        
        if (pattr->PTHREADATTR_stStackByteSize == 0) {                  /*  继承创建者                  */
            lwattr.THREADATTR_stStackByteSize = ptcbCur->TCB_stStackSize * sizeof(LW_STACK);
        } else {
            lwattr.THREADATTR_pstkLowAddr     = (PLW_STACK)pattr->PTHREADATTR_pvStackAddr;
            lwattr.THREADATTR_stStackByteSize = pattr->PTHREADATTR_stStackByteSize;
        }
        
        if (pattr->PTHREADATTR_iInherit == PTHREAD_INHERIT_SCHED) {     /*  是否继承优先级              */
            lwattr.THREADATTR_ucPriority =  ptcbCur->TCB_ucPriority;
        } else {
            lwattr.THREADATTR_ucPriority = 
                (UINT8)PX_PRIORITY_CONVERT(pattr->PTHREADATTR_schedparam.sched_priority);
        }
                                                                        /*  总是填充与检查堆栈使用情况  */
        lwattr.THREADATTR_ulOption = pattr->PTHREADATTR_ulOption | LW_OPTION_THREAD_STK_CHK;
    
    } else
#if LW_CFG_POSIXEX_EN > 0
           if (pvpCtx->PVPCTX_iThreadDefMethod == PTHREAD_NULL_ATTR_METHOD_USE_INHERIT)
#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */
    {                                                                   /*  堆栈大小和优先级全部继承    */
        lwattr.THREADATTR_stStackByteSize = ptcbCur->TCB_stStackSize * sizeof(LW_STACK);
        lwattr.THREADATTR_ucPriority      = ptcbCur->TCB_ucPriority;
    }
    
    lwattr.THREADATTR_pvArg = arg;                                      /*  记录参数                    */
    
    /*
     *  初始化线程.
     */
    ulId = API_ThreadInit(pcName, (PTHREAD_START_ROUTINE)start_routine, &lwattr, LW_NULL);
    if (ulId == 0) {
        iErrNo = errno;
        switch (iErrNo) {
        
        case ERROR_THREAD_FULL:
            errno = EAGAIN;
            return  (EAGAIN);
            
        case ERROR_KERNEL_LOW_MEMORY:
            errno = ENOMEM;
            return  (ENOMEM);
        
        default:
            return  (iErrNo);
        }
    }
    
    if (pattr) {
        UINT8       ucPolicy        = (UINT8)pattr->PTHREADATTR_iSchedPolicy;
        UINT8       ucActivatedMode = LW_OPTION_RESPOND_AUTO;           /*  默认响应方式                */
        
        if (pattr->PTHREADATTR_iInherit == PTHREAD_INHERIT_SCHED) {     /*  继承调度策略                */
            API_ThreadGetSchedParam(ptcbCur->TCB_ulId, &ucPolicy, &ucActivatedMode);
        }
        
        API_ThreadSetSchedParam(ulId, ucPolicy, ucActivatedMode);       /*  设置调度策略                */
    }
    
    if (pthread) {
        *pthread = ulId;                                                /*  保存线程句柄                */
    }
    
    API_ThreadStart(ulId);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_cancel
** 功能描述: cancel 一个 posix 线程.
** 输　入  : thread       线程 id.
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_cancel (pthread_t  thread)
{
    ULONG   ulError;

    PX_ID_VERIFY(thread, pthread_t);
    
    ulError = API_ThreadCancel(&thread);
    switch (ulError) {
    
    case ERROR_KERNEL_HANDLE_NULL:
    case ERROR_THREAD_NULL:
        errno = ESRCH;
        return  (ESRCH);
        
    default:
        return  ((int)ulError);
    }
}
/*********************************************************************************************************
** 函数名称: pthread_testcancel
** 功能描述: testcancel 当前 posix 线程.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
void  pthread_testcancel (void)
{
    API_ThreadTestCancel();
}
/*********************************************************************************************************
** 函数名称: pthread_join
** 功能描述: join 一个 posix 线程.
** 输　入  : thread       线程 id.
**           ppstatus     获取线程返回值
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_join (pthread_t  thread, void **ppstatus)
{
    ULONG   ulError;
    
    ulError = API_ThreadJoin(thread, ppstatus);
    switch (ulError) {
    
    case ERROR_KERNEL_HANDLE_NULL:
    case ERROR_THREAD_NULL:
        errno = ESRCH;
        return  (ESRCH);
        
    case ERROR_THREAD_JOIN_SELF:
        errno = EDEADLK;
        return  (EDEADLK);
        
    case ERROR_THREAD_DETACHED:
        errno = EINVAL;
        return  (EINVAL);
        
    default:
        return  ((int)ulError);
    }
}
/*********************************************************************************************************
** 函数名称: pthread_detach
** 功能描述: detach 一个 posix 线程.
** 输　入  : thread       线程 id.
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_detach (pthread_t  thread)
{
    ULONG   ulError;

    PX_ID_VERIFY(thread, pthread_t);
    
    ulError = API_ThreadDetach(thread);
    switch (ulError) {
    
    case ERROR_KERNEL_HANDLE_NULL:
    case ERROR_THREAD_NULL:
        errno = ESRCH;
        return  (ESRCH);
        
    case ERROR_THREAD_DETACHED:
        errno = EINVAL;
        return  (EINVAL);
        
    default:
        return  ((int)ulError);
    }
}
/*********************************************************************************************************
** 函数名称: pthread_equal
** 功能描述: 判断两个 posix 线程是否相同.
** 输　入  : thread1       线程 id.
**           thread2       线程 id.
** 输　出  : true or false
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_equal (pthread_t  thread1, pthread_t  thread2)
{
    PX_ID_VERIFY(thread1, pthread_t);
    PX_ID_VERIFY(thread2, pthread_t);

    return  (thread1 == thread2);
}
/*********************************************************************************************************
** 函数名称: pthread_exit
** 功能描述: 删除当前 posix 线程.
** 输　入  : status        exit 代码
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
void  pthread_exit (void *status)
{
    LW_OBJECT_HANDLE    ulId = API_ThreadIdSelf();

    API_ThreadDelete(&ulId, status);
}
/*********************************************************************************************************
** 函数名称: pthread_self
** 功能描述: 获得当前 posix 线程句柄.
** 输　入  : NONE
** 输　出  : 句柄
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
pthread_t  pthread_self (void)
{
    return  (API_ThreadIdSelf());
}
/*********************************************************************************************************
** 函数名称: pthread_yield
** 功能描述: 将当前任务插入到同优先级调度器链表的最后, 主动让出一次 CPU 调度.
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_yield (void)
{
    API_ThreadYield(API_ThreadIdSelf());
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_kill
** 功能描述: 向指定 posix 线程发送一个信号.
** 输　入  : thread        线程句柄
**           signo         信号
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_SIGNAL_EN > 0

LW_API 
int  pthread_kill (pthread_t  thread, int signo)
{
    int  error, err_no;

    PX_ID_VERIFY(thread, pthread_t);
    
    if (thread < LW_CFG_MAX_THREADS) {
        errno = ESRCH;
        return  (ESRCH);
    }
    
    error = kill(thread, signo);
    if (error < ERROR_NONE) {
        err_no = errno;
        switch (err_no) {
    
        case ERROR_KERNEL_HANDLE_NULL:
        case ERROR_THREAD_NULL:
            errno = ESRCH;
            return  (ESRCH);
            
        default:
            return  (err_no);
        }
    
    } else {
        return  (error);
    }
}
/*********************************************************************************************************
** 函数名称: pthread_sigmask
** 功能描述: 修改 posix 线程信号掩码.
** 输　入  : how           方法
**           newmask       新的信号掩码
**           oldmask       旧的信号掩码
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_sigmask (int  how, const sigset_t  *newmask, sigset_t *oldmask)
{
    int  error, err_no;
    
    error = sigprocmask(how, newmask, oldmask);
    if (error < ERROR_NONE) {
        err_no = errno;
        switch (err_no) {
    
        case ERROR_KERNEL_HANDLE_NULL:
        case ERROR_THREAD_NULL:
            errno = ESRCH;
            return  (ESRCH);
            
        default:
            return  (err_no);
        }
    
    } else {
        return  (error);
    }
}

#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
/*********************************************************************************************************
** 函数名称: pthread_cleanup_pop
** 功能描述: 将一个压栈函数运行并释放
** 输　入  : iNeedRun          是否执行
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
void  pthread_cleanup_pop (int  iNeedRun)
{
    API_ThreadCleanupPop((BOOL)iNeedRun);
}
/*********************************************************************************************************
** 函数名称: pthread_cleanup_push
** 功能描述: 将一个清除函数压入函数堆栈
** 输　入  : pfunc         函数指针
**           arg           函数参数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
void  pthread_cleanup_push (void (*pfunc)(void *), void *arg)
{
    API_ThreadCleanupPush(pfunc, arg);
}
/*********************************************************************************************************
** 函数名称: pthread_getschedparam
** 功能描述: 获得调度器参数
** 输　入  : thread        线程句柄
**           piPolicy      调度策略(返回)
**           pschedparam   调度器参数(返回)
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_getschedparam (pthread_t            thread, 
                            int                 *piPolicy, 
                            struct sched_param  *pschedparam)
{
    UINT8       ucPriority;
    UINT8       ucPolicy;
    
    PX_ID_VERIFY(thread, pthread_t);
    
    if (pschedparam) {
        if (API_ThreadGetPriority(thread, &ucPriority)) {
            errno = ESRCH;
            return  (ESRCH);
        }
        pschedparam->sched_priority = PX_PRIORITY_CONVERT(ucPriority);
    }
    
    if (piPolicy) {
        if (API_ThreadGetSchedParam(thread, &ucPolicy, LW_NULL)) {
            errno = ESRCH;
            return  (ESRCH);
        }
        *piPolicy = (int)ucPolicy;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_getschedparam
** 功能描述: 设置调度器参数
** 输　入  : thread        线程句柄
**           iPolicy       调度策略
**           pschedparam   调度器参数
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_setschedparam (pthread_t                  thread, 
                            int                        iPolicy, 
                            const struct sched_param  *pschedparam)
{
    UINT8       ucPriority;
    UINT8       ucActivatedMode;

    if (pschedparam == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((iPolicy != LW_OPTION_SCHED_FIFO) &&
        (iPolicy != LW_OPTION_SCHED_RR)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if ((pschedparam->sched_priority < __PX_PRIORITY_MIN) ||
        (pschedparam->sched_priority > __PX_PRIORITY_MAX)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    ucPriority= (UINT8)PX_PRIORITY_CONVERT(pschedparam->sched_priority);
    
    PX_ID_VERIFY(thread, pid_t);
    
    if (API_ThreadGetSchedParam(thread,
                                LW_NULL,
                                &ucActivatedMode)) {
        errno = ESRCH;
        return  (ESRCH);
    }
    
    API_ThreadSetSchedParam(thread, (UINT8)iPolicy, ucActivatedMode);
    
    if (API_ThreadSetPriority(thread, ucPriority)) {
        errno = ESRCH;
        return  (ESRCH);
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: pthread_onec
** 功能描述: 线程安全的仅执行一次指定函数
** 输　入  : thread        线程句柄
**           once          onec_t参数
**           pfunc         函数指针
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_once (pthread_once_t  *once, void (*pfunc)(void))
{
    int  error;
    
    error = API_ThreadOnce(once, pfunc);
    if (error < ERROR_NONE) {
        return  (errno);
    } else {
        return  (error);
    }
}
/*********************************************************************************************************
** 函数名称: pthread_setschedprio
** 功能描述: 设置线程优先级
** 输　入  : thread        线程句柄
**           prio          优先级
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_setschedprio (pthread_t  thread, int  prio)
{
    UINT8       ucPriority;

    if ((prio < __PX_PRIORITY_MIN) ||
        (prio > __PX_PRIORITY_MAX)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    PX_ID_VERIFY(thread, pthread_t);
    
    ucPriority= (UINT8)PX_PRIORITY_CONVERT(prio);
    
    if (API_ThreadSetPriority(thread, ucPriority)) {
        errno = ESRCH;
        return  (ESRCH);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_getschedprio
** 功能描述: 获得线程优先级
** 输　入  : thread        线程句柄
**           prio          优先级(返回)
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_getschedprio (pthread_t  thread, int  *prio)
{
    UINT8       ucPriority;

    PX_ID_VERIFY(thread, pthread_t);

    if (prio) {
        if (API_ThreadGetPriority(thread, &ucPriority)) {
            errno = ESRCH;
            return  (ESRCH);
        }
        *prio = PX_PRIORITY_CONVERT(ucPriority);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_setcancelstate
** 功能描述: 设置取消线程是否使能
** 输　入  : newstate      新的状态
**           poldstate     先前的状态(返回)
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_setcancelstate (int  newstate, int  *poldstate)
{
    ULONG   ulError = API_ThreadSetCancelState(newstate, poldstate);
    
    if (ulError) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_setcanceltype
** 功能描述: 设置当前线程被动取消时的动作
** 输　入  : newtype      新的动作
**           poldtype     先前的动作(返回)
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_setcanceltype (int  newtype, int  *poldtype)
{
    ULONG   ulError = API_ThreadSetCancelType(newtype, poldtype);
    
    if (ulError) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_setconcurrency
** 功能描述: 设置线程新的并行级别
** 输　入  : newlevel      新并行级别
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_setconcurrency (int  newlevel)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_getconcurrency
** 功能描述: 获得当前线程的并行级别
** 输　入  : NONE
** 输　出  : 当前线程的并行级别
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_getconcurrency (void)
{
    return  (LW_CFG_MAX_THREADS);
}
/*********************************************************************************************************
** 函数名称: pthread_getcpuclockid
** 功能描述: 获得线程 CPU 时间 clock id.
** 输　入  : thread    线程句柄
**           clock_id  时钟 id
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_getcpuclockid (pthread_t thread, clockid_t *clock_id)
{
    if (!clock_id) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *clock_id = CLOCK_THREAD_CPUTIME_ID;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_null_attr_method_np
** 功能描述: 设置创建线程时当线程的属性为 NULL 时的处理方法
** 输　入  : method        新的操作类型
**           old_method    之前的操作类型
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_POSIXEX_EN > 0

LW_API int  pthread_null_attr_method_np (int  method, int *old_method)
{
    __PX_VPROC_CONTEXT  *pvpCtx = _posixVprocCtxGet();

    if (old_method) {
        *old_method = pvpCtx->PVPCTX_iThreadDefMethod;
    }

    if ((method == PTHREAD_NULL_ATTR_METHOD_USE_INHERIT) ||
        (method == PTHREAD_NULL_ATTR_METHOD_USE_DEFSETTING)) {
        pvpCtx->PVPCTX_iThreadDefMethod = method;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_setname_np
** 功能描述: 设置线程名字
** 输　入  : thread    线程句柄
**           name      线程名字
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_setname_np (pthread_t  thread, const char  *name)
{
    ULONG   ulError;
    
    PX_ID_VERIFY(thread, pthread_t);

    ulError = API_ThreadSetName(thread, name);
    if (ulError == ERROR_KERNEL_PNAME_TOO_LONG) {
        errno = ERANGE;
        return  (ERANGE);
    
    } else if (ulError) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_getname_np
** 功能描述: 获得线程名字
** 输　入  : thread    线程句柄
**           name      线程名字
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_getname_np (pthread_t  thread, char  *name, size_t len)
{
    ULONG   ulError;
    CHAR    cNameBuffer[LW_CFG_OBJECT_NAME_SIZE];
    
    PX_ID_VERIFY(thread, pthread_t);
    
    ulError = API_ThreadGetName(thread, cNameBuffer);
    if (ulError) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (lib_strlen(cNameBuffer) >= len) {
        errno = ERANGE;
        return  (ERANGE);
    }
    
    lib_strcpy(name, cNameBuffer);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_safe_np
** 功能描述: 线程进入安全模式, 任何对本线程的删除操作都会推迟到线程退出安全模式时进行.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
void  pthread_safe_np (void)
{
    API_ThreadSafe();
}
/*********************************************************************************************************
** 函数名称: pthread_unsafe_np
** 功能描述: 线程退出安全模式.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
void  pthread_unsafe_np (void)
{
    API_ThreadUnsafe();
}
/*********************************************************************************************************
** 函数名称: pthread_int_lock_np
** 功能描述: 线程锁定当前所在核中断, 不再响应中断.
** 输　入  : irqctx        体系结构相关中断状态保存结构 (用户不需要关心具体内容)
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_int_lock_np (pthread_int_t *irqctx)
{
    if (!irqctx) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *irqctx = KN_INT_DISABLE();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_int_unlock_np
** 功能描述: 线程解锁当前所在核中断, 开始响应中断.
** 输　入  : irqctx        体系结构相关中断状态保存结构 (用户不需要关心具体内容)
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_int_unlock_np (pthread_int_t irqctx)
{
    KN_INT_ENABLE(irqctx);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_setaffinity_np
** 功能描述: 设置线程调度的 CPU 集合
** 输　入  : pid           进程 / 线程 ID
**           setsize       CPU 集合大小
**           set           CPU 集合
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : SylixOS 目前仅支持将任务锁定到一个 CPU, 所以这里只能将线程锁定到指定的最小 CPU 号上.
**
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_setaffinity_np (pthread_t  thread, size_t setsize, const cpu_set_t *set)
{
#if LW_CFG_SMP_EN > 0
    if (!setsize || !set) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_ThreadSetAffinity(thread, setsize, (PLW_CLASS_CPUSET)set)) {
        errno = ESRCH;
        return  (ESRCH);
    }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_getaffinity_np
** 功能描述: 获取线程调度的 CPU 集合
** 输　入  : pid           进程 / 线程 ID
**           setsize       CPU 集合大小
**           set           CPU 集合
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_getaffinity_np (pthread_t  thread, size_t setsize, cpu_set_t *set)
{
#if LW_CFG_SMP_EN > 0
    if ((setsize < sizeof(cpu_set_t)) || !set) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_ThreadGetAffinity(thread, setsize, set)) {
        errno = ESRCH;
        return  (ESRCH);
    }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_POSIXEX_EN > 0       */
/*********************************************************************************************************
** 函数名称: pthread_getid
** 功能描述: 通过线程名获得线程句柄
** 输　入  : name          线程名
**           pthread       线程句柄
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_GJB7714_EN > 0

LW_API 
int  pthread_getid (const char *name, pthread_t *pthread)
{
    INT     i;

#if LW_CFG_MODULELOADER_EN > 0
    pid_t   pid = getpid();
#else
    pid_t   pid = 0;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    PLW_CLASS_TCB      ptcb;
    LW_CLASS_TCB_DESC  tcbdesc;

    if (!name || !pthread) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    for (i = 0; i < LW_CFG_MAX_THREADS; i++) {
        ptcb = _K_ptcbTCBIdTable[i];                                    /*  获得 TCB 控制块             */
        if (ptcb == LW_NULL) {                                          /*  线程不存在                  */
            continue;
        }
        
        if (API_ThreadDesc(ptcb->TCB_ulId, &tcbdesc)) {
            continue;
        }
        
#if LW_CFG_MODULELOADER_EN > 0
        if (pid != vprocGetPidByTcbdesc(&tcbdesc)) {
            continue;
        }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
        if (lib_strcmp(name, tcbdesc.TCBD_cThreadName)) {
            continue;
        }
        
        break;
    }
    
    if (i < LW_CFG_MAX_THREADS) {
        *pthread = tcbdesc.TCBD_ulId;
        return  (ERROR_NONE);
        
    } else {
        errno = ESRCH;
        return  (ESRCH);
    }
}
/*********************************************************************************************************
** 函数名称: pthread_getname
** 功能描述: 获得线程名
** 输　入  : pthread       线程句柄
**           name          线程名
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_getname (pthread_t thread, char *name)
{
    CHAR    cNameBuffer[LW_CFG_OBJECT_NAME_SIZE];
    
    if (!name) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    PX_ID_VERIFY(thread, pthread_t);
    
    if (API_ThreadGetName(thread, cNameBuffer)) {
        errno = ESRCH;
        return  (ESRCH);
    }

    lib_strcpy(name, cNameBuffer);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_suspend
** 功能描述: 线程挂起
** 输　入  : pthread       线程句柄
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_suspend (pthread_t thread)
{
    PX_ID_VERIFY(thread, pthread_t);
    
    if (API_ThreadSuspend(thread)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_resume
** 功能描述: 线程解除挂起
** 输　入  : pthread       线程句柄
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_resume (pthread_t thread)
{
    PX_ID_VERIFY(thread, pthread_t);
    
    if (API_ThreadResume(thread)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_delay
** 功能描述: 线程等待
** 输　入  : ticks       延迟时钟数
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_delay (int  ticks)
{
    ULONG   ulTick;
    
    if (ticks < 0) {
        return  (ERROR_NONE);
    }
    
    if (ticks == 0) {
        API_ThreadYield(API_ThreadIdSelf());
        return  (ERROR_NONE);
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        errno = ENOTSUP;
        return  (ENOTSUP);
    }
    
    ulTick = (ULONG)ticks;
    
    return  ((int)API_TimeSleepEx(ulTick, LW_TRUE));
}
/*********************************************************************************************************
** 函数名称: pthread_lock
** 功能描述: 线程锁定当前 CPU 调度
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_lock (void)
{
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        errno = ECALLEDINISR;
        return  (ECALLEDINISR);
    }

    API_ThreadLock();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_unlock
** 功能描述: 线程解锁当前 CPU 调度
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_unlock (void)
{
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        errno = ECALLEDINISR;
        return  (ECALLEDINISR);
    }
    
    API_ThreadUnlock();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_is_ready
** 功能描述: 线程是否就绪
** 输　入  : pthread       线程句柄
** 输　出  : 是否就绪
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
boolean  pthread_is_ready (pthread_t thread)
{
    PX_ID_VERIFY(thread, pthread_t);
    
    if (!API_ThreadVerify(thread)) {
        errno = ESRCH;
        return  (LW_FALSE);
    }
    
    return  (API_ThreadIsReady(thread));
}
/*********************************************************************************************************
** 函数名称: pthread_is_suspend
** 功能描述: 线程是否挂起
** 输　入  : pthread       线程句柄
** 输　出  : 是否挂起
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
boolean  pthread_is_suspend (pthread_t thread)
{
    PX_ID_VERIFY(thread, pthread_t);
    
    if (!API_ThreadVerify(thread)) {
        errno = ESRCH;
        return  (LW_FALSE);
    }
    
    if (API_ThreadIsSuspend(thread)) {
        return  (LW_TRUE);
    
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** 函数名称: pthread_verifyid
** 功能描述: 检查指定线程是否存在
** 输　入  : pthread       线程句柄
** 输　出  : 0: 任务存在 -1: 任务不存在
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_verifyid (pthread_t thread)
{
    PX_ID_VERIFY(thread, pthread_t);
    
    if (API_ThreadVerify(thread)) {
        return  (0);
    
    } else {
        return  (-1);
    }
}
/*********************************************************************************************************
** 函数名称: pthread_cancelforce
** 功能描述: 强制线程删除
** 输　入  : pthread       线程句柄
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_cancelforce (pthread_t thread)
{
    PX_ID_VERIFY(thread, pthread_t);
    
    if (API_ThreadForceDelete(&thread, LW_NULL)) {
        errno = ESRCH;
        return  (ESRCH);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_getinfo
** 功能描述: 获得线程信息
** 输　入  : pthread       线程句柄
**           info          线程信息
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_getinfo (pthread_t thread, pthread_info_t *info)
{
    if (!info) {
        errno = EINVAL;
        return  (EINVAL);
    }

    PX_ID_VERIFY(thread, pthread_t);
    
    if (API_ThreadDesc(thread, info)) {
        errno = ESRCH;
        return  (ESRCH);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_getregs
** 功能描述: 获得线程寄存器表
** 输　入  : pthread       线程句柄
**           pregs         线程寄存器表
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_getregs (pthread_t thread, REG_SET *pregs)
{
    UINT16         usIndex;
    PLW_CLASS_TCB  ptcb;
    ARCH_REG_CTX  *pregctxGet;
    ARCH_REG_T     regSp;
    
    if (!pregs) {
        errno = EINVAL;
        return  (EINVAL);
    }

    PX_ID_VERIFY(thread, pthread_t);
    
    if (thread == API_ThreadIdSelf()) {
        errno = ENOTSUP;
        return  (ENOTSUP);
    }
    
    usIndex = _ObjectGetIndex(thread);
    
    if (!_ObjectClassOK(thread, _OBJECT_THREAD)) {                      /*  检查 ID 类型有效性          */
        errno = ESRCH;
        return  (ESRCH);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        errno = ESRCH;
        return  (ESRCH);
    }
    
    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        errno = ESRCH;
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    pregctxGet = archTaskRegsGet(ptcb->TCB_pstkStackNow, &regSp);
    *pregs     = *pregctxGet;
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_show
** 功能描述: 显示线程信息
** 输　入  : pthread       线程句柄
**           level         显示等级 (未使用)
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_show (pthread_t thread, int level)
{
#if LW_CFG_MODULELOADER_EN > 0
    pid_t   pid = getpid();
#else
    pid_t   pid = 0;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

    (VOID)thread;

    API_ThreadShowEx(pid);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_showstack
** 功能描述: 显示线程堆栈信息
** 输　入  : pthread       线程句柄
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_showstack (pthread_t thread)
{
    (VOID)thread;
    
    API_StackShow();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_showstackframe
** 功能描述: 显示线程调用栈信息
** 输　入  : pthread       线程句柄
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_showstackframe (pthread_t thread)
{
    PX_ID_VERIFY(thread, pthread_t);
    
    if (thread != pthread_self()) {
        errno = ENOTSUP;
        return  (ENOTSUP);
    }

    API_BacktraceShow(STD_OUT, 100);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_addvar
** 功能描述: 线程加入私有变量
** 输　入  : pthread       线程句柄
**           pvar          私有变量地址
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if (LW_CFG_SMP_EN == 0) && (LW_CFG_CPU_WORD_LENGHT == 32)

LW_API 
int  pthread_addvar (pthread_t thread, int *pvar)
{
    ULONG   ulError;

    if (!pvar) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    PX_ID_VERIFY(thread, pthread_t);
    
    ulError = API_ThreadVarAdd(thread, (ULONG *)pvar);
    if (ulError == ERROR_THREAD_VAR_FULL) {
        errno = ENOSPC;
        return  (ENOSPC);
    
    } else if (ulError) {
        errno = ESRCH;
        return  (ESRCH);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_delvar
** 功能描述: 线程删除私有变量
** 输　入  : pthread       线程句柄
**           pvar          私有变量地址
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_delvar (pthread_t thread, int *pvar)
{
    if (!pvar) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    PX_ID_VERIFY(thread, pthread_t);

    if (API_ThreadVarDelete(thread, (ULONG *)pvar)) {
        errno = ESRCH;
        return  (ESRCH);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_setvar
** 功能描述: 设置线程私有变量
** 输　入  : pthread       线程句柄
**           pvar          私有变量地址
**           value         变量值
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_setvar (pthread_t thread, int *pvar, int value)
{
    if (!pvar) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    PX_ID_VERIFY(thread, pthread_t);

    if (API_ThreadVarSet(thread, (ULONG *)pvar, (ULONG)value)) {
        errno = ESRCH;
        return  (ESRCH);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_getvar
** 功能描述: 获得线程私有变量
** 输　入  : pthread       线程句柄
**           pvar          私有变量地址
**           value         变量值
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_getvar (pthread_t thread, int *pvar, int *value)
{
    if (!pvar || !value) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    PX_ID_VERIFY(thread, pthread_t);

    *value = (int)API_ThreadVarGet(thread, (ULONG *)pvar);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SMP_EN == 0          */
/*********************************************************************************************************
** 函数名称: pthread_create_hook_add
** 功能描述: 添加一个任务创建 HOOK
** 输　入  : create_hook   任务创建 HOOK
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_create_hook_add (OS_STATUS (*create_hook)(pthread_t))
{
#if LW_CFG_MODULELOADER_EN > 0
    if (getpid()) {
        errno = EACCES;
        return  (EACCES);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    if (!create_hook) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_SystemHookAdd((LW_HOOK_FUNC)create_hook, 
                          LW_OPTION_THREAD_CREATE_HOOK)) {
        errno = EAGAIN;
        return  (EAGAIN);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_create_hook_delete
** 功能描述: 删除一个任务创建 HOOK
** 输　入  : create_hook   任务创建 HOOK
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_create_hook_delete (OS_STATUS (*create_hook)(pthread_t))
{
#if LW_CFG_MODULELOADER_EN > 0
    if (getpid()) {
        errno = EACCES;
        return  (EACCES);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    if (!create_hook) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_SystemHookDelete((LW_HOOK_FUNC)create_hook, 
                             LW_OPTION_THREAD_CREATE_HOOK)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_switch_hook_add
** 功能描述: 添加一个任务切换 HOOK
** 输　入  : switch_hook   任务切换 HOOK
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_switch_hook_add (OS_STATUS (*switch_hook)(pthread_t, pthread_t))
{
#if LW_CFG_MODULELOADER_EN > 0
    if (getpid()) {
        errno = EACCES;
        return  (EACCES);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    if (!switch_hook) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_SystemHookAdd((LW_HOOK_FUNC)switch_hook, 
                          LW_OPTION_THREAD_SWAP_HOOK)) {
        errno = EAGAIN;
        return  (EAGAIN);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_switch_hook_delete
** 功能描述: 删除一个任务切换 HOOK
** 输　入  : switch_hook   任务切换 HOOK
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_switch_hook_delete (OS_STATUS (*switch_hook)(pthread_t, pthread_t))
{
#if LW_CFG_MODULELOADER_EN > 0
    if (getpid()) {
        errno = EACCES;
        return  (EACCES);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    if (!switch_hook) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_SystemHookDelete((LW_HOOK_FUNC)switch_hook, 
                             LW_OPTION_THREAD_SWAP_HOOK)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_close_hook_add
** 功能描述: 添加一个任务删除 HOOK
** 输　入  : close_hook    任务删除 HOOK
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_close_hook_add (OS_STATUS (*close_hook)(pthread_t))
{
#if LW_CFG_MODULELOADER_EN > 0
    if (getpid()) {
        errno = EACCES;
        return  (EACCES);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    if (!close_hook) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_SystemHookAdd((LW_HOOK_FUNC)close_hook, 
                          LW_OPTION_THREAD_DELETE_HOOK)) {
        errno = EAGAIN;
        return  (EAGAIN);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: pthread_close_hook_delete
** 功能描述: 删除一个任务删除 HOOK
** 输　入  : close_hook   任务删除 HOOK
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  pthread_close_hook_delete (OS_STATUS (*close_hook)(pthread_t))
{
#if LW_CFG_MODULELOADER_EN > 0
    if (getpid()) {
        errno = EACCES;
        return  (EACCES);
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
    
    if (!close_hook) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (API_SystemHookDelete((LW_HOOK_FUNC)close_hook, 
                             LW_OPTION_THREAD_DELETE_HOOK)) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_GJB7714_EN > 0       */
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

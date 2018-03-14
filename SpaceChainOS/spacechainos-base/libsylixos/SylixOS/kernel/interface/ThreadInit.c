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
** 文   件   名: ThreadInit.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 18 日
**
** 描        述: 这是系统建立线程属性块函数。

** BUG
2007.01.08  对内存开辟可以不使用 Lock() 和 Unlok()
2007.07.18  加入了 _DebugHandle() 功能
2007.11.08  将用户堆命名为内核堆.
2007.12.24  在进入 BuildTcb 时需要进入安全模式, 最后再退出安全模式.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2008.05.31  使用 __KERNEL_MODE_...().
2008.08.17  加入 TCB 清零.
2009.02.03  加入 TCB 扩展结构的创建
2009.04.06  将堆栈填充字节确定为 LW_CFG_STK_EMPTY_FLAG. 以提高堆栈检查的准确性.
2009.05.26  pthreadattr 为空时, 使用默认属性创建线程.
2009.06.02  修改了对 pthreadattr 判断的位置. 去掉了对浮点堆栈的判断改由 BSP 完成.
2010.01.22  _K_usThreadCounter 进入内核后修改.
2012.03.21  使用新的 thread attr 获取方式.
2013.05.07  TCB 不再放在堆栈中, 而是从 TCB 池中开始分配.
2013.08.27  加入内核事件监控器.
2013.09.17  加入线程堆栈警戒指针处理.
2015.11.24  进程内的线程使用 vmm 堆栈.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadInit
** 功能描述: 初始化一个线程
** 输　入  : pcName                 线程名
**           pfuncThread            指线程代码段起始地址
**           pthreadattr            线程属性集合指针
**           pulId                  线程生成的ID指针     可以为 NULL
** 输　出  : pulId                  线程句柄             同 ID 一个概念
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
LW_OBJECT_HANDLE  API_ThreadInit (CPCHAR                   pcName,
                                  PTHREAD_START_ROUTINE    pfuncThread,
                                  PLW_CLASS_THREADATTR     pthreadattr,
                                  LW_OBJECT_ID            *pulId)
{
    static LW_CLASS_THREADATTR     threadattrDefault;

    REGISTER PLW_STACK             pstkTop;
    REGISTER PLW_STACK             pstkButtom;
    REGISTER PLW_STACK             pstkGuard;
    REGISTER PLW_STACK             pstkLowAddress;
    REGISTER PLW_STACK             pstkFristFree;
    
             size_t                stStackSize;                         /*  堆栈大小(单位：字)          */
             size_t                stGuardSize;
                         
             PLW_CLASS_TCB         ptcb;
             ULONG                 ulIdTemp;
             
             ULONG                 ulError;
             INT                   iErrLevel = 0;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (threadattrDefault.THREADATTR_stStackByteSize == 0) {
        threadattrDefault = API_ThreadAttrGetDefault();                 /*  初始化默认属性              */
    }
    
    if (pthreadattr == LW_NULL) {
        pthreadattr = &threadattrDefault;                               /*  使用默认属性                */
    }                                                                   /*  默认属性总是使用自动分配堆栈*/
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pfuncThread) {                                                 /*  指线程代码段起始地址为空    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread code segment not found.\r\n");
        _ErrorHandle(EINVAL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (_StackSizeCheck(pthreadattr->THREADATTR_stStackByteSize)) {     /*  堆栈大小不正确              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread stack size low.\r\n");
        _ErrorHandle(ERROR_THREAD_STACKSIZE_LACK);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (_PriorityCheck(pthreadattr->THREADATTR_ucPriority)) {           /*  优先级错误                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread priority invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_PRIORITY_WRONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
#endif

    stGuardSize = pthreadattr->THREADATTR_stGuardSize;

    if (stGuardSize < (ARCH_STK_MIN_WORD_SIZE * sizeof(LW_STACK))) {
        stGuardSize = LW_CFG_THREAD_DEFAULT_GUARD_SIZE;
    }

    if (stGuardSize > pthreadattr->THREADATTR_stStackByteSize) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread stack guard size invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_STACK_NULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (_Object_Name_Invalid(pcName)) {                                 /*  检查名字有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
        _ErrorHandle(ERROR_KERNEL_PNAME_TOO_LONG);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    __KERNEL_MODE_PROC(
        ptcb = _Allocate_Tcb_Object();                                  /*  获得一个 TCB                */
    );
    
    if (!ptcb) {                                                        /*  检查是否可以建立线程        */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is no ID to buide a thread.\r\n");
        _ErrorHandle(ERROR_THREAD_FULL);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    if (LW_SYS_STATUS_IS_RUNNING()) {
        _ThreadSafeInternal();                                          /*  进入安全模式                */
    }
    
    lib_bzero(&ptcb->TCB_pstkStackTop, 
              sizeof(LW_CLASS_TCB) - 
              _LIST_OFFSETOF(LW_CLASS_TCB, TCB_pstkStackTop));          /*  TCB 清零                    */
    
    if (!pthreadattr->THREADATTR_pstkLowAddr) {                         /*  是否从内核堆中开辟堆栈      */
        pstkLowAddress = _StackAllocate(ptcb,
                                        pthreadattr->THREADATTR_ulOption,
                                        pthreadattr->THREADATTR_stStackByteSize);
        if (!pstkLowAddress) {                                          /*  开辟失败                    */
            iErrLevel = 1;
            _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel low memory.\r\n");
            _ErrorHandle(ERROR_KERNEL_LOW_MEMORY);
            goto    __error_handle;
        }
    
    } else {
        pstkLowAddress = pthreadattr->THREADATTR_pstkLowAddr;           /*  堆栈的低地址                */
    }
    
    stStackSize = _CalWordAlign(pthreadattr->THREADATTR_stStackByteSize);
    stGuardSize = _CalWordAlign(stGuardSize);

#if CPU_STK_GROWTH == 0                                                 /*  寻找堆栈头尾                */
    pstkTop    = pstkLowAddress;
    pstkButtom = pstkLowAddress + stStackSize - 1;
    pstkGuard  = pstkButtom - stGuardSize - 1;
#else
    pstkTop    = pstkLowAddress + stStackSize - 1;
    pstkButtom = pstkLowAddress;
    pstkGuard  = pstkButtom + stGuardSize - 1;
#endif                                                                  /*  CPU_STK_GROWTH == 0         */

    ulIdTemp = _MakeObjectId(_OBJECT_THREAD, 
                             LW_CFG_PROCESSOR_NUMBER, 
                             ptcb->TCB_usIndex);                        /*  构建对象 id                 */
    
    if (pthreadattr->THREADATTR_ulOption & LW_OPTION_THREAD_STK_CLR) {
        lib_memset(pstkLowAddress, LW_CFG_STK_EMPTY_FLAG, 
                   pthreadattr->THREADATTR_stStackByteSize);            /*  需要清除堆栈                */
    }
                                                                        /*  初始化堆栈，SHELL           */
    pstkFristFree = archTaskCtxCreate((PTHREAD_START_ROUTINE)_ThreadShell, 
                                      (PVOID)pfuncThread,               /*  真正的可执行代码体          */
                                      pstkTop, 
                                      pthreadattr->THREADATTR_ulOption);
    
    if (pcName) {                                                       /*  拷贝名字                    */
        lib_strcpy(ptcb->TCB_cThreadName, pcName);
    
    } else {
        ptcb->TCB_cThreadName[0] = PX_EOS;
    }
    
    ulError = _TCBBuildExt(ptcb);                                       /*  首先先初始化扩展结构        */
    if (ulError) {
        iErrLevel = 2;
        _ErrorHandle(ulError);
        goto    __error_handle;
    }
    
    _TCBBuild(pthreadattr->THREADATTR_ucPriority,                       /*  构建 TCB                    */
              pstkFristFree,                                            /*  空闲栈区地址                */
              pstkTop,                                                  /*  主栈区地址                  */
              pstkButtom,                                               /*  栈底                        */
              pstkGuard,
              pthreadattr->THREADATTR_pvExt,
              pstkLowAddress,
              stStackSize,                                              /*  相对于字对齐的堆栈大小      */
              ulIdTemp,
              (pthreadattr->THREADATTR_ulOption | 
               LW_OPTION_THREAD_INIT),                                  /*  初始化                      */
              pfuncThread,
              ptcb,
              pthreadattr->THREADATTR_pvArg);

    if (pthreadattr->THREADATTR_pstkLowAddr) {                          /*  自动开辟检查                */
        ptcb->TCB_ucStackAutoAllocFlag = LW_STACK_MANUAL_ALLOC;
    
    } else {
        ptcb->TCB_ucStackAutoAllocFlag = LW_STACK_AUTO_ALLOC;
    }
    
    __KERNEL_MODE_PROC(_K_usThreadCounter++;);                          /*  线程数量++                  */
    
    __LW_OBJECT_CREATE_HOOK(ulIdTemp, pthreadattr->THREADATTR_ulOption);
    
    MONITOR_EVT_LONG5(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_INIT, 
                      ulIdTemp, 
                      pthreadattr->THREADATTR_ulOption, 
                      pthreadattr->THREADATTR_stStackByteSize,
                      pthreadattr->THREADATTR_ucPriority,
                      pthreadattr->THREADATTR_pvArg,
                      pcName);
    
    if (pulId) {
        *pulId = ulIdTemp;                                              /*  记录 ID                     */
    }
    
    if (LW_SYS_STATUS_IS_RUNNING()) {
        _ThreadUnsafeInternal();                                        /*  退出安全模式                */
    }
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "thread \"%s\" has been initialized.\r\n", (pcName ? pcName : ""));
    
    return  (ulIdTemp);
    
__error_handle:
    if (iErrLevel > 1) {
        if (!pthreadattr->THREADATTR_pstkLowAddr) {
            _StackFree(ptcb, pstkLowAddress);                           /*  释放堆栈空间                */
        }
    }
    if (iErrLevel > 0) {
        __KERNEL_MODE_PROC(
            _Free_Tcb_Object(ptcb);                                     /*  释放 ID                     */
        );
    }
    if (LW_SYS_STATUS_IS_RUNNING()) {
        _ThreadUnsafeInternal();                                        /*  退出安全模式                */
    }
    
    return  (LW_OBJECT_HANDLE_INVALID);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

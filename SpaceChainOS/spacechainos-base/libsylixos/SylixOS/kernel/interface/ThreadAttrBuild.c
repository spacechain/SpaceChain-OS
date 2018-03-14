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
** 文   件   名: ThreadAttrBuild.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 18 日
**
** 描        述: 这是系统建立线程属性块函数。

** BUG
2007.05.07  加入 API_ThreadAttrGetDefault() 和 API_ThreadAttrGet() 函数
2007.07.18  加入 _DebugHandle() 功能。
2007.08.29  API_ThreadAttrBuildEx() 加入地址对齐检查功能.
2007.11.08  将用户堆改为内核堆.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2013.05.04  API_ThreadAttrGet 总是获取堆栈大小和真实堆栈地址.
2013.09.17  加入对线程堆栈警戒区设置功能.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadAttrGetDefault
** 功能描述: 获得线程内核默认属性块
** 输　入  : NONE
** 输　出  : 线程内核默认属性块
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
LW_CLASS_THREADATTR  API_ThreadAttrGetDefault (VOID)
{
    static LW_CLASS_THREADATTR  threadattrDefault = {LW_NULL,
                                                     LW_CFG_THREAD_DEFAULT_GUARD_SIZE,
                                                     LW_CFG_THREAD_DEFAULT_STK_SIZE, 
                                                     LW_PRIO_NORMAL,
                                                     LW_OPTION_THREAD_STK_CHK,
                                                     LW_NULL,                          
                                                     LW_NULL};
    return  (threadattrDefault);
}
/*********************************************************************************************************
** 函数名称: API_ThreadAttrGet
** 功能描述: 获得指定线程属性块相关属性
** 输　入  : ulId            线程ID
** 输　出  : 指定线程属性块相关属性
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
LW_CLASS_THREADATTR  API_ThreadAttrGet (LW_OBJECT_HANDLE  ulId)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
    
             LW_CLASS_THREADATTR   threadattr;
             
    lib_bzero(&threadattr, sizeof(LW_CLASS_THREADATTR));                /*  如果返回堆栈大小为 0  则错误*/
             
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        threadattr = API_ThreadAttrGetDefault();
        return  (threadattr);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        threadattr = API_ThreadAttrGetDefault();
        return  (threadattr);
    }
#endif

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  退出内核                    */
        _ErrorHandle(ERROR_THREAD_NULL);
        threadattr = API_ThreadAttrGetDefault();
        return  (threadattr);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];                                  /*  获得线程控制块              */
    
    threadattr.THREADATTR_pstkLowAddr     = ptcb->TCB_pstkStackLowAddr; /*  全部堆栈区低内存起始地址    */
    threadattr.THREADATTR_stStackByteSize = (ptcb->TCB_stStackSize * sizeof(LW_STACK));                        
                                                                        /*  全部堆栈区大小(字节)        */
    threadattr.THREADATTR_ucPriority      = ptcb->TCB_ucPriority;       /*  线程优先级                  */
    threadattr.THREADATTR_ulOption        = ptcb->TCB_ulOption;         /*  任务选项                    */
    threadattr.THREADATTR_pvArg           = ptcb->TCB_pvArg;            /*  线程参数                    */
    threadattr.THREADATTR_pvExt           = ptcb->TCB_pvStackExt;       /*  扩展数据段指针              */
    
#if CPU_STK_GROWTH == 0
    threadattr.THREADATTR_stGuardSize = (size_t)(ptcb->TCB_pstkStackBottom - ptcb->TCB_pstkStackGuard);
#else
    threadattr.THREADATTR_stGuardSize = (size_t)(ptcb->TCB_pstkStackGuard - ptcb->TCB_pstkStackBottom);
#endif
    
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (threadattr);
}
/*********************************************************************************************************
** 函数名称: API_ThreadAttrBuild
** 功能描述: 建立线程属性块, 注意：没有设置 FP 堆栈
** 输　入  : pthreadattr        指向要生成的属性块
**           stStackByteSize    堆栈大小    (字节)
**           ucPriority         线程优先级
**           ulOption           线程选项
**           pvArg              线程参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG   API_ThreadAttrBuild (PLW_CLASS_THREADATTR    pthreadattr,
                             size_t                  stStackByteSize, 
                             UINT8                   ucPriority, 
                             ULONG                   ulOption, 
                             PVOID                   pvArg)
{
    if (!stStackByteSize) {                                             /*  如果没有设置堆栈大小        */
        stStackByteSize = LW_CFG_THREAD_DEFAULT_STK_SIZE;               /*  使用默认堆栈大小            */
    }
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!pthreadattr) {                                                 /*  需要生成的对象为空          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread attribute pointer invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_ATTR_NULL);
        return  (ERROR_THREAD_ATTR_NULL);
    }

    if (_StackSizeCheck(stStackByteSize)) {                             /*  堆栈大小不正确              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread stack size lack.\r\n");
        _ErrorHandle(ERROR_THREAD_STACKSIZE_LACK);
        return  (ERROR_THREAD_STACKSIZE_LACK);
    }
    
    if (_PriorityCheck(ucPriority)) {                                   /*  优先级错误                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread priority invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_PRIORITY_WRONG);
        return  (ERROR_THREAD_PRIORITY_WRONG);
    }
#endif

    pthreadattr->THREADATTR_pstkLowAddr     = LW_NULL;                  /*  系统自行分配堆栈            */
    pthreadattr->THREADATTR_stGuardSize     = LW_CFG_THREAD_DEFAULT_GUARD_SIZE;
    pthreadattr->THREADATTR_stStackByteSize = stStackByteSize;
    pthreadattr->THREADATTR_ucPriority      = ucPriority;
    pthreadattr->THREADATTR_ulOption        = ulOption;
    pthreadattr->THREADATTR_pvArg           = pvArg;
    pthreadattr->THREADATTR_pvExt           = LW_NULL;                  /*  没有附加信息                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ThreadAttrBuildEx
** 功能描述: 建立线程属性块高级函数，堆栈不从内核堆中开辟，由用户自行指定
** 输　入  : pthreadattr        指向要生成的属性块
**           pstkStackTop       全部堆栈低地址 (堆栈方向无关)
**           stStackByteSize    堆栈大小    (字节)
**           ucPriority         线程优先级
**           ulOption           线程选项
**           pvArg              线程参数
**           pvExt              扩展数据段指针
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG   API_ThreadAttrBuildEx (PLW_CLASS_THREADATTR    pthreadattr,
                               PLW_STACK               pstkStackTop, 
                               size_t                  stStackByteSize, 
                               UINT8                   ucPriority, 
                               ULONG                   ulOption, 
                               PVOID                   pvArg,
                               PVOID                   pvExt)
{
#if LW_CFG_ARG_CHK_EN > 0
    if (!pthreadattr) {                                                 /*  需要生成的对象为空          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread attribute pointer invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_ATTR_NULL);
        return  (ERROR_THREAD_ATTR_NULL);
    }
    
    if (!pstkStackTop) {                                                /*  堆栈地址不正确              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread stack pointer invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_STACK_NULL);
        return  (ERROR_THREAD_STACK_NULL);
    }
    
    if (!_Addresses_Is_Aligned(pstkStackTop)) {                         /*  检查地址是否对齐            */
        _ErrorHandle(ERROR_KERNEL_MEMORY);
        return  (ERROR_KERNEL_MEMORY);
    }
    
    if (_StackSizeCheck(stStackByteSize)) {                             /*  堆栈大小不正确              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread stack size lack.\r\n");
        _ErrorHandle(ERROR_THREAD_STACKSIZE_LACK);
        return  (ERROR_THREAD_STACKSIZE_LACK);
    }
    
    if (_PriorityCheck(ucPriority)) {                                   /*  优先级错误                  */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread priority invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_PRIORITY_WRONG);
        return  (ERROR_THREAD_PRIORITY_WRONG);
    }
#endif

    pthreadattr->THREADATTR_pstkLowAddr     = pstkStackTop;
    pthreadattr->THREADATTR_stGuardSize     = LW_CFG_THREAD_DEFAULT_GUARD_SIZE;
    pthreadattr->THREADATTR_stStackByteSize = stStackByteSize;
    pthreadattr->THREADATTR_ucPriority      = ucPriority;
    pthreadattr->THREADATTR_ulOption        = ulOption;
    pthreadattr->THREADATTR_pvArg           = pvArg;
    pthreadattr->THREADATTR_pvExt           = pvExt;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ThreadAttrBuildFP
** 功能描述: 建立线程属性块 FP 堆栈 (此函数为旧版本兼容函数, 不建议使用此方法建立浮点堆栈)
**           1.0.0 版本后, 此函数不再有效
** 输　入  : pthreadattr    指向要生成的属性块
**           pvFP              FP堆栈地址   (堆栈方向相关)
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG   API_ThreadAttrBuildFP (PLW_CLASS_THREADATTR  pthreadattr, PVOID  pvFP)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ThreadAttrSetGuardSize
** 功能描述: 建立线程属性块设置堆栈警戒区大小
** 输　入  : pthreadattr        指向要生成的属性块
**           stGuardSize        警戒区大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG   API_ThreadAttrSetGuardSize (PLW_CLASS_THREADATTR    pthreadattr,
                                    size_t                  stGuardSize)
{
#if LW_CFG_ARG_CHK_EN > 0
    if (!pthreadattr) {                                                 /*  需要生成的对象为空          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread attribute pointer invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_ATTR_NULL);
        return  (ERROR_THREAD_ATTR_NULL);
    }
#endif

    if (stGuardSize < (ARCH_STK_MIN_WORD_SIZE * sizeof(LW_STACK))) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread attribute pointer invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_STACK_NULL);
        return  (ERROR_THREAD_STACK_NULL);
    }

    pthreadattr->THREADATTR_stGuardSize = stGuardSize;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ThreadAttrSetStackSize
** 功能描述: 建立线程属性块设置堆栈大小
** 输　入  : pthreadattr       指向要生成的属性块
**           stStackByteSize   堆栈大小 (字节)
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG   API_ThreadAttrSetStackSize (PLW_CLASS_THREADATTR    pthreadattr,
                                    size_t                  stStackByteSize)
{
#if LW_CFG_ARG_CHK_EN > 0
    if (!pthreadattr) {                                                 /*  需要生成的对象为空          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread attribute pointer invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_ATTR_NULL);
        return  (ERROR_THREAD_ATTR_NULL);
    }
    
    if (_StackSizeCheck(stStackByteSize)) {                             /*  堆栈大小不正确              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread stack size lack.\r\n");
        _ErrorHandle(ERROR_THREAD_STACKSIZE_LACK);
        return  (ERROR_THREAD_STACKSIZE_LACK);
    }
#endif

    pthreadattr->THREADATTR_stStackByteSize = stStackByteSize;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ThreadAttrSetArg
** 功能描述: 建立线程属性块设置入口参数
** 输　入  : pthreadattr       指向要生成的属性块
**           pvArg             线程参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG   API_ThreadAttrSetArg (PLW_CLASS_THREADATTR    pthreadattr,
                              PVOID                   pvArg)
{
#if LW_CFG_ARG_CHK_EN > 0
    if (!pthreadattr) {                                                 /*  需要生成的对象为空          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread attribute pointer invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_ATTR_NULL);
        return  (ERROR_THREAD_ATTR_NULL);
    }
#endif

    pthreadattr->THREADATTR_pvArg = pvArg;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

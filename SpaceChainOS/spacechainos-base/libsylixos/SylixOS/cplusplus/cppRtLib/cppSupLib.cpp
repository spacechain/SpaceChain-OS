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
** 文   件   名: cppSupLib.cpp
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 03 月 07 日
**
** 描        述: 操作系统平台 C++ run time support lib. 
                 (为内核模块提供简单的 C++ 支持, 应用程序则使用 libstdc++ or libsupc++)

** BUG:
2011.03.22  __cxa_finalize() 当句柄有效时, 将执行所有的相同句柄的析构函数.
2013.05.21  增加注释.
*********************************************************************************************************/
#include "SylixOS.h"
/*********************************************************************************************************
  C 环境函数
*********************************************************************************************************/
extern "C" {
/*********************************************************************************************************
  sylixos 内核定义
*********************************************************************************************************/
#ifndef LW_OPTION_OBJECT_GLOBAL
#define LW_OPTION_OBJECT_GLOBAL     0x80000000
#endif
/*********************************************************************************************************
  链表操作宏
*********************************************************************************************************/
#define _LIST_OFFSETOF(type, member)                          \
        ((size_t)&((type *)0)->member)
#define _LIST_CONTAINER_OF(ptr, type, member)                 \
        ((type *)((size_t)ptr - _LIST_OFFSETOF(type, member)))
#define _LIST_ENTRY(ptr, type, member)                        \
        _LIST_CONTAINER_OF(ptr, type, member)
#define _LIST_LINE_GET_NEXT(pline)      ((pline)->LINE_plistNext)
/*********************************************************************************************************
  sylixos 内核函数
*********************************************************************************************************/
extern VOID  _List_Line_Add_Ahead(PLW_LIST_LINE  plineNew, LW_LIST_LINE_HEADER  *pplineHeader);
extern VOID  _List_Line_Del(PLW_LIST_LINE  plineDel, LW_LIST_LINE_HEADER  *pplineHeader);
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
extern VOID  __cppRtDoCtors(VOID);
extern VOID  __cppRtDoDtors(VOID);
extern VOID  __cppRtDummy(VOID);
/*********************************************************************************************************
  C++ func list
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE                        CPPFL_lineManage;
    VOIDFUNCPTR                         CPPFL_pfunc;
    PVOID                               CPPFL_pvArg;
    PVOID                               CPPFL_pvHandle;
} __LW_CPP_FUNC_LIST;

static LW_LIST_LINE_HEADER              _G_plineCppFuncList = LW_NULL;
static LW_OBJECT_HANDLE                 _G_ulCppRtLock = 0;
#define __LW_CPP_RT_LOCK()              API_SemaphoreMPend(_G_ulCppRtLock, LW_OPTION_WAIT_INFINITE)
#define __LW_CPP_RT_UNLOCK()            API_SemaphoreMPost(_G_ulCppRtLock)

#if LW_CFG_THREAD_EXT_EN > 0
static LW_THREAD_COND                   _G_condGuard = LW_THREAD_COND_INITIALIZER;
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
/*********************************************************************************************************
  guard 宏操作
*********************************************************************************************************/
#define __LW_CPP_GUARD_GET(piGv)        (*((char *)(piGv)))
#define __LW_CPP_GUARD_SET(piGv, x)     (*((char *)(piGv))  = x)

#define __LW_CPP_GUARD_MAKE_RELE(piGv)  (*((char *)(piGv)) |= 0x40)     /*  应经被 construction 完毕    */
#define __LW_CPP_GUARD_IS_RELE(piGv)    (*((char *)(piGv)) &  0x40)
/*********************************************************************************************************
  __cxa_guard_acquire
  __cxa_guard_release
  __cxa_guard_abort
  
  函数组主要解决的是静态局部对象构造可重入问题, 
  例如以下程序:
  
  void  foo ()
  {
      static student   tom;
      
      ...
  }
  
  调用 foo() 函数, 编译器生成的代码需要首先检查对象 tom 是否被建立, 如果没有构造, 则需要构造 tom. 但是为了
  防止多个任务同时调用 foo() 函数, 所以这里需要 __cxa_guard_acquire 和 __cxa_guard_release 配合来实现可重
  入. 编译器生成的伪代码如下:
  
  ...
  if (__cxa_guard_acquire(&tom_stat) == 1) {
      ...
      construct tom
      ...
      __cxa_guard_release(&tom_stat);
  }
  
  其中 tom_stat 为编译器为 tom 对象分配的状态记录"变量".
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: __cxa_guard_acquire
** 功能描述: 获得 piGv 相关的对象是否可以被构造, 
** 输　入  : piGv          对象构造保护变量
** 输　出  : 0: 不可被构造, 1 可以被构造
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int  __cxa_guard_acquire (int volatile  *piGv)
{
    int     iIsCanCon;

    __LW_CPP_RT_LOCK();
    
__retry:
    if (__LW_CPP_GUARD_IS_RELE(piGv)) {                                 /*  已经被 construction 完毕    */
        
        iIsCanCon = 0;                                                  /*  不可 construction           */
    
    } else if (__LW_CPP_GUARD_GET(piGv)) {                              /*  正在被 construction         */

#if LW_CFG_THREAD_EXT_EN > 0
        API_ThreadCondWait(&_G_condGuard, _G_ulCppRtLock, LW_OPTION_WAIT_INFINITE);
        goto    __retry;
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
    
    } else {
        
        __LW_CPP_GUARD_SET(piGv, 1);                                    /*  设置为正在 construction     */
        iIsCanCon = 1;                                                  /*  可以 construction           */
    }
    
    __LW_CPP_RT_UNLOCK();
    
    return  (iIsCanCon);
}
/*********************************************************************************************************
** 函数名称: __cxa_guard_release
** 功能描述: piGv 相关的对象被构造完毕后调用此函数. 这个对象不可再被构造
** 输　入  : piGv          对象构造保护变量
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
void  __cxa_guard_release (int volatile *piGv)
{
    __LW_CPP_RT_LOCK();
    
    __LW_CPP_GUARD_SET(piGv, 1);                                        /*  不可 construction           */
    __LW_CPP_GUARD_MAKE_RELE(piGv);                                     /*  设置为 release 标志         */

#if LW_CFG_THREAD_EXT_EN > 0
    API_ThreadCondBroadcast(&_G_condGuard);
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
    
    __LW_CPP_RT_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: __cxa_guard_abort
** 功能描述: piGv 相关的对象被析构后调用此函数, 此对象又可被构造
** 输　入  : piGv          对象构造保护变量
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
void  __cxa_guard_abort (int volatile *piGv)
{
    __LW_CPP_RT_LOCK();
    
    __LW_CPP_GUARD_SET(piGv, 0);                                        /*  可以 construction           */

#if LW_CFG_THREAD_EXT_EN > 0
    API_ThreadCondBroadcast(&_G_condGuard);
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */

    __LW_CPP_RT_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: __cxa_pure_virtual
** 功能描述: 在析构函数中调用虚方法时, 系统调用此函数.
** 输　入  : ANY
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
void __cxa_pure_virtual ()
{
#ifdef printk
    printk(KERN_ERR "C++ run time error - __cxa_pure_virtual should never be called\n");
#endif                                                                  /*  printk                      */
    lib_abort();
}
/*********************************************************************************************************
** 函数名称: __cxa_atexit
** 功能描述: 设置 __cxa_finalize 时需要运行的回调方法. 
** 输　入  : f         函数指针
**           p         参数
**           d         dso_handle 句柄
** 输　出  : 0: 成功  -1:失败
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int  __cxa_atexit (void (*f)(void *), void *p, void *d)
{
    __LW_CPP_FUNC_LIST      *pcppfl = (__LW_CPP_FUNC_LIST *)lib_malloc_new(sizeof(__LW_CPP_FUNC_LIST));
    
    if (pcppfl == LW_NULL) {
#ifdef printk
        printk(KERN_ERR "C++ run time error - __cxa_atexit system low memory\n");
#endif                                                                  /*  printk                      */
        return  (PX_ERROR);
    }
    
    pcppfl->CPPFL_pfunc    = (VOIDFUNCPTR)f;
    pcppfl->CPPFL_pvArg    = p;
    pcppfl->CPPFL_pvHandle = d;
    
    __LW_CPP_RT_LOCK();
    _List_Line_Add_Ahead(&pcppfl->CPPFL_lineManage, &_G_plineCppFuncList);
    __LW_CPP_RT_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __cxa_finalize
** 功能描述: 调用被 __cxa_atexit 安装的方法, (一般为析构函数)
** 输　入  : d         dso_handle 句柄 (NULL 表示运行所有)
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
void __cxa_finalize (void  *d)
{
    LW_LIST_LINE         *plinTemp;
    __LW_CPP_FUNC_LIST   *pcppfl = LW_NULL;
    
    if (d) {
        __LW_CPP_RT_LOCK();
        plinTemp = _G_plineCppFuncList;
        while (plinTemp) {
            pcppfl    = _LIST_ENTRY(plinTemp, __LW_CPP_FUNC_LIST, CPPFL_lineManage);
            plinTemp  = _LIST_LINE_GET_NEXT(plinTemp);
            if (pcppfl->CPPFL_pvHandle == d) {
                _List_Line_Del(&pcppfl->CPPFL_lineManage, &_G_plineCppFuncList);
                __LW_CPP_RT_UNLOCK();
                if (pcppfl->CPPFL_pfunc) {
                    LW_SOFUNC_PREPARE(pcppfl->CPPFL_pfunc);
                    pcppfl->CPPFL_pfunc(pcppfl->CPPFL_pvArg);
                }
                lib_free(pcppfl);
                __LW_CPP_RT_LOCK();
            }
        }
        __LW_CPP_RT_UNLOCK();
    
    } else {
        __LW_CPP_RT_LOCK();
        while (_G_plineCppFuncList) {
            plinTemp  = _G_plineCppFuncList;
            pcppfl    = _LIST_ENTRY(plinTemp, __LW_CPP_FUNC_LIST, CPPFL_lineManage);
            _List_Line_Del(&pcppfl->CPPFL_lineManage, &_G_plineCppFuncList);
            __LW_CPP_RT_UNLOCK();
            if (pcppfl->CPPFL_pfunc) {
                LW_SOFUNC_PREPARE(pcppfl->CPPFL_pfunc);
                pcppfl->CPPFL_pfunc(pcppfl->CPPFL_pvArg);
            }
            lib_free(pcppfl);
            __LW_CPP_RT_LOCK();
        }
        __LW_CPP_RT_UNLOCK();
    }
}
/*********************************************************************************************************
** 函数名称: __cxa_module_finalize
** 功能描述: module 使用此函数执行 module 内部被 __cxa_atexit 安装的方法, (一般为析构函数)
** 输　入  : pvBase        动态库代码段基址
**           stLen         动态库代码段长度
**           bCall         是否调用函数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
void __cxa_module_finalize (void *pvBase, size_t stLen, BOOL bCall)
{
     LW_LIST_LINE         *plinTemp;
    __LW_CPP_FUNC_LIST   *pcppfl = LW_NULL;
    
    __LW_CPP_RT_LOCK();
    plinTemp = _G_plineCppFuncList;
    while (plinTemp) {
        pcppfl    = _LIST_ENTRY(plinTemp, __LW_CPP_FUNC_LIST, CPPFL_lineManage);
        plinTemp  = _LIST_LINE_GET_NEXT(plinTemp);
        if (((PCHAR)pcppfl->CPPFL_pvHandle >= (PCHAR)pvBase) &&
            ((PCHAR)pcppfl->CPPFL_pvHandle <  (PCHAR)pvBase + stLen)) {
            _List_Line_Del(&pcppfl->CPPFL_lineManage, &_G_plineCppFuncList);
            __LW_CPP_RT_UNLOCK();
            if (pcppfl->CPPFL_pfunc && bCall) {
                LW_SOFUNC_PREPARE(pcppfl->CPPFL_pfunc);
                pcppfl->CPPFL_pfunc(pcppfl->CPPFL_pvArg);
            }
            lib_free(pcppfl);
            __LW_CPP_RT_LOCK();
        }
    }
    __LW_CPP_RT_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: _cppRtInit
** 功能描述: C++ 运行时支持初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _cppRtInit (VOID)
{
    _G_ulCppRtLock = API_SemaphoreMCreate("cpprt_lock", 
                                          LW_PRIO_DEF_CEILING, 
                                          LW_OPTION_INHERIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                          LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (!_G_ulCppRtLock) {
        return  (PX_ERROR);
    }
    
#if LW_CFG_THREAD_EXT_EN > 0
    if (API_ThreadCondInit(&_G_condGuard, LW_THREAD_PROCESS_SHARED) != ERROR_NONE) {
        return  (PX_ERROR);
    }
#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
    
    __cppRtDummy();
    __cppRtDoCtors();                                                   /*  运行全局对象构造函数        */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _cppRtInit
** 功能描述: C++ 运行时卸载
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _cppRtUninit (VOID)
{
    __cxa_finalize(LW_NULL);

    __cppRtDoDtors();                                                   /*  运行全局对象析构函数        */
    
    if (_G_ulCppRtLock) {
        API_SemaphoreMDelete(&_G_ulCppRtLock);
    }
    
    API_ThreadCondDestroy(&_G_condGuard);
}
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

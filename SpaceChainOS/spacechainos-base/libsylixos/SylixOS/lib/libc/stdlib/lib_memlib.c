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
** 文   件   名: lib_memlib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 08 月 30 日
**
** 描        述: 内存管理库.

** BUG
2009.03.06  malloc(0) 可以分配出最小的内存.
2009.07.16  加入 mallocalign 函数.
2011.03.05  分配内存时加入了跟踪函数名信息.
2011.07.19  将 realloc 参数检查交由内核处理.
2013.02.22  加入 memalign 与 xmemalign 标准函数.
2013.07.12  加入 posix_memalign 函数.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  错误打印
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
#define __LIB_PERROR(s)         perror(s)
#else
#define __LIB_PERROR(s)
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)...   */
/*********************************************************************************************************
** 函数名称: lib_malloc
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  lib_malloc (size_t  stNBytes)
{
    stNBytes = (stNBytes) ? stNBytes : 1;

    return  (__SHEAP_ALLOC(stNBytes));
}
/*********************************************************************************************************
** 函数名称: lib_xmalloc
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  lib_xmalloc (size_t  stSize)
{
    void  *ptr = lib_malloc(stSize);
    
    if (ptr == LW_NULL) {
        __LIB_PERROR("lib_xmalloc() not enough memory");
        exit(0);
    }
    
    return  (ptr);
}
/*********************************************************************************************************
** 函数名称: lib_mallocalign
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  lib_mallocalign (size_t  stNBytes, size_t  stAlign)
{
    if (stAlign & (stAlign - 1)) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    stNBytes = (stNBytes) ? stNBytes : 1;
    
    return  (__SHEAP_ALLOC_ALIGN(stNBytes, stAlign));
}
/*********************************************************************************************************
** 函数名称: lib_xmalloc
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  lib_xmallocalign (size_t  stSize, size_t  stAlign)
{
    void  *ptr = lib_mallocalign(stSize, stAlign);
    
    if (ptr == LW_NULL) {
        __LIB_PERROR("lib_xmallocalign() not enough memory");
        exit(0);
    }
    
    return  (ptr);
}
/*********************************************************************************************************
** 函数名称: lib_memalign
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  lib_memalign (size_t  stAlign, size_t  stNBytes)
{
    if (stAlign & (stAlign - 1)) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    stNBytes = (stNBytes) ? stNBytes : 1;
    
    return  (__SHEAP_ALLOC_ALIGN(stNBytes, stAlign));
}
/*********************************************************************************************************
** 函数名称: lib_xmemalign
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  lib_xmemalign (size_t  stAlign, size_t  stNBytes)
{
    void  *ptr = lib_mallocalign(stNBytes, stAlign);
    
    if (ptr == LW_NULL) {
        __LIB_PERROR("lib_xmemalign() not enough memory");
        exit(0);
    }
    
    return  (ptr);
}
/*********************************************************************************************************
** 函数名称: lib_free
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID   lib_free (PVOID  pvPtr)
{
    if (pvPtr) {
#if LW_CFG_USER_HEAP_SAFETY > 0
        _HeapFree(_K_pheapSystem, pvPtr, LW_TRUE, __func__);            /*  需要加入安全性算法          */
#else
        _HeapFree(_K_pheapSystem, pvPtr, LW_FALSE, __func__);
#endif                                                                  /*  LW_CFG_USER_HEAP_SAFETY     */
    }
}
/*********************************************************************************************************
** 函数名称: lib_calloc
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  lib_calloc (size_t  stNNum, size_t  stSize)
{
    size_t   stTotalSize = stNNum * stSize;
    PVOID    pvMem;
    
    if (stTotalSize) {
        pvMem = __SHEAP_ALLOC(stTotalSize);
        if (pvMem) {
            lib_bzero(pvMem, stTotalSize);
        }
    } else {
        pvMem = LW_NULL;
    }
    
    return  (pvMem);
}
/*********************************************************************************************************
** 函数名称: lib_xcalloc
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  lib_xcalloc (size_t  stNNum, size_t  stSize)
{
    void  *ptr = lib_calloc(stNNum, stSize);
    
    if (ptr == LW_NULL) {
        __LIB_PERROR("lib_xcalloc() not enough memory");
        exit(0);
    }
    
    return  (ptr);
}
/*********************************************************************************************************
** 函数名称: lib_realloc
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  lib_realloc (PVOID  pvPtr, size_t  stNewSize)
{
    return  (_HeapRealloc(_K_pheapSystem, pvPtr, stNewSize, LW_TRUE, __func__));
}
/*********************************************************************************************************
** 函数名称: lib_xrealloc
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  lib_xrealloc (PVOID  pvPtr, size_t  stNewSize)
{
    void  *ptr = lib_realloc(pvPtr, stNewSize);
    
    if ((ptr == LW_NULL) && stNewSize) {
        __LIB_PERROR("lib_xrealloc() not enough memory");
        exit(0);
    }
    
    return  (ptr);
}
/*********************************************************************************************************
** 函数名称: lib_posix_memalign
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  lib_posix_memalign (PVOID *ppvMem, size_t  stAlign, size_t  stNBytes)
{
    if (ppvMem == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    if (stAlign & (stAlign - 1)) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    stNBytes = (stNBytes) ? stNBytes : 1;
    
    *ppvMem = __SHEAP_ALLOC_ALIGN(stNBytes, stAlign);
    if (*ppvMem == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (ENOMEM);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  下面为专为 C++ 定制重载 new 操作符用函数
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: lib_malloc_new
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  lib_malloc_new (size_t  stNBytes)
{
    stNBytes = (stNBytes) ? stNBytes : 1;

    return  (_HeapAllocate(_K_pheapSystem, stNBytes, "C++ new"));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

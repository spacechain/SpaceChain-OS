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
** 文   件   名: cache.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 09 月 27 日
**
** 描        述: 平台无关的高速缓冲器管理部件 (针对 L1-cache).

** BUG
2007.12.22 整理注释.
2008.05.02 修改了 API_CacheFuncsSet() 内部运算符的相关错误.
2008.06.06 当 D-CACHE 为回写类型时, 使 CACHE 不命中之前, 必须首先进行回写.
2008.12.05 加入对齐 DMA 内存管理.
2009.03.09 修改内存对齐开辟的错误.
2009.06.18 取消 CACHE 对地址映射的管理, 完全交由 VMM 组件完成.
2009.07.11 API_CacheLibInit() 只能初始化一次.
2011.05.20 API_CacheInvalidate() 驱动程序必须确保回写型 cache 先回写再无效.
2011.12.10 在 cache 关键性操作时, 需要关中断! 一定不能发生当前处理器抢占!
2012.01.18 不需要在 API_CacheTextUpdate() 进行抵制有效性的判断.
2012.01.18 cache 关键性操作时防止多核抢占. (待商议可行性)
2013.07.20 TextUpdate 支持 SMP 系统.
2013.12.19 API_CacheLibPrimaryInit() 加入 CACHE 名称字串, arch 程序通过此字串提供相应的 CACHE 驱动程序.
2014.01.03 简化 cache api 设计.
2014.07.27 加入物理页面操作.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
   
   下面是只使用 CACHE 的系统结构图
   
   +---------------+     +-----------------+     +--------------+
   |               |     |                 |     |              |
   |  INSTRUCTION  |---->|    PROCESSOR    |<--->|  DATA CACHE  | (3)
   |     CACHE     |     |                 |     |  (copyback)  |
   |               |     |                 |     |              |
   +---------------+     +-----------------+     +--------------+
           ^                     (2)                     ^
           |                                             |
           |                                     +--------------+
           |                                     |              |
           |                                     | WRITE BUFFER |
           |                                     |              |
           |                                     +--------------+
           |             +-----------------+             |
           |             |                 |         (1) |
           +-------------|       RAM       |<------------+
                         |                 |
                         +-----------------+
                             ^         ^
                             |         |
           +-------------+   |         |   +-------------+
           |             |   |         |   |             |
           | DMA Devices |<--+         +-->| VMEbus, etc.|
           |             |                 |             |
           +-------------+                 +-------------+

*********************************************************************************************************/
/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
/*********************************************************************************************************
  全局变量定义 
*********************************************************************************************************/
static CACHE_MODE   _G_uiICacheMode = CACHE_DISABLED;
static CACHE_MODE   _G_uiDCacheMode = CACHE_DISABLED;
/*********************************************************************************************************
  cache 操作上下文锁定
*********************************************************************************************************/
#define __CACHE_OP_ENTER(iregInterLevel)    {   iregInterLevel = KN_INT_DISABLE(); KN_SMP_MB(); }
#define __CACHE_OP_EXIT(iregInterLevel)     {   KN_INT_ENABLE(iregInterLevel);  }
/*********************************************************************************************************
  全局结构变量定义 
*********************************************************************************************************/
LW_CACHE_OP     _G_cacheopLib = {                                       /*  the cache primitives        */
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
    0,
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */

    CACHE_LOCATION_VIVT,
    CACHE_LOCATION_VIVT,
    32,
    32,
    LW_CFG_VMM_PAGE_SIZE,
    LW_CFG_VMM_PAGE_SIZE,                                               /*  def: No Cache Alias problem */
    
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
    LW_NULL,                                                            /*  cacheEnable()               */
    LW_NULL,                                                            /*  cacheDisable()              */
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */

    LW_NULL,                                                            /*  cacheLock()                 */
    LW_NULL,                                                            /*  cacheUnlock()               */
    LW_NULL,                                                            /*  cacheFlush()                */
    LW_NULL,                                                            /*  cacheFlushPage()            */

#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
    LW_NULL,                                                            /*  cacheInvalidate()           */
    LW_NULL,                                                            /*  cacheInvalidatePage()       */
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */

    LW_NULL,                                                            /*  cacheClear()                */
    LW_NULL,                                                            /*  cacheClearPage()            */
    LW_NULL,                                                            /*  cacheTextUpdate()           */
    LW_NULL,                                                            /*  cacheDataUpdate()           */
    LW_NULL,                                                            /*  cacheDmaMalloc()            */
    LW_NULL,                                                            /*  cacheDmaMallocAlign()       */
    LW_NULL,                                                            /*  cacheDmaFree()              */
};
/*********************************************************************************************************
  TextUpdate Parameter
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

typedef struct {
    PVOID       TUA_pvAddr;
    size_t      TUA_stSize;
} LW_CACHE_TU_ARG;

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  体系结构函数声明
*********************************************************************************************************/
extern VOID   __ARCH_CACHE_INIT(CACHE_MODE  uiInstruction, CACHE_MODE  uiData, CPCHAR  pcMachineName);
/*********************************************************************************************************
** 函数名称: API_CacheGetLibBlock
** 功能描述: 获得系统级 LW_CACHE_OP 结构, (用于 BSP 程序初始化 CACHE 系统)
** 输　入  : NONE
** 输　出  : &_G_cacheopLib
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
LW_CACHE_OP *API_CacheGetLibBlock (VOID)
{
    return  (&_G_cacheopLib);
}
/*********************************************************************************************************
** 函数名称: API_CacheGetOption
** 功能描述: 获得系统级 LW_CACHE_OP 结构 OPTION 字段.
** 输　入  : NONE
** 输　出  : &_G_cacheopLib
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
ULONG  API_CacheGetOption (VOID)
{
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0
    return  (0);
#else
    return  (_G_cacheopLib.CACHEOP_ulOption);
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
}
/*********************************************************************************************************
** 函数名称: API_CacheGetMode
** 功能描述: 获得 CACHE 模式.
** 输　入  : cachetype     cache 类型
** 输　出  : CACHE 模式
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
CACHE_MODE  API_CacheGetMode (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        return  (_G_uiICacheMode);
    } else {
        return  (_G_uiDCacheMode);
    }
}
/*********************************************************************************************************
** 函数名称: API_CacheLibPrimaryInit
** 功能描述: 初始化 CACHE 功能, CPU 构架相关.
** 输　入  : uiInstruction                 初始指令 CACHE 模式
**           uiData                        初始数据 CACHE 模式
**           pcMachineName                 当前运行机器名称.
** 输　出  : BSP 函数返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
ULONG   API_CacheLibPrimaryInit (CACHE_MODE  uiInstruction, CACHE_MODE  uiData, CPCHAR  pcMachineName)
{
    static BOOL bIsInit = LW_FALSE;

    if (bIsInit) {
        return  (ERROR_NONE);
    }

    _G_uiICacheMode = uiInstruction;
    _G_uiDCacheMode = uiData;                                           /*  保存 D-CACHE 模式           */
    
    __ARCH_CACHE_INIT(uiInstruction, uiData, pcMachineName);            /*  初始化 CACHE                */
    API_CacheFuncsSet();
    
    bIsInit = LW_TRUE;
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "CACHE initilaized.\r\n");
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_CacheLibSInit
** 功能描述: 初始化从核 CACHE 功能, CPU 构架相关。
** 输　入  : pcMachineName                 当前运行机器名称.
** 输　出  : OK 
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

LW_API  
ULONG   API_CacheLibSecondaryInit (CPCHAR  pcMachineName)
{
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** 函数名称: API_CacheLocation
** 功能描述: 获取 CACHE 的位置
** 输　入  : cachetype     cache 类型
** 输　出  : BSP 提供的 CACHE 位置类型信息
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_CacheLocation (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        return  (_G_cacheopLib.CACHEOP_iILoc);
    
    } else {
        return  (_G_cacheopLib.CACHEOP_iDLoc);
    }
}
/*********************************************************************************************************
** 函数名称: API_CacheLine
** 功能描述: 获取 CACHE line 大小
** 输　入  : cachetype     cache 类型
** 输　出  : CACHE 行大小
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_CacheLine (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        return  (_G_cacheopLib.CACHEOP_iICacheLine);
    
    } else {
        return  (_G_cacheopLib.CACHEOP_iDCacheLine);
    }
}
/*********************************************************************************************************
** 函数名称: API_CacheWaySize
** 功能描述: 获取一路 DCACHE 大小
** 输　入  : cachetype     cache 类型
** 输　出  : 一路 CACHE 大小
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
size_t  API_CacheWaySize (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        return  (_G_cacheopLib.CACHEOP_iICacheWaySize);
    
    } else {
        return  (_G_cacheopLib.CACHEOP_iDCacheWaySize);
    }
}
/*********************************************************************************************************
** 函数名称: API_CacheAliasProb
** 功能描述: VIPT CACHE 是否具有 alias 风险. (两个 DCACHE 行映射同一物理地址)
** 输　入  : NONE
** 输　出  : LW_TRUE  有 alias 风险
**           LW_FALSE 无 alias 风险
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
BOOL  API_CacheAliasProb (VOID)
{
    if (_G_cacheopLib.CACHEOP_iDLoc == CACHE_LOCATION_VIVT) {
        return  (LW_TRUE);
    }
    
    if ((_G_cacheopLib.CACHEOP_iDLoc == CACHE_LOCATION_VIPT) &&
        (_G_cacheopLib.CACHEOP_iDCacheWaySize > LW_CFG_VMM_PAGE_SIZE)) {
        return  (LW_TRUE);
    }
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: API_CacheEnable
** 功能描述: 使能指定类型的 CACHE
** 输　入  : cachetype                     CACHE 类型
** 输　出  : BSP 函数返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT    API_CacheEnable (LW_CACHE_TYPE  cachetype)
{
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0
    return  (ERROR_NONE);
    
#else
    INTREG  iregInterLevel;
    INT     iError;

    __CACHE_OP_ENTER(iregInterLevel);                                   /*  开始操作 cache              */
    iError = ((_G_cacheopLib.CACHEOP_pfuncEnable == LW_NULL) ? PX_ERROR : 
              (_G_cacheopLib.CACHEOP_pfuncEnable)(cachetype));
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  结束操作 cache              */
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "%sCACHE enable.\r\n",
                 (cachetype == INSTRUCTION_CACHE) ? "I-" : "D-");
                 
#ifdef LW_CFG_CPU_ARCH_ARM
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_ARM_SPINLOCK_DEP_CACHE > 0)
    if (cachetype == DATA_CACHE) {
        __ARCH_SPIN_WORK();                                             /*  spin lock 使能              */
    }
#endif
#endif

    return  (iError);
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
}
/*********************************************************************************************************
** 函数名称: API_CacheDisable
** 功能描述: 禁能指定类型的 CACHE
** 输　入  : cachetype                     CACHE 类型
** 输　出  : BSP 函数返回值
** 全局变量: 
** 调用模块: 
** 注  意  : dcache 在关闭前, 驱动程序必须先清空数据.
                                           API 函数
*********************************************************************************************************/
LW_API  
INT    API_CacheDisable (LW_CACHE_TYPE  cachetype)
{
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0
    return  (ERROR_NONE);
    
#else
    INTREG  iregInterLevel;
    INT     iError;

    __CACHE_OP_ENTER(iregInterLevel);                                   /*  开始操作 cache              */
    iError = ((_G_cacheopLib.CACHEOP_pfuncDisable == LW_NULL) ? PX_ERROR : 
              (_G_cacheopLib.CACHEOP_pfuncDisable)(cachetype));
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  结束操作 cache              */
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "%sCACHE disable.\r\n",
                 (cachetype == INSTRUCTION_CACHE) ? "I-" : "D-");
    
    return  (iError);
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
}
/*********************************************************************************************************
** 函数名称: API_CacheLock
** 功能描述: 指定类型的 CACHE 锁定一个地址
** 输　入  : cachetype                     CACHE 类型
**           pvAdrs                        需要锁定的虚拟地址
**           stBytes                       长度
** 输　出  : BSP 函数返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT    API_CacheLock (LW_CACHE_TYPE   cachetype, 
                      PVOID           pvAdrs, 
                      size_t          stBytes)
{
    INTREG  iregInterLevel;
    INT     iError;
    
    __CACHE_OP_ENTER(iregInterLevel);                                   /*  开始操作 cache              */
    iError = ((_G_cacheopLib.CACHEOP_pfuncLock == LW_NULL) ? PX_ERROR : 
              (_G_cacheopLib.CACHEOP_pfuncLock)(cachetype, pvAdrs, stBytes));
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  结束操作 cache              */
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_CacheUnlock
** 功能描述: 指定类型的 CACHE 解锁一个地址
** 输　入  : cachetype                     CACHE 类型
**           pvAdrs                        需要解锁的虚拟地址
**           stBytes                       长度
** 输　出  : BSP 函数返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT    API_CacheUnlock (LW_CACHE_TYPE   cachetype, 
                        PVOID           pvAdrs, 
                        size_t          stBytes)
{
    INTREG  iregInterLevel;
    INT     iError;
    
    __CACHE_OP_ENTER(iregInterLevel);                                   /*  开始操作 cache              */
    iError = ((_G_cacheopLib.CACHEOP_pfuncUnlock == LW_NULL) ? PX_ERROR : 
              (_G_cacheopLib.CACHEOP_pfuncUnlock)(cachetype, pvAdrs, stBytes));
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  结束操作 cache              */
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_CacheFlush
** 功能描述: 指定类型的 CACHE 将所有或者指定的数据项清空(回写入内存)
** 输　入  : cachetype                     CACHE 类型
**           pvAdrs                        虚拟地址
**           stBytes                       长度
** 输　出  : BSP 函数返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT    API_CacheFlush (LW_CACHE_TYPE   cachetype, 
                       PVOID           pvAdrs, 
                       size_t          stBytes)
{
    INTREG  iregInterLevel;
    INT     iError;
    
    __CACHE_OP_ENTER(iregInterLevel);                                   /*  开始操作 cache              */
    iError = ((_G_cacheopLib.CACHEOP_pfuncFlush == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncFlush)(cachetype, pvAdrs, stBytes));
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  结束操作 cache              */
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_CacheFlushPage
** 功能描述: 指定类型的 CACHE 使指定的页面回写
** 输　入  : cachetype                     CACHE 类型
**           pvAdrs                        虚拟地址
**           pvPdrs                        物理地址
**           stBytes                       长度 (页面长度整数倍)
** 输　出  : BSP 函数返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT    API_CacheFlushPage (LW_CACHE_TYPE   cachetype, 
                           PVOID           pvAdrs, 
                           PVOID           pvPdrs,
                           size_t          stBytes)
{
    INTREG  iregInterLevel;
    INT     iError;
    
    __CACHE_OP_ENTER(iregInterLevel);                                   /*  开始操作 cache              */
#if LW_CFG_VMM_EN > 0
    iError = ((_G_cacheopLib.CACHEOP_pfuncFlushPage == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncFlushPage)(cachetype, pvAdrs, pvPdrs, stBytes));
#else
    iError = ((_G_cacheopLib.CACHEOP_pfuncFlush == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncFlush)(cachetype, pvAdrs, stBytes));
#endif
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  结束操作 cache              */
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_CacheInvalidate
** 功能描述: 指定类型的 CACHE 使部分或全部表项无效(访问不命中)
** 输　入  : cachetype                     CACHE 类型
**           pvAdrs                        虚拟地址
**           stBytes                       长度
** 输　出  : BSP 函数返回值
** 全局变量: 
** 调用模块: 
** 注  意  : invalidate 操作必须确保相关的回写与 cache 行对齐.

                                           API 函数
*********************************************************************************************************/
LW_API  
INT    API_CacheInvalidate (LW_CACHE_TYPE   cachetype, 
                            PVOID           pvAdrs, 
                            size_t          stBytes)
{
    INTREG  iregInterLevel;
    INT     iError;
    
    __CACHE_OP_ENTER(iregInterLevel);                                   /*  开始操作 cache              */
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0
    iError = ((_G_cacheopLib.CACHEOP_pfuncClear == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncClear)(cachetype, pvAdrs, stBytes));
#else
    iError = ((_G_cacheopLib.CACHEOP_pfuncInvalidate == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncInvalidate)(cachetype, pvAdrs, stBytes));
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  结束操作 cache              */
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_CacheInvalidatePage
** 功能描述: 指定类型的 CACHE 使指定的页面无效(访问不命中)
** 输　入  : cachetype                     CACHE 类型
**           pvAdrs                        虚拟地址
**           pvPdrs                        物理地址
**           stBytes                       长度 (页面长度整数倍)
** 输　出  : BSP 函数返回值
** 全局变量: 
** 调用模块: 
** 注  意  : invalidate 操作必须确保相关的回写与 cache 行对齐.

                                           API 函数
*********************************************************************************************************/
LW_API  
INT    API_CacheInvalidatePage (LW_CACHE_TYPE   cachetype, 
                                PVOID           pvAdrs, 
                                PVOID           pvPdrs,
                                size_t          stBytes)
{
    INTREG  iregInterLevel;
    INT     iError;
    
    __CACHE_OP_ENTER(iregInterLevel);                                   /*  开始操作 cache              */
#if LW_CFG_VMM_L4_HYPERVISOR_EN > 0
#if LW_CFG_VMM_EN > 0
    iError = ((_G_cacheopLib.CACHEOP_pfuncClearPage == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncClearPage)(cachetype, pvAdrs, pvPdrs, stBytes));
#else
    iError = ((_G_cacheopLib.CACHEOP_pfuncClear == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncClear)(cachetype, pvAdrs, stBytes));
#endif
#else
#if LW_CFG_VMM_EN > 0
    iError = ((_G_cacheopLib.CACHEOP_pfuncInvalidatePage == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncInvalidatePage)(cachetype, pvAdrs, pvPdrs, stBytes));
#else
    iError = ((_G_cacheopLib.CACHEOP_pfuncInvalidate == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncInvalidate)(cachetype, pvAdrs, stBytes));
#endif
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  结束操作 cache              */
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_CacheClear
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype                     CACHE 类型
**           pvAdrs                        虚拟地址
**           stBytes                       长度
** 输　出  : BSP 函数返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT    API_CacheClear (LW_CACHE_TYPE   cachetype, 
                       PVOID           pvAdrs, 
                       size_t          stBytes)
{
    INTREG  iregInterLevel;
    INT     iError;

    __CACHE_OP_ENTER(iregInterLevel);                                   /*  开始操作 cache              */
    iError = ((_G_cacheopLib.CACHEOP_pfuncClear == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncClear)(cachetype, pvAdrs, stBytes));
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  结束操作 cache              */
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_CacheClearPage
** 功能描述: 指定类型的 CACHE 使指定的页面回写并无效
** 输　入  : cachetype                     CACHE 类型
**           pvAdrs                        虚拟地址
**           pvPdrs                        物理地址
**           stBytes                       长度 (页面长度整数倍)
** 输　出  : BSP 函数返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT    API_CacheClearPage (LW_CACHE_TYPE   cachetype, 
                           PVOID           pvAdrs, 
                           PVOID           pvPdrs,
                           size_t          stBytes)
{
    INTREG  iregInterLevel;
    INT     iError;

    __CACHE_OP_ENTER(iregInterLevel);                                   /*  开始操作 cache              */
#if LW_CFG_VMM_EN > 0
    iError = ((_G_cacheopLib.CACHEOP_pfuncClearPage == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncClearPage)(cachetype, pvAdrs, pvPdrs, stBytes));
#else
    iError = ((_G_cacheopLib.CACHEOP_pfuncClear == LW_NULL) ? ERROR_NONE : 
              (_G_cacheopLib.CACHEOP_pfuncClear)(cachetype, pvAdrs, stBytes));
#endif
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  结束操作 cache              */
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __cacheTextUpdate
** 功能描述: 清空(回写内存) D CACHE 无效(访问不命中) I CACHE
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
** 输　出  : BSP 函数返回值
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_VMM_L4_HYPERVISOR_EN == 0)

static INT __cacheTextUpdate (LW_CACHE_TU_ARG *ptuarg)
{
    INTREG  iregInterLevel;
    INT     iError;
    
    __CACHE_OP_ENTER(iregInterLevel);                                   /*  开始操作 cache              */
    iError = ((_G_cacheopLib.CACHEOP_pfuncTextUpdate == LW_NULL) ? ERROR_NONE :
              (_G_cacheopLib.CACHEOP_pfuncTextUpdate)(ptuarg->TUA_pvAddr, ptuarg->TUA_stSize));
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  结束操作 cache              */
    
    return  (iError);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
                                                                        /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
/*********************************************************************************************************
** 函数名称: API_CacheTextUpdate
** 功能描述: 清空(回写内存) D CACHE 无效(访问不命中) I CACHE
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
** 输　出  : BSP 函数返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT    API_CacheTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    INTREG          iregInterLevel;
    INT             iError;
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_VMM_L4_HYPERVISOR_EN == 0)
    LW_CACHE_TU_ARG tuarg;
    BOOL            bLock;
#endif                                                                  /*  LW_CFG_SMP_EN               */
                                                                        /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
    __CACHE_OP_ENTER(iregInterLevel);                                   /*  开始操作 cache              */
    iError = ((_G_cacheopLib.CACHEOP_pfuncTextUpdate == LW_NULL) ? ERROR_NONE :
              (_G_cacheopLib.CACHEOP_pfuncTextUpdate)(pvAdrs, stBytes));
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  结束操作 cache              */

#if (LW_CFG_SMP_EN > 0) && (LW_CFG_VMM_L4_HYPERVISOR_EN == 0)
    if ((_G_cacheopLib.CACHEOP_ulOption & CACHE_TEXT_UPDATE_MP) && 
        (LW_NCPUS > 1)) {                                               /*  需要通知其他 CPU            */
        tuarg.TUA_pvAddr = pvAdrs;
        tuarg.TUA_stSize = stBytes;
        
        bLock = __SMP_CPU_LOCK();                                       /*  锁定当前 CPU 执行           */
        _SmpCallFuncAllOther(__cacheTextUpdate, &tuarg,
                             LW_NULL, LW_NULL, IPIM_OPT_NORMAL);        /*  通知其他的 CPU              */
        __SMP_CPU_UNLOCK(bLock);                                        /*  解锁当前 CPU 执行           */
    }
#endif                                                                  /*  LW_CFG_SMP_EN               */
                                                                        /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_CacheDataUpdate
** 功能描述: 回写 D CACHE (仅回写 CPU 独享级 CACHE)
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
**           bInv                          是否为回写无效
** 输　出  : BSP 函数返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT    API_CacheDataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    INTREG  iregInterLevel;
    INT     iError;
    
    __CACHE_OP_ENTER(iregInterLevel);                                   /*  开始操作 cache              */
    iError = ((_G_cacheopLib.CACHEOP_pfuncDataUpdate == LW_NULL) ? ERROR_NONE :
              (_G_cacheopLib.CACHEOP_pfuncDataUpdate)(pvAdrs, stBytes, bInv));
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  结束操作 cache              */
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_CacheLocalTextUpdate
** 功能描述: 清空(回写内存) D CACHE 无效(访问不命中) I CACHE (仅对当前 CPU 有效)
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
** 输　出  : BSP 函数返回值
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT    API_CacheLocalTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    INTREG  iregInterLevel;
    INT     iError;
    
    __CACHE_OP_ENTER(iregInterLevel);                                   /*  开始操作 cache              */
    iError = ((_G_cacheopLib.CACHEOP_pfuncTextUpdate == LW_NULL) ? ERROR_NONE :
              (_G_cacheopLib.CACHEOP_pfuncTextUpdate)(pvAdrs, stBytes));
    __CACHE_OP_EXIT(iregInterLevel);                                    /*  结束操作 cache              */
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_CacheDmaMalloc
** 功能描述: 开辟一块非缓冲的内存
** 输　入  : 
**           stBytes                       长度
** 输　出  : 开辟的空间首地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PVOID    API_CacheDmaMalloc (size_t   stBytes)
{
    return  ((_G_cacheopLib.CACHEOP_pfuncDmaMalloc == LW_NULL) ?
             (__KHEAP_ALLOC(stBytes)) : 
             (_G_cacheopLib.CACHEOP_pfuncDmaMalloc(stBytes)));
}
/*********************************************************************************************************
** 函数名称: API_CacheDmaMallocAlign
** 功能描述: 开辟一块非缓冲的内存, 指定内存对齐关系
** 输　入  : 
**           stBytes                       长度
**           stAlign                       对齐字节数
** 输　出  : 开辟的空间首地址
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
PVOID    API_CacheDmaMallocAlign (size_t   stBytes, size_t  stAlign)
{
    if (stAlign & (stAlign - 1)) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    return  ((_G_cacheopLib.CACHEOP_pfuncDmaMallocAlign == LW_NULL) ?
             (__KHEAP_ALLOC_ALIGN(stBytes, stAlign)) : 
             (_G_cacheopLib.CACHEOP_pfuncDmaMallocAlign(stBytes, stAlign)));
}
/*********************************************************************************************************
** 函数名称: API_CacheDmaFree
** 功能描述: 归还一块非缓冲的内存
** 输　入  : 
**           pvBuf                         开辟时的首地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID    API_CacheDmaFree (PVOID  pvBuf)
{
    if (_G_cacheopLib.CACHEOP_pfuncDmaFree) {
        _G_cacheopLib.CACHEOP_pfuncDmaFree(pvBuf);
    
    } else {
        __KHEAP_FREE(pvBuf);
    }
}
/*********************************************************************************************************
** 函数名称: API_CacheDmaFlush
** 功能描述: DMA 区域 CACHE 将所有或者指定的数据项清空(回写入内存)
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT    API_CacheDmaFlush (PVOID  pvAdrs, size_t  stBytes)
{
    return  ((_G_cacheopLib.CACHEOP_pfuncDmaMalloc == LW_NULL) ?
             (API_CacheFlush(DATA_CACHE, pvAdrs, stBytes)) : 
             (ERROR_NONE));
}
/*********************************************************************************************************
** 函数名称: API_CacheDmaInvalidate
** 功能描述: DMA 区域 CACHE 使部分或全部表项无效(访问不命中)
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT    API_CacheDmaInvalidate (PVOID  pvAdrs, size_t  stBytes)
{
    return  ((_G_cacheopLib.CACHEOP_pfuncDmaMalloc == LW_NULL) ?
             (API_CacheInvalidate(DATA_CACHE, pvAdrs, stBytes)) : 
             (ERROR_NONE));
}
/*********************************************************************************************************
** 函数名称: API_CacheDmaClear
** 功能描述: DMA 区域 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT    API_CacheDmaClear (PVOID  pvAdrs, size_t  stBytes)
{
    return  ((_G_cacheopLib.CACHEOP_pfuncDmaMalloc == LW_NULL) ?
             (API_CacheClear(DATA_CACHE, pvAdrs, stBytes)) : 
             (ERROR_NONE));
}
/*********************************************************************************************************
** 函数名称: API_CacheFuncsSet
** 功能描述: 根据配置信息配置 CACHE 函数表
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID    API_CacheFuncsSet (VOID)
{
    if (_G_uiDCacheMode & CACHE_WRITETHROUGH) {                         /*  属于写通模式 D CACHE        */
        _G_cacheopLib.CACHEOP_pfuncFlush      = LW_NULL;
        _G_cacheopLib.CACHEOP_pfuncFlushPage  = LW_NULL;
    }
    
    if (_G_uiDCacheMode & CACHE_SNOOP_ENABLE) {                         /*  D CACHE 始终与内存一致      */
        _G_cacheopLib.CACHEOP_pfuncFlush          = LW_NULL;
        _G_cacheopLib.CACHEOP_pfuncFlushPage      = LW_NULL;
#if LW_CFG_VMM_L4_HYPERVISOR_EN == 0
        _G_cacheopLib.CACHEOP_pfuncInvalidate     = LW_NULL;
        _G_cacheopLib.CACHEOP_pfuncInvalidatePage = LW_NULL;
#endif                                                                  /* !LW_CFG_VMM_L4_HYPERVISOR_EN */
        _G_cacheopLib.CACHEOP_pfuncClear          = LW_NULL;
        _G_cacheopLib.CACHEOP_pfuncClearPage      = LW_NULL;
    }
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

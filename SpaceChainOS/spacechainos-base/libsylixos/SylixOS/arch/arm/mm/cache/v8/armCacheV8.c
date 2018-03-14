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
** 文   件   名: armCacheV8.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 05 月 01 日
**
** 描        述: ARMv8 体系构架 CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv8A, R 体系构架
*********************************************************************************************************/
#if !defined(__SYLIXOS_ARM_ARCH_M__)
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "../armCacheCommon.h"
#include "../../mmu/armMmuCommon.h"
#include "../../../common/cp15/armCp15.h"
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
extern VOID     armDCacheV7Disable(VOID);
extern VOID     armDCacheV7FlushPoU(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     armDCacheV7FlushAll(VOID);
extern VOID     armDCacheV7FlushAllPoU(VOID);
extern VOID     armDCacheV7ClearAll(VOID);
extern UINT32   armCacheV7CCSIDR(VOID);
/*********************************************************************************************************
  CACHE 参数
*********************************************************************************************************/
static UINT32                           uiArmV8ICacheLineSize;
static UINT32                           uiArmV8DCacheLineSize;
#define ARMv8_CACHE_LOOP_OP_MAX_SIZE    (32 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
** 函数名称: armCacheV8Enable
** 功能描述: 使能 CACHE 
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  armCacheV8Enable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        armICacheEnable();
        armBranchPredictionEnable();

    } else {
        armDCacheEnable();
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV8Disable
** 功能描述: 禁能 CACHE 
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  armCacheV8Disable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        armICacheDisable();
        armBranchPredictionDisable();
        
    } else {
        armDCacheV7Disable();
    }
     
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV8Flush
** 功能描述: CACHE 脏数据回写
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : 由于 L2 为物理地址 tag 所以这里暂时使用 L2 全部回写指令.
*********************************************************************************************************/
static INT	armCacheV8Flush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == DATA_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7FlushAll();                                      /*  全部回写                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            armDCacheFlush(pvAdrs, (PVOID)ulEnd, uiArmV8DCacheLineSize);/*  部分回写                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV8FlushPage
** 功能描述: CACHE 脏数据回写
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT	armCacheV8FlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == DATA_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7FlushAll();                                      /*  全部回写                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            armDCacheFlush(pvAdrs, (PVOID)ulEnd, uiArmV8DCacheLineSize);/*  部分回写                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV8Invalidate
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址 (pvAdrs 必须等于物理地址)
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : 此函数如果操作 DCACHE pvAdrs 虚拟地址与物理地址必须相同.
*********************************************************************************************************/
static INT	armCacheV8Invalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE 全部无效             */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8ICacheLineSize);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV8ICacheLineSize);
        }
    } else {
        if (stBytes > 0) {                                              /*  必须 > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
            
            if (ulStart & ((addr_t)uiArmV8DCacheLineSize - 1)) {        /*  起始地址非 cache line 对齐  */
                ulStart &= ~((addr_t)uiArmV8DCacheLineSize - 1);
                armDCacheClear((PVOID)ulStart, (PVOID)ulStart, uiArmV8DCacheLineSize);
                ulStart += uiArmV8DCacheLineSize;
            }
            
            if (ulEnd & ((addr_t)uiArmV8DCacheLineSize - 1)) {          /*  结束地址非 cache line 对齐  */
                ulEnd &= ~((addr_t)uiArmV8DCacheLineSize - 1);
                armDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, uiArmV8DCacheLineSize);
            }
                                                                        /*  仅无效对齐部分              */
            armDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, uiArmV8DCacheLineSize);
            
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV8InvalidatePage
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT	armCacheV8InvalidatePage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE 全部无效             */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8ICacheLineSize);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV8ICacheLineSize);
        }
    } else {
        if (stBytes > 0) {                                              /*  必须 > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
                    
            if (ulStart & ((addr_t)uiArmV8DCacheLineSize - 1)) {        /*  起始地址非 cache line 对齐  */
                ulStart &= ~((addr_t)uiArmV8DCacheLineSize - 1);
                armDCacheClear((PVOID)ulStart, (PVOID)ulStart, uiArmV8DCacheLineSize);
                ulStart += uiArmV8DCacheLineSize;
            }
            
            if (ulEnd & ((addr_t)uiArmV8DCacheLineSize - 1)) {          /*  结束地址非 cache line 对齐  */
                ulEnd &= ~((addr_t)uiArmV8DCacheLineSize - 1);
                armDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, uiArmV8DCacheLineSize);
            }
                                                                        /*  仅无效对齐部分              */
            armDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, uiArmV8DCacheLineSize);
            
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV8Clear
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : 由于 L2 为物理地址 tag 所以这里暂时使用 L2 全部回写并无效指令.
*********************************************************************************************************/
static INT	armCacheV8Clear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE 全部无效             */
            
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8ICacheLineSize);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV8ICacheLineSize);
        }
    } else {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7ClearAll();                                      /*  全部回写并无效              */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            armDCacheClear(pvAdrs, (PVOID)ulEnd, uiArmV8DCacheLineSize);/*  部分回写并无效              */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV8ClearPage
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT	armCacheV8ClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE 全部无效             */
            
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8ICacheLineSize);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV8ICacheLineSize);
        }
    } else {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7ClearAll();                                      /*  全部回写并无效              */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            armDCacheClear(pvAdrs, (PVOID)ulEnd, uiArmV8DCacheLineSize);/*  部分回写并无效              */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV8Lock
** 功能描述: 锁定指定类型的 CACHE 
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT	armCacheV8Lock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: armCacheV8Unlock
** 功能描述: 解锁指定类型的 CACHE 
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT	armCacheV8Unlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: armCacheV8TextUpdate
** 功能描述: 清空(回写内存) D CACHE 无效(访问不命中) I CACHE
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : L2 cache 为统一 CACHE 所以 text update 不需要操作 L2 cache.
*********************************************************************************************************/
static INT	armCacheV8TextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
        armDCacheV7FlushAllPoU();                                       /*  DCACHE 全部回写             */
        armICacheInvalidateAll();                                       /*  ICACHE 全部无效             */
        
    } else {
        PVOID   pvAdrsBak = pvAdrs;

        ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
        armDCacheV7FlushPoU(pvAdrs, (PVOID)ulEnd, uiArmV8DCacheLineSize);

        ARM_CACHE_GET_END(pvAdrsBak, stBytes, ulEnd, uiArmV8ICacheLineSize);
        armICacheInvalidate(pvAdrsBak, (PVOID)ulEnd, uiArmV8ICacheLineSize);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV8DataUpdate
** 功能描述: 回写 D CACHE (仅回写 CPU 独享级 CACHE)
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
**           bInv                          是否为回写无效
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : L2 cache 为统一 CACHE 所以 data update 不需要操作 L2 cache.
*********************************************************************************************************/
static INT	armCacheV8DataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    addr_t  ulEnd;
    
    if (bInv == LW_FALSE) {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7FlushAll();                                      /*  全部回写                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            armDCacheFlush(pvAdrs, (PVOID)ulEnd, uiArmV8DCacheLineSize);/*  部分回写                    */
        }
    
    } else {
        if (stBytes >= ARMv8_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7ClearAll();                                      /*  全部回写                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV8DCacheLineSize);
            armDCacheClear(pvAdrs, (PVOID)ulEnd, uiArmV8DCacheLineSize);/*  部分回写                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archCacheV8Init
** 功能描述: 初始化 CACHE 
** 输　入  : pcacheop       CACHE 操作函数集
**           uiInstruction  指令 CACHE 参数
**           uiData         数据 CACHE 参数
**           pcMachineName  机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  armCacheV8Init (LW_CACHE_OP *pcacheop,
                      CACHE_MODE   uiInstruction, 
                      CACHE_MODE   uiData, 
                      CPCHAR       pcMachineName)
{
    UINT32  uiCCSIDR;

#define ARMv8_CCSIDR_LINESIZE_MASK      0x7
#define ARMv8_CCSIDR_LINESIZE(x)        ((x) & ARMv8_CCSIDR_LINESIZE_MASK)
#define ARMv8_CACHE_LINESIZE(x)         (16 << ARMv8_CCSIDR_LINESIZE(x))

#define ARMv8_CCSIDR_NUMSET_MASK        0xFFFE000
#define ARMv8_CCSIDR_NUMSET(x)          ((x) & ARMv8_CCSIDR_NUMSET_MASK)
#define ARMv8_CACHE_NUMSET(x)           ((ARMv8_CCSIDR_NUMSET(x) >> 13) + 1)

#define ARMv8_CCSIDR_WAYNUM_MSK         0x1FF8
#define ARMv8_CCSIDR_WAYNUM(x)          ((x) & ARMv8_CCSIDR_WAYNUM_MSK)
#define ARMv8_CACHE_WAYNUM(x)           ((ARMv8_CCSIDR_NUMSET(x) >> 3) + 1)

#if LW_CFG_SMP_EN > 0
    pcacheop->CACHEOP_ulOption = CACHE_TEXT_UPDATE_MP;
#else
    pcacheop->CACHEOP_ulOption = 0ul;
#endif                                                                  /*  LW_CFG_SMP_EN               */

    uiCCSIDR                      = armCacheV7CCSIDR();
    pcacheop->CACHEOP_iICacheLine = ARMv8_CACHE_LINESIZE(uiCCSIDR);
    pcacheop->CACHEOP_iDCacheLine = ARMv8_CACHE_LINESIZE(uiCCSIDR);
    
    uiArmV8ICacheLineSize = (UINT32)pcacheop->CACHEOP_iICacheLine;
    uiArmV8DCacheLineSize = (UINT32)pcacheop->CACHEOP_iDCacheLine;
    
    pcacheop->CACHEOP_iICacheWaySize = uiArmV8ICacheLineSize
                                     * ARMv8_CACHE_NUMSET(uiCCSIDR);    /*  DCACHE WaySize              */
    pcacheop->CACHEOP_iDCacheWaySize = uiArmV8DCacheLineSize
                                     * ARMv8_CACHE_NUMSET(uiCCSIDR);    /*  DCACHE WaySize              */

    _DebugFormat(__LOGMESSAGE_LEVEL, "ARMv8 I-Cache line size = %u bytes, Way size = %u bytes.\r\n",
                 pcacheop->CACHEOP_iICacheLine, pcacheop->CACHEOP_iICacheWaySize);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ARMv8 D-Cache line size = %u bytes, Way size = %u bytes.\r\n",
                 pcacheop->CACHEOP_iDCacheLine, pcacheop->CACHEOP_iDCacheWaySize);

    if ((lib_strcmp(pcMachineName, ARM_MACHINE_A53) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_A57) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_A72) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_A73) == 0)) {            /*  ARMv8                       */
        pcacheop->CACHEOP_iILoc = CACHE_LOCATION_PIPT;
        pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;
    }
    
    pcacheop->CACHEOP_pfuncEnable  = armCacheV8Enable;
    pcacheop->CACHEOP_pfuncDisable = armCacheV8Disable;
    
    pcacheop->CACHEOP_pfuncLock    = armCacheV8Lock;                    /*  暂时不支持锁定操作          */
    pcacheop->CACHEOP_pfuncUnlock  = armCacheV8Unlock;
    
    pcacheop->CACHEOP_pfuncFlush          = armCacheV8Flush;
    pcacheop->CACHEOP_pfuncFlushPage      = armCacheV8FlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = armCacheV8Invalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = armCacheV8InvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = armCacheV8Clear;
    pcacheop->CACHEOP_pfuncClearPage      = armCacheV8ClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = armCacheV8TextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = armCacheV8DataUpdate;
    
#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: archCacheV8Reset
** 功能描述: 复位 CACHE 
** 输　入  : pcMachineName  机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 如果有 lockdown 必须首先 unlock & invalidate 才能启动
*********************************************************************************************************/
VOID  armCacheV8Reset (CPCHAR  pcMachineName)
{
    armICacheInvalidateAll();
    armDCacheV7Disable();
    armICacheDisable();
    armBranchPredictorInvalidate();
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
/*********************************************************************************************************
  END
*********************************************************************************************************/

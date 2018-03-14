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
** 文   件   名: armCacheV7.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 09 日
**
** 描        述: ARMv7 体系构架 CACHE 驱动.
**
** BUG:
2014.11.12  L2 CACHE 只有 CPU 0 才能操作.
2015.08.21  修正 Invalidate 操作结束地址计算错误.
2015.11.25  Text Update 不需要清分支预测.
            Text Update 使用 armDCacheV7FlushPoU() 回写 DCACHE.
2016.04.29  加入 data update 功能.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv7A, R 体系构架
*********************************************************************************************************/
#if !defined(__SYLIXOS_ARM_ARCH_M__)
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "../armCacheCommon.h"
#include "../../mmu/armMmuCommon.h"
#include "../../../common/cp15/armCp15.h"
#if defined(__SYLIXOS_ARM_ARCH_R__)
#include "../../mpu/v7r/armMpuV7R.h"
#endif
/*********************************************************************************************************
  L2 CACHE 支持
*********************************************************************************************************/
#if LW_CFG_ARM_CACHE_L2 > 0
#include "../l2/armL2.h"
/*********************************************************************************************************
  L1 CACHE 状态
*********************************************************************************************************/
static INT      iCacheStatus = 0;
#define L1_CACHE_I_EN   0x01
#define L1_CACHE_D_EN   0x02
#define L1_CACHE_EN     (L1_CACHE_I_EN | L1_CACHE_D_EN)
#define L1_CACHE_DIS    0x00
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
#define ARMV7_CSSELR_IND_DATA_UNIFIED   0
#define ARMV7_CSSELR_IND_INSTRUCTION    1

extern VOID     armDCacheV7Disable(VOID);
extern VOID     armDCacheV7FlushPoU(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     armDCacheV7FlushAll(VOID);
extern VOID     armDCacheV7FlushAllPoU(VOID);
extern VOID     armDCacheV7ClearAll(VOID);
extern UINT32   armCacheV7CCSIDR(VOID);
/*********************************************************************************************************
  CACHE 参数
*********************************************************************************************************/
static UINT32                           uiArmV7ICacheLineSize;
static UINT32                           uiArmV7DCacheLineSize;
#define ARMv7_CACHE_LOOP_OP_MAX_SIZE    (16 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
** 函数名称: armCacheV7Enable
** 功能描述: 使能 CACHE 
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  armCacheV7Enable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        armICacheEnable();
#if LW_CFG_ARM_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus |= L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
        armBranchPredictionEnable();

    } else {
        armDCacheEnable();
#if LW_CFG_ARM_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus |= L1_CACHE_D_EN;
        }
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
    }
    
#if LW_CFG_ARM_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) && 
        (iCacheStatus == L1_CACHE_EN)) {
        armL2Enable();
    }
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV7Disable
** 功能描述: 禁能 CACHE 
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  armCacheV7Disable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        armICacheDisable();
#if LW_CFG_ARM_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus &= ~L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
        armBranchPredictionDisable();
        
    } else {
        armDCacheV7Disable();
#if LW_CFG_ARM_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus &= ~L1_CACHE_D_EN;
        }
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
    }
    
#if LW_CFG_ARM_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) && 
        (iCacheStatus == L1_CACHE_DIS)) {
        armL2Disable();
    }
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
     
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV7Flush
** 功能描述: CACHE 脏数据回写
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : 由于 L2 为物理地址 tag 所以这里暂时使用 L2 全部回写指令.
*********************************************************************************************************/
static INT	armCacheV7Flush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == DATA_CACHE) {
        if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7FlushAll();                                      /*  全部回写                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7DCacheLineSize);
            armDCacheFlush(pvAdrs, (PVOID)ulEnd, uiArmV7DCacheLineSize);/*  部分回写                    */
        }
        
#if LW_CFG_ARM_CACHE_L2 > 0
        armL2FlushAll();
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV7FlushPage
** 功能描述: CACHE 脏数据回写
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT	armCacheV7FlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == DATA_CACHE) {
        if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7FlushAll();                                      /*  全部回写                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7DCacheLineSize);
            armDCacheFlush(pvAdrs, (PVOID)ulEnd, uiArmV7DCacheLineSize);/*  部分回写                    */
        }
        
#if LW_CFG_ARM_CACHE_L2 > 0
        armL2Flush(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV7Invalidate
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址 (pvAdrs 必须等于物理地址)
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : 此函数如果操作 DCACHE pvAdrs 虚拟地址与物理地址必须相同.
*********************************************************************************************************/
static INT	armCacheV7Invalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE 全部无效             */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7ICacheLineSize);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV7ICacheLineSize);
        }
    } else {
        if (stBytes > 0) {                                              /*  必须 > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
            
            if (ulStart & ((addr_t)uiArmV7DCacheLineSize - 1)) {        /*  起始地址非 cache line 对齐  */
                ulStart &= ~((addr_t)uiArmV7DCacheLineSize - 1);
                armDCacheClear((PVOID)ulStart, (PVOID)ulStart, uiArmV7DCacheLineSize);
                ulStart += uiArmV7DCacheLineSize;
            }
            
            if (ulEnd & ((addr_t)uiArmV7DCacheLineSize - 1)) {          /*  结束地址非 cache line 对齐  */
                ulEnd &= ~((addr_t)uiArmV7DCacheLineSize - 1);
                armDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, uiArmV7DCacheLineSize);
            }
                                                                        /*  仅无效对齐部分              */
            armDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, uiArmV7DCacheLineSize);
            
#if LW_CFG_ARM_CACHE_L2 > 0
            armL2Invalidate(pvAdrs, stBytes);                           /*  虚拟与物理地址必须相同      */
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV7InvalidatePage
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT	armCacheV7InvalidatePage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE 全部无效             */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7ICacheLineSize);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV7ICacheLineSize);
        }
    } else {
        if (stBytes > 0) {                                              /*  必须 > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
                    
            if (ulStart & ((addr_t)uiArmV7DCacheLineSize - 1)) {        /*  起始地址非 cache line 对齐  */
                ulStart &= ~((addr_t)uiArmV7DCacheLineSize - 1);
                armDCacheClear((PVOID)ulStart, (PVOID)ulStart, uiArmV7DCacheLineSize);
                ulStart += uiArmV7DCacheLineSize;
            }
            
            if (ulEnd & ((addr_t)uiArmV7DCacheLineSize - 1)) {          /*  结束地址非 cache line 对齐  */
                ulEnd &= ~((addr_t)uiArmV7DCacheLineSize - 1);
                armDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, uiArmV7DCacheLineSize);
            }
                                                                        /*  仅无效对齐部分              */
            armDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, uiArmV7DCacheLineSize);
            
#if LW_CFG_ARM_CACHE_L2 > 0
            armL2Invalidate(pvPdrs, stBytes);                           /*  虚拟与物理地址必须相同      */
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV7Clear
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : 由于 L2 为物理地址 tag 所以这里暂时使用 L2 全部回写并无效指令.
*********************************************************************************************************/
static INT	armCacheV7Clear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE 全部无效             */
            
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7ICacheLineSize);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV7ICacheLineSize);
        }
    } else {
        if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7ClearAll();                                      /*  全部回写并无效              */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7DCacheLineSize);
            armDCacheClear(pvAdrs, (PVOID)ulEnd, uiArmV7DCacheLineSize);/*  部分回写并无效              */
        }
        
#if LW_CFG_ARM_CACHE_L2 > 0
        armL2ClearAll();
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV7ClearPage
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT	armCacheV7ClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
            armICacheInvalidateAll();                                   /*  ICACHE 全部无效             */
            
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7ICacheLineSize);
            armICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiArmV7ICacheLineSize);
        }
    } else {
        if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7ClearAll();                                      /*  全部回写并无效              */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7DCacheLineSize);
            armDCacheClear(pvAdrs, (PVOID)ulEnd, uiArmV7DCacheLineSize);/*  部分回写并无效              */
        }
        
#if LW_CFG_ARM_CACHE_L2 > 0
        armL2Clear(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV7Lock
** 功能描述: 锁定指定类型的 CACHE 
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT	armCacheV7Lock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: armCacheV7Unlock
** 功能描述: 解锁指定类型的 CACHE 
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT	armCacheV7Unlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: armCacheV7TextUpdate
** 功能描述: 清空(回写内存) D CACHE 无效(访问不命中) I CACHE
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : L2 cache 为统一 CACHE 所以 text update 不需要操作 L2 cache.
*********************************************************************************************************/
static INT	armCacheV7TextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
        armDCacheV7FlushAllPoU();                                       /*  DCACHE 全部回写             */
        armICacheInvalidateAll();                                       /*  ICACHE 全部无效             */
        
    } else {
        PVOID   pvAdrsBak = pvAdrs;

        ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7DCacheLineSize);
        armDCacheV7FlushPoU(pvAdrs, (PVOID)ulEnd, uiArmV7DCacheLineSize);

        ARM_CACHE_GET_END(pvAdrsBak, stBytes, ulEnd, uiArmV7ICacheLineSize);
        armICacheInvalidate(pvAdrsBak, (PVOID)ulEnd, uiArmV7ICacheLineSize);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV7DataUpdate
** 功能描述: 回写 D CACHE (仅回写 CPU 独享级 CACHE)
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
**           bInv                          是否为回写无效
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : L2 cache 为统一 CACHE 所以 data update 不需要操作 L2 cache.
*********************************************************************************************************/
static INT	armCacheV7DataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    addr_t  ulEnd;
    
    if (bInv == LW_FALSE) {
        if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7FlushAll();                                      /*  全部回写                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7DCacheLineSize);
            armDCacheFlush(pvAdrs, (PVOID)ulEnd, uiArmV7DCacheLineSize);/*  部分回写                    */
        }
    
    } else {
        if (stBytes >= ARMv7_CACHE_LOOP_OP_MAX_SIZE) {
            armDCacheV7ClearAll();                                      /*  全部回写                    */
        
        } else {
            ARM_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiArmV7DCacheLineSize);
            armDCacheClear(pvAdrs, (PVOID)ulEnd, uiArmV7DCacheLineSize);/*  部分回写                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archCacheV7Init
** 功能描述: 初始化 CACHE 
** 输　入  : pcacheop       CACHE 操作函数集
**           uiInstruction  指令 CACHE 参数
**           uiData         数据 CACHE 参数
**           pcMachineName  机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  armCacheV7Init (LW_CACHE_OP *pcacheop, 
                      CACHE_MODE   uiInstruction, 
                      CACHE_MODE   uiData, 
                      CPCHAR       pcMachineName)
{
    UINT32  uiCCSIDR;

#define ARMv7_CCSIDR_LINESIZE_MASK      0x7
#define ARMv7_CCSIDR_LINESIZE(x)        ((x) & ARMv7_CCSIDR_LINESIZE_MASK)
#define ARMv7_CACHE_LINESIZE(x)         (16 << ARMv7_CCSIDR_LINESIZE(x))

#define ARMv7_CCSIDR_NUMSET_MASK        0xFFFE000
#define ARMv7_CCSIDR_NUMSET(x)          ((x) & ARMv7_CCSIDR_NUMSET_MASK)
#define ARMv7_CACHE_NUMSET(x)           ((ARMv7_CCSIDR_NUMSET(x) >> 13) + 1)

#define ARMv7_CCSIDR_WAYNUM_MSK         0x1FF8
#define ARMv7_CCSIDR_WAYNUM(x)          ((x) & ARMv7_CCSIDR_WAYNUM_MSK)
#define ARMv7_CACHE_WAYNUM(x)           ((ARMv7_CCSIDR_NUMSET(x) >> 3) + 1)

#if LW_CFG_ARM_CACHE_L2 > 0
    armL2Init(uiInstruction, uiData, pcMachineName);
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */

#if LW_CFG_SMP_EN > 0
    pcacheop->CACHEOP_ulOption = CACHE_TEXT_UPDATE_MP;
#else
    pcacheop->CACHEOP_ulOption = 0ul;
#endif                                                                  /*  LW_CFG_SMP_EN               */

    uiCCSIDR                      = armCacheV7CCSIDR();
    pcacheop->CACHEOP_iICacheLine = ARMv7_CACHE_LINESIZE(uiCCSIDR);
    pcacheop->CACHEOP_iDCacheLine = ARMv7_CACHE_LINESIZE(uiCCSIDR);
    
    uiArmV7ICacheLineSize = (UINT32)pcacheop->CACHEOP_iICacheLine;
    uiArmV7DCacheLineSize = (UINT32)pcacheop->CACHEOP_iDCacheLine;
    
    pcacheop->CACHEOP_iICacheWaySize = uiArmV7ICacheLineSize
                                     * ARMv7_CACHE_NUMSET(uiCCSIDR);    /*  DCACHE WaySize              */
    pcacheop->CACHEOP_iDCacheWaySize = uiArmV7DCacheLineSize
                                     * ARMv7_CACHE_NUMSET(uiCCSIDR);    /*  DCACHE WaySize              */

    _DebugFormat(__LOGMESSAGE_LEVEL, "ARMv7 I-Cache line size = %u bytes, Way size = %u bytes.\r\n",
                 pcacheop->CACHEOP_iICacheLine, pcacheop->CACHEOP_iICacheWaySize);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ARMv7 D-Cache line size = %u bytes, Way size = %u bytes.\r\n",
                 pcacheop->CACHEOP_iDCacheLine, pcacheop->CACHEOP_iDCacheWaySize);

    if ((lib_strcmp(pcMachineName, ARM_MACHINE_A5) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_A7) == 0)) {
        pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIPT;
        pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;
        
    } else if (lib_strcmp(pcMachineName, ARM_MACHINE_A9) == 0) {
        pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIPT;
        pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;
        armAuxControlFeatureEnable(AUX_CTRL_A9_L1_PREFETCH);            /*  Cortex-A9 使能 L1 预取      */
    
    } else if (lib_strcmp(pcMachineName, ARM_MACHINE_A8) == 0) {
        pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIPT;
        pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;
        armAuxControlFeatureEnable(AUX_CTRL_A8_FORCE_ETM_CLK |
                                   AUX_CTRL_A8_FORCE_MAIN_CLK |
                                   AUX_CTRL_A8_L1NEON |
                                   AUX_CTRL_A8_FORCE_NEON_CLK |
                                   AUX_CTRL_A8_FORCE_NEON_SIGNAL);
    
    } else if (lib_strcmp(pcMachineName, ARM_MACHINE_A15) == 0) {
        pcacheop->CACHEOP_iILoc = CACHE_LOCATION_PIPT;
        pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;
        armAuxControlFeatureEnable(AUX_CTRL_A15_FORCE_MAIN_CLK |
                                   AUX_CTRL_A15_FORCE_NEON_CLK);
    
    } else if (lib_strcmp(pcMachineName, ARM_MACHINE_A17) == 0) {
        pcacheop->CACHEOP_iILoc = CACHE_LOCATION_PIPT;
        pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;
        armAuxControlFeatureEnable(AUX_CTRL_A17_L1_PREFETCH |
                                   AUX_CTRL_A17_L2_PREFETCH);

    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_R4) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_R5) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_R7) == 0)) {
        pcacheop->CACHEOP_iILoc = CACHE_LOCATION_PIPT;
        pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;
    }
    
    pcacheop->CACHEOP_pfuncEnable  = armCacheV7Enable;
    pcacheop->CACHEOP_pfuncDisable = armCacheV7Disable;
    
    pcacheop->CACHEOP_pfuncLock    = armCacheV7Lock;                    /*  暂时不支持锁定操作          */
    pcacheop->CACHEOP_pfuncUnlock  = armCacheV7Unlock;
    
    pcacheop->CACHEOP_pfuncFlush          = armCacheV7Flush;
    pcacheop->CACHEOP_pfuncFlushPage      = armCacheV7FlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = armCacheV7Invalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = armCacheV7InvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = armCacheV7Clear;
    pcacheop->CACHEOP_pfuncClearPage      = armCacheV7ClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = armCacheV7TextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = armCacheV7DataUpdate;
    
#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;

#elif (LW_CFG_VMM_EN == 0) && (LW_CFG_ARM_MPU > 0)
    pcacheop->CACHEOP_pfuncDmaMalloc      = armMpuV7RDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = armMpuV7RDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = armMpuV7RDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: archCacheV7Reset
** 功能描述: 复位 CACHE 
** 输　入  : pcMachineName  机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 如果有 lockdown 必须首先 unlock & invalidate 才能启动
*********************************************************************************************************/
VOID  armCacheV7Reset (CPCHAR  pcMachineName)
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

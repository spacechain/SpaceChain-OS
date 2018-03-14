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
** 文   件   名: armCacheV7M.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 11 月 08 日
**
** 描        述: ARMv7 Cortex-Mx 体系构架 CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  ARMv7M 体系构架
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__)
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "../../mpu/v7m/armMpuV7M.h"
/*********************************************************************************************************
** 函数名称: armCacheV7MEnable
** 功能描述: 使能 CACHE
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  armCacheV7MEnable (LW_CACHE_TYPE  cachetype)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV7MDisable
** 功能描述: 禁能 CACHE
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  armCacheV7MDisable (LW_CACHE_TYPE  cachetype)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV7MFlush
** 功能描述: CACHE 脏数据回写
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  armCacheV7MFlush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV7MInvalidate
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  armCacheV7MInvalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV7MClear
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  armCacheV7MClear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: armCacheV7MLock
** 功能描述: 锁定指定类型的 CACHE
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  armCacheV7MLock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: armCacheV7MUnlock
** 功能描述: 解锁指定类型的 CACHE
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  armCacheV7MUnlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: armCacheV7MTextUpdate
** 功能描述: 清空(回写内存) D CACHE 无效(访问不命中) I CACHE
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : L2 cache 为统一 CACHE 所以 text update 不需要操作 L2 cache.
*********************************************************************************************************/
static INT  armCacheV7MTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archCacheV7MInit
** 功能描述: 初始化 CACHE
** 输　入  : pcacheop       CACHE 操作函数集
**           uiInstruction  指令 CACHE 参数
**           uiData         数据 CACHE 参数
**           pcMachineName  机器名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  armCacheV7MInit (LW_CACHE_OP *pcacheop,
                               CACHE_MODE   uiInstruction,
                               CACHE_MODE   uiData,
                               CPCHAR       pcMachineName)
{
    pcacheop->CACHEOP_ulOption = 0ul;

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_PIPT;
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_PIPT;

    pcacheop->CACHEOP_iICacheLine = 32;
    pcacheop->CACHEOP_iDCacheLine = 32;

    pcacheop->CACHEOP_iICacheWaySize = ((16 * LW_CFG_KB_SIZE) / 64);
    pcacheop->CACHEOP_iDCacheWaySize = ((16 * LW_CFG_KB_SIZE) / 64);

    pcacheop->CACHEOP_pfuncEnable  = armCacheV7MEnable;
    pcacheop->CACHEOP_pfuncDisable = armCacheV7MDisable;

    pcacheop->CACHEOP_pfuncLock    = armCacheV7MLock;
    pcacheop->CACHEOP_pfuncUnlock  = armCacheV7MUnlock;

    pcacheop->CACHEOP_pfuncFlush          = armCacheV7MFlush;
    pcacheop->CACHEOP_pfuncFlushPage      = LW_NULL;
    pcacheop->CACHEOP_pfuncInvalidate     = armCacheV7MInvalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = LW_NULL;
    pcacheop->CACHEOP_pfuncClear          = armCacheV7MClear;
    pcacheop->CACHEOP_pfuncClearPage      = LW_NULL;
    pcacheop->CACHEOP_pfuncTextUpdate     = armCacheV7MTextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = LW_NULL;

#if (LW_CFG_VMM_EN == 0) && (LW_CFG_ARM_MPU > 0)
    pcacheop->CACHEOP_pfuncDmaMalloc      = armMpuV7MDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = armMpuV7MDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = armMpuV7MDmaFree;
#endif                                                                  /*  LW_CFG_ARM_MPU > 0          */
}
/*********************************************************************************************************
** 函数名称: archCacheV7MReset
** 功能描述: 复位 CACHE
** 输　入  : pcMachineName  机器名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  armCacheV7MReset (CPCHAR  pcMachineName)
{
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
#endif                                                                  /*  !__SYLIXOS_ARM_ARCH_M__     */
/*********************************************************************************************************
  END
*********************************************************************************************************/

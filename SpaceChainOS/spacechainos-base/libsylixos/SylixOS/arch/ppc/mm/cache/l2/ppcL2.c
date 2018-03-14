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
** 文   件   名: ppcL2.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 01 月 28 日
**
** 描        述: PowerPC 体系构架 L2 CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_IO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁减配置
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0 && LW_CFG_PPC_CACHE_L2 > 0
#include "ppcL2.h"
/*********************************************************************************************************
  L2 锁 (多核共享一个 L2 CACHE, 所以操作时需要加自旋锁, 由于外层已经关中断, 这里只需锁自旋锁即可)
*********************************************************************************************************/
static LW_SPINLOCK_DEFINE_CACHE_ALIGN(_G_l2sl);
#define L2_OP_ENTER()   LW_SPIN_LOCK_IGNIRQ(&_G_l2sl)
#define L2_OP_EXIT()    LW_SPIN_UNLOCK_IGNIRQ(&_G_l2sl)
/*********************************************************************************************************
  L2 驱动
*********************************************************************************************************/
static L2C_DRVIER       _G_l2cdrv;
/*********************************************************************************************************
  声明 L2 控制器初始化函数
*********************************************************************************************************/
VOID  ppcL2Cache750Init(L2C_DRVIER  *pl2cdrv,
                        CACHE_MODE   uiInstruction,
                        CACHE_MODE   uiData,
                        CPCHAR       pcMachineName);
/*********************************************************************************************************
** 函数名称: bspL2CacheInit
** 功能描述: BSP 相关的 L2 CACHE 初始化
** 输　入  : pl2cdrv            L2 驱动
**           uiInstruction      指令 CACHE 类型
**           uiData             数据 CACHE 类型
**           pcMachineName      机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_WEAK VOID  bspL2CacheInit (PVOID        pl2cdrv,
                              CACHE_MODE   uiInstruction,
                              CACHE_MODE   uiData,
                              CPCHAR       pcMachineName)
{
    _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
}
/*********************************************************************************************************
** 函数名称: ppcL2Enable
** 功能描述: 使能 L2 CACHE
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  ppcL2Enable (VOID)
{
    L2_OP_ENTER();
    if (_G_l2cdrv.L2CD_pfuncEnable) {
        _G_l2cdrv.L2CD_pfuncEnable(&_G_l2cdrv);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** 函数名称: ppcL2Disable
** 功能描述: 禁能 L2 CACHE 
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  ppcL2Disable (VOID)
{
    L2_OP_ENTER();
    if (_G_l2cdrv.L2CD_pfuncDisable) {
        _G_l2cdrv.L2CD_pfuncDisable(&_G_l2cdrv);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** 函数名称: ppcL2IsEnable
** 功能描述: L2 CACHE 是否打开
** 输　入  : NONE
** 输　出  : L2 CACHE 是否打开
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
BOOL  ppcL2IsEnable (VOID)
{
    BOOL    bIsEnable;

    L2_OP_ENTER();
    if (_G_l2cdrv.L2CD_pfuncIsEnable) {
        bIsEnable = _G_l2cdrv.L2CD_pfuncIsEnable(&_G_l2cdrv);
    } else {
        bIsEnable = LW_FALSE;
    }
    L2_OP_EXIT();
    
    return  (bIsEnable);
}
/*********************************************************************************************************
** 函数名称: ppcL2Sync
** 功能描述: L2 CACHE 同步
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  ppcL2Sync (VOID)
{
    L2_OP_ENTER();
    if (_G_l2cdrv.L2CD_pfuncSync) {
        _G_l2cdrv.L2CD_pfuncSync(&_G_l2cdrv);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** 函数名称: ppcL2FlushAll
** 功能描述: L2 CACHE 回写所有脏数据
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  ppcL2FlushAll (VOID)
{
    L2_OP_ENTER();
    if (_G_l2cdrv.L2CD_pfuncFlushAll) {
        _G_l2cdrv.L2CD_pfuncFlushAll(&_G_l2cdrv);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** 函数名称: ppcL2Flush
** 功能描述: L2 CACHE 回写部分脏数据
** 输　入  : pvPdrs        起始虚拟地址
**           stBytes       数据块大小
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  ppcL2Flush (PVOID  pvPdrs, size_t  stBytes)
{
    L2_OP_ENTER();
    if (_G_l2cdrv.L2CD_pfuncFlush) {
        _G_l2cdrv.L2CD_pfuncFlush(&_G_l2cdrv, pvPdrs, stBytes);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** 函数名称: ppcL2InvalidateAll
** 功能描述: L2 CACHE 无效
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  ppcL2InvalidateAll (VOID)
{
    L2_OP_ENTER();
    if (_G_l2cdrv.L2CD_pfuncInvalidateAll) {
        _G_l2cdrv.L2CD_pfuncInvalidateAll(&_G_l2cdrv);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** 函数名称: ppcL2InvalidateAll
** 功能描述: L2 CACHE 无效
** 输　入  : pvPdrs        起始虚拟地址
**           stBytes       数据块大小
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  ppcL2Invalidate (PVOID  pvPdrs, size_t  stBytes)
{
    L2_OP_ENTER();
    if (_G_l2cdrv.L2CD_pfuncInvalidate) {
        _G_l2cdrv.L2CD_pfuncInvalidate(&_G_l2cdrv, pvPdrs, stBytes);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** 函数名称: ppcL2ClearAll
** 功能描述: L2 CACHE 回写并无效
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  ppcL2ClearAll (VOID)
{
    L2_OP_ENTER();
    if (_G_l2cdrv.L2CD_pfuncClearAll) {
        _G_l2cdrv.L2CD_pfuncClearAll(&_G_l2cdrv);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** 函数名称: ppcL2Clear
** 功能描述: L2 CACHE 回写并无效
** 输　入  : pvPdrs        起始虚拟地址
**           stBytes       数据块大小
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  ppcL2Clear (PVOID  pvPdrs, size_t  stBytes)
{
    L2_OP_ENTER();
    if (_G_l2cdrv.L2CD_pfuncClear) {
        _G_l2cdrv.L2CD_pfuncClear(&_G_l2cdrv, pvPdrs, stBytes);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** 函数名称: ppcL2Name
** 功能描述: 获得 L2 CACHE 控制器名称
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
CPCHAR  ppcL2Name (VOID)
{
    return  (_G_l2cdrv.L2CD_pcName);
}
/*********************************************************************************************************
** 函数名称: ppcL2Init
** 功能描述: 初始化 L2 CACHE 控制器
** 输　入  : uiInstruction      指令 CACHE 类型
**           uiData             数据 CACHE 类型
**           pcMachineName      机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  ppcL2Init (CACHE_MODE   uiInstruction,
                 CACHE_MODE   uiData,
                 CPCHAR       pcMachineName)
{
    LW_SPIN_INIT(&_G_l2sl);
    
    if ((lib_strcmp(pcMachineName, PPC_MACHINE_750)  == 0) ||
        (lib_strcmp(pcMachineName, PPC_MACHINE_745X) == 0)) {
        _G_l2cdrv.L2CD_pcName    = pcMachineName;
        _G_l2cdrv.L2CD_uiType    = 0;
        _G_l2cdrv.L2CD_uiRelease = 0;
        
        _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s L2 cache controller initialization.\r\n", 
                     LW_CFG_CPU_ARCH_FAMILY, _G_l2cdrv.L2CD_pcName);

        ppcL2Cache750Init(&_G_l2cdrv, uiInstruction, uiData, pcMachineName);

    } else {
        bspL2CacheInit((PVOID)(&_G_l2cdrv), uiInstruction, uiData, pcMachineName);
    }
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                                                                        /*  LW_CFG_PPC_CACHE_L2 > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/

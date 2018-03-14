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
** 文   件   名: armL2.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 09 日
**
** 描        述: ARM 体系构架 L2 CACHE 驱动
** 注        意: 无论处理器是大端还是小端, L2 控制器均为小端.
**
** BUG:
2015.08.20  加入对 ARMv8 支持.
*********************************************************************************************************/
#define  __SYLIXOS_IO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁减配置
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0 && LW_CFG_ARM_CACHE_L2 > 0
#include "armL2.h"
#include "../../../common/cp15/armCp15.h"
/*********************************************************************************************************
  L2 锁 (多核共享一个 L2 CACHE, 所以操作时需要加自旋锁, 由于外层已经关中断, 这里只需锁自旋锁即可)
*********************************************************************************************************/
static LW_SPINLOCK_DEFINE_CACHE_ALIGN(l2sl);
#define L2_OP_ENTER()   LW_SPIN_LOCK_IGNIRQ(&l2sl)
#define L2_OP_EXIT()    LW_SPIN_UNLOCK_IGNIRQ(&l2sl)
/*********************************************************************************************************
  L2 驱动
*********************************************************************************************************/
static L2C_DRVIER       l2cdrv;
/*********************************************************************************************************
  L2 控制器初始化函数
*********************************************************************************************************/
extern VOID     armL2A8Init(L2C_DRVIER  *pl2cdrv,
                            CACHE_MODE   uiInstruction,
                            CACHE_MODE   uiData,
                            CPCHAR       pcMachineName);
extern VOID     armL2A17Init(L2C_DRVIER  *pl2cdrv,
                             CACHE_MODE   uiInstruction,
                             CACHE_MODE   uiData,
                             CPCHAR       pcMachineName);
extern VOID     armL2x0Init(L2C_DRVIER  *pl2cdrv,
                            CACHE_MODE   uiInstruction,
                            CACHE_MODE   uiData,
                            CPCHAR       pcMachineName,
                            UINT32       uiAux);
/*********************************************************************************************************
** 函数名称: armL2Enable
** 功能描述: 使能 L2 CACHE 
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID armL2Enable (VOID)
{
    L2_OP_ENTER();
    if (l2cdrv.L2CD_pfuncEnable) {
        l2cdrv.L2CD_pfuncEnable(&l2cdrv);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** 函数名称: armL2Disable
** 功能描述: 禁能 L2 CACHE 
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID armL2Disable (VOID)
{
    L2_OP_ENTER();
    if (l2cdrv.L2CD_pfuncDisable) {
        l2cdrv.L2CD_pfuncDisable(&l2cdrv);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** 函数名称: armL2IsEnable
** 功能描述: L2 CACHE 是否打开
** 输　入  : NONE
** 输　出  : L2 CACHE 是否打开
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
BOOL armL2IsEnable (VOID)
{
    BOOL    bIsEnable;

    L2_OP_ENTER();
    if (l2cdrv.L2CD_pfuncIsEnable) {
        bIsEnable = l2cdrv.L2CD_pfuncIsEnable(&l2cdrv);
    } else {
        bIsEnable = LW_FALSE;
    }
    L2_OP_EXIT();
    
    return  (bIsEnable);
}
/*********************************************************************************************************
** 函数名称: armL2Sync
** 功能描述: L2 CACHE 同步
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID armL2Sync (VOID)
{
    L2_OP_ENTER();
    if (l2cdrv.L2CD_pfuncSync) {
        l2cdrv.L2CD_pfuncSync(&l2cdrv);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** 函数名称: armL2FlushAll
** 功能描述: L2 CACHE 回写所有脏数据
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID armL2FlushAll (VOID)
{
    L2_OP_ENTER();
    if (l2cdrv.L2CD_pfuncFlushAll) {
        l2cdrv.L2CD_pfuncFlushAll(&l2cdrv);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** 函数名称: armL2Flush
** 功能描述: L2 CACHE 回写部分脏数据
** 输　入  : pvPdrs        起始虚拟地址
**           stBytes       数据块大小
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID armL2Flush (PVOID  pvPdrs, size_t  stBytes)
{
    L2_OP_ENTER();
    if (l2cdrv.L2CD_pfuncFlush) {
        l2cdrv.L2CD_pfuncFlush(&l2cdrv, pvPdrs, stBytes);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** 函数名称: armL2InvalidateAll
** 功能描述: L2 CACHE 无效
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID armL2InvalidateAll (VOID)
{
    L2_OP_ENTER();
    if (l2cdrv.L2CD_pfuncInvalidateAll) {
        l2cdrv.L2CD_pfuncInvalidateAll(&l2cdrv);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** 函数名称: armL2InvalidateAll
** 功能描述: L2 CACHE 无效
** 输　入  : pvPdrs        起始虚拟地址
**           stBytes       数据块大小
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID armL2Invalidate (PVOID  pvPdrs, size_t  stBytes)
{
    L2_OP_ENTER();
    if (l2cdrv.L2CD_pfuncInvalidate) {
        l2cdrv.L2CD_pfuncInvalidate(&l2cdrv, pvPdrs, stBytes);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** 函数名称: armL2ClearAll
** 功能描述: L2 CACHE 回写并无效
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID armL2ClearAll (VOID)
{
    L2_OP_ENTER();
    if (l2cdrv.L2CD_pfuncClearAll) {
        l2cdrv.L2CD_pfuncClearAll(&l2cdrv);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** 函数名称: armL2Clear
** 功能描述: L2 CACHE 回写并无效
** 输　入  : pvPdrs        起始虚拟地址
**           stBytes       数据块大小
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID armL2Clear (PVOID  pvPdrs, size_t  stBytes)
{
    L2_OP_ENTER();
    if (l2cdrv.L2CD_pfuncClear) {
        l2cdrv.L2CD_pfuncClear(&l2cdrv, pvPdrs, stBytes);
    }
    L2_OP_EXIT();
}
/*********************************************************************************************************
** 函数名称: armL2Name
** 功能描述: 获得 L2 CACHE 控制器名称
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
CPCHAR  armL2Name (VOID)
{
    return  (l2cdrv.L2CD_pcName);
}
/*********************************************************************************************************
** 函数名称: armL2Init
** 功能描述: 初始化 L2 CACHE 控制器
** 输　入  : uiInstruction      指令 CACHE 类型
**           uiData             数据 CACHE 类型
**           pcMachineName      机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID armL2Init (CACHE_MODE   uiInstruction,
                CACHE_MODE   uiData,
                CPCHAR       pcMachineName)
{
    UINT32  uiCacheId;
    UINT32  uiAux;
    UINT32  uiAuxVal;
    UINT32  uiAuxMask;
    UINT32  uiWays;
    UINT32  uiWaySize;
    UINT32  uiWaySizeShift = L2C_WAY_SIZE_SHIFT;

    LW_SPIN_INIT(&l2sl);
    
    if (lib_strcmp(pcMachineName, ARM_MACHINE_A8) == 0) {               /*  A8 处理器 L2 CACHE          */
        l2cdrv.L2CD_pcName    = ARM_MACHINE_A8;
        l2cdrv.L2CD_ulBase    = 0ul;
        l2cdrv.L2CD_uiWayMask = 0;
        l2cdrv.L2CD_uiAux     = 0;
        l2cdrv.L2CD_uiType    = 0;
        l2cdrv.L2CD_uiRelease = 0;
        
        _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s L2 cache controller initialization.\r\n", 
                     LW_CFG_CPU_ARCH_FAMILY, l2cdrv.L2CD_pcName);
        
        armL2A8Init(&l2cdrv, uiInstruction, uiData, pcMachineName);

    } else if (lib_strcmp(pcMachineName, ARM_MACHINE_A7) == 0) {        /*  A7 处理器 L2 CACHE          */
        l2cdrv.L2CD_pcName    = ARM_MACHINE_A7;
        l2cdrv.L2CD_ulBase    = 0ul;
        l2cdrv.L2CD_uiWayMask = 0;
        l2cdrv.L2CD_uiAux     = 0;
        l2cdrv.L2CD_uiType    = 0;
        l2cdrv.L2CD_uiRelease = 0;
        
        _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s L2 cache controller initialization.\r\n", 
                     LW_CFG_CPU_ARCH_FAMILY, l2cdrv.L2CD_pcName);
                     
    } else if (lib_strcmp(pcMachineName, ARM_MACHINE_A15) == 0) {       /*  A15 处理器 L2 CACHE         */
#if LW_CFG_ARM_CACHE_L2_ECC > 0
        UINT32  uiL2Ctl = armA1xL2CtlGet();
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2_ECC > 0 */

        l2cdrv.L2CD_pcName    = ARM_MACHINE_A15;
        l2cdrv.L2CD_ulBase    = 0ul;
        l2cdrv.L2CD_uiWayMask = 0;
        l2cdrv.L2CD_uiAux     = 0;
        l2cdrv.L2CD_uiType    = 0;
        l2cdrv.L2CD_uiRelease = 0;
        
        _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s L2 cache controller initialization.\r\n", 
                     LW_CFG_CPU_ARCH_FAMILY, l2cdrv.L2CD_pcName);

#if LW_CFG_ARM_CACHE_L2_ECC > 0
        uiL2Ctl |= A15_L2_CTL_L2_ECC_EN;
        armA1xL2CtlSet(uiL2Ctl);
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2_ECC > 0 */

    } else if (lib_strcmp(pcMachineName, ARM_MACHINE_A17) == 0) {       /*  A17 处理器 L2 CACHE         */
        l2cdrv.L2CD_pcName    = ARM_MACHINE_A17;
        l2cdrv.L2CD_ulBase    = 0ul;
        l2cdrv.L2CD_uiWayMask = 0;
        l2cdrv.L2CD_uiAux     = 0;
        l2cdrv.L2CD_uiType    = 0;
        l2cdrv.L2CD_uiRelease = 0;
        
        _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s L2 cache controller initialization.\r\n", 
                     LW_CFG_CPU_ARCH_FAMILY, l2cdrv.L2CD_pcName);
        
        armL2A17Init(&l2cdrv, uiInstruction, uiData, pcMachineName);
        
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_A5)  == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A9)  == 0)) {
        if (bspL2CBase(&(l2cdrv.L2CD_ulBase))) {                        /*  获得控制器基地址            */
            return;
        }
        if (bspL2CAux(&uiAuxVal, &uiAuxMask)) {
            return;
        }
    
        uiCacheId = read32_le(L2C_BASE(&l2cdrv) + L2C_CACHE_ID);        /*  获取 ID 寄存器              */
        l2cdrv.L2CD_uiRelease = (uiCacheId & 0x1f);
        l2cdrv.L2CD_uiType    = (uiCacheId >> 6) & 0xf;
        
        uiAux  = read32_le(L2C_BASE(&l2cdrv) + L2C_AUX_CTRL);
        uiAux &= uiAuxMask;
        uiAux |= uiAuxVal;
        
        switch (l2cdrv.L2CD_uiType) {
    
        case 0x01:
            l2cdrv.L2CD_pcName = "PL210";
            uiWays = (uiAux >> 13) & 0xf;
            break;
            
        case 0x02:
            l2cdrv.L2CD_pcName = "PL220";
            uiWays = (uiAux >> 13) & 0xf;
            break;
            
        case 0x03:
            l2cdrv.L2CD_pcName = "PL310";
            if (uiAux & (1 << 16)) {
                uiWays = 16;
            } else {
                uiWays = 8;
            }
            break;
            
        default:
            _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown l2-cache type.\r\n");
            return;
        }
    
        L2C_AUX(&l2cdrv)     = uiAux;
        L2C_WAYMASK(&l2cdrv) = (1 << uiWays) - 1;
        
        uiWaySize = (uiAux & L2C_AUX_CTRL_WAY_SIZE_MASK) >> 17;
        uiWaySize = 1 << (uiWaySize + uiWaySizeShift);
        
        l2cdrv.L2CD_stSize = uiWays * uiWaySize * LW_CFG_KB_SIZE;
        
        _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s L2 cache controller initialization.\r\n",
                     LW_CFG_CPU_ARCH_FAMILY, l2cdrv.L2CD_pcName);
        
        armL2x0Init(&l2cdrv, uiInstruction, uiData, pcMachineName, uiAux);
        
        if ((lib_strcmp(pcMachineName, ARM_MACHINE_A9) == 0) &&
            (l2cdrv.L2CD_uiType == 0x03)) {                             /*  PL310 L2 Controler support  */
            armAuxControlFeatureEnable(AUX_CTRL_A9_L2_PREFETCH);        /*  L2: Prefetch Enable         */
        }
    
    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_A53) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A57) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A72) == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A73) == 0)) {     /*  ARMv8                       */
        l2cdrv.L2CD_pcName    = "ARMv8";
        l2cdrv.L2CD_ulBase    = 0ul;
        l2cdrv.L2CD_uiWayMask = 0;
        l2cdrv.L2CD_uiAux     = 0;
        l2cdrv.L2CD_uiType    = 0;
        l2cdrv.L2CD_uiRelease = 0;

        _DebugFormat(__LOGMESSAGE_LEVEL, "%s L2 cache controller initialization.\r\n",
                     l2cdrv.L2CD_pcName);
    
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
    }
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                                                                        /*  LW_CFG_ARM_CACHE_L2 > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/

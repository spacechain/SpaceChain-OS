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
** 文   件   名: armScu.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2014 年 01 月 03 日
**
** 描        述: ARM SNOOP CONTROL UNIT.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#include "armScu.h"
#include "../../common/cp15/armCp15.h"
/*********************************************************************************************************
  Snoop Control Unit 寄存器偏移
*********************************************************************************************************/
#define SCU_REGS_BASE_OFFSET    0                       /*  Snoop Control Unit 寄存器偏移               */
/*********************************************************************************************************
  Snoop Control Unit 寄存器
*********************************************************************************************************/
typedef struct {
    volatile UINT32     SCU_uiControl;                  /*  SCU Control Register.                       */
    volatile UINT32     SCU_uiConfigure;                /*  SCU Configuration Register.                 */
    volatile UINT32     SCU_uiCpuPowerStatus;           /*  SCU CPU Power Status Register.              */
    volatile UINT32     SCU_uiInvalidateAll;            /*  SCU Invalidate All Registers in Secure State*/
    volatile UINT32     SCU_uiReserves1[12];            /*  Reserves.                                   */
    volatile UINT32     SCU_uiFilteringStart;           /*  Filtering Start Address Register.           */
    volatile UINT32     SCU_uiFilteringEnd;             /*  Filtering End Address Register.             */
    volatile UINT32     SCU_uiReserves2[2];
    volatile UINT32     SCU_uiSAC;                      /*  SCU Access Control (SAC) Register.          */
    volatile UINT32     SCU_uiSNSAC;                    /*  SCU Non-secure Access Control (SNSAC) Reg.  */
} SCU_REGS;
/*********************************************************************************************************
** 函数名称: armScuGet
** 功能描述: 获得 SNOOP CONTROL UNIT
** 输　入  : NONE
** 输　出  : SNOOP CONTROL UNIT 基址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_INLINE SCU_REGS  *armScuGet (VOID)
{
    REGISTER addr_t  ulBase = armPrivatePeriphBaseGet() + SCU_REGS_BASE_OFFSET;

    return  ((SCU_REGS *)ulBase);
}
/*********************************************************************************************************
** 函数名称: armScuFeatureEnable
** 功能描述: SCU 特性使能
** 输　入  : uiFeatures        特性
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armScuFeatureEnable (UINT32  uiFeatures)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    write32(read32((addr_t)&pScu->SCU_uiControl) | uiFeatures, (addr_t)&pScu->SCU_uiControl);
}
/*********************************************************************************************************
** 函数名称: armScuFeatureDisable
** 功能描述: SCU 特性禁能
** 输　入  : uiFeatures        特性
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armScuFeatureDisable (UINT32  uiFeatures)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    write32(read32((addr_t)&pScu->SCU_uiControl) & (~uiFeatures), (addr_t)&pScu->SCU_uiControl);
}
/*********************************************************************************************************
** 函数名称: armScuFeatureGet
** 功能描述: 获得 SCU 特性
** 输　入  : NONE
** 输　出  : SCU 特性
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT32  armScuFeatureGet (VOID)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    return  (read32((addr_t)&pScu->SCU_uiControl));
}
/*********************************************************************************************************
** 函数名称: armScuTagRamSize
** 功能描述: 获得 Tag Ram Size
** 输　入  : NONE
** 输　出  : Tag Ram Size
**
**      Bits [15:14] indicate Cortex-A9 processor CPU3 tag RAM size if present.
**      Bits [13:12] indicate Cortex-A9 processor CPU2 tag RAM size if present.
**      Bits [11:10] indicate Cortex-A9 processor CPU1 tag RAM size if present.
**      Bits [9:8]   indicate Cortex-A9 processor CPU0 tag RAM size.
**      The encoding is as follows:
**      b00   16KB cache,  64 indexes per tag RAM.
**      b01   32KB cache, 128 indexes per tag RAM.
**      b10   64KB cache, 256 indexes per tag RAM.
**      b11   Reserved
**
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT32  armScuTagRamSize (VOID)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    return  ((read32((addr_t)&pScu->SCU_uiConfigure) >> 8) & 0xFF);
}
/*********************************************************************************************************
** 函数名称: armScuCpuMpStatus
** 功能描述: 获得 CPU MP 状态
** 输　入  : NONE
** 输　出  : CPU MP 状态
**
**      0  This Cortex-A9 processor is in AMP mode, not taking part in coherency, or not present.
**      1  This Cortex-A9 processor is in SMP mode, taking part in coherency.
**
**      Bit 3 is for CPU3
**      Bit 2 is for CPU2
**      Bit 1 is for CPU1
**      Bit 0 is for CPU0
**
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT32  armScuCpuMpStatus (VOID)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    return  ((read32((addr_t)&pScu->SCU_uiConfigure) >> 4) & 0xF);
}
/*********************************************************************************************************
** 函数名称: armScuCpuNumber
** 功能描述: 获得 CPU 数目
** 输　入  : NONE
** 输　出  : CPU 数目
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT32  armScuCpuNumber (VOID)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    return  (((read32((addr_t)&pScu->SCU_uiConfigure) >> 0) & 0x3) + 1);
}
/*********************************************************************************************************
** 函数名称: armScuSecureInvalidateAll
** 功能描述: Invalidates the SCU tag RAMs on a per Cortex-A9 processor and per way basis
** 输　入  : uiCPUId       CPU ID
**           uiWays        Specifies the ways that must be invalidated for CPU(ID)
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armScuSecureInvalidateAll (UINT32  uiCPUId,  UINT32  uiWays)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    switch (uiCPUId) {

    case 0:
    case 1:
    case 2:
    case 3:
        uiWays &= 0xF;
        write32(uiWays << (uiCPUId * 4), (addr_t)&pScu->SCU_uiInvalidateAll);
        break;

    default:
        break;
    }
}
/*********************************************************************************************************
** 函数名称: armScuFilteringSet
** 功能描述: 设置 Filtering Start and End Address
** 输　入  : uiStart       Start Address
**           uiEnd         End Address
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armScuFilteringSet (UINT32  uiStart,  UINT32  uiEnd)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    write32(uiStart, (addr_t)&pScu->SCU_uiFilteringStart);
    write32(uiEnd,   (addr_t)&pScu->SCU_uiFilteringEnd);
}
/*********************************************************************************************************
** 函数名称: armScuAccessCtrlSet
** 功能描述: 设置 SCU 访问控制
** 输　入  : uiCpuBits         CPU 位集
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armScuAccessCtrlSet (UINT32  uiCpuBits)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    write32(uiCpuBits, (addr_t)&pScu->SCU_uiSAC);
}
/*********************************************************************************************************
** 函数名称: armScuNonAccessCtrlSet
** 功能描述: 设置 SCU 非安全模式访问控制
** 输　入  : uiValue           值
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armScuNonAccessCtrlSet (UINT32  uiValue)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    write32(uiValue, (addr_t)&pScu->SCU_uiSNSAC);
}
/*********************************************************************************************************
** 函数名称: armScuCpuPowerStatusGet
** 功能描述: 获得 CPU 电源状态
** 输　入  : NONE
** 输　出  : CPU 电源状态
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT32  armScuCpuPowerStatusGet (VOID)
{
    REGISTER SCU_REGS  *pScu = armScuGet();

    return  (read32((addr_t)&pScu->SCU_uiCpuPowerStatus));
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/

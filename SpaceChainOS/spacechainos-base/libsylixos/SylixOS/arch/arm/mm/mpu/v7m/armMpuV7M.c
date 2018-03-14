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
** 文   件   名: armMpuV7M.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 11 月 08 日
**
** 描        述: ARM 体系构架 MPU 驱动.
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
#if (LW_CFG_VMM_EN == 0) && (LW_CFG_ARM_MPU > 0)
/*********************************************************************************************************
  最大内存分区数
*********************************************************************************************************/
#define ARM_MPU_MAX_REGIONS     8                               /*  总 REGION 数                        */
/*********************************************************************************************************
  寄存器地址
*********************************************************************************************************/
#define ARM_MPU_TYPE        (*(volatile UINT32 *)0xe000ed90)    /*  MPU Type Register                   */
#define ARM_MPU_CTRL        (*(volatile UINT32 *)0xe000ed94)    /*  MPU Control Register                */
#define ARM_MPU_RNR         (*(volatile UINT32 *)0xe000ed98)    /*  MPU Region Number Register          */
#define ARM_MPU_RBAR        (*(volatile UINT32 *)0xe000ed9c)    /*  MPU Region Base Address Register    */
#define ARM_MPU_RASR        (*(volatile UINT32 *)0xe000eda0)    /*  MPU Region Attr and Size Register   */
#define ARM_MPU_RBAR_A1     (*(volatile UINT32 *)0xe000eda4)    /*  MPU alias registers                 */
#define ARM_MPU_RASR_A1     (*(volatile UINT32 *)0xe000eda8)
#define ARM_MPU_RBAR_A2     (*(volatile UINT32 *)0xe000edac)
#define ARM_MPU_RASR_A2     (*(volatile UINT32 *)0xe000edb0)
#define ARM_MPU_RBAR_A3     (*(volatile UINT32 *)0xe000edb4)
#define ARM_MPU_RASR_A3     (*(volatile UINT32 *)0xe000edb8)
/*********************************************************************************************************
  CTRL 寄存器定义
*********************************************************************************************************/
#define ARM_MPU_CTRL_OFT_PRIVDEFENA     2
#define ARM_MPU_CTRL_OFT_HFNMIENA       1
#define ARM_MPU_CTRL_OFT_ENABLE         0
/*********************************************************************************************************
  RASR 寄存器定义
*********************************************************************************************************/
#define ARM_MPU_RASR_OFT_XN             28
#define ARM_MPU_RASR_OFT_AP             24
#define ARM_MPU_RASR_OFT_TEX            19
#define ARM_MPU_RASR_OFT_S              18
#define ARM_MPU_RASR_OFT_C              17
#define ARM_MPU_RASR_OFT_B              16
#define ARM_MPU_RASR_OFT_SRD            8
#define ARM_MPU_RASR_OFT_SIZE           1
#define ARM_MPU_RASR_OFT_EN             0
/*********************************************************************************************************
  DMA Pool
*********************************************************************************************************/
static LW_CLASS_HEAP    _G_heapDmaPool;
/*********************************************************************************************************
** 函数名称: armMpuV7MInit
** 功能描述: 初始化 MPU
** 输　入  : pcMachineName  机器名称
**           mpuregion      内存属性与布局表
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armMpuV7MInit (CPCHAR  pcMachineName, const ARM_MPU_REGION  mpuregion[])
{
    PARM_MPU_REGION   pmpuregion;
    UINT32            uiVal;
    UINT32            uiAP;
    size_t            stSize;

    if (mpuregion == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "MPU mpuregion[] invalid.\r\n");
        return;
    }

    ARM_MPU_CTRL &= ~1;

    for (pmpuregion = (PARM_MPU_REGION)mpuregion;
         (pmpuregion->MPUD_ucSize  != ARM_MPU_REGION_SIZE_END) &&
         (pmpuregion->MPUD_ucNumber < ARM_MPU_MAX_REGIONS);
         pmpuregion++) {

        ARM_MPU_RNR = (UINT32)pmpuregion->MPUD_ucNumber;

        if (pmpuregion->MPUD_bEnable) {
            ARM_MPU_RBAR = pmpuregion->MPUD_uiAddr;

            uiVal = 0;
            uiAP  = pmpuregion->MPUD_uiAP;

            if (uiAP & ARM_MPU_AP_FLAG_EXEC) {
                uiAP &= ~ARM_MPU_AP_FLAG_EXEC;

            } else {
                uiVal |= (1 << ARM_MPU_RASR_OFT_XN);
            }

            uiVal |= (uiAP << ARM_MPU_RASR_OFT_AP);

            switch (pmpuregion->MPUD_uiCPS) {

            case ARM_MPU_CPS_STRONG_ORDER_S:
                uiVal |= (1 << ARM_MPU_RASR_OFT_S);
                break;

            case ARM_MPU_CPS_DEVICE:
                uiVal |= (2 << ARM_MPU_RASR_OFT_TEX);
                break;

            case ARM_MPU_CPS_DEVICE_S:
                uiVal |= (1 << ARM_MPU_RASR_OFT_S);
                uiVal |= (1 << ARM_MPU_RASR_OFT_B);
                break;

            case ARM_MPU_CPS_NORMAL_WT:
                uiVal |= (1 << ARM_MPU_RASR_OFT_C);
                break;

            case ARM_MPU_CPS_NORMAL_WT_S:
                uiVal |= (1 << ARM_MPU_RASR_OFT_S);
                uiVal |= (1 << ARM_MPU_RASR_OFT_C);
                break;

            case ARM_MPU_CPS_NORMAL_WB:
                uiVal |= (1 << ARM_MPU_RASR_OFT_C);
                uiVal |= (1 << ARM_MPU_RASR_OFT_B);
                break;

            case ARM_MPU_CPS_NORMAL_WB_S:
                uiVal |= (1 << ARM_MPU_RASR_OFT_S);
                uiVal |= (1 << ARM_MPU_RASR_OFT_C);
                uiVal |= (1 << ARM_MPU_RASR_OFT_B);
                break;

            case ARM_MPU_CPS_NORMAL_NC:
                uiVal |= (1 << ARM_MPU_RASR_OFT_TEX);
                break;

            case ARM_MPU_CPS_NORMAL_NC_S:
                uiVal |= (1 << ARM_MPU_RASR_OFT_TEX);
                uiVal |= (1 << ARM_MPU_RASR_OFT_S);
                break;

            case ARM_MPU_CPS_NORMAL_WBA:
                uiVal |= (1 << ARM_MPU_RASR_OFT_TEX);
                uiVal |= (1 << ARM_MPU_RASR_OFT_C);
                uiVal |= (1 << ARM_MPU_RASR_OFT_B);
                break;

            case ARM_MPU_CPS_NORMAL_WBA_S:
                uiVal |= (1 << ARM_MPU_RASR_OFT_TEX);
                uiVal |= (1 << ARM_MPU_RASR_OFT_S);
                uiVal |= (1 << ARM_MPU_RASR_OFT_C);
                uiVal |= (1 << ARM_MPU_RASR_OFT_B);
                break;

            default:
                _DebugHandle(__ERRORMESSAGE_LEVEL, "MPU CPS invalid.\r\n");
                return;
            }

            uiVal |= (pmpuregion->MPUD_ucSize << ARM_MPU_RASR_OFT_SIZE);

            ARM_MPU_RASR = uiVal | (1 << ARM_MPU_RASR_OFT_EN);

            if (pmpuregion->MPUD_bDmaPool &&
                (pmpuregion->MPUD_ucSize >= ARM_MPU_REGION_SIZE_512B)) {
                stSize = ((size_t)2 << pmpuregion->MPUD_ucSize);
                if (!_G_heapDmaPool.HEAP_stTotalByteSize) {
                    _HeapCtor(&_G_heapDmaPool,
                              (PVOID)pmpuregion->MPUD_uiAddr, stSize);

                } else {
                    _HeapAddMemory(&_G_heapDmaPool, (PVOID)pmpuregion->MPUD_uiAddr, stSize);
                }
            }

        } else {
            ARM_MPU_RBAR = 0;
            ARM_MPU_RASR = 0;
        }
    }

    ARM_MPU_CTRL |= 1;
}
/*********************************************************************************************************
** 函数名称: armMpuV7MDmaAlloc
** 功能描述: 从 DMA 内存池中分配内存
** 输　入  : stSize     需要的内存大小
** 输　出  : 内存首地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PVOID  armMpuV7MDmaAlloc (size_t  stSize)
{
    if (_G_heapDmaPool.HEAP_stTotalByteSize) {
        return  (_HeapAllocate(&_G_heapDmaPool, stSize, __func__));

    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: armMpuV7MDmaAllocAlign
** 功能描述: 从 DMA 内存池中分配内存
** 输　入  : stSize     需要的内存大小
**           stAlign    对齐要求
** 输　出  : 内存首地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PVOID  armMpuV7MDmaAllocAlign (size_t  stSize, size_t  stAlign)
{
    if (_G_heapDmaPool.HEAP_stTotalByteSize) {
        return  (_HeapAllocateAlign(&_G_heapDmaPool, stSize, stAlign, __func__));

    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: armMpuV7MDmaFree
** 功能描述: 释放从 DMA 内存池中分配的内存
** 输　入  : pvDmaMem   内存首地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armMpuV7MDmaFree (PVOID  pvDmaMem)
{
    if (_G_heapDmaPool.HEAP_stTotalByteSize) {
        _HeapFree(&_G_heapDmaPool, pvDmaMem, LW_FALSE, __func__);
    }
}

#endif                                                                  /*  LW_CFG_ARM_MPU > 0          */
#endif                                                                  /*  __SYLIXOS_ARM_ARCH_M__      */
/*********************************************************************************************************
  END
*********************************************************************************************************/

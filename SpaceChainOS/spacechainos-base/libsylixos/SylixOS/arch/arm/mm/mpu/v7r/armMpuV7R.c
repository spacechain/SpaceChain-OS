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
** 文   件   名: armMpuV7R.c
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
  ARMv7R 体系构架
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_R__)
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if (LW_CFG_VMM_EN == 0) && (LW_CFG_ARM_MPU > 0)
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
extern VOID     armMpuV7REnable(VOID);
extern VOID     armMpuV7RDisable(VOID);
extern VOID     armMpuV7RBgEnable(VOID);
extern VOID     armMpuV7RBgDisable(VOID);
extern UINT8    armMpuV7RGetRegionNum(VOID);
extern UINT8    armMpuV7RGetRegionNumIns(VOID);
extern UINT8    armMpuV7RAreRegionsSeparate(VOID);
extern VOID     armMpuV7RSelectRegion(UINT8);
extern VOID     armMpuV7RSetRegionBase(UINT32);
extern VOID     armMpuV7RSetRegionAP(UINT32);
extern VOID     armMpuV7RSetRegionSize(UINT32);
extern VOID     armMpuV7RSetRegionBaseIns(UINT32);
extern VOID     armMpuV7RSetRegionAPIns(UINT32);
extern VOID     armMpuV7RSetRegionSizeIns(UINT32);
/*********************************************************************************************************
  DMA Pool
*********************************************************************************************************/
static LW_CLASS_HEAP    _G_heapDmaPool;
/*********************************************************************************************************
** 函数名称: armMpuV7RGetAPDesc
** 功能描述: 转换 AP 权限描述符
** 输　入  : pmpuregion     内存属性与布局表
** 输　出  : AP 权限描述符
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT32  armMpuV7RGetAPDesc (const PARM_MPU_REGION   pmpuregion)
{
    UINT32  uiVal = 0;
    UINT32  uiAP  = pmpuregion->MPUD_uiAP;

    if (uiAP & ARM_MPU_AP_FLAG_EXEC) {
        uiAP &= ~ARM_MPU_AP_FLAG_EXEC;

    } else {
        uiVal |= 0x1000;
    }

    uiVal |= (uiAP << 8);

    switch (pmpuregion->MPUD_uiCPS) {

    case ARM_MPU_CPS_STRONG_ORDER_S:
        break;

    case ARM_MPU_CPS_DEVICE:
        uiVal |= 0x0010;
        break;

    case ARM_MPU_CPS_DEVICE_S:
        uiVal |= 0x0001;
        break;

    case ARM_MPU_CPS_NORMAL_WT:
        uiVal |= 0x0002;
        break;

    case ARM_MPU_CPS_NORMAL_WT_S:
        uiVal |= 0x0006;
        break;

    case ARM_MPU_CPS_NORMAL_WB:
        uiVal |= 0x0003;
        break;

    case ARM_MPU_CPS_NORMAL_WB_S:
        uiVal |= 0x0007;
        break;

    case ARM_MPU_CPS_NORMAL_NC:
        uiVal |= 0x0008;
        break;

    case ARM_MPU_CPS_NORMAL_NC_S:
        uiVal |= 0x000c;
        break;

    case ARM_MPU_CPS_NORMAL_WBA:
        uiVal |= 0x000b;
        break;

    case ARM_MPU_CPS_NORMAL_WBA_S:
        uiVal |= 0x000f;
        break;

    default:
        _DebugHandle(__ERRORMESSAGE_LEVEL, "MPU CPS invalid.\r\n");
        return  (0);
    }

    return  (uiVal);
}
/*********************************************************************************************************
** 函数名称: armMpuV7RInit
** 功能描述: 初始化 MPU
** 输　入  : pcMachineName  机器名称
**           mpuregion      内存属性与布局表
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armMpuV7RInit (CPCHAR  pcMachineName, const ARM_MPU_REGION  mpuregion[])
{
    UINT8             ucSeparate;
    UINT8             ucMaxRegions;
    UINT8             ucMaxRegionsIns;

    PARM_MPU_REGION   pmpuregion;
    UINT32            uiVal;
    size_t            stSize;

    if (mpuregion == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "MPU mpuregion[] invalid.\r\n");
        return;
    }

    armMpuV7RDisable();
    armMpuV7RBgDisable();

    ucMaxRegions = armMpuV7RGetRegionNum();
    ucSeparate   = armMpuV7RAreRegionsSeparate() & 1;
    if (ucSeparate) {
        ucMaxRegionsIns = armMpuV7RGetRegionNumIns();
    } else {
        ucMaxRegionsIns = 0;
    }

    pmpuregion = (PARM_MPU_REGION)mpuregion;
    while (pmpuregion->MPUD_ucSize != ARM_MPU_REGION_SIZE_END) {
        if (ucSeparate) {
            if (pmpuregion->MPUD_ucIsIns) {
                if (pmpuregion->MPUD_ucNumber >= ucMaxRegionsIns) {
                    _DebugHandle(__ERRORMESSAGE_LEVEL, "MPU region invalid.\r\n");
                    break;
                }

                armMpuV7RSelectRegion(pmpuregion->MPUD_ucNumber);
                armMpuV7RSetRegionBaseIns(pmpuregion->MPUD_uiAddr);

                uiVal = (pmpuregion->MPUD_ucSize << 1) | ((pmpuregion->MPUD_bEnable) ? 1 : 0);
                armMpuV7RSetRegionSizeIns(uiVal);

                uiVal = armMpuV7RGetAPDesc(pmpuregion);
                armMpuV7RSetRegionAPIns(uiVal);

            } else {
                if (pmpuregion->MPUD_ucNumber >= ucMaxRegions) {
                    _DebugHandle(__ERRORMESSAGE_LEVEL, "MPU region invalid.\r\n");
                    break;
                }

                armMpuV7RSelectRegion(pmpuregion->MPUD_ucNumber);
                armMpuV7RSetRegionBase(pmpuregion->MPUD_uiAddr);

                uiVal = (pmpuregion->MPUD_ucSize << 1) | ((pmpuregion->MPUD_bEnable) ? 1 : 0);
                armMpuV7RSetRegionSize(uiVal);

                uiVal = armMpuV7RGetAPDesc(pmpuregion);
                armMpuV7RSetRegionAP(uiVal);
            }

        } else {
            if (pmpuregion->MPUD_ucNumber >= ucMaxRegions) {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "MPU region invalid.\r\n");
                break;
            }

            armMpuV7RSelectRegion(pmpuregion->MPUD_ucNumber);
            armMpuV7RSetRegionBase(pmpuregion->MPUD_uiAddr);

            uiVal = (pmpuregion->MPUD_ucSize << 1) | ((pmpuregion->MPUD_bEnable) ? 1 : 0);
            armMpuV7RSetRegionSize(uiVal);

            uiVal = armMpuV7RGetAPDesc(pmpuregion);
            armMpuV7RSetRegionAP(uiVal);
        }

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

        pmpuregion++;
    }

    armMpuV7REnable();
}
/*********************************************************************************************************
** 函数名称: armMpuV7RDmaAlloc
** 功能描述: 从 DMA 内存池中分配内存
** 输　入  : stSize     需要的内存大小
** 输　出  : 内存首地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PVOID  armMpuV7RDmaAlloc (size_t  stSize)
{
    if (_G_heapDmaPool.HEAP_stTotalByteSize) {
        return  (_HeapAllocate(&_G_heapDmaPool, stSize, __func__));

    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: armMpuV7RDmaAllocAlign
** 功能描述: 从 DMA 内存池中分配内存
** 输　入  : stSize     需要的内存大小
**           stAlign    对齐要求
** 输　出  : 内存首地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PVOID  armMpuV7RDmaAllocAlign (size_t  stSize, size_t  stAlign)
{
    if (_G_heapDmaPool.HEAP_stTotalByteSize) {
        return  (_HeapAllocateAlign(&_G_heapDmaPool, stSize, stAlign, __func__));

    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: armMpuV7RDmaFree
** 功能描述: 释放从 DMA 内存池中分配的内存
** 输　入  : pvDmaMem   内存首地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  armMpuV7RDmaFree (PVOID  pvDmaMem)
{
    if (_G_heapDmaPool.HEAP_stTotalByteSize) {
        _HeapFree(&_G_heapDmaPool, pvDmaMem, LW_FALSE, __func__);
    }
}

#endif                                                                  /*  LW_CFG_ARM_MPU > 0          */
#endif                                                                  /*  __SYLIXOS_ARM_ARCH_R__      */
/*********************************************************************************************************
  END
*********************************************************************************************************/

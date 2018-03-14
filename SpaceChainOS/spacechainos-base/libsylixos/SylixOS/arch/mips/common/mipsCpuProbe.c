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
** 文   件   名: mipsCpuProbe.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 07 月 18 日
**
** 描        述: MIPS 体系构架 CPU 探测.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/mips/common/cp0/mipsCp0.h"
#include "arch/mips/common/mipsCpuProbe.h"
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
UINT    _G_uiMipsProcessorId = 0;                                       /*  处理器 ID                   */
UINT    _G_uiMipsCpuType     = PRID_IMP_UNKNOWN;                        /*  CPU 类型                    */
/*********************************************************************************************************
** 函数名称: mispCpuProbeLegacy
** 功能描述: 探测 Legacy CPU
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  mispCpuProbeLegacy (VOID)
{
    switch (_G_uiMipsProcessorId & PRID_IMP_MASK) {

    case PRID_IMP_LOONGSON_64:                                          /*  Loongson-2/3                */
        switch (_G_uiMipsProcessorId & PRID_REV_MASK) {

        case PRID_REV_LOONGSON2E:
        case PRID_REV_LOONGSON2F:
            _G_uiMipsCpuType = CPU_LOONGSON2;
            break;

        case PRID_REV_LOONGSON3A_R1:
        case PRID_REV_LOONGSON3A_R2:
        case PRID_REV_LOONGSON3A_R3:
        case PRID_REV_LOONGSON3B_R1:
        case PRID_REV_LOONGSON3B_R2:
            _G_uiMipsCpuType = CPU_LOONGSON3;
            break;
        }
        break;

    case PRID_IMP_LOONGSON_32:                                          /*  Loongson-1                  */
        _G_uiMipsCpuType = CPU_LOONGSON1;
        break;

    default:
        break;
    }
}
/*********************************************************************************************************
** 函数名称: mispCpuProbeIngenic
** 功能描述: 探测君正 CPU
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  mispCpuProbeIngenic (VOID)
{
    switch (_G_uiMipsProcessorId & PRID_IMP_MASK) {

    case PRID_IMP_JZRISC:
        _G_uiMipsCpuType = CPU_JZRISC;
        break;

    default:
        break;
    }
}
/*********************************************************************************************************
** 函数名称: mipsCpuProbe
** 功能描述: mips CPU 探测
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  mipsCpuProbe (VOID)
{
    _G_uiMipsProcessorId = mipsCp0PRIdRead();
    switch (_G_uiMipsProcessorId & PRID_COMP_MASK) {

    case PRID_COMP_LEGACY:
        mispCpuProbeLegacy();
        break;

    case PRID_COMP_INGENIC:
    case PRID_COMP_INGENIC_4780:
        mispCpuProbeIngenic();
        break;
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

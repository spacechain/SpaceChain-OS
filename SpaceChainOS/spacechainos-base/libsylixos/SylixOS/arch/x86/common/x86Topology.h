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
** 文   件   名: x86Topology.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 04 月 13 日
**
** 描        述: x86 体系构架处理器 CPU 拓扑.
*********************************************************************************************************/

#ifndef __X86_TOPOLOGY_H
#define __X86_TOPOLOGY_H

/*********************************************************************************************************
  头文件
*********************************************************************************************************/
#include "arch/x86/common/x86CpuId.h"
#include "arch/x86/apic/x86IoApic.h"
#include "arch/x86/apic/x86LocalApic.h"
/*********************************************************************************************************
  CPU 集类型及其相关的宏
*********************************************************************************************************/
typedef UINT    X86_CPUSET_T;

#define X86_CPUSET_SET(cpuset, n)       ((cpuset) |= (1 << (n)))
#define X86_CPUSET_ISSET(cpuset, n)     ((cpuset) & (1 << (n)))
#define X86_CPUSET_CLR(cpuset, n)       ((cpuset) &= ~(1 << (n)))
#define X86_CPUSET_ZERO(cpuset)         ((cpuset) = 0)
#define X86_CPUSET_ISZERO(cpuset)       ((cpuset) == 0)
/*********************************************************************************************************
  CPU 拓扑结构
*********************************************************************************************************/
typedef struct {
    UINT            TOP_uiMaxLProcsCore;                    /*  Maximum Logical Processors per Core     */
    UINT            TOP_uiMaxCoresPkg;                      /*  Maximum Number of Cores per Package     */
    UINT            TOP_uiMaxLProcsPkg;                     /*  Maximum Logical Processors per Package  */

    UINT            TOP_uiSmtMaskWidth;                     /*  SMT Bit Width                           */
    UINT            TOP_uiCoreMaskWidth;                    /*  Core Bit Width                          */
    UINT            TOP_uiPkgMaskWidth;                     /*  Package Bit Width                       */

    UINT            TOP_uiNumCores;                         /*  Number of Cores                         */
    UINT            TOP_uiNumPkgs;                          /*  Number of Packages                      */
    UINT            TOP_uiNumLProcsEnabled;                 /*  Number of Logical Processors Enabled    */

    UCHAR           TOP_uiPkgIdMask;                        /*  Package ID Mask                         */

    UCHAR           TOP_aucSmtIds[X86_CPUID_MAX_NUM_CPUS];  /*  SMT Ids                                 */
    UCHAR           TOP_aucCoreIds[X86_CPUID_MAX_NUM_CPUS]; /*  Core Ids                                */
    UCHAR           TOP_aucPkgIds[X86_CPUID_MAX_NUM_CPUS];  /*  Package Ids                             */

    UINT            TOP_auiPhysIdx[X86_CPUID_MAX_NUM_CPUS]; /*  CPU ID table                            */

    X86_CPUSET_T    TOP_smtSet[X86_CPUID_MAX_NUM_CPUS];     /*  SMT CPU set                             */

    UINT8           TOP_ucCpuBSP;                           /*  BSP Local APIC ID                       */
} X86_CPU_TOPOLOGY;
/*********************************************************************************************************
  全局变量声明
*********************************************************************************************************/
extern X86_CPU_TOPOLOGY _G_x86CpuTopology;                  /*  CPU 拓扑                                */
                                                            /*  Local APIC ID->逻辑处理器ID             */
extern UINT8            _G_aucX86CpuIndexTable[X86_CPUID_MAX_NUM_CPUS];   
extern UINT             _G_uiX86BaseCpuPhysIndex;           /*  Base CPU Phy index                      */
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
/*
 * 逻辑 Processor 数目
 */
#define X86_LPROC_NR                    (_G_x86CpuTopology.TOP_uiNumLProcsEnabled)

/*
 * 物理 Processor 数目
 */
#define X86_PPROC_NR                    (_G_x86CpuTopology.TOP_uiNumCores)

/*
 * 判断是否有 SMT 技术
 */
#define X86_HAS_SMT                     (X86_LPROC_NR != X86_PPROC_NR)

/*
 * APICID -> LOGICID(0 ~ LW_NCPUS-1)
 */
#define X86_APICID_TO_LOGICID(apicid)   (_G_aucX86CpuIndexTable[(apicid)] - _G_uiX86BaseCpuPhysIndex)

/*
 * LOGICID(0 ~ LW_NCPUS-1) -> APICID
 */
#define X86_LOGICID_TO_APICID(logicid)  (_G_x86CpuTopology.TOP_auiPhysIdx[(logicid) + _G_uiX86BaseCpuPhysIndex])

/*
 * LOGICID -> PHYID
 */
#define X86_LOGICID_TO_PHYID(logicid)   (X86_HAS_SMT ? \
                                        ((logicid) >> _G_x86CpuTopology.TOP_uiSmtMaskWidth) : (logicid))

/*
 * 逻辑 Processor 是否存在
 */
#define X86_APICID_PRESEND(apicid)      ((apicid) == _G_x86CpuTopology.TOP_ucCpuBSP ? \
                                        LW_TRUE : (BOOL)X86_APICID_TO_LOGICID(apicid))

/*
 * 逻辑 Processor 是否为 HT 核
 */
#define X86_APICID_IS_HT(apicid)        (X86_HAS_SMT && ((apicid) & _G_x86CpuTopology.TOP_uiSmtMaskWidth))

/*
 * 获得 BSP 的 APICID
 */
#define X86_BSP_APICID                  (_G_x86CpuTopology.TOP_ucCpuBSP)
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
static LW_INLINE UINT  x86CpuPhysIndexGet (VOID)
{
    return  (_G_aucX86CpuIndexTable[x86LocalApicId()]);
}

#endif                                                                  /*  __X86_TOPOLOGY_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/

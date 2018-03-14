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
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#define  __KERNEL_NCPUS_SET
#include "SylixOS.h"
#include "arch/x86/mpconfig/x86MpApic.h"
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define BITS_PER_BYTE           8                                       /*  1 字节 8 位                 */
#define BITS_PER_APIC_ID        BITS_PER_BYTE                           /*  1 字节表示 APIC ID          */
/*********************************************************************************************************
  位图相关宏定义
*********************************************************************************************************/
#define BMP_WORD_BIT_SIZE       (32)

#define BMP_BIT_GET(bitNo, addr)                                            \
                    ((*(UINT32 *)(((UINT32 *)(addr)) + ((bitNo) >> 5))) &   \
                    (0x80000000 >> ((bitNo) & (BMP_WORD_BIT_SIZE - 1))))

#define BMP_BIT_SET(bitNo, addr)                                            \
                    (*(UINT32 *)(((UINT32 *)(addr)) + ((bitNo) >> 5))) |=   \
                    (0x80000000 >> ((bitNo) & (BMP_WORD_BIT_SIZE - 1)))

#define BMP_BIT_CLEAR(bitNo, addr)                                          \
                    (*(UINT32 *)(((UINT32 *)(addr)) + ((bitNo) >> 5))) &=   \
                    ~(0x80000000 >> ((bitNo) & (BMP_WORD_BIT_SIZE - 1)))

#define BMP_BIT_TOGGLE(bitNo, addr)                                         \
                    (*(UINT32 *)(((UINT32 *)(addr)) + ((bitNo) >> 5))) ^=   \
                    (0x80000000 >> ((bitNo) & (BMP_WORD_BIT_SIZE - 1)))
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
X86_CPU_TOPOLOGY        _G_x86CpuTopology;                              /*  CPU 拓扑                    */
/*********************************************************************************************************
** 函数名称: x86CpuTopologyInit
** 功能描述: 用初始化过程产生的数据(X86_MP_APIC_DATA 结构)构建 CPU 拓扑结构
** 输　入  : pMpApicData       X86_MP_APIC_DATA 结构
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  x86CpuTopologyInit (X86_MP_APIC_DATA  *pMpApicData)
{
    X86_CPU_TOPOLOGY  *pCpuTopology = &_G_x86CpuTopology;
#if LW_CFG_SMP_EN > 0
    INT       i, iApicIdToCpuIndex, iSmtGroupIx;
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    INT       iIndex;
    INTREG    iregInterLevel;
    UINT32    auiPkgBmp[X86_CPUID_MAX_NUM_CPUS  / (BITS_PER_BYTE * sizeof(UINT32))];
    UINT32    auiCoreBmp[X86_CPUID_MAX_NUM_CPUS / (BITS_PER_BYTE * sizeof(UINT32))];
    UINT8     aucMpCpuIndexTable[X86_CPUID_MAX_NUM_CPUS];
    UINT8    *pucMpLoApicIndexTable;
    INT       iNumCpus;
#if LW_CFG_SMP_EN == 0
    UINT8     ucApicId;
#endif                                                                  /*  LW_CFG_SMP_EN == 0          */

#if LW_CFG_SMP_EN > 0
    iNumCpus = _G_uiX86CpuCount;
    pucMpLoApicIndexTable = (UINT8 *)X86_MPAPIC_PHYS_TO_VIRT(pMpApicData, MPAPIC_uiLaLoc);
#else
    iNumCpus = 1;
    ucApicId = x86CpuIdInitialApicId();
    pucMpLoApicIndexTable = &ucApicId;
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

    if (iNumCpus > LW_CFG_MAX_PROCESSORS) {
        _K_ulNCpus = LW_CFG_MAX_PROCESSORS;
    
    } else {
        _K_ulNCpus = iNumCpus;                                          /*  设置逻辑 Processor 数目     */
    }

    /*
     * Map the MP Table CPU Topology found
     */
    lib_bzero((VOID *)pCpuTopology, sizeof(X86_CPU_TOPOLOGY));

    pCpuTopology->TOP_ucCpuBSP        = pMpApicData->MPAPIC_ucCpuBSP;

    pCpuTopology->TOP_uiMaxLProcsPkg  = x86CpuIdMaxNumLProcsPerPkg();
    pCpuTopology->TOP_uiMaxCoresPkg   = x86CpuIdMaxNumCoresPerPkg();
    pCpuTopology->TOP_uiMaxLProcsCore = x86CpuIdMaxNumLProcsPerCore();

    pCpuTopology->TOP_uiCoreMaskWidth = x86CpuIdBitFieldWidth(pCpuTopology->TOP_uiMaxCoresPkg);
    pCpuTopology->TOP_uiSmtMaskWidth  = x86CpuIdBitFieldWidth(pCpuTopology->TOP_uiMaxLProcsCore);

    /*
     * APIC ID is made up by Package ID, Core ID and SMT ID,
     * the rest bits besides those used for Core ID and SMT ID are
     * used to represent Package ID.
     */
    pCpuTopology->TOP_uiPkgMaskWidth = BITS_PER_APIC_ID -
             (pCpuTopology->TOP_uiCoreMaskWidth + pCpuTopology->TOP_uiSmtMaskWidth);

    lib_bzero((VOID *)aucMpCpuIndexTable, sizeof(aucMpCpuIndexTable));
    lib_bzero((VOID *)auiPkgBmp,          sizeof(auiPkgBmp));
    lib_bzero((VOID *)auiCoreBmp,         sizeof(auiCoreBmp));

    for (iIndex = 0; iIndex < iNumCpus; iIndex++) {
        iregInterLevel = KN_INT_DISABLE();

        /*
         * Store SMT ID & Core ID of each logical processor
         *
         * Shift value for SMT ID is 0
         * Shift value for Core ID is the mask width for maximum logical processors per core
         */
        pCpuTopology->TOP_aucSmtIds[iIndex]  = x86CpuIdBitField((UCHAR)pucMpLoApicIndexTable[iIndex],
                                                                (UCHAR)pCpuTopology->TOP_uiMaxLProcsCore,
                                                                (UCHAR)0);
        pCpuTopology->TOP_aucCoreIds[iIndex] = x86CpuIdBitField(
                                                (UCHAR)pucMpLoApicIndexTable[iIndex],
                                                (UCHAR)pCpuTopology->TOP_uiMaxCoresPkg,
                                                (UCHAR)x86CpuIdBitFieldWidth(pCpuTopology->TOP_uiMaxLProcsCore));

        /*
         * Extract Package ID, assumes single cluster
         * Shift value is mask width for maximum logical processors per package
         */
        pCpuTopology->TOP_uiPkgIdMask        = ((UCHAR)(0xff <<
                                               x86CpuIdBitFieldWidth(pCpuTopology->TOP_uiMaxLProcsPkg)));
        pCpuTopology->TOP_aucPkgIds[iIndex]  = pucMpLoApicIndexTable[iIndex] & pCpuTopology->TOP_uiPkgIdMask;
        pCpuTopology->TOP_auiPhysIdx[iIndex] = pucMpLoApicIndexTable[iIndex];
        aucMpCpuIndexTable[pucMpLoApicIndexTable[iIndex]] = (UINT8)iIndex;

        pCpuTopology->TOP_uiNumLProcsEnabled++;

        /*
         * Use bitmap to mark if the corresponding packageId has been counted
         */
        if (BMP_BIT_GET(pCpuTopology->TOP_aucPkgIds[iIndex], auiPkgBmp) == 0) {
            BMP_BIT_SET(pCpuTopology->TOP_aucPkgIds[iIndex], auiPkgBmp);
            pCpuTopology->TOP_uiNumPkgs++;
        }

        /*
         * Use bitmap to mark if the corresponding core has been counted
         */
        if (BMP_BIT_GET(pCpuTopology->TOP_aucPkgIds[iIndex] |
                        pCpuTopology->TOP_aucCoreIds[iIndex], auiCoreBmp) == 0) {
            BMP_BIT_SET(pCpuTopology->TOP_aucPkgIds[iIndex] |
                        pCpuTopology->TOP_aucCoreIds[iIndex], auiCoreBmp);
            pCpuTopology->TOP_uiNumCores++;
        }

        KN_INT_ENABLE(iregInterLevel);
    }

#if LW_CFG_SMP_EN > 0
    if (iNumCpus > 1) {
        /*
         * HT/SMT:  Has physcal/logical pair
         */
        if (pCpuTopology->TOP_aucSmtIds[1] != 0) {
            for (iIndex = 0; iIndex < iNumCpus; iIndex++) {
                iSmtGroupIx = pCpuTopology->TOP_aucPkgIds[iIndex] | pCpuTopology->TOP_aucCoreIds[iIndex];

                for (i = 0; i < pCpuTopology->TOP_uiMaxLProcsCore; i++) {
                    iApicIdToCpuIndex = aucMpCpuIndexTable[iSmtGroupIx + i];

                    /*
                     * Find the cpu index of the APIC ID, if it equals to 0,
                     * and the corresponding apic ID is not the fist entry of
                     * the APIC ID array, then the APIC ID is not contained in
                     * the APIC ID table.
                     */
                    if ((iApicIdToCpuIndex == 0) && ((iSmtGroupIx + i) != pucMpLoApicIndexTable[0])) {
                        continue;
                    }

                    X86_CPUSET_SET(pCpuTopology->TOP_smtSet[iIndex], iApicIdToCpuIndex);
                }
            }
        } else {
            /*
             * Without HT/SMT: We have NO physical/logical pair
             */
            for (iIndex = 0; iIndex < iNumCpus; iIndex++) {
                X86_CPUSET_SET(pCpuTopology->TOP_smtSet[iIndex], iIndex);
            }
        }
    } else {
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
        X86_CPUSET_SET(pCpuTopology->TOP_smtSet[0], 0);
#if LW_CFG_SMP_EN > 0
    }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: x86CpuTopologyShow
** 功能描述: 显示 CPU 拓扑
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  x86CpuTopologyShow (VOID)
{
    X86_CPU_TOPOLOGY  *pCpuTopology = &_G_x86CpuTopology;
    INT                iNumCpus     = _G_uiX86CpuCount;
    INT                i;

    printf("\n Logical Processor Breakdown: \n\n");

    printf(" Maximum Logical Processors per Package: 0x%x \n", pCpuTopology->TOP_uiMaxLProcsPkg);
    printf(" Maximum Number of Cores per Package   : 0x%x \n", pCpuTopology->TOP_uiMaxCoresPkg);
    printf(" Maximum Logical Processors per Core   : 0x%x \n", pCpuTopology->TOP_uiMaxLProcsCore);

    printf("\n Number of Packages: 0x%x \n", pCpuTopology->TOP_uiNumPkgs);
    printf(" Number of Cores   : 0x%x \n",   pCpuTopology->TOP_uiNumCores);

    printf("\n APIC ID Bit Widths: Package 0x%x, Core 0x%x, SMT 0x%x \n\n",
            pCpuTopology->TOP_uiPkgMaskWidth,
            pCpuTopology->TOP_uiCoreMaskWidth,
            pCpuTopology->TOP_uiSmtMaskWidth);

    printf(" CPU Topology :  \n\n");

    for (i = 0; i < iNumCpus; i++) {
        printf(" [%02d]. TOP_aucPkgIds 0x%x (0x%x), TOP_aucCoreIds 0x%x (0x%x), TOP_aucSmtIds 0x%x \n",
                i,
                ((pCpuTopology->TOP_aucPkgIds[i]) >>
                 (pCpuTopology->TOP_uiCoreMaskWidth + pCpuTopology->TOP_uiSmtMaskWidth)),
                  pCpuTopology->TOP_aucPkgIds[i],
                ((pCpuTopology->TOP_aucCoreIds[i]) >> (pCpuTopology->TOP_uiSmtMaskWidth)),
                  pCpuTopology->TOP_aucCoreIds[i],
                  pCpuTopology->TOP_aucSmtIds[i]);
    }

    printf("\n CPU ID Table :  \n\n");

    for (i = 0; i < iNumCpus; i++) {
        printf(" [%02d]. TOP_auiPhysIdx[%02d] = 0x%x \n", i, i,  pCpuTopology->TOP_auiPhysIdx[i]);
    }

    printf("\n Number of Logical Processors Enabled: 0x%x \n\n", pCpuTopology->TOP_uiNumLProcsEnabled);

#if LW_CFG_SMP_EN > 0
    printf(" TOP_smtSet = {  \n");

    if (iNumCpus > 1) {
        /*
         * HT/SMT: Has physical/logical pair
         */
        if (pCpuTopology->TOP_aucSmtIds[1] != 0) {
            for (i = 0; i < iNumCpus; i += 2) {
                printf("            0x%0x,", pCpuTopology->TOP_smtSet[i]);
                printf(" 0x%0x,\n",          pCpuTopology->TOP_smtSet[i + 1]);
            }
        } else {
            /*
             * Without HT/SMT: We have NO physical/logical pair
             **/
            for (i=0; i < iNumCpus; i++) {
                printf("            0x%0x,\n", pCpuTopology->TOP_smtSet[i]);
            }
        }
    } else {
        printf("            0x%0x,\n", pCpuTopology->TOP_smtSet[0]);
    }

    printf("            }  \n");
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

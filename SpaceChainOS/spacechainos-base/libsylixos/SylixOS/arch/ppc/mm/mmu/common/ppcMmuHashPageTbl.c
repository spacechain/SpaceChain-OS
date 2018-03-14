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
** 文   件   名: ppcMmuHashPageTbl.c
**
** 创   建   人: Dang.YueDong (党跃东)
**
** 文件创建日期: 2016 年 01 月 15 日
**
** 描        述: PowerPC 体系构架 MMU Hashed 页表驱动.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "./ppcMmuHashPageTbl.h"
/*********************************************************************************************************
  外部接口声明
*********************************************************************************************************/
extern VOID  ppcMmuSetSR(UINT32  uiSRn, UINT32  uiValue);
extern VOID  ppcMmuSetSDR1(UINT32);
extern VOID  ppcHashPageTblPteSet(PTE    *pPte,
                                  UINT32  uiWord0,
                                  UINT32  uiWord1,
                                  UINT32  uiEffectiveAddr);
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
static SR       _G_SRs[16];
static UINT32   _G_uiHashPageTblOrg;
static UINT32   _G_uiHashPageTblMask;
static LW_SPINLOCK_DEFINE_CACHE_ALIGN(_G_slHashPageTblLock);

#define __MMU_PTE_DEBUG
#ifdef __MMU_PTE_DEBUG
static UINT     _G_uiPtegNr = 0;
static UINT     _G_uiPteMissCounter = 0;
#endif                                                                  /*  defined(__MMU_PTE_DEBUG)    */
/*********************************************************************************************************
** 函数名称: ppcMmuPtegAddrGet
** 功能描述: 通过有效地址获得 PTEG
** 输　入  : effectiveAddr           有效地址
**           ppPrimPteg              主 PTEG
**           ppSecPteg               次 PTEG
**           puiAPI                  API
**           puiVSID                 VSID
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcMmuPtegAddrGet (EA       effectiveAddr,
                                PTEG   **ppPrimPteg,
                                PTEG   **ppSecPteg,
                                UINT32  *puiAPI,
                                UINT32  *puiVSID)
{
    UINT32  uiPageIndex;
    UINT32  uiHashValue1;
    UINT32  uiHashValue2;
    UINT32  uiHashValue1L;
    UINT32  uiHashValue1H;
    UINT32  uiHashValue2L;
    UINT32  uiHashValue2H;

    *puiVSID      = _G_SRs[effectiveAddr.field.EA_ucSRn].field.SR_uiVSID;
    uiPageIndex   = effectiveAddr.field.EA_uiPageIndex;
    *puiAPI       = (uiPageIndex & MMU_EA_API) >> MMU_EA_API_SHIFT;

    uiHashValue1  = (*puiVSID & MMU_VSID_PRIM_HASH) ^ uiPageIndex;
    uiHashValue2  = ~uiHashValue1;

    uiHashValue1L = (uiHashValue1 & MMU_HASH_VALUE_LOW)  << MMU_PTE_HASH_VALUE_LOW_SHIFT;
    uiHashValue1H = (uiHashValue1 & MMU_HASH_VALUE_HIGH) >> MMU_HASH_VALUE_HIGH_SHIFT;

    uiHashValue2L = (uiHashValue2 & MMU_HASH_VALUE_LOW)  << MMU_PTE_HASH_VALUE_LOW_SHIFT;
    uiHashValue2H = (uiHashValue2 & MMU_HASH_VALUE_HIGH) >> MMU_HASH_VALUE_HIGH_SHIFT;

    uiHashValue1H = (uiHashValue1H & _G_uiHashPageTblMask) << MMU_PTE_HASH_VALUE_HIGH_SHIFT;
    uiHashValue2H = (uiHashValue2H & _G_uiHashPageTblMask) << MMU_PTE_HASH_VALUE_HIGH_SHIFT;

    *ppPrimPteg   = (PTEG *)(_G_uiHashPageTblOrg | uiHashValue1H | uiHashValue1L);
    *ppSecPteg    = (PTEG *)(_G_uiHashPageTblOrg | uiHashValue2H | uiHashValue2L);
}
/*********************************************************************************************************
** 函数名称: ppcMmuHashPageTblInit
** 功能描述: 初始化 Hashed 页表
** 输　入  : uiMemSize               内存大小
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  ppcMmuHashPageTblInit (UINT32  uiMemSize)
{
    UINT32  stHashPageTblSize;
    PVOID   pvHashPageTblAddr;
    INT     i;

    LW_SPIN_INIT(&_G_slHashPageTblLock);

    /*
     * 配置 16 个段寄存器，让 VA = EA
     */
    for (i = 0; i < 16; i++) {
        _G_SRs[i].SR_uiValue      = 0;
        _G_SRs[i].field.SR_bT     = 0;
        _G_SRs[i].field.SR_bKS    = 1;
        _G_SRs[i].field.SR_bKP    = 1;
        _G_SRs[i].field.SR_bN     = 0;
        _G_SRs[i].field.SR_uiVSID = i;
        ppcMmuSetSR(i, _G_SRs[i].SR_uiValue);
    }

    if (uiMemSize < MMU_PTE_MIN_SIZE_8M) {
        return  (PX_ERROR);
    }

    if (uiMemSize >= MMU_PTE_MIN_SIZE_4G) {
        _G_uiHashPageTblOrg  = MMU_SDR1_HTABORG_4G;
        _G_uiHashPageTblMask = MMU_SDR1_HTABMASK_4G;
        stHashPageTblSize    = MMU_PTE_MIN_SIZE_4G;

    } else if (uiMemSize >= MMU_PTE_MIN_SIZE_2G) {
        _G_uiHashPageTblOrg  = MMU_SDR1_HTABORG_2G;
        _G_uiHashPageTblMask = MMU_SDR1_HTABMASK_2G;
        stHashPageTblSize    = MMU_PTE_MIN_SIZE_2G;

    } else if (uiMemSize >= MMU_PTE_MIN_SIZE_1G) {
        _G_uiHashPageTblOrg  = MMU_SDR1_HTABORG_1G;
        _G_uiHashPageTblMask = MMU_SDR1_HTABMASK_1G;
        stHashPageTblSize    = MMU_PTE_MIN_SIZE_1G;

    } else if (uiMemSize >= MMU_PTE_MIN_SIZE_512M) {
        _G_uiHashPageTblOrg  = MMU_SDR1_HTABORG_512M;
        _G_uiHashPageTblMask = MMU_SDR1_HTABMASK_512M;
        stHashPageTblSize    = MMU_PTE_MIN_SIZE_512M;

    } else if (uiMemSize >= MMU_PTE_MIN_SIZE_256M) {
        _G_uiHashPageTblOrg  = MMU_SDR1_HTABORG_256M;
        _G_uiHashPageTblMask = MMU_SDR1_HTABMASK_256M;
        stHashPageTblSize    = MMU_PTE_MIN_SIZE_256M;

    } else if (uiMemSize >= MMU_PTE_MIN_SIZE_128M) {
        _G_uiHashPageTblOrg  = MMU_SDR1_HTABORG_128M;
        _G_uiHashPageTblMask = MMU_SDR1_HTABMASK_128M;
        stHashPageTblSize    = MMU_PTE_MIN_SIZE_128M;

    } else if (uiMemSize >= MMU_PTE_MIN_SIZE_64M) {
        _G_uiHashPageTblOrg  = MMU_SDR1_HTABORG_64M;
        _G_uiHashPageTblMask = MMU_SDR1_HTABMASK_64M;
        stHashPageTblSize    = MMU_PTE_MIN_SIZE_64M;

    } else if (uiMemSize >= MMU_PTE_MIN_SIZE_32M) {
        _G_uiHashPageTblOrg  = MMU_SDR1_HTABORG_32M;
        _G_uiHashPageTblMask = MMU_SDR1_HTABMASK_32M;
        stHashPageTblSize    = MMU_PTE_MIN_SIZE_32M;

    } else if (uiMemSize >= MMU_PTE_MIN_SIZE_16M) {
        _G_uiHashPageTblOrg  = MMU_SDR1_HTABORG_16M;
        _G_uiHashPageTblMask = MMU_SDR1_HTABMASK_16M;
        stHashPageTblSize    = MMU_PTE_MIN_SIZE_16M;

    } else {
        _G_uiHashPageTblOrg  = MMU_SDR1_HTABORG_8M;
        _G_uiHashPageTblMask = MMU_SDR1_HTABMASK_8M;
        stHashPageTblSize    = MMU_PTE_MIN_SIZE_8M;
    }

    /*
     * Hashed 页表所在的内存的内存访问属性 M 位必须为 1 (即硬件保证内存一致性)
     */
    pvHashPageTblAddr = __KHEAP_ALLOC_ALIGN(stHashPageTblSize, stHashPageTblSize);
    if (pvHashPageTblAddr) {
        lib_bzero(pvHashPageTblAddr, stHashPageTblSize);

        _G_uiHashPageTblOrg &= (UINT32)pvHashPageTblAddr;

        ppcMmuSetSDR1(_G_uiHashPageTblOrg | _G_uiHashPageTblMask);

#ifdef __MMU_PTE_DEBUG
        _G_uiPtegNr = stHashPageTblSize / 64;
#endif                                                                  /*  defined(__MMU_PTE_DEBUG)    */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppcHashPageTblSearchInvalidPte
** 功能描述: 搜索无效 PTE
** 输　入  : pPteg                 PTEG
** 输　出  : PTE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PTE  *ppcHashPageTblSearchInvalidPte (PTEG  *pPteg)
{
    UINT  uiIndex;
    PTE  *pPte;

    for (uiIndex = 0; uiIndex < MMU_PTES_IN_PTEG; ++uiIndex) {
        pPte = &pPteg->PTEG_PTEs[uiIndex];
        if (!pPte->field.PTE_bV) {
            return  (pPte);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: ppcHashPageTblSearchEliminatePte
** 功能描述: 强制淘汰一个 PTE
** 输　入  : pPrimPteg             主 PTEG
**           pSecPteg              次 PTEG
**           pbIsPrimary           是否主 PTE
** 输　出  : PTE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PTE  *ppcHashPageTblSearchEliminatePte (PTEG  *pPrimPteg,
                                               PTEG  *pSecPteg,
                                               BOOL  *pbIsPrimary)
{
    UINT  uiIndex;
    PTE  *pPte;

    for (uiIndex = 0; uiIndex < MMU_PTES_IN_PTEG; ++uiIndex) {
        pPte = &pPrimPteg->PTEG_PTEs[uiIndex];
        if (!pPte->field.PTE_bR) {
            *pbIsPrimary = LW_TRUE;
            return  (pPte);
        }
    }

    for (uiIndex = 0; uiIndex < MMU_PTES_IN_PTEG; ++uiIndex) {
        pPte = &pSecPteg->PTEG_PTEs[uiIndex];
        if (!pPte->field.PTE_bR) {
            *pbIsPrimary = LW_FALSE;
            return  (pPte);
        }
    }

    for (uiIndex = 0; uiIndex < MMU_PTES_IN_PTEG; ++uiIndex) {
        pPte = &pPrimPteg->PTEG_PTEs[uiIndex];
        if (!pPte->field.PTE_bC) {
            *pbIsPrimary = LW_TRUE;
            return  (pPte);
        }
    }

    for (uiIndex = 0; uiIndex < MMU_PTES_IN_PTEG; ++uiIndex) {
        pPte = &pSecPteg->PTEG_PTEs[uiIndex];
        if (!pPte->field.PTE_bC) {
            *pbIsPrimary = LW_FALSE;
            return  (pPte);
        }
    }

    pPte = &pPrimPteg->PTEG_PTEs[0];
    *pbIsPrimary = LW_TRUE;

    return  (pPte);
}
/*********************************************************************************************************
** 函数名称: ppcHashPageTblPteAdd
** 功能描述: 增加一个 PTE 到 Hash 页表
** 输　入  : pPte                  PTE
**           effectiveAddr         有效地址
**           uiPteValue1           PTE 值1
**           uiAPI                 API
**           uiVSID                VSID
**           bR                    Referenced bit
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  ppcHashPageTblPteAdd (PTE    *pPte,
                                   EA      effectiveAddr,
                                   UINT32  uiPteValue1,
                                   BOOL    bIsPrimary,
                                   UINT32  uiAPI,
                                   UINT32  uiVSID,
                                   BOOL    bR)
{
    PTE     pteTemp;

    pteTemp.words.PTE_uiWord0 = 0;
    pteTemp.words.PTE_uiWord1 = 0;

    pteTemp.field.PTE_uiVSID  = uiVSID;
    pteTemp.field.PTE_bH      = !bIsPrimary;
    pteTemp.field.PTE_ucAPI   = uiAPI;
    pteTemp.field.PTE_bV      = 1;
    pteTemp.field.PTE_bR      = bR;
    ppcHashPageTblPteSet(pPte,
                         pteTemp.words.PTE_uiWord0,
                         uiPteValue1,
                         effectiveAddr.EA_uiValue);
}
/*********************************************************************************************************
** 函数名称: ppcHashPageTblSearchPteByEA
** 功能描述: 通过有效地址查找 PTE
** 输　入  : pPteg                 PTEG
**           effectiveAddr         有效地址
**           uiAPI                 API
**           uiVSID                VSID
** 输　出  : PTE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PTE  *ppcHashPageTblSearchPteByEA (PTEG   *pPteg,
                                          EA      effectiveAddr,
                                          UINT32  uiAPI,
                                          UINT32  uiVSID)
{
    UINT    uiIndex;
    PTE    *pPte;

    for (uiIndex = 0; uiIndex < MMU_PTES_IN_PTEG; ++uiIndex) {
        pPte = &pPteg->PTEG_PTEs[uiIndex];
        if ((pPte->field.PTE_bV) &&
            (pPte->field.PTE_uiVSID == uiVSID) &&
            (pPte->field.PTE_ucAPI  == uiAPI)) {
            return  (pPte);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: ppcMmuHashPageTblMakeTrans
** 功能描述: 构建映射关系
** 输　入  : ulEffectiveAddr       有效地址
**           uiPteValue1           PTE 值1
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  ppcMmuHashPageTblMakeTrans (addr_t  ulEffectiveAddr,
                                  UINT32  uiPteValue1)
{
    INTREG  iregInterLevel;
    PTEG   *pPrimPteg;
    PTEG   *pSecPteg;
    PTE    *pPte;
    BOOL    bIsPrimary = LW_TRUE;
    EA      effectiveAddr;
    UINT32  uiAPI;
    UINT32  uiVSID;

    effectiveAddr.EA_uiValue = ulEffectiveAddr;
    ppcMmuPtegAddrGet(effectiveAddr,
                      &pPrimPteg,
                      &pSecPteg,
                      &uiAPI,
                      &uiVSID);

    LW_SPIN_LOCK_QUICK(&_G_slHashPageTblLock, &iregInterLevel);

    /*
     * 搜索 EA 的 PTE
     */
    pPte = ppcHashPageTblSearchPteByEA(pPrimPteg,
                                       effectiveAddr,
                                       uiAPI,
                                       uiVSID);
    if (!pPte) {
        pPte = ppcHashPageTblSearchPteByEA(pSecPteg,
                                           effectiveAddr,
                                           uiAPI,
                                           uiVSID);
    }

    if (pPte) {
        /*
         * 找到了
         */
        if (uiPteValue1) {
            /*
             * 改变 PTE
             */
            ppcHashPageTblPteSet(pPte,
                                 pPte->words.PTE_uiWord0,
                                 uiPteValue1,
                                 ulEffectiveAddr);
        } else {
            /*
             * 删除 PTE
             */
            ppcHashPageTblPteSet(pPte,
                                 0,
                                 0,
                                 ulEffectiveAddr);
        }
    } else {
        if (uiPteValue1) {
            /*
             * 没有找到，查找一个无效的 PTE
             */
            pPte = ppcHashPageTblSearchInvalidPte(pPrimPteg);
            if (!pPte) {
                pPte = ppcHashPageTblSearchInvalidPte(pSecPteg);
                if (pPte) {
                    bIsPrimary = LW_FALSE;
                }
            }

            if (pPte) {
                /*
                 * 找到无效的 PTE，增加一个 PTE
                 */
                ppcHashPageTblPteAdd(pPte,
                                     effectiveAddr,
                                     uiPteValue1,
                                     bIsPrimary,
                                     uiAPI,
                                     uiVSID,
                                     LW_FALSE);
            } else {
                /*
                 * 没有无效的 PTE，外部会无效 TLB，不作强制淘汰 PTE，让其产生 MISS
                 */
            }
        } else {
            /*
             * 没有无效的 PTE，外部会无效 TLB，不作强制淘汰 PTE，让其产生 MISS
             */
        }
    }

    LW_SPIN_UNLOCK_QUICK(&_G_slHashPageTblLock, iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: ppcMmuHashPageTblFlagSet
** 功能描述: 设置映射标志
** 输　入  : ulEffectiveAddr       有效地址
**           uiPteValue1           PTE 值1
** 输　出  : NONE
** 全局变量:
** 调用模块: 
** 说  明  : 如果没有找到，外部会无效 TLB，不作查找无效 PTE 和强制淘汰 PTE，让其产生 MISS
*********************************************************************************************************/
VOID  ppcMmuHashPageTblFlagSet (addr_t  ulEffectiveAddr,
                                UINT32  uiPteValue1)
{
    INTREG  iregInterLevel;
    PTEG   *pPrimPteg;
    PTEG   *pSecPteg;
    PTE    *pPte;
    EA      effectiveAddr;
    UINT32  uiAPI;
    UINT32  uiVSID;

    effectiveAddr.EA_uiValue = ulEffectiveAddr;
    ppcMmuPtegAddrGet(effectiveAddr,
                      &pPrimPteg,
                      &pSecPteg,
                      &uiAPI,
                      &uiVSID);

    LW_SPIN_LOCK_QUICK(&_G_slHashPageTblLock, &iregInterLevel);

    /*
     * 搜索 EA 的 PTE
     */
    pPte = ppcHashPageTblSearchPteByEA(pPrimPteg,
                                       effectiveAddr,
                                       uiAPI,
                                       uiVSID);
    if (!pPte) {
        pPte = ppcHashPageTblSearchPteByEA(pSecPteg,
                                           effectiveAddr,
                                           uiAPI,
                                           uiVSID);
    }

    if (pPte) {
        /*
         * 找到了
         */
        if (uiPteValue1) {
            /*
             * 改变 PTE
             */
            ppcHashPageTblPteSet(pPte,
                                 pPte->words.PTE_uiWord0,
                                 uiPteValue1,
                                 ulEffectiveAddr);
        } else {
            /*
             * 删除 PTE
             */
            ppcHashPageTblPteSet(pPte,
                                 0,
                                 0,
                                 ulEffectiveAddr);
        }
    } else {
        /*
         * 没有找到，外部会无效 TLB，不作查找无效 PTE 和强制淘汰 PTE，让其产生 MISS
         */
    }

    LW_SPIN_UNLOCK_QUICK(&_G_slHashPageTblLock, iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: ppcMmuHashPageTblPteMiss
** 功能描述: 处理 PTE MISS
** 输　入  : ulEffectiveAddr       有效地址
**           uiPteValue1           PTE 值1
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  ppcMmuHashPageTblPteMiss (addr_t  ulEffectiveAddr,
                                UINT32  uiPteValue1)
{
    INTREG  iregInterLevel;
    PTEG   *pPrimPteg;
    PTEG   *pSecPteg;
    PTE    *pPte;
    BOOL    bIsPrimary = LW_TRUE;
    EA      effectiveAddr;
    UINT32  uiAPI;
    UINT32  uiVSID;

    effectiveAddr.EA_uiValue = ulEffectiveAddr;
    ppcMmuPtegAddrGet(effectiveAddr,
                      &pPrimPteg,
                      &pSecPteg,
                      &uiAPI,
                      &uiVSID);

    LW_SPIN_LOCK_QUICK(&_G_slHashPageTblLock, &iregInterLevel);

#ifdef __MMU_PTE_DEBUG
    _G_uiPteMissCounter++;
#endif                                                                  /*  defined(__MMU_PTE_DEBUG)    */

    /*
     * 搜索一个无效的 PTE
     */
    pPte = ppcHashPageTblSearchInvalidPte(pPrimPteg);
    if (!pPte) {
        pPte = ppcHashPageTblSearchInvalidPte(pSecPteg);
        if (pPte) {
            bIsPrimary = LW_FALSE;
        }
    }

    if (!pPte) {
        /*
         * 强制淘汰一个 PTE
         */
        pPte = ppcHashPageTblSearchEliminatePte(pPrimPteg,
                                                pSecPteg,
                                                &bIsPrimary);

        /*
         * 这里并不无效被淘汰的 PTE 对应的 TLB
         * See << programming_environment_manual >> Figure 7-17 and Figure 7-26
         */
    }

    /*
     * 增加一个 PTE
     */
    ppcHashPageTblPteAdd(pPte,
                         effectiveAddr,
                         uiPteValue1,
                         bIsPrimary,
                         uiAPI,
                         uiVSID,
                         LW_FALSE);

    LW_SPIN_UNLOCK_QUICK(&_G_slHashPageTblLock, iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: ppcMmuHashPageTblPtePreLoad
** 功能描述: PTE 预加载
** 输　入  : ulEffectiveAddr       有效地址
**           uiPteValue1           PTE 值1
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  ppcMmuHashPageTblPtePreLoad (addr_t  ulEffectiveAddr,
                                   UINT32  uiPteValue1)
{
    INTREG  iregInterLevel;
    PTEG   *pPrimPteg;
    PTEG   *pSecPteg;
    PTE    *pPte;
    BOOL    bIsPrimary = LW_TRUE;
    EA      effectiveAddr;
    UINT32  uiAPI;
    UINT32  uiVSID;

    effectiveAddr.EA_uiValue = ulEffectiveAddr;
    ppcMmuPtegAddrGet(effectiveAddr,
                      &pPrimPteg,
                      &pSecPteg,
                      &uiAPI,
                      &uiVSID);

    LW_SPIN_LOCK_QUICK(&_G_slHashPageTblLock, &iregInterLevel);

    /*
     * 搜索 EA 的 PTE
     */
    pPte = ppcHashPageTblSearchPteByEA(pPrimPteg,
                                       effectiveAddr,
                                       uiAPI,
                                       uiVSID);
    if (!pPte) {
        pPte = ppcHashPageTblSearchPteByEA(pSecPteg,
                                           effectiveAddr,
                                           uiAPI,
                                           uiVSID);
    }

    if (!pPte) {
        /*
         * 搜索一个无效的 PTE
         */
        pPte = ppcHashPageTblSearchInvalidPte(pPrimPteg);
        if (!pPte) {
            pPte = ppcHashPageTblSearchInvalidPte(pSecPteg);
            if (pPte) {
                bIsPrimary = LW_FALSE;
            }
        }

        if (!pPte) {
            /*
             * 强制淘汰一个 PTE
             */
            pPte = ppcHashPageTblSearchEliminatePte(pPrimPteg,
                                                    pSecPteg,
                                                    &bIsPrimary);

            /*
             * 这里并不无效被淘汰的 PTE 对应的 TLB
             * See << programming_environment_manual >> Figure 7-17 and Figure 7-26
             */
        }

        /*
         * 增加一个 PTE
         */
        ppcHashPageTblPteAdd(pPte,
                             effectiveAddr,
                             uiPteValue1,
                             bIsPrimary,
                             uiAPI,
                             uiVSID,
                             LW_TRUE);                                  /*  避免立即被淘汰              */
    }

    LW_SPIN_UNLOCK_QUICK(&_G_slHashPageTblLock, iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: ppcMmuHashPageTblShow
** 功能描述: 打印 MMU HASH PAGE 信息
** 输　入  : NONE
** 输　出  : ERROR_NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#ifdef __MMU_PTE_DEBUG

INT  ppcMmuHashPageTblShow (VOID)
{
    PTEG   *pPteg = (PTEG *)_G_uiHashPageTblOrg;
    INT     i, j;
    INT     iCount;
    INT     iNewLine = 0;

    printf("PTEG usage:\n");

    for (i = 0; i < _G_uiPtegNr; i++, pPteg++) {
        iCount = 0;
        for (j = 0; j < 8; j++) {
            if (pPteg->PTEG_PTEs[j].field.PTE_bV) {
                iCount++;
            }
        }

        if (iCount) {
            printf("%5d-%d ", i, iCount);
            if (++iNewLine % 10 == 0) {
                printf("\n");
            }
        }
    }

    printf("\n\nPTE Miss Counter = %d\n", _G_uiPteMissCounter);
    printf("PTEG Number\t = %d\n", _G_uiPtegNr);

    return  (ERROR_NONE);
}

#endif                                                                  /*  defined(__MMU_PTE_DEBUG)    */
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/

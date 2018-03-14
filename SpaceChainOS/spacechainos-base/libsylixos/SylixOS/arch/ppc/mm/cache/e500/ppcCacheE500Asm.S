;/*********************************************************************************************************
;**
;**                                    中国软件开源组织
;**
;**                                   嵌入式实时操作系统
;**
;**                                       SylixOS(TM)
;**
;**                               Copyright  All Rights Reserved
;**
;**--------------文件信息--------------------------------------------------------------------------------
;**
;** 文   件   名: ppcCacheE500Asm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 05 月 03 日
;**
;** 描        述: PowerPC E500 体系构架 CACHE 驱动.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/cache_cfg.h>

#if LW_CFG_CACHE_EN > 0

#define  __SYLIXOS_PPC_E500__
#include "arch/ppc/arch_e500.h"

    EXPORT_LABEL(ppcE500ICacheEnable)
    EXPORT_LABEL(ppcE500DCacheEnable)

    EXPORT_LABEL(ppcE500ICacheDisable)
    EXPORT_LABEL(ppcE500DCacheDisable)

    EXPORT_LABEL(ppcE500ICacheInvalidate)

    EXPORT_LABEL(ppcE500DCacheInvalidate)
    EXPORT_LABEL(ppcE500DCacheClear)
    EXPORT_LABEL(ppcE500DCacheFlush)

    EXPORT_LABEL(ppcE500ICacheInvalidateAll)

    EXPORT_LABEL(ppcE500DCacheFlushAll)

    EXPORT_LABEL(ppcE500TextUpdate)

    EXPORT_LABEL(ppcE500BranchPredictionDisable)
    EXPORT_LABEL(ppcE500BranchPredictionEnable)
    EXPORT_LABEL(ppcE500BranchPredictorInvalidate)

    EXPORT_LABEL(ppcE500CacheGetL1CFG0)
    EXPORT_LABEL(ppcE500CacheGetL1CFG1)

    IMPORT_LABEL(_G_pucPpcCacheReadBuffer)

    IMPORT_LABEL(PPC_E500_ICACHE_LINE_NUM)
    IMPORT_LABEL(PPC_E500_DCACHE_LINE_NUM)
    IMPORT_LABEL(PPC_E500_DCACHE_ALIGN_SIZE)

;/*********************************************************************************************************
;  定义
;*********************************************************************************************************/

;/* pages are 4K bytes */
#define PPC_MMU_PAGE_SIZE               0x1000

;/* RPN field offset in TLB Word 0 (left shift this number of bits) */
#define PPC_MMU_RPN_SHIFT               12

    FILE_BEGIN()

;/*********************************************************************************************************
;  获得 L1CFG0 寄存器
;*********************************************************************************************************/

FUNC_DEF(ppcE500CacheGetL1CFG0)
    MFSPR   R3 , L1CFG0
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  获得 L1CFG1 寄存器
;*********************************************************************************************************/

FUNC_DEF(ppcE500CacheGetL1CFG1)
    MFSPR   R3 , L1CFG1
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  使能 ICACHE
;*********************************************************************************************************/

FUNC_DEF(ppcE500ICacheEnable)
    MFSPR   R4 , L1CSR1
    ANDI.   R5 , R4 , ARCH_PPC_L1CSR_E
    BNE     ppcE500CacheRetOK                                   ;/*  enabled just return OK              */

    ORI     R5 , R4 , ARCH_PPC_L1CSR_FI                         ;/*  flash invalidate and enable         */
    ;/* nothing reqd before updating L1CSR1 register */
    MTSPR   L1CSR1 , R5                                         ;/*  flash invalidate L1 instruction     */
                                                                ;/*  cache - 1 CPU cycle                 */
    ISYNC                                                       ;/*  isync reqd after                    */

    ORI     R5 , R4 , ARCH_PPC_L1CSR_E
    ;/* nothing reqd before updating L1CSR1 register */
    MTSPR   L1CSR1 , R5                                         ;/*  enable instruction cache            */
    ISYNC                                                       ;/*  isync reqd after                    */

    B       ppcE500CacheRetOK                                   ;/*  return OK                           */
    FUNC_END()

;/*********************************************************************************************************
;  使能 DCACHE
;*********************************************************************************************************/

FUNC_DEF(ppcE500DCacheEnable)
    MFSPR   R4 , L1CSR0
    ANDI.   R5 , R4 , ARCH_PPC_L1CSR_E
    BNE     ppcE500CacheRetOK                                   ;/*  enabled just return OK              */

    ORI     R5 , R4 , ARCH_PPC_L1CSR_FI                         ;/*  flash invalidate then enable        */
    MSYNC
    ISYNC
    ;/* msync isync reqd before updating L1CSR0 register */
    MTSPR   L1CSR0 , R5                                         ;/*  flash invalidate data cache         */
                                                                ;/*  - 1 CPU cycle                       */
    ORI     R5 , R4 , ARCH_PPC_L1CSR_E
    ;/* msync isync reqd before updating L1CSR0 register */
    MSYNC
    ISYNC
    MTSPR   L1CSR0 , R5                                         ;/*  enable data cache                   */
    ISYNC                                                       ;/*  isync reqd after L1CSR0 update      */

    B       ppcE500CacheRetOK                                   ;/*  return OK                           */
    FUNC_END()

;/*********************************************************************************************************
;  禁能 ICACHE
;*********************************************************************************************************/

FUNC_DEF(ppcE500ICacheDisable)
    MFSPR   R4 , L1CSR1
    ANDI.   R5 , R4 , ARCH_PPC_L1CSR_E
    BEQ     ppcE500CacheRetOK                                   ;/*  not enabled just return OK          */

    ;/*
     ;* The following 8 lines disable the L1 instruction cache via
     ;* masking the enable bit in the L1CSR1 register
     ;*/
    LIS     R5 , HIADJ(~ARCH_PPC_L1CSR_E)
    ADDI    R5 , R5 , LO(~ARCH_PPC_L1CSR_E)
    AND     R5 , R5 , R4
    ;/* nothing reqd before updating L1CSR1 register */
    MTSPR   L1CSR1 , R5
    ;/* isync reqd after updating L1CSR1 register */
    ISYNC
    B       ppcE500CacheRetOK                                   ;/*  return OK                           */
    FUNC_END()

;/*********************************************************************************************************
;  禁能 DCACHE
;*********************************************************************************************************/

FUNC_DEF(ppcE500DCacheDisable)
    MFSPR   R4 , L1CSR0
    ANDI.   R5 , R4 , ARCH_PPC_L1CSR_E
    BEQ     ppcE500CacheRetOK                                   ;/*  not enabled just return OK          */

    ;/*
     ;* if the MMU is enabled , the only way to disable the cache is
     ;* via the MMU.
     ;*/
    LIS     R6 , HIADJ(PPC_E500_DCACHE_LINE_NUM)
    LWZ     R6 , LO(PPC_E500_DCACHE_LINE_NUM)(R6)

    ;/*
     ;* R6 contains the count of cache lines to be fetched & flushed.
     ;* Convert to a count of pages covered, and fetch a word from
     ;* each page to ensure that all addresses involved are in
     ;* the TLB so that reloads do not disrupt the flush loop.
     ;* A simple shift without round-up is sufficient because
     ;* the R6 value is always a multiple of the shift count.
     ;*/
    SRWI    R5 , R6 , PPC_MMU_RPN_SHIFT - ARCH_PPC_CACHE_ALIGN_SHIFT
    MTSPR   CTR, R5
    LIS     R5 , HIADJ(_G_pucPpcCacheReadBuffer)
    lwz     R5 , LO(_G_pucPpcCacheReadBuffer)(R5)
    LI      R9 , PPC_MMU_PAGE_SIZE

    ;/*
     ;* There might be a page boundary between here and the end of
     ;* the function, so make sure both pages are in the I-TLB.
     ;*/
    B       cacheE500DisableLoadItlb

    ;/* Here we prefetch the TLBs for the pages used int he cache flush */
LINE_LABEL(cacheE500DisableLoadDtlb)
    LBZU    R7  , 0(R5)
    ADD     R5  , R5 , R9
    BDNZ    cacheE500DisableLoadDtlb

    MTSPR   CTR , R6                                            ;/* load CTR with the number of index    */

    ;/*
     ;* load up R5 with the buffer address minus
     ;* one cache block size
     ;*/
    LIS     R7  , HIADJ(PPC_E500_DCACHE_ALIGN_SIZE)
    LWZ     R7  , LO(PPC_E500_DCACHE_ALIGN_SIZE)(R7)
    LIS     R10 , HIADJ(_G_pucPpcCacheReadBuffer)
    LWZ     R10 , LO(_G_pucPpcCacheReadBuffer)(R10)

    SUBF    R5  , R7 , R10                                      ;/*  buf ptr to text - cache line size   */

LINE_LABEL(cacheE500DisableLoad)
    ADD     R5  , R7 , R5                                       ;/*  + cache line size                   */
    LBZU    R10 , 0(R5)                                         ;/*  load the data cache with new block  */
    BDNZ    cacheE500DisableLoad                                ;/*  loop till cache ctr is zero         */
    ISYNC

    MTSPR   CTR , R6                                            ;/*  load CTR with the number of index   */

    LIS     R10 , HIADJ(_G_pucPpcCacheReadBuffer)
    LWZ     R10 , LO(_G_pucPpcCacheReadBuffer)(R10)
    SUBF    R5  , R7 , R10                                      ;/*  buf ptr to text - cache line size   */

LINE_LABEL(cacheE500DisableFlush)
    ADD     R5 , R7 , R5                                        ;/*  + cache line size                   */
    DCBF    0  , R5                                             ;/*  flush the data cache block          */
    BDNZ    cacheE500DisableFlush                               ;/*  loop till cache ctr is zero         */
    SYNC
    ISYNC
    MFSPR   R5 , L1CSR0
    LIS     R4 , HIADJ(~ARCH_PPC_L1CSR_E)
    ADDI    R4 , R4 , LO(~ARCH_PPC_L1CSR_E)
    AND     R5 , R5 , R4                                        ;/*  Mask out the cache enable bit       */
    ;/* msync isync reqd before updating L1CSR0 register */
    MSYNC
    ISYNC
    MTSPR   L1CSR0 , R5                                         ;/*  disable the data cache              */

    B       ppcE500CacheRetOK                                   ;/*  return OK                           */

LINE_LABEL(cacheE500DisableLoadItlb)
    B       cacheE500DisableLoadDtlb
    FUNC_END()

;/*********************************************************************************************************
;  D-CACHE 所有数据回写
;*********************************************************************************************************/

FUNC_DEF(ppcE500DCacheFlushAll)
    LIS     R6 , HIADJ(PPC_E500_DCACHE_LINE_NUM)
    LWZ     R6 , LO(PPC_E500_DCACHE_LINE_NUM)(R6)

    ;/*
     ;* R6 contains the count of cache lines to be fetched & flushed.
     ;* Convert to a count of pages covered, and fetch a word from
     ;* each page to ensure that all addresses involved are in
     ;* the TLB so that reloads do not disrupt the flush loop.
     ;* A simple shift without round-up is sufficient because
     ;* the R6 value is always a multiple of the shift count.
     ;*/
    SRWI    R5 , R6 , PPC_MMU_RPN_SHIFT - ARCH_PPC_CACHE_ALIGN_SHIFT
    MTSPR   CTR , R5
    LIS     R5 , HIADJ(_G_pucPpcCacheReadBuffer)
    LWZ     R5 , LO(_G_pucPpcCacheReadBuffer)(R5)
    LI      R9 , PPC_MMU_PAGE_SIZE

    ;/*
     ;* There might be a page boundary between here and the end of
     ;* the function, so make sure both pages are in the I-TLB.
     ;*/
    B       cacheE500DisableLoadItlb2

    ;/* Here we prefetch the TLBs for the pages used int he cache flush */
LINE_LABEL(cacheE500DisableLoadDtlb2)
    LBZU    R7 , 0(R5)
    ADD     R5 , R5 , R9
    BDNZ    cacheE500DisableLoadDtlb2

    MTSPR   CTR , R6                                            ;/*  load CTR with the number of index   */

    ;/*
     ;* load up R5 with the buffer address minus
     ;* one cache block size
     ;*/
    LIS     R7 , HIADJ(PPC_E500_DCACHE_ALIGN_SIZE)
    LWZ     R7 , LO(PPC_E500_DCACHE_ALIGN_SIZE)(R7)
    LIS     R10 , HIADJ(_G_pucPpcCacheReadBuffer)
    LWZ     R10 , LO(_G_pucPpcCacheReadBuffer)(R10)

    SUBF    R5 , R7 , R10                                       ;/*  buf ptr to text - cache line size   */

LINE_LABEL(cacheE500DisableLoad2)
    ADD     R5 , R7 , R5                                        ;/*  + cache line size                   */
    LBZU    R10 , 0(R5)                                         ;/*  load the data cache with new block  */
    BDNZ    cacheE500DisableLoad2                               ;/*  loop till cache ctr is zero         */
    ISYNC

    MTSPR   CTR , R6                                            ;/*  load CTR with the number of index   */

    LIS     R10 , HIADJ(_G_pucPpcCacheReadBuffer)
    LWZ     R10 , LO(_G_pucPpcCacheReadBuffer)(R10)
    SUBF    R5 , R7 , R10                                       ;/*  buf ptr to text - cache line size   */

LINE_LABEL(cacheE500DisableFlush2)
    ADD     R5 , R7 , R5                                        ;/*  + cache line size                   */
    DCBF    0 , R5                                              ;/*  flush the data cache block          */
    BDNZ    cacheE500DisableFlush2                              ;/*  loop till cache ctr is zero         */

    B       ppcE500CacheRetOK                                   ;/*  return OK                           */

LINE_LABEL(cacheE500DisableLoadItlb2)
    B       cacheE500DisableLoadDtlb2
    FUNC_END()

;/*********************************************************************************************************
;  I-CACHE 所有数据无效
;*********************************************************************************************************/

FUNC_DEF(ppcE500ICacheInvalidateAll)
    MFSPR   R4 , L1CSR1
    ORI     R5 , R4 , ARCH_PPC_L1CSR_FI                         ;/*  flash invalidate and enable         */

    ;/* nothing reqd before updating L1CSR1 register */
    MTSPR   L1CSR1 , R5                                         ;/*  flash invalidate L1 instruction     */
                                                                ;/*  cache - 1 CPU cycle                 */
    B       ppcE500CacheRetOK                                   ;/*  return OK                           */
    FUNC_END()

;/*********************************************************************************************************
;  无效 ICACHE R3=void *start; R4=void *end; R5=cache line size
;*********************************************************************************************************/

FUNC_DEF(ppcE500ICacheInvalidate)
    ICBI    R0 , R3
    ADD     R3 , R3 , R5
    CMPLW   R3 , R4
    BLT     ppcE500ICacheInvalidate

    B       ppcE500CacheRetOK                                   ;/*  return OK                           */
    FUNC_END()

;/*********************************************************************************************************
;  将指定虚拟地址的 DCACHE 无效  R3=void *start; R4=void *end; R5=cache line size
;*********************************************************************************************************/

FUNC_DEF(ppcE500DCacheInvalidate)
    DCBI    R0 , R3
    ADD     R3 , R3 , R5
    CMPLW   R3 , R4
    BLT     ppcE500DCacheInvalidate

    B       ppcE500CacheRetOK                                   ;/*  return OK                           */
    FUNC_END()

;/*********************************************************************************************************
;  将指定虚拟地址的 DCACHE 回写并无效  R3=void *start; R4=void *end; R5=cache line size
;*********************************************************************************************************/

FUNC_DEF(ppcE500DCacheClear)
    DCBF    R0 , R3                                             ;/*  PPC "flush" == SylixOS "clear"      */
    ADD     R3 , R3 , R5
    CMPLW   R3 , R4
    BLT     ppcE500DCacheClear

    B       ppcE500CacheRetOK                                   ;/*  return OK                           */
    FUNC_END()

;/*********************************************************************************************************
;  将指定虚拟地址的 DCACHE 回写  R3=void *start; R4=void *end; R5=cache line size
;*********************************************************************************************************/

FUNC_DEF(ppcE500DCacheFlush)
    DCBST   R0 , R3                                             ;/*  PPC "store" == SylixOS "flush"      */
    ADD     R3 , R3 , R5
    CMPLW   R3 , R4
    BLT     ppcE500DCacheFlush

    B       ppcE500CacheRetOK                                   ;/*  return OK                           */
    FUNC_END()

;/*********************************************************************************************************
;  Text update
;*********************************************************************************************************/

FUNC_DEF(ppcE500TextUpdate)
    DCBST   R0 , R3
    SYNC
    ICBI    R0 , R3
    ADD     R3 , R3 , R6
    CMPLW   R3 , R4
    BLT     ppcE500TextUpdate

    B       ppcE500CacheRetOK                                   ;/*  return OK                           */
    FUNC_END()

;/*********************************************************************************************************
;  返回成功
;*********************************************************************************************************/

FUNC_DEF(ppcE500CacheRetOK)
    ISYNC
    SYNC
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  禁能分支预测
;*********************************************************************************************************/

FUNC_DEF(ppcE500BranchPredictionDisable)
    MFSPR   R3 ,    BUCSR
    ANDI.   R4 ,    R3 , LO(~ARCH_PPC_BUCSR_E)
    MTSPR   BUCSR , R4
    ISYNC
    MFSPR   R3 ,    BUCSR
    ANDI.   R4 ,    R3 , LO(~ARCH_PPC_BUCSR_FI)
    MTSPR   BUCSR , R4
    ISYNC
    ORI     R4 ,    R3 , LO(ARCH_PPC_BUCSR_FI)
    MTSPR   BUCSR , R4
    ISYNC
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  使能分支预测
;*********************************************************************************************************/

FUNC_DEF(ppcE500BranchPredictionEnable)
    MFSPR   R3 ,    BUCSR
    ANDI.   R4 ,    R3 , ARCH_PPC_BUCSR_E
    BNELR                                                   ;/*  return if already enabled               */
    ANDI.   R4 ,    R3 , LO(~ARCH_PPC_BUCSR_FI)
    MTSPR   BUCSR , R4
    ISYNC
    ORI     R4 ,    R3 , LO(ARCH_PPC_BUCSR_FI)
    MTSPR   BUCSR , R4
    ISYNC
    MFSPR   R3 ,    BUCSR
    ORI     R4 ,    R3 , LO(ARCH_PPC_BUCSR_E)
    MTSPR   BUCSR , R4
    ISYNC
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  无效分支预测
;*********************************************************************************************************/

FUNC_DEF(ppcE500BranchPredictorInvalidate)
    MFSPR   R3 ,    BUCSR
    ANDI.   R4 ,    R3 , LO(~ARCH_PPC_BUCSR_FI)
    MTSPR   BUCSR , R4
    ISYNC
    ORI     R4 ,    R3 , LO(ARCH_PPC_BUCSR_FI)
    MTSPR   BUCSR , R4
    ISYNC
    BLR
    FUNC_END()

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

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
;** 文   件   名: ppcL2Cache750Asm.s
;**
;** 创   建   人: Yang.HaiFeng (杨海峰)
;**
;** 文件创建日期: 2016 年 02 月 29 日
;**
;** 描        述: MPC750 体系构架 L2 CACHE 驱动.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/cache_cfg.h>

#if LW_CFG_CACHE_EN > 0 && LW_CFG_PPC_CACHE_L2 > 0

#include "./ppcL2Cache750.h"

    EXPORT_LABEL(ppc750L2CacheSetCR)
    EXPORT_LABEL(ppc750L2CacheGetCR)

    EXPORT_LABEL(ppc750L2CacheInit)

    EXPORT_LABEL(ppc750L2CacheEnable)
    EXPORT_LABEL(ppc750L2CacheDisable)
    EXPORT_LABEL(ppc750L2CacheIsEnable)

    EXPORT_LABEL(ppc750L2CacheInvalidateAll)

    EXPORT_LABEL(ppc750L2CacheFlushAllSW)
    EXPORT_LABEL(ppc750L2CacheFlushAllHW)
    EXPORT_LABEL(ppc750L2CacheSetWriteThrough)

    FILE_BEGIN()

;/*********************************************************************************************************
;  设置 L2CR 寄存器
;*********************************************************************************************************/

FUNC_DEF(ppc750L2CacheSetCR)
    MTSPR   L2CR_REG , R3
    BCLR    20 , 0
    FUNC_END()

;/*********************************************************************************************************
;  获取 L2CR 寄存器
;*********************************************************************************************************/

FUNC_DEF(ppc750L2CacheGetCR)
    MFSPR   R3 , L2CR_REG
    BCLR    20 , 0
    FUNC_END()

;/*********************************************************************************************************
;  L2 Cache 全局无效化操作
;*********************************************************************************************************/

FUNC_DEF(ppc750L2CacheInvalidateAll)
    MFSPR   R9 , PVR                                            ;/*  Read PVR                            */
    RLWINM  R9 , R9 , 16 , 16, 31                               ;/*  Shift down 16 bits                  */
    CMPLI   0  , 0  , R9, VGER                                  ;/*  Vger pvr                            */
    BEQ     invalidateMPC745X

    ISYNC

    SYNC
    MFSPR   R3 , L2CR_REG
    ANDIS.  R3 , R3 , L2CR_DISABLE_MASK_U                       ;/*  Disable L2 cache                    */
    MTSPR   L2CR_REG, R3

#ifdef __SYLIXOS_PPC745X__
    DSSALL                                                      ;/*  dssall                              */
                                                                ;/*  Stop all data stream.               */
#endif

    SYNC

    ORIS    R3 , R3 , L2CR_GBL_INV_U                            ;/*  Set global invalidate command       */
    MTSPR   L2CR_REG , R3
    SYNC

    ;/*
    ; * Monitoring the L2CR[L2IP] bit to determine when the global
    ; * invalidation operation is completed.
    ; */
LINE_LABEL(invalidateInProgress)
    MFSPR   R3 , L2CR_REG
    ANDI.   R3 , R3 , L2CR_IP
    BNE     invalidateInProgress

    SYNC
    MFSPR   R3 , L2CR_REG
    RLWINM  R3 , R3 , 0 , 11 , 9                                ;/*  Zero out the invalidate bit         */
    MTSPR   L2CR_REG, R3
    SYNC

    BLR

LINE_LABEL(invalidateMPC745X)
    ISYNC
    SYNC

    MFSPR   R3 , L2CR_REG
    ANDIS.  R4 , R3 , 0x8000
    BEQ     invalidateMPC745XDone

#ifdef __SYLIXOS_PPC745X__
    DSSALL                                                      ;/*  dssall                              */
                                                                ;/*  Stop all data stream.               */
#endif

    SYNC

    ORIS    R3 , R3 , L2CR_GBL_INV_U                            ;/*  Set global invalidate command       */
    MTSPR   L2CR_REG , R3
    SYNC

    ISYNC                                                       ;/*  Somerset test card code does this   */

LINE_LABEL(invalidateMPC745XWait)
    MFSPR   R5 , 1017
    SYNC
    MTCRF   0xFF , R5                                           ;/*  Is L2CR[L2I/10] still set?          */
    BC      0xC , 10 , invalidateMPC745XWait
    ISYNC                                                       ;/*  Somerset test card code does this   */
LINE_LABEL(invalidateMPC745XDone)
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  初始化 L2 Cache
;*********************************************************************************************************/

FUNC_DEF(ppc750L2CacheInit)
    OR      R6 , R3 , R3                                        ;/*  R6 暂存 R3 的参数(CR 的值)          */

    MFSPR   R9 , PVR                                            ;/*  Read PVR                            */
    RLWINM  R9 , R9 , 16 , 16 , 31                              ;/*  Shift down 16 bits                  */
    CMPLI   0  , 0 , R9 , VGER                                  ;/*  Vger pvr                            */
    BEQ     initMPC745X

    MFSPR   R3 , L2CR_REG
    ANDIS.  R4 , R3 , L2CR_EN_U
    BNE     l2EnableDone

    MFLR    R7

    SYNC
    ADDI    R3 , R0 , 0x0
    ORIS    R3 , R3 , L2CR_CLK_2
    MTSPR   L2CR_REG , R3
    SYNC

    BL      ppc750L2CacheInvalidateAll                          ;/*  该函数不会破坏 R6                   */
    SYNC
    MFSPR   R3 , L2CR_REG
    OR      R3 , R3 , R6                                        ;/*  或上 R6 的值                        */
    MTSPR   L2CR_REG , R3
    SYNC

    ISYNC

    MTLR    R7

LINE_LABEL(l2EnableDone)
    BLR

LINE_LABEL(initMPC745X)
    MFSPR   R3 , L2CR_REG
    ANDIS.  R4 , R3 , L2CR_EN_U
    BNE     l2EnableDone

    SYNC

    LIS     R5 , 0x8000
    ORI     R5 , R5 , 0x0000
    LIS     R4 , 0x7011
    ORI     R4 , R4 , 0x1000

    AND     R3 , R3 , R4
    MTSPR   L2CR_REG , R3

    SYNC

    MFLR    R7
    BL      ppc750L2CacheInvalidateAll

    ISYNC

    MTLR    R7

    BLR
    FUNC_END()

;/*********************************************************************************************************
;  使能 L2 Cache
;*********************************************************************************************************/

FUNC_DEF(ppc750L2CacheEnable)
    SYNC
    MFSPR   R4, L2CR_REG
    ISYNC
    ORIS    R4, R4, L2CR_EN_U
    MTSPR   L2CR_REG, R4
    ISYNC
    SYNC
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  禁能 L2 Cache
;*********************************************************************************************************/

FUNC_DEF(ppc750L2CacheDisable)
    SYNC
    MFSPR   R3 , L2CR_REG                                       ;/*  Read L2 control register            */
    ANDIS.  R3 , R3 , L2CR_DISABLE_MASK_U                       ;/*  Disable L2 backside                 */
    SYNC

    B       preFetchL2DisableCode

LINE_LABEL(codeIsPrefetched)
    MTSPR   L2CR_REG , R3
    SYNC
    ISYNC

    B       pastFetchL2DisableCode

LINE_LABEL(preFetchL2DisableCode)
    SYNC
    ISYNC
    B       codeIsPrefetched

LINE_LABEL(pastFetchL2DisableCode)
    BCLR    20 , 0
    FUNC_END()

;/*********************************************************************************************************
;  判断 L2 Cache 是否使能
;*********************************************************************************************************/

FUNC_DEF(ppc750L2CacheIsEnable)
    MFSPR   R4 , L2CR_REG
    ANDIS.  R4 , R4 , L2CR_EN_U
    BNE     returnTrue
    LI      R3 , 0
    SYNC
    BLR
LINE_LABEL(returnTrue)
    LI      R3 , 1
    SYNC
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  L2 Cache 软件 Flush 操作
;
;  VOID  ppc750L2CacheFlushAllSW(size_t  stSize, UINT8  *pucReadBuffer);
;*********************************************************************************************************/

FUNC_DEF(ppc750L2CacheFlushAllSW)
    SYNC
    MTSPR  CTR , R3
    SUBI   R4 , R4 , (L1CACHE_ALIGN_SIZE)
    ANDI.  R5 , R5 , 0x0

LINE_LABEL(loop)
    LBZU   R5 , L1CACHE_ALIGN_SIZE(R4)
    BDNZ   loop
    SYNC
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  L2 Cache 硬件 Flush 操作
;*********************************************************************************************************/

FUNC_DEF(ppc750L2CacheFlushAllHW)
    MFSPR   R4 , PVR                                            ;/*  Read PVR                            */
    SRAWI   R3 , R4 , 16                                        ;/*  Shift down 16 bits                  */

    CMPI    0 , 0 , R3 , 0x000C                                 ;/*  Max pvr                             */
    BEQ     flushL2Hw
    CMPI    0 , 0 , R3 , -32756                                 ;/*  0x800C Nitro pvr                    */
    BEQ     flushL2Hw
    CMPI    0 , 0 , R3 , -32768                                 ;/*  0x8000 Vger pvr                     */
    BEQ     flushL2HwVger
    CMPI    0 , 0 , R3 , -32767                                 ;/*  0x8001 Apollo pvr                   */
    BEQ     flushL2HwVger

LINE_LABEL(flushL2Hw)
    ISYNC

#ifdef __SYLIXOS_PPC745X__
    DSSALL                                                      ;/*  dssall                              */
                                                                ;/*  Stop all data stream.               */
#endif
    SYNC
    MFSPR   R3 , L2CR_REG
    ORI     R3 , R3 , L2CR_HWFLUSH
    MTSPR   L2CR_REG , R3
    SYNC
    BLR

    ;/* Flush the L2 for Vger using the hardware assist.  Vger is just
    ; * slightly different, of course.
    ; */
LINE_LABEL(flushL2HwVger)
    ISYNC

#ifdef __SYLIXOS_PPC745X__
    DSSALL                                                      ;/*  dssall                              */
                                                                ;/*  Stop all data stream.               */
#endif
    SYNC
    MFSPR   R3 , L2CR_REG
    SYNC

    ANDIS.  R4 , R3 , 0x8000                                    ;/*  If L2 is not on, dont               */
    BEQ     flushL2Done                                         ;/*  Flush                               */

    ORI     R3 , R3 , 0x0800                                    ;/*  Set the HWF bit.                    */

    SYNC
    MTSPR   L2CR_REG , R3
    SYNC

LINE_LABEL(flushL2Wait)
    MFSPR   R3 , L2CR_REG
    SYNC
    ANDI.   R3 , R3 , 0x0800                                    ;/*  L2HWF still set?                    */
    BNE     flushL2Wait

LINE_LABEL(flushL2Done)
    SYNC
    ISYNC

    BLR
    FUNC_END()

;/*********************************************************************************************************
;  设置 L2 Cache 为写穿透模式
;*********************************************************************************************************/

FUNC_DEF(ppc750L2CacheSetWriteThrough)
    ISYNC
    SYNC
    MFSPR   R3 , L2CR_REG
    ORIS    R3 , R3 , L2CR_WT
    MTSPR   L2CR_REG , R3
    SYNC
    BLR
    FUNC_END()

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

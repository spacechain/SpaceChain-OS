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
;** 文   件   名: armMmuCommon.S
;**
;** 创   建   人: Han.Hui (韩辉)
;**
;** 文件创建日期: 2013 年 12 月 09 日
;**
;** 描        述: ARM 体系架构 MMU 通用函数支持.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/vmm_cfg.h>

#if !defined(__SYLIXOS_ARM_ARCH_M__)

    FILE_BEGIN()

    EXPORT_LABEL(armMmuAbtFaultStatus)
    EXPORT_LABEL(armMmuPreFaultStatus)
    EXPORT_LABEL(armMmuAbtFaultAddr)

#if LW_CFG_VMM_EN > 0
    EXPORT_LABEL(armMmuInitSysRom)
    EXPORT_LABEL(armMmuEnable)
    EXPORT_LABEL(armMmuDisable)
    EXPORT_LABEL(armMmuEnableWriteBuffer)
    EXPORT_LABEL(armMmuDisableWriteBuffer)
    EXPORT_LABEL(armMmuEnableAlignFault)
    EXPORT_LABEL(armMmuDisableAlignFault)
    EXPORT_LABEL(armMmuSetDomain)
    EXPORT_LABEL(armMmuSetTTBase)
    EXPORT_LABEL(armMmuSetTTBase1)
    EXPORT_LABEL(armMmuInvalidateTLB)
    EXPORT_LABEL(armMmuInvalidateTLBMVA)
    EXPORT_LABEL(armMmuSetProcessId)
#endif
     
;/*********************************************************************************************************
;  获取错误信息
;*********************************************************************************************************/

FUNC_DEF(armMmuAbtFaultStatus)
    MRC     p15, 0, R0, c5, c0, 0
    BX      LR
    FUNC_END()
    
FUNC_DEF(armMmuPreFaultStatus)
    MRC     p15, 0, R0, c5, c0, 1
    BX      LR
    FUNC_END()
    
FUNC_DEF(armMmuAbtFaultAddr)
    MRC     p15, 0, R0, c6, c0, 0
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  初始化 SYS ROM
;*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

FUNC_DEF(armMmuInitSysRom)
    MRC     p15, 0, R0, c1, c0, 0
    BIC     R0, R0, #P15_R1_SYS
    ORR     R0, R0, #P15_R1_ROM
    MCR     p15, 0, R0, c1, c0, 0
    BX      LR
    FUNC_END()
    
;/*********************************************************************************************************
;  使能/禁能 MMU
;*********************************************************************************************************/

FUNC_DEF(armMmuEnable)
#if __SYLIXOS_ARM_ARCH__ >= 6
    ARM_DSB()
    ARM_ISB()
#else
    ARM_NOP(R0)                                                         ;/* 2 * NOP                      */
    ARM_NOP(R0)
#endif
    MRC     p15, 0, R0, c1, c0, 0
    ORR     R0, R0, #P15_R1_M
    MCR     p15, 0, R0, c1, c0, 0
#if __SYLIXOS_ARM_ARCH__ >= 6
    ARM_DSB()
    ARM_ISB()
#else
    ARM_NOP(R0)                                                         ;/* 2 * NOP                      */
    ARM_NOP(R0)
#endif
    BX      LR
    FUNC_END()
    
FUNC_DEF(armMmuDisable)
#if __SYLIXOS_ARM_ARCH__ >= 6
    ARM_DSB()
    ARM_ISB()
#else
    ARM_NOP(R0)                                                         ;/* 2 * NOP                      */
    ARM_NOP(R0)
#endif
    MRC     p15, 0, R0, c1, c0, 0
    BIC     R0, R0, #P15_R1_M
    MCR     p15, 0, R0, c1, c0, 0
#if __SYLIXOS_ARM_ARCH__ >= 6
    ARM_DSB()
    ARM_ISB()
#else
    ARM_NOP(R0)                                                         ;/* 2 * NOP                      */
    ARM_NOP(R0)
#endif
    BX      LR
    FUNC_END()
    
;/*********************************************************************************************************
;  使能/禁能 写缓冲
;*********************************************************************************************************/

FUNC_DEF(armMmuEnableWriteBuffer)
    MRC     p15, 0, R0, c1, c0, 0
    ORR     R0, R0, #P15_R1_W
    MCR     p15, 0, R0, c1, c0, 0
    BX      LR
    FUNC_END()
    
FUNC_DEF(armMmuDisableWriteBuffer)
    MRC     p15, 0, R0, c1, c0, 0
    BIC     R0, R0, #P15_R1_W
    MCR     p15, 0, R0, c1, c0, 0
    BX      LR
    FUNC_END()
    
;/*********************************************************************************************************
;  使能/禁能 地址对齐检测
;*********************************************************************************************************/

FUNC_DEF(armMmuEnableAlignFault)
    MRC     p15, 0, R0, c1, c0, 0
    ORR     R0, R0, #P15_R1_A
    MCR     p15, 0, R0, c1, c0, 0
    BX      LR
    FUNC_END()
    
FUNC_DEF(armMmuDisableAlignFault)
    MRC     p15, 0, R0, c1, c0, 0
    BIC     R0, R0, #P15_R1_A
    MCR     p15, 0, R0, c1, c0, 0
    BX      LR
    FUNC_END()
    
;/*********************************************************************************************************
;  设置域属性
;*********************************************************************************************************/

FUNC_DEF(armMmuSetDomain)
    MCR     p15, 0, R0, c3, c0, 0
    BX      LR
    FUNC_END()
    
;/*********************************************************************************************************
;  设置页表基地址 TTBR0 为进程相关页表  TTBR1 为系统全局页表
;*********************************************************************************************************/

FUNC_DEF(armMmuSetTTBase)
    ARM_DSB()
    ARM_ISB()
    MCR     p15, 0, R0, c2, c0, 0                                       ;/* Set TT Base                  */

#if __SYLIXOS_ARM_ARCH__ >= 6
    MOV     R0, #0
#if LW_CFG_SMP_EN > 0
    SMP_UP_CHECK(R1)
    MCRHI   p15, 0, R0, c7, c1, 6                                       ;/* BPIALLIS SMP                 */
    MCRLS   p15, 0, R0, c7, c5, 6                                       ;/* BPIALL UP                    */
#else
    MCR     p15, 0, R0, c7, c5, 6                                       ;/* BPIALL UP                    */
#endif
#endif
    ARM_NOP(R0)                                                         ;/* 3 * NOP                      */
    ARM_NOP(R0)
    ARM_NOP(R0)
    ARM_DSB()
    ARM_ISB()
    BX      LR
    FUNC_END()
    
FUNC_DEF(armMmuSetTTBase1)
    ARM_DSB()
    ARM_ISB()
    MCR     p15, 0, R0, c2, c0, 1

#if __SYLIXOS_ARM_ARCH__ >= 6
    MOV     R0, #0
#if LW_CFG_SMP_EN > 0
    SMP_UP_CHECK(R1)
    MCRHI   p15, 0, R0, c7, c1, 6                                       ;/* BPIALLIS SMP                 */
    MCRLS   p15, 0, R0, c7, c5, 6                                       ;/* BPIALL UP                    */
#else
    MCR     p15, 0, R0, c7, c5, 6                                       ;/* BPIALL UP                    */
#endif
#endif
    ARM_NOP(R0)                                                         ;/* 3 * NOP                      */
    ARM_NOP(R0)
    ARM_NOP(R0)
    ARM_DSB()
    ARM_ISB()
    BX      LR
    FUNC_END()
    
;/*********************************************************************************************************
;  TLB 相关操作
;*********************************************************************************************************/

FUNC_DEF(armMmuInvalidateTLB)                                           ;/* 快表中的所有项无效           */
    ARM_DSB()
    ARM_ISB()
    MOV     R0, #0
#if LW_CFG_SMP_EN > 0
    SMP_UP_CHECK(R1)
    MCRHI   p15, 0, R0, c8, c3, 0                                       ;/* TLBIALLIS SMP                */
    MCRLS   p15, 0, R0, c8, c7, 0                                       ;/* TLBIALL UP                   */
#if __SYLIXOS_ARM_ARCH__ >= 6
    MCRHI   p15, 0, R0, c7, c1, 6                                       ;/* BPIALLIS SMP                 */
    MCRLS   p15, 0, R0, c7, c5, 6                                       ;/* BPIALL UP                    */
#endif
#else
    MCR     p15, 0, R0, c8, c7, 0                                       ;/* TLBIALL UP                   */
#if __SYLIXOS_ARM_ARCH__ >= 6
    MCR     p15, 0, R0, c7, c5, 6                                       ;/* BPIALL UP                    */
#endif
#endif
    ARM_NOP(R0)                                                         ;/* 3 * NOP                      */
    ARM_NOP(R0)
    ARM_NOP(R0)
    ARM_DSB()
    ARM_ISB()
    BX      LR
    FUNC_END()
    
FUNC_DEF(armMmuInvalidateTLBMVA)                                        ;/* 无效快表指定地址变换条目     */
    ARM_DSB()
    ARM_ISB()
#if LW_CFG_SMP_EN > 0
    SMP_UP_CHECK(R1)
    MCRHI   p15, 0, R0, c8, c3, 1                                       ;/* TLBIMVAIS SMP                */
    MCRLS   p15, 0, R0, c8, c7, 1                                       ;/* TLBIMVA UP                   */
#if __SYLIXOS_ARM_ARCH__ >= 6
    MOV     R0, #0
    MCRHI   p15, 0, R0, c7, c1, 6                                       ;/* BPIALLIS SMP                 */
    MCRLS   p15, 0, R0, c7, c5, 6                                       ;/* BPIALL UP                    */
#endif
#else
    MCR     p15, 0, R0, c8, c7, 1                                       ;/* TLBIMVA UP                   */
#if __SYLIXOS_ARM_ARCH__ >= 6
    MOV     R0, #0
    MCR     p15, 0, R0, c7, c5, 6                                       ;/* BPIALL UP                    */
#endif
#endif
    ARM_NOP(R0)                                                         ;/* 3 * NOP                      */
    ARM_NOP(R0)
    ARM_NOP(R0)
    ARM_DSB()
    ARM_ISB()
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  设置进程 ID
;*********************************************************************************************************/

FUNC_DEF(armMmuSetProcessId)
    MCR     p15, 0, R0, c13, c0, 0
    BX      LR
    FUNC_END()

#endif

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

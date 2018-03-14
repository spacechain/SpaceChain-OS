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
;** 文   件   名: armVfpV3Asm.S
;**
;** 创   建   人: Han.Hui (韩辉)
;**
;** 文件创建日期: 2013 年 12 月 07 日
;**
;** 描        述: ARM 体系架构 VFPv3 支持.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if !defined(__SYLIXOS_ARM_ARCH_M__)

#if LW_CFG_CPU_FPU_EN > 0

    FILE_BEGIN()
    
    EXPORT_LABEL(armVfpV3Save16)
    EXPORT_LABEL(armVfpV3Restore16)
    EXPORT_LABEL(armVfpV3Save32)
    EXPORT_LABEL(armVfpV3Restore32)
    
;/*********************************************************************************************************
;  浮点运算器基本操作函数
;*********************************************************************************************************/
;/*********************************************************************************************************
;  VFP-D16
;   +-----------+
;   | freg[31]  |    + 0x98  <-- (r0 + 152)
;   |  ...      |
;   | freg[2]   |    + 0x24
;   | freg[1]   |    + 0x20
;   | freg[0]   |    + 0x1C  <-- (r0 + 28)
;   | mfvfr1    |    + 0x18
;   | mfvfr0    |    + 0x14
;   | fpinst2   |    + 0x10
;   | fpinst    |    + 0x0C
;   | fpexc     |    + 0x08
;   | fpscr     |    + 0x04
;   | fpsid     | <-- cpu_fpu_context ( = r0 )
;   +-----------+
;*********************************************************************************************************/

#define VFP_FPSID       0x00
#define VFP_FPSCR       0x04
#define VFP_FPEXC       0x08
#define VFP_FPINST      0x0C
#define VFP_FPINST2     0x10
#define VFP_MVVFR0      0x14
#define VFP_MVVFR1      0x18
#define VFP_GPR         0x1C

;/*********************************************************************************************************
;  VFPv3 恢复控制寄存器宏
;*********************************************************************************************************/

MACRO_DEF(VFPv3_SAVE_CTRL)
    FMRX    R1, FPSCR
    STR     R1, [R0, #VFP_FPSCR]        ;/*  保存 FPSCR                                                  */

    FMRX    R1, FPEXC
    STR     R1, [R0, #VFP_FPEXC]        ;/*  保存 FPEXC                                                  */

    ORR     R1, R1, #((1 << 28) | (1 << 31))
    FMXR    FPEXC, R1                   ;/*  Set FPEXC.EX=1 and FPEXC.FP2V=1                             */

    FMRX    R1, FPEXC                   ;/*  Read back the FPEXC register                                */
    TST     R1, #(1 << 31)              ;/*  if FPEXC.EX == 0 then                                       */
    BEQ     1f                          ;/*  Neither FPINST nor FPINST2 are implemented                  */

    FMRX    R2, FPINST                  ;/*  FPINST is implemented                                       */
    STR     R2, [R0, #VFP_FPINST]       ;/*  保存 FPINST                                                 */

    TST     R1, #(1 << 28)              ;/*  if FPEXC.FP2V == 0 then                                     */
    BEQ     1f                          ;/*  FPINST2 is not implemented                                  */

    FMRX    R3, FPINST2                 ;/*  FPINST2 is implemented                                      */
    STR     R3, [R0, #VFP_FPINST2]      ;/*  保存 FPINST2                                                */
1:
    ADD     R1, R0, #VFP_GPR
    MACRO_END()

;/*********************************************************************************************************
;  VFPv3 恢复控制寄存器宏
;*********************************************************************************************************/

MACRO_DEF(VFPv3_RESTORE_CTRL)
    FMRX    R1, FPEXC
    ORR     R1, R1, #((1 << 28) | (1 << 31))
    FMXR    FPEXC, R1                   ;/*  Set FPEXC.EX=1 and FPEXC.FP2V=1                             */

    FMRX    R1, FPEXC                   ;/*  Read back the FPEXC register                                */
    TST     R1, #(1 << 31)              ;/*  if FPEXC.EX == 0 then                                       */
    BEQ     1f                          ;/*  Neither FPINST nor FPINST2 are implemented                  */

    LDR     R2, [R0, #VFP_FPINST]       ;/*  FPINST is implemented                                       */
    FMXR    FPINST, R2                  ;/*  恢复 FPINST                                                 */

    TST     R1, #(1 << 28)              ;/*  if FPEXC.FP2V == 0 then                                     */
    BEQ     1f                          ;/*  FPINST2 is not implemented                                  */

    LDR     R3, [R0, #VFP_FPINST2]      ;/*  FPINST2 is implemented                                      */
    FMXR    FPINST2, R3                 ;/*  恢复 FPINST2                                                */
1:
    LDR     R1, [R0, #VFP_FPEXC]
    FMXR    FPEXC, R1                   ;/*  恢复 FPEXC                                                  */

    LDR     R1, [R0, #VFP_FPSCR]
    FMXR    FPSCR, R1                   ;/*  恢复 FPSCR                                                  */

    ADD     R1, R0, #VFP_GPR
    MACRO_END()

;/*********************************************************************************************************
;  VFPv3 保存/恢复寄存器
;*********************************************************************************************************/

FUNC_DEF(armVfpV3Save16)
    VFPv3_SAVE_CTRL
    FSTMIAD R1, {d0-d15}                ;/*  保存 D0-D15                                                 */
    BX      LR
    FUNC_END()

FUNC_DEF(armVfpV3Restore16)
    VFPv3_RESTORE_CTRL
    FLDMIAD R1, {d0-d15}                ;/*  恢复 D0-D15                                                */
    BX      LR
    FUNC_END()
    
;/*********************************************************************************************************
;  VFP-D32
;   +-----------+
;   | freg[63]  |    + 0x118 <-- (r0 + 280)
;   |  ...      |
;   | freg[31]  |    + 0x98  <-- (r0 + 152)
;   |  ...      |
;   | freg[2]   |    + 0x24
;   | freg[1]   |    + 0x20
;   | freg[0]   |    + 0x1C  <-- (r0 + 28)
;   | mfvfr1    |    + 0x18
;   | mfvfr0    |    + 0x14
;   | fpinst2   |    + 0x10
;   | fpinst    |    + 0x0C
;   | fpexc     |    + 0x08
;   | fpscr     |    + 0x04
;   | fpsid     | <-- cpu_fpu_context ( = r0 )
;   +-----------+
;*********************************************************************************************************/

FUNC_DEF(armVfpV3Save32)
    VFPv3_SAVE_CTRL
    FSTMIAD R1!, {d0-d15}               ;/*  保存 D0-D15                                                 */
    FSTMIAD R1,  {d16-d31}              ;/*  保存 D16-D31                                                */
    BX      LR
    FUNC_END()
    
FUNC_DEF(armVfpV3Restore32)
    VFPv3_RESTORE_CTRL
    FLDMIAD R1!, {d0-d15}               ;/*  恢复 D0-D15                                                 */
    FLDMIAD R1,  {d16-d31}              ;/*  恢复 D16-D31                                                */
    BX      LR
    FUNC_END()
    
    FILE_END()

#endif
#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

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
;** 文   件   名: ppcMmu603Asm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 03 月 12 日
;**
;** 描        述: PowerPC 603 体系构架 MMU 驱动.
;**
;** 说        明:
;** 参考自 <<e300 Power Architecture CoreFamily Reference Manual>>
;** 的章节 6.5.2.2.2 Code for Example Interrupt Handlers
;** 但它有几处错误:
;** 1. cmp c0, r1, r3 应该为 cmp  c0, 0, r1, r3
;** 2. srw r1, r1, 8  应该为 srwi r1, r1, 8
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/vmm_cfg.h>

#if LW_CFG_VMM_EN > 0

#include "arch/ppc/arch_603.h"

    FILE_BEGIN()

    IMPORT_LABEL(archInstructionStorageExceptionEntry)
    IMPORT_LABEL(archDataStorageExceptionEntry)

    EXPORT_LABEL(ppc603InstructionTranslationMissEntry)
    EXPORT_LABEL(ppc603DataLoadTranslationMissEntry)
    EXPORT_LABEL(ppc603DataStoreTranslationMissEntry)

;/*********************************************************************************************************
;  Instruction Translation Miss Entry
;
;  Instruction TB miss flow
;  Entry:
;  Vector = 1000
;  SRR0       -> address of instruction that missed
;  SRR1       -> 0:3=cr0 4=lru way bit 16:31 = saved MSR
;  MSR<TGPR>  -> 1
;  IMISS      -> ea that missed
;  ICMP       -> the compare value for the va that missed
;  HASH1      -> pointer to first hash pteg
;  hash2      -> pointer to second hash pteg
;
;  Register usage:
;   R0 is saved counter
;   R1 is junk
;   R2 is pointer to pteg
;   R3 is current compare value
;*********************************************************************************************************/

FUNC_DEF(ppc603InstructionTranslationMissEntry)
    MFSPR   R2 , HASH1                              ;/*  get first pointer                               */
    ADDI    R1 , 0 , 8                              ;/*  load 8 for counter                              */
    MFCTR   R0                                      ;/*  save counter                                    */
    MFSPR   R3 , ICMP                               ;/*  get first compare value                         */
    ADDI    R2 , R2 , -8                            ;/*  pre dec the pointer                             */

LINE_LABEL(im0)
    MTCTR   R1                                      ;/*  load counter                                    */

LINE_LABEL(im1)
    LWZU    R1 , 8(R2)                              ;/*  get next pte                                    */
    CMP     CR0 , 0 , R1 , R3                       ;/*  see if found pte                                */
    BDNZF   EQ , im1                                ;/*  dec count br if cmp ne and if count not zero    */
    BNE     instrSecHash                            ;/*  if not found set up second hash or exit         */
    L       R1 , +4(R2)                             ;/*  load tlb entry lower-word                       */
    ANDI.   R3 , R1 , 8                             ;/*  check G bit                                     */
    BNE     doISIp                                  ;/*  if guarded, take an ISI                         */
    MTCTR   R0                                      ;/*  restore counter                                 */
    MFSPR   R0 , IMISS                              ;/*  get the miss address for the tlbli              */
    MFSPR   R3 , SRR1                               ;/*  get the saved cr0 bits                          */
    MTCRF   0x80 , R3                               ;/*  restore CR0                                     */
    MTSPR   RPA , R1                                ;/*  set the pte                                     */
    ORI     R1 , R1 , 0x100                         ;/*  set reference bit                               */
    SRWI    R1 , R1 , 8                             ;/*  get byte 7 of pte                               */
    TLBLI   R0                                      ;/*  load the itlb                                   */
    STB     R1 , +6(R2)                             ;/*  update page table                               */
    RFI                                             ;/*  return to executing program                     */

;/*********************************************************************************************************
;  Register usage:
;   R0 is saved counter
;   R1 is junk
;   R2 is pointer to pteg
;   R3 is current compare value
;*********************************************************************************************************/

LINE_LABEL(instrSecHash)
    ANDI.   R1 , R3 , 0x0040                        ;/*  see if we have done second hash                 */
    BNE     doISI                                   ;/*  if so, go to ISI interrupt                      */
    MFSPR   R2 , HASH2                              ;/*  get the second pointer                          */
    ORI     R3 , R3 , 0x0040                        ;/*  change the compare value                        */
    ADDI    R1 , 0 , 8                              ;/*  load 8 for counter                              */
    ADDI    R2 , R2 , -8                            ;/*  pre dec for update on load                      */
    B       im0                                     ;/*  try second hash                                 */

;/*********************************************************************************************************
;  entry Not Found: synthesize an ISI interrupt
;  guarded memory protection violation: synthesize an ISI interrupt
;  Register usage:
;   R0 is saved counter
;   R1 is junk
;   R2 is pointer to pteg
;   R3 is current compare value
;*********************************************************************************************************/

LINE_LABEL(doISIp)
    MFSPR   R3 , SRR1                               ;/*  get srr1                                        */
    ANDI.   R2 , R3 , 0xffff                        ;/*  clean upper srr1                                */

    ADDIS   R2 , R2 , 0x0800                        ;/*  or in srr<4> = 1 to flag prot violation         */
    B       isi1

LINE_LABEL(doISI)
    MFSPR   R3 , SRR1                               ;/*  get srr1                                        */
    ANDI.   R2 , R3 , 0xffff                        ;/*  clean srr1                                      */
    ADDIS   R2 , R2 , 0x4000                        ;/*  or in srr1<1> = 1 to flag pte not found         */

LINE_LABEL(isi1)
    MTCTR   R0                                      ;/*  restore counter                                 */
    MTSPR   SRR1 , R2                               ;/*  set srr1                                        */
    MFMSR   R0                                      ;/*  get msr                                         */
    XORIS   R0 , R0 , 0x8000                        ;/*  flip the msr<tgpr> bit                          */
    MTCRF   0x80 , R3                               ;/*  restore CR0                                     */
    MTMSR   R0                                      ;/*  flip back to the native gprs                    */
    B       archInstructionStorageExceptionEntry    ;/*  go to instr. access interrupt                   */

    FUNC_END()

;/*********************************************************************************************************
;  Data Load Translation Miss Entry
;
;  Data TLB miss flow
;  Entry:
;  Vector = 1100
;  SRR0      -> address of load that caused the interrupt
;  SRR1      -> 0:3=cr0 4=lru way bit 5=1 16:31 = saved MSR
;  MSR<TGPR> -> 1
;  DMISS     -> ea that missed
;  DCMP      -> the compare value for the va that missed
;  HASH1     -> pointer to first hash pteg
;  HASH2     -> pointer to second hash pteg
;
;  Register usage:
;   R0 is saved counter
;   R1 is junk
;   R2 is pointer to pteg
;   R3 is current compare value
;*********************************************************************************************************/

FUNC_DEF(ppc603DataLoadTranslationMissEntry)

LINE_LABEL(tlbDataMiss)
    MFSPR   R2 , HASH1                              ;/*  get first pointer                               */
    ADDI    R1 , 0 , 8                              ;/*  load 8 for counter                              */
    MFCTR   R0                                      ;/*  save counter                                    */
    MFSPR   R3 , DCMP                               ;/*  get first compare value                         */
    ADDI    R2 , R2 , -8                            ;/*  pre dec the pointer                             */

LINE_LABEL(dm0)
    MTCTR   R1                                      ;/*  load counter                                    */

LINE_LABEL(dm1)
    LWZU    R1 , 8(R2)                              ;/*  get next pte                                    */
    CMP     CR0 , 0 , R1 , R3                       ;/*  see if found pte                                */
    BDNZF   EQ , dm1                                ;/*  dec count br if cmp ne and if count not zero    */
    BNE     dataSecHash                             ;/*  if not found set up second hash or exit         */
    L       R1 , +4(R2)                             ;/*  load tlb entry lower-word                       */
    MTCTR   R0                                      ;/*  restore counter                                 */
    MFSPR   R0 , DMISS                              ;/*  get the miss address for the tlbld              */
    MFSPR   R3 , SRR1                               ;/*  get the saved cr0 bits                          */
    MTCRF   0x80 , R3                               ;/*  restore CR0                                     */
    MTSPR   RPA , R1                                ;/*  set the pte                                     */
    ORI     R1 , R1 , 0x100                         ;/*  set reference bit                               */
    SRWI    R1 , R1 , 8                             ;/*  get byte 7 of pte                               */
    TLBLD   R0                                      ;/*  load the dtlb                                   */
    STB     R1 , +6(R2)                             ;/*  update page table                               */
    RFI                                             ;/*  return to executing program                     */

;/*********************************************************************************************************
;  Register usage:
;   R0 is saved counter
;   R1 is junk
;   R2 is pointer to pteg
;   R3 is current compare value
;*********************************************************************************************************/

LINE_LABEL(dataSecHash)
    ANDI.   R1 , R3 , 0x0040                        ;/*  see if we have done second hash                 */
    BNE     doDSI                                   ;/*  if so, go to DSI interrupt                      */
    MFSPR   R2 , HASH2                              ;/*  get the second pointer                          */
    ORI     R3 , R3 , 0x0040                        ;/*  change the compare value                        */
    ADDI    R1 , 0 , 8                              ;/*  load 8 for counter                              */
    ADDI    R2 , R2 , -8                            ;/*  pre dec for update on load                      */
    B       dm0                                     ;/*  try second hash                                 */

    FUNC_END()

;/*********************************************************************************************************
;  Data Store Translation Miss Entry
;
;  C=0 in dtlb and dtlb miss on store flow
;  Entry:
;  Vec = 1200
;  SRR0      -> address of store that caused the interrupt
;  SRR1      -> 0:3=cr0 4=lru way bit 5=1 16:31 = saved MSR
;  MSR<TGPR> -> 1
;  DMISS     -> ea that missed
;  DCMP      -> the compare value for the va that missed
;  HASH1     -> pointer to first hash pteg
;  HASH2     -> pointer to second hash pteg
;
;  Register usage:
;   R0 is saved counter
;   R1 is junk
;   R2 is pointer to pteg
;   R3 is current compare value
;*********************************************************************************************************/

FUNC_DEF(ppc603DataStoreTranslationMissEntry)

LINE_LABEL(tlbCeq0)
    MFSPR   R2 , HASH1                              ;/*  get first pointer                               */
    ADDI    R1 , 0 , 8                              ;/*  load 8 for counter                              */
    MFCTR   R0                                      ;/*  save counter                                    */
    MFSPR   R3 , DCMP                               ;/*  get first compare value                         */
    ADDI    R2 , R2 , -8                            ;/*  pre dec the pointer                             */

LINE_LABEL(ceq0)
    MTCTR   R1                                      ;/*  load counter                                    */

LINE_LABEL(ceq1)
    LWZU    R1 , 8(R2)                              ;/*  get next pte                                    */
    CMP     CR0 , 0 , R1 , R3                       ;/*  see if found pte                                */
    BDNZF   EQ , ceq1                               ;/*  dec count br if cmp ne and if count not zero    */
    BNE     cEq0SecHash                             ;/*  if not found set up second hash or exit         */
    L       R1 , +4(R2)                             ;/*  load tlb entry lower-word                       */
    ANDI.   R3 , R1 , 0x80                          ;/*  check the C-bit                                 */
    BEQ     cEq0ChkProt                             ;/*  if (C==0) go check protection modes             */

LINE_LABEL(ceq2)
    MTCTR   R0                                      ;/*  restore counter                                 */
    MFSPR   R0 , DMISS                              ;/*  get the miss address for the tlbld              */
    MFSPR   R3 , SRR1                               ;/*  get the saved cr0 bits                          */
    MTCRF   0x80 , R3                               ;/*  restore CR0                                     */
    MTSPR   RPA , R1                                ;/*  set the pte                                     */
    TLBLD   R0                                      ;/*  load the dtlb                                   */
    RFI                                             ;/*  return to executing program                     */

;/*********************************************************************************************************
;  Register usage:
;   R0 is saved counter
;   R1 is junk
;   R2 is pointer to pteg
;   R3 is current compare value
;*********************************************************************************************************/

LINE_LABEL(cEq0SecHash)
    ANDI.   R1 , R3 , 0x0040                        ;/*  see if we have done second hash                 */
    BNE     doDSI                                   ;/*  if so, go to DSI interrupt                      */
    MFSPR   R2 , HASH2                              ;/*  get the second pointer                          */
    ORI     R3 , R3 , 0x0040                        ;/*  change the compare value                        */
    ADDI    R1 , 0 , 8                              ;/*  load 8 for counter                              */
    ADDI    R2 , R2 , -8                            ;/*  pre dec for update on load                      */
    B       ceq0                                    ;/*  try second hash                                 */

;/*********************************************************************************************************
;  entry found and PTE(c-bit==0):
;  (check protection before setting PTE(c-bit)
;  Register usage:
;   R0 is saved counter
;   R1 is junk
;   R2 is pointer to pteg
;   R3 is current compare value
;*********************************************************************************************************/

LINE_LABEL(cEq0ChkProt)
    RLWINM. R3 , R1 , 30 , 0 , 1                    ;/*  test PP                                         */
    BGE-    chk0                                    ;/*  if (PP==00 or PP==01) goto chk0:                */
    ANDI.   R3 , R1 , 1                             ;/*  test PP[0]                                      */
    BEQ+    chk2                                    ;/*  return if PP[0]==0                              */
    B       doDSIp                                  ;/*  else DSIp                                       */

LINE_LABEL(chk0)
    MFSPR   R3 , SRR1                               ;/*  get old msr                                     */
    ANDIS.  R3 , R3 , 0x0008                        ;/*  test the KEY bit (SRR1-bit 12)                  */
    BEQ     chk2                                    ;/*  if (KEY==0) goto chk2:                          */
    B       doDSIp                                  ;/*  else DSIp                                       */

LINE_LABEL(chk2)
    ORI     R1 , R1 , 0x180                         ;/*  set reference and change bit                    */
    STH     R1 , 6(R2)                              ;/*  update page table                               */
    B       ceq2                                    ;/*  and back we go                                  */

;/*********************************************************************************************************
;  entry Not Found: synthesize a DSI interrupt
;  Register usage:
;   R0 is saved counter
;   R1 is junk
;   R2 is pointer to pteg
;   R3 is current compare value
;*********************************************************************************************************/

LINE_LABEL(doDSI)
    MFSPR   R3 , SRR1                               ;/*  get srr1                                        */
    RLWINM  R1 , R3 , 9 , 6 , 6                     ;/*  get srr1<flag> to bit 6 for load/store,zero rest*/
    ADDIS   R1 , R1 , 0x4000                        ;/*  or in dsisr<1> = 1 to flag pte not found        */
    B       dsi1

LINE_LABEL(doDSIp)
    MFSPR   R3 , SRR1                               ;/*  get srr1                                        */
    RLWINM  R1 , R3 , 9 , 6 , 6                     ;/*  get srr1<flag> to bit 6 for load/store,zero rest*/
    ADDIS   R1 , R1 , 0x0800                        ;/*  or in dsisr<4> = 1 to flag prot violation       */

LINE_LABEL(dsi1)
    MTCTR   R0                                      ;/*  restore counter                                 */
    ANDI.   R2 , R3 , 0xffff                        ;/*  clear upper bits of srr1                        */
    MTSPR   SRR1 , R2                               ;/*  set srr1                                        */
    MTSPR   DSISR , R1                              ;/*  load the dsisr                                  */
    MFSPR   R1 , DMISS                              ;/*  get miss address                                */
    RLWINM. R2 , R2 , 0 , 31 , 31                   ;/*  test LE bit                                     */
    BEQ     dsi2                                    ;/*  if little endian then:                          */
    XOR     R1 , R1 , 0x07                          ;/*  de-mung the data address                        */

LINE_LABEL(dsi2)
    MTSPR   DAR , R1                                ;/*  put in dar                                      */
    MFMSR   R0                                      ;/*  get msr                                         */
    XORIS   R0 , R0 , 0x2                           ;/*  flip the msr<tgpr> bit                          */
    MTCRF   0x80 , R3                               ;/*  restore CR0                                     */
    MTMSR   R0                                      ;/*  flip back to the native gprs                    */
    B       archDataStorageExceptionEntry           ;/*  branch to DSI interrupt                         */

    FUNC_END()

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

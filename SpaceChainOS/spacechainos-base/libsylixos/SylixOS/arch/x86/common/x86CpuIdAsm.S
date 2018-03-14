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
;** 文   件   名: x86CpuIdAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2016 年 07 月 04 日
;**
;** 描        述: x86 体系构架处理器 ID 探测.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include "x86CpuId.h"

    FILE_BEGIN()

    EXPORT_LABEL(x86CpuIdHWMTSupported)
    EXPORT_LABEL(x86CpuIdMaxNumLProcsPerPkg)
    EXPORT_LABEL(x86CpuIdMaxNumCoresPerPkg)
    EXPORT_LABEL(x86CpuIdInitialApicId)
    EXPORT_LABEL(x86CpuIdBitFieldWidth)
    EXPORT_LABEL(x86CpuIdBitFieldWidth)
    EXPORT_LABEL(x86CpuIdProbeHw)

;/*********************************************************************************************************
;  判断是否支持 hardware multi-threaded
;  BOOL  x86CpuIdHWMTSupported (VOID)
;*********************************************************************************************************/

FUNC_DEF(x86CpuIdHWMTSupported)
    PUSHL   %EBX                                        ;/*  Save EBX which is used by CPUID             */

    MOVL    $X86_CPUID_KEY1 , %EAX                      ;/*  Basic CPUID Information                     */
    CPUID                                               ;/*  Serializing instruction                     */

    MOVL    %EDX , %EAX                                 ;/*  Hardware multi-threaded support bit in EDX  */
    ANDL    $X86_CPUID_HWMT_BIT , %EAX                  ;/*  Return X86_CPUID_HWMT_BIT value             */

    TESTL   %EAX , %EAX
    JZ      __x86NoHWMT_0                               ;/*  Special case , no hw multi-threaded support */

    MOVL    $0x1 , %EAX                                 ;/*  Return LW_TRUE                              */
    POPL    %EBX                                        ;/*  Restore EBX                                 */
    RET

LINE_LABEL(__x86NoHWMT_0)
    MOVL    $0x0 , %EAX                                 ;/*  Return LW_FALSE                             */
    POPL    %EBX                                        ;/*  Restore EBX                                 */
    RET
    FUNC_END(x86CpuIdHWMTSupported)

;/*********************************************************************************************************
;  获得每个 Package 的逻辑处理器数目
;  UINT32  x86CpuIdMaxNumLProcsPerPkg (VOID)
;*********************************************************************************************************/

FUNC_DEF(x86CpuIdMaxNumLProcsPerPkg)
    PUSHL   %EBX                                        ;/*  Save EBX which is used by CPUID              */

    MOVL    $X86_CPUID_KEY1 , %EAX                      ;/*  Basic CPUID Information                      */
    CPUID                                               ;/*  Serializing instruction                      */

    MOVL    %EDX , %EAX                                 ;/*  Hardware multi-threaded support bit in EDX   */
    ANDL    $X86_CPUID_HWMT_BIT , %EAX                  ;/*  X86_CPUID_HWMT_BIT value                     */

    TESTL   %EAX , %EAX
    JZ      __x86NoHWMT_1                               ;/*  Special case , no hw multi-threaded support  */

    MOVL    %EBX , %EAX                                 ;/*  Max num of addressable IDs for LPs in package*/
    ANDL    $X86_CPUID_LOGICAL_BITS , %EAX              ;/*  Mask X86_CPUID_LOGICAL_BITS                  */
    RORL    $X86_CPUID_LOGICAL_SHIFT , %EAX             ;/*  Log processor per package                    */

    POPL    %EBX                                        ;/*  Restore EBX                                  */
    RET

LINE_LABEL(__x86NoHWMT_1)
    MOVL    $0x01 , %EAX                                ;/*  Return one logical processor per package     */
    POPL    %EBX                                        ;/*  Restore EBX                                  */
    RET
    FUNC_END(x86CpuIdMaxNumLProcsPerPkg)

;/*********************************************************************************************************
;  获得每个 Package 的 Cores 数目
;  UINT32  x86CpuIdMaxNumCoresPerPkg (VOID)
;*********************************************************************************************************/

FUNC_DEF(x86CpuIdMaxNumCoresPerPkg)
    PUSHL   %EBX                                        ;/*  Save EBX which is used by CPUID             */

    MOVL    $X86_CPUID_KEY1 , %EAX                      ;/*  Basic CPUID Information                     */
    CPUID                                               ;/*  Serializing instruction                     */

    MOVL    %EDX , %EAX                                 ;/*  Hardware multi-threaded support bit in EDX  */
    ANDL    $X86_CPUID_HWMT_BIT , %EAX                  ;/*  X86_CPUID_HWMT_BIT value                    */

    TESTL   %EAX , %EAX
    JZ      __x86NoHWMT_2                               ;/*  Special case , no hw multi-threaded support */

    MOVL    $X86_CPUID_KEY0 , %EAX                      ;/*  Basic CPUID Information                     */
    CPUID                                               ;/*  Serializing instruction                     */

    CMPL    $X86_CPUID_LEAF_4 , %EAX                    ;/*  Does CPUID support leaf 4                   */
    JL      __x86NoHWMT_2                               ;/*  If not , must be single core                */

    MOVL    $X86_CPUID_LEAF_4 , %EAX                    ;/*  Determine num cores                         */
    MOVL    $0 , %ECX                                   ;/*  Start with first level , index = 0          */
    CPUID                                               ;/*  Serializing instruction                     */

                                                        ;/*  EAX has info on number of cores             */
    ANDL    $X86_CPUID_CORE_BITS , %EAX                 ;/*  Mask X86_CPUID_CORE_BITS                    */
    RORL    $X86_CPUID_CORE_SHIFT , %EAX                ;/*  Max number cores per package                */
    INCL    %EAX                                        ;/*  Increment by 1                              */

    POPL    %EBX                                        ;/*  Restore EBX                                 */
    RET

LINE_LABEL(__x86NoHWMT_2)
    MOVL    $0x01 , %EAX                                ;/*  Must be a single core per package           */
    POPL    %EBX                                        ;/*  Restore EBX                                 */
    RET
    FUNC_END(x86CpuIdMaxNumCoresPerPkg)

;/*********************************************************************************************************
;  获得逻辑处理器的初始 APIC ID
;  UINT8  x86CpuIdInitialApicId (VOID)
;*********************************************************************************************************/

FUNC_DEF(x86CpuIdInitialApicId)
    PUSHL   %EBX                                        ;/*  Save EBX which is used by CPUID             */

    MOVL    $X86_CPUID_KEY1 , %EAX                      ;/*  Basic CPUID Information                     */
    CPUID                                               ;/*  Serializing instruction                     */

    MOVL    %EBX , %EAX                                 ;/*  Initial Apic Id info for logical processor  */
    ANDL    $X86_CPUID_APIC_ID_BITS , %EAX
    RORL    $X86_CPUID_APIC_ID_SHIFT , %EAX             ;/*  Logical processor Apic Id                   */

    POPL    %EBX                                        ;/*  Restore EBX                                 */
    RET
    FUNC_END(x86CpuIdInitialApicId)

;/*********************************************************************************************************
;  Get the width of a bit field based on the maximum number of unique identifiers
;  UINT32  x86CpuIdBitFieldWidth (UINT32 uiCountItem)
;*********************************************************************************************************/

FUNC_DEF(x86CpuIdBitFieldWidth)
    MOVL    X86_SP_ARG1(%ESP),%EDX                      ;/*  Get uiCountItem from stack                  */

    MOVL    $0 , %ECX
    MOVL    %ECX , %EAX                                 ;/*  Initialize width                            */

    DECL    %EDX
    BSR     %DX , %CX
    JZ      __x86Done

    INC     %CX
    MOVL    %ECX , %EAX                                 ;/*  Update width                                */

LINE_LABEL(__x86Done)
    RET
    FUNC_END(x86CpuIdBitFieldWidth)

;/*********************************************************************************************************
;  执行 CPUID 指令探测处理特性到 X86_CPUID 结构
;  UINT32  x86CpuIdProbeHw (X86_CPUID *)
;*********************************************************************************************************/

FUNC_DEF(x86CpuIdProbeHw)
    PUSHL   %EBP
    MOVL    %ESP , %EBP
    PUSHFL                                              ;/*  Save EFLAGS                                 */
    PUSHL   %EBX
    PUSHL   %ECX
    PUSHL   %EDX
    PUSHL   %ESI
    PUSHL   %EDI
    MOVL    8(%EBP) , %ESI                              ;/*  Pointer to Updated CPUID Structure          */
    CLI                                                 ;/*  Disable interrupt                           */

    ;/*
    ; * Execute CPUID to get vendor , family , model , stepping , features
    ; */

    ;/*
    ; * EAX=0 , Get the highest value and the vendor ID
    ; */
    MOVL    $0 , %EAX                                   ;/*  Set EAX 0                                   */
    CPUID                                               ;/*  Execute CPUID                               */

    ;/*
    ; * Check the vendor ID
    ; */
    CMPL    %EBX , _G_x86VendorIdIntel + 0              ;/*  Compare vendor id[0]                        */
    JNE     x86CpuProbeExit

    CMPL    %EDX , _G_x86VendorIdIntel + 4              ;/*  Compare vendor id[1]                        */
    JNE     x86CpuProbeExit

    CMPL    %ECX , _G_x86VendorIdIntel + 8              ;/*  Compare vendor id[2]                        */
    JNE     x86CpuProbeExit

    MOVL    %EAX , X86_CPUID_HIGHVALUE(%ESI)            ;/*  Save high value                             */
    MOVL    %EBX , X86_CPUID_VENDORID(%ESI)             ;/*  Save vendor id[0]                           */
    MOVL    %EDX , X86_CPUID_VENDORID + 4(%ESI)         ;/*  Save vendor id[1]                           */
    MOVL    %ECX , X86_CPUID_VENDORID + 8(%ESI)         ;/*  Save vendor id[2]                           */
    CMPL    $1 , %EAX                                   ;/*  Is CPUID(1) ok?                             */
    JL      x86CpuProbeExtended                         ;/*  No: extended probe                          */

    ;/*
    ; * EAX=1 , Get the processor signature and feature flags
    ; */
    MOVL    $1 , %EAX                                   ;/*  Set EAX 1                                   */
    CPUID                                               ;/*  Execute CPUID                               */
    MOVL    %EAX , X86_CPUID_SIGNATURE(%ESI)            ;/*  Save signature                              */
    MOVL    %EDX , X86_CPUID_FEATURES_EDX(%ESI)         ;/*  Save feature EDX                            */
    MOVL    %ECX , X86_CPUID_FEATURES_ECX(%ESI)         ;/*  Save feature ECX                            */
    MOVL    %EBX , X86_CPUID_FEATURES_EBX(%ESI)         ;/*  Save feature EBX                            */
    CMPL    $2 , X86_CPUID_HIGHVALUE(%ESI)              ;/*  Is CPUID(2) ok?                             */
    JL      x86CpuProbeExtended                         ;/*  No: extended probe                          */

    ;/*
    ; * EAX=2 , Get the cache descriptors
    ; */
    MOVL    $0 , %EDI                                   ;/*  Set to zero                                 */
    MOVL    $2 , %EAX                                   ;/*  Set EAX 2                                   */
    CPUID                                               ;/*  Execute CPUID                               */

    ;/*
    ; * The lower 8 bits of EAX contain the value that identifies
    ; * the number of times CPUID must be executed in order to obtain
    ; * a complete image of the processor's caching systems.
    ; */
    MOVL    %EAX , %EDI                                 ;/*  Store counter value                         */
    CMP     $1 , %AL                                    ;/*  Is count > 1?                               */
    JLE     x86CpuProbeDecode                           ;/*  Yes , continue CPUID execution              */
    SUB     $1 , %DI                                    ;/*  Already executed once , so decrement count  */

LINE_LABEL(x86CpuProbeRep)
    CPUID                                               ;/*  Execute CPUID                               */
    SUB     $1 ,  %DI                                   ;/*  Decrement count                             */
    TEST    %DI , %DI                                   ;/*  Is count 0?                                 */
    JNZ     x86CpuProbeRep                              ;/*  No , continue execution until count==0      */

LINE_LABEL(x86CpuProbeDecode)
    MOVL    %EAX , X86_CPUID_CACHE_EAX(%ESI)            ;/*  Save config EAX                             */
    MOVL    %EBX , X86_CPUID_CACHE_EBX(%ESI)            ;/*  Save config EBX                             */
    MOVL    %ECX , X86_CPUID_CACHE_ECX(%ESI)            ;/*  Save config ECX                             */
    MOVL    %EDX , X86_CPUID_CACHE_EDX(%ESI)            ;/*  Save config EDX                             */
    CMPL    $3 , X86_CPUID_HIGHVALUE(%ESI)              ;/*  Is CPUID(3) ok?                             */
    JL      x86CpuProbeExtended                         ;/*  No: extended probe                          */

    ;/*
    ; * EAX=3 , processor serial number
    ; */
    MOVL    $3 , %EAX                                   ;/*  Set EAX 3                                   */
    CPUID                                               ;/*  Execute CPUID                               */
    MOVL    %EDX , X86_CPUID_SERIALNO(%ESI)             ;/*  Save serialno[2]                            */
    MOVL    %ECX , X86_CPUID_SERIALNO + 4(%ESI)         ;/*  Save serialno[3]                            */
    CMPL    $4 , X86_CPUID_HIGHVALUE(%ESI)              ;/*  Is CPUID(4) ok?                             */
    JL      x86CpuProbeExtended                         ;/*  No: extended probe                          */

    ;/*
    ; * EAX=4 , get deterministic cache parameters
    ; */
    MOVL    $0 , %EDI                                   ;/*  Set to zero                                 */
    ADDL    $X86_CPUID_CACHE_PARAMS , %ESI

LINE_LABEL(x86CpuProbeCache)
    ;/*
    ; * Execute until EAX[4:0] == 0 , incrementing ECX
    ; */
    MOVL    $4 ,   %EAX                                 ;/*  Set EAX 4                                   */
    MOVL    %EDI , %ECX                                 ;/*  Set ECX                                     */
    CPUID                                               ;/*  Execute CPUID                               */

    ;/*
    ; * Store cache parameters
    ; */
    MOVL    %EAX ,   (%ESI)                             ;/*  Save parameters EAX                         */
    MOVL    %EBX ,  4(%ESI)                             ;/*  Save parameters EBX                         */
    MOVL    %ECX ,  8(%ESI)                             ;/*  Save parameters ECX                         */
    MOVL    %EDX , 12(%ESI)                             ;/*  Save parameters EDX                         */

    ;/*
    ; * Check for valid cache info
    ; */
    ANDL    $X86_CPUID_CACHE_TYPE , %EAX
    JZ      x86CpuProbeCacheEnd

    ADDL    $1 ,   %EDI

    ;/*
    ; * Only storage for 4 caches are supported
    ; */
    CMPL    $4 ,   %EDI
    JE      x86CpuProbeCacheEnd

    ADDL    $16 ,  %ESI
    JMP     x86CpuProbeCache

LINE_LABEL(x86CpuProbeCacheEnd)
    ;/*
    ; * Restore CPUID struct pointer
    ; */
    MOVL    8(%EBP) , %ESI

    MOVL    %EDI , X86_CPUID_CACHE_COUNT(%ESI)

    CMPL    $5 , X86_CPUID_HIGHVALUE(%ESI)              ;/*  Is CPUID(5) ok?                             */
    JL      x86CpuProbeExtended                         ;/*  No: extended probe                          */

    ;/*
    ; * EAX=5 , MONITOR/MWAIT parameters
    ; */
    ;/*
    ; * If monitor/mwait is supported , execute query
    ; */
    TESTL   $X86_CPUID_MON , X86_CPUID_FEATURES_ECX(%ESI)
    JZ      x86CpuProbeSensor

    MOVL    $0x5  , %EAX                                ;/*  Set EAX 0x5                                 */
    CPUID                                               ;/*  Execute CPUID                               */

    MOVL    %EAX , X86_CPUID_MONITOR_EAX(%ESI)          ;/*  Save config EAX                             */
    MOVL    %EBX , X86_CPUID_MONITOR_EBX(%ESI)          ;/*  Save config EBX                             */
    MOVL    %ECX , X86_CPUID_MONITOR_ECX(%ESI)          ;/*  Save config ECX                             */
    MOVL    %EDX , X86_CPUID_MONITOR_EDX(%ESI)          ;/*  Save config EDX                             */

    CMPL    $6 , X86_CPUID_HIGHVALUE(%ESI)              ;/*  Is CPUID(6) ok?                             */
    JL      x86CpuProbeExtended                         ;/*  No: extended probe                          */

    ;/*
    ; * EAX=6 , Digital Thermal Sensor and Power Management parameters
    ; */
LINE_LABEL(x86CpuProbeSensor)
    MOVL    $0x6  , %EAX                                ;/*  Set EAX 0x6                                 */
    CPUID                                               ;/*  Execute CPUID                               */

    MOVL    %EAX , X86_CPUID_DTSPM_EAX(%ESI)            ;/*  Save config EAX                             */
    MOVL    %EBX , X86_CPUID_DTSPM_EBX(%ESI)            ;/*  Save config EBX                             */
    MOVL    %ECX , X86_CPUID_DTSPM_ECX(%ESI)            ;/*  Save config ECX                             */
    MOVL    %EDX , X86_CPUID_DTSPM_EDX(%ESI)            ;/*  Save config EDX                             */

    CMPL    $9 , X86_CPUID_HIGHVALUE(%ESI)              ;/*  Is CPUID(9) ok?                             */
    JL      x86CpuProbeExtended                         ;/*  No: extended probe                          */

    ;/*
    ; * EAX=9 , Direct Cache Access (DCA) parameters
    ; */
    MOVL    $0x9  , %EAX                                ;/*  Set EAX 0x9                                 */
    CPUID                                               ;/*  Execute CPUID                               */
    MOVL    %EAX , X86_CPUID_DCA_EAX(%ESI)              ;/*  Save config EAX                             */

    CMPL    $0xa , X86_CPUID_HIGHVALUE(%ESI)            ;/*  Is CPUID(10) ok?                            */
    JL      x86CpuProbeExtended                         ;/*  No: extended probe                          */

    ;/*
    ; * EAX=0xa , Performance Monitor Features
    ; */
    MOVL    $0xa  , %EAX                                ;/*  Set EAX 0xA                                 */
    CPUID                                               ;/*  Execute CPUID                               */

    MOVL    %EAX , X86_CPUID_PMON_EAX(%ESI)             ;/*  Save config EAX                             */
    MOVL    %EBX , X86_CPUID_PMON_EBX(%ESI)             ;/*  Save config EBX                             */
    MOVL    %ECX , X86_CPUID_PMON_ECX(%ESI)             ;/*  Save config ECX                             */
    MOVL    %EDX , X86_CPUID_PMON_EDX(%ESI)             ;/*  Save config EDX                             */

    CMPL    $0xb , X86_CPUID_HIGHVALUE(%ESI)            ;/*  Is CPUID(11) ok?                            */
    JL      x86CpuProbeExtended                         ;/*  No: extended probe                          */

    ;/*
    ; * EAX=0xb , x2APIC Features / Processor Topology
    ; */
    MOVL    $0 , %EDI                                   ;/*  Set to zero                                 */
    ADDL    $X86_CPUID_PTOP_PARAMS , %ESI

LINE_LABEL(x86CpuProbeTop)
    ;/*
    ; * Execute until EAX = EBX = 0 , incrementing ECX
    ; */
    MOVL    $0xb , %EAX                                 ;/*  Set EAX 0xB                                 */
    MOVL    %EDI , %ECX                                 ;/*  Set ECX                                     */
    CPUID                                               ;/*  Execute CPUID                               */

    ;/*
    ; * Store features
    ; */
    MOVL    %EAX ,   (%ESI)                             ;/*  Save parameters EAX                         */
    MOVL    %EBX ,  4(%ESI)                             ;/*  Save parameters EBX                         */
    MOVL    %ECX ,  8(%ESI)                             ;/*  Save parameters ECX                         */
    MOVL    %EDX , 12(%ESI)                             ;/*  Save parameters EDX                         */

    ;/*
    ; * Increment storage pointer
    ; */
    ADDL    $16 ,  %ESI

    ;/*
    ; * Increment ECX counter
    ; */
    ADDL    $1 ,   %EDI

    ;/*
    ; * Only storage for n cores are supported
    ; */
    CMPL    $32 ,   %EDI
    JE      x86CpuProbeTopEnd

    ;/*
    ; * Check for exit case
    ; */
    CMPL    $0 ,  %EAX
    JNE     x86CpuProbeTop

    CMPL    $0 ,  %EBX
    JNE     x86CpuProbeTop

LINE_LABEL(x86CpuProbeTopEnd)
    ;/*
    ; * Restore CPUID struct pointer
    ; */
    MOVL    8(%EBP) , %ESI

    MOVL    %EDI , X86_CPUID_PTOP_COUNT(%ESI)

    CMPL    $0xd , X86_CPUID_HIGHVALUE(%ESI)            ;/*  Is CPUID(13) ok?                            */
    JL      x86CpuProbeExtended                         ;/*  No: extended probe                          */

    ;/*
    ; * EAX=0xd , XSAVE Features
    ; */
    MOVL    $0xd  , %EAX                                ;/*  Set EAX 0xD                                 */
    MOVL    $0x0  , %ECX                                ;/*  Set ECX 0x0                                 */
    CPUID                                               ;/*  Execute CPUID                               */
    MOVL    %EAX , X86_CPUID_XSAVE_EAX(%ESI)            ;/*  Save config EAX                             */
    MOVL    %EBX , X86_CPUID_XSAVE_EBX(%ESI)            ;/*  Save config EBX                             */
    MOVL    %ECX , X86_CPUID_XSAVE_ECX(%ESI)            ;/*  Save config ECX                             */
    MOVL    %EDX , X86_CPUID_XSAVE_EDX(%ESI)            ;/*  Save config EDX                             */

LINE_LABEL(x86CpuProbeExtended)
    ;/*
    ; * EAX=0x80000000 , Get the highest value and Brand String if supported
    ; */
    MOVL    $0x80000000 , %EAX                          ;/*  Set EAX 0x80000000                          */
    CPUID                                               ;/*  Execute CPUID                               */

    MOVL    %EAX , X86_CPUID_HIGHVALUE_EXT(%ESI)        ;/*  Save high value                             */
    CMPL    $0x80000001 , %EAX                          ;/*  Is CPUID(0x80000001) ok?                    */
    JL      x86CpuProbeExit                             ;/*  No: end probe                               */

    ;/*
    ; * EAX=0x8000001 , Get extended Features
    ; */
    MOVL    $0x80000001 , %EAX                          ;/*  Set EAX 0x80000001                          */
    CPUID                                               ;/*  Execute CPUID                               */

    MOVL    %EDX , X86_CPUID_EXT_FEATURES_EDX(%ESI)     ;/*  Save feature EDX                            */
    MOVL    %ECX , X86_CPUID_EXT_FEATURES_ECX(%ESI)     ;/*  Save feature ECX                            */
    CMPL    $0x80000002 , X86_CPUID_HIGHVALUE_EXT(%ESI) ;/*  Is CPUID(0x80000002) ok?                    */
    JL      x86CpuProbeExit                             ;/*  No: end probe                               */

    ;/*
    ; * EAX=0x8000000[234] , Get the Brand String
    ; */
    MOVL    $0x80000002 , %EAX                          ;/*  Set EAX 0x80000002                          */
    CPUID                                               ;/*  Execute CPUID                               */

    MOVL    %EAX , X86_CPUID_BRAND_STR(%ESI)            ;/*  Save brandStr[0]                            */
    MOVL    %EBX , X86_CPUID_BRAND_STR + 4(%ESI)        ;/*  Save brandStr[1]                            */
    MOVL    %ECX , X86_CPUID_BRAND_STR + 8(%ESI)        ;/*  Save brandStr[2]                            */
    MOVL    %EDX , X86_CPUID_BRAND_STR + 12(%ESI)       ;/*  Save brandStr[3]                            */
    CMPL    $0x80000003 , X86_CPUID_HIGHVALUE_EXT(%ESI) ;/*  Is CPUID(0x80000003) ok?                    */
    JL      x86CpuProbeExit                             ;/*  No: end probe                               */

    MOVL    $0x80000003 , %EAX                          ;/*  Set EAX 0x80000003                          */
    CPUID                                               ;/*  Execute CPUID                               */

    MOVL    %EAX , X86_CPUID_BRAND_STR + 16(%ESI)       ;/*  Save brandStr[4]                            */
    MOVL    %EBX , X86_CPUID_BRAND_STR + 20(%ESI)       ;/*  Save brandStr[5]                            */
    MOVL    %ECX , X86_CPUID_BRAND_STR + 24(%ESI)       ;/*  Save brandStr[6]                            */
    MOVL    %EDX , X86_CPUID_BRAND_STR + 28(%ESI)       ;/*  Save brandStr[7]                            */
    CMPL    $0x80000004 , X86_CPUID_HIGHVALUE_EXT(%ESI) ;/*  Is CPUID(0x80000004) ok?                    */
    JL      x86CpuProbeExit                             ;/*  No: end probe                               */

    MOVL    $0x80000004 , %EAX                          ;/*  Set EAX 0x80000004                          */
    CPUID                                               ;/*  Execute CPUID                               */

    MOVL    %EAX , X86_CPUID_BRAND_STR + 32(%ESI)       ;/*  Save brandStr[8]                            */
    MOVL    %EBX , X86_CPUID_BRAND_STR + 36(%ESI)       ;/*  Save brandStr[9]                            */
    MOVL    %ECX , X86_CPUID_BRAND_STR + 40(%ESI)       ;/*  Save brandStr[10]                           */
    MOVL    %EDX , X86_CPUID_BRAND_STR + 44(%ESI)       ;/*  Save brandStr[11]                           */

    CMPL    $0x80000006 , X86_CPUID_HIGHVALUE_EXT(%ESI) ;/*  Is CPUID(0x80000006) ok?                    */
    JL      x86CpuProbeExit                             ;/*  No: end probe                               */

    ;/*
    ; * EAX=0x80000006 L2 Cache Features
    ; */
    MOVL    $0x80000006 , %EAX                          ;/*  Set EAX 0x80000006                          */
    CPUID                                               ;/*  Execute CPUID                               */

    MOVL    %EAX , X86_CPUID_L2CACHE_EAX(%ESI)          ;/*  Save feature EAX                            */
    MOVL    %EBX , X86_CPUID_L2CACHE_EBX(%ESI)          ;/*  Save feature EBX                            */
    MOVL    %ECX , X86_CPUID_L2CACHE_ECX(%ESI)          ;/*  Save feature ECX                            */
    MOVL    %EDX , X86_CPUID_L2CACHE_EDX(%ESI)          ;/*  Save feature EDX                            */

    CMPL    $0x80000007 , X86_CPUID_HIGHVALUE_EXT(%ESI) ;/*  Is CPUID(0x80000007) ok?                    */
    JL      x86CpuProbeExit                             ;/*  No: end probe                               */

    ;/*
    ; * EAX=0x80000007 APM Features
    ; */
    MOVL    $0x80000007 , %EAX                          ;/*  Set EAX 0x80000007                          */
    CPUID                                               ;/*  Execute CPUID                               */

    MOVL    %EAX , X86_CPUID_APM_EAX(%ESI)              ;/*  Save feature EAX                            */
    MOVL    %EBX , X86_CPUID_APM_EBX(%ESI)              ;/*  Save feature EBX                            */
    MOVL    %ECX , X86_CPUID_APM_ECX(%ESI)              ;/*  Save feature ECX                            */
    MOVL    %EDX , X86_CPUID_APM_EDX(%ESI)              ;/*  Save feature EDX                            */

    CMPL    $0x80000008 , X86_CPUID_HIGHVALUE_EXT(%ESI) ;/*  Is CPUID(0x80000008) ok?                    */
    JL      x86CpuProbeExit                             ;/*  No: end probe                               */

    ;/*
    ; * EAX=0x80000008 Virtual/Phys address size
    ; */
    MOVL    $0x80000008 , %EAX                          ;/*  Set EAX 0x80000008                          */
    CPUID                                               ;/*  Execute CPUID                               */

    MOVL    %EAX , X86_CPUID_VPADRS_EAX(%ESI)           ;/*  Save feature EAX                            */
    MOVL    %EBX , X86_CPUID_VPADRS_EBX(%ESI)           ;/*  Save feature EBX                            */
    MOVL    %ECX , X86_CPUID_VPADRS_ECX(%ESI)           ;/*  Save feature ECX                            */
    MOVL    %EDX , X86_CPUID_VPADRS_EDX(%ESI)           ;/*  Save feature EDX                            */

LINE_LABEL(x86CpuProbeExit)
    POPL    %EDI
    POPL    %ESI
    POPL    %EDX
    POPL    %ECX
    POPL    %EBX
    POPFL                                               ;/*  Restore EFLAGS                              */
    LEAVE
    RET
    FUNC_END(x86CpuIdProbeHw)

;/*********************************************************************************************************
;  CPUID vendor ID - Intel
;*********************************************************************************************************/

FUNC_LABEL(_G_x86VendorIdIntel)
    .ascii  "GenuineIntel"

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

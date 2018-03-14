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
;** 文   件   名: sparcVectorAsm.S
;**
;** 创   建   人: Xu.Guizhou (徐贵洲)
;**
;** 文件创建日期: 2017 年 05 月 15 日
;**
;** 描        述: SPARC 体系构架异常向量表.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/kernel_cfg.h>
#include <config/mp/mp_cfg.h>

    FILE_BEGIN()

    IMPORT_LABEL(reset)
    IMPORT_LABEL(archDataStoreErrEntry)
    IMPORT_LABEL(archInstAccessMmuMissEntry)
    IMPORT_LABEL(archInstAccessErrEntry)
    IMPORT_LABEL(archRRegAccessErrEntry)
    IMPORT_LABEL(archInstAccessExcEntry)
    IMPORT_LABEL(archPrivInstEntry)
    IMPORT_LABEL(archIllegalInstEntry)
    IMPORT_LABEL(archFpDisableEntry)
    IMPORT_LABEL(archCpDisableEntry)
    IMPORT_LABEL(archUnimplFlushEntry)
    IMPORT_LABEL(archWatchPointDectectEntry)
    IMPORT_LABEL(archMemAddrNoAlignEntry)
    IMPORT_LABEL(archFpExcEntry)
    IMPORT_LABEL(archCpExcEntry)
    IMPORT_LABEL(archDataAccessErrEntry)
    IMPORT_LABEL(archDataAccessMmuMissEntry)
    IMPORT_LABEL(archDataAccessExcEntry)
    IMPORT_LABEL(archTagOverFlowEntry)
    IMPORT_LABEL(archDivZeroEntry)
    IMPORT_LABEL(archTrapInstEntry)
    IMPORT_LABEL(archIntEntry)
    IMPORT_LABEL(archWinOverFlowEntry)
    IMPORT_LABEL(archWinUnderFlowEntry)
    IMPORT_LABEL(archWinFlushTrapEntry)

    EXPORT_LABEL(vector)

;/*********************************************************************************************************
;  这里的异常向量表 vector 声明为弱符号, BSP 可以重新实现异常向量表
;*********************************************************************************************************/

    WEAK(vector)

;/*********************************************************************************************************
;  异常向量表
;*********************************************************************************************************/

    SECTION(.vector)

    .balign 4096, 0

FUNC_DEF(vector)
    RTRAP(0x00, reset );                            ;/*  00 reset trap                                   */
    TRAP( 0x01, archInstAccessExcEntry );           ;/*  01 instruction access exception                 */
    TRAP( 0x02, archIllegalInstEntry );             ;/*  02 illegal instruction                          */
    TRAP( 0x03, archPrivInstEntry );                ;/*  03 privileged instruction                       */
    TRAP( 0x04, archFpDisableEntry );               ;/*  04 fp disabled                                  */
    TRAP( 0x05, archWinOverFlowEntry );             ;/*  05 window overflow                              */
    TRAP( 0x06, archWinUnderFlowEntry );            ;/*  06 window underflow                             */
    TRAP( 0x07, archMemAddrNoAlignEntry );          ;/*  07 memory address not aligned                   */
    TRAP( 0x08, archFpExcEntry );                   ;/*  08 fp exception                                 */
    TRAP( 0x09, archDataAccessExcEntry );           ;/*  09 data access exception                        */
    TRAP( 0x0a, archTagOverFlowEntry );             ;/*  0A tag overflow                                 */
    TRAP( 0x0b, archWatchPointDectectEntry );       ;/*  0B watchpoint_detected                          */
    BAD_TRAP;                                       ;/*  0C undefined                                    */
    BAD_TRAP;                                       ;/*  0D undefined                                    */
    BAD_TRAP;                                       ;/*  0E undefined                                    */
    BAD_TRAP;                                       ;/*  0F undefined                                    */
    BAD_TRAP;                                       ;/*  10 undefined                                    */

    ;/*
    ; *  interrupt_level_n traps
    ; */
    TRAP( 0x11, archIntEntry );
    TRAP( 0x12, archIntEntry );
    TRAP( 0x13, archIntEntry );
    TRAP( 0x14, archIntEntry );
    TRAP( 0x15, archIntEntry );
    TRAP( 0x16, archIntEntry );
    TRAP( 0x17, archIntEntry );
    TRAP( 0x18, archIntEntry );
    TRAP( 0x19, archIntEntry );
    TRAP( 0x1a, archIntEntry );
    TRAP( 0x1b, archIntEntry );
    TRAP( 0x1c, archIntEntry );
    TRAP( 0x1d, archIntEntry );
    TRAP( 0x1e, archIntEntry );
    TRAP( 0x1f, archIntEntry );

    TRAP( 0x20, archRRegAccessErrEntry );           ;/*  20 r_register_access_error                      */
                                                    ;/*  register file EDAC error (LEON-FT only!)        */
    TRAP( 0x21, archInstAccessErrEntry );           ;/*  21 instruction_access_error                     */
    BAD_TRAP;                                       ;/*  22 undefined                                    */
    BAD_TRAP;                                       ;/*  23 undefined                                    */
    TRAP( 0x24, archCpDisableEntry );               ;/*  24 cp_disabled                                  */
    TRAP( 0x25, archUnimplFlushEntry );             ;/*  25 unimplemented_FLUSH                          */
    BAD_TRAP;                                       ;/*  26 undefined                                    */
    BAD_TRAP;                                       ;/*  27 undefined                                    */
    TRAP( 0x28, archCpExcEntry );                   ;/*  28 cp_exception                                 */
    TRAP( 0x29, archDataAccessErrEntry );           ;/*  29 data_access_error                            */
    TRAP( 0x2a, archDivZeroEntry );                 ;/*  2A division_by_zero                             */
    TRAP( 0x2b, archDataStoreErrEntry );            ;/*  2B data_store_error                             */
    TRAP( 0x2c, archDataAccessMmuMissEntry );       ;/*  2C data_access_MMU_miss                         */
    BAD_TRAP;                                       ;/*  2D undefined                                    */
    BAD_TRAP;                                       ;/*  2E undefined                                    */
    BAD_TRAP;                                       ;/*  2F undefined                                    */

    BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;         ;/*  30 - 33 undefined                               */
    BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;         ;/*  34 - 37 undefined                               */
    BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;         ;/*  38 - 3B undefined                               */
    TRAP( 0x3c, archInstAccessMmuMissEntry );       ;/*  3C instruction_access_MMU_miss                  */
              BAD_TRAP; BAD_TRAP; BAD_TRAP;         ;/*  3D - 3F undefined                               */
    BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;         ;/*  40 - 43 undefined                               */
    BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;         ;/*  44 - 47 undefined                               */
    BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;         ;/*  48 - 4B undefined                               */
    BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;         ;/*  4C - 4F undefined                               */
    BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;         ;/*  50 - 53 undefined                               */
    BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;         ;/*  54 - 57 undefined                               */
    BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;         ;/*  58 - 5B undefined                               */
    BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;         ;/*  5C - 5F undefined                               */
    BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;         ;/*  60 - 63 undefined                               */
    BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;         ;/*  64 - 67 undefined                               */
    BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;         ;/*  68 - 6B undefined                               */
    BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;         ;/*  6C - 6F undefined                               */
    BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;         ;/*  70 - 73 undefined                               */
    BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;         ;/*  74 - 77 undefined                               */
    BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;         ;/*  78 - 7B undefined                               */
    BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;         ;/*  7C - 7F undefined                               */

    ;/*
    ; *  Software traps
    ; *
    ; *  NOTE: At the risk of being redundant... this is not a full
    ; *        table.  The setjmp on the SPARC requires a window flush trap
    ; *        handler and SylixOS will preserve the entries that were
    ; *        installed before.
    ; */
    TRAP( 0x80, archTrapInstEntry );                ;/*  80 syscall SW trap                              */
    TRAP( 0x81, archTrapInstEntry );                ;/*  81 breakpoint SW trap                           */
    TRAP( 0x82, archTrapInstEntry );                ;/*  81 abortpoint SW trap                           */
    TRAP( 0x83, archWinFlushTrapEntry );            ;/*  83 flush windows SW trap                        */

    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  84 - 87                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  88 - 8B                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  8C - 8F                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  90 - 93                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  94 - 97                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  98 - 9B                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  9C - 9F                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  A0 - A3                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  A4 - A7                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  A8 - AB                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  AC - AF                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  B0 - B3                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  B4 - B7                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  B8 - BB                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  BC - BF                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  C0 - C3                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  C4 - C7                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  C8 - CB                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  CC - CF                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  D0 - D3                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  D4 - D7                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  D8 - DB                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  DC - DF                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  E0 - E3                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  E4 - E7                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  E8 - EB                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  EC - EF                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  F0 - F3                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  F4 - F7                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  F8 - FB                                         */
    SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;     ;/*  FC - FF                                         */

    FUNC_END(vector)

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/

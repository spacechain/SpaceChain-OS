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
** 文   件   名: arch_def.h
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 10 月 13 日
**
** 描        述: MIPS 相关定义.
*********************************************************************************************************/

#ifndef __MIPS_ARCH_DEF_H
#define __MIPS_ARCH_DEF_H

/*********************************************************************************************************
  MIPS 通用寄存器定义
*********************************************************************************************************/

#if defined(__ASSEMBLY__) || defined(ASSEMBLY)
#define ZERO            $0                                              /*  wired zero                  */
#define AT              $at                                             /*  assembler temp              */
#define V0              $2                                              /*  return reg 0                */
#define V1              $3                                              /*  return reg 1                */
#define A0              $4                                              /*  arg reg 0                   */
#define A1              $5                                              /*  arg reg 1                   */
#define A2              $6                                              /*  arg reg 2                   */
#define A3              $7                                              /*  arg reg 3                   */
#define T0              $8                                              /*  caller saved 0              */
#define T1              $9                                              /*  caller saved 1              */
#define T2              $10                                             /*  caller saved 2              */
#define T3              $11                                             /*  caller saved 3              */
#define T4              $12                                             /*  caller saved 4              */
#define T5              $13                                             /*  caller saved 5              */
#define T6              $14                                             /*  caller saved 6              */
#define T7              $15                                             /*  caller saved 7              */
#define S0              $16                                             /*  callee saved 0              */
#define S1              $17                                             /*  callee saved 1              */
#define S2              $18                                             /*  callee saved 2              */
#define S3              $19                                             /*  callee saved 3              */
#define S4              $20                                             /*  callee saved 4              */
#define S5              $21                                             /*  callee saved 5              */
#define S6              $22                                             /*  callee saved 6              */
#define S7              $23                                             /*  callee saved 7              */
#define T8              $24                                             /*  caller saved 8              */
#define T9              $25                                             /*  caller saved 9              */
#define K0              $26                                             /*  kernel temp 0               */
#define K1              $27                                             /*  kernel temp 1               */
#define GP              $28                                             /*  global pointer              */
#define SP              $29                                             /*  stack pointer               */
#define S8              $30                                             /*  callee saved 8              */
#define FP              S8                                              /*  callee saved 8              */
#define RA              $31                                             /*  return address              */

#elif defined(__SYLIXOS_KERNEL)
#define MIPS_ZERO       "$0"                                            /*  wired zero                  */
#define MIPS_AT         "$at"                                           /*  assembler temp              */
#define MIPS_V0         "$2"                                            /*  return reg 0                */
#define MIPS_V1         "$3"                                            /*  return reg 1                */
#define MIPS_A0         "$4"                                            /*  arg reg 0                   */
#define MIPS_A1         "$5"                                            /*  arg reg 1                   */
#define MIPS_A2         "$6"                                            /*  arg reg 2                   */
#define MIPS_A3         "$7"                                            /*  arg reg 3                   */
#define MIPS_T0         "$8"                                            /*  caller saved 0              */
#define MIPS_T1         "$9"                                            /*  caller saved 1              */
#define MIPS_T2         "$10"                                           /*  caller saved 2              */
#define MIPS_T3         "$11"                                           /*  caller saved 3              */
#define MIPS_T4         "$12"                                           /*  caller saved 4              */
#define MIPS_T5         "$13"                                           /*  caller saved 5              */
#define MIPS_T6         "$14"                                           /*  caller saved 6              */
#define MIPS_T7         "$15"                                           /*  caller saved 7              */
#define MIPS_S0         "$16"                                           /*  callee saved 0              */
#define MIPS_S1         "$17"                                           /*  callee saved 1              */
#define MIPS_S2         "$18"                                           /*  callee saved 2              */
#define MIPS_S3         "$19"                                           /*  callee saved 3              */
#define MIPS_S4         "$20"                                           /*  callee saved 4              */
#define MIPS_S5         "$21"                                           /*  callee saved 5              */
#define MIPS_S6         "$22"                                           /*  callee saved 6              */
#define MIPS_S7         "$23"                                           /*  callee saved 7              */
#define MIPS_T8         "$24"                                           /*  caller saved 8              */
#define MIPS_T9         "$25"                                           /*  caller saved 9              */
#define MIPS_K0         "$26"                                           /*  kernel temp 0               */
#define MIPS_K1         "$27"                                           /*  kernel temp 1               */
#define MIPS_GP         "$28"                                           /*  global pointer              */
#define MIPS_SP         "$29"                                           /*  stack pointer               */
#define MIPS_S8         "$30"                                           /*  callee saved 8              */
#define MIPS_FP         MIPS_S8                                         /*  callee saved 8              */
#define MIPS_RA         "$31"                                           /*  return address              */
#endif                                                                  /*  __ASSEMBLY__                */

/*********************************************************************************************************
  MIPS 协处理器 0 寄存器定义
*********************************************************************************************************/

#if defined(__ASSEMBLY__) || defined(ASSEMBLY)
#define CP0_INX         $0                                              /*  tlb index                   */
#define CP0_RAND        $1                                              /*  tlb random                  */
#define CP0_TLBLO       $2                                              /*  tlb entry low               */
#define CP0_TLBLO0      $2                                              /*  tlb entry low 0             */
#define CP0_TLBLO1      $3                                              /*  tlb entry low 1             */
#define CP0_CTXT        $4                                              /*  tlb context                 */
#define CP0_PAGEMASK    $5                                              /*  page mask                   */
#define CP0_WIRED       $6                                              /*  lb wired entries            */
#define CP0_HWRENA      $7
#define CP0_BADVADDR    $8                                              /*  bad virtual address         */
#define CP0_COUNT       $9                                              /*  count                       */
#define CP0_TLBHI       $10                                             /*  tlb entry hi                */
#define CP0_COMPARE     $11                                             /*  compare                     */
#define CP0_STATUS      $12                                             /*  status register             */
#define CP0_INTCTL      $12,1                                           /*  interrupts vector set       */
#define CP0_CAUSE       $13                                             /*  exception cause             */
#define CP0_EPC         $14                                             /*  exception pc                */
#define CP0_PRID        $15
#define CP0_EBASE       $15,1
#define CP0_CONFIG      $16
#define CP0_CONFIG1     $16,1
#define CP0_CONFIG2     $16,2
#define CP0_CONFIG3     $16,3
#define CP0_LLADDR      $17
#define CP0_WATCHLO     $18
#define CP0_WATCHHI     $19
#define CP0_ECC         $26
#define CP0_CACHEERR    $27
#define CP0_TAGLO       $28
#define CP0_TAGHI       $29
#define CP0_DataLO      $28,1
#define CP0_DataHI      $29,1
#define CP0_ERRPC       $30

/*********************************************************************************************************
  Loongson2/3x CP0 寄存器定义
*********************************************************************************************************/

#define CP0_DIAG        $22
#define CP0_PAGEGRAIN   $5,1
#define CP0_GSCONFIG    $16,6

#elif defined(__SYLIXOS_KERNEL)
#define CP0_INX         "$0"                                            /*  tlb index                   */
#define CP0_RAND        "$1"                                            /*  tlb random                  */
#define CP0_TLBLO       "$2"                                            /*  tlb entry low               */
#define CP0_TLBLO0      "$2"                                            /*  tlb entry low 0             */
#define CP0_TLBLO1      "$3"                                            /*  tlb entry low 1             */
#define CP0_CTXT        "$4"                                            /*  tlb context                 */
#define CP0_PAGEMASK    "$5"                                            /*  page mask                   */
#define CP0_WIRED       "$6"                                            /*  lb wired entries            */
#define CP0_HWRENA      "$7"
#define CP0_BADVADDR    "$8"                                            /*  bad virtual address         */
#define CP0_COUNT       "$9"                                            /*  count                       */
#define CP0_TLBHI       "$10"                                           /*  tlb entry hi                */
#define CP0_COMPARE     "$11"                                           /*  compare                     */
#define CP0_STATUS      "$12"                                           /*  status register             */
#define CP0_INTCTL      "$12,1"                                         /*  interrupts vector set       */
#define CP0_CAUSE       "$13"                                           /*  exception cause             */
#define CP0_EPC         "$14"                                           /*  exception pc                */
#define CP0_PRID        "$15"
#define CP0_EBASE       "$15,1"
#define CP0_CONFIG      "$16"
#define CP0_CONFIG1     "$16,1"
#define CP0_CONFIG2     "$16,2"
#define CP0_CONFIG3     "$16,3"
#define CP0_LLADDR      "$17"
#define CP0_WATCHLO     "$18"
#define CP0_WATCHHI     "$19"
#define CP0_DIAG        "$22"                                           /*  loongson2/3 only have       */
#define CP0_ECC         "$26"
#define CP0_CACHEERR    "$27"
#define CP0_TAGLO       "$28"
#define CP0_TAGHI       "$29"
#define CP0_DataLO      "$28,1"
#define CP0_DataHI      "$29,1"
#define CP0_ERRPC       "$30"
#endif                                                                  /*  __ASSEMBLY__                */

/*********************************************************************************************************
  MIPS 协处理器 1 寄存器定义
*********************************************************************************************************/

#if defined(__ASSEMBLY__) || defined(ASSEMBLY)
#define FP0             $f0                                             /*  return reg 0~3              */
#define FP1             $f1
#define FP2             $f2
#define FP3             $f3
#define FP4             $f4                                             /*  caller saved 0~7            */
#define FP5             $f5
#define FP6             $f6
#define FP7             $f7
#define FP8             $f8
#define FP9             $f9
#define FP10            $f10
#define FP11            $f11
#define FP12            $f12                                            /*  arg reg 0~3                 */
#define FP13            $f13
#define FP14            $f14
#define FP15            $f15
#define FP16            $f16                                            /*  caller saved 8~11           */
#define FP17            $f17
#define FP18            $f18
#define FP19            $f19
#define FP20            $f20                                            /*  callee saved 0~11           */
#define FP21            $f21
#define FP22            $f22
#define FP23            $f23
#define FP24            $f24
#define FP25            $f25
#define FP26            $f26
#define FP27            $f27
#define FP28            $f28
#define FP29            $f29
#define FP30            $f30
#define FP31            $f31

#define CP1_FIR         $0                                              /*  implementation/revision reg */
#define CP1_FCCR        $25                                             /*  Condition Code reg          */
#define CP1_FEXR        $26                                             /*  Exception reg               */
#define CP1_FENR        $28                                             /*  Enables reg                 */
#define CP1_STATUS      $31                                             /*  control/status reg          */
#elif defined(__SYLIXOS_KERNEL)
#define FP0             "$f0"                                           /*  return reg 0~3              */
#define FP1             "$f1"
#define FP2             "$f2"
#define FP3             "$f3"
#define FP4             "$f4"                                           /*  caller saved 0~7            */
#define FP5             "$f5"
#define FP6             "$f6"
#define FP7             "$f7"
#define FP8             "$f8"
#define FP9             "$f9"
#define FP10            "$f10"
#define FP11            "$f11"
#define FP12            "$f12"                                          /*  arg reg 0~3                 */
#define FP13            "$f13"
#define FP14            "$f14"
#define FP15            "$f15"
#define FP16            "$f16"                                          /*  caller saved 8~11           */
#define FP17            "$f17"
#define FP18            "$f18"
#define FP19            "$f19"
#define FP20            "$f20"                                          /*  callee saved 0~11           */
#define FP21            "$f21"
#define FP22            "$f22"
#define FP23            "$f23"
#define FP24            "$f24"
#define FP25            "$f25"
#define FP26            "$f26"
#define FP27            "$f27"
#define FP28            "$f28"
#define FP29            "$f29"
#define FP30            "$f30"
#define FP31            "$f31"

#define CP1_FIR         "$0"                                            /*  implementation/revision reg */
#define CP1_FCCR        "$25"                                           /*  Condition Code reg          */
#define CP1_FEXR        "$26"                                           /*  Exception reg               */
#define CP1_FENR        "$28"                                           /*  Enables reg                 */
#define CP1_STATUS      "$31"                                           /*  control/status reg          */
#endif                                                                  /*  __ASSEMBLY__                */

/*********************************************************************************************************
  MIPS defines in kernel
*********************************************************************************************************/
#if defined(__SYLIXOS_KERNEL) || defined(__ASSEMBLY__) || defined(ASSEMBLY)

#define MIPS_EXEC_INS(ins)  __asm__ __volatile__ (ins)

/*********************************************************************************************************
  CP0 Status Register
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |C|C|C|C|R|F|R|M|P|B|T|S|M| | R |I|I|I|I|I|I|I|I|K|S|U|U|R|E|E|I|
 * |U|U|U|U|P|R|E|X|X|E|S|R|M| | s |M|M|M|M|M|M|M|M|X|X|X|M|s|R|X|E| Status
 * |3|2|1|0| | | | | |V| | |I| | v |7|6|5|4|3|2|1|0| | | | |v|L|L| |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_StatusCU      28                                              /*  Coprocessor enable (R/W)    */
#define M_StatusCU      (0xf << S_StatusCU)
#define S_StatusCU3     31
#define M_StatusCU3     (0x1 << S_StatusCU3)
#define S_StatusCU2     30
#define M_StatusCU2     (0x1 << S_StatusCU2)
#define S_StatusCU1     29
#define M_StatusCU1     (0x1 << S_StatusCU1)
#define S_StatusCU0     28
#define M_StatusCU0     (0x1 << S_StatusCU0)
#define S_StatusRP      27                                              /*  Enable reduced power
                                                                         *  mode (R/W)                  */
#define M_StatusRP      (0x1 << S_StatusRP)
#define S_StatusFR      26                                              /*  Enable 64-bit FPRs
                                                                         *  (MIPS64 only) (R/W)         */
#define M_StatusFR      (0x1 << S_StatusFR)
#define S_StatusRE      25                                              /*  Enable reverse endian (R/W) */
#define M_StatusRE      (0x1 << S_StatusRE)
#define S_StatusMX      24                                              /*  Enable access to MDMX
                                                                         *  resources (MIPS64 only)(R/W)*/
#define M_StatusMX      (0x1 << S_StatusMX)
#define S_StatusPX      23                                              /*  Enable access to 64-bit
                                                                         *  instructions/data
                                                                         *  (MIPS64 only) (R/W)         */
#define M_StatusPX      (0x1 << S_StatusPX)
#define S_StatusBEV     22                                              /*  Enable Boot Exception
                                                                         *  Vectors (R/W)               */
#define M_StatusBEV     (0x1 << S_StatusBEV)
#define S_StatusTS      21                                              /*  Denote TLB shutdown (R/W)   */
#define M_StatusTS      (0x1 << S_StatusTS)
#define S_StatusSR      20                                              /*  Denote soft reset (R/W)     */
#define M_StatusSR      (0x1 << S_StatusSR)
#define S_StatusNMI     19
#define M_StatusNMI     (0x1 << S_StatusNMI)                            /*  Denote NMI (R/W)            */
#define S_StatusIM      8                                               /*  Interrupt mask (R/W)        */
#define M_StatusIM      (0xff << S_StatusIM)
#define S_StatusIM7     15
#define M_StatusIM7     (0x1 << S_StatusIM7)
#define S_StatusIM6     14
#define M_StatusIM6     (0x1 << S_StatusIM6)
#define S_StatusIM5     13
#define M_StatusIM5     (0x1 << S_StatusIM5)
#define S_StatusIM4     12
#define M_StatusIM4     (0x1 << S_StatusIM4)
#define S_StatusIM3     11
#define M_StatusIM3     (0x1 << S_StatusIM3)
#define S_StatusIM2     10
#define M_StatusIM2     (0x1 << S_StatusIM2)
#define S_StatusIM1     9
#define M_StatusIM1     (0x1 << S_StatusIM1)
#define S_StatusIM0     8
#define M_StatusIM0     (0x1 << S_StatusIM0)
#define S_StatusKX      7                                               /*  Enable access to extended   */
                                                                        /*  kernel addresses            */
                                                                        /*  (MIPS64 only) (R/W)         */
#define M_StatusKX      (0x1 << S_StatusKX)
#define S_StatusSX      6                                               /*  Enable access to extended   */
                                                                        /*  supervisor addresses        */
                                                                        /*  (MIPS64 only) (R/W)         */
#define M_StatusSX      (0x1 << S_StatusSX)
#define S_StatusUX      5                                               /*  Enable access to extended   */
                                                                        /*  user addresses (MIPS64 only)*/
#define M_StatusUX      (0x1 << S_StatusUX)
#define S_StatusKSU     3                                               /*  Two-bit current mode (R/W)  */
#define M_StatusKSU     (0x3 << S_StatusKSU)
#define S_StatusUM      4                                               /*  User mode if supervisor mode*/
                                                                        /*  not implemented (R/W)       */
#define M_StatusUM      (0x1 << S_StatusUM)
#define S_StatusSM      3                                               /*  Supervisor mode (R/W)       */
#define M_StatusSM      (0x1 << S_StatusSM)
#define S_StatusERL     2                                               /*  Denotes error level (R/W)   */
#define M_StatusERL     (0x1 << S_StatusERL)
#define S_StatusEXL     1                                               /*  Denotes exception level(R/W)*/
#define M_StatusEXL     (0x1 << S_StatusEXL)
#define S_StatusIE      0                                               /*  Enables interrupts (R/W)    */
#define M_StatusIE      (0x1 << S_StatusIE)

/*********************************************************************************************************
 *  CP0 Cause Register
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |B| | C |       |I|W|           |I|I|I|I|I|I|I|I| |         | R |
 * |D| | E | Rsvd  |V|P|    Rsvd   |P|P|P|P|P|P|P|P| | ExcCode | s | Cause
 * | | |   |       | | |           |7|6|5|4|3|2|1|0| |         | v |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_CauseBD       31
#define M_CauseBD       (0x1 << S_CauseBD)
#define S_CauseCE       28
#define M_CauseCE       (0x3<< S_CauseCE)
#define S_CauseIV       23
#define M_CauseIV       (0x1 << S_CauseIV)
#define S_CauseWP       22
#define M_CauseWP       (0x1 << S_CauseWP)
#define S_CauseIP       8
#define M_CauseIP       (0xff << S_CauseIP)
#define S_CauseIPEXT    10
#define M_CauseIPEXT    (0x3f << S_CauseIPEXT)
#define S_CauseIP7      15
#define M_CauseIP7      (0x1 << S_CauseIP7)
#define S_CauseIP6      14
#define M_CauseIP6      (0x1 << S_CauseIP6)
#define S_CauseIP5      13
#define M_CauseIP5      (0x1 << S_CauseIP5)
#define S_CauseIP4      12
#define M_CauseIP4      (0x1 << S_CauseIP4)
#define S_CauseIP3      11
#define M_CauseIP3      (0x1 << S_CauseIP3)
#define S_CauseIP2      10
#define M_CauseIP2      (0x1 << S_CauseIP2)
#define S_CauseIP1      9
#define M_CauseIP1      (0x1 << S_CauseIP1)
#define S_CauseIP0      8
#define M_CauseIP0      (0x1 << S_CauseIP0)
#define S_CauseExcCode  2
#define M_CauseExcCode  (0x1f << S_CauseExcCode)

/*********************************************************************************************************
  Values in the ExcCode field
*********************************************************************************************************/

#define EX_INT          0                                               /*  Interrupt                   */
#define EXC_INT         (EX_INT << S_CauseExcCode)
#define EX_MOD          1                                               /*  TLB modified                */
#define EXC_MOD         (EX_MOD << S_CauseExcCode)
#define EX_TLBL         2                                               /*  TLB exception(load or ifetch*/
#define EXC_TLBL        (EX_TLBL << S_CauseExcCode)
#define EX_TLBS         3                                               /*  TLB exception (store)       */
#define EXC_TLBS        (EX_TLBS << S_CauseExcCode)
#define EX_ADEL         4                                               /*  Address error(load or ifetch*/
#define EXC_ADEL        (EX_ADEL << S_CauseExcCode)
#define EX_ADES         5                                               /*  Address error (store)       */
#define EXC_ADES        (EX_ADES << S_CauseExcCode)
#define EX_IBE          6                                               /*  Instruction Bus Error       */
#define EXC_IBE         (EX_IBE << S_CauseExcCode)
#define EX_DBE          7                                               /*  Data Bus Error              */
#define EXC_DBE         (EX_DBE << S_CauseExcCode)
#define EX_SYS          8                                               /*  Syscall                     */
#define EXC_SYS         (EX_SYS << S_CauseExcCode)
#define EX_SYSCALL      EX_SYS
#define EXC_SYSCALL     EXC_SYS
#define EX_BP           9                                               /*  Breakpoint                  */
#define EXC_BP          (EX_BP << S_CauseExcCode)
#define EX_BREAK        EX_BP
#define EXC_BREAK       EXC_BP
#define EX_RI           10                                              /*  Reserved instruction        */
#define EXC_RI          (EX_RI << S_CauseExcCode)
#define EX_CPU          11                                              /*  CoProcessor Unusable        */
#define EXC_CPU         (EX_CPU << S_CauseExcCode)
#define EX_OV           12                                              /*  OVerflow                    */
#define EXC_OV          (EX_OV << S_CauseExcCode)
#define EX_TR           13                                              /*  Trap instruction            */
#define EXC_TR          (EX_TR << S_CauseExcCode)
#define EX_TRAP         EX_TR
#define EXC_TRAP        EXC_TR
#define EX_FPE          15                                              /*  floating point exception    */
#define EXC_FPE         (EX_FPE << S_CauseExcCode)
#define EX_C2E          18                                              /*  COP2 exception              */
#define EXC_C2E         (EX_C2E << S_CauseExcCode)
#define EX_MDMX         22                                              /*  MDMX exception              */
#define EXC_MDMX        (EX_MDMX << S_CauseExcCode)
#define EX_WATCH        23                                              /*  Watch exception             */
#define EXC_WATCH       (EX_WATCH << S_CauseExcCode)
#define EX_MCHECK       24                                              /*  Machine check exception     */
#define EXC_MCHECK      (EX_MCHECK << S_CauseExcCode)
#define EX_THREAD       25                                              /*  Thread exception            */
#define EXC_THREAD      (EX_THREAD << S_CauseExcCode)
#define EX_DSP          26                                              /*  DSP Unusable                */
#define EXC_DSP         (EX_DSP << S_CauseExcCode)
#define EX_CacheErr     30                                              /*  Cache error caused re-entry */
                                                                        /*  to Debug Mode               */
#define EXC_CacheErr    (EX_CacheErr << S_CauseExcCode)

/*********************************************************************************************************
 *  CP0 Config Register
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |M|                             |B| A |  A  |               | K | Config
 * | | Reserved for Implementations|E| T |  R  |    Reserved   | 0 |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_ConfigMore    31                                              /*  Additional config registers */
                                                                        /*  present (R)                 */
#define M_ConfigMore    (0x1 << S_ConfigMore)
#define S_ConfigImpl    16                                              /*  Implementation-specific     */
                                                                        /*  fields                      */
#define M_ConfigImpl    (0x7fff << S_ConfigImpl)
#define S_ConfigBE      15                                              /*  Denotes big-endian          */
                                                                        /*  operation (R)               */
#define M_ConfigBE      (0x1 << S_ConfigBE)
#define S_ConfigAT      13                                              /*  Architecture type (R)       */
#define M_ConfigAT      (0x3 << S_ConfigAT)
#define S_ConfigAR      10                                              /*  Architecture revision (R)   */
#define M_ConfigAR      (0x7 << S_ConfigAR)
#define S_ConfigMT      7                                               /*  MMU Type (R)                */
#define M_ConfigMT      (0x7 << S_ConfigMT)
#define S_ConfigK0      0                                               /*  Kseg0 coherency             */
                                                                        /*  algorithm (R/W)             */
#define M_ConfigK0      (0x7 << S_ConfigK0)

/*********************************************************************************************************
  Bits in the CP0 config register
  Generic bits
*********************************************************************************************************/

#define MIPS_CACHABLE_NO_WA          0
#define MIPS_CACHABLE_WA             1
#define MIPS_UNCACHED                2
#define MIPS_CACHABLE_NONCOHERENT    3
#define MIPS_CACHABLE_CE             4
#define MIPS_CACHABLE_COW            5
#define MIPS_CACHABLE_CUW            6
#define MIPS_CACHABLE_ACCELERATED    7

/*********************************************************************************************************
 *  CP0 Config1 Register
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |M|  MMU Size |  IS |  IL |  IA |  DS |  DL |  DA |C|M|P|W|C|E|F| Config1
 * | |           |     |     |     |     |     |     |2|D|C|R|A|P|P|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_Config1More       31                                          /*  Additional Config registers */
                                                                        /*  present (R)                 */
#define M_Config1More       (0x1 << S_Config1More)
#define S_Config1MMUSize    25                                          /*  Number of MMU entries-1 (R) */
#define M_Config1MMUSize    (0x3f << S_Config1MMUSize)
#define S_Config1IS         22                                          /*  Icache sets per way (R)     */
#define M_Config1IS         (0x7 << S_Config1IS)
#define S_Config1IL         19                                          /*  Icache line size (R)        */
#define M_Config1IL         (0x7 << S_Config1IL)
#define S_Config1IA         16                                          /*  Icache associativity-1 (R)  */
#define M_Config1IA         (0x7 << S_Config1IA)
#define S_Config1DS         13                                          /*  Dcache sets per way (R)     */
#define M_Config1DS         (0x7 << S_Config1DS)
#define S_Config1DL         10                                          /*  Dcache line size (R)        */
#define M_Config1DL         (0x7 << S_Config1DL)
#define S_Config1DA         7                                           /*  Dcache associativity (R)    */
#define M_Config1DA         (0x7 << S_Config1DA)
#define S_Config1C2         6                                           /*  Coprocessor 2 present (R)   */
#define M_Config1C2         (0x1 << S_Config1C2)
#define S_Config1MD         5                                           /*  Denotes MDMX present (R)    */
#define M_Config1MD         (0x1 << S_Config1MD)
#define S_Config1PC         4                                           /*  Denotes performance         */
                                                                        /*  counters present (R)        */
#define M_Config1PC         (0x1 << S_Config1PC)
#define S_Config1WR         3                                           /*  Denotes watch registers     */
                                                                        /*  present (R)                 */
#define M_Config1WR         (0x1 << S_Config1WR)
#define S_Config1CA         2                                           /*  Denotes MIPS-16 present (R) */
#define M_Config1CA         (0x1 << S_Config1CA)
#define S_Config1EP         1                                           /*  Denotes EJTAG present (R)   */
#define M_Config1EP         (0x1 << S_Config1EP)
#define S_Config1FP         0                                           /*  Denotes floating point      */
                                                                        /*  present (R)                 */
#define M_Config1FP         (0x1 << S_Config1FP)

/*********************************************************************************************************
 *  CP0 Config2 Register
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |M| TU  |   TS  |  TL   |  TA   |  SU   |  SS   |   SL  |  SA   |   Config2
 * | |     |       |       |       |       |       |       |       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_Config2More       31                                          /*  Additional Config registers */
                                                                        /*  present (R)                 */
#define M_Config2More       (0x1 << S_Config2More)
#define S_Config2SM         1                                           /*  Denotes SmartMIPS ASE       */

#define S_Config2TU         28                                          /*  Tcache contrl or status bit */
#define M_Config2TU         (0x7 << S_Config2TU)
#define S_Config2TS         24                                          /*  Tcache sets per way (R)     */
#define M_Config2TS         (0xF << S_Config2TS)
#define S_Config2TL         20                                          /*  Tcache line size (R)        */
#define M_Config2TL         (0xF << S_Config2TL)
#define S_Config2TA         16                                          /*  Tcache associativity (R)    */
#define M_Config2TA         (0xF << S_Config2TA)
#define S_Config2SU         12                                          /*  Scache contrl or status bit */
#define M_Config2SU         (0xF << S_Config2SU)
#define S_Config2SS         8                                           /*  Scache sets per way (R)     */
#define M_Config2SS         (0xF << S_Config2SS)
#define S_Config2SL         4                                           /*  Scache line size (R)        */
#define M_Config2SL         (0xF << S_Config2SL)
#define S_Config2SA         0                                           /*  Scache associativity (R)    */
#define M_Config2SA         (0xF << S_Config2SA)

/*********************************************************************************************************
 *  CP0 IntCtl Register
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |IPTI |IPPCI|                               |    VS   |         |IntCtl
 * |     |     |            0                  |         |    0    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_IntCtlIPPI    29
#define M_IntCtlIPPI    (0x7 << S_IntCtlIPPI)
#define S_IntCtlIPPCI   26
#define M_IntCtlIPPCI   (0x7 << S_IntCtlIPPCI)
#define S_IntCtlVS      5
#define M_IntCtlVS      (0x1f << S_IntCtlVS)

/*********************************************************************************************************
 *  CP0 CacheErr Register
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |T|L|E|E|E|E|E|E|E|E|                                           |CacheErr
 * |P|E|D|T|S|E|B|I|1|0|                                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_CacheType     31                                              /*  reference type              */
#define M_CacheType     (0x1 << S_CacheType)                            /*  0=Instr, 1=Data             */
#define S_CacheLevel    30                                              /*  reference level             */
#define M_CacheLevel    (0x1 << S_CacheLevel)                           /*  0=Primary, 1=Secondary      */
#define S_CacheData     29                                              /*  data field                  */
#define M_CacheData     (0x1 << S_CacheData)                            /*  0=No error, 1=Error         */
#define S_CacheTag      28                                              /*  Tag field                   */
#define M_CacheTag      (0x1 << S_CacheTag)                             /*  0=No error, 1=Error         */
#define S_CacheBus      27                                              /*  error on bus                */
#define M_CacheBus      (0x1 << S_CacheBus)                             /*  0=No, 1=Yes                 */
#define S_CacheECC      26                                              /*  ECC error                   */
#define M_CacheECC      (0x1 << S_CacheECC)                             /*  0=No, 1=Yes                 */
#define S_CacheBoth     25                                              /*  Data & Instruction error    */
#define M_CacheBoth     (0x1 << S_CacheBoth)                            /*  0=No, 1=Yes                 */
#define S_CacheEI       24
#define M_CacheEI       (0x1 << S_CacheEI)
#define S_CacheE1       23
#define M_CacheE1       (0x1 << S_CacheE1)
#define S_CacheE0       22
#define M_CacheE0       (0x1 << S_CacheE0)

/*********************************************************************************************************
 *   CP1 Implementation Register
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |Reserved for Additional|3|P|D|S| Implementation|   Revision    | FIR
 * |  Configuration Bits   |D|S| | |               |               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_FIRConfigS    16
#define M_FIRConfigS    (0x1 << S_FIRConfigS)
#define S_FIRConfigD    17
#define M_FIRConfigD    (0x1 << S_FIRConfigD)
#define S_FIRConfigPS   18
#define M_FIRConfigPS   (0x1 << S_FIRConfigPS)
#define S_FIRConfig3D   19
#define M_FIRConfig3D   (0x1 << S_FIRConfig3D)
#define M_FIRConfigAll  (M_FIRConfigS | M_FIRConfigD | M_FIRConfigPS | M_FIRConfig3D)
#define S_FIRConfigF64  22
#define M_FIRConfigF64  (0x01 << S_FIRConfigF64)

#define S_FIRImp        8
#define M_FIRImp        (0xff << S_FIRImp)
#define S_FIRRev        0
#define M_FIRRev        (0xff << S_FIRRev)

/*********************************************************************************************************
 *   CP1 Condition Register
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                      0                        |      CC       | FCCR
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_FCCRCC        0
#define M_FCCRCC        (0xff << S_FCCRCC)
#define S_FCCRCC7       7
#define M_FCCRCC7       (0x1 << S_FCCRCC7)
#define S_FCCRCC6       6
#define M_FCCRCC6       (0x1 << S_FCCRCC6)
#define S_FCCRCC5       5
#define M_FCCRCC5       (0x1 << S_FCCRCC5)
#define S_FCCRCC4       4
#define M_FCCRCC4       (0x1 << S_FCCRCC4)
#define S_FCCRCC3       3
#define M_FCCRCC3       (0x1 << S_FCCRCC3)
#define S_FCCRCC2       2
#define M_FCCRCC2       (0x1 << S_FCCRCC2)
#define S_FCCRCC1       1
#define M_FCCRCC1       (0x1 << S_FCCRCC1)
#define S_FCCRCC0       0
#define M_FCCRCC0       (0x1 << S_FCCRCC0)

/*********************************************************************************************************
 *   CP1 Exception Register
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |             0             |   Cause   |    0    |  Flags  | 0 | FEXR
 * |                           |E|V|Z|O|U|I|         |V|Z|O|U|I|   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_FEXRExc       12
#define M_FEXRExc       (0x3f << S_FEXRExc)
#define S_FEXRExcE      17
#define M_FEXRExcE      (0x1 << S_FEXRExcE)
#define S_FEXRExcV      16
#define M_FEXRExcV      (0x1 << S_FEXRExcV)
#define S_FEXRExcZ      15
#define M_FEXRExcZ      (0x1 << S_FEXRExcZ)
#define S_FEXRExcO      14
#define M_FEXRExcO      (0x1 << S_FEXRExcO)
#define S_FEXRExcU      13
#define M_FEXRExcU      (0x1 << S_FEXRExcU)
#define S_FEXRExcI      12
#define M_FEXRExcI      (0x1 << S_FEXRExcI)

#define S_FEXRFlg       2
#define M_FEXRFlg       (0x1f << S_FEXRFlg)
#define S_FEXRFlgV      6
#define M_FEXRFlgV      (0x1 << S_FEXRFlgV)
#define S_FEXRFlgZ      5
#define M_FEXRFlgZ      (0x1 << S_FEXRFlgZ)
#define S_FEXRFlgO      4
#define M_FEXRFlgO      (0x1 << S_FEXRFlgO)
#define S_FEXRFlgU      3
#define M_FEXRFlgU      (0x1 << S_FEXRFlgU)
#define S_FEXRFlgI      2
#define M_FEXRFlgI      (0x1 << S_FEXRFlgI)

/*********************************************************************************************************
 *   CP1 Enables Register
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                   0                   | Enables |   0   |F|RM | FENR
 * |                                       |V|Z|O|U|I|       |S|   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_FENREna       7
#define M_FENREna       (0x1f << S_FENREna)
#define S_FENREnaV      11
#define M_FENREnaV      (0x1 << S_FENREnaV)
#define S_FENREnaZ      10
#define M_FENREnaZ      (0x1 << S_FENREnaZ)
#define S_FENREnaO      9
#define M_FENREnaO      (0x1 << S_FENREnaO)
#define S_FENREnaU      8
#define M_FENREnaU      (0x1 << S_FENREnaU)
#define S_FENREnaI      7
#define M_FENREnaI      (0x1 << S_FENREnaI)

#define S_FENRFS        2
#define M_FENRFS        (0x1 << S_FENRFS)

#define S_FENRRM        0
#define M_FENRRM        (0x3 << S_FENRRM)

/*********************************************************************************************************
 *   CP1 Control/Status Register
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |     FCC     |F|C|Imp|  0  |   Cause   | Enables |  Flags  | RM| FCSR
 * |7|6|5|4|3|2|1|S|C|   |     |E|V|Z|O|U|I|V|Z|O|U|I|V|Z|O|U|I|   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_FCSRFCC7_1    25                                              /*  Floating point condition    */
#define M_FCSRFCC7_1    (0x7f << S_FCSRFCC7_1)                          /*  codes 7..1 (R/W)            */
#define S_FCSRCC7       31
#define M_FCSRCC7       (0x1 << S_FCSRCC7)
#define S_FCSRCC6       30
#define M_FCSRCC6       (0x1 << S_FCSRCC6)
#define S_FCSRCC5       29
#define M_FCSRCC5       (0x1 << S_FCSRCC5)
#define S_FCSRCC4       28
#define M_FCSRCC4       (0x1 << S_FCSRCC4)
#define S_FCSRCC3       27
#define M_FCSRCC3       (0x1 << S_FCSRCC3)
#define S_FCSRCC2       26
#define M_FCSRCC2       (0x1 << S_FCSRCC2)

#define S_FCSRCC1       25
#define M_FCSRCC1       (0x1 << S_FCSRCC1)
#define S_FCSRFS        24                                              /*  Flush denorms to zero (R/W) */
#define M_FCSRFS        (0x1 << S_FCSRFS)

#define S_FCSRCC0       23                                              /*  Floating point condition    */
#define M_FCSRCC0       (0x1 << S_FCSRCC0)                              /*  code 0 (R/W)                */
#define S_FCSRCC        S_FCSRCC0
#define M_FCSRCC        M_FCSRCC0

#define S_FCSRImpl      21                                              /*  Implementation-specific     */
#define M_FCSRImpl      (0x3 << S_FCSRImpl)                             /*  control bits (R/W)          */

#define S_FCSRExc       12                                              /*  Exception cause (R/W)       */
#define M_FCSRExc       (0x3f << S_FCSRExc)
#define S_FCSRExcE      17
#define M_FCSRExcE      (0x1 << S_FCSRExcE)
#define S_FCSRExcV      16
#define M_FCSRExcV      (0x1 << S_FCSRExcV)
#define S_FCSRExcZ      15
#define M_FCSRExcZ      (0x1 << S_FCSRExcZ)
#define S_FCSRExcO      14
#define M_FCSRExcO      (0x1 << S_FCSRExcO)
#define S_FCSRExcU      13
#define M_FCSRExcU      (0x1 << S_FCSRExcU)
#define S_FCSRExcI      12
#define M_FCSRExcI      (0x1 << S_FCSRExcI)

#define S_FCSREna       7                                               /*  Exception enable (R/W)      */
#define M_FCSREna       (0x1f << S_FCSREna)
#define S_FCSREnaV      11
#define M_FCSREnaV      (0x1 << S_FCSREnaV)
#define S_FCSREnaZ      10
#define M_FCSREnaZ      (0x1 << S_FCSREnaZ)
#define S_FCSREnaO      9
#define M_FCSREnaO      (0x1 << S_FCSREnaO)
#define S_FCSREnaU      8
#define M_FCSREnaU      (0x1 << S_FCSREnaU)
#define S_FCSREnaI      7
#define M_FCSREnaI      (0x1 << S_FCSREnaI)

#define S_FCSRFlg       2                                               /*  Exception flags (R/W)       */
#define M_FCSRFlg       (0x1f << S_FCSRFlg)
#define S_FCSRFlgV      6
#define M_FCSRFlgV      (0x1 << S_FCSRFlgV)
#define S_FCSRFlgZ      5
#define M_FCSRFlgZ      (0x1 << S_FCSRFlgZ)
#define S_FCSRFlgO      4
#define M_FCSRFlgO      (0x1 << S_FCSRFlgO)
#define S_FCSRFlgU      3
#define M_FCSRFlgU      (0x1 << S_FCSRFlgU)
#define S_FCSRFlgI      2
#define M_FCSRFlgI      (0x1 << S_FCSRFlgI)

#define S_FCSRRM        0                                               /*  Rounding mode (R/W)         */
#define M_FCSRRM        (0x3 << S_FCSRRM)

/*********************************************************************************************************
  Values in the rounding mode field (of both FCSR and FCCR)
*********************************************************************************************************/

#define K_FCSRRM_RN     0
#define K_FCSRRM_RZ     1
#define K_FCSRRM_RP     2
#define K_FCSRRM_RM     3

/*********************************************************************************************************
  Virtual Address Definitions
*********************************************************************************************************/

#define A_K0BASE        0x80000000
#define A_K1BASE        0xa0000000
#define A_K2BASE        0xc0000000
#define A_K3BASE        0xe0000000
#define M_KMAPPED       0x40000000                                      /*  KnSEG address is mapped     */
                                                                        /*  if bit is one               */
/*********************************************************************************************************
  Cache Operations available on all MIPS processors
  some  Operations Implementation Dependent on .....
*********************************************************************************************************/

#define Index_Invalidate_I               0x0                            /*  0       0                   */
#define Index_Writeback_Inv_D            0x1                            /*  0       1                   */
#define Index_Invalidate_T               0x2                            /*  0       2                   */
#define Index_Writeback_Inv_S            0x3                            /*  0       3                   */
#define Index_Load_Tag_I                 0x4                            /*  1       0                   */
#define Index_Load_Tag_D                 0x5                            /*  1       1                   */
#define Index_Load_Tag_T                 0x6                            /*  1       2                   */
#define Index_Load_Tag_S                 0x7                            /*  1       3                   */
#define Index_Store_Tag_I                0x8                            /*  2       0                   */
#define Index_Store_Tag_D                0x9                            /*  2       1                   */
#define Index_Store_Tag_T                0xA                            /*  2       2                   */
#define Index_Store_Tag_S                0xB                            /*  2       3                   */
                                                                        /*  3       0                   */
                                                                        /*  3       1                   */
                                                                        /*  3       2                   */
                                                                        /*  3       3                   */
#define Hit_Invalidate_I                 0x10                           /*  4       0                   */
#define Hit_Invalidate_D                 0x11                           /*  4       1                   */
#define Hit_Invalidate_T                 0x12                           /*  4       2                   */
#define Hit_Invalidate_S                 0x13                           /*  4       3                   */
#define Fill_I                           0x14                           /*  5       0                   */
#define Hit_Writeback_Inv_D              0x15                           /*  5       1                   */
#define Hit_Writeback_Inv_T              0x16                           /*  5       2                   */
#define Hit_Writeback_Inv_S              0x17                           /*  5       3                   */
                                                                        /*  6       0                   */
#define Hit_Writeback_D                  0x19                           /*  6       1                   */
#define Hit_Writeback_T                  0x1A                           /*  6       2                   */
#define Hit_Writeback_S                  0x1B                           /*  6       3                   */
#define Fetch_Lock_I                     0x1C                           /*  7       0                   */
#define Fetch_Lock_D                     0x1D                           /*  7       1                   */
                                                                        /*  7       2                   */
                                                                        /*  7       3                   */

/*********************************************************************************************************
  处理器 ID
*********************************************************************************************************/

#define PRID_REV_LOONGSON1B             0x0020
#define PRID_REV_LOONGSON2K             0x0001
#define PRID_REV_LOONGSON2E             0x0002
#define PRID_REV_LOONGSON2F             0x0003
#define PRID_REV_LOONGSON3A_R1          0x0005
#define PRID_REV_LOONGSON3B_R1          0x0006
#define PRID_REV_LOONGSON3B_R2          0x0007
#define PRID_REV_LOONGSON3A_R2          0x0008
#define PRID_REV_LOONGSON3A_R3          0x0009

#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __ARCH_DEF_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/

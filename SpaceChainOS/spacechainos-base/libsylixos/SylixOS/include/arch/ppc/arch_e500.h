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
** 文   件   名: arch_e500.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 03 月 30 日
**
** 描        述: PowerPC E500 特有寄存器等定义.
*********************************************************************************************************/

#ifndef __PPC_ARCH_E500_H
#define __PPC_ARCH_E500_H

/*********************************************************************************************************
  Erratum CPU29 for E500 exists on all Rev.2 cores.
  mtmsr with change to IS bit may corrupt system state.
  Workaround is to use rfi instead of mtmsr.
*********************************************************************************************************/

#ifndef __SYLIXOS_PPC_E200__
#ifndef __SYLIXOS_PPC_E500MC__
#define PPC85XX_ERRATA_CPU29        1
#endif                                              /*  __SYLIXOS_PPC_E500MC__                          */
#endif                                              /*  __SYLIXOS_PPC_E200__                            */

/*********************************************************************************************************
  Rev 1 silicon errata for P4080
*********************************************************************************************************/
#ifdef  __SYLIXOS_PPC_E500MC__
#undef  P4080_ERRATUM_CPU6
#undef  P4080_ERRATUM_CPU8
#endif                                              /*  __SYLIXOS_PPC_E500MC__                          */

/*********************************************************************************************************
  Maximum cache line size for the cpu
*********************************************************************************************************/

#ifdef  __SYLIXOS_PPC_E500MC__
#define ARCH_PPC_CACHE_ALIGN_SHIFT  6               /*  cache align size = 64                           */
#else
#define ARCH_PPC_CACHE_ALIGN_SHIFT  5               /*  cache align size = 32                           */
#endif                                              /*  __SYLIXOS_PPC_E500MC__                          */

#ifdef  __SYLIXOS_PPC_E500MC__
#define ARCH_PPC_CACHE_ALIGN_SIZE   64
#else
#define ARCH_PPC_CACHE_ALIGN_SIZE   32
#endif                                              /*  __SYLIXOS_PPC_E500MC__                          */

/*********************************************************************************************************
  配置相关的寄存器及定义
*********************************************************************************************************/

#define SVR                         1023            /*  system version register                         */
#define PVR                         287             /*  processor version register                      */
#define PIR                         286
#define PVR_VALUE                   0x80210030      /*  expected value in PVR                           */

/*********************************************************************************************************
  异常相关的寄存器及定义
*********************************************************************************************************/

#define CSRR0                       58              /*  Critical SRR0                                   */
#define CSRR1                       59              /*  Critical SRR1                                   */

#define DEAR                        61              /*  Data Exception Address Register                 */

#define ESR                         62              /*  Exception Syndrome Register                     */

#define IVPR                        63              /*  Interrupt Vector Prefix Register                */

#define IVOR0                       400             /*  IVOR Critical Input                             */
#define IVOR1                       401             /*  IVOR Machine Check                              */
#define IVOR2                       402             /*  IVOR Data Storage                               */
#define IVOR3                       403             /*  IVOR Instruction Storage                        */
#define IVOR4                       404             /*  IVOR External Input                             */
#define IVOR5                       405             /*  IVOR Alignment                                  */
#define IVOR6                       406             /*  IVOR Program                                    */
#define IVOR7                       407             /*  IVOR Floating Point Unavailable                 */
#define IVOR8                       408             /*  IVOR System Call                                */
#define IVOR9                       409             /*  IVOR Auxiliary Processor Unavailable            */
#define IVOR10                      410             /*  IVOR Decrementer                                */
#define IVOR11                      411             /*  IVOR Fixed Interval Timer                       */
#define IVOR12                      412             /*  IVOR Watchdog Timer                             */
#define IVOR13                      413             /*  IVOR Data TLB Error                             */
#define IVOR14                      414             /*  IVOR Instruction TLB Error                      */
#define IVOR15                      415             /*  IVOR Debug                                      */

#define IVOR32                      528             /*  IVOR SPE                                        */
#define IVOR33                      529             /*  IVOR Vector FP Data                             */
#define IVOR34                      530             /*  IVOR Vector FP Round                            */

#ifndef __SYLIXOS_PPC_E200__
#define IVOR35                      531             /*  IVOR Performance Monitor                        */
#endif                                              /*  __SYLIXOS_PPC_E200__                            */

#ifdef  __SYLIXOS_PPC_E500MC__
#define IVOR36                      532             /*  IVOR Processor Doorbell Interrupt               */
#define IVOR37                      533             /*  IVOR Processor Doorbell Critical Interrupt      */
#define IVOR38                      534             /*  IVOR Guest Processor Doorbell Interrupt         */
#define IVOR39                      535             /*  IVOR Guest Processor Doorbell Critical Interrupt*/
#define IVOR40                      536             /*  IVOR Ultravisor System Call Interrupt           */
#define IVOR41                      537             /*  IVOR Ultravisor Privilege Interrupt             */
#endif                                              /*  __SYLIXOS_PPC_E500MC__                          */

/*********************************************************************************************************
  Exception syndrome register mask bits:
  0 - error not occured 1 - error occured
*********************************************************************************************************/

#define ARCH_PPC_ESR_PIL_U          0x0800          /*  Pgm Interrupt -- Illegal Insn                   */
#define ARCH_PPC_ESR_PPR_U          0x0400          /*  Pgm Interrupt -- Previleged Insn                */
#define ARCH_PPC_ESR_PTR_U          0x0200          /*  Pgm Interrupt -- Trap                           */

#ifdef  __SYLIXOS_PPC_E500MC__
#define ARCH_PPC_ESR_FP_U           0x0100          /*  Floating Point Operation                        */
#endif                                              /*  __SYLIXOS_PPC_E500MC__                          */

#define ARCH_PPC_ESR_ST_U           0x0080          /*  Store Operation                                 */
#define ARCH_PPC_ESR_DLK_U          0x0020          /*  Data Locked -- DSI occured                      */
#define ARCH_PPC_ESR_ILK_U          0x0010          /*  Inst Locked -- DSI occured                      */
#define ARCH_PPC_ESR_AP_U           0x0008          /*  AP Operation                                    */
#define ARCH_PPC_ESR_BO_U           0x0002          /*  Byte Ordering Exception                         */
#define ARCH_PPC_ESR_PIL            0x08000000      /*  Pgm Interrupt -- Illegal Insn                   */
#define ARCH_PPC_ESR_PPR            0x04000000      /*  Pgm Interrupt -- Previleged Insn                */
#define ARCH_PPC_ESR_PTR            0x02000000      /*  Pgm Interrupt -- Trap                           */

#ifdef  __SYLIXOS_PPC_E500MC__
#define ARCH_PPC_ESR_FP             0x01000000      /*  Floating Point Operation                        */
#endif                                              /*  __SYLIXOS_PPC_E500MC__                          */

#define ARCH_PPC_ESR_ST             0x00800000      /*  Store Operation                                 */
#define ARCH_PPC_ESR_DLK            0x00200000      /*  Data Storage Interrupt -- Locking               */
#define ARCH_PPC_ESR_ILK            0x00100000      /*  Inst Locked -- DSI occured                      */
#define ARCH_PPC_ESR_AP             0x00080000      /*  AP Operation                                    */
#define ARCH_PPC_ESR_BO             0x00020000      /*  Byte Ordering Exception                         */

#define ARCH_PPC_ESR_SPE            0x00000080      /*  SPE exception                                   */

/*********************************************************************************************************
  机器检查异常相关的寄存器及定义
*********************************************************************************************************/

#ifndef __SYLIXOS_PPC_E200__
#define MCSRR0                      570             /*  Machine Check SRR0                              */
#define MCSRR1                      571             /*  Machine Check SRR1                              */
#define MCAR                        573             /*  Machine Check Address Register                  */
#else
#define MCSRR0                      CSRR0           /*  Machine Check SRR0                              */
#define MCSRR1                      CSRR1           /*  Machine Check SRR1                              */
#undef  MCAR                                        /*  Machine Check Address Register                  */
#endif                                              /*  __SYLIXOS_PPC_E200__                            */

#define MCSR                        572             /*  Machine Check Syndrome Register                 */

#ifndef __SYLIXOS_PPC_E500MC__
                                                    /*  MCSR bit definitions                            */
#define ARCH_PPC_MCSR_BIT_MCP       0               /*  Machine check input pin                         */
#define ARCH_PPC_MCSR_BIT_ICPERR    1               /*  Instruction cache parity error                  */
#define ARCH_PPC_MCSR_BIT_DCP_PERR  2               /*  Data cache push parity error                    */
#define ARCH_PPC_MCSR_BIT_DCPERR    3               /*  Data cache parity error                         */
#define ARCH_PPC_MCSR_BIT_GL_CI     15              /*  guarded load etc. using it for clear MCSR       */
#define ARCH_PPC_MCSR_BIT_BUS_IAERR 24              /*  Bus instruction address error                   */
#define ARCH_PPC_MCSR_BIT_BUS_RAERR 25              /*  Bus read address error                          */
#define ARCH_PPC_MCSR_BIT_BUS_WAERR 26              /*  Bus write address error                         */
#define ARCH_PPC_MCSR_BIT_BUS_IBERR 27              /*  Bus instruction data bus error                  */
#define ARCH_PPC_MCSR_BIT_BUS_RBERR 28              /*  Bus read data bus error                         */
#define ARCH_PPC_MCSR_BIT_BUS_WBERR 29              /*  Bus write bus error                             */
#define ARCH_PPC_MCSR_BIT_BUS_IPERR 30              /*  Bus instruction parity error                    */
#define ARCH_PPC_MCSR_BIT_BUS_RPERR 31              /*  Bus read parity error                           */
#else
                                                    /*  MCSR bit definitions                            */
#define ARCH_PPC_MCSR_BIT_MCP        0              /*  mchk input signal assert                        */
#define ARCH_PPC_MCSR_BIT_ICPERR     1              /*  i-cache/tag parity error                        */
#define ARCH_PPC_MCSR_BIT_DCPERR     2              /*  d-cache/tag parity error                        */
#define ARCH_PPC_MCSR_BIT_L2MMU_MHIT 4              /*  L2 MMU simultaneous hit                         */
#define ARCH_PPC_MCSR_BIT_NMI       11              /*  nonmaskable interrupt                           */
#define ARCH_PPC_MCSR_BIT_MAV       12              /*  MCAR valid                                      */
#define ARCH_PPC_MCSR_BIT_MEA       13              /*  MCAR effective address                          */
#define ARCH_PPC_MCSR_BIT_IF        15              /*  instr fetch error report                        */
#define ARCH_PPC_MCSR_BIT_LD        16              /*  load instr error report                         */
#define ARCH_PPC_MCSR_BIT_ST        17              /*  store instr error report                        */
#define ARCH_PPC_MCSR_BIT_LDG       18              /*  guarded ld instr error                          */
#define ARCH_PPC_MCSR_BIT_TLBSYNC   30              /*  simultaneous tlbsync                            */
#define ARCH_PPC_MCSR_BIT_BSL2_ERR  31              /*  L2 cache error                                  */
#endif                                              /*  __SYLIXOS_PPC_E500MC__                          */

/*********************************************************************************************************
  虚拟化相关的寄存器及定义
*********************************************************************************************************/

#ifdef  __SYLIXOS_PPC_E500MC__
                                                    /*  Guest O/S registers                             */
#define GSPRG0                      592             /*  Guest SPR General 0                             */
#define GSPRG1                      593             /*  Guest SPR General 1                             */
#define GSPRG2                      594             /*  Guest SPR General 2                             */
#define GSPRG3                      595             /*  Guest SPR General 3                             */
#define GESR                        596             /*  Guest Exception Syndrome Register               */
#define GEPR                        698             /*  Guest External Input Vector Register            */
#define GSRR0                       699             /*  Guest SRR0                                      */
#define GSRR1                       700             /*  Guest SRR1                                      */
#define GPIR                        701             /*  Guest PIR                                       */

                                                    /*  Ultravisor registers                            */
#define EPR                         702             /*  External Input Proxy Register (RO)              */
#define UVCSR                       703             /*  Ultravisor Control and Status Register          */
#define GIVPR                       912             /*  Guest Interrupt Vector Prefix Register          */
#define GIVOR2                      913             /*  Guest IVOR Data Storage                         */
#define GIVOR3                      914             /*  Guest IVOR Instruction Storage                  */
#define GIVOR4                      915             /*  Guest IVOR External Input                       */
#define GIVOR8                      918             /*  Guest IVOR System Call                          */
#define GIVOR13                     919             /*  Guest IVOR Data TLB Error                       */
#define GIVOR14                     920             /*  Guest IVOR Instruction TLB Error                */
#endif                                              /*  __SYLIXOS_PPC_E500MC__                          */

/*********************************************************************************************************
  SPE 相关的寄存器及定义
*********************************************************************************************************/

#define SPEFSCR                     512             /*  SPE Floating-pt Status and Control Register     */

                                                    /*  High word error bits                            */
#define ARCH_PPC_SPEFSCR_SOVH       0x80000000
#define ARCH_PPC_SPEFSCR_OVH        0x40000000
#define ARCH_PPC_SPEFSCR_FGH        0x20000000
#define ARCH_PPC_SPEFSCR_FXH        0x10000000
#define ARCH_PPC_SPEFSCR_FINVH      0x08000000
#define ARCH_PPC_SPEFSCR_FDBZH      0x04000000
#define ARCH_PPC_SPEFSCR_FUNFH      0x02000000
#define ARCH_PPC_SPEFSCR_FOVFH      0x01000000
                                                    /*  Status Bits                                     */
#define ARCH_PPC_SPEFSCR_FINXS      0x00200000
#define ARCH_PPC_SPEFSCR_FINVS      0x00100000
#define ARCH_PPC_SPEFSCR_FDBZS      0x00080000
#define ARCH_PPC_SPEFSCR_FUNFS      0x00040000
#define ARCH_PPC_SPEFSCR_FOVFS      0x00020000
#define ARCH_PPC_SPEFSCR_MODE       0x00010000

#define ARCH_PPC_SPEFSCR_SOV        0x00008000
#define ARCH_PPC_SPEFSCR_OV         0x00004000
#define ARCH_PPC_SPEFSCR_FG         0x00002000
#define ARCH_PPC_SPEFSCR_FX         0x00001000
#define ARCH_PPC_SPEFSCR_FINV       0x00000800
#define ARCH_PPC_SPEFSCR_FDBZ       0x00000400
#define ARCH_PPC_SPEFSCR_FUNF       0x00000200
#define ARCH_PPC_SPEFSCR_FOVF       0x00000100

#define ARCH_PPC_SPEFSCR_FINXE      0x00000040
#define ARCH_PPC_SPEFSCR_FINVE      0x00000020
#define ARCH_PPC_SPEFSCR_FDBZE      0x00000010
#define ARCH_PPC_SPEFSCR_FUNFE      0x00000008
#define ARCH_PPC_SPEFSCR_FOVFE      0x00000004

#define ARCH_PPC_SPEFSCR_FRMC_RND_NR    0x00000000
#define ARCH_PPC_SPEFSCR_FRMC_RND_ZERO  0x00000001
#define ARCH_PPC_SPEFSCR_FRMC_RND_PINF  0x00000002
#define ARCH_PPC_SPEFSCR_FRMC_RND_NINF  0x00000003

/*********************************************************************************************************
  通用的寄存器及定义
  SPRG0-SPRG3 are defined correctly in assembler.h
*********************************************************************************************************/

#define USPRG0                      256             /*  User Special Purpose Register General 0         */

#define SPRG4_R                     260             /*  Special Purpose Register General 4, read        */
#define SPRG4_W                     276             /*  Special Purpose Register General 4, write       */
#define SPRG5_R                     261             /*  Special Purpose Register General 5, read        */
#define SPRG5_W                     277             /*  Special Purpose Register General 5, write       */
#define SPRG6_R                     262             /*  Special Purpose Register General 6, read        */
#define SPRG6_W                     278             /*  Special Purpose Register General 6, write       */
#define SPRG7_R                     263             /*  Special Purpose Register General 7, read        */
#define SPRG7_W                     279             /*  Special Purpose Register General 7, write       */

/*********************************************************************************************************
  时间相关的寄存器及定义
*********************************************************************************************************/

#define DECAR                       54

#define TBL_R                       268             /*  Time Base Lower, read                           */
#define TBL_W                       284             /*  Time Base Lower, write                          */
#define TBU_R                       269             /*  Time Base Upper, read                           */
#define TBU_W                       285             /*  Time Base Upper, write                          */

#define TCR                         340             /*  Timer Control Register                          */
#define TSR                         336             /*  Timer Status Register                           */

                                                    /*  Bits in the upper half of TCR                   */
#define ARCH_PPC_TCR_WP_U           0xc000          /*  Watchdog Timer Period                           */
#define ARCH_PPC_TCR_WRC_U          0x3000          /*  Watchdog Timer Reset Control                    */
#define ARCH_PPC_TCR_WIE_U          0x0800          /*  Watchdog Timer Interrupt Enable                 */
#define ARCH_PPC_TCR_DIE_U          0x0400          /*  Decrementer Interrupt Enable                    */
#define ARCH_PPC_TCR_FP_U           0x0300          /*  Fixed Interval Timer Period                     */
#define ARCH_PPC_TCR_FIE_U          0x0080          /*  Fixed Interval Timer Interrupt Enable           */
#define ARCH_PPC_TCR_ARE_U          0x0040          /*  Decrementer Auto-Reload Enable                  */
#define ARCH_PPC_TCR_WPEXT_U        0x0040          /*  Decrementer Auto-Reload Enable                  */
#define ARCH_PPC_TCR_FPEXT_U        0x0040          /*  Decrementer Auto-Reload Enable                  */

                                                    /*  Bits in the upper half of TSR                   */
#define ARCH_PPC_TSR_ENW_U          0x8000          /*  Enable Next Watchdog Timer Exception            */
#define ARCH_PPC_TSR_WIS_U          0x4000          /*  Watchdog Timer Interrupt Status                 */
#define ARCH_PPC_TSR_WRS_U          0x3000          /*  Watchdog Timer Reset Status                     */
#define ARCH_PPC_TSR_DIS_U          0x0800          /*  Decrementer Interrupt Status                    */
#define ARCH_PPC_TSR_FIS_U          0x0400          /*  Fixed Interval Timer Interrupt Status           */

                                                    /*  versions of the aligned for 32-bit TCR/TSR REG  */
#define ARCH_PPC_TCR_DIE            (ARCH_PPC_TCR_DIE_U << 16)
#define ARCH_PPC_TSR_DIS            (ARCH_PPC_TSR_DIS_U << 16)

/*********************************************************************************************************
  调试相关的寄存器及定义
*********************************************************************************************************/

#ifdef  __SYLIXOS_PPC_E500MC__
#define DBSRWR                      564             /*  Debug Status Register Write Register            */
#define DSRR0                       574             /*  Debug SRR0                                      */
#define DSRR1                       575             /*  Debug SRR1                                      */
#endif                                              /*  __SYLIXOS_PPC_E500MC__                          */

#define DBCR0                       308             /*  Debug Control Register 0                        */
#define DBCR1                       309             /*  Debug Control Register 1                        */
#define DBCR2                       310             /*  Debug Control Register 2                        */

#define DBSR                        304             /*  Debug Status Register                           */

#define IAC1                        312             /*  Instr Address Compare Register 1                */
#define IAC2                        313             /*  Instr Address Compare Register 2                */

#define DAC1                        316             /*  Data Address Compare Register 1                 */
#define DAC2                        317             /*  Data Address Compare Register 2                 */

                                                    /*  Debug control register 0                        */
#define ARCH_PPC_DBCR0_IDM_U        0x4000          /*  internal debug mode                             */
#define ARCH_PPC_DBCR0_RST_U        0x3000          /*  reset                                           */
#define ARCH_PPC_DBCR0_ICMP_U       0x0800          /*  instruction completion debug event              */
#define ARCH_PPC_DBCR0_BRT_U        0x0400          /*  branch taken debug event                        */
#define ARCH_PPC_DBCR0_IRPT_U       0x0200          /*  interrupt debug event                           */
#define ARCH_PPC_DBCR0_TRAP_U       0x0100          /*  trap debug event                                */
#define ARCH_PPC_DBCR0_IAC1_U       0x0080          /*  instruction address compare 1                   */
#define ARCH_PPC_DBCR0_IAC2_U       0x0040          /*  instruction address compare 2                   */
#define ARCH_PPC_DBCR0_DAC1R_U      0x0008          /*  data address compare 1 Read                     */
#define ARCH_PPC_DBCR0_DAC1W_U      0x0004          /*  data address compare 1 Write                    */
#define ARCH_PPC_DBCR0_DAC2R_U      0x0002          /*  data address compare 2 Read                     */
#define ARCH_PPC_DBCR0_DAC2W_U      0x0001          /*  data address compare 2 Write                    */
#define ARCH_PPC_DBCR0_IDM          0x40000000      /*  internal debug mode                             */
#define ARCH_PPC_DBCR0_RST          0x30000000      /*  reset                                           */
#define ARCH_PPC_DBCR0_ICMP         0x08000000      /*  instruction completion debug event              */
#define ARCH_PPC_DBCR0_BRT          0x04000000      /*  branch taken                                    */
#define ARCH_PPC_DBCR0_IRPT         0x02000000      /*  exception debug event                           */
#define ARCH_PPC_DBCR0_TRAP         0x01000000      /*  trap debug event                                */
#define ARCH_PPC_DBCR0_IAC1         0x00800000      /*  instruction address compare 1                   */
#define ARCH_PPC_DBCR0_IAC2         0x00400000      /*  instruction address compare 2                   */
#define ARCH_PPC_DBCR0_DAC1R        0x00080000      /*  data address compare 1 Read                     */
#define ARCH_PPC_DBCR0_DAC1W        0x00040000      /*  data address compare 1 Write                    */
#define ARCH_PPC_DBCR0_DAC2R        0x00020000      /*  data address compare 2 Read                     */
#define ARCH_PPC_DBCR0_DAC2W        0x00010000      /*  data address compare 2 Write                    */
#define ARCH_PPC_DBCR0_RET          0x00008000      /*  return debug event                              */
#define ARCH_PPC_DBCR0_FT           0x00000001      /*  freeze timers on debug                          */

                                                    /*  Debug control register 1                        */
#define ARCH_PPC_DBCR1_IAC1US_U     0xc000          /*  IAC 1 User/Supervisor                           */
#define ARCH_PPC_DBCR1_IAC1ER_U     0x3000          /*  IAC 1 Effective/Real                            */
#define ARCH_PPC_DBCR1_IAC2US_U     0x0c00          /*  IAC 2 User/Supervisor                           */
#define ARCH_PPC_DBCR1_IAC2ER_U     0x0300          /*  IAC 2 Effective/Real                            */
#define ARCH_PPC_DBCR1_IAC12M_U     0x00c0          /*  IAC 1/2 Mode                                    */
#define ARCH_PPC_DBCR1_IAC12AT_U    0x0001          /*  IAC 1/2 Auto-Toggle Enable                      */
#define ARCH_PPC_DBCR1_IAC1US       0xc0000000      /*  IAC 1 User/Supervisor                           */
#define ARCH_PPC_DBCR1_IAC1ER       0x30000000      /*  IAC 1 Effective/Real                            */
#define ARCH_PPC_DBCR1_IAC2US       0x0c000000      /*  IAC 2 User/Supervisor                           */
#define ARCH_PPC_DBCR1_IAC2ER       0x03000000      /*  IAC 2 Effective/Real                            */
#define ARCH_PPC_DBCR1_IAC12M       0x00c00000      /*  IAC 1/2 Mode                                    */

                                                    /*  Debug control register 2                        */
#define ARCH_PPC_DBCR2_DAC1US_U     0xc000          /*  DAC 1 User/Supervisor                           */
#define ARCH_PPC_DBCR2_DAC1ER_U     0x3000          /*  DAC 1 Effective/Real                            */
#define ARCH_PPC_DBCR2_DAC2US_U     0x0c00          /*  DAC 2 User/Supervisor                           */
#define ARCH_PPC_DBCR2_DAC2ER_U     0x0300          /*  DAC 2 Effective/Real                            */
#define ARCH_PPC_DBCR2_DAC12M_U     0x00c0          /*  DAC 1/2 Mode                                    */
#define ARCH_PPC_DBCR2_DAC1US       0xc0000000      /*  DAC 1 User/Supervisor                           */
#define ARCH_PPC_DBCR2_DAC1ER       0x30000000      /*  DAC 1 Effective/Real                            */
#define ARCH_PPC_DBCR2_DAC2US       0x0c000000      /*  DAC 2 User/Supervisor                           */
#define ARCH_PPC_DBCR2_DAC2ER       0x03000000      /*  DAC 2 Effective/Real                            */
#define ARCH_PPC_DBCR2_DAC12M       0x00c00000      /*  DAC 1/2 Mode                                    */

                                                    /*  Debug status register                           */
#define ARCH_PPC_DBSR_IDE_U         0x8000          /*  Imprecise Debug Event                           */
#define ARCH_PPC_DBSR_UDE_U         0x4000          /*  Unconditional Debug Event                       */
#define ARCH_PPC_DBSR_MRR_U         0x3000          /*  Most Recent Reset                               */
#define ARCH_PPC_DBSR_ICMP_U        0x0800          /*  Instruction Completion Debug Event              */
#define ARCH_PPC_DBSR_BRT_U         0x0400          /*  Branch Taken Debug Event                        */
#define ARCH_PPC_DBSR_IRPT_U        0x0200          /*  Interrupt Debug Event                           */
#define ARCH_PPC_DBSR_TRAP_U        0x0100          /*  Trap Debug Event                                */
#define ARCH_PPC_DBSR_IAC1_U        0x0080          /*  IAC 1 Debug Event                               */
#define ARCH_PPC_DBSR_IAC2_U        0x0040          /*  IAC 2 Debug Event                               */
#define ARCH_PPC_DBSR_DAC1R_U       0x0008          /*  DAC/DVC 1 Read Debug Event                      */
#define ARCH_PPC_DBSR_DAC1W_U       0x0004          /*  DAC/DVC 1 Write Debug Event                     */
#define ARCH_PPC_DBSR_DAC2R_U       0x0002          /*  DAC/DVC 2 Read Debug Event                      */
#define ARCH_PPC_DBSR_DAC2W_U       0x0001          /*  DAC/DVC 2 Write Debug Event                     */
#define ARCH_PPC_DBSR_IDE           0x80000000      /*  Imprecise Debug Event                           */
#define ARCH_PPC_DBSR_UDE           0x40000000      /*  Unconditional Debug Event                       */
#define ARCH_PPC_DBSR_MRR           0x30000000      /*  Most Recent Reset                               */
#define ARCH_PPC_DBSR_ICMP          0x08000000      /*  Instruction Completion Debug Event              */
#define ARCH_PPC_DBSR_BRT           0x04000000      /*  Branch Taken Debug Event                        */
#define ARCH_PPC_DBSR_IRPT          0x02000000      /*  Interrupt Debug Event                           */
#define ARCH_PPC_DBSR_TRAP          0x01000000      /*  Trap Debug Event                                */
#define ARCH_PPC_DBSR_IAC1          0x00800000      /*  IAC 1 Debug Event                               */
#define ARCH_PPC_DBSR_IAC2          0x00400000      /*  IAC 2 Debug Event                               */
#define ARCH_PPC_DBSR_DAC1R         0x00080000      /*  DAC/DVC 1 Read Debug Event                      */
#define ARCH_PPC_DBSR_DAC1W         0x00040000      /*  DAC/DVC 1 Write Debug Event                     */
#define ARCH_PPC_DBSR_DAC2R         0x00020000      /*  DAC/DVC 2 Read Debug Event                      */
#define ARCH_PPC_DBSR_DAC2W         0x00010000      /*  DAC/DVC 2 Write Debug Event                     */
#define ARCH_PPC_DBSR_RET           0x00008000      /*  Return Debug Event                              */

                                                    /*  Mask for hardware breakpoints                   */
#define ARCH_PPC_DBSR_HWBP_MSK      (ARCH_PPC_DBSR_IAC1  | ARCH_PPC_DBSR_IAC2  | \
                                     ARCH_PPC_DBSR_DAC1R | ARCH_PPC_DBSR_DAC1W | \
                                     ARCH_PPC_DBSR_DAC2R | ARCH_PPC_DBSR_DAC2W)

/*********************************************************************************************************
  L1-Cache 相关的寄存器及定义
*********************************************************************************************************/

#define L1CFG0                      515             /*  L1 Config Register 0                            */
#define L1CFG1                      516             /*  L1 Config Register 1                            */

#define L1CSR0                      1010            /*  L1 Control Status Register 0                    */
#define L1CSR1                      1011            /*  L1 Control Status Register 1                    */

#ifdef  __SYLIXOS_PPC_E500MC__
#define L1CSR2                      606             /*  L1 Control Status Register 2                    */
#endif                                              /*  __SYLIXOS_PPC_E500MC__                          */

#ifdef  __SYLIXOS_PPC_E500MC__
#define ARCH_PPC_L1CSR2_DCWS        0x40000000      /*  Data cache write shadow                         */
#endif                                              /*  __SYLIXOS_PPC_E500MC__                          */

#ifdef  __SYLIXOS_PPC_E200__
#define L1FINV0                     1016
#endif                                              /*  __SYLIXOS_PPC_E200__                            */

                                                    /* Instruction and D-Cache bit fields are the same  */
#ifndef __SYLIXOS_PPC_E200__
#define ARCH_PPC_L1CSR_E            0x00000001      /*  Enable                                          */
#define ARCH_PPC_L1CSR_FI           0x00000002      /*  Flash Invalidate                                */
#define ARCH_PPC_L1CSR_FLR          0x00000100      /*  Lock Bits Flash                                 */
#define ARCH_PPC_L1CSR_LO           0x00000200      /*  Lock Overflow                                   */
#define ARCH_PPC_L1CSR_UL           0x00000400      /*  Unable to lock   - status bit                   */
#define ARCH_PPC_L1CSR_UL_V(x)      (x >> 10)
#define ARCH_PPC_L1CSR_SLC          0x00000800      /*  Snoop lock clear  - status bit                  */
#define ARCH_PPC_L1CSR_SLC_V(x)     (x >> 11)
#define ARCH_PPC_L1CSR_PIE          0x00008000      /*  Parity Injection Enable                         */
#define ARCH_PPC_L1CSR_CPE          0x00010000      /*  Parity Enable                                   */

                                                    /* Instruction and D-Cache bit fields are the same  */
#define ARCH_PPC_L1CFG_SIZE_MASK    0x00000FFF
#define ARCH_PPC_L1CFG_NWAY_MASK    0x000FF000
#define ARCH_PPC_L1CFG_NWAY_V(x)    (x >> 12)
#define ARCH_PPC_L1CFG_PA_MASK      0x00100000
#define ARCH_PPC_L1CFG_PA_V(x)      (x >> 16)
#define ARCH_PPC_L1CFG_LA_MASK      0x00200000
#define ARCH_PPC_L1CFG_LA_V(x)      (x >> 17)
#define ARCH_PPC_L1CFG_REPL_MASK    0x00400000
#define ARCH_PPC_L1CFG_REPL_V(x)    (x >> 18)
#define ARCH_PPC_L1CFG_BSIZE_MASK   0x01800000
#define ARCH_PPC_L1CFG_BSIZE_V(x)   (x >> 19)
#define ARCH_PPC_L1CFG_CARCH_MASK   0xC0000000      /*  L1CFG0 only                                     */
#define ARCH_PPC_L1CFG_CARCH_V(x)   (x >> 30)

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)
typedef union {
#if 0                                               /*  See <PowerPC e500 CoreFamily Reference Manual>  */
    struct {
        UINT    L1CFG0_ucCARCH     :  2;
        UINT                       :  5;
        UINT    L1CFG0_ucCBSIZE    :  2;
        UINT    L1CFG0_ucCREPL     :  2;
        UINT    L1CFG0_bCLA        :  1;
        UINT    L1CFG0_bCPA        :  1;
        UINT                       :  5;
        UINT    L1CFG0_ucCNWAY     :  3;
        UINT                       :  3;
        UINT    L1CFG0_ucCSIZE     :  8;
    };
#else
    /*
     * See <A Programmer’s Reference Manual for Freescale Power Architecture Processors
     * Supports e500 core family (e500v1, e500v2, e500mc, e5500, e6500)
     * e200 core family >
     */
    struct {
        UINT    L1CFG0_ucCARCH     :  2;
        UINT    L1CFG0_bCWPA       :  1;
        UINT    L1CFG0_bCFAHA      :  1;
        UINT    L1CFG0_bCFISWA     :  1;
        UINT                       :  2;
        UINT    L1CFG0_ucCBSIZE    :  2;
        UINT    L1CFG0_ucCREPL     :  2;
        UINT    L1CFG0_bCLA        :  1;
        UINT    L1CFG0_bCPA        :  1;
        UINT    L1CFG0_ucCNWAY     :  8;
        UINT    L1CFG0_usCSIZE     : 11;
    };
#endif
    UINT32      L1CFG0_uiValue;
} E500_L1CFG0;

typedef union {
    struct {
        UINT                       :  7;
        UINT    L1CFG1_ucCBSIZE    :  2;
        UINT    L1CFG1_ucCREPL     :  2;
        UINT    L1CFG1_bCLA        :  1;
        UINT    L1CFG1_bCPA        :  1;
        UINT    L1CFG1_ucCNWAY     :  8;
        UINT    L1CFG1_ucCSIZE     : 11;
    };
    UINT32      L1CFG1_uiValue;
} E500_L1CFG1;
#endif                                              /*  __ASSEMBLY__                                    */

#else

#define ARCH_PPC_L1CSR_WID_MSK      0xf0000000
#define ARCH_PPC_L1CSR_WID_SHFT     28
#define ARCH_PPC_L1CSR_WDD_MSK      0x0f000000
#define ARCH_PPC_L1CSR_WDD_SHFT     24
#define ARCH_PPC_L1CSR_WID_SET(x)   ((x<<ARCH_PPC_L1CSR_WID_SHFT) & ARCH_PPC_L1CSR_WID_MSK)
#define ARCH_PPC_L1CSR_WDD_SET(x)   ((x<<ARCH_PPC_L1CSR_WDD_SHFT) & ARCH_PPC_L1CSR_WDD_MSK)
#define ARCH_PPC_L1CSR_WID_GET(x)   ((x & ARCH_PPC_L1CSR_WID_MSK)>>ARCH_PPC_L1CSR_WID_SHFT)
#define ARCH_PPC_L1CSR_WDD_GET(x)   ((x & ARCH_PPC_L1CSR_WDD_MSK)>>ARCH_PPC_L1CSR_WDD_SHFT)

#define ARCH_PPC_L1CSR_AWID         0x00800000
#define ARCH_PPC_L1CSR_AWDD         0x00400000
#define ARCH_PPC_L1CSR_CWM          0x00100000
#define ARCH_PPC_L1CSR_DPB          0x00080000
#define ARCH_PPC_L1CSR_DSB          0x00040000
#define ARCH_PPC_L1CSR_DSTRM        0x00020000
#define ARCH_PPC_L1CSR_CPE          0x00010000
#define ARCH_PPC_L1CSR_CUL          0x00000800
#define ARCH_PPC_L1CSR_CLO          0x00000400
#define ARCH_PPC_L1CSR_CLFC         0x00000200
#define ARCH_PPC_L1CSR_CABT         0x00000004
#define ARCH_PPC_L1CSR_CINV         0x00000002
#define ARCH_PPC_L1CSR_CE           0x00000001

#define ARCH_PPC_L1CFG_SIZE_MASK    0x000007FF
#define ARCH_PPC_L1CFG_NWAY_MASK    0x007FF100
#define ARCH_PPC_L1CFG_NWAY_V(x)    (x >> 11)
#define ARCH_PPC_L1CFG_PA_MASK      0x00080000
#define ARCH_PPC_L1CFG_PA_V(x)      (x >> 19)
#define ARCH_PPC_L1CFG_LA_MASK      0x00100000
#define ARCH_PPC_L1CFG_LA_V(x)      (x >> 20)
#define ARCH_PPC_L1CFG_REPL_MASK    0x00600000
#define ARCH_PPC_L1CFG_REPL_V(x)    (x >> 21)
#define ARCH_PPC_L1CFG_BSIZE_MASK   0x01800000
#define ARCH_PPC_L1CFG_BSIZE_V(x)   (x >> 23)
#define ARCH_PPC_L1CFG_CFISWA_MASK  0x08000000
#define ARCH_PPC_L1CFG_CFAHA_MASK   0x10000000
#define ARCH_PPC_L1CFG_CWPA_MASK    0x20000000
#define ARCH_PPC_L1CFG_CARCH_MASK   0xC0000000
#define ARCH_PPC_L1CFG_CARCH_V(x)   (x >> 30)

#define ARCH_PPC_L1FINV0_CWAY_MSK       0x07000000
#define ARCH_PPC_L1FINV0_CWAY_SET(x)    ((x << 24) & ARCH_PPC_L1FINV0_CWAY_MSK)
#define ARCH_PPC_L1FINV0_CSET_MSK       0x00000FC0
#define ARCH_PPC_L1FINV0_CSET_SET(x)    ((x << 6) & ARCH_PPC_L1FINV0_CSET_MSK)
#define ARCH_PPC_L1FINV0_CCMD_MSK       0x00000003
#define ARCH_PPC_L1FINV0_CCMD_SET(x)    (x & ARCH_PPC_L1FINV0_CCMD_MSK)

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)
typedef union {
    struct {
        UINT    L1CFG0_ucCARCH     :  2;
        UINT    L1CFG0_bCWPA       :  1;
        UINT    L1CFG0_bCFAHA      :  1;
        UINT    L1CFG0_bCFISWA     :  1;
        UINT                       :  2;
        UINT    L1CFG0_ucCBSIZE    :  2;
        UINT    L1CFG0_ucCREPL     :  2;
        UINT    L1CFG0_bCLA        :  1;
        UINT    L1CFG0_bCPA        :  1;
        UINT    L1CFG0_ucCNWAY     :  8;
        UINT    L1CFG0_usCSIZE     : 11;
    };
    UINT32      L1CFG0_uiValue;
} E200_L1CFG0;
#endif                                              /*  __ASSEMBLY__                                    */

#endif                                              /*  __SYLIXOS_PPC_E200__                            */

/*********************************************************************************************************
  L2-Cache 相关的寄存器及定义
*********************************************************************************************************/

#ifdef __SYLIXOS_PPC_E500MC__
#define L2CAPTDATAHI                988             /*  L2 Error Capture Data High                      */
#define L2CAPTDATALO                989             /*  L2 Error Capture Data Low                       */
#define L2CAPTECC                   990             /*  L2 Error Capture ECC Syndrome                   */
#define L2CFG0                      519             /*  L2 Configuration Register                       */
#define L2CSR0                      1017            /*  L2 Control and Status Register 0                */
#define L2CSR1                      1018            /*  L2 Control and Status Register 1                */
#define L2ERRADDR                   722             /*  L2 Error Address                                */
#define L2ERRATTR                   721             /*  L2 Error Attribute                              */
#define L2ERRCTL                    724             /*  L2 Error Control                                */
#define L2ERRDET                    991             /*  L2 Error Detect                                 */
#define L2ERRDIS                    725             /*  L2 Error Disable                                */
#define L2ERREADDR                  723             /*  L2 Error Extended Address                       */
#define L2ERRINJCTL                 987             /*  L2 Error Injection Control                      */
#define L2ERRINJHI                  985             /*  L2 Error Injection Mask High                    */
#define L2ERRINJLO                  986             /*  L2 Error Injection Mask Low                     */
#define L2ERRINTEN                  720             /*  L2 Error Interrupt Enable                       */
#endif                                              /*  __SYLIXOS_PPC_E500MC__                          */

/*********************************************************************************************************
  MMU 相关的寄存器及定义
*********************************************************************************************************/

#define MAS0                        624             /*  MMU Assist Register 0                           */
#define MAS1                        625             /*  MMU Assist Register 1                           */
#define MAS2                        626             /*  MMU Assist Register 2                           */
#define MAS3                        627             /*  MMU Assist Register 3                           */
#define MAS4                        628             /*  MMU Assist Register 4                           */
#define MAS5                        339             /*  MMU Assist Register 5 (E500MC)                  */
#define MAS6                        630             /*  MMU Assist Register 6                           */
#define MAS7                        944             /*  MMU Assist Register 7 (E500MC E500V2)           */
#define MAS8                        341             /*  MMU Assist Register 8 (E500MC)                  */

#define TLB0CFG                     688             /*  TLB 0 Config Register                           */
#define TLB1CFG                     689             /*  TLB 1 Config Register                           */

#define MMUCSR0                     1012            /*  MMU Control Status Register 0                   */
#define MMUCFG                      1015            /*  MMU Config Register                             */

#ifdef  __SYLIXOS_PPC_E500MC__
#define LPIDR                       338             /*  Logical Partition ID Register                   */
#endif                                              /*  __SYLIXOS_PPC_E500MC__                          */

#define PID                         48
#define PID_MASK                    0x0FF
#define PID0                        48
#define PID1                        633             /*  (E500MC)                                        */
#define PID2                        634             /*  (E500MC)                                        */

#define ARCH_PPC_MMUCSR0_L2TLB1_FI      0x00000002
#define ARCH_PPC_MMUCSR0_L2TLB1_FI_V(x) (x >> 1)
#define ARCH_PPC_MMUCSR0_L2TLB0_FI      0x00000004
#define ARCH_PPC_MMUCSR0_L2TLB0_FI_V(x) (x >> 2)
#define ARCH_PPC_MMUCSR0_DL1MMU_FI      0x00000008
#define ARCH_PPC_MMUCSR0_DL1MMU_FI_V(x) (x >> 3)
#define ARCH_PPC_MMUCSR0_IL1MMU_FI      0x00000010
#define ARCH_PPC_MMUCSR0_IL1MMU_FI_V(x) (x >> 4)

#define ARCH_PPC_MAS0_NV            0x00000000
#define ARCH_PPC_MAS0_ESEL_MASK     0x03ff0000
#define ARCH_PPC_MAS0_ESEL_BIT      16
#define ARCH_PPC_MAS0_ESEL_V(x)     (x >> ARCH_PPC_MAS0_ESEL_BIT)
#define ARCH_PPC_MAS0_TLBSEL1       0x10000000
#define ARCH_PPC_MAS0_TLBSEL_MASK   0x30000000

/*********************************************************************************************************
  Range of hardware context numbers (PID register & TLB TID field)
*********************************************************************************************************/

#define MMU_ASID_MIN                1
#define MMU_ASID_MAX                255
#define MMU_ASID_GLOBAL             MMU_ASID_MIN

/*********************************************************************************************************
  分支预测的寄存器及定义
*********************************************************************************************************/

#define BUCSR                       1013

#define ARCH_PPC_BUCSR_FI           0x200           /*  Invalidate branch cache                         */
#define ARCH_PPC_BUCSR_E            0x1             /*  Enable branch prediction                        */

/*********************************************************************************************************
  实现相关的寄存器及定义
*********************************************************************************************************/

#define HID0                        1008
#define HID1                        1009            /*  (E500MC)                                        */

                                                    /*  hardware dependent register 0                   */
#define ARCH_PPC_HID0_DOZE_U        0x0080          /*  DOZE power management mode                      */
#define ARCH_PPC_HID0_NAP_U         0x0040          /*  NAP power management mode                       */
#define ARCH_PPC_HID0_SLEEP_U       0x0020          /*  SLEEP power management mode                     */
#define ARCH_PPC_HID0_DOZE          0x00800000      /*  DOZE power management mode                      */
#define ARCH_PPC_HID0_NAP           0x00400000      /*  NAP power management mode                       */
#define ARCH_PPC_HID0_SLEEP         0x00200000      /*  SLEEP power management mode                     */
#define ARCH_PPC_HID0_TBEN          0x00004000      /*  time base enable                                */

                                                    /*  The following (E500MC E500V2)                   */
#define ARCH_PPC_HID0_MAS7EN        0x00000080      /*  Enable use of MAS7 for tlbre                    */
#define ARCH_PPC_HID0_DCFA          0x00000040      /*  Use this bit to flush only valid entries        */
#define ARCH_PPC_HID0_BIT_MAS7EN    24
#define ARCH_PPC_HID0_BIT_DCFA      25
#define ARCH_PPC_85XX_USE_DCFA

                                                    /*  hardware dependent register 1                   */
#define ARCH_PPC_HID1_ABE           0x00001000      /*  Address broadcast enable                        */

/*********************************************************************************************************
  MSR 相关的定义
*********************************************************************************************************/
/*********************************************************************************************************
  MSR (upper half) definitions
  arch_def.h defines a generic MSR.  Here we define changes from that base
*********************************************************************************************************/

#undef  ARCH_PPC_MSR_SF_U                           /*  64 bit mode not implemented                     */
#undef  ARCH_PPC_MSR_ILE_U                          /*  little endian mode not supported                */

/*********************************************************************************************************
  wait state enable is equivalent to the power down enable for 60x
*********************************************************************************************************/

#define ARCH_PPC_MSR_WE_U           0x0004          /*  wait state enable                               */

#undef  ARCH_PPC_MSR_POW_U
#define ARCH_PPC_MSR_POW_U          ARCH_PPC_MSR_WE_U

#define ARCH_PPC_MSR_WE             0x00040000      /*  wait state enable                               */

#define ARCH_PPC_MSR_CE_U           0x0002          /*  critical interrupt enable                       */
#define ARCH_PPC_MSR_CE             0x00020000      /*  critical interrupt enable                       */

/*********************************************************************************************************
  MSR (lower half) definitions
*********************************************************************************************************/

#undef  ARCH_PPC_MSR_SE                             /*  single step unsupported                         */
#undef  ARCH_PPC_MSR_BE                             /*  branch trace not supported                      */
#undef  ARCH_PPC_MSR_IP                             /*  exception prefix bit not supported              */
#undef  ARCH_PPC_MSR_RI                             /*  recoverable interrupt unsupported               */
#undef  ARCH_PPC_MSR_LE                             /*  little endian mode unsupported                  */
#undef  ARCH_PPC_MSR_BIT_LE                         /*  little endian mode unsupported                  */

/*********************************************************************************************************
  Machine check exception class is new to PPC in E500.  Although
  bit position is same as classic MSR[ME] and is named the same,
  the define of ARCH_PPC_MSR_MCE signifies the present of this
  class of exception.  If present, both ARCH_PPC_MSR_MCE and
  ARCH_PPC_MSR_ME should be defined to the mask of 0x1000.
  In addition, Critical Exception Class is also a requirement.
  The critical exception code stub does not mask exceptions
  and is used for machine check exception class as well.
  Therefore, XXX should be defined.
*********************************************************************************************************/

#ifndef __SYLIXOS_PPC_E200__
#define ARCH_PPC_MSR_MCE            ARCH_PPC_MSR_ME /*  machine check enable                            */
#else
#undef  ARCH_PPC_MSR_MCE
#endif                                              /*  __SYLIXOS_PPC_E200__                            */

#define ARCH_PPC_MSR_DE             0x0200          /*  debug exception enable                          */
#define ARCH_PPC_MSR_IS             0x0020          /*  insn address space selector                     */
#define ARCH_PPC_MSR_DS             0x0010          /*  data address space selector                     */

#define ARCH_PPC_MSR_BIT_WE         13
#define ARCH_PPC_MSR_BIT_CE         14
#define ARCH_PPC_MSR_BIT_DE         22
#define ARCH_PPC_MSR_BIT_IS         26
#define ARCH_PPC_MSR_BIT_DS         27

/*********************************************************************************************************
  ARCH_PPC_INT_MASK definition (mask EE & CE bits) : overwrite the one in arch_def.h
*********************************************************************************************************/

#if 0                                               /*  不关闭临界中断                                  */
#undef  ARCH_PPC_INT_MASK
#define ARCH_PPC_INT_MASK(src, des)         \
        RLWINM  des, src, 0, ARCH_PPC_MSR_BIT_EE + 1, ARCH_PPC_MSR_BIT_EE - 1; \
        RLWINM  des, des, 0, ARCH_PPC_MSR_BIT_CE + 1, ARCH_PPC_MSR_BIT_CE - 1
#endif

#undef  ARCH_PPC_INT_WE_MASK
#define ARCH_PPC_INT_WE_MASK(src, des)      \
        RLWINM  des, src, 0, ARCH_PPC_MSR_BIT_WE + 1, ARCH_PPC_MSR_BIT_WE - 1

                                                    /*  E500core other than e500mc has no FPU           */
#define ARCH_PPC_MSR_BIT_FP         18
#define ARCH_PPC_MSR_BIT_FE0        20
#define ARCH_PPC_MSR_BIT_FE1        23

#define ARCH_PPC_MSR_SPE_U          0x0200
#define ARCH_PPC_MSR_SPE            0x02000000

#endif                                              /*  __PPC_ARCH_E500_H                               */
/*********************************************************************************************************
  END
*********************************************************************************************************/

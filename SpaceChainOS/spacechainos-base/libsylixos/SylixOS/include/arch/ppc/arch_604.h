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
** 文   件   名: arch_604.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 01 月 25 日
**
** 描        述: PowerPC 604 相关定义.
*********************************************************************************************************/
/*********************************************************************************************************
  本头文件适用于如下 PowerPC 处理器:
  PPC604, PPC750, PPC604E, PPC604R, MPC7400, MPC755, MPC745, PPC750FX, MPC7450, MPC7440, MPC7441
  MPC7455  MPC7445, MPC7457  MPC7447, MPC7447A, MPC7448, MPC7410
  包括这个头文件前, 应该定义 __SYLIXOS_PPC_CPU__ 为 __SYLIXOS_PPC604__, 即处理器家族为 PowerPC 604 家族
  然后再根据实际的处理器, 定义处理器识别符, 如:
  __SYLIXOS_PPC745X__, __SYLIXOS_PPC750__, __SYLIXOS_PPC_HAVE_ALTIVEC
*********************************************************************************************************/

#ifndef __PPC_ARCH_604_H
#define __PPC_ARCH_604_H

/*********************************************************************************************************
  Special Purpose Register PowerPC604 specific
*********************************************************************************************************/

#define IMMR                        638             /*  Internal mem map reg - from 82xx slave SIU      */

#undef  ASR                                         /*  64 bit implementation only                      */

#define MMCR0                       952             /*  Monitor Mode Control Register 0                 */
#define PMC1                        953             /*  Performance Monitor Counter Register 1          */
#define PMC2                        954             /*  Performance Monitor Counter Register 2          */
#define SIA                         955             /*  Sampled Instruction Address Register            */
#define SDA                         959             /*  Sampled Data Address Register                   */
#define HID0                        1008            /*  hardware implementation register 0              */
#define HID1                        1009            /*  hardware implementation register 1              */
#define HID2                        1011            /*  hardware implementation register 2 (MPC755)     */
#define IABR                        1010            /*  Instruction Address Breakpoint Register         */
#define DABR                        1013            /*  Data Address Breakpoint Register                */

#ifdef __SYLIXOS_PPC745X__
#define PIR                         1023            /*  Processor Identification Register               */
#define ICTC                        1019            /*  Instruction cache throttling                    */
#endif                                              /*  __SYLIXOS_PPC745X__                             */

#define SVR                         286             /*  system version register                         */
#define PVR                         287             /*  processor version register                      */

#if __SYLIXOS_PPC_HAVE_ALTIVEC > 0                  /*  MSSCR0 exists only in MPC74xx/8641              */
#define MSSCR0                      1014            /*  Memory Subsystem Control Register               */
#endif                                              /*  __SYLIXOS_PPC_HAVE_ALTIVEC > 0                  */

/*********************************************************************************************************
  MSSCR0 definitions
*********************************************************************************************************/

#if __SYLIXOS_PPC_HAVE_ALTIVEC > 0
#ifdef __SYLIXOS_PPC745X__
/*********************************************************************************************************
  The following MSSCR0 fields exist in MPC744x, MPC745x, and MPC8641
  (PVR=800xxxxx).  We use the ARCH_PPC_MSSCR0_ID bit to recognize CPU 0
  in the bootrom, and define the other fields here for completeness.
*********************************************************************************************************/

#define ARCH_PPC_MSSCR0_DTQ_U       0x1c00          /*  DTQ size                                        */
#define ARCH_PPC_MSSCR0_EIDIS_U     0x0100          /*  disable external MPX intervention               */
#define ARCH_PPC_MSSCR0_L3TCEXT_U   0x0020          /*  L3 turnaround clock count (7457)                */
#define ARCH_PPC_MSSCR0_ABD_U       0x0010          /*  Address Bus Driven mode                         */
#define ARCH_PPC_MSSCR0_L3TCEN_U    0x0008          /*  L3 turnaround clock enable (7457)               */
#define ARCH_PPC_MSSCR0_L3TC_U      0x0006          /*  L3 turnaround clock count (7457)                */
#define ARCH_PPC_MSSCR0_BMODE       0xc000          /*  Bus Mode                                        */
#define ARCH_PPC_MSSCR0_ID          0x0020          /*  Set if SMP CPU ID != 0                          */
#define ARCH_PPC_MSSCR0_BIT_ID      26
#define ARCH_PPC_MSSCR0_L2PFE       0x0003          /*  # of L2 prefetch engines enabled                */

#else                                               /*  __SYLIXOS_PPC745X__                             */
/*********************************************************************************************************
  The following MSSCR0 fields exist only in MPC7400 (PVR=000Cxxxx)
  and MPC7410 (PVR=800Cxxxx).  We use the DL1HWF bit in cacheALib.s,
  and define the other fields here for completeness.  All are in the
  MS half of the register.
*********************************************************************************************************/

#define ARCH_PPC_MSSCR0_SHDEN_U     0x8000          /*  Shared-state (MESI) enable                      */
#define ARCH_PPC_MSSCR0_SHDPEN3_U   0x4000          /*  MEI mode SHD0/SHD1 signal enable                */
#define ARCH_PPC_MSSCR0_L1_INTVEN_U 0x3800          /*  L1 data cache HIT intervention                  */
#define ARCH_PPC_MSSCR0_L2_INTVEN_U 0x0700          /*  L2 data cache HIT intervention                  */
#define ARCH_PPC_MSSCR0_DL1HWF_U    0x0080          /*  L1 data cache hardware flush                    */
#define ARCH_PPC_MSSCR0_BIT_DL1HWF  8
#define ARCH_PPC_MSSCR0_EMODE_U     0x0020          /*  MPX bus mode (read-only)                        */
#define ARCH_PPC_MSSCR0_ABD_U       0x0010          /*  Address bus driven (read-only)                  */
#endif                                              /*  __SYLIXOS_PPC745X__                             */
#endif                                              /*  __SYLIXOS_PPC_HAVE_ALTIVEC > 0                  */

/*********************************************************************************************************
  HID definitions
*********************************************************************************************************/

#define ARCH_PPC_HID0_EMCP          0x80000000      /*  Enable machine check pin                        */
#define ARCH_PPC_HID0_ECPC          0x40000000      /*  Enable cache parity check                       */
#define ARCH_PPC_HID0_EBA           0x20000000      /*  Enable address bus parity                       */
#define ARCH_PPC_HID0_EBD           0x10000000      /*  Enable data bus parity                          */

#ifdef __SYLIXOS_PPC745X__
#define ARCH_PPC_HID0_TBEN          0x04000000      /*  Enable timebase & decr                          */
#define ARCH_PPC_HID0_NAP_U         0x0040          /*  NAP power management mode                       */
#define ARCH_PPC_HID0_SLEEP_U       0x0020          /*  SLEEP power management mode                     */
#define ARCH_PPC_HID0_DPM_U         0x0010          /*  enable dynamic power management                 */
#define ARCH_PPC_HID0_NAP           0x00400000      /*  NAP power management mode                       */
#define ARCH_PPC_HID0_SLEEP         0x00200000      /*  SLEEP power management mode                     */
#define ARCH_PPC_HID0_DPM           0x00100000      /*  enable dynamic power management                 */
#endif                                              /*  __SYLIXOS_PPC745X__                             */

#define ARCH_PPC_HID0_PAR           0x01000000
#define ARCH_PPC_HID0_NHR           0x00010000      /*  Not hard reset                                  */
#define ARCH_PPC_HID0_ICE           0x00008000      /*  inst cache enable                               */
#define ARCH_PPC_HID0_DCE           0x00004000      /*  data cache enable                               */
#define ARCH_PPC_HID0_ILOCK         0x00002000      /*  inst cache lock                                 */
#define ARCH_PPC_HID0_DLOCK         0x00001000      /*  data cache lock                                 */
#define ARCH_PPC_HID0_ICFI          0x00000800      /*  inst cache flash invalidate                     */
#define ARCH_PPC_HID0_DCFI          0x00000400      /*  data cache flash invalidate                     */
#define ARCH_PPC_HID0_SIED          0x00000080      /*  serial instr exec disable                       */
#define ARCH_PPC_HID0_DCFA          0x00000040      /*  dCache flush assist on 7xx                      */
#define ARCH_PPC_HID0_BHTE          0x00000004      /*  branch history table enable                     */

#define ARCH_PPC_HID0_XBSEN         0x0100          /*  Extended Block Size enable                      */

#ifdef __SYLIXOS_PPC745X__
#define ARCH_PPC_HID0_TBEN_U        0x0400          /*  Enable timebase & decr                          */
#define ARCH_PPC_HID1_EMCP          0x80000000      /*  Enable machine check pin                        */
#define ARCH_PPC_HID1_EBA           0x20000000      /*  Enable address bus parity                       */
#define ARCH_PPC_HID1_EBD           0x10000000      /*  Enable data bus parity                          */
#define ARCH_PPC_HID1_DSF4          0x00400000      /*  Dynamic frequency scaling                       */
#define ARCH_PPC_HID1_DSF2          0x00200000      /*  Dynamic frequency scaling                       */
#define ARCH_PPC_HID1_PC5           0x00040000      /*  PLL configuration bit 5                         */
#define ARCH_PPC_HID1_PC0           0x00020000      /*  PLL configuration bit 0                         */
#define ARCH_PPC_HID1_PC1           0x00008000      /*  PLL configuration bit 1                         */
#define ARCH_PPC_HID1_PC2           0x00004000      /*  PLL configuration bit 2                         */
#define ARCH_PPC_HID1_PC3           0x00002000      /*  PLL configuration bit 3                         */
#define ARCH_PPC_HID1_PC4           0x00001000      /*  PLL configuration bit 4                         */
#define ARCH_PPC_HID1_SYNCBE        0x00000800      /*  addr. broadcast for sync, eieio                 */
#define ARCH_PPC_HID1_ABE           0x00000400      /*  addr. broadcast enable                          */
#define ARCH_PPC_ICTC_FI_MASK       0x000001FE      /*  Instruction forwarding interval                 */
#define ARCH_PPC_ICTC_FI_45         0x0000005A      /*  fetch instr. every 45 cycles                    */
#define ARCH_PPC_ICTC_FI_85         0x000000AA      /*  fetch instr. every 85 cycles                    */
#define ARCH_PPC_ICTC_FI_130        0x00000104      /*  fetch instr. every 130 cycles                   */
#define ARCH_PPC_ICTC_FI_175        0x0000015E      /*  fetch instr. every 175 cycles                   */
#define ARCH_PPC_ICTC_FI_210        0x000001A4      /*  fetch instr. every 210 cycles                   */
#define ARCH_PPC_ICTC_FI_255        0x000001FE      /*  fetch instr. every 255 cycles                   */
#define ARCH_PPC_ICTC_FI_EN         0x00000001      /*  Instruction cache throttling enable             */
#endif                                              /*  __SYLIXOS_PPC745X__                             */

#define ARCH_PPC_HID0_HIGH_BAT_EN_U 0x0080          /*  High Bat enable on MPC7455                      */
#define ARCH_PPC_HID2_HIGH_BAT_EN_U 0x0004          /*  High Bat enable on MPC755                       */

                                                    /*  HID bit definitions                             */
#define ARCH_PPC_HID0_BIT_ICE       16              /*  HID0 ICE bit for 604                            */
#define ARCH_PPC_HID0_BIT_DCE       17              /*  HID0 DCE bit for 604                            */
#define ARCH_PPC_HID0_BIT_ILOCK     18              /*  HID0 ILOCK bit for 604                          */
#define ARCH_PPC_HID0_BIT_DLOCK     19              /*  HID0 DLOCK bit for 604                          */
#define ARCH_PPC_HID0_BIT_ICFI      20              /*  HID0 ICFI bit for 604                           */
#define ARCH_PPC_HID0_BIT_DCFI      21              /*  HID0 DCFI bit for 604                           */
#define ARCH_PPC_HID0_BIT_SIED      24              /*  HID0 SIED bit for 604                           */
#define ARCH_PPC_HID0_BIT_DCFA      25              /*  HID0 DCFA bit for 7xx/74xx                      */
#define ARCH_PPC_HID0_BIT_BHTE      29              /*  HID0 BHTE bit for 604                           */

#define ARCH_PPC_HID0_BIT_XBSEN     23              /*  HID0 XBSEN bit for 7455                         */
#define ARCH_PPC_HID0_BIT_HIGH_BAT_EN 8             /*  HID0 HIGH_BAT_EN bit for 7455                   */
#define ARCH_PPC_HID2_BIT_HIGH_BAT_EN 13            /*  HID2 HIGH_BAT_EN bit for 755                    */

/*********************************************************************************************************
  SVR definitions
*********************************************************************************************************/

#define ARCH_PPC_SVR_VER_MASK       0xffffff00      /*  "version" field                                 */
#define ARCH_PPC_SVR_REV_MASK       0x000000ff      /*  "revision" field                                */
#define ARCH_PPC_SVR_MPC8641        0x80900000      /*  single-core 8641                                */
#define ARCH_PPC_SVR_MPC8641D       0x80910000      /*  dual-core 8641D                                 */

/*********************************************************************************************************
  PVR definitions
*********************************************************************************************************/
/*********************************************************************************************************
  Values of upper half of PVR, used in cache code
  to discern cache size and select flush algorithm
*********************************************************************************************************/

#define ARCH_PPC_PVR_PPC604_U       0x0004
#define ARCH_PPC_PVR_PPC750_U       0x0008
#define ARCH_PPC_PVR_PPC604E_U      0x0009
#define ARCH_PPC_PVR_PPC604R_U      0x000a
#define ARCH_PPC_PVR_MPC7400_U      0x000c
#define ARCH_PPC_PVR_MPC755_U       0x3200
#define ARCH_PPC_PVR_MPC745_U       0x3202
#define ARCH_PPC_PVR_PPC750FX_0_U   0x7000          /*  lowest PPC750FX PVR value                       */
#define ARCH_PPC_PVR_PPC750FX_f_U   0x700f          /*  highest PPC750FX PVR value                      */
#define ARCH_PPC_PVR_MPC7450_U      0x8000          /*  also 7440, 7441                                 */
#define ARCH_PPC_PVR_MPC7455_U      0x8001          /*  also 7445                                       */
#define ARCH_PPC_PVR_MPC7457_U      0x8002          /*  also 7447                                       */
#define ARCH_PPC_PVR_MPC7447A_U     0x8003
#define ARCH_PPC_PVR_MPC7448_U      0x8004
#define ARCH_PPC_PVR_MPC7410_U      0x800c

/*********************************************************************************************************
  MSR bit definitions
*********************************************************************************************************/

#define ARCH_PPC_MSR_BIT_POW        13              /*  MSR Power Management bit - POW                  */
#define ARCH_PPC_MSR_BIT_ILE        15              /*  MSR Excep little endian bit - ILE               */
#define ARCH_PPC_MSR_BIT_FP         18              /*  MSR Floating Ponit Aval. bit - FP               */
#define ARCH_PPC_MSR_BIT_FE0        20              /*  MSR FP exception mode 0 bit - FE0               */
#define ARCH_PPC_MSR_BIT_SE         21              /*  MSR Single Step Trace bit - SE                  */
#define ARCH_PPC_MSR_BIT_BE         22              /*  MSR Branch Trace Enable bit - BE                */
#define ARCH_PPC_MSR_BIT_FE1        23              /*  MSR FP exception mode 1 bit - FE1               */
#define ARCH_PPC_MSR_BIT_IP         25              /*  MSR Exception Prefix bit - EP                   */
#define ARCH_PPC_MSR_BIT_IR         26              /*  MSR Inst Translation bit - IR                   */
#define ARCH_PPC_MSR_BIT_DR         27              /*  MSR Data Translation bit - DR                   */
#define ARCH_PPC_MSR_BIT_PM         29              /*  MSR Performance Monitor bit - MR                */
#define ARCH_PPC_MSR_BIT_RI         30              /*  MSR Exception Recoverable bit - RI              */

#if __SYLIXOS_PPC_HAVE_ALTIVEC > 0
#define ARCH_PPC_MSR_VEC            0x02000000      /*  Bit 6 of MSR                                    */
#define ARCH_PPC_MSR_BIT_VEC        06              /*  MSR Altivec Available bit - VEC                 */
#endif                                              /*  __SYLIXOS_PPC_HAVE_ALTIVEC > 0                  */

                                                    /*  MSR MMU/RI Bit extraction                       */
#define ARCH_PPC_MSR_MMU_RI_EXTRACT(src, dst) \
    LI  dst, ARCH_PPC_MSR_IR | ARCH_PPC_MSR_DR | ARCH_PPC_MSR_RI ; \
    AND dst, dst, src

#define ARCH_PPC_MSR_MMU_RI_FP_EXTRACT(src, dst) \
    LI  dst, ARCH_PPC_MSR_IR | ARCH_PPC_MSR_DR | ARCH_PPC_MSR_RI | ARCH_PPC_MSR_FP ; \
    AND dst, dst, src

/*********************************************************************************************************
  IABR bit definitions
*********************************************************************************************************/
                                                    /*  set and get address in IABR                     */
#define ARCH_PPC_IABR_ADD(x)        ((x) & 0xFFFFFFFC)

#define ARCH_PPC_IABR_BE            0x00000002      /*  breakpoint enabled                              */
#define ARCH_PPC_IABR_TE            0x00000001      /*  translation enabled                             */

/*********************************************************************************************************
  DABR bit definitions
*********************************************************************************************************/
                                                    /*  set and get address in DABR                     */
#define ARCH_PPC_DABR_DAB(x)        ((x) & 0xFFFFFFF8)

#define ARCH_PPC_DABR_BT            0x00000004      /*  breakpoint translation                          */
#define ARCH_PPC_DABR_DW            0x00000002      /*  data write enable                               */
#define ARCH_PPC_DABR_DR            0x00000001      /*  data read enable                                */

                                                    /*  mask for read and write operations              */
#define ARCH_PPC_DABR_D_MASK        (ARCH_PPC_DABR_DW | ARCH_PPC_DABR_DR)

/*********************************************************************************************************
  DSISR bit definitions
*********************************************************************************************************/

#define ARCH_PPC_DSISR_BRK          0x00400000      /*  DABR match occurs                               */

/*********************************************************************************************************
  CACHE definitions
*********************************************************************************************************/

#define ARCH_PPC_CACHE_ALIGN_SHIFT  5               /*  cache line size = 32                            */
#define ARCH_PPC_CACHE_ALIGN_SIZE   32

#endif                                                                  /*  __PPC_ARCH_604_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/

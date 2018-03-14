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
** 文   件   名: arch_603.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 01 月 25 日
**
** 描        述: PowerPC 603 相关定义.
*********************************************************************************************************/
/*********************************************************************************************************
  本头文件适用于如下 PowerPC 处理器:
  PPC603, PPCEC603, MPC83XX, G2LE
  包括这个头文件前, 应该定义 __SYLIXOS_PPC_CPU__ 为 __SYLIXOS_PPC603__ 或 __SYLIXOS_PPCEC603__,
  即处理器家族为 PowerPC 603 家族, 然后再根据实际的处理器, 定义处理器识别符, 如:
  __SYLIXOS_PPC83XX__, __SYLIXOS_PPCG2LE__
*********************************************************************************************************/

#ifndef __PPC_ARCH_603_H
#define __PPC_ARCH_603_H

/*********************************************************************************************************
  Special Purpose Register PowerPC603 specific
*********************************************************************************************************/

#define TBLR                        268             /*  lower time base register (read only)            */
#define TBUR                        269             /*  upper time base register (read only)            */
#define TBLW                        TBL             /*  lower time base register (write only)           */
#define TBUW                        TBU             /*  upper time base register (write only)           */

#define IMMR                        638             /*  Internal mem map reg - from 82xx slave SIU      */

                                                    /*  software table search registers                 */
#define DMISS                       976             /*  data tlb miss address register                  */
#define DCMP                        977             /*  data tlb miss compare register                  */
#define HASH1                       978             /*  PTEG1 address register                          */
#define HASH2                       979             /*  PTEG2 address register                          */
#define IMISS                       980             /*  instruction tlb miss address register           */
#define ICMP                        981             /*  instruction tlb mis compare register            */
#define RPA                         982             /*  real page address register                      */

#define HID0                        1008            /*  hardware implementation register 0              */
#define HID1                        1009            /*  hardware implementation register 1              */
#define IABR                        1010            /*  instruction address breakpoint register         */

#if (defined(__SYLIXOS_PPC83XX__) || defined(__SYLIXOS_PPCG2LE__))
                                                    /*  These variants provide SPRG4-7                  */
#define SPRG4                       276             /*  0x114                                           */
#define SPRG5                       277             /*  0x115                                           */
#define SPRG6                       278             /*  0x116                                           */
#define SPRG7                       279             /*  0x117                                           */

#define IABR2                       1018
#define IBCR                        309
#define DABR                        1013
#define DABR2                       317
#define DBCR                        310
#define HID2                        1011
#endif                                              /* (defined(__SYLIXOS_PPC83XX__) ||                 */
                                                    /*  defined(__SYLIXOS_PPCG2LE__))                   */
/*********************************************************************************************************
  HID definitions
*********************************************************************************************************/
/*********************************************************************************************************
  HID0   - hardware implemntation register
  HID2   - instruction address breakpoint register
*********************************************************************************************************/

#define ARCH_PPC_HID0_EMCP          0x80000000      /*  enable machine check pin                        */
#define ARCH_PPC_HID0_EBA           0x20000000      /*  enable bus adress parity checking               */
#define ARCH_PPC_HID0_EBD           0x10000000      /*  enable bus data parity checking                 */
#define ARCH_PPC_HID0_SBCLK         0x08000000      /*  select bus clck for test clck pin               */
#define ARCH_PPC_HID0_EICE          0x04000000      /*  enable ICE outputs                              */
#define ARCH_PPC_HID0_ECLK          0x02000000      /*  enable external test clock pin                  */
#define ARCH_PPC_HID0_PAR           0x01000000      /*  disable precharge of ARTRY                      */

/*********************************************************************************************************
  ARCH_PPC_HID0_XX_U definitions are used with in assembly to minimize the number
  instructions used for setting a bit mask in a general purpose register
*********************************************************************************************************/

#define ARCH_PPC_HID0_DOZE_U        0x0080          /*  DOZE power management mode                      */
#define ARCH_PPC_HID0_NAP_U         0x0040          /*  NAP power management mode                       */
#define ARCH_PPC_HID0_SLEEP_U       0x0020          /*  SLEEP power management mode                     */
#define ARCH_PPC_HID0_DPM_U         0x0010          /*  enable dynamic power management                 */
#define ARCH_PPC_HID0_DOZE          0x00800000      /*  DOZE power management mode                      */
#define ARCH_PPC_HID0_NAP           0x00400000      /*  NAP power management mode                       */
#define ARCH_PPC_HID0_SLEEP         0x00200000      /*  SLEEP power management mode                     */
#define ARCH_PPC_HID0_DPM           0x00100000      /*  enable dynamic power management                 */
#define ARCH_PPC_HID0_RISEG         0x00080000      /*  reserved for test                               */
#define ARCH_PPC_HID0_NHR           0x00010000      /*  reserved                                        */
#define ARCH_PPC_HID0_ICE           0x00008000      /*  inst cache enable                               */
#define ARCH_PPC_HID0_DCE           0x00004000      /*  data cache enable                               */
#define ARCH_PPC_HID0_ILOCK         0x00002000      /*  inst cache lock                                 */
#define ARCH_PPC_HID0_DLOCK         0x00001000      /*  data cache lock                                 */
#define ARCH_PPC_HID0_ICFI          0x00000800      /*  inst cache flash invalidate                     */
#define ARCH_PPC_HID0_DCFI          0x00000400      /*  data cache flash invalidate                     */
#define ARCH_PPC_HID0_SIED          0x00000080      /*  serial instr exec disable                       */
#define ARCH_PPC_HID0_BHTE          0x00000004      /*  branch history table enable                     */

                                                    /*  HID0 bit definitions                            */
#define ARCH_PPC_HID0_BIT_ICE       16              /*  HID0 ICE bit for 603                            */
#define ARCH_PPC_HID0_BIT_DCE       17              /*  HID0 DCE bit for 603                            */
#define ARCH_PPC_HID0_BIT_ILOCK     18              /*  HID0 ILOCK bit for 603                          */
#define ARCH_PPC_HID0_BIT_DLOCK     19              /*  HID0 DLOCK bit for 603                          */
#define ARCH_PPC_HID0_BIT_ICFI      20              /*  HID0 ICFI bit for 603                           */
#define ARCH_PPC_HID0_BIT_DCFI      21              /*  HID0 DCFI bit for 603                           */
#define ARCH_PPC_HID0_BIT_SIED      24              /*  HID0 SIED bit for 603                           */
#define ARCH_PPC_HID0_BIT_BHTE      29              /*  HID0 BHTE bit for 603                           */

#if (defined(__SYLIXOS_PPC83XX__) || defined(__SYLIXOS_PPCG2LE__))
#define ARCH_PPC_HID2_HIGH_BAT_EN_U 0x0004          /*  High Bat enable                                 */
#endif                                              /* (defined(__SYLIXOS_PPC83XX__) ||                 */
                                                    /*  defined(__SYLIXOS_PPCG2LE__))                   */
/*********************************************************************************************************
  MSR bit definitions
*********************************************************************************************************/

#define ARCH_PPC_MSR_TGPR           0x00020000      /*  temporary gpr remapping                         */

#define ARCH_PPC_MSR_BIT_POW        13              /*  MSR Power Management bit - POW                  */
#define ARCH_PPC_MSR_BIT_TGPR       14              /*  MSR Temporary GPR remapping - TGPR              */
#define ARCH_PPC_MSR_BIT_ILE        15              /*  MSR Excep little endian bit - ILE               */
#define ARCH_PPC_MSR_BIT_FP         18              /*  MSR Floating Ponit Aval. bit - FP               */
#define ARCH_PPC_MSR_BIT_FE0        20              /*  MSR FP exception mode 0 bit - FE0               */
#define ARCH_PPC_MSR_BIT_SE         21              /*  MSR Single Step Trace bit - SE                  */
#define ARCH_PPC_MSR_BIT_BE         22              /*  MSR Branch Trace Enable bit - BE                */
#define ARCH_PPC_MSR_BIT_FE1        23              /*  MSR FP exception mode 1 bit - FE1               */
#define ARCH_PPC_MSR_BIT_IP         25              /*  MSR Exception Prefix bit - EP                   */
#define ARCH_PPC_MSR_BIT_IR         26              /*  MSR Inst Translation bit - IR                   */
#define ARCH_PPC_MSR_BIT_DR         27              /*  MSR Data Translation bit - DR                   */
#define ARCH_PPC_MSR_BIT_RI         30              /*  MSR Exception Recoverable bit - RI              */

                                                    /*  PowerPC EC 603 does not have floating point unit*/
#if (__SYLIXOS_PPC_CPU__ == __SYLIXOS_PPCEC603__)
#undef  ARCH_PPC_MSR_FP                             /*  hardware floating point unsupported             */
#undef  ARCH_PPC_MSR_BIT_FP                         /*  MSR Floating Ponit Aval. bit - FP               */
#undef  ARCH_PPC_MSR_BIT_FE0                        /*  MSR FP exception mode 0 bit - FE0               */
#undef  ARCH_PPC_MSR_BIT_FE1                        /*  MSR FP exception mode 1 bit - FE1               */
#undef  ARCH_PPC_MSR_FE1                            /*  floating point not supported                    */
#undef  ARCH_PPC_MSR_FE0                            /*  floating point not supported                    */
#endif                                              /*  __SYLIXOS_PPC_CPU__ == __SYLIXOS_PPCEC603__     */

                                                    /*  MSR MMU/RI Bit extraction                       */
#define ARCH_PPC_MSR_MMU_RI_EXTRACT(src, dst) \
    LI  dst, ARCH_PPC_MSR_IR | ARCH_PPC_MSR_DR | ARCH_PPC_MSR_RI ; \
    AND dst, dst, src

#ifndef ARCH_PPC_MSR_FP
                                                    /*  No FP, so ARCH_PPC_MSR_MMU_RI_FP_EXTRACT Same   */
#define ARCH_PPC_MSR_MMU_RI_FP_EXTRACT  ARCH_PPC_MSR_MMU_RI_EXTRACT
#else                                               /*  ARCH_PPC_MSR_FP                                 */
#define ARCH_PPC_MSR_MMU_RI_FP_EXTRACT(src, dst) \
    LI  dst, ARCH_PPC_MSR_IR | ARCH_PPC_MSR_DR | ARCH_PPC_MSR_RI | ARCH_PPC_MSR_FP ; \
    AND dst, dst, src
#endif                                              /*  ARCH_PPC_MSR_FP                                 */

/*********************************************************************************************************
  IABR bit definitions
*********************************************************************************************************/
                                                    /*  set and get address in IABR                     */
#define ARCH_PPC_IABR_ADD(x)        ((x) & 0xFFFFFFFC)

#define ARCH_PPC_IABR_BE            0x00000002      /*  breakpoint enabled                              */

/*********************************************************************************************************
  DABR bits definitions
*********************************************************************************************************/

#if (defined (__SYLIXOS_PPC83XX__) || defined (__SYLIXOS_PPCG2LE__))
                                                    /*  set and get address in DABR                     */
#define ARCH_PPC_DABR_DAB(x)        ((x) & 0xFFFFFFF8)

#define ARCH_PPC_DABR_BT            0x00000004      /*  breakpoint translation                          */
#define ARCH_PPC_DABR_DW            0x00000002      /*  data write enable                               */
#define ARCH_PPC_DABR_DR            0x00000001      /*  data read enable                                */

                                                    /*  mask for read and write operations              */
#define ARCH_PPC_DABR_D_MSK         (ARCH_PPC_DABR_DW | ARCH_PPC_DABR_DR)
#endif                                              /* (defined(__SYLIXOS_PPC83XX__) ||                 */
                                                    /*  defined(__SYLIXOS_PPCG2LE__))                   */
/*********************************************************************************************************
  DSISR bits definitions
*********************************************************************************************************/

#if (defined (__SYLIXOS_PPC83XX__) || defined (__SYLIXOS_PPCG2LE__))
#define ARCH_PPC_DSISR_BRK          0x00400000      /*  DABR match occurs                               */
#endif                                              /* (defined(__SYLIXOS_PPC83XX__) ||                 */
                                                    /*  defined(__SYLIXOS_PPCG2LE__))                   */
/*********************************************************************************************************
  CACHE definitions
*********************************************************************************************************/

#define ARCH_PPC_CACHE_ALIGN_SHIFT  5               /*  cache line size = 32                            */
#define ARCH_PPC_CACHE_ALIGN_SIZE   32

#endif                                              /*  __PPC_ARCH_603_H                                */
/*********************************************************************************************************
  END
*********************************************************************************************************/

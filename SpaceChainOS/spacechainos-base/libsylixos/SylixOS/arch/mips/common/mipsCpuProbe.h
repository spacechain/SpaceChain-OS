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
** 文   件   名: mipsCpuProbe.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 07 月 18 日
**
** 描        述: MIPS 体系构架 CPU 探测.
*********************************************************************************************************/

#ifndef __ARCH_MIPSCPUPROBE_H
#define __ARCH_MIPSCPUPROBE_H

/*********************************************************************************************************
  As of the MIPS32 and MIPS64 specs from MTI, the PRId register (CP0
  register 15, select 0) is defined in this (backwards compatible) way:

  +----------------+----------------+----------------+----------------+
  | Company Options| Company ID     | Processor ID   | Revision       |
  +----------------+----------------+----------------+----------------+
  31             24 23            16 15             8 7

  I don't have docs for all the previous processors, but my impression is
  that bits 16-23 have been 0 for all MIPS processors before the MIPS32/64 spec.
*********************************************************************************************************/
#define PRID_OPT_MASK           0xff000000
/*********************************************************************************************************
  Assigned Company values for bits 23:16 of the PRId register.
*********************************************************************************************************/
#define PRID_COMP_MASK          0xff0000

#define PRID_COMP_LEGACY        0x000000
#define PRID_COMP_MIPS          0x010000
#define PRID_COMP_BROADCOM      0x020000
#define PRID_COMP_ALCHEMY       0x030000
#define PRID_COMP_SIBYTE        0x040000
#define PRID_COMP_SANDCRAFT     0x050000
#define PRID_COMP_NXP           0x060000
#define PRID_COMP_TOSHIBA       0x070000
#define PRID_COMP_LSI           0x080000
#define PRID_COMP_LEXRA         0x0b0000
#define PRID_COMP_NETLOGIC      0x0c0000
#define PRID_COMP_CAVIUM        0x0d0000
#define PRID_COMP_INGENIC       0xd00000
#define PRID_COMP_INGENIC_4780  0xe10000
/*********************************************************************************************************
  Assigned Processor ID (implementation) values for bits 15:8 of the PRId
  register.  In order to detect a certain CPU type exactly eventually
  additional registers may need to be examined.
*********************************************************************************************************/
#define PRID_IMP_MASK           0xff00
/*********************************************************************************************************
  These are valid when 23:16 == PRID_COMP_LEGACY
*********************************************************************************************************/
#define PRID_IMP_R2000          0x0100
#define PRID_IMP_AU1_REV1       0x0100
#define PRID_IMP_AU1_REV2       0x0200
#define PRID_IMP_R3000          0x0200                                  /*  Same as R2000A              */
#define PRID_IMP_R6000          0x0300                                  /*  Same as R3000A              */
#define PRID_IMP_R4000          0x0400
#define PRID_IMP_R6000A         0x0600
#define PRID_IMP_R10000         0x0900
#define PRID_IMP_R4300          0x0b00
#define PRID_IMP_VR41XX         0x0c00
#define PRID_IMP_R12000         0x0e00
#define PRID_IMP_R14000         0x0f00
#define PRID_IMP_R8000          0x1000
#define PRID_IMP_PR4450         0x1200
#define PRID_IMP_R4600          0x2000
#define PRID_IMP_R4700          0x2100
#define PRID_IMP_TX39           0x2200
#define PRID_IMP_R4640          0x2200
#define PRID_IMP_R4650          0x2200                                  /*  Same as R4640               */
#define PRID_IMP_R5000          0x2300
#define PRID_IMP_TX49           0x2d00
#define PRID_IMP_SONIC          0x2400
#define PRID_IMP_MAGIC          0x2500
#define PRID_IMP_RM7000         0x2700
#define PRID_IMP_NEVADA         0x2800                                  /*  RM5260 ???                  */
#define PRID_IMP_RM9000         0x3400
#define PRID_IMP_LOONGSON_32    0x4200                                  /*  Loongson-1                  */
#define PRID_IMP_R5432          0x5400
#define PRID_IMP_R5500          0x5500
#define PRID_IMP_LOONGSON_64    0x6300                                  /*  Loongson-2/3                */

#define PRID_IMP_UNKNOWN        0xff00
/*********************************************************************************************************
  These are the PRID's for when 23:16 == PRID_COMP_MIPS
*********************************************************************************************************/
#define PRID_IMP_4KC            0x8000
#define PRID_IMP_5KC            0x8100
#define PRID_IMP_20KC           0x8200
#define PRID_IMP_4KEC           0x8400
#define PRID_IMP_4KSC           0x8600
#define PRID_IMP_25KF           0x8800
#define PRID_IMP_5KE            0x8900
#define PRID_IMP_4KECR2         0x9000
#define PRID_IMP_4KEMPR2        0x9100
#define PRID_IMP_4KSD           0x9200
#define PRID_IMP_24K            0x9300
#define PRID_IMP_34K            0x9500
#define PRID_IMP_24KE           0x9600
#define PRID_IMP_74K            0x9700
#define PRID_IMP_1004K          0x9900
#define PRID_IMP_1074K          0x9a00
#define PRID_IMP_M14KC          0x9c00
#define PRID_IMP_M14KEC         0x9e00
#define PRID_IMP_INTERAPTIV_UP  0xa000
#define PRID_IMP_INTERAPTIV_MP  0xa100
#define PRID_IMP_PROAPTIV_UP    0xa200
#define PRID_IMP_PROAPTIV_MP    0xa300
#define PRID_IMP_M5150          0xa700
#define PRID_IMP_P5600          0xa800
/*********************************************************************************************************
  These are the PRID's for when 23:16 == PRID_COMP_INGENIC
*********************************************************************************************************/
#define PRID_IMP_JZRISC         0x0200
/*********************************************************************************************************
  Particular Revision values for bits 7:0 of the PRId register.
*********************************************************************************************************/
#define PRID_REV_MASK           0x00ff
/*********************************************************************************************************
  Definitions for 7:0 on legacy processors
*********************************************************************************************************/
#define PRID_REV_TX4927         0x0022
#define PRID_REV_TX4937         0x0030
#define PRID_REV_R4400          0x0040
#define PRID_REV_R3000A         0x0030
#define PRID_REV_R3000          0x0020
#define PRID_REV_R2000A         0x0010
#define PRID_REV_TX3912         0x0010
#define PRID_REV_TX3922         0x0030
#define PRID_REV_TX3927         0x0040
#define PRID_REV_VR4111         0x0050
#define PRID_REV_VR4181         0x0050                                  /*  Same as VR4111              */
#define PRID_REV_VR4121         0x0060
#define PRID_REV_VR4122         0x0070
#define PRID_REV_VR4181A        0x0070                                  /*  Same as VR4122              */
#define PRID_REV_VR4130         0x0080
#define PRID_REV_34K_V1_0_2     0x0022
#define PRID_REV_LOONGSON1B     0x0020
#define PRID_REV_LOONGSON2E     0x0002
#define PRID_REV_LOONGSON2F     0x0003
#define PRID_REV_LOONGSON3A     0x0005
#define PRID_REV_LOONGSON3B_R1  0x0006
#define PRID_REV_LOONGSON3B_R2  0x0007

#if !defined(__ASSEMBLY__)

typedef enum cpu_type_enum {
    CPU_UNKNOWN,

    /*
     * R2000 class processors
     */
    CPU_R2000, CPU_R3000, CPU_R3000A, CPU_R3041, CPU_R3051, CPU_R3052,
    CPU_R3081, CPU_R3081E,

    /*
     * R6000 class processors
     */
    CPU_R6000, CPU_R6000A,

    /*
     * R4000 class processors
     */
    CPU_R4000PC, CPU_R4000SC, CPU_R4000MC, CPU_R4200, CPU_R4300, CPU_R4310,
    CPU_R4400PC, CPU_R4400SC, CPU_R4400MC, CPU_R4600, CPU_R4640, CPU_R4650,
    CPU_R4700, CPU_R5000, CPU_R5500, CPU_NEVADA, CPU_R5432, CPU_R10000,
    CPU_R12000, CPU_R14000, CPU_VR41XX, CPU_VR4111, CPU_VR4121, CPU_VR4122,
    CPU_VR4131, CPU_VR4133, CPU_VR4181, CPU_VR4181A, CPU_RM7000,
    CPU_SR71000, CPU_TX49XX,

    /*
     * R8000 class processors
     */
    CPU_R8000,

    /*
     * TX3900 class processors
     */
    CPU_TX3912, CPU_TX3922, CPU_TX3927,

    /*
     * MIPS32 class processors
     */
    CPU_4KC, CPU_4KEC, CPU_4KSC, CPU_24K, CPU_34K, CPU_1004K, CPU_74K,
    CPU_ALCHEMY, CPU_PR4450, CPU_BMIPS32, CPU_BMIPS3300, CPU_BMIPS4350,
    CPU_BMIPS4380, CPU_BMIPS5000, CPU_JZRISC, CPU_LOONGSON1, CPU_M14KC,
    CPU_M14KEC, CPU_INTERAPTIV, CPU_P5600, CPU_PROAPTIV, CPU_1074K, CPU_M5150,

    /*
     * MIPS64 class processors
     */
    CPU_5KC, CPU_5KE, CPU_20KC, CPU_25KF, CPU_SB1, CPU_SB1A, CPU_LOONGSON2,
    CPU_LOONGSON3, CPU_CAVIUM_OCTEON, CPU_CAVIUM_OCTEON_PLUS,
    CPU_CAVIUM_OCTEON2, CPU_CAVIUM_OCTEON3, CPU_XLR, CPU_XLP,

    CPU_LAST
} MIPS_CPU_TYPE;
/*********************************************************************************************************
  全局变量声明
*********************************************************************************************************/
extern UINT    _G_uiMipsProcessorId;                                    /*  处理器 ID                   */
extern UINT    _G_uiMipsCpuType;                                        /*  CPU 类型                    */
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
VOID  mipsCpuProbe(VOID);

#endif                                                                  /*  !__ASSEMBLY                 */

#endif                                                                  /*  __ARCH_MIPSCPUPROBE_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

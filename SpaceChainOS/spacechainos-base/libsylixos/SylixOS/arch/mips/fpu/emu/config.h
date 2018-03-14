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
** 文   件   名: config.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 11 月 25 日
**
** 描        述: MIPS FPU 模拟配置文件.
*********************************************************************************************************/

#ifndef __MIPS_FPUEMU_CONFIG_H
#define __MIPS_FPUEMU_CONFIG_H

/*********************************************************************************************************
  配置选项
*********************************************************************************************************/

#define CONFIG_64BIT                    0
#define CONFIG_32BIT                    1

#define CONFIG_MIPS32_O32               1
#define CONFIG_MIPS_O32_FP64_SUPPORT    0

#define CONFIG_DEBUG_FPU_EMU

#undef  CONFIG_SYS_SUPPORTS_MIPS16
#undef  CONFIG_SYS_SUPPORTS_MICROMIPS

#undef  CONFIG_CPU_CAVIUM_OCTEON

#define NO_R6EMU                        LW_FALSE

#ifdef CONFIG_SYS_SUPPORTS_MICROMIPS
#define cpu_has_mmips                   LW_TRUE
#else
#define cpu_has_mmips                   LW_FALSE
#endif

#if CONFIG_64BIT > 0
#define cpu_has_mips64                  LW_TRUE
#else
#define cpu_has_mips64                  LW_FALSE
#endif

#define cpu_has_mips_r                  LW_TRUE
#define cpu_has_mips_r6                 LW_TRUE
#define cpu_has_mips_3_4_5              LW_TRUE
#define cpu_has_mips_2_3_4_5_r          LW_TRUE
#define cpu_has_mips_4_5_64_r2_r6       LW_TRUE
#define cpu_has_mips_4_5_r              LW_TRUE
#define cpu_has_mips_3_4_5_64_r2_r6     LW_TRUE
#define cpu_has_mips_r2_r6              LW_TRUE

#endif                                                                  /*  __MIPS_FPUEMU_CONFIG_H      */
/*********************************************************************************************************
  END
*********************************************************************************************************/

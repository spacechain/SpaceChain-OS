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
** 文   件   名: endian_cfg.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 03 月 13 日
**
** 描        述: 处理器 (或编译器) 大小端定义.
*********************************************************************************************************/

#ifndef __ENDIAN_CFG_H
#define __ENDIAN_CFG_H

/*********************************************************************************************************
  浮点大小端定义 
*********************************************************************************************************/

/*********************************************************************************************************
  GCC EABI 关于 ARM VFP 相关说明
  
  GCC preprocessor macros for floating point
  
  When porting code to "armel", the following preprocessor macros are interesting:

  __VFP_FP__   means that the floating point format in use is that of the ARM VFP unit, which is 
               native-endian IEEE-754.
               
  __MAVERICK__ means that the floating point format is that of the Cirrus Logic MaverickCrunch, 
               which is also IEEE-754 and is always little-endian.
               
  __SOFTFP__   means that instead of floating point instructions, library calls are being generated for 
               floating point math operations so that the code will run on a processor without an FPU.
               
  __VFP_FP__ and __MAVERICK__ are mutually exclusive. If neither is set, that means the floating point 
                              format in use is the old mixed-endian 45670123 format of the FPA unit.

  Note that __VFP_FP__ does not mean that VFP code generation has been selected. It only speaks of the 
  
  floating point data format in use and is normally set when soft-float has been selected. The correct test 
  
  for VFP code generation, for example around asm fragments containing VFP instructions, is

  #if (defined(__VFP_FP__) && !defined(__SOFTFP__))
  
  Paradoxically, the -mfloat-abi=softfp does not set the __SOFTFP___ macro, since it selects real 
  
  floating point instructions using the soft-float ABI at function-call interfaces.

  __ARM_PCS_VFP is set instead (or as well?) when compiling for the armhf VFP hardfloat port.

  By default in Debian armel, __VFP_FP__ && __SOFTFP__ are selected.

  Struct packing and alignment

  With the new ABI, default structure packing changes, as do some default data sizes and alignment 
  
  (which also have a knock-on effect on structure packing). In particular the minimum size and alignment 
  
  of a structure was 4 bytes. Under the EABI there is no minimum and the alignment is determined by the 
  
  types of the components it contains. This will break programs that know too much about the way 
  
  structures are packed and can break code that writes binary files by dumping and reading structures.
  
  
  -mfloat-abi=soft    使用这个参数时，其将调用软浮点库(softfloat lib)来支持对浮点的运算.
  
  -mfloat-abi=softfp and  -mfloat-abi=hard 这两个参数都用来产生硬浮点指令，至于产生哪里类型的硬浮点指令，
  
  需要由-mfpu=xxx参数来指令。这两个参数不同的地方是：-mfloat-abi=softfp生成的代码采用兼容软浮点调用接口
  
  (即使用 -mfloat-abi=soft 时的调用接口)
  
  操作系统内核, 驱动程序, BSP, 内核模块可以采用 -mfloat-abi=soft 编译.
  
  如果存在 VFP 应用程序可使用 -mfloat-abi=softfp 来编译, 当然, 浮点指令集是通过 -mfpu=? 来选择的, 例如
  
  -mfpu=vfp

*********************************************************************************************************/

#if defined(__CC_ARM) || defined(LW_CFG_CPU_ARCH_ARM)                   /*  armcc & arm-sylixos-eabi-gcc*/
#define LW_CFG_DOUBLE_MIX_ENDIAN            0                           /*  armcc default native-endian */

#elif defined(__GNUC__) && defined(__arm__)

#if !defined(__VFP_FP__) || \
    (defined(__VFP_FP__) && defined(__MAVERICK__))
#define LW_CFG_DOUBLE_MIX_ENDIAN            1                           /*  arm-sylixos-eabi-gcc        */
                                                                        /*  old mixed-endian            */
#else
#define LW_CFG_DOUBLE_MIX_ENDIAN            0                           /*  native-endian               */
#endif                                                                  /*  __VFP_FP__ & __MAVERICK__   */

#else
#define LW_CFG_DOUBLE_MIX_ENDIAN            0                           /*  default native-endian       */
#endif                                                                  /*  __CC_ARM                    */
                                                                        
#endif                                                                  /*  __ENDIAN_CFG_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/

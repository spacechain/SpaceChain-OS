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
** 文   件   名: arch_float.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 05 月 04 日
**
** 描        述: ARM 浮点相关.
*********************************************************************************************************/

#ifndef __ARM_ARCH_FLOAT_H
#define __ARM_ARCH_FLOAT_H

#include "asm/archprob.h"

/*********************************************************************************************************
  1: native-endian double  
     与处理器其他类型存储完全相同, 
                    
  2: mixed-endian double  
     混合大小端, (32位字内存储与处理器相同, 两个32位之间按照大端存储)
     
  arm eabi 使用 native-endian
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL
/*********************************************************************************************************
  ARMv7M 体系构架
*********************************************************************************************************/
#if defined(__SYLIXOS_ARM_ARCH_M__)
/*********************************************************************************************************
  线程浮点运算器上下文

    +-----------+
    | vfp_gpr31 |    + 0x88
    |  ...      |
    | vfp_gpr2  |    + 0x10
    | vfp_gpr1  |    + 0x0c
    | vfp_gpr0  |    + 0x08  <-- (r0 + 8)
    | pad       |    + 0x04
    | fpscr     | <-- arch_fpu_ctx ( = r0 )
    +-----------+
*********************************************************************************************************/

typedef struct arch_fpu_ctx {                                           /* ARMv7M VFPv4/VFPv5 上下文    */
    UINT32              FPUCTX_uiFpscr;                                 /* status and control register  */
    UINT32              FPUCTX_uiPad;                                   /* pad                          */
    UINT32              FPUCTX_uiDreg[32];                              /* general purpose Reg  D0 ~ D15*/
} ARCH_FPU_CTX;

#define ARCH_FPU_CTX_ALIGN      4                                       /* FPU CTX align size           */

#else
/*********************************************************************************************************
  ARMv7A, R 体系构架
*********************************************************************************************************/
/*********************************************************************************************************
  线程浮点运算器上下文
  
  注意: VFPv2 支持 16 个 double 寄存器, VFPv3 支持 32 个 double 型寄存器。
  
  VFPv2: (dreg is s)
    +-----------+
    | freg[31]  |    + 0x98 <-- (r0 + 152)
    |  ...      |
    | freg[2]   |    + 0x24
    | freg[1]   |    + 0x20
    | freg[0]   |    + 0x1C  <-- (r0 + 28)
    | mfvfr1    |    + 0x18
    | mfvfr0    |    + 0x14
    | fpinst2   |    + 0x10
    | fpinst    |    + 0x0C
    | fpexc     |    + 0x08
    | fpscr     |    + 0x04
    | fpsid     | <-- arch_fpu_ctx ( = r0 )
    +-----------+
    
  VFPv3: (dreg is s)
    +-----------+
    | freg[63]  |    + 0x118 <-- (r0 + 280)
    |  ...      |
    | freg[2]   |    + 0x24
    | freg[1]   |    + 0x20
    | freg[0]   |    + 0x1C  <-- (r0 + 28)
    | mfvfr1    |    + 0x18
    | mfvfr0    |    + 0x14
    | fpinst2   |    + 0x10
    | fpinst    |    + 0x0C
    | fpexc     |    + 0x08
    | fpscr     |    + 0x04
    | fpsid     | <-- arch_fpu_ctx ( = r0 )
    +-----------+
*********************************************************************************************************/

typedef struct arch_fpu_ctx {                                           /* VFPv2/VFPv3 上下文           */
    UINT32              FPUCTX_uiFpsid;                                 /* system ID register           */
    UINT32              FPUCTX_uiFpscr;                                 /* status and control register  */
    UINT32              FPUCTX_uiFpexc;                                 /* exception register           */
    UINT32              FPUCTX_uiFpinst;                                /* instruction register         */
    UINT32              FPUCTX_uiFpinst2;                               /* instruction register         */
    UINT32              FPUCTX_uiMfvfr0;                                /* media and VFP feature Reg    */
    UINT32              FPUCTX_uiMfvfr1;                                /* media and VFP feature Reg    */
    UINT32              FPUCTX_uiDreg[32 * 2];                          /* general purpose Reg  D0 ~ D32*/
                                                                        /* equ -> S0 ~ S64              */
} ARCH_FPU_CTX;

#define ARCH_FPU_CTX_ALIGN      4                                       /* FPU CTX align size           */

#endif                                                                  /* !__SYLIXOS_ARM_ARCH_M__      */
/*********************************************************************************************************
  float 格式 (使用 union 类型作为中间转换, 避免 GCC 3.x.x strict aliasing warning)
*********************************************************************************************************/

#define __ARCH_FLOAT_EXP_NAN           255                              /*  NaN 或者无穷大的 Exp 值     */

#if LW_CFG_CPU_ENDIAN == 0
typedef struct __cpu_float_field {
    unsigned int        frac : 23;
    unsigned int        exp  :  8;
    unsigned int        sig  :  1;
} __CPU_FLOAT_FIELD;
#else
typedef struct __cpu_float_field {
    unsigned int        sig  :  1;
    unsigned int        exp  :  8;
    unsigned int        frac : 23;
} __CPU_FLOAT_FIELD;
#endif                                                                  /*  LW_CFG_CPU_ENDIAN           */

typedef union __cpu_float {
    __CPU_FLOAT_FIELD   fltfield;                                       /*  float 位域字段              */
    float               flt;                                            /*  float 占位                  */
} __CPU_FLOAT;

static LW_INLINE INT  __ARCH_FLOAT_ISNAN (float  x)
{
    __CPU_FLOAT     cpuflt;
    
    cpuflt.flt = x;

    return  ((cpuflt.fltfield.exp == __ARCH_FLOAT_EXP_NAN) && (cpuflt.fltfield.frac != 0));
}

static LW_INLINE INT  __ARCH_FLOAT_ISINF (float  x)
{
    __CPU_FLOAT     cpuflt;
    
    cpuflt.flt = x;
    
    return  ((cpuflt.fltfield.exp == __ARCH_FLOAT_EXP_NAN) && (cpuflt.fltfield.frac == 0));
}

/*********************************************************************************************************
  double 格式
*********************************************************************************************************/

#define __ARCH_DOUBLE_EXP_NAN           2047                            /*  NaN 或者无穷大的 Exp 值     */
#define __ARCH_DOUBLE_INC_FLOAT_H          0                            /*  是否引用编译器 float.h 文件 */

/*********************************************************************************************************
  arm-sylixos-eabi-gcc ... GNU
*********************************************************************************************************/

#if LW_CFG_CPU_ENDIAN == 0
#if LW_CFG_DOUBLE_MIX_ENDIAN > 0
typedef struct __cpu_double_field {                                     /*  old mixed-endian            */
    unsigned int        frach : 20;
    unsigned int        exp   : 11;
    unsigned int        sig   :  1;
    
    unsigned int        fracl : 32;                                     /*  低 32 位放入高地址          */
} __CPU_DOUBLE_FIELD;
#else
typedef struct __cpu_double_field {                                     /*  native-endian               */
    unsigned int        fracl : 32;                                     /*  低 32 位放入低地址          */
    
    unsigned int        frach : 20;
    unsigned int        exp   : 11;
    unsigned int        sig   :  1;
} __CPU_DOUBLE_FIELD;
#endif                                                                  /*  __ARCH_DOUBLE_MIX_ENDIAN    */
#else
typedef struct __cpu_double_field {
    unsigned int        sig   :  1;
    unsigned int        exp   : 11;
    unsigned int        frach : 20;

    unsigned int        fracl : 32;
} __CPU_DOUBLE_FIELD;
#endif                                                                  /*  LW_CFG_CPU_ENDIAN           */

typedef union __cpu_double {
    __CPU_DOUBLE_FIELD  dblfield;                                       /*  float 位域字段              */
    double              dbl;                                            /*  float 占位                  */
} __CPU_DOUBLE;

static LW_INLINE INT  __ARCH_DOUBLE_ISNAN (double  x)
{
    __CPU_DOUBLE     dblflt;
    
    dblflt.dbl = x;
    
    return  ((dblflt.dblfield.exp == __ARCH_DOUBLE_EXP_NAN) && 
             ((dblflt.dblfield.fracl != 0) && 
              (dblflt.dblfield.frach != 0)));
}

static LW_INLINE INT  __ARCH_DOUBLE_ISINF (double  x)
{
    __CPU_DOUBLE     dblflt;
    
    dblflt.dbl = x;
    
    return  ((dblflt.dblfield.exp == __ARCH_DOUBLE_EXP_NAN) && 
             ((dblflt.dblfield.fracl == 0) || 
              (dblflt.dblfield.frach == 0)));
}

#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __ARM_ARCH_FLOAT_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/

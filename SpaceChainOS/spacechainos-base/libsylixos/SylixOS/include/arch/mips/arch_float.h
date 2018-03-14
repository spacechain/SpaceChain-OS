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
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 09 月 01 日
**
** 描        述: MIPS 浮点相关.
*********************************************************************************************************/

#ifndef __MIPS_ARCH_FLOAT_H
#define __MIPS_ARCH_FLOAT_H

/*********************************************************************************************************
  Number of fp registers on coprocessor
*********************************************************************************************************/
#if defined(__SYLIXOS_KERNEL) || defined(__ASSEMBLY__) || defined(ASSEMBLY)

#define FP_NUM_DREGS            32

#define FP0_OFFSET              0x0                                     /*  OFFSET(FPX[0])              */
#define FP1_OFFSET              0x8                                     /*  OFFSET(FPX[1])              */
#define FP2_OFFSET              0x10                                    /*  OFFSET(FPX[2])              */
#define FP3_OFFSET              0x18                                    /*  OFFSET(FPX[3])              */
#define FP4_OFFSET              0x20                                    /*  OFFSET(FPX[4])              */
#define FP5_OFFSET              0x28                                    /*  OFFSET(FPX[5])              */
#define FP6_OFFSET              0x30                                    /*  OFFSET(FPX[6])              */
#define FP7_OFFSET              0x38                                    /*  OFFSET(FPX[7])              */
#define FP8_OFFSET              0x40                                    /*  OFFSET(FPX[8])              */
#define FP9_OFFSET              0x48                                    /*  OFFSET(FPX[9])              */
#define FP10_OFFSET             0x50                                    /*  OFFSET(FPX[10])             */
#define FP11_OFFSET             0x58                                    /*  OFFSET(FPX[11])             */
#define FP12_OFFSET             0x60                                    /*  OFFSET(FPX[12])             */
#define FP13_OFFSET             0x68                                    /*  OFFSET(FPX[13])             */
#define FP14_OFFSET             0x70                                    /*  OFFSET(FPX[14])             */
#define FP15_OFFSET             0x78                                    /*  OFFSET(FPX[15])             */
#define FP16_OFFSET             0x80                                    /*  OFFSET(FPX[16])             */
#define FP17_OFFSET             0x88                                    /*  OFFSET(FPX[17])             */
#define FP18_OFFSET             0x90                                    /*  OFFSET(FPX[18])             */
#define FP19_OFFSET             0x98                                    /*  OFFSET(FPX[19])             */
#define FP20_OFFSET             0xa0                                    /*  OFFSET(FPX[20])             */
#define FP21_OFFSET             0xa8                                    /*  OFFSET(FPX[21])             */
#define FP22_OFFSET             0xb0                                    /*  OFFSET(FPX[22])             */
#define FP23_OFFSET             0xb8                                    /*  OFFSET(FPX[23])             */
#define FP24_OFFSET             0xc0                                    /*  OFFSET(FPX[24])             */
#define FP25_OFFSET             0xc8                                    /*  OFFSET(FPX[25])             */
#define FP26_OFFSET             0xd0                                    /*  OFFSET(FPX[26])             */
#define FP27_OFFSET             0xd8                                    /*  OFFSET(FPX[27])             */
#define FP28_OFFSET             0xe0                                    /*  OFFSET(FPX[28])             */
#define FP29_OFFSET             0xe8                                    /*  OFFSET(FPX[29])             */
#define FP30_OFFSET             0xf0                                    /*  OFFSET(FPX[30])             */
#define FP31_OFFSET             0xf8                                    /*  OFFSET(FPX[31])             */
#define FPCSR_OFFSET            0x100                                   /*  OFFSET(ARCH_FPU_CTX, fpcsr) */

/*********************************************************************************************************
  汇编代码不包含以下内容
*********************************************************************************************************/

#if (!defined(__ASSEMBLY__)) && (!defined(ASSEMBLY))

#define FPU_REG_WIDTH   64

union fpureg {
    UINT32              val32[FPU_REG_WIDTH / 32];
    UINT64              val64[FPU_REG_WIDTH / 64];
};

/*
 * It would be nice to add some more fields for emulator statistics,
 * the additional information is private to the FPU emulator for now.
 */
typedef struct arch_fpu_ctx {                                           /*  FP_CONTEXT 上下文           */
    union fpureg        FPUCTX_reg[FP_NUM_DREGS];
    UINT32              FPUCTX_uiFpcsr;
    UINT32              FPUCTX_uiMsacsr;
} ARCH_FPU_CTX;

#define ARCH_FPU_CTX_ALIGN      8                                       /*  FPU CTX align size          */

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
  mips-sylixos-elf-gcc ... GNU
*********************************************************************************************************/

#if LW_CFG_CPU_ENDIAN == 0
typedef struct __cpu_double_field {
    unsigned int        fracl : 32;

    unsigned int        frach : 20;
    unsigned int        exp   : 11;
    unsigned int        sig   :  1;
} __CPU_DOUBLE_FIELD;
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

/*********************************************************************************************************
  MIPS FPU 模拟帧
*********************************************************************************************************/

typedef UINT  MIPS_INSTRUCTION;

typedef struct {
    MIPS_INSTRUCTION      MIPSFPUE_emul;

#define BRK_MEMU          514                                           /*  Used by FPU emulator        */
#define BREAK_MATH(micromips)   (((micromips) ? 0x7 : 0xd) | (BRK_MEMU << 16))
    MIPS_INSTRUCTION      MIPSFPUE_badinst;
} MIPS_FPU_EMU_FRAME;

#endif                                                                  /*  __ASSEMBLY__                */
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __MIPS_ARCH_FLOAT_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

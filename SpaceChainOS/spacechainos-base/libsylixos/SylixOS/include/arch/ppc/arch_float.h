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
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 11 月 26 日
**
** 描        述: PowerPC 浮点相关.
*********************************************************************************************************/

#ifndef __PPC_ARCH_FLOAT_H
#define __PPC_ARCH_FLOAT_H

/*********************************************************************************************************
  FPU 浮点数据寄存器的总数
*********************************************************************************************************/
#if defined(__SYLIXOS_KERNEL) || defined(__ASSEMBLY__) || defined(ASSEMBLY)

#define FP_DREG_NR              32

/*********************************************************************************************************
  定义 ARCH_FPU_CTX FPU 成员偏移
*********************************************************************************************************/

#define XFPR(n)                 ((n) * 8)
#define XFPSCR                  (FP_DREG_NR * 8)
#define XFPSCR_COPY             (XFPSCR + 4)

/*********************************************************************************************************
  SPE 浮点数据寄存器的总数
*********************************************************************************************************/

#define SPE_GPR_NR              32
#define SPE_ACC_NR              2

/*********************************************************************************************************
  定义 ARCH_FPU_CTX SPE 成员偏移
*********************************************************************************************************/

#define SPE_OFFSET(n)           (4 * (n))

/*********************************************************************************************************
  ALTIVEC 浮点数据寄存器的总数
*********************************************************************************************************/

#define ALTIVEC_REG_NR          32

/*********************************************************************************************************
  定义 ARCH_FPU_CTX ALTIVEC 成员偏移
*********************************************************************************************************/

#define ALTIVEC_OFFSET(n)       ((n) * 16)
#define ALTIVEC_VSCR_OFFSET     (ALTIVEC_REG_NR * 16)
#define ALTIVEC_VRSAVE_OFFSET   (ALTIVEC_VSCR_OFFSET + 16)
#define ALTIVEC_CTX_SIZE        (16 * (ALTIVEC_REG_NR + 2))

/*********************************************************************************************************
  汇编代码不包含以下内容
*********************************************************************************************************/

#if (!defined(__ASSEMBLY__)) && (!defined(ASSEMBLY))

/*********************************************************************************************************
  1: native-endian double  
     与处理器其他类型存储完全相同, 
                    
  2: mixed-endian double  
     混合大小端, (32位字内存储与处理器相同, 两个32位之间按照大端存储)
     
  PowerPC eabi 使用 native-endian
*********************************************************************************************************/

/*********************************************************************************************************
  定义 ALTIVEC 寄存器
*********************************************************************************************************/

typedef UINT32          ALTIVEC_REG[4];                                 /*  128 位矢量寄存器            */

/*********************************************************************************************************
  线程浮点运算器上下文
*********************************************************************************************************/

typedef union arch_fpu_ctx {                                            /*  VFP 上下文           		*/
    struct {
        double          FPUCTX_dfDreg[FP_DREG_NR];                      /*  32 个 double 寄存器         */
        UINT32          FPUCTX_uiFpscr;                                 /*  状态和控制寄存器            */
        UINT32          FPUCTX_uiFpscrCopy;                             /*  状态和控制寄存器的拷贝      */
    };
    struct {
        UINT32          SPECTX_uiGpr[SPE_GPR_NR];                       /*  32 个 GPR 的高 32 位        */
        UINT32          SPECTX_uiAcc[SPE_ACC_NR];                       /*  2 个 ACC 寄存器             */
        UINT32          SPECTX_uiSpefscr;                               /*  SPEFSCR 寄存器              */
    };
    struct {
        ALTIVEC_REG     VECCTX_regs[ALTIVEC_REG_NR];                    /*  32 个矢量寄存器             */
        UINT32          VECCTX_uiVscr[4];                               /*  VSCR                        */
        UINT32          VECCTX_uiVrsave;                                /*  VRSAVE                      */
        UINT32          VECCTX_uiPad0;                                  /*  16 字节大小对齐             */
        UINT32          VECCTX_uiPad1;
        UINT32          VECCTX_uiPad2;
    };
} ARCH_FPU_CTX;

#define ARCH_FPU_CTX_ALIGN      4                                       /* FPU CTX align size           */

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
  ppc-sylixos-eabi-gcc ... GNU
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

#endif
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __PPC_ARCH_FLOAT_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/

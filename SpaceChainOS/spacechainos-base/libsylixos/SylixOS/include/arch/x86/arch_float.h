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
** 文件创建日期: 2016 年 06 月 25 日
**
** 描        述: x86 浮点相关.
*********************************************************************************************************/

#ifndef __X86_ARCH_FLOAT_H
#define __X86_ARCH_FLOAT_H

/*********************************************************************************************************
  1: native-endian double  
     与处理器其他类型存储完全相同, 
                    
  2: mixed-endian double  
     混合大小端, (32位字内存储与处理器相同, 两个32位之间按照大端存储)
     
  x86 使用 native-endian
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL
/*********************************************************************************************************
  number of FP/MM and XMM registers on coprocessor
*********************************************************************************************************/

#define X86_FP_REGS_NR          8                                       /*  number of FP/MM registers   */

#if LW_CFG_CPU_WORD_LENGHT == 32
#define X86_XMM_REGS_NR         8                                       /*  number of XMM registers     */
#define X86_FP_RESERVED_NR      14                                      /*  reserved area in X_CTX      */
#else
#define X86_XMM_REGS_NR         16                                      /*  number of XMM registers     */
#define X86_FP_RESERVED_NR      6                                       /*  reserved area in X_CTX      */
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 32*/

#define X86_FP_XSAVE_HDR_SIZE   64                                      /*  size of the XSAVE header    */

/*********************************************************************************************************
  X86_FPU_DOUBLEX - double extended precision
*********************************************************************************************************/

typedef struct {
    UINT8                   DOUBLEX_ucF[10];                            /*  ST[0-7] or MM[0-7]          */
} X86_FPU_DOUBLEX;

/*********************************************************************************************************
  X86_FPU_DOUBLEX_SSE - double extended precision used in X86_FPU_X_CTX for SSE
*********************************************************************************************************/

typedef struct {
    UINT8                   SSE_ucXMM[16];                              /*  for 128 bits XMM registers  */
} X86_FPU_DOUBLEX_SSE;

/*********************************************************************************************************
  X86_FPU_OLD_CTX - Old FP context used by fsave/frstor instruction
*********************************************************************************************************/

typedef struct {
    INT                     OCTX_iFpcr;                                 /*  4    control word           */
    INT                     OCTX_iFpsr;                                 /*  4    status word            */
    INT                     OCTX_iFpTag;                                /*  4    tag word               */
    INT                     OCTX_iIP;                                   /*  4    inst pointer           */
    INT16                   OCTX_sCS;                                   /*  2    inst pointer selector  */
    INT16                   OCTX_sOP;                                   /*  2    last FP inst op code   */
    INT                     OCTX_iDP;                                   /*  4    data pointer           */
    INT                     OCTX_iDS;                                   /*  4    data pointer selector  */
    X86_FPU_DOUBLEX         OCTX_fpx[X86_FP_REGS_NR];                   /*  8*10 FR[0-7] non-TOS rel.   */
} X86_FPU_OLD_CTX;                                                      /*  108  bytes total            */

/*********************************************************************************************************
  X86_FPU_X_CTX - New FP context used by fxsave/fxrstor instruction
*********************************************************************************************************/

typedef struct {
    INT16                   XCTX_sFpcr;                                 /*  2     control word          */
    INT16                   XCTX_sFpsr;                                 /*  2     status word           */
    INT16                   XCTX_sFpTag;                                /*  2     tag word              */
    INT16                   XCTX_sOP;                                   /*  2     last FP inst op code  */
    INT                     XCTX_iIP;                                   /*  4     instruction pointer   */
    INT                     XCTX_iCS;                                   /*  4     inst pointer selector */
    INT                     XCTX_iDP;                                   /*  4     data pointer          */
    INT                     XCTX_iDS;                                   /*  4     data pointer selector */
    INT                     XCTX_iReserved0;                            /*  4     reserved              */
    INT                     XCTX_iReserved1;                            /*  4     reserved              */
    X86_FPU_DOUBLEX_SSE     XCTX_fpx[X86_FP_REGS_NR];                   /*  8*16  FR[0-7] non-TOS rel.  */
    X86_FPU_DOUBLEX_SSE     XCTX_xmm[X86_XMM_REGS_NR];                  /*  8*16  XMM[0-7]              */
    X86_FPU_DOUBLEX_SSE     XCTX_reserved2[X86_FP_RESERVED_NR];         /*  14*16 reserved              */
} X86_FPU_X_CTX;                                                        /*  512   bytes total           */

/*********************************************************************************************************
  X86_FPU_X_EXT_CTXT - Extended FP context used by xsave/xrstor instruction
*********************************************************************************************************/

typedef struct {
    INT16                   ECTX_sFpcr;                                 /*  2     control word          */
    INT16                   ECTX_sFpsr;                                 /*  2     status word           */
    INT16                   ECTX_sFpTag;                                /*  2     tag word              */
    INT16                   ECTX_sOP;                                   /*  2     last FP inst op code  */
    INT                     ECTX_iIP;                                   /*  4     instruction pointer   */
    INT                     ECTX_iCS;                                   /*  4     inst pointer selector */
    INT                     ECTX_iDP;                                   /*  4     data pointer          */
    INT                     ECTX_iDS;                                   /*  4     data pointer selector */
    INT                     ECTX_iReserved0;                            /*  4     reserved              */
    INT                     ECTX_iReserved1;                            /*  4     reserved              */
    X86_FPU_DOUBLEX_SSE     ECTX_fpx[X86_FP_REGS_NR];                   /*  8*16  FR[0-7] non-TOS rel.  */
    X86_FPU_DOUBLEX_SSE     ECTX_xmm[X86_XMM_REGS_NR];                  /*  8*16  XMM[0-7]              */
    X86_FPU_DOUBLEX_SSE     ECTX_reserved2[X86_FP_RESERVED_NR];         /*  14*16 reserved              */
    UINT8                   ECTX_ucXSaveHeader[X86_FP_XSAVE_HDR_SIZE];  /*  64    XSave header          */
    X86_FPU_DOUBLEX_SSE     ECTX_ymm[X86_XMM_REGS_NR];                  /*  8*16                        */
                                                                        /*  higher 128 bits of YMM[0-7] */
} X86_FPU_X_EXT_CTX;

/*********************************************************************************************************
  线程浮点运算器上下文
*********************************************************************************************************/

#define ARCH_FPU_CTX_ALIGN      64                                      /*  FPU CTX align size          */

union arch_fpu_ctx {
    X86_FPU_OLD_CTX         FPUCTX_oldCtx;                              /*  x87 MMX context             */
    X86_FPU_X_CTX           FPUCTX_XCtx;                                /*  SSE context                 */
    X86_FPU_X_EXT_CTX       FPUCTX_XExtCtx;                             /*  Extended FP context         */
    UINT8                   FPUCTX_ucXExtSz[LW_CFG_CPU_FPU_XSAVE_SIZE]; /*  Extended FP context Max Size*/
} __attribute__ ((packed, aligned(ARCH_FPU_CTX_ALIGN)));                /*  !按最大 CACHE 对齐大小对齐! */

typedef union arch_fpu_ctx  ARCH_FPU_CTX;

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
  i386-sylixos-elf-gcc ... GNU
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

#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __X86_ARCH_FLOAT_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/

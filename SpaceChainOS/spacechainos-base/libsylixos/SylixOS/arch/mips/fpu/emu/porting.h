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
** 文   件   名: porting.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 11 月 24 日
**
** 描        述: MIPS FPU 模拟移植文件.
*********************************************************************************************************/

#ifndef __MIPS_FPUEMU_PORTING_H
#define __MIPS_FPUEMU_PORTING_H

/*********************************************************************************************************
  基本数据类型与定义
*********************************************************************************************************/

typedef BOOL                            bool;

#ifndef true
#define true                            LW_TRUE
#endif

#ifndef false
#define false                           LW_FALSE
#endif

#define _ULCAST_                        (unsigned long)

#define __cold

#define __maybe_unused                  __attribute__((unused))

#define IS_ENABLED(x)                   (x)

#define perf_sw_event(a, b, c, d)       do { } while (0)

/*
 * 注意: 始终认为可以访问，如果地址不合法，可能会产生异常嵌套，可以通过异常嵌套时打印的调用栈分析应用
 * 中有问题的代码
 */
#define access_ok(type, va, len)        1

#define put_user(val, va)               (*va = val, 0)
#define __put_user(val, va)             (*va = val, 0)

#define get_user(val, va)               (val = *va, 0)
#define __get_user(val, va)             (val = *va, 0)

#define cond_resched()                  do { } while (0)

#define preempt_disable()               do { } while (0)
#define preempt_enable()                do { } while (0)

#define unreachable()                   do { } while (0)

#define cp0_epc                         REG_uiCP0EPC
#define regs                            REG_uiReg

#define fpr                             FPUCTX_reg
#define fcr31                           FPUCTX_uiFpcsr

/*********************************************************************************************************
  调试相关
*********************************************************************************************************/

#ifdef CONFIG_DEBUG_FPU_EMU

struct mips_fpu_emulator_stats {
    unsigned long emulated;
    unsigned long loads;
    unsigned long stores;
    unsigned long cp1ops;
    unsigned long cp1xops;
    unsigned long errors;
    unsigned long ieee754_inexact;
    unsigned long ieee754_underflow;
    unsigned long ieee754_overflow;
    unsigned long ieee754_zerodiv;
    unsigned long ieee754_invalidop;
    unsigned long ds_emul;
};

extern struct mips_fpu_emulator_stats fpuemustats[LW_CFG_MAX_PROCESSORS];

#define MIPS_FPU_EMU_INC_STATS(M)           \
do {                                        \
    fpuemustats[LW_CPU_GET_CUR_ID()].M++;   \
} while (0)

#else
#define MIPS_FPU_EMU_INC_STATS(M)           do { } while (0)
#endif /* CONFIG_DEBUG_FPU_EMU */

/*********************************************************************************************************
  FPU REG 访问
*********************************************************************************************************/

#if BYTE_ORDER == BIG_ENDIAN
# define FPR_IDX(width, idx)    ((idx) ^ ((64 / (width)) - 1))
#else
# define FPR_IDX(width, idx)    (idx)
#endif

#define BUILD_FPR_ACCESS(width) \
static inline UINT##width get_fpr##width(union fpureg *fpr, unsigned idx)  \
{                                   \
    return fpr->val##width[FPR_IDX(width, idx)];            \
}                                   \
                                    \
static inline void set_fpr##width(union fpureg *fpr, unsigned idx,  \
                  UINT##width val)             \
{                                   \
    fpr->val##width[FPR_IDX(width, idx)] = val;         \
}

BUILD_FPR_ACCESS(32)
BUILD_FPR_ACCESS(64)

/*********************************************************************************************************
  最大最小值
*********************************************************************************************************/

#define U8_MAX      ((u8)~0U)
#define S8_MAX      ((s8)(U8_MAX>>1))
#define S8_MIN      ((s8)(-S8_MAX - 1))
#define U16_MAX     ((u16)~0U)
#define S16_MAX     ((s16)(U16_MAX>>1))
#define S16_MIN     ((s16)(-S16_MAX - 1))
#define U32_MAX     ((u32)~0U)
#define S32_MAX     ((s32)(U32_MAX>>1))
#define S32_MIN     ((s32)(-S32_MAX - 1))
#define U64_MAX     ((u64)~0ULL)
#define S64_MAX     ((s64)(U64_MAX>>1))
#define S64_MIN     ((s64)(-S64_MAX - 1))

/*********************************************************************************************************
  打印信息
*********************************************************************************************************/

#define pr_info(args...)
#define pr_warn(args...)
#define pr_err(args...)
#define pr_debug(args...)

/*********************************************************************************************************
  获得当前 CPU 当前线程的 FPU 寄存器
*********************************************************************************************************/

extern ARCH_FPU_CTX  _G_mipsFpuCtx[LW_CFG_MAX_PROCESSORS];

#define GET_CUR_THREAD_FPU_FPR          \
        _G_mipsFpuCtx[LW_CPU_GET_CUR_ID()].FPUCTX_reg

#define GET_CUR_THREAD_FPU_FCR31        \
        _G_mipsFpuCtx[LW_CPU_GET_CUR_ID()].FPUCTX_uiFpcsr

/*********************************************************************************************************
  microMIPS bitfields
*********************************************************************************************************/

#define MM_POOL32A_MINOR_MASK           0x3f
#define MM_POOL32A_MINOR_SHIFT          0x6
#define MM_MIPS32_COND_FC               0x30

/*********************************************************************************************************
  MIPS FPU 寄存器相关定义
*********************************************************************************************************/

/*
 * Bits in the MIPS32/64 coprocessor 1 (FPU) revision register.
 */
#define MIPS_FPIR_S         (_ULCAST_(1) << 16)
#define MIPS_FPIR_D         (_ULCAST_(1) << 17)
#define MIPS_FPIR_PS        (_ULCAST_(1) << 18)
#define MIPS_FPIR_3D        (_ULCAST_(1) << 19)
#define MIPS_FPIR_W         (_ULCAST_(1) << 20)
#define MIPS_FPIR_L         (_ULCAST_(1) << 21)
#define MIPS_FPIR_F64       (_ULCAST_(1) << 22)
#define MIPS_FPIR_HAS2008   (_ULCAST_(1) << 23)
#define MIPS_FPIR_UFRP      (_ULCAST_(1) << 28)
#define MIPS_FPIR_FREP      (_ULCAST_(1) << 29)

/*
 * Bits in the MIPS32/64 coprocessor 1 (FPU) condition codes register.
 */
#define MIPS_FCCR_CONDX_S   0
#define MIPS_FCCR_CONDX     (_ULCAST_(255) << MIPS_FCCR_CONDX_S)
#define MIPS_FCCR_COND0_S   0
#define MIPS_FCCR_COND0     (_ULCAST_(1) << MIPS_FCCR_COND0_S)
#define MIPS_FCCR_COND1_S   1
#define MIPS_FCCR_COND1     (_ULCAST_(1) << MIPS_FCCR_COND1_S)
#define MIPS_FCCR_COND2_S   2
#define MIPS_FCCR_COND2     (_ULCAST_(1) << MIPS_FCCR_COND2_S)
#define MIPS_FCCR_COND3_S   3
#define MIPS_FCCR_COND3     (_ULCAST_(1) << MIPS_FCCR_COND3_S)
#define MIPS_FCCR_COND4_S   4
#define MIPS_FCCR_COND4     (_ULCAST_(1) << MIPS_FCCR_COND4_S)
#define MIPS_FCCR_COND5_S   5
#define MIPS_FCCR_COND5     (_ULCAST_(1) << MIPS_FCCR_COND5_S)
#define MIPS_FCCR_COND6_S   6
#define MIPS_FCCR_COND6     (_ULCAST_(1) << MIPS_FCCR_COND6_S)
#define MIPS_FCCR_COND7_S   7
#define MIPS_FCCR_COND7     (_ULCAST_(1) << MIPS_FCCR_COND7_S)

/*
 * Bits in the MIPS32/64 coprocessor 1 (FPU) enables register.
 */
#define MIPS_FENR_FS_S      2
#define MIPS_FENR_FS        (_ULCAST_(1) << MIPS_FENR_FS_S)

/*
 * FPU Status Register Values
 */
#define FPU_CSR_COND_S  23                  /* $fcc0 */
#define FPU_CSR_COND    (_ULCAST_(1) << FPU_CSR_COND_S)

#define FPU_CSR_FS_S    24      /* flush denormalised results to 0 */
#define FPU_CSR_FS      (_ULCAST_(1) << FPU_CSR_FS_S)

#define FPU_CSR_CONDX_S 25                  /* $fcc[7:1] */
#define FPU_CSR_CONDX   (_ULCAST_(127) << FPU_CSR_CONDX_S)
#define FPU_CSR_COND1_S 25                  /* $fcc1 */
#define FPU_CSR_COND1   (_ULCAST_(1) << FPU_CSR_COND1_S)
#define FPU_CSR_COND2_S 26                  /* $fcc2 */
#define FPU_CSR_COND2   (_ULCAST_(1) << FPU_CSR_COND2_S)
#define FPU_CSR_COND3_S 27                  /* $fcc3 */
#define FPU_CSR_COND3   (_ULCAST_(1) << FPU_CSR_COND3_S)
#define FPU_CSR_COND4_S 28                  /* $fcc4 */
#define FPU_CSR_COND4   (_ULCAST_(1) << FPU_CSR_COND4_S)
#define FPU_CSR_COND5_S 29                  /* $fcc5 */
#define FPU_CSR_COND5   (_ULCAST_(1) << FPU_CSR_COND5_S)
#define FPU_CSR_COND6_S 30                  /* $fcc6 */
#define FPU_CSR_COND6   (_ULCAST_(1) << FPU_CSR_COND6_S)
#define FPU_CSR_COND7_S 31                  /* $fcc7 */
#define FPU_CSR_COND7   (_ULCAST_(1) << FPU_CSR_COND7_S)

/*
 * Bits 22:20 of the FPU Status Register will be read as 0,
 * and should be written as zero.
 */
#define FPU_CSR_RSVD    (_ULCAST_(7) << 20)

#define FPU_CSR_ABS2008 (_ULCAST_(1) << 19)
#define FPU_CSR_NAN2008 (_ULCAST_(1) << 18)

/*
 * X the exception cause indicator
 * E the exception enable
 * S the sticky/flag bit
*/
#define FPU_CSR_ALL_X   0x0003f000
#define FPU_CSR_UNI_X   0x00020000
#define FPU_CSR_INV_X   0x00010000
#define FPU_CSR_DIV_X   0x00008000
#define FPU_CSR_OVF_X   0x00004000
#define FPU_CSR_UDF_X   0x00002000
#define FPU_CSR_INE_X   0x00001000

#define FPU_CSR_ALL_E   0x00000f80
#define FPU_CSR_INV_E   0x00000800
#define FPU_CSR_DIV_E   0x00000400
#define FPU_CSR_OVF_E   0x00000200
#define FPU_CSR_UDF_E   0x00000100
#define FPU_CSR_INE_E   0x00000080

#define FPU_CSR_ALL_S   0x0000007c
#define FPU_CSR_INV_S   0x00000040
#define FPU_CSR_DIV_S   0x00000020
#define FPU_CSR_OVF_S   0x00000010
#define FPU_CSR_UDF_S   0x00000008
#define FPU_CSR_INE_S   0x00000004

/* Bits 0 and 1 of FPU Status Register specify the rounding mode */
#define FPU_CSR_RM  0x00000003
#define FPU_CSR_RN  0x0 /* nearest */
#define FPU_CSR_RZ  0x1 /* towards zero */
#define FPU_CSR_RU  0x2 /* towards +Infinity */
#define FPU_CSR_RD  0x3 /* towards -Infinity */

/*
 * Macros for handling the ISA mode bit for MIPS16 and microMIPS.
 */
#if defined(CONFIG_SYS_SUPPORTS_MIPS16) || \
    defined(CONFIG_SYS_SUPPORTS_MICROMIPS)
#define get_isa16_mode(x)       ((x) & 0x1)
#define msk_isa16_mode(x)       ((x) & ~0x1)
#define set_isa16_mode(x)       do { (x) |= 0x1; } while(0)
#else
#define get_isa16_mode(x)       0
#define msk_isa16_mode(x)       (x)
#define set_isa16_mode(x)       do { } while(0)
#endif

/*
 * microMIPS instructions can be 16-bit or 32-bit in length. This
 * returns a 1 if the instruction is 16-bit and a 0 if 32-bit.
 */
static inline int mm_insn_16bit(u16 insn)
{
    u16 opcode = (insn >> 10) & 0x7;

    return (opcode >= 1 && opcode <= 3) ? 1 : 0;
}

#endif                                                                  /*  __MIPS_FPUEMU_PORTING_H     */
/*********************************************************************************************************
  END
*********************************************************************************************************/

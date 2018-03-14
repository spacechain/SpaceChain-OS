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
** 文   件   名: assembler.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 06 月 25 日
**
** 描        述: x86 汇编相关.
*********************************************************************************************************/

#ifndef __ASMX86_ASSEMBLER_H
#define __ASMX86_ASSEMBLER_H

/*********************************************************************************************************
  x86 architecture assembly special code
*********************************************************************************************************/

#if defined(__ASSEMBLY__) || defined(ASSEMBLY)

#ifndef __MP_CFG_H
#include "../SylixOS/config/mp/mp_cfg.h"
#endif

/*********************************************************************************************************
  assembler define
*********************************************************************************************************/

#ifdef __GNUC__
#  define EXPORT_LABEL(label)       .global label
#  define IMPORT_LABEL(label)       .extern label

#  define FUNC_LABEL(func)          func:
#  define LINE_LABEL(line)          line:

#  define FUNC_DEF(func)  \
        .type   func, %function;  \
func:

#  define FUNC_END(func)    \
        .size   func, . - func
        
#  define MACRO_DEF(mfunc...)   \
        .macro  mfunc
        
#  define MACRO_END()   \
        .endm

#  define FILE_BEGIN()  \
        .text

#  define FILE_END()    \
        .end
        
#  define SECTION(sec)  \
        .section sec

#  define WEAK(sym)     \
        .weak   sym

#else                                                                   /*  __GNUC__                    */

#endif                                                                  /*  !__GNUC__                   */

/*********************************************************************************************************
  参数的 SP 偏移
*********************************************************************************************************/

#if LW_CFG_CPU_WORD_LENGHT == 32

#define X86_SP_ARG0             0
#define X86_SP_ARG1             4
#define X86_SP_ARG1W            6
#define X86_SP_ARG2             8
#define X86_SP_ARG2W            10
#define X86_SP_ARG3             12
#define X86_SP_ARG3W            14
#define X86_SP_ARG4             16
#define X86_SP_ARG5             20
#define X86_SP_ARG6             24
#define X86_SP_ARG7             28
#define X86_SP_ARG8             32
#define X86_SP_ARG9             36
#define X86_SP_ARG10            40
#define X86_SP_ARG11            44
#define X86_SP_ARG12            48

/*********************************************************************************************************
  fp offsets to arguments
*********************************************************************************************************/

#define X86_FP_ARG1             8
#define X86_FP_ARG1W            10
#define X86_FP_ARG2             12
#define X86_FP_ARG2W            14
#define X86_FP_ARG3             16
#define X86_FP_ARG3W            18
#define X86_FP_ARG4             20
#define X86_FP_ARG5             24
#define X86_FP_ARG6             28
#define X86_FP_ARG7             32
#define X86_FP_ARG8             36
#define X86_FP_ARG9             40
#define X86_FP_ARG10            44
#define X86_FP_ARG11            48
#define X86_FP_ARG12            52

/*********************************************************************************************************
  double arguments
*********************************************************************************************************/

#define X86_DOUBLE_DARG1        8
#define X86_DOUBLE_DARG1L       12
#define X86_DOUBLE_DARG2        16
#define X86_DOUBLE_DARG2L       20
#define X86_DOUBLE_DARG3        24
#define X86_DOUBLE_DARG3L       28
#define X86_DOUBLE_DARG4        32
#define X86_DOUBLE_DARG4L       36

#else

/*********************************************************************************************************

  URL: https://en.wikipedia.org/wiki/X86_calling_conventions

  The calling convention of the System V AMD64 ABI is followed on Solaris, Linux, FreeBSD, macOS,
  and is the de facto standard among Unix and Unix-like operating systems.

  The first six integer or pointer arguments are passed in registers RDI, RSI, RDX, RCX
  (R10 in the Linux kernel interface), R8, and R9,
  while XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6 and XMM7 are used for certain floating point arguments.
  As in the Microsoft x64 calling convention, additional arguments are passed on the stack
  and the return value is stored in RAX and RDX.

  If the callee wishes to use registers RBP, RBX, and R12–R15,
  it must restore their original values before returning control to the caller.
  All others must be saved by the caller if it wishes to preserve their values.

  If the callee is a variadic function, then the number of floating point arguments passed to
  the function in vector registers must be provided by the caller in the RAX register.

  Unlike the Microsoft calling convention, a shadow space is not provided;
  on function entry, the return address is adjacent to the seventh integer argument on the stack.

  <System V Application Binary Interface AMD64 Architecture Processor Supplement>

  Here are a table of usage of the registers:

    +-----------------------------------------------------------------------------------+
    |                                                                  Preserved across |
    |  Register                         Usage                          function calls   |
    +-----------------------------------------------------------------------------------+
    |  %rax         temporary register; with variable arguments        No               |
    |               passes information about the number of SSE reg+                     |
    |               isters used; 1st return register                                    |
    +-----------------------------------------------------------------------------------+
    |  %rbx         callee-saved register; optionally used as base     Yes              |
    |               pointer                                                             |
    +-----------------------------------------------------------------------------------+
    |  %rcx         used to pass 4th integer argument to functions     No               |
    +-----------------------------------------------------------------------------------+
    |  %rdx         used to pass 3rd argument to functions; 2nd return No               |
    |               register                                                            |
    +-----------------------------------------------------------------------------------+
    |  %rsp         stack pointer                                      Yes              |
    +-----------------------------------------------------------------------------------+
    |  %rbp         callee-saved register; optionally used as frame    Yes              |
    |               pointer                                                             |
    +-----------------------------------------------------------------------------------+
    |  %rsi         used to pass 2nd argument to functions             No               |
    +-----------------------------------------------------------------------------------+
    |  %rdi         used to pass 1st argument to functions             No               |
    +-----------------------------------------------------------------------------------+
    |  %r8          used to pass 5th argument to functions             No               |
    +-----------------------------------------------------------------------------------+
    |  %r9          used to pass 6th argument to functions             No               |
    +-----------------------------------------------------------------------------------+
    |  %r10         temporary register, used for passing a functions   No               |
    |               static chain pointer                                                |
    +-----------------------------------------------------------------------------------+
    |  %r11         temporary register                                 No               |
    +-----------------------------------------------------------------------------------+
    |  %r12|r15     callee-saved registers                             Yes              |
    +-----------------------------------------------------------------------------------+
    |  %xmm0 %xmm1  used to pass and return floating point arguments   No               |
    +-----------------------------------------------------------------------------------+
    |  %xmm2 %xmm7  used to pass floating point arguments              No               |
    +-----------------------------------------------------------------------------------+
    |  %xmm8 %xmm15 temporary registers                                No               |
    +-----------------------------------------------------------------------------------+
    |  %mmx0 %mmx7  temporary registers                                No               |
    +-----------------------------------------------------------------------------------+
    |  %st0,%st1    temporary registers;                               No               |
    |               used to return long double arguments                                |
    +-----------------------------------------------------------------------------------+
    |  %st2 %st7    temporary registers                                No               |
    +-----------------------------------------------------------------------------------+
    |  %fs          Reserved for system                                No               |
    |               (as thread specific data register)                                  |
    +-----------------------------------------------------------------------------------+
    |  mxcsr        SSE2 control and status word                       partial          |
    +-----------------------------------------------------------------------------------+
    |  x87 SW       x87 status word                                    No               |
    +-----------------------------------------------------------------------------------+
    |  x87 CW       x87 control word                                   Yes              |
    +-----------------------------------------------------------------------------------+
    |  %bnd0–%bnd3  used to pass/return bounds of pointer              No               |
    |               arguments/return values                                             |
    +-----------------------------------------------------------------------------------+
*********************************************************************************************************/

/*********************************************************************************************************
  x86-64 前 6 个非浮点参数的使用寄存器传递
*********************************************************************************************************/

#define X86_64_ARG0             %RDI
#define X86_64_ARG1             %RSI
#define X86_64_ARG2             %RDX
#define X86_64_ARG3             %RCX
#define X86_64_ARG4             %R8
#define X86_64_ARG5             %R9

#define X86_64_ARG0DW           %EDI
#define X86_64_ARG1DW           %ESI
#define X86_64_ARG2DW           %EDX
#define X86_64_ARG3DW           %ECX
#define X86_64_ARG4DW           %R8D
#define X86_64_ARG5DW           %R9D

#define X86_64_RETREG           %RAX

#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 32*/

/*********************************************************************************************************
  size define
*********************************************************************************************************/

#ifndef LW_CFG_KB_SIZE
#define LW_CFG_KB_SIZE          (1024)
#define LW_CFG_MB_SIZE          (1024 * LW_CFG_KB_SIZE)
#define LW_CFG_GB_SIZE          (1024 * LW_CFG_MB_SIZE)
#endif

#endif                                                                  /*  __ASSEMBLY__                */

/*********************************************************************************************************
  CR0 寄存器相关位定义
*********************************************************************************************************/

#define X86_CR0_PE              0x00000001                      /*  Protection Enable                   */
#define X86_CR0_MP              0x00000002                      /*  Monitor coProcessor                 */
#define X86_CR0_EM              0x00000004                      /*  Emulation                           */
#define X86_CR0_TS              0x00000008                      /*  Task Switched                       */
#define X86_CR0_ET              0x00000010                      /*  Extension Type                      */
#define X86_CR0_NE              0x00000020                      /*  Numeric Error                       */
#define X86_CR0_WP              0x00010000                      /*  Write Protect                       */
#define X86_CR0_AM              0x00040000                      /*  Alignment Mask                      */
#define X86_CR0_NW              0x20000000                      /*  Not Writethrough                    */
#define X86_CR0_CD              0x40000000                      /*  Cache Disable                       */
#define X86_CR0_PG              0x80000000                      /*  Paging                              */
#define X86_CR0_NW_NOT          0xdfffffff                      /*  Cache write through                 */
#define X86_CR0_CD_NOT          0xbfffffff                      /*  Cache disable                       */

/*********************************************************************************************************
  CR4 寄存器相关位定义
*********************************************************************************************************/

#define X86_CR4_VME             0x00000001                      /*  virtual-8086 mode extensions        */
#define X86_CR4_PVI             0x00000002                      /*  protected-mode virtual interrupts   */
#define X86_CR4_TSD             0x00000004                      /*  timestamp disable                   */
#define X86_CR4_DE              0x00000008                      /*  debugging extensions                */
#define X86_CR4_PSE             0x00000010                      /*  page size extensions                */
#define X86_CR4_PAE             0x00000020                      /*  physical address extension          */
#define X86_CR4_MCE             0x00000040                      /*  machine check enable                */
#define X86_CR4_PGE             0x00000080                      /*  page global enable                  */
#define X86_CR4_PCE             0x00000100                      /*  performance-monitoring enable       */
#define X86_CR4_OSFXSR          0x00000200                      /*  use fxsave/fxrstor instructions     */
#define X86_CR4_OSXMMEXCEPT     0x00000400                      /*  streaming SIMD exception            */
#define X86_CR4_OSXSAVE         0x00040000                      /*  use xsave/xrstor instructions       */

/*********************************************************************************************************
  EFLAGS 位
*********************************************************************************************************/

#define _BITUL(n)               (1 << (n))

#define X86_EFLAGS_CF_BIT       0                                       /*  Carry Flag                  */
#define X86_EFLAGS_CF           _BITUL(X86_EFLAGS_CF_BIT)
#define X86_EFLAGS_FIXED_BIT    1                                       /*  Bit 1 - always on           */
#define X86_EFLAGS_FIXED        _BITUL(X86_EFLAGS_FIXED_BIT)
#define X86_EFLAGS_PF_BIT       2                                       /*  Parity Flag                 */
#define X86_EFLAGS_PF           _BITUL(X86_EFLAGS_PF_BIT)
#define X86_EFLAGS_AF_BIT       4                                       /*  Auxiliary carry Flag        */
#define X86_EFLAGS_AF           _BITUL(X86_EFLAGS_AF_BIT)
#define X86_EFLAGS_ZF_BIT       6                                       /*  Zero Flag                   */
#define X86_EFLAGS_ZF           _BITUL(X86_EFLAGS_ZF_BIT)
#define X86_EFLAGS_SF_BIT       7                                       /*  Sign Flag                   */
#define X86_EFLAGS_SF           _BITUL(X86_EFLAGS_SF_BIT)
#define X86_EFLAGS_TF_BIT       8                                       /*  Trap Flag                   */
#define X86_EFLAGS_TF           _BITUL(X86_EFLAGS_TF_BIT)
#define X86_EFLAGS_IF_BIT       9                                       /*  Interrupt Flag              */
#define X86_EFLAGS_IF           _BITUL(X86_EFLAGS_IF_BIT)
#define X86_EFLAGS_DF_BIT       10                                      /*  Direction Flag              */
#define X86_EFLAGS_DF           _BITUL(X86_EFLAGS_DF_BIT)
#define X86_EFLAGS_OF_BIT       11                                      /*  Overflow Flag               */
#define X86_EFLAGS_OF           _BITUL(X86_EFLAGS_OF_BIT)
#define X86_EFLAGS_IOPL_BIT     12                                      /*  I/O Privilege Level (2 bits)*/
#define X86_EFLAGS_IOPL         (_AC(3,UL) << X86_EFLAGS_IOPL_BIT)
#define X86_EFLAGS_NT_BIT       14                                      /*  Nested Task                 */
#define X86_EFLAGS_NT           _BITUL(X86_EFLAGS_NT_BIT)
#define X86_EFLAGS_RF_BIT       16                                      /*  Resume Flag                 */
#define X86_EFLAGS_RF           _BITUL(X86_EFLAGS_RF_BIT)
#define X86_EFLAGS_VM_BIT       17                                      /*  Virtual Mode                */
#define X86_EFLAGS_VM           _BITUL(X86_EFLAGS_VM_BIT)
#define X86_EFLAGS_AC_BIT       18                                      /*  Align Check/Access Control  */
#define X86_EFLAGS_AC           _BITUL(X86_EFLAGS_AC_BIT)
#define X86_EFLAGS_VIF_BIT      19                                      /*  Virtual Interrupt Flag      */
#define X86_EFLAGS_VIF          _BITUL(X86_EFLAGS_VIF_BIT)
#define X86_EFLAGS_VIP_BIT      20                                      /*  Virtual Interrupt Pending   */
#define X86_EFLAGS_VIP          _BITUL(X86_EFLAGS_VIP_BIT)
#define X86_EFLAGS_ID_BIT       21                                      /*  CPUID detection             */
#define X86_EFLAGS_ID           _BITUL(X86_EFLAGS_ID_BIT)

/*********************************************************************************************************
  CLFLUSH
*********************************************************************************************************/

#define X86_CLFLUSH_DEF_BYTES   64                                      /*  def bytes flushed by CLFLUSH*/
#define X86_CLFLUSH_MAX_BYTES   255                                     /*  max bytes flushed by CLFLUSH*/

/*********************************************************************************************************
  CPUID: signature 位定义
*********************************************************************************************************/

#define X86_CPUID_STEPID        0x0000000f                      /*  processor stepping id mask          */
#define X86_CPUID_MODEL         0x000000f0                      /*  processor model mask                */
#define X86_CPUID_FAMILY        0x00000f00                      /*  processor family mask               */
#define X86_CPUID_TYPE          0x00003000                      /*  processor type mask                 */
#define X86_CPUID_EXT_MODEL     0x000f0000                      /*  processor extended model mask       */
#define X86_CPUID_EXT_FAMILY    0x0ff00000                      /*  processor extended family mask      */
#define X86_CPUID_486           0x00000400                      /*  family: 486                         */
#define X86_CPUID_PENTIUM       0x00000500                      /*  family: Pentium                     */
#define X86_CPUID_PENTIUMPRO    0x00000600                      /*  family: Pentium PRO                 */
#define X86_CPUID_EXTENDED      0x00000f00                      /*  family: Extended                    */
#define X86_CPUID_PENTIUM4      0x00000000                      /*  extended family: PENTIUM4           */
#define X86_CPUID_ORIG          0x00000000                      /*  type: original OEM                  */
#define X86_CPUID_OVERD         0x00001000                      /*  type: overdrive                     */
#define X86_CPUID_DUAL          0x00002000                      /*  type: dual                          */
#define X86_CPUID_CHUNKS        0x0000ff00                      /*  bytes flushed by CLFLUSH mask       */
#define X86_CPUID_CHUNKS_TO_BYTES_SHIFT  5                      /*  convert chunks field to byte count  */

/*********************************************************************************************************
  CPUID: feature 位定义
*********************************************************************************************************/

#define X86_CPUID_FPU           0x00000001                      /*  FPU on chip                         */
#define X86_CPUID_VME           0x00000002                      /*  virtual 8086 mode enhancement       */
#define X86_CPUID_DE            0x00000004                      /*  debugging extensions                */
#define X86_CPUID_PSE           0x00000008                      /*  page size extension                 */
#define X86_CPUID_TSC           0x00000010                      /*  time stamp counter                  */
#define X86_CPUID_MSR           0x00000020                      /*  RDMSR and WRMSR support             */
#define X86_CPUID_PAE           0x00000040                      /*  physical address extensions         */
#define X86_CPUID_MCE           0x00000080                      /*  machine check exception             */
#define X86_CPUID_CXS           0x00000100                      /*  CMPXCHG8 inst                       */
#define X86_CPUID_APIC          0x00000200                      /*  APIC on chip                        */
#define X86_CPUID_SEP           0x00000800                      /*  SEP, Fast System Call               */
#define X86_CPUID_MTRR          0x00001000                      /*  MTRR                                */
#define X86_CPUID_PGE           0x00002000                      /*  PTE global bit                      */
#define X86_CPUID_MCA           0x00004000                      /*  machine check arch.                 */
#define X86_CPUID_CMOV          0x00008000                      /*  cond. move/cmp. inst                */
#define X86_CPUID_PAT           0x00010000                      /*  page attribute table                */
#define X86_CPUID_PSE36         0x00020000                      /*  36 bit page size extension          */
#define X86_CPUID_PSNUM         0x00040000                      /*  processor serial number             */
#define X86_CPUID_CLFLUSH       0x00080000                      /*  CLFLUSH inst supported              */
#define X86_CPUID_DTS           0x00200000                      /*  Debug Store                         */
#define X86_CPUID_ACPI          0x00400000                      /*  TM and SCC supported                */
#define X86_CPUID_MMX           0x00800000                      /*  MMX technology supported            */
#define X86_CPUID_FXSR          0x01000000                      /*  fast FP save and restore            */
#define X86_CPUID_SSE           0x02000000                      /*  SSE supported                       */
#define X86_CPUID_SSE2          0x04000000                      /*  SSE2 supported                      */
#define X86_CPUID_SS            0x08000000                      /*  Self Snoop supported                */
#define X86_CPUID_HTT           0x10000000                      /*  Hyper Threading Technology          */
#define X86_CPUID_TM            0x20000000                      /*  Thermal Monitor supported           */

/*********************************************************************************************************
  CPUID: extended feature 位定义
*********************************************************************************************************/

#define X86_CPUID_GV3           0x00000080                      /*  Geyserville 3 supported             */
#define X86_CPUID_TM2           0x00000100                      /*  Thermal Monitor 2 supported         */

/*********************************************************************************************************
  CPUID: X86_CPUID 结构内的偏移
*********************************************************************************************************/

#define X86_CPUID_HIGHVALUE     0                               /*  offset to highestValue              */
#define X86_CPUID_VENDORID      4                               /*  offset to vendorId                  */
#define X86_CPUID_SIGNATURE     16                              /*  offset to signature                 */
#define X86_CPUID_FEATURES_EBX  20                              /*  offset to featuresEbx               */
#define X86_CPUID_FEATURES_ECX  24                              /*  offset to featuresEcx               */
#define X86_CPUID_FEATURES_EDX  28                              /*  offset to featuresEdx               */
#define X86_CPUID_CACHE_EAX     32                              /*  offset to cacheEax                  */
#define X86_CPUID_CACHE_EBX     36                              /*  offset to cacheEbx                  */
#define X86_CPUID_CACHE_ECX     40                              /*  offset to cacheEcx                  */
#define X86_CPUID_CACHE_EDX     44                              /*  offset to cacheEdx                  */
#define X86_CPUID_SERIALNO      48                              /*  offset to serialNo64                */
#define X86_CPUID_BRAND_STR     56                              /*  offset to brandString[0]            */

#endif                                                                  /*  __ASMX86_ASSEMBLER_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/

/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: k_const.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 12 日
**
** 描        述: 这是系统控制常量定义。

** BUG:
2009.07.28  更新对齐内存大小计算方法.
*********************************************************************************************************/

#ifndef  __K_CONST_H
#define  __K_CONST_H

/*********************************************************************************************************
  全局就绪表
*********************************************************************************************************/

#define LW_GLOBAL_RDY_PCBBMAP()     (&(_K_pcbbmapGlobalReady))
#define LW_GLOBAL_RDY_BMAP()        (&(_K_pcbbmapGlobalReady.PCBM_bmap))
#define LW_GLOBAL_RDY_PPCB(prio)    (&(_K_pcbbmapGlobalReady.PCBM_pcb[prio]))

/*********************************************************************************************************
  TCB 的字节数定义，相对于字对齐的字节数
*********************************************************************************************************/

#define __SEGMENT_BLOCK_SIZE_ALIGN  ROUND_UP(sizeof(LW_CLASS_SEGMENT), LW_CFG_HEAP_ALIGNMENT)

#if LW_CFG_COROUTINE_EN > 0
#define __CRCB_SIZE_ALIGN           ROUND_UP(sizeof(LW_CLASS_COROUTINE), sizeof(LW_STACK))
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */

/*********************************************************************************************************
  最小堆栈的字节数
*********************************************************************************************************/

#define __STK_MINMUM_BYTE_SIZE      (ARCH_STK_MIN_WORD_SIZE * sizeof(LW_STACK))

/*********************************************************************************************************
  EVENT WAIT PRIORITY QUEUE
*********************************************************************************************************/

#define __EVENT_Q_SIZE              8                                   /*  Q HASH SIZE                 */
#define __EVENT_Q_EPRIO             32                                  /*  256 / 8                     */
#define __EVENT_Q_SHIFT             5                                   /*  2 ^ 5 = 32                  */

/*********************************************************************************************************
  内核配置
*********************************************************************************************************/

#define LW_KERN_FLAG_INT_FPU        0x01                                /*  中断中是否可以使用浮点      */
#define LW_KERN_FLAG_BUG_REBOOT     0x02                                /*  内核探测到 BUG 是否需要重启 */
#define LW_KERN_FLAG_SMP_FSCHED     0x04                                /*  SMP 是否使能快速调度        */
                                                                        /*  快速调度将产生大量的核间中断*/
#define LW_KERN_FLAG_SMT_BSCHED     0x08                                /*  SMT 均衡调度                */
#define LW_KERN_FLAG_NO_ITIMER      0x10                                /*  不支持 ITIMER_REAL          */
                                                                        /*         ITIMER_VIRTUAL       */
                                                                        /*         ITIMER_PROF          */
                                                                        /*  提高 tick 中断执行速度      */
#define LW_KERN_FLAG_TMCVT_SIMPLE   0x20                                /*  timespec 转换为 tick 方法   */

/*********************************************************************************************************
  内核是否支持浮点状态
*********************************************************************************************************/

#define LW_KERN_FPU_EN_SET(en)                              \
        do {                                                \
            if (en) {                                       \
                _K_ulKernFlags |= LW_KERN_FLAG_INT_FPU;     \
            } else {                                        \
                _K_ulKernFlags &= ~LW_KERN_FLAG_INT_FPU;    \
            }                                               \
        } while (0)
#define LW_KERN_FPU_EN_GET()        (_K_ulKernFlags & LW_KERN_FLAG_INT_FPU)

/*********************************************************************************************************
  内核 bug 检测到 bug 后是否重启
*********************************************************************************************************/

#define LW_KERN_BUG_REBOOT_EN_SET(en)                       \
        do {                                                \
            if (en) {                                       \
                _K_ulKernFlags |= LW_KERN_FLAG_BUG_REBOOT;  \
            } else {                                        \
                _K_ulKernFlags &= ~LW_KERN_FLAG_BUG_REBOOT; \
            }                                               \
        } while (0)
#define LW_KERN_BUG_REBOOT_EN_GET() (_K_ulKernFlags & LW_KERN_FLAG_BUG_REBOOT)

/*********************************************************************************************************
  内核 SMP 系统快速调度
*********************************************************************************************************/

#define LW_KERN_SMP_FSCHED_EN_SET(en)                       \
        do {                                                \
            if (en) {                                       \
                _K_ulKernFlags |= LW_KERN_FLAG_SMP_FSCHED;  \
            } else {                                        \
                _K_ulKernFlags &= ~LW_KERN_FLAG_SMP_FSCHED; \
            }                                               \
        } while (0)
#define LW_KERN_SMP_FSCHED_EN_GET() (_K_ulKernFlags & LW_KERN_FLAG_SMP_FSCHED)

/*********************************************************************************************************
  内核 SMT 系统调度优化
*********************************************************************************************************/
#if (LW_CFG_SMP_EN > 0) && (LW_CFG_CPU_ARCH_SMT > 0)

#define LW_KERN_SMT_BSCHED_EN_SET(en)                       \
        do {                                                \
            if (en) {                                       \
                _K_ulKernFlags |= LW_KERN_FLAG_SMT_BSCHED;  \
            } else {                                        \
                _K_ulKernFlags &= ~LW_KERN_FLAG_SMT_BSCHED; \
            }                                               \
        } while (0)
#define LW_KERN_SMT_BSCHED_EN_GET() (_K_ulKernFlags & LW_KERN_FLAG_SMT_BSCHED)

#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
/*********************************************************************************************************
  不支持 ITIMER_REAL, ITIMER_VIRTUAL, ITIMER_PROF
*********************************************************************************************************/

#define LW_KERN_NO_ITIMER_EN_SET(en)                        \
        do {                                                \
            if (en) {                                       \
                _K_ulKernFlags |= LW_KERN_FLAG_NO_ITIMER;   \
            } else {                                        \
                _K_ulKernFlags &= ~LW_KERN_FLAG_NO_ITIMER;  \
            }                                               \
        } while (0)
#define LW_KERN_NO_ITIMER_EN_GET()  (_K_ulKernFlags & LW_KERN_FLAG_NO_ITIMER)

/*********************************************************************************************************
  timespec 转换为 tick 超时方法 (SIMPLE 速度快精度会损失一个 tick)
*********************************************************************************************************/

#define LW_KERN_TMCVT_SIMPLE_EN_SET(en)                         \
        do {                                                    \
            if (en) {                                           \
                _K_ulKernFlags |= LW_KERN_FLAG_TMCVT_SIMPLE;    \
            } else {                                            \
                _K_ulKernFlags &= ~LW_KERN_FLAG_TMCVT_SIMPLE;   \
            }                                                   \
        } while (0)
#define LW_KERN_TMCVT_SIMPLE_EN_GET()  (_K_ulKernFlags & LW_KERN_FLAG_TMCVT_SIMPLE)

/*********************************************************************************************************
  系统状态
*********************************************************************************************************/

#define LW_SYS_STATUS_SET(status)   (_K_ucSysStatus = status)
#define LW_SYS_STATUS_GET()         (_K_ucSysStatus)

#define LW_SYS_STATUS_INIT          0
#define LW_SYS_STATUS_RUNNING       1

#define LW_SYS_STATUS_IS_INIT()     (_K_ucSysStatus == LW_SYS_STATUS_INIT)
#define LW_SYS_STATUS_IS_RUNNING()  (_K_ucSysStatus == LW_SYS_STATUS_RUNNING)

/*********************************************************************************************************
  中断向量表
*********************************************************************************************************/

#define LW_IVEC_GET_IDESC(vector)       \
        (&_K_idescTable[vector])

#define LW_IVEC_GET_FLAG(vector)        \
        (_K_idescTable[vector].IDESC_ulFlag)
        
#define LW_IVEC_SET_FLAG(vector, flag)  \
        (_K_idescTable[vector].IDESC_ulFlag = (flag))

/*********************************************************************************************************
  堆配置
*********************************************************************************************************/

#if LW_CFG_CDUMP_EN > 0
#define LW_KERNEL_HEAP_MIN_SIZE     (LW_CFG_KB_SIZE + LW_CFG_CDUMP_CALL_STACK_DEPTH)
#else
#define LW_KERNEL_HEAP_MIN_SIZE     LW_CFG_KB_SIZE
#endif

#define LW_SYSTEM_HEAP_MIN_SIZE     LW_CFG_KB_SIZE

/*********************************************************************************************************
  崩溃信息暂存配置
*********************************************************************************************************/

#if LW_CFG_CDUMP_EN > 0
#define LW_KERNEL_HEAP_START(a)     ((PVOID)a)
#define LW_KERNEL_HEAP_SIZE(s)      ((size_t)s - LW_CFG_CDUMP_BUF_SIZE)
#define LW_KERNEL_CDUMP_START(a, s) ((PVOID)((addr_t)a + LW_KERNEL_HEAP_SIZE(s)))
#define LW_KERNEL_CDUMP_SIZE(s)     LW_CFG_CDUMP_BUF_SIZE
#else
#define LW_KERNEL_HEAP_START(a)     ((PVOID)a)
#define LW_KERNEL_HEAP_SIZE(s)      ((size_t)s)
#endif

/*********************************************************************************************************
  调试等级定义
*********************************************************************************************************/

#define __LOGMESSAGE_LEVEL          0x1                                 /*  系统运行状态信息            */
#define __ERRORMESSAGE_LEVEL        0x2                                 /*  系统故障信息                */
#define __BUGMESSAGE_LEVEL          0x4                                 /*  操作系统 BUG 信息           */
#define __PRINTMESSAGE_LEVEL        0x8                                 /*  直接打印输出信息            */
#define __ALL_LEVEL                 0xf                                 /*  所有类型                    */

/*********************************************************************************************************
  CONST posix
*********************************************************************************************************/

#define PX_ROOT                     '/'
#define PX_STR_ROOT                 "/"
#define PX_DIVIDER                  PX_ROOT
#define PX_STR_DIVIDER              PX_STR_ROOT

#endif                                                                  /*  __K_CONST_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/

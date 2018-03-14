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
** 文   件   名: bspLib.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 7 月 31 日
**
** 描        述: 处理器需要为 SylixOS 提供的功能支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/x86/asm/hwcap.h"
#include "arch/x86/common/x86CpuId.h"
#include "arch/x86/acpi/x86AcpiLib.h"
#include "arch/x86/param/x86Param.h"
/*********************************************************************************************************
** 函数名称: bspInfoCpu
** 功能描述: BSP CPU 信息
** 输　入  : NONE
** 输　出  : CPU 信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK CPCHAR  bspInfoCpu (VOID)
{
    return  (X86_FEATURE_CPU_INFO);
}
/*********************************************************************************************************
** 函数名称: bspInfoCache
** 功能描述: BSP CACHE 信息
** 输　入  : NONE
** 输　出  : CACHE 信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK CPCHAR  bspInfoCache (VOID)
{
    return  (X86_FEATURE_CACHE_INFO);
}
/*********************************************************************************************************
** 函数名称: bspInfoHwcap
** 功能描述: BSP 硬件特性
** 输　入  : NONE
** 输　出  : 硬件特性 (如果支持硬浮点, 可以加入 HWCAP_FPU, HWCAP_MMX, HWCAP_SSE, HWCAP_SSE2, HWCAP_AVX)
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK ULONG  bspInfoHwcap (VOID)
{
    ULONG  ulHwcap = 0;

    if (X86_FEATURE_HAS_X87FPU) {
        ulHwcap |= HWCAP_FPU;
    }

    if (X86_FEATURE_HAS_MMX) {
        ulHwcap |= HWCAP_MMX;
    }

    if (X86_FEATURE_HAS_SSE) {
        ulHwcap |= HWCAP_SSE;
    }

    if (X86_FEATURE_HAS_SSE2) {
        ulHwcap |= HWCAP_SSE2;
    }

    if (X86_FEATURE_HAS_AVX) {
        ulHwcap |= HWCAP_AVX;
    }

    return  (ulHwcap);
}
/*********************************************************************************************************
  MMU 相关接口
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: bspMmuPgdMaxNum
** 功能描述: 获得 PGD 池的数量
** 输  入  : NONE
** 输  出  : PGD 池的数量 (1 个池可映射 4GB 空间, 推荐返回 1)
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK ULONG  bspMmuPgdMaxNum (VOID)
{
    return  (1);
}
/*********************************************************************************************************
** 函数名称: bspMmuPmdMaxNum
** 功能描述: 获得 PMD 池的数量
** 输  入  : NONE
** 输  出  : PMD 池的数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_CPU_WORD_LENGHT == 64

LW_WEAK ULONG  bspMmuPmdMaxNum (VOID)
{
    return  (1);
}
/*********************************************************************************************************
** 函数名称: bspMmuPtsMaxNum
** 功能描述: 获得 PTS 池的数量
** 输  入  : NONE
** 输  出  : PTS 池的数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK ULONG  bspMmuPtsMaxNum (VOID)
{
    /*
     * 16GB 内存: 页表池大约消耗 32MB  的堆内存
     * 32GB 内存: 页表池大约消耗 64MB  的堆内存
     * 64GB 内存: 页表池大约消耗 128MB 的堆内存
     */
    return  (32);                                                       /*  32GB                        */
}

#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 64*/
/*********************************************************************************************************
** 函数名称: bspMmuPgdMaxNum
** 功能描述: 获得 PTE 池的数量
** 输  入  : NONE
** 输  出  : PTE 池的数量 (映射 4GB 空间, 需要 1024 个 PTE 池)
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK ULONG  bspMmuPteMaxNum (VOID)
{
#if LW_CFG_CPU_WORD_LENGHT == 32
    return  (1024);
#else
    return  (bspMmuPtsMaxNum() << 9);
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 32*/
}
/*********************************************************************************************************
  电源相关接口
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: bspResetByKeyboard
** 功能描述: 通过键盘重新启动
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  bspResetByKeyboard (VOID)
{
    INT     i;
    UINT8   ucTemp;

    /*
     * Clear all keyboard buffers (output and command buffers)
     */
    do {
        ucTemp = in8(STATUS_8042);
        if (ucTemp & KBD_KDIB) {                                        /*  Empty keyboard data         */
            in8(DATA_8042);
        }
    } while (ucTemp & KBD_UDIB);                                        /*  Empty user data             */

    for (i = 0; i < 10; i++) {
        out8(KBD_RESET, STATUS_8042);                                   /*  Pulse CPU reset line        */
        bspDelayUs(50);
    }

    while (1) {
        X86_HLT();
    }
}
/*********************************************************************************************************
** 函数名称: bspReboot
** 功能描述: 系统重新启动
** 输　入  : iRebootType       重启类型
**           ulStartAddress    启动开始地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  bspReboot (INT  iRebootType, addr_t  ulStartAddress)
{
    (VOID)iRebootType;
    (VOID)ulStartAddress;

    if (x86AcpiAvailable()) {                                           /*  ACPI 可用                   */
        if (iRebootType == LW_REBOOT_SHUTDOWN) {                        /*  关机的重启类型              */
            AcpiEnterSleepStatePrep(ACPI_STATE_S5);
            AcpiEnterSleepState(ACPI_STATE_S5);                         /*  连电源在内的所有设备全部关闭*/

        } else {
            AcpiReset();                                                /*  ACPI 重启                   */
        }
        bspResetByKeyboard();                                           /*  上面失败了，使用键盘重启    */

    } else {
        bspResetByKeyboard();                                           /*  ACPI 不可用，使用键盘重启   */
    }
}
/*********************************************************************************************************
** 函数名称: bspDelay720Ns
** 功能描述: 延迟 720 纳秒
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  bspDelay720Ns (VOID)
{
    CHAR  cTemp __unused;

    cTemp = in8(UNUSED_ISA_IO_ADDRESS);                                 /*  消耗 720 NS                 */
}
/*********************************************************************************************************
  TICK 相关接口
*********************************************************************************************************/
/*********************************************************************************************************
  配置
*********************************************************************************************************/
#define TICK_IN_THREAD                  0
/*********************************************************************************************************
  定时器驱动函数声明
*********************************************************************************************************/
INT   bsp8254TickInit(ULONG  *pulVector);
VOID  bsp8254TickHook(INT64  i64Tick);
VOID  bsp8254TickHighResolution(struct timespec  *ptv);

INT   bspHpetTickInit(ACPI_TABLE_HPET  *pAcpiHpetPhy, ULONG  *pulVector);
VOID  bspHpetTickHook(INT64  i64Tick);
VOID  bspHpetTickHighResolution(struct timespec  *ptv);
/*********************************************************************************************************
  定时器驱动回调函数变量定义
*********************************************************************************************************/
static VOID  (*_G_pfuncTickHook)(INT64  i64Tick) = LW_NULL;
static VOID  (*_G_pfuncTickHighResolution)(struct timespec  *ptv) = LW_NULL;
/*********************************************************************************************************
  操作系统时钟服务线程句柄
*********************************************************************************************************/
#if TICK_IN_THREAD > 0
static LW_HANDLE            _G_htKernelTicks;
#endif                                                                  /*  TICK_IN_THREAD > 0          */
/*********************************************************************************************************
** 函数名称: __tickThread
** 功能描述: 初始化 tick 服务线程
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if TICK_IN_THREAD > 0

static VOID  __tickThread (VOID)
{
    for (;;) {
        API_ThreadSuspend(_G_htKernelTicks);
        API_KernelTicks();                                              /*  内核 TICKS 通知             */
        API_TimerHTicks();                                              /*  高速 TIMER TICKS 通知       */
    }
}

#endif                                                                  /*  TICK_IN_THREAD > 0          */
/*********************************************************************************************************
** 函数名称: __tickTimerIsr
** 功能描述: tick 定时器中断服务程序
** 输  入  : NONE
** 输  出  : 中断服务返回
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static irqreturn_t  __tickTimerIsr (VOID)
{
    API_KernelTicksContext();                                           /*  保存被时钟中断的线程控制块  */

#if TICK_IN_THREAD > 0
    API_ThreadResume(_G_htKernelTicks);
#else
    API_KernelTicks();                                                  /*  内核 TICKS 通知             */
    API_TimerHTicks();                                                  /*  高速 TIMER TICKS 通知       */
#endif                                                                  /*  TICK_IN_THREAD > 0          */

    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** 函数名称: bspTickInit
** 功能描述: 初始化 tick 时钟
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  bspTickInit (VOID)
{
#if TICK_IN_THREAD > 0
    LW_CLASS_THREADATTR  threadattr;
#endif
    ULONG                ulVector;
    INT                  iHpetInitOk = PX_ERROR;

#if TICK_IN_THREAD > 0
    API_ThreadAttrBuild(&threadattr, (16 * LW_CFG_KB_SIZE),
                        LW_PRIO_T_TICK,
                        LW_OPTION_THREAD_STK_CHK |
                        LW_OPTION_THREAD_UNSELECT |
                        LW_OPTION_OBJECT_GLOBAL |
                        LW_OPTION_THREAD_SAFE, LW_NULL);

    _G_htKernelTicks = API_ThreadCreate("t_tick", (PTHREAD_START_ROUTINE)__tickThread,
                                        &threadattr, LW_NULL);
#endif                                                                  /*  TICK_IN_THREAD > 0          */

    if (_G_pAcpiHpet && archKernelParamGet()->X86_bUseHpet) {           /*  有 HPET 并且指定使用 HPET   */
        _G_pfuncTickHook           = bspHpetTickHook;
        _G_pfuncTickHighResolution = bspHpetTickHighResolution;

        iHpetInitOk = bspHpetTickInit(_G_pAcpiHpet, &ulVector);         /*  初始化 HPET                 */
    }

    if (iHpetInitOk != ERROR_NONE) {                                    /*  没有 HPET 或且初始化失败    */
        _G_pfuncTickHook           = bsp8254TickHook;                   /*  使用 8254 定时器            */
        _G_pfuncTickHighResolution = bsp8254TickHighResolution;

        bsp8254TickInit(&ulVector);                                     /*  初始化 8254 定时器          */
    }

    API_InterVectorConnect(ulVector,
                           (PINT_SVR_ROUTINE)__tickTimerIsr,
                           LW_NULL,
                           "tick_timer");                               /*  连接定时器中断服务程序      */

    API_InterVectorEnable(ulVector);                                    /*  使能定时器中断              */
}
/*********************************************************************************************************
** 函数名称: bspTickHook
** 功能描述: 每个操作系统时钟节拍，系统将调用这个函数
** 输  入  : i64Tick      系统当前时钟
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  bspTickHook (INT64  i64Tick)
{
    _G_pfuncTickHook(i64Tick);
}
/*********************************************************************************************************
** 函数名称: bspTickHighResolution
** 功能描述: 修正从最近一次 tick 到当前的精确时间.
** 输　入  : ptv       需要修正的时间
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  bspTickHighResolution (struct timespec  *ptv)
{
    _G_pfuncTickHighResolution(ptv);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

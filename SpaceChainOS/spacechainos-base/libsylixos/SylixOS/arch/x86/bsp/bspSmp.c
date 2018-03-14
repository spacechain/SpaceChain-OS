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
** 文   件   名: bspSmp.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 7 月 31 日
**
** 描        述: 处理器需要为 SylixOS 提供的功能支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/x86/common/x86Topology.h"
#include "arch/x86/mpcore/x86MpCore.h"
/*********************************************************************************************************
  多核相关接口
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define AP_BOOT_ENTRY_ADDR          0x7000                              /*  AP 进入点地址(实模式地址)   */
#if LW_CFG_CPU_WORD_LENGHT == 32
#define AP_BOOT_STACK_SIZE          (32 * LW_CFG_KB_SIZE)               /*  AP 初始化过程堆栈大小       */
#else
#define AP_BOOT_STACK_SIZE          (64 * LW_CFG_KB_SIZE)               /*  AP 初始化过程堆栈大小       */
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 32*/
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
#if LW_CFG_CPU_WORD_LENGHT == 32
static const UINT8  _G_ucX86ApEntryCode[] = {                           /*  x86 AP 进入点代码           */
    0xfa, 0x31, 0xc0, 0x8e, 0xd8, 0x8e, 0xc0, 0x8e, 0xd0, 0x0f, 0x01,
    0x16, 0x50, 0x70, 0x0f, 0x20, 0xc0, 0x66, 0x83, 0xc8, 0x01, 0x0f,
    0x22, 0xc0, 0x66, 0xea, 0x20, 0x70, 0x00, 0x00, 0x08, 0x00, 0x66,
    0xb8, 0x10, 0x00, 0x8e, 0xd8, 0x8e, 0xc0, 0x8e, 0xd0, 0x66, 0xb8,
    0x00, 0x00, 0x8e, 0xe0, 0x8e, 0xe8, 0x8b, 0x25, 0xfc, 0x6f, 0x00,
    0x00, 0xff, 0x15, 0xf8, 0x6f, 0x00, 0x00, 0x66, 0xb8, 0x00, 0x8a,
    0x66, 0x89, 0xc2, 0x66, 0xef, 0x66, 0xb8, 0xe0, 0x8a, 0x66, 0xef,
    0xf4, 0xeb, 0xfd, 0x17, 0x00, 0x60, 0x70, 0x00, 0x00, 0x66, 0x2e,
    0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x9a,
    0xcf, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xcf, 0x00, 0x00
};

#else
static const UINT8  _G_ucX86ApEntryCode[] = {                           /*  x86-64 AP 进入点代码        */
    0xfa, 0x31, 0xc0, 0x8e, 0xd8, 0x8e, 0xc0, 0x8e, 0xd0, 0x0f, 0x01,
    0x16, 0x90, 0x70, 0x0f, 0x20, 0xc0, 0x66, 0x83, 0xc8, 0x01, 0x0f,
    0x22, 0xc0, 0x66, 0xea, 0x20, 0x70, 0x00, 0x00, 0x08, 0x00, 0xfa,
    0x89, 0xc7, 0x89, 0xde, 0x0f, 0x01, 0x15, 0xc0, 0x70, 0x00, 0x00,
    0xa1, 0xe8, 0x6f, 0x00, 0x00, 0x0f, 0x22, 0xd8, 0x0f, 0x20, 0xe0,
    0x83, 0xc8, 0x20, 0x0f, 0x22, 0xe0, 0xb9, 0x80, 0x00, 0x00, 0xc0,
    0x0f, 0x32, 0x0d, 0x00, 0x01, 0x00, 0x00, 0x0f, 0x30, 0x0f, 0x20,
    0xc0, 0x0d, 0x00, 0x00, 0x00, 0x80, 0x0f, 0x22, 0xc0, 0xea, 0x5d,
    0x70, 0x00, 0x00, 0x08, 0x00, 0x66, 0xb8, 0x10, 0x00, 0x8e, 0xd8,
    0x8e, 0xc0, 0x8e, 0xd0, 0x8e, 0xe8, 0x8e, 0xe0, 0x48, 0x8b, 0x24,
    0x25, 0xf8, 0x6f, 0x00, 0x00, 0xff, 0x14, 0x25, 0xf0, 0x6f, 0x00,
    0x00, 0x66, 0xb8, 0x00, 0x8a, 0x66, 0x89, 0xc2, 0x66, 0xef, 0x66,
    0xb8, 0xe0, 0x8a, 0x66, 0xef, 0xf4, 0xeb, 0xfd, 0x0f, 0x1f, 0x40,
    0x00, 0x17, 0x00, 0xa0, 0x70, 0x00, 0x00, 0x66, 0x2e, 0x0f, 0x1f,
    0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x9a, 0xcf, 0x00,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xcf, 0x00, 0x0f, 0x1f, 0x84,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x27, 0x00, 0xd0, 0x70, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x66, 0x0f, 0x1f, 0x44, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
    0x00, 0x9a, 0xaf, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x93, 0xaf,
    0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0xfa, 0xaf, 0x00, 0xff, 0xff,
    0x00, 0x00, 0x00, 0xf3, 0xaf, 0x00, 0x00
};
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 32*/

static SPINLOCKTYPE _G_slX86ApLock = 0;                                 /*  AP 启动自旋锁               */
/*********************************************************************************************************
** 函数名称: x86CpuIpiStartSecondary
** 功能描述: 启动 AP Processor
** 输　入  : ucLocalApicID     Local APIC ID
**           uiEntryAddr       实模式进入点地址
**           uiTimes           重发次数
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  x86CpuIpiStartSecondary (UINT8  ucLocalApicId, addr_t  ulEntryAddr, UINT32  uiTimes)
{
    LOAPIC_IPI_INFO  ipiInfo;
    INT              i;

    /*
     * BSP sends AP an INIT IPI
     */

    /*
     * INIT Level De-assert not supported in the Pentium 4, Xeon processors
     */

    /*
     * Do an INIT IPI: assert RESET
     */
    ipiInfo.ICR.ICR_uiApicId     = ucLocalApicId;
    ipiInfo.ICR.ICR_uiShortHand  = 0;
    ipiInfo.ICR.ICR_uiTrigger    = 0;
    ipiInfo.ICR.ICR_uiLevel      = 1;
    ipiInfo.ICR.ICR_uiDestMode   = 0;
    ipiInfo.ICR.ICR_uiDeliMode   = 5;
    ipiInfo.ICR.ICR_uiVectorNo   = 0;

    if (x86LocalApicSendIpiRaw(ipiInfo.IPI_uiAll) != ERROR_NONE) {
        return  (PX_ERROR);
    }

    /*
     * BSP delay 10msec
     */
    for (i = 0; i < 15000; i++) {                                       /*  15000*720 ~= 10.8 msec      */
        bspDelay720Ns();                                                /*  720ns                       */
    }

    while (uiTimes-- > 0) {
        /*
         * BSP sends AP a STARTUP IPI again
         */
        ipiInfo.ICR.ICR_uiApicId     = ucLocalApicId;
        ipiInfo.ICR.ICR_uiShortHand  = 0;
        ipiInfo.ICR.ICR_uiTrigger    = 0;
        ipiInfo.ICR.ICR_uiLevel      = 0;
        ipiInfo.ICR.ICR_uiDestMode   = 0;
        ipiInfo.ICR.ICR_uiDeliMode   = 6;
        ipiInfo.ICR.ICR_uiVectorNo   = (UINT8)(ulEntryAddr >> 12);

        if (x86LocalApicSendIpiRaw(ipiInfo.IPI_uiAll) != ERROR_NONE) {
            return  (PX_ERROR);
        }

        /*
         * BSP delays 200usec again
         */
        for (i = 0; i < 300; i++) {                                     /*  300*720 ~= 216usec          */
            bspDelay720Ns();                                            /*  720ns                       */
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: bspCpuUp
** 功能描述: 启动一个 CPU
** 输  入  : ulCPUId      目标 CPU
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID   bspCpuUp (ULONG  ulCPUId)
{
    ULONG      *pulEntryAddr;
    PLW_STACK   pstkStack;
    UINT32      uiOldResetVector;                                       /*  Warm reset vector           */
    UINT8       uiOldShutdownCode;                                      /*  CMOS shutdown code          */

    pstkStack = sys_malloc(AP_BOOT_STACK_SIZE);
    if (pstkStack == LW_NULL) {
        _PrintFormat("Can't start CPU#%d, system low memory!\r\n", ulCPUId);
        return;
    }
    pulEntryAddr = (ULONG *)AP_BOOT_ENTRY_ADDR;

    x86SpinLock(&_G_slX86ApLock);

    lib_memcpy(pulEntryAddr, _G_ucX86ApEntryCode, (size_t)sizeof(_G_ucX86ApEntryCode));

    *(pulEntryAddr - 1) = ROUND_DOWN(((addr_t)pstkStack) + AP_BOOT_STACK_SIZE - 1, 16);
    *(pulEntryAddr - 2) = (addr_t)bspSecondaryInit;
#if LW_CFG_CPU_WORD_LENGHT == 64
    *(pulEntryAddr - 3) = (addr_t)x64BootstrapPageTbl();                /*  x86-64 AP 引导程序页表      */
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 64*/

    /*
     * Set the AP entry point address in WARM_REST_VECTOR
     */
    uiOldResetVector = *(volatile UINT32 *)WARM_RESET_VECTOR;

    /*
     * Selects shutdown status register
     */
    out8(BIOS_SHUTDOWN_STATUS, RTC_INDEX);
    /*
     * Get BIOS shutdown code
     */
    uiOldShutdownCode = in8(RTC_DATA);

    *(volatile UINT16 *)(WARM_RESET_VECTOR + 0) = 0;
    *(volatile UINT16 *)(WARM_RESET_VECTOR + 2) = (((addr_t)pulEntryAddr) >> 4);

    /*
     * Initialze the BIOS shutdown code to be 0xa
     */
    /*
     * Selects shutdown status register
     */
    out8(BIOS_SHUTDOWN_STATUS, RTC_INDEX);
    /*
     * Set BIOS shutdown code to 0xa
     */
    out8(0xa, RTC_DATA);

    X86_WBINVD();

    x86CpuIpiStartSecondary(X86_LOGICID_TO_APICID(ulCPUId), (addr_t)pulEntryAddr, 2);

    /*
     * Restore the WARM_REST_VECTOR and the BIOS shutdown code
     */
    *(volatile UINT32 *)WARM_RESET_VECTOR = uiOldResetVector;
    out8(BIOS_SHUTDOWN_STATUS, RTC_INDEX);                              /*  Shutdown status reg         */
    out8(uiOldShutdownCode,    RTC_DATA);                               /*  Set BIOS shutdown code      */

    X86_WBINVD();
}
/*********************************************************************************************************
** 函数名称: bspCpuUpDone
** 功能描述: 一个 CPU 启动完成
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  bspCpuUpDone (VOID)
{
    x86SpinUnlock(&_G_slX86ApLock);
}
/*********************************************************************************************************
** 函数名称: bspSecondaryCpusUp
** 功能描述: 启动所有的 Secondary CPU
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  bspSecondaryCpusUp (VOID)
{
    UINT8  ucLocalApicId;
    ULONG  ulCPUId;

    if (LW_NCPUS <= 1) {
        return;
    }

    if (X86_HAS_SMT) {                                                  /*  存在超线程                  */
        LW_CPU_FOREACH_EXCEPT(ulCPUId, 0) {
            ucLocalApicId = X86_LOGICID_TO_APICID(ulCPUId);
            if (X86_APICID_PRESEND(ucLocalApicId)) {
                if (X86_APICID_IS_HT(ucLocalApicId)) {                  /*  先启动所有物理核            */
                    continue;
                }
                API_CpuUp(ulCPUId);
            }
        }

        LW_CPU_FOREACH_EXCEPT(ulCPUId, 0) {
            ucLocalApicId = X86_LOGICID_TO_APICID(ulCPUId);
            if (X86_APICID_PRESEND(ucLocalApicId)) {
                if (X86_APICID_IS_HT(ucLocalApicId)) {                  /*  再启动所有逻辑核            */
                    API_CpuUp(ulCPUId);
                }
            }
        }

    } else {                                                            /*  不存在超线程                */
        LW_CPU_FOREACH_EXCEPT(ulCPUId, 0) {
            ucLocalApicId = X86_LOGICID_TO_APICID(ulCPUId);
            if (X86_APICID_PRESEND(ucLocalApicId)) {
                API_CpuUp(ulCPUId);
            }
        }
    }
}
/*********************************************************************************************************
** 函数名称: bspCpuLogic2Physical
** 功能描述: 获得逻辑 CPU 的物理 ID
** 输  入  : ulCPUId      目标 CPU
** 输  出  : 物理 ID
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_CPU_ARCH_SMT > 0

LW_WEAK ULONG  bspCpuLogic2Physical (ULONG  ulCPUId)
{
    return  (X86_LOGICID_TO_PHYID(ulCPUId));
}

#endif                                                                  /*  LW_CFG_CPU_ARCH_SMT > 0     */
/*********************************************************************************************************
** 函数名称: bspCpuDown
** 功能描述: 停止一个 CPU
** 输  入  : ulCPUId      目标 CPU
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_SMP_CPU_DOWN_EN > 0

LW_WEAK VOID   bspCpuDown (ULONG  ulCPUId)
{
    KN_INT_DISABLE();

    X86_WBINVD();

    while (1) {
        X86_HLT();
    }
}

#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
** 函数名称: bspCpuIpiVectorInstall
** 功能描述: 安装 IPI 向量
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  bspCpuIpiVectorInstall (VOID)
{
    ULONG  ulCPUId;

    if (LW_NCPUS <= 1) {
        return;
    }

    ulCPUId = LW_CPU_GET_CUR_ID();
    API_InterVectorIpi(ulCPUId, X86_IPI_VECTOR(ulCPUId));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

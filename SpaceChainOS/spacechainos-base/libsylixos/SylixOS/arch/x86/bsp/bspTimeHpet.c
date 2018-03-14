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
** 文   件   名: bspTimeHpet.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 7 月 12 日
**
** 描        述: HPET 定时器函数库.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/x86/common/x86Idt.h"
#include "arch/x86/acpi/x86AcpiLib.h"
/*********************************************************************************************************
  HPET 地址
*********************************************************************************************************/
static addr_t               _G_ulHpetBase;
/*********************************************************************************************************
  精确时间换算参数
*********************************************************************************************************/
static UINT32               _G_uiFullCnt;
static UINT32               _G_uiComparatorCur;
static UINT64               _G_ui64NSecPerCnt7;                         /*  提高 7bit 精度              */
/*********************************************************************************************************
  HPET 寄存器定义
*********************************************************************************************************/
#define HPET_BASE                       _G_ulHpetBase

#define HPET_ID_LO                      (HPET_BASE + 0)
#define HPET_ID_HI                      (HPET_ID_LO + 4)

#define HPET_CONFIG_LO                  (HPET_BASE + 0x10)
#define HPET_CONFIG_HI                  (HPET_CONFIG_LO + 4)

#define HPET_STATUS_LO                  (HPET_BASE + 0x20)
#define HPET_STATUS_HI                  (HPET_STATUS_LO + 4)

#define HPET_COUNTER_LO                 (HPET_BASE + 0xf0)
#define HPET_COUNTER_HI                 (HPET_COUNTER_LO + 4)

#define HPET_TIMER_CONFIG_LO(t)         (HPET_BASE + 0x100 + (t) * 0x20)
#define HPET_TIMER_CONFIG_HI(t)         (HPET_TIMER_CONFIG_LO(t) + 4)

#define HPET_TIMER_COMPARATOR_LO(t)     (HPET_BASE + 0x108 + (t) * 0x20)
#define HPET_TIMER_COMPARATOR_HI(t)     (HPET_TIMER_COMPARATOR_LO(t) + 4)

#define HPET_TIMER_FSB_INTR_LO(t)       (HPET_BASE + 0x110 + (t) * 0x20)
#define HPET_TIMER_FSB_INTR_HI(t)       (HPET_TIMER_FSB_INTR_LO(t) + 4)
/*********************************************************************************************************
** 函数名称: __bspAcpiHpetMap
** 功能描述: 映射 ACPI HPET 表
** 输  入  : ulAcpiPhyAddr    ACPI HPET 表物理地址
**           ulAcpiSize       ACPI HPET 表长度
** 输  出  : ACPI HPET 表虚拟地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  *__bspAcpiHpetMap (addr_t  ulAcpiPhyAddr, size_t  ulAcpiSize)
{
    addr_t  ulPhyBase = ROUND_DOWN(ulAcpiPhyAddr, LW_CFG_VMM_PAGE_SIZE);
    addr_t  ulOffset  = ulAcpiPhyAddr - ulPhyBase;
    addr_t  ulVirBase;

    ulAcpiSize += ulOffset;
    ulAcpiSize  = ROUND_UP(ulAcpiSize, LW_CFG_VMM_PAGE_SIZE);

    ulVirBase = (addr_t)API_VmmIoRemapNocache((PVOID)ulPhyBase, ulAcpiSize);
    if (ulVirBase) {
        return  (VOID *)(ulVirBase + ulOffset);
    } else {
        return  (VOID *)(LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: bspHpetTickInit
** 功能描述: 初始化 tick 时钟
** 输  入  : pHpet         ACPI HPET 表
**           pulVector     中断向量号
** 输  出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  bspHpetTickInit (ACPI_TABLE_HPET  *pAcpiHpetPhy, ULONG  *pulVector)
{
    ACPI_TABLE_HPET  *pAcpiHpet;
    UINT64            ui64FsPerCnt;

    /*
     * 映射 ACPI HPET 表
     */
    pAcpiHpet = __bspAcpiHpetMap((addr_t)pAcpiHpetPhy, LW_CFG_VMM_PAGE_SIZE);
    if (!pAcpiHpet) {
        return  (PX_ERROR);
    }

    /*
     * 映射 HPET 寄存器
     */
    _G_ulHpetBase = (addr_t)API_VmmIoRemapNocache((PVOID)pAcpiHpet->Address.Address,
                                                  LW_CFG_VMM_PAGE_SIZE);

    /*
     * 解除映射 ACPI HPET 表
     */
    API_VmmIoUnmap((PVOID)(((addr_t)pAcpiHpet) & LW_CFG_VMM_PAGE_MASK));

    if (!_G_ulHpetBase) {                                               /*  映射 HPET 寄存器失败        */
        return  (PX_ERROR);
    }

    /*
     * 由于 HPET 不一定支持 64 位模式(如 QEMU), 所以固定使用 32 位模式
     */
    write32(0, HPET_CONFIG_LO);                                         /*  停止定时器                  */

    ui64FsPerCnt = read32(HPET_ID_HI);

    _G_uiFullCnt       = (1000000000000000ULL / ui64FsPerCnt) / LW_TICK_HZ;
    _G_ui64NSecPerCnt7 = (ui64FsPerCnt << 7) / 1000000;

    write32(0, HPET_COUNTER_LO);                                        /*  复位计数器                  */
    write32(0, HPET_COUNTER_HI);

    write32((1 << 8) |                                                  /*  32 位模式                   */
            (1 << 6) |                                                  /*  设置值                      */
            (1 << 3) |                                                  /*  Preiodic 模式               */
            (1 << 2) |                                                  /*  中断使能                    */
            (0 << 1),                                                   /*  边沿触发模式                */
            HPET_TIMER_CONFIG_LO(0));                                   /*  配置 timer0                 */
    write32(0, HPET_TIMER_CONFIG_HI(0));

    write32(_G_uiFullCnt, HPET_TIMER_COMPARATOR_LO(0));                 /*  设置 timer0 comparator      */
    write32(0,            HPET_TIMER_COMPARATOR_HI(0));

    write32((1 << 0) |                                                  /*  开始计数                    */
            (1 << 1),                                                   /*  legacy replacement mode     */
            HPET_CONFIG_LO);                                            /*  启动定时器                  */

    if (bspIntModeGet() == X86_INT_MODE_SYMMETRIC_IO) {                 /*  IOAPIC mapping              */
        *pulVector = LW_IRQ_2;                                          /*  IRQ2                        */

    } else {                                                            /*  PIC mapping                 */
        *pulVector = X86_IRQ_TIMER;                                     /*  IRQ0                        */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: bspHpetTickHook
** 功能描述: 每个操作系统时钟节拍，系统将调用这个函数
** 输  入  : i64Tick      系统当前时钟
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  bspHpetTickHook (INT64  i64Tick)
{
    (VOID)i64Tick;

    _G_uiComparatorCur = read32(HPET_TIMER_COMPARATOR_LO(0)) - _G_uiFullCnt;
}
/*********************************************************************************************************
** 函数名称: bspHpetTickHighResolution
** 功能描述: 修正从最近一次 tick 到当前的精确时间.
** 输　入  : ptv       需要修正的时间
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  bspHpetTickHighResolution (struct timespec  *ptv)
{
    UINT32  uiCntDiff = read32(HPET_COUNTER_LO) - _G_uiComparatorCur;

    ptv->tv_nsec += (_G_ui64NSecPerCnt7 * uiCntDiff) >> 7;
    if (ptv->tv_nsec >= 1000000000) {
        ptv->tv_nsec -= 1000000000;
        ptv->tv_sec++;
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

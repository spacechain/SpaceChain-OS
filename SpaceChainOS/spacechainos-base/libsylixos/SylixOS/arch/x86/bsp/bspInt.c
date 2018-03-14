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
** 文   件   名: bspInt.c
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
#include "arch/x86/common/x86Topology.h"
#include "driver/int/i8259a.h"
/*********************************************************************************************************
  中断相关

  x86 Int Vector:
  +-----------------+
  |  0              |
  |                 |
  |  x86 exception  |
  |                 |
  |  31             |           SylixOS Int Vector:
  +-----------------+ --------> +-----------------+
  |  32             |           |  0              |
  |                 |           |                 |
  |   x86 IO IRQ    |  INT MAP  |     IO IRQ      | ------> 16
  |                 |           |                 | PCI x 8 Intx
  |                 |           |                 | ------> 23
  +-----------------+           +-----------------+
  |                 |           |                 |
  |  Local APIC IRQ |  INT MAP  |  Local APIC IRQ |
  |                 |           |                 |
  +-----------------+           +-----------------+
  |                 |           |                 |
  |   x86 SMP IPI   |  INT MAP  |     SMP IPI     |
  |                 |           |                 |
  +-----------------+           +-----------------+
  |                 |           |                 |
  | x86 MSI & other |  INT MAP  |   MSI & other   |
  |                 |           |                 |
  +-----------------+ --------> +-----------------+

*********************************************************************************************************/
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
/*********************************************************************************************************
  向量使能与禁能锁
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#define VECTOR_OP_LOCK()            LW_SPIN_LOCK_IGNIRQ(&_K_slVectorTable)
#define VECTOR_OP_UNLOCK()          LW_SPIN_UNLOCK_IGNIRQ(&_K_slVectorTable)
#else
#define VECTOR_OP_LOCK()
#define VECTOR_OP_UNLOCK()
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
static I8259A_CTL   _G_i8259aData = {                                   /*  i8259A 平台数据             */
    .iobase_master  = PIC1_BASE_ADR,
    .iobase_slave   = PIC2_BASE_ADR,
    .trigger        = 0,
    .manual_eoi     = 0,
    .vector_base    = X86_IRQ_BASE,
};

UINT                _G_uiX86IntMode = X86_INT_MODE_PIC;                 /*  中断模式                    */

UINT                _G_uiX86IoVectorNr;                                 /*  IO 中断数目                 */

UINT                _G_uiX86LocalApicVectorBase;                        /*  Local APIC 中断开始         */
UINT                _G_uiX86LocalApicVectorNr;                          /*  Local APIC 中断数目         */

UINT                _G_uiX86IpiVectorBase;                              /*  IPI 中断开始                */
UINT                _G_uiX86IpiVectorNr;                                /*  IPI 中断数目                */

UINT                _G_uiX86MsiVectorBase;                              /*  MSI 中断开始                */
UINT                _G_uiX86MsiVectorNr;                                /*  MSI 中断数目                */
/*********************************************************************************************************
** 函数名称: __x86Is8259Vector
** 功能描述: 判断一个中断是否为 8259 中断
** 输  入  : ulVector     中断向量
** 输　出  : LW_TRUE OR LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  __x86Is8259Vector (ULONG  ulVector)
{
    if (_G_uiX86IntMode != X86_INT_MODE_SYMMETRIC_IO) {
        if (ulVector < N_PIC_IRQS) {
            return  (LW_TRUE);
        }
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: __x86IsIoApicVector
** 功能描述: 判断一个中断是否为 IOAPIC 中断
** 输  入  : ulVector     中断向量
** 输　出  : LW_TRUE OR LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static  BOOL  __x86IsIoApicVector (ULONG  ulVector)
{
    if (_G_uiX86IntMode == X86_INT_MODE_SYMMETRIC_IO) {
        if (ulVector < _G_uiX86IoVectorNr) {
            return  (LW_TRUE);
        }
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: __x86IsIpiVector
** 功能描述: 判断一个中断是否为 Local APIC 中断
** 输  入  : ulVector     中断向量
** 输　出  : LW_TRUE OR LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  __x86IsLocalApicVector (ULONG  ulVector)
{
    if (_G_uiX86IntMode != X86_INT_MODE_PIC) {
        if ((ulVector >= _G_uiX86LocalApicVectorBase) &&
            (ulVector < (_G_uiX86LocalApicVectorBase + _G_uiX86LocalApicVectorNr))) {
            return  (LW_TRUE);
        }
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: __x86IsIpiVector
** 功能描述: 判断一个中断是否为 IPI 中断
** 输  入  : ulVector     中断向量
** 输　出  : LW_TRUE OR LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  __x86IsIpiVector (ULONG  ulVector)
{
    if (_G_uiX86IntMode == X86_INT_MODE_SYMMETRIC_IO) {
        if ((ulVector >= _G_uiX86IpiVectorBase) &&
            (ulVector < (_G_uiX86IpiVectorBase + _G_uiX86IpiVectorNr))) {
            return  (LW_TRUE);
        }
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: __x86IsMsiVector
** 功能描述: 判断一个中断是否为 MSI 中断
** 输  入  : ulVector     中断向量
** 输　出  : LW_TRUE OR LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  __x86IsMsiVector (ULONG  ulVector)
{
    if (_G_uiX86IntMode != X86_INT_MODE_PIC) {
        if ((ulVector >= _G_uiX86MsiVectorBase) &&
            (ulVector < (_G_uiX86MsiVectorBase + _G_uiX86MsiVectorNr))) {
            return  (LW_TRUE);
        }
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: bspIntInit
** 功能描述: 中断系统初始化
** 输  入  : NONE
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  bspIntInit (VOID)
{
    /*
     * 如果某中断为链式中断，请加入形如:
     * API_InterVectorSetFlag(LW_IRQ_4, LW_IRQ_FLAG_QUEUE);
     * 的代码.
     *
     * 如果某中断可用作初始化随机化种子，请加入形如:
     * API_InterVectorSetFlag(LW_IRQ_0, LW_IRQ_FLAG_SAMPLE_RAND);
     * 的代码.
     */

    _G_uiX86IntMode = X86_INT_MODE_PIC;                                 /*  复位后默认 PIC 模式         */

    i8259aInit(&_G_i8259aData);                                         /*  初始化 8259A                */
    i8259aIrqEnable(&_G_i8259aData, LW_IRQ_2);                          /*  使能 IRQ2(Slave PIC)        */

    if (LW_NCPUS > 1) {                                                 /*  多核一定会有 APIC           */
        _G_uiX86IntMode = X86_INT_MODE_SYMMETRIC_IO;                    /*  为了支持多核                */
                                                                        /*  用 SYMMETRIC_IO 模式        */
    } else if (X86_FEATURE_HAS_APIC) {                                  /*  单核但也有 APIC             */
        /*
         * MP 配置表读出的 PCI 设备中断可能是 16 ~ 23, 即必须使用 IOAPIC,
         * 这时必须用 SYMMETRIC_IO 模式, 不能用虚拟线模式
         */
        _G_uiX86IntMode = X86_INT_MODE_SYMMETRIC_IO;                    /*  用 SYMMETRIC_IO 模式        */
    }

    if (_G_uiX86IntMode != X86_INT_MODE_PIC) {                          /*  非 PIC 模式                 */

        x86LocalApicInit(&_G_uiX86LocalApicVectorNr);                   /*  初始化 Local APIC           */

        if (_G_uiX86IntMode == X86_INT_MODE_SYMMETRIC_IO) {             /*  SYMMETRIC_IO 模式           */
            x86IoApicInitAll(&_G_uiX86IoVectorNr);                      /*  初始化所有的 IOAPIC         */

        } else {                                                        /*  虚拟线模式                  */
            /*
             * 注意: 虚拟线模式只支 Over PIC, 不支持 Over IOAPIC
             */
            _G_uiX86IoVectorNr = N_PIC_IRQS;
        }

        _G_uiX86LocalApicVectorBase = _G_uiX86IoVectorNr;               /*  Local APIC 中断向量开始     */

        _G_uiX86IpiVectorBase = _G_uiX86IoVectorNr + \
                                _G_uiX86LocalApicVectorNr;              /*  IPI 中断向量开始            */
        if (_G_uiX86IntMode == X86_INT_MODE_SYMMETRIC_IO) {             /*  SYMMETRIC_IO 模式           */
            _G_uiX86IpiVectorNr = LW_NCPUS;                             /*  IPI 中断数目就是 LW_NCPUS   */

        } else {                                                        /*  虚拟线模式                  */
            _G_uiX86IpiVectorNr = 0;                                    /*  单核，无 IPI                */
        }

        _G_uiX86MsiVectorBase = _G_uiX86IpiVectorBase + \
                                _G_uiX86IpiVectorNr;                    /*  MSI 中断向量开始            */
        _G_uiX86MsiVectorNr   = min(X86_IRQ_NUM - _G_uiX86IoVectorNr - \
                                    _G_uiX86LocalApicVectorNr - _G_uiX86IpiVectorNr,
                                    16);                                /*  MSI 中断数目                */

        if (_G_uiX86IntMode == X86_INT_MODE_SYMMETRIC_IO) {             /*  SYMMETRIC_IO 模式           */
            /*
             * 8 个 PCI 中断设置为链式中断
             */
            API_InterVectorSetFlag(LW_IRQ_16, LW_IRQ_FLAG_QUEUE);
            API_InterVectorSetFlag(LW_IRQ_17, LW_IRQ_FLAG_QUEUE);
            API_InterVectorSetFlag(LW_IRQ_18, LW_IRQ_FLAG_QUEUE);
            API_InterVectorSetFlag(LW_IRQ_19, LW_IRQ_FLAG_QUEUE);
            API_InterVectorSetFlag(LW_IRQ_20, LW_IRQ_FLAG_QUEUE);
            API_InterVectorSetFlag(LW_IRQ_21, LW_IRQ_FLAG_QUEUE);
            API_InterVectorSetFlag(LW_IRQ_22, LW_IRQ_FLAG_QUEUE);
            API_InterVectorSetFlag(LW_IRQ_23, LW_IRQ_FLAG_QUEUE);
        }
    }
}
/*********************************************************************************************************
** 函数名称: bspIntModeGet
** 功能描述: 获得中断模式
** 输  入  : NONE
** 输  出  : 中断模式
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK UINT  bspIntModeGet (VOID)
{
    return  (_G_uiX86IntMode);
}
/*********************************************************************************************************
** 函数名称: bspIntHandle
** 功能描述: 中断处理函数
** 输  入  : ulVector     中断向量
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  bspIntHandle (ULONG  ulVector)
{
    archIntHandle(ulVector, LW_FALSE);                                  /*  不允许中断嵌套(MSI 中断无法 */
                                                                        /*  屏蔽)                       */
    VECTOR_OP_LOCK();
    if (__x86Is8259Vector(ulVector)) {
        if (_G_i8259aData.manual_eoi) {                                 /*  需要手动清中断              */
            i8259aIrqEoi(&_G_i8259aData, (UINT)ulVector);               /*  清 8259 中断                */
        }

    } else if (__x86IsIoApicVector(ulVector)) {
        x86IoApicIrqEoi(ulVector);                                      /*  清 IOAPIC 中断              */
    }

    if (X86_FEATURE_HAS_APIC) {                                         /*  有 APIC                     */
        x86LocalApicEoi();                                              /*  清 Local APIC 中断          */
    }
    VECTOR_OP_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: bspIntVectorEnable
** 功能描述: 使能指定的中断向量
** 输  入  : ulVector     中断向量
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  bspIntVectorEnable (ULONG  ulVector)
{
    if (__x86Is8259Vector(ulVector)) {
        i8259aIrqEnable(&_G_i8259aData, ulVector);

    } else if (__x86IsIoApicVector(ulVector)) {
        x86IoApicIrqEnable(ulVector);

    } else if (__x86IsLocalApicVector(ulVector)) {
        x86LocalApicIrqEnable(ulVector - _G_uiX86LocalApicVectorBase);
    }

    /*
     * IPI MSI 不能开关
     */
}
/*********************************************************************************************************
** 函数名称: bspIntVectorDisable
** 功能描述: 禁能指定的中断向量
** 输  入  : ulVector     中断向量
** 输  出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK VOID  bspIntVectorDisable (ULONG  ulVector)
{
    if (__x86Is8259Vector(ulVector)) {
        i8259aIrqDisable(&_G_i8259aData, ulVector);

    } else if (__x86IsIoApicVector(ulVector)) {
        x86IoApicIrqDisable(ulVector);

    } else if (__x86IsLocalApicVector(ulVector)) {
        x86LocalApicIrqDisable(ulVector - _G_uiX86LocalApicVectorBase);
    }

    /*
     * IPI MSI 不能开关
     */
}
/*********************************************************************************************************
** 函数名称: bspIntVectorIsEnable
** 功能描述: 检查指定的中断向量是否使能
** 输  入  : ulVector     中断向量
** 输  出  : LW_FALSE 或 LW_TRUE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK BOOL  bspIntVectorIsEnable (ULONG  ulVector)
{
    if (__x86Is8259Vector(ulVector)) {
        return  (i8259aIrqIsEnable(&_G_i8259aData, ulVector));

    } else if (__x86IsIoApicVector(ulVector)) {
        return  (x86IoApicIrqIsEnable(ulVector));

    } else if (__x86IsLocalApicVector(ulVector)) {
        return  (x86LocalApicIrqIsEnable(ulVector - _G_uiX86LocalApicVectorBase));

    } else if (__x86IsIpiVector(ulVector)) {
        /*
         * IPI MSI 不能开关
         */
        return  (LW_TRUE);

    } else if (__x86IsMsiVector(ulVector)) {
        return  (LW_TRUE);
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: bspIntVectorSetPriority
** 功能描述: 设置指定的中断向量的优先级
** 输  入  : ulVector     中断向量
**           uiPrio       优先级
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_INTER_PRIO > 0

LW_WEAK ULONG   bspIntVectorSetPriority (ULONG  ulVector, UINT  uiPrio)
{
    /*
     * 不能设置优先级
     */
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: bspIntVectorGetPriority
** 功能描述: 获取指定的中断向量的优先级
** 输  入  : ulVector     中断向量
**           puiPrio      优先级
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK ULONG   bspIntVectorGetPriority (ULONG  ulVector, UINT  *puiPrio)
{
    *puiPrio = ulVector;                                                /*  Vector 越大，优先级越高     */
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_INTER_PRIO > 0       */
/*********************************************************************************************************
** 函数名称: bspIntVectorSetTarget
** 功能描述: 设置指定的中断向量的目标 CPU
** 输　入  : ulVector      中断向量
**           stSize        CPU 掩码集内存大小
**           pcpuset       CPU 掩码
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_INTER_TARGET > 0

LW_WEAK ULONG   bspIntVectorSetTarget (ULONG  ulVector, size_t  stSize, const PLW_CLASS_CPUSET  pcpuset)
{
    if (__x86IsIoApicVector(ulVector)) {
        ULONG   i;
        ULONG   ulNumChk;

        ulNumChk = ((ULONG)stSize << 3);
        ulNumChk = (ulNumChk > LW_NCPUS) ? LW_NCPUS : ulNumChk;

        for (i = 0; i < ulNumChk; i++) {
            if (LW_CPU_ISSET(i, pcpuset)) {
                x86IoApicIrqSetTarget(ulVector, X86_LOGICID_TO_APICID(i));
                break;                                                  /*  只能设置一个 Target CPU     */
            }
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: bspIntVectorGetTarget
** 功能描述: 获取指定的中断向量的目标 CPU
** 输　入  : ulVector      中断向量
**           stSize        CPU 掩码集内存大小
**           pcpuset       CPU 掩码
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_WEAK ULONG   bspIntVectorGetTarget (ULONG  ulVector, size_t  stSize, PLW_CLASS_CPUSET  pcpuset)
{
    LW_CPU_ZERO(pcpuset);

    if (__x86IsIoApicVector(ulVector)) {
        UINT8  ucTargetLocalApicId;

        x86IoApicIrqGetTarget(ulVector, &ucTargetLocalApicId);
        LW_CPU_SET(X86_APICID_TO_LOGICID(ucTargetLocalApicId), pcpuset);

    } else if (__x86IsLocalApicVector(ulVector)) {
        LW_CPU_SET(LW_CPU_GET_CUR_ID(), pcpuset);

    } else if (__x86IsIpiVector(ulVector)) {
        LW_CPU_SET((ulVector - _G_uiX86IpiVectorBase), pcpuset);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_INTER_TARGET > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/

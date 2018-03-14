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
** 文   件   名: ppcExcE500.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 05 月 04 日
**
** 描        述: PowerPC E500 体系构架异常处理.
*********************************************************************************************************/

#ifndef __ARCH_PPCEXCE500_H
#define __ARCH_PPCEXCE500_H

/*********************************************************************************************************
  中断向量定义
*********************************************************************************************************/

#define PPC_E500_ISA_VECTOR_BASE            0
#define PPC_E500_ARCH_VECTOR_BASE           16
#define PPC_E500_MPIC_VECTOR_BASE           32

#define PPC_E500_ARCH_DEC_VECTOR            (PPC_E500_ARCH_VECTOR_BASE + 0)
#define PPC_E500_ARCH_WATCHDOG_VECTOR       (PPC_E500_ARCH_VECTOR_BASE + 1)
#define PPC_E500_ARCH_TIMER_VECTOR          (PPC_E500_ARCH_VECTOR_BASE + 2)

/*********************************************************************************************************
  以下几个异常处理函数为弱函数, BSP 可以根据需要实现它们
*********************************************************************************************************/

VOID  archE500PerfMonitorExceptionHandle(addr_t  ulRetAddr);
VOID  archE500DoorbellExceptionHandle(addr_t  ulRetAddr);
VOID  archE500DoorbellCriticalExceptionHandle(addr_t  ulRetAddr);
VOID  archE500DecrementerInterruptHandle(addr_t  ulRetAddr);
VOID  archE500TimerInterruptHandle(addr_t  ulRetAddr);
VOID  archE500WatchdogInterruptHandle(addr_t  ulRetAddr);

/*********************************************************************************************************
  函数声明
*********************************************************************************************************/

VOID  archE500DecrementerInterruptAck(VOID);
VOID  archE500DecrementerInterruptEnable(VOID);
VOID  archE500DecrementerInterruptDisable(VOID);
BOOL  archE500DecrementerInterruptIsEnable(VOID);
VOID  archE500DecrementerAutoReloadEnable(VOID);
VOID  archE500DecrementerAutoReloadDisable(VOID);

VOID  archE500TimerInterruptAck(VOID);
VOID  archE500TimerInterruptEnable(VOID);
VOID  archE500TimerInterruptDisable(VOID);
BOOL  archE500TimerInterruptIsEnable(VOID);

VOID  archE500WatchdogInterruptAck(VOID);
VOID  archE500WatchdogInterruptEnable(VOID);
VOID  archE500WatchdogInterruptDisable(VOID);
BOOL  archE500WatchdogInterruptIsEnable(VOID);

VOID  archE500VectorInit(CPCHAR  pcMachineName, addr_t  ulVectorBase);

#endif                                                                  /*  __ARCH_PPCEXCE500_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: i8259a.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2015 年 10 月 28 日
**
** 描        述: Intel 8259A 中断控制器支持.
*********************************************************************************************************/

#ifndef __I8259A_H
#define __I8259A_H

#include "SylixOS.h"

/*********************************************************************************************************
  i8259a control
*********************************************************************************************************/

typedef struct i8259a_ctl I8259A_CTL;

struct i8259a_ctl {
    UINT    cached_irq_mask;

    /*
     *  user MUST set following members before calling this module api.
     */
    addr_t  iobase_master;                                              /* 8259-1 I/O base address      */
                                                                        /* eg. IO-BASE + 0x20           */

    addr_t  iobase_slave;                                               /* 8259-2 I/O base address      */
                                                                        /* eg. IO-BASE + 0xa0           */

    INT     trigger;                                                    /* 0 : edge trigger             */
                                                                        /* 1 : level trigger            */

    INT     manual_eoi;                                                 /* 1 : manual EOI               */
                                                                        /* 0 : auto EOI                 */
    UINT    vector_base;                                                /* base vector                  */
};

/*********************************************************************************************************
  i8259a functions
*********************************************************************************************************/

VOID  i8259aIrqDisable(I8259A_CTL *pctl, UINT  irq);
VOID  i8259aIrqEnable(I8259A_CTL *pctl, UINT  irq);
BOOL  i8259aIrqIsEnable(I8259A_CTL *pctl, UINT  irq);
BOOL  i8259aIrqIsPending(I8259A_CTL *pctl, UINT  irq);
VOID  i8259aIrqEoi(I8259A_CTL *pctl, UINT  irq);
INT   i8259aIrq(I8259A_CTL *pctl);
VOID  i8259aInit(I8259A_CTL *pctl);

#endif                                                                  /*  __I8259A_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

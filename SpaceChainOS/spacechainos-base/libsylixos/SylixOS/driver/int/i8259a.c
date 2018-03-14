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
** 文   件   名: i8259a.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2015 年 10 月 28 日
**
** 描        述: Intel 8259A 中断控制器支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "../SylixOS/config/driver/drv_cfg.h"
/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if LW_CFG_DRV_INT_I8259A > 0
#include "i8259a.h"
/*********************************************************************************************************
  i8259a PIC registers
*********************************************************************************************************/
#define PIC_MASTER_CMD      (pctl->iobase_master + 0x00)
#define PIC_MASTER_IMR      (pctl->iobase_master + 0x01)
#define PIC_MASTER_ISR      PIC_MASTER_CMD
#define PIC_MASTER_POLL     PIC_MASTER_ISR
#define PIC_MASTER_OCW3     PIC_MASTER_ISR
#define PIC_SLAVE_CMD       (pctl->iobase_slave + 0x00)
#define PIC_SLAVE_IMR       (pctl->iobase_slave + 0x01)
/*********************************************************************************************************
  i8259a PIC related value
*********************************************************************************************************/
#define PIC_CASCADE_IR      2
#define MASTER_ICW4_DEFAULT 0x01
#define SLAVE_ICW4_DEFAULT  0x01
#define PIC_ICW4_AEOI       2
#define NON_SPEC_EOI        0x20
/*********************************************************************************************************
  This contains the irq mask for both 8259a irq controllers
*********************************************************************************************************/
#define cached_master_mask  (pctl->cached_irq_mask)
#define cached_slave_mask   (pctl->cached_irq_mask >> 8)
/*********************************************************************************************************
** 函数名称: i8259aIrqDisable
** 功能描述: 关闭 8259a 指定通道中断
** 输　入  : pctl           8259a 控制块
**           irq            8259a 内部中断号
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  i8259aIrqDisable (I8259A_CTL *pctl, UINT  irq)
{
    UINT  mask;

    mask = 1 << irq;
    pctl->cached_irq_mask |= mask;

    if (irq < 8) {
        out8(cached_master_mask, PIC_MASTER_IMR);
    } else {
        out8(cached_slave_mask, PIC_SLAVE_IMR);
    }
}
/*********************************************************************************************************
** 函数名称: i8259aIrqEnable
** 功能描述: 打开 8259a 指定通道中断
** 输　入  : pctl           8259a 控制块
**           irq            8259a 内部中断号
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  i8259aIrqEnable (I8259A_CTL *pctl, UINT  irq)
{
    UINT  mask;

    mask = ~(1 << irq);
    pctl->cached_irq_mask &= mask;

    if (irq < 8) {
        out8(cached_master_mask, PIC_MASTER_IMR);
    } else {
        out8(cached_slave_mask, PIC_SLAVE_IMR);
    }
}
/*********************************************************************************************************
** 函数名称: i8259aIrqIsEnable
** 功能描述: 检测 8259a 指定通道中断是否打开
** 输　入  : pctl           8259a 控制块
**           irq            8259a 内部中断号
** 输　出  : 中断是否打开
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL  i8259aIrqIsEnable (I8259A_CTL *pctl, UINT  irq)
{
    UINT  mask;

    mask = 1 << irq;

    if (pctl->cached_irq_mask & mask) {
        return  (LW_FALSE);
    } else {
        return  (LW_TRUE);
    }
}
/*********************************************************************************************************
** 函数名称: i8259aIrqIsPending
** 功能描述: 检测 8259a 指定通道中断是否等待
** 输　入  : pctl           8259a 控制块
**           irq            8259a 内部中断号
** 输　出  : 中断是 pending
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL  i8259aIrqIsPending (I8259A_CTL *pctl, UINT  irq)
{
    UINT  mask;
    UINT8 cmd;

    mask = 1 << irq;

    if (irq < 8) {
        out8(0x0A, PIC_MASTER_CMD);
        cmd = in8(PIC_MASTER_CMD) & mask;                               /*  读取 IRR 寄存器             */

        out8(0x0B, PIC_MASTER_CMD);
        cmd |= in8(PIC_MASTER_CMD) & mask;                              /*  读取 ISR 寄存器             */
    
	} else {
        cmd = in8(PIC_SLAVE_CMD) & (mask >> 8);
    }

    if (cmd) {
        return  (LW_TRUE);
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** 函数名称: i8259aIrqEoi
** 功能描述: 通知 8259a 中断处理结束
** 输　入  : pctl           8259a 控制块
**           irq            8259a 内部中断号
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  i8259aIrqEoi (I8259A_CTL *pctl, UINT  irq)
{
    if (irq >= 8) {
        out8(NON_SPEC_EOI, PIC_SLAVE_CMD);
    }
    out8(NON_SPEC_EOI, PIC_MASTER_CMD);
}
/*********************************************************************************************************
** 函数名称: i8259aIrq
** 功能描述: 检测 8259a 哪一路中断请求中断
** 输　入  : pctl           8259a 控制块
** 输　出  : 8259a 中断通道 (-1 表示中断无效)
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  i8259aIrq (I8259A_CTL *pctl)
{
    INT    irq;

    /*
     * Perform an interrupt acknowledge cycle on controller 1.
     */
    out8(0x0C, PIC_MASTER_CMD);                                         /*  prepare for poll            */
    irq = in8(PIC_MASTER_CMD) & 7;
    if (irq == PIC_CASCADE_IR) {
        /*
         * Interrupt is cascaded so perform interrupt
         * acknowledge on controller 2.
         */
        out8(0x0C, PIC_SLAVE_CMD);                                      /*  prepare for poll            */
        irq = (in8(PIC_SLAVE_CMD) & 7) + 8;
    }

    if (unlikely(irq == 7)) {
        /*
         * This may be a spurious interrupt.
         *
         * Read the interrupt status register (ISR). If the most
         * significant bit is not set then there is no valid
         * interrupt.
         */
        out8(0x0B, PIC_MASTER_ISR);                                     /*  ISR register                */
        if (~in8(PIC_MASTER_ISR) & 0x80) {
            irq = -1;
        }
    }

    return  (irq);
}
/*********************************************************************************************************
** 函数名称: i8259aInit
** 功能描述: 初始化 8259a 中断控制器
** 输　入  : pctl           8259a 控制块
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  i8259aInit (I8259A_CTL *pctl)
{
    UINT8   cmd;

    if (pctl->trigger) {
        cmd = 0x19;
    } else {
        cmd = 0x11;
    }

    pctl->cached_irq_mask = 0xffff;

    out8(0xff, PIC_MASTER_IMR);                                         /*  mask all of 8259A-1 master  */
    out8(0xff, PIC_SLAVE_IMR);                                          /*  mask all of 8259A-2 slave   */

    out8(cmd, PIC_MASTER_CMD);                                          /*  ICW1: select 8259A-1 init   */
    out8(pctl->vector_base, PIC_MASTER_IMR);                            /*  ICW2:master IR0 mapped to 0 */
    out8(1u << PIC_CASCADE_IR, PIC_MASTER_IMR);                         /*  (master) has a slave on IR2 */

    if (pctl->manual_eoi) {
        out8(MASTER_ICW4_DEFAULT, PIC_MASTER_IMR);                      /*  master does Manual EOI      */
    } else {
        out8(MASTER_ICW4_DEFAULT | PIC_ICW4_AEOI, PIC_MASTER_IMR);      /*  master does Auto EOI        */
    }

    out8(cmd, PIC_SLAVE_CMD);                                           /*  ICW1: select 8259A-2 init   */
    out8(pctl->vector_base + 8, PIC_SLAVE_IMR);                         /*  ICW2: slave IR0 mapped to 8 */
    out8(PIC_CASCADE_IR, PIC_SLAVE_IMR);                                /*  slave on master's IR2       */

    if (pctl->manual_eoi) {
        out8(SLAVE_ICW4_DEFAULT, PIC_SLAVE_IMR);                        /*  master does Manual EOI      */
    } else {
        out8(SLAVE_ICW4_DEFAULT | PIC_ICW4_AEOI, PIC_SLAVE_IMR);        /*  slave's support for AEOI in */
    }                                                                   /*  flat mode is to be          */
                                                                        /*  investigated                */
    bspDelayUs(100);                                                    /*  wait for 8259A to initialize*/

    out8(cached_master_mask, PIC_MASTER_IMR);                           /*  restore master IRQ mask     */
    out8(cached_slave_mask, PIC_SLAVE_IMR);                             /*  restore slave IRQ mask      */
}

#endif                                                                  /*  LW_CFG_DRV_INT_I8259A > 0   */
/*********************************************************************************************************
  END
*********************************************************************************************************/

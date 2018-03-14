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
** 文   件   名: x86Cr.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2017 年 06 月 02 日
**
** 描        述: x86 体系构架处理器 CR 寄存器读写.
*********************************************************************************************************/

#ifndef __ARCH_X86CR_H
#define __ARCH_X86CR_H

typedef ARCH_REG_T  X86_CR_REG;

extern X86_CR_REG   x86Cr0Get(VOID);
extern VOID         x86Cr0Set(X86_CR_REG  uiValue);

extern X86_CR_REG   x86Cr1Get(VOID);
extern VOID         x86Cr1Set(X86_CR_REG  uiValue);

extern X86_CR_REG   x86Cr2Get(VOID);
extern VOID         x86Cr2Set(X86_CR_REG  uiValue);

extern X86_CR_REG   x86Cr3Get(VOID);
extern VOID         x86Cr3Set(X86_CR_REG  uiValue);

extern X86_CR_REG   x86Cr4Get(VOID);
extern VOID         x86Cr4Set(X86_CR_REG  uiValue);

#endif                                                                  /*  __ARCH_X86CR_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/

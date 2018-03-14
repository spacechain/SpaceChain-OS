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
** 文   件   名: lib_stdarg.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 03 月 13 日
**
** 描        述: stdarg.h.

** BUG:
2009.06.30  使用 GCC 强烈推荐使用编译器内建 stdarg.h
2010.08.26  当使用 GCC 编译时，这里直接使用编译器内建函数。
*********************************************************************************************************/

#ifndef __LIB_STDARG_H
#define __LIB_STDARG_H

/*********************************************************************************************************
  注意:
        通常情况下, 不确定参数个数的参数传递都使用堆栈, RISC 与 CISC 相同. 确定第一个参数的位置是通过最后
        一个确定的参数在参数堆栈中的位置.
        例如: printf(fmt, ...); 这个函数通过 fmt 参数在堆栈中的位置, 然后通过某种算法偏移量来确定后面各个
        参数的位置. 但是, 前提条件是: fmt 不能是寄存器传参, 也就是说 fmt 必须使用堆栈传参, 这个算法才能生
        效. 当然, 在绝大多数编译器上, 对待变参数数量传参都是用堆栈, 类似的方法都可以处理.
        
        但是 GCC 在 -O3 优化等级时使用了 -finline-function 优化选项. 此选项对使用频率较高的函数进行了内联
        处理, 很多参数变成了寄存器直接传参, 对于可变个数参数传递, 作为基准参数(上例中的 fmt)如果是寄存器
        类型, 后面的参数将会有灾难性的后果. 2009.06.30 GCC -O3 测试 ioctl 时, 产生了此类问题. ioctl 的第
        二个参数为寄存器变量, 导致第三个参数直接错误. 
        
        所以强烈推荐使用 GCC 编译 SylixOS 时, 使用 GGC 内建的 stdarg.h 头文件.
*********************************************************************************************************/

/*********************************************************************************************************
  变长参数堆栈处理 (__EXCLIB_STDARG 表示引用外部 stdarg.h)
*********************************************************************************************************/
#if defined(__EXCLIB_STDARG)
#if defined(__TMS320C6X__)
#include <arch/c6x/arch_stdarg.h>
#else
#include <stdarg.h>                                                     /*  使用外部 stdarg 库          */
#endif                                                                  /*  __TMS320C6X__               */
/*********************************************************************************************************
  GCC 这里直接使用编译器内建函数, 否则 -O3 (-finline-function) 优化时可能出现错误
*********************************************************************************************************/
#elif defined(__GNUC__)
#ifndef va_start
#define va_start(v,l)        __builtin_va_start(v,l)
#endif                                                                  /*  va_start                    */

#ifndef va_end
#define va_end(v)            __builtin_va_end(v)
#endif                                                                  /*  va_end                      */

#ifndef va_arg
#define va_arg(v,l)          __builtin_va_arg(v,l)
#endif                                                                  /*  va_arg                      */

#ifndef _VA_LIST_DEFINED
#ifndef _VA_LIST
#ifndef _VA_LIST_T_H
#ifndef __va_list__

typedef __builtin_va_list    va_list;

#endif                                                                  /*  __va_list__                 */
#endif                                                                  /*  _VA_LIST_T_H                */
#endif                                                                  /*  _VA_LIST                    */
#endif                                                                  /*  _VA_LIST_DEFINED            */

#ifndef _VA_LIST
#define _VA_LIST
#endif                                                                  /*  _VA_LIST                    */
#ifndef _VA_LIST_DEFINED
#define _VA_LIST_DEFINED
#endif                                                                  /*  _VA_LIST_DEFINED            */
#ifndef _VA_LIST_T_H
#define _VA_LIST_T_H
#endif                                                                  /*  _VA_LIST_T_H                */
#ifndef __va_list__
#define __va_list__
#endif                                                                  /*  __va_list__                 */

#if !defined(__STRICT_ANSI__) || __STDC_VERSION__ + 0 >= 199900L || defined(__GXX_EXPERIMENTAL_CXX0X__)
#ifndef va_copy
#define va_copy(d,s)	     __builtin_va_copy(d,s)
#endif                                                                  /*  va_copy                     */
#endif                                                                  /*  __STRICT_ANSI__ ...         */

#ifndef __va_copy
#define __va_copy(d,s)	     __builtin_va_copy(d,s)
#endif                                                                  /*  __va_copy                   */
/*********************************************************************************************************
  当以下运算异常时, 请选择外部 stdarg 库
*********************************************************************************************************/
#else
#include "../SylixOS/config/cpu/cpu_cfg.h"
#include "../SylixOS/include/arch/arch_inc.h"

typedef char  *va_list;

#define __STACK_SIZEOF(n)    ((sizeof(n) + sizeof(LW_STACK) - 1) & ~(sizeof(LW_STACK) - 1))

#if LW_CFG_ARG_STACK_GROWTH == 0                                        /*  连续参数地址递增            */
#define va_start(ap, v)      (ap = (va_list)&v + __STACK_SIZEOF(v))
#define va_arg(ap, t)        (*(t *)((ap += __STACK_SIZEOF(t)) - __STACK_SIZEOF(t)))
#define va_end(ap)           (ap = (va_list)0)
#else                                                                   /*  连续参数地址递减            */
#define va_start(ap, v)      (ap = (va_list)&v - __STACK_SIZEOF(v))
#define va_arg(ap, t)        (*(t *)((ap -= __STACK_SIZEOF(t)) + __STACK_SIZEOF(t)))
#define va_end(ap)           (ap = (va_list)0)
#endif                                                                  /*  LW_CFG_ARG_STACK_GROWTH     */

#endif                                                                  /*  __EXLIB_STDARG || __GNUC__  */

#endif                                                                  /*  __LIB_STDARG_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: ppcBacktrace.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 12 月 15 日
**
** 描        述: PowerPC 体系构架堆栈回溯 (来源于 glibc).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  Only GCC support now.
*********************************************************************************************************/
#ifdef   __GNUC__
#include "ppcBacktrace.h"
/*********************************************************************************************************
   This is the stack layout we see with every stack frame.
   Note that every routine is required by the ABI to lay out the stack
   like this.

            +----------------+        +-----------------+
    %r1  -> | %r1 last frame | -----> | %r1 last frame  | ---> ... --> NULL
            |                |        |                 |
            | (unused)       |        | return address  |
            +----------------+        +-----------------+
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: backtrace
** 功能描述: 获得当前任务调用栈
** 输　入  : array     获取数组
**           size      数组大小
** 输　出  : 获取的数目
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  backtrace (void  **array, int  size)
{
    struct layout  *current;
    int             count;

    /* Force gcc to spill LR.  */
    asm volatile ("" : "=l"(current));

    /* Get the address on top-of-stack.  */
    asm volatile ("lwz %0 , 0(1)" : "=r"(current));
    current = BOUNDED_1(current);

    for (count = 0;
         current != NULL && count < size;
         current = BOUNDED_1(current->next), count++) {
        array[count] = current->return_address;
    }

    /* It's possible the second-last stack frame can't return
       (that is, it's __libc_start_main), in which case
       the CRT startup code will have set its LR to 'NULL'.  */
    if (count > 0 && array[count - 1] == NULL) {
        count--;
    }

    return  (count);
}

#endif                                                                  /*  __GNUC__                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/

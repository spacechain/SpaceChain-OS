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
** 文   件   名: armBacktrace.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 09 日
**
** 描        述: ARM 体系构架堆栈回溯.
*********************************************************************************************************/

#ifndef __ARCH_ARMBACKTRACE_H
#define __ARCH_ARMBACKTRACE_H

#include "sys/cdefs.h"

/*********************************************************************************************************
  Do not compiling with -fbounded-pointers!
*********************************************************************************************************/

#define BOUNDS_VIOLATED
#define CHECK_BOUNDS_LOW(ARG)       (ARG)
#define CHECK_BOUNDS_HIGH(ARG)      (ARG)
#define CHECK_1(ARG)                (ARG)
#define CHECK_1_NULL_OK(ARG)        (ARG)
#define CHECK_N(ARG, N)             (ARG)
#define CHECK_N_NULL_OK(ARG, N)     (ARG)
#define CHECK_STRING(ARG)           (ARG)
#define CHECK_SIGSET(SET)           (SET)
#define CHECK_SIGSET_NULL_OK(SET)   (SET)
#define CHECK_IOCTL(ARG, CMD)       (ARG)
#define CHECK_FCNTL(ARG, CMD)       (ARG)
#define CHECK_N_PAGES(ARG, NBYTES)  (ARG)
#define BOUNDED_N(PTR, N)           (PTR)
#define BOUNDED_1(PTR) BOUNDED_N    (PTR, 1)

/*********************************************************************************************************
  ARM 栈帧结构
*********************************************************************************************************/

#define ADJUST_FRAME_POINTER(frame) (struct layout *)((char *)frame - 4)

struct layout {
    void *__unbounded next;
    void *__unbounded return_address;
};

/*********************************************************************************************************
  arm-sylixos-eabi-gcc eg.

  void func (void)
  {
      call_func();
  }

 00000000 <func>:
  000:   e92d4800    push    {fp, lr}
  004:   e28db004    add fp, sp, #4
  008:   e3a00001    mov r0, #1
  00c:   ebffffbb    bl  0 <call_func>
  010:   e24bd004    sub sp, fp, #4
  014:   e8bd4800    pop {fp, lr}
  018:   e12fff1e    bx  lr

  fp is r11 register.
  __builtin_frame_address(0) will return r11 register.
  "add fp, sp, #4" fp point to return address so, we will sub 4 to detemine frame address.

  void func (void)
  {
      *((int *)0x12345678) = 1;
  }

  00000100 <func>:
   100:   e52db004    push    {fp}
   104:   e28db000    add fp, sp, #0
   108:   e59f3010    ldr r3, [pc, #16]
   10c:   e3a02001    mov r2, #1
   110:   e5832000    str r2, [r3]
   114:   e28bd000    add sp, fp, #0
   118:   e49db004    pop {fp}
   11c:   e12fff1e    bx  lr

  lr do not push to stack, so this func can not backtrace. (in signal context)
*********************************************************************************************************/

#endif                                                                  /*  __ARCH_ARMBACKTRACE_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

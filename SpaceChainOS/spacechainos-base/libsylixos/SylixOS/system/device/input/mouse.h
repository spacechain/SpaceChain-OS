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
** 文   件   名: mouse.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 09 月 09 日
**
** 描        述: 标准鼠标/触摸屏驱动.
** 注        意: 这里仅是一个设备驱动的接口规范, 所以不包含在 SylixOS.h 头文件中.
                 鼠标和键盘的读取 read() 不能产生阻塞, 如果没有事件产生, 就立即返回读取失败.
                 对鼠标和键盘的事件阻塞必须通过 select() 完成.
*********************************************************************************************************/

#ifndef __MOUSE_H
#define __MOUSE_H

/*********************************************************************************************************
  注意: 鼠标设备的 read() 操作, 第三个参数与返回值为字节数, 不是 mouse_event_notify 的个数.
        ioctl() FIONREAD 与 FIONWRITE 结果的单位都是字节.
  例如:
        mouse_event_notify   mnotify;
        ssize_t              size;
        ssize_t              mnotify_num;
        ...
        size        = read(mon_fd, &mnotify, 1 * sizeof(keyboard_event_notify));
        mnotify_num = size / sizeof(mouse_event_notify);
*********************************************************************************************************/

#include "sys/types.h"

/*********************************************************************************************************
  mouse max wheel(s)
*********************************************************************************************************/

#define MOUSE_MAX_WHEEL             2

/*********************************************************************************************************
  mouse button stat
*********************************************************************************************************/

#define MOUSE_LEFT                  0x01
#define MOUSE_RIGHT                 0x02
#define MOUSE_MIDDLE                0x04

#define MOUSE_BUTTON4               0x08
#define MOUSE_BUTTON5               0x10
#define MOUSE_BUTTON6               0x20
#define MOUSE_BUTTON7               0x40
#define MOUSE_BUTTON8               0x80

/*********************************************************************************************************
  mouse coordinate type
*********************************************************************************************************/

#define MOUSE_CTYPE_REL             0                                   /*  relative coordinate         */
#define MOUSE_CTYPE_ABS             1                                   /*  absolutely coordinate       */

/*********************************************************************************************************
  mouse event 
  
  eg.
  
  mouse_event_notify   event;
  
  read(fd, (char *)&event, sizeof(mouse_event_notify));
*********************************************************************************************************/

typedef struct mouse_event_notify {
    int32_t             ctype;                                          /*  coordinate type             */

    int32_t             kstat;                                          /*  mouse button stat           */
    int32_t             wscroll[MOUSE_MAX_WHEEL];                       /*  wheel scroll                */
    
    int32_t             xmovement;                                      /*  the relative displacement   */
    int32_t             ymovement;
    
    /*
     *  if use absolutely coordinate (such as touch screen)
     *  if you use touch screen:
     *  (kstat & MOUSE_LEFT) != 0 (press)
     *  (kstat & MOUSE_LEFT) == 0 (release)
     */
#define xanalog         xmovement                                       /*  analog samples values       */
#define yanalog         ymovement
} mouse_event_notify;

typedef mouse_event_notify          touchscreen_event_notify;

#endif                                                                  /*  __MOUSE_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/

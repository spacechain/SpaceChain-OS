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
** 文   件   名: keyboard.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 09 月 09 日
**
** 描        述: 标准键盘驱动.
** 注        意: 这里仅是一个设备驱动的接口规范, 所以不包含在 SylixOS.h 头文件中.
                 鼠标和键盘的读取 read() 不能产生阻塞, 如果没有事件产生, 就立即返回读取失败.
                 对鼠标和键盘的事件阻塞必须通过 select() 完成.
*********************************************************************************************************/

#ifndef __KEYBOARD_H
#define __KEYBOARD_H

/*********************************************************************************************************
  注意: 键盘设备的 read() 操作, 第三个参数与返回值为字节数, 不是 keyboard_event_notify 的个数.
        ioctl() FIONREAD 与 FIONWRITE 结果的单位都是字节.
  例如:
        keyboard_event_notify   knotify[10];
        ssize_t                 size;
        ssize_t                 knotify_num;
        ...
        size        = read(key_fd, knotify, 10 * sizeof(keyboard_event_notify));
        knotify_num = size / sizeof(keyboard_event_notify);
*********************************************************************************************************/

#include "sys/types.h"
#include "sys/ioccom.h"

/*********************************************************************************************************
  keyboard max key event at same time
*********************************************************************************************************/

#define KE_MAX_KEY_ST               4

/*********************************************************************************************************
  keyboard event type
*********************************************************************************************************/

#define KE_PRESS                    1
#define KE_RELEASE                  2

/*********************************************************************************************************
  keyboard event led stat
*********************************************************************************************************/

#define KE_LED_NUMLOCK              0x01
#define KE_LED_CAPSLOCK             0x02
#define KE_LED_SCROLLLOCK           0x04

/*********************************************************************************************************
  keyboard event func-key stat
*********************************************************************************************************/

#define KE_FK_CTRL                  0x01
#define KE_FK_ALT                   0x02
#define KE_FK_SHIFT                 0x04

#define KE_FK_CTRLR                 0x10
#define KE_FK_ALTR                  0x20
#define KE_FK_SHIFTR                0x40

/*********************************************************************************************************
  keyboard event 
  
  eg.
  
  keyboard_event_notify   event;
  
  read(fd, (void *)&event, sizeof(keyboard_event_notify));
  
  event.keymsg[0] is SylixOS keyboard code
  event.keymsg[1] is standard keyboard scancode
*********************************************************************************************************/

typedef struct keyboard_event_notify {
    int32_t             nmsg;                                           /*  message num, usually one msg*/
    int32_t             type;                                           /*  press or release            */
    int32_t             ledstate;                                       /*  LED stat                    */
    int32_t             fkstat;                                         /*  func-key stat               */
    int32_t             keymsg[KE_MAX_KEY_ST * 2];                      /*  key code                    */
} keyboard_event_notify;

/*********************************************************************************************************
  key ioctl
*********************************************************************************************************/

#define KBD_CTL_SET_SIM_CONTSTRIKE  _IOW('k', 1, INT)                   /*  driver sim continuous strike*/
#define KBD_CTL_GET_SIM_CONTSTRIKE  _IOR('k', 1, INT)

/*********************************************************************************************************
  key type
*********************************************************************************************************/

#define KT_ASCII                    0
#define KT_FN                       1

/*********************************************************************************************************
  key type & value macro
*********************************************************************************************************/

#define K(t, v)                     (((t) << 8) | (v))
#define KTYP(x)                     ((x) >> 8)
#define KVAL(x)                     ((x) & 0xff)

/*********************************************************************************************************
  key code (ASCII keypad)
  
  'a' =  K(KT_ASCII, 'a')
  ...
  'z' =  K(KT_ASCII, 'z')
  
  '.' =  K(KT_ASCII, '.')
  '=' =  K(KT_ASCII, '=')
  ...
*********************************************************************************************************/

/*********************************************************************************************************
  key code (Numeric keypad)
*********************************************************************************************************/

#define K_NP0                       K(KT_ASCII, 230)
#define K_NP1                       K(KT_ASCII, 231)
#define K_NP2                       K(KT_ASCII, 232)
#define K_NP3                       K(KT_ASCII, 233)
#define K_NP4                       K(KT_ASCII, 234)
#define K_NP5                       K(KT_ASCII, 235)
#define K_NP6                       K(KT_ASCII, 236)
#define K_NP7                       K(KT_ASCII, 237)
#define K_NP8                       K(KT_ASCII, 238)
#define K_NP9                       K(KT_ASCII, 239)

#define K_NP_PERIOD                 K(KT_ASCII, 240)
#define K_NP_DIVIDE                 K(KT_ASCII, 241)
#define K_NP_MULTIPLY               K(KT_ASCII, 242)
#define K_NP_MINUS                  K(KT_ASCII, 243)
#define K_NP_PLUS                   K(KT_ASCII, 244)
#define K_NP_ENTER                  K(KT_ASCII, 245)
#define K_NP_EQUALS                 K(KT_ASCII, 246)

/*********************************************************************************************************
  key code
*********************************************************************************************************/

#define K_F1                        K(KT_FN, 0)
#define K_F2                        K(KT_FN, 1)
#define K_F3                        K(KT_FN, 2)
#define K_F4                        K(KT_FN, 3)
#define K_F5                        K(KT_FN, 4)
#define K_F6                        K(KT_FN, 5)
#define K_F7                        K(KT_FN, 6)
#define K_F8                        K(KT_FN, 7)
#define K_F9                        K(KT_FN, 8)
#define K_F10                       K(KT_FN, 9)
#define K_F11                       K(KT_FN, 10)
#define K_F12                       K(KT_FN, 11)
#define K_F13                       K(KT_FN, 12)
#define K_F14                       K(KT_FN, 13)
#define K_F15                       K(KT_FN, 14)
#define K_F16                       K(KT_FN, 15)
#define K_F17                       K(KT_FN, 16)
#define K_F18                       K(KT_FN, 17)
#define K_F19                       K(KT_FN, 18)
#define K_F20                       K(KT_FN, 19)

#define K_ESC                       K(KT_FN, 20)
#define K_TAB                       K(KT_FN, 21)
#define K_CAPSLOCK                  K(KT_FN, 22)
#define K_SHIFT                     K(KT_FN, 23)
#define K_CTRL                      K(KT_FN, 24)
#define K_ALT                       K(KT_FN, 25)
#define K_SHIFTR                    K(KT_FN, 26)
#define K_CTRLR                     K(KT_FN, 27)
#define K_ALTR                      K(KT_FN, 28)
#define K_ENTER                     K(KT_FN, 29)
#define K_BACKSPACE                 K(KT_FN, 30)

#define K_PRINTSCREEN               K(KT_FN, 31)
#define K_SCROLLLOCK                K(KT_FN, 32)
#define K_PAUSEBREAK                K(KT_FN, 33)

#define K_INSERT                    K(KT_FN, 34)
#define K_HOME                      K(KT_FN, 35)
#define K_DELETE                    K(KT_FN, 36)
#define K_END                       K(KT_FN, 37)
#define K_PAGEUP                    K(KT_FN, 38)
#define K_PAGEDOWN                  K(KT_FN, 39)

#define K_UP                        K(KT_FN, 40)
#define K_DOWN                      K(KT_FN, 41)
#define K_LEFT                      K(KT_FN, 42)
#define K_RIGHT                     K(KT_FN, 43)

#define K_NUMLOCK                   K(KT_FN, 44)

/*********************************************************************************************************
  microsoft key code
*********************************************************************************************************/

#define K_WIN                       K(KT_FN, 128)
#define K_WINR                      K(KT_FN, 129)
#define K_RCLICK                    K(KT_FN, 130)

#endif                                                                  /*  __KEYBOARD_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/

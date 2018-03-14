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
** 文   件   名: gpiofd.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 11 月 20 日
**
** 描        述: 用户态操作 GPIO 接口.
*********************************************************************************************************/

#ifndef __SYS_GPIOFD_H
#define __SYS_GPIOFD_H

#include "unistd.h"

/*********************************************************************************************************
 gpiofd ioctl() cmd
*********************************************************************************************************/

#define GPIO_CMD_SET_FLAGS          FIOSETOPTIONS
#define GPIO_CMD_GET_FLAGS          FIOGETOPTIONS

/*********************************************************************************************************
 gpiofd ioctl() arg3 and gpiofd() arg3 gpio_flags
*********************************************************************************************************/

#define GPIO_FLAG_DIR_OUT           0x0000
#define GPIO_FLAG_DIR_IN            0x0001

#define GPIO_FLAG_INIT_LOW          0x0000
#define GPIO_FLAG_INIT_HIGH         0x0002

#define GPIO_FLAG_IN                (GPIO_FLAG_DIR_IN)
#define GPIO_FLAG_OUT_INIT_LOW      (GPIO_FLAG_DIR_OUT | GPIO_FLAG_INIT_LOW)
#define GPIO_FLAG_OUT_INIT_HIGH     (GPIO_FLAG_DIR_OUT | GPIO_FLAG_INIT_HIGH)

#define GPIO_FLAG_OPEN_DRAIN        0x0004
#define GPIO_FLAG_OPEN_SOURCE       0x0008

#define GPIO_FLAG_PULL_DEFAULT      0x0000
#define GPIO_FLAG_PULL_UP           0x0010
#define GPIO_FLAG_PULL_DOWN         0x0020
#define GPIO_FLAG_PULL_DISABLE      0x0040

#define GPIO_FLAG_TRIG_FALL         0x0100
#define GPIO_FLAG_TRIG_RISE         0x0200
#define GPIO_FLAG_TRIG_LEVEL        0x0400

/*********************************************************************************************************
 gpiofd() arg2 flags
*********************************************************************************************************/

#define GFD_CLOEXEC                 O_CLOEXEC
#define GFD_NONBLOCK                O_NONBLOCK

/*********************************************************************************************************
 gpiofd api
*********************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

int gpiofd(unsigned int gpio, int flags, int gpio_flags);
int gpiofd_read(int fd, uint8_t *value);
int gpiofd_write(int fd, uint8_t  value);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __SYS_GPIOFD_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/

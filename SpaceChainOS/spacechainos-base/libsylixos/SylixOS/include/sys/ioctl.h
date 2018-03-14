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
** 文   件   名: ioctl.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 04 月 02 日
**
** 描        述: 兼容 C 库.
*********************************************************************************************************/

#ifndef __SYS_IOCTL_H
#define __SYS_IOCTL_H

#ifndef __SYLIXOS_H
#include "SylixOS.h"
#endif                                                                  /*  __SYLIXOS_H                 */

#include "ioccom.h"

/*********************************************************************************************************
  tty
*********************************************************************************************************/

/*********************************************************************************************************
  Pun for SunOS prior to 3.2.  SunOS 3.2 and later support TIOCGWINSZ
  and TIOCSWINSZ (yes, even 3.2-3.5, the fact that it wasn't documented
  notwithstanding).
*********************************************************************************************************/

struct ttysize {
    unsigned short    ts_lines;
    unsigned short    ts_cols;
    unsigned short    ts_xxx;
    unsigned short    ts_yyy;
};

/*********************************************************************************************************
  Window/terminal size structure.  This information is stored by the kernel
  in order to provide a consistent interface, but is not used by the kernel.
*********************************************************************************************************/

struct winsize {
    unsigned short    ws_row;                                           /* rows, in characters          */
    unsigned short    ws_col;                                           /* columns, in characters       */
    unsigned short    ws_xpixel;                                        /* horizontal size, pixels      */
    unsigned short    ws_ypixel;                                        /* vertical size, pixels        */
};

/*********************************************************************************************************
  
*********************************************************************************************************/

#define TIOCGWINSZ      _IOR('t', 104, struct winsize)                  /* get window size              */
#define TIOCSWINSZ      _IOW('t', 103, struct winsize)                  /* set window size              */

#endif                                                                  /*  __SYS_IOCTL_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/

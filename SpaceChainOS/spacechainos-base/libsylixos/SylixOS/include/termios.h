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
** 文   件   名: termios.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 01 月 10 日
**
** 描        述: 兼容 termios 库. (有限支持)
*********************************************************************************************************/

#ifndef __TERMIOS_H
#define __TERMIOS_H

#ifndef __SYLIXOS_H
#include "SylixOS.h"
#endif                                                                  /*  __SYLIXOS_H                 */

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************
  data types
*********************************************************************************************************/

typedef unsigned char   cc_t;
typedef unsigned int    speed_t;
typedef unsigned int    tcflag_t;

struct termios {
    tcflag_t    c_iflag;                                                /* input mode flags             */
    tcflag_t    c_oflag;                                                /* output mode flags            */
    tcflag_t    c_cflag;                                                /* control mode flags           */
    tcflag_t    c_lflag;                                                /* local mode flags             */
    cc_t        c_line;                                                 /* NOT USE!                     */
    cc_t        c_cc[NCCS];                                             /* control characters           */
};
/*********************************************************************************************************
  c_cc characters (defined in tty.h)
*********************************************************************************************************/

/*********************************************************************************************************
  c_iflag bits
*********************************************************************************************************/

#define IGNBRK      0000001                                             /* Ignore break condition.      */
#define BRKINT      0000002                                             /* Signal interrupt on break.   */
#define IGNPAR      0000004                                             /* Ignore char with parity err. */
#define PARMRK      0000010                                             /* Mark parity errors.          */
#define INPCK       0000020                                             /* Enable input parity check.   */
#define ISTRIP      0000040                                             /* Strip character              */
#define INLCR       0000100                                             /* Ignore CR                    */
#define IGNCR       0000200
#define ICRNL       0000400                                             /* Map CR to NL on input.       */
#define IUCLC       0001000                                             /* Map upper-case to lower-case */
                                                                        /*  on input (LEGACY).          */
#define IXON        0002000                                             /* Enable start/stop input ctl. */
#define IXANY       0004000                                             /* Enable any character to      */
                                                                        /* restart output.              */
#define IXOFF       0010000                                             /* Enable start/stop output ctl.*/
#define IMAXBEL     0020000

/*********************************************************************************************************
  c_oflag bits
*********************************************************************************************************/

#define OPOST       0000001                                             /* Post-process output          */
#define OLCUC       0000002                                             /* Map lower-case to upper-case */
                                                                        /* on output (LEGACY).          */
#define ONLCR       0000004                                             /* Map NL to CR-NL on output.   */
#define OCRNL       0000010                                             /* Map CR to NL on output.      */
#define ONOCR       0000020                                             /* No CR output at column 0.    */
#define ONLRET      0000040                                             /* NL performs CR function.     */
#define OFILL       0000100                                             /* Use fill characters for delay*/
#define OFDEL       0000200

#define NLDLY       0000400                                             /* Select newline delays:       */
#define   NL0       0000000                                             /* Newline character type 0.    */
#define   NL1       0000400                                             /* Newline character type 1.    */

#define CRDLY       0003000                                             /* Select carriage-return delays*/
#define   CR0       0000000                                             /* Carriage-return delay type 0.*/
#define   CR1       0001000                                             /* Carriage-return delay type 1.*/
#define   CR2       0002000                                             /* Carriage-return delay type 2.*/
#define   CR3       0003000                                             /* Carriage-return delay type 3.*/

#define TABDLY      0014000                                             /* Select horizontal-tab delays:*/
#define   TAB0      0000000                                             /* Horizontal-tab delay type 0. */
#define   TAB1      0004000                                             /* Horizontal-tab delay type 1. */
#define   TAB2      0010000                                             /* Horizontal-tab delay type 2. */
#define   TAB3      0014000                                             /* Expand tabs to spaces.       */
#define   XTABS     0014000

#define BSDLY       0020000                                             /* Select backspace delays:     */
#define   BS0       0000000                                             /* Backspace-delay type 0.      */
#define   BS1       0020000                                             /* Backspace-delay type 1.      */

#define VTDLY       0040000                                             /* Select vertical-tab delays:  */
#define   VT0       0000000                                             /* Vertical-tab delay type 0.   */
#define   VT1       0040000                                             /* Vertical-tab delay type 1.   */

#define FFDLY       0100000                                             /* Select form-feed delays:     */
#define   FF0       0000000                                             /* Form-feed delay type 0.      */
#define   FF1       0100000                                             /* Form-feed delay type 1.      */

/*********************************************************************************************************
  c_cflag bit meaning (SylixOS not support CIBAUD)
*********************************************************************************************************/

#define CBAUD       0x100F0000
#define  B0         0x00000000
#define  B50        0x00010000
#define  B75        0x00020000
#define  B110       0x00030000
#define  B134       0x00040000
#define  B150       0x00050000
#define  B200       0x00060000
#define  B300       0x00070000
#define  B600       0x00080000
#define  B1200      0x00090000
#define  B1800      0x000a0000
#define  B2400      0x000b0000
#define  B4800      0x000c0000
#define  B9600      0x000d0000
#define  B19200     0x000e0000
#define  B38400     0x000f0000

#define EXTA        B19200
#define EXTB        B38400

#define CSTOPB      STOPB

#define CBAUDEX     0x10000000
#define  B57600     0x10010000
#define  B115200    0x10020000
#define  B230400    0x10030000
#define  B460800    0x10040000
#define  B500000    0x10050000
#define  B576000    0x10060000
#define  B921600    0x10070000
#define  B1000000   0x10080000
#define  B1152000   0x10090000
#define  B1500000   0x100a0000
#define  B2000000   0x100b0000
#define  B2500000   0x100c0000
#define  B3000000   0x100d0000
#define  B3500000   0x100e0000
#define  B4000000   0x100f0000

#define CIBAUD      0x00000000                                          /* input baud rate (not used)   */
#define CRTSCTS     0x20000000                                          /* flow control                 */

/*********************************************************************************************************
  c_lflag bits
*********************************************************************************************************/

#define ISIG        0000001                                             /* Enable signals.              */
#define ICANON      0000002                                             /* Canonical input              */
                                                                        /* (erase and kill processing). */
#define XCASE       0000004                                             /* Canonical upper/lower        */
                                                                        /* presentation (LEGACY).       */
#define ECHO        0000010                                             /* Enable echo.                 */
#define ECHOE       0000020                                             /* Echo erase character as      */
                                                                        /* error-correcting backspace.  */
#define ECHOK       0000040                                             /* Echo KILL.                   */
#define ECHONL      0000100                                             /* Echo NL.                     */
#define NOFLSH      0000200                                             /* Disable flush after          */
                                                                        /* interrupt or quit.           */
#define TOSTOP      0000400                                             /* Send SIGTTOU for background  */
                                                                        /* output.                      */
#define ECHOCTL     0001000
#define ECHOPRT     0002000
#define ECHOKE      0004000
#define FLUSHO      0010000
#define PENDIN      0040000
#define IEXTEN      0100000                                             /* Enable extended input        */
                                                                        /* character processing.        */

/*********************************************************************************************************
  tcflow() and TCXONC use these
*********************************************************************************************************/

#define TCOOFF      0                                                   /* Transmit a STOP char,        */
                                                                        /* intended to suspend input.   */
#define TCOON       1                                                   /* Transmit a START character,  */
                                                                        /* intended to restart input.   */
#define TCIOFF      2                                                   /* Suspend output.              */
#define TCION       3                                                   /* Restart output.              */

/*********************************************************************************************************
  tcflush() and TCFLSH use these
*********************************************************************************************************/

#define TCIFLUSH    0                                                   /* Flush pending input. Flush   */
                                                                        /* untransmitted output.        */
#define TCOFLUSH    1                                                   /* Flush untransmitted output.  */
#define TCIOFLUSH   2                                                   /* Flush both pending input and */
                                                                        /* untransmitted output.        */

/*********************************************************************************************************
  tcsetattr uses these
*********************************************************************************************************/

#define TCSANOW     0                                                   /* Change attributes immediately*/
#define TCSADRAIN   1                                                   /* Change attributes when output*/
                                                                        /* has drained.                 */
#define TCSAFLUSH   2                                                   /* Change attributes when output*/
                                                                        /* has drained; also flush      */
                                                                        /* pending input.               */
#ifndef TCSASOFT
#define TCSASOFT    0
#endif

/*********************************************************************************************************
  termios api
*********************************************************************************************************/

LW_API speed_t cfgetispeed(const struct termios *);
LW_API speed_t cfgetospeed(const struct termios *);
LW_API int     cfsetispeed(struct termios *, speed_t);
LW_API int     cfsetospeed(struct termios *, speed_t);
LW_API int     tcdrain(int);
LW_API int     tcflow(int, int);
LW_API int     tcflush(int, int);
LW_API int     tcgetattr(int, struct termios *);
LW_API pid_t   tcgetsid(int);
LW_API int     tcsendbreak(int, int);
LW_API int     tcsetattr(int, int, const struct termios *);
LW_API void    cfmakeraw(struct termios *);

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __TERMIOS_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/

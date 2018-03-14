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
** 文   件   名: vim_fix.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 02 月 10 日
**
** 描        述: vi 编辑器移植层
*********************************************************************************************************/

#ifndef __VIM_FIX_H
#define __VIM_FIX_H

#include "SylixOS.h"

/*********************************************************************************************************
  vi 配置
*********************************************************************************************************/

#ifndef  ALIGN1
#define  ALIGN1
#endif                                                                  /*  ALIGN1                      */

#ifndef  MAIN_EXTERNALLY_VISIBLE
#define  MAIN_EXTERNALLY_VISIBLE
#endif                                                                  /*  MAIN_EXTERNALLY_VISIBLE     */

typedef  int    smallint;

#define  ENABLE_LOCALE_SUPPORT                  0
#define  ENABLE_FEATURE_ALLOW_EXEC              0

#define  ENABLE_FEATURE_VI_8BIT                 1
#define  ENABLE_FEATURE_VI_OPTIMIZE_CURSOR      1
#if LW_CFG_SIGNAL_EN > 0
#define  ENABLE_FEATURE_VI_USE_SIGNALS          1
#else
#define  ENABLE_FEATURE_VI_USE_SIGNALS          0
#endif
#define  ENABLE_FEATURE_VI_CRASHME              0
#define  ENABLE_FEATURE_VI_DOT_CMD              1
#define  ENABLE_FEATURE_VI_YANKMARK             1
#define  ENABLE_FEATURE_VI_READONLY             0
#define  ENABLE_FEATURE_VI_SEARCH               1
#define  ENABLE_FEATURE_VI_COLON                1
#define  ENABLE_FEATURE_VI_SETOPTS              1
#define  ENABLE_FEATURE_VI_WIN_RESIZE           1
#define  ENABLE_FEATURE_VI_SET                  1

#define  CONFIG_FEATURE_VI_MAX_LEN              2048

#define RESERVE_CONFIG_BUFFER(buffer,len)       char    buffer[len]

#define  BB_VER                                 "busybox vi for sylixos : 1.10.2"
#define  BB_BT                                  ""

#ifndef  __cplusplus
#define  inline                                 LW_INLINE
#endif                                                                  /*  __cplusplus                 */

#if ENABLE_FEATURE_VI_COLON
#define USE_FEATURE_VI_COLON(x)                 x
#else
#define USE_FEATURE_VI_COLON(x)
#endif

#if ENABLE_FEATURE_VI_READONLY
#define USE_FEATURE_VI_READONLY(...)            __VA_ARGS__
#else
#define USE_FEATURE_VI_READONLY(...)
#endif

#if ENABLE_FEATURE_VI_YANKMARK
#define USE_FEATURE_VI_YANKMARK(x)              x
#else
#define USE_FEATURE_VI_YANKMARK(x)
#endif

#if ENABLE_FEATURE_VI_SEARCH
#define USE_FEATURE_VI_SEARCH(x)                x
#else
#define USE_FEATURE_VI_SEARCH(x)
#endif

#define ARRAY_SIZE(x)                           (sizeof(x) / sizeof((x)[0]))

#define SET_PTR_TO_GLOBALS(x) do {                      \
	    (*(struct globals**)&ptr_to_globals) = (x);     \
} while (0)
#define bb_show_usage()

#include "unistd.h"
#include "string.h"
#include "stdlib.h"
#include "time.h"
#include "poll.h"
#include "ctype.h"
#include "termios.h"
#include "setjmp.h"

int     vi_safe_poll(struct pollfd fds[], nfds_t nfds, int timeout);
ssize_t vi_safe_read(int fd, void *buf, size_t len);
ssize_t vi_safe_write(int fd, const void *buf, size_t len);

#define safe_poll(fds, nfds, timeout)           vi_safe_poll(fds, nfds, timeout)
#define safe_read(fd, buf , count)              vi_safe_read(fd, buf, count)
#define safe_write(fd,buf , count)              vi_safe_write(fd, buf, count)

#ifdef __GNUC__
#define ATTRIBUTE_UNUSED                        __attribute__((unused))
#else
#define ATTRIBUTE_UNUSED
#endif

/*********************************************************************************************************
  sylixos 函数声明
*********************************************************************************************************/

#define  __VI_TERMINAL_WIDTH                    80
#define  __VI_TERMINAL_HEIGHT                   24

extern   int        get_terminal_width_height(const int  fd, int  *width, int  *height);
extern   char      *last_char_is(const char *s, int c);
extern   void      *lib_xzalloc(size_t s);
extern   ssize_t    full_write(int fd, const void *buf, size_t len);
extern   int        isblank(int c);
extern   int        bb_putchar(int ch);

#define xzalloc     lib_xzalloc

#endif                                                                  /*  __VIM_FIX_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/

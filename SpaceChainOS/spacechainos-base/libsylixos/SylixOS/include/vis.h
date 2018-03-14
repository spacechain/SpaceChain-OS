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
** 文   件   名: vis.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 01 月 17 日
**
** 描        述: BSD vis.h, 需要外部库支持.
*********************************************************************************************************/

#ifndef __VIS_H
#define __VIS_H

#include "sys/types.h"

/*********************************************************************************************************
  to select alternate encoding format
*********************************************************************************************************/
#define VIS_OCTAL           0x01                            /* use octal \ddd format                    */
#define VIS_CSTYLE          0x02                            /* use \[nrft0..] where appropiate          */

/*********************************************************************************************************
  to alter set of characters encoded (default is to encode all
  non-graphic except space, tab, and newline).
*********************************************************************************************************/
#define VIS_SP              0x04                            /* also encode space                        */
#define VIS_TAB             0x08                            /* also encode tab                          */
#define VIS_NL              0x10                            /* also encode newline                      */
#define VIS_WHITE           (VIS_SP | VIS_TAB | VIS_NL)
#define VIS_SAFE            0x20                            /* only encode "unsafe" characters          */

/*********************************************************************************************************
  other
*********************************************************************************************************/
#define VIS_NOSLASH         0x40                            /* inhibit printing '\'                     */
#define VIS_HTTPSTYLE       0x80                            /* http-style escape % HEX HEX              */

/*********************************************************************************************************
 * unvis return codes
*********************************************************************************************************/
#define UNVIS_VALID         1                               /* character valid                          */
#define UNVIS_VALIDPUSH     2                               /* character valid, push back passed char   */
#define UNVIS_NOCHAR        3                               /* valid sequence, no character produced    */
#define UNVIS_SYNBAD        -1                              /* unrecognized escape sequence             */
#define UNVIS_ERROR         -2                              /* decoder in unknown state (unrecoverable) */

/*********************************************************************************************************
 * unvis flags
*********************************************************************************************************/
#define UNVIS_END    1    /* no more characters */

#include "sys/cdefs.h"

__BEGIN_DECLS

char   *vis(char *, int, int, int);
char   *svis(char *, int, int, int, const char *);
int     strvis(char *, const char *, int);
int     strsvis(char *, const char *, int, const char *);
int     strvisx(char *, const char *, size_t, int);
int     strsvisx(char *, const char *, size_t, int, const char *);
int     strunvis(char *, const char *);
int     strunvisx(char *, const char *, int);
#ifndef __LIBC12_SOURCE__
int     unvis(char *, int, int *, int);
#endif

__END_DECLS

#endif                                                                  /*  __UTMPX_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/

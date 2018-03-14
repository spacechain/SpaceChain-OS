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
** 文   件   名: netdb.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 11 月 04 日
**
** 描        述: netdb.
*********************************************************************************************************/

#ifndef  __NETDB_H
#define  __NETDB_H

#include "lwip/netdb.h"

#ifndef NETDB_INTERNAL
#define NETDB_INTERNAL  -1
#endif

#ifndef NETDB_SUCCESS
#define NETDB_SUCCESS    0
#endif

/* Indicates that the socket is intended for bind()+listen(). */
#ifndef AI_PASSIVE
# define AI_PASSIVE     1
#endif /* AI_PASSIVE */

/* Return the canonical name. */
#ifndef AI_CANONNAME
# define AI_CANONNAME   2
#endif /* AI_CANONNAME */

/* The HostName argument must be a numeric address */
#ifndef AI_NUMERICHOST
# define AI_NUMERICHOST 4
#endif /* AI_NUMERICHOST */

#ifndef AI_NUMERICSERV
# define AI_NUMERICSERV 8 /* prevent service name resolution */
#endif /* AI_NUMERICSERV */

/* valid flags for addrinfo (not a standard def, apps should not use it) */
#ifndef AI_MASK
#  define AI_MASK	\
        (AI_PASSIVE | AI_CANONNAME | AI_NUMERICHOST | AI_NUMERICSERV)
#endif /* AI_MASK */

/*
 * Maximum length of FQDN and servie name for getnameinfo().
 */
#undef NI_MAXHOST
#define NI_MAXHOST	1025

#undef NI_MAXSERV
#define NI_MAXSERV	32

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

void  herror(const char *s);
const char *hstrerror(int err);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __NETDB_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/

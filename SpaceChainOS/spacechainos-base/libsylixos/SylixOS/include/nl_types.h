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
** 文   件   名: nl_types.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 09 月 17 日
**
** 描        述: 消息类别, 需要外部库支持.
*********************************************************************************************************/

#ifndef __NL_TYPES_H
#define __NL_TYPES_H

#include "sys/cdefs.h"

#ifdef _NLS_PRIVATE
/*
 * MESSAGE CATALOG FILE FORMAT.
 *
 * The NetBSD message catalog format is similar to the format used by
 * Svr4 systems.  The differences are:
 *   * fixed byte order (big endian)
 *   * fixed data field sizes
 *
 * A message catalog contains four data types: a catalog header, one
 * or more set headers, one or more message headers, and one or more
 * text strings.
 */

#define _NLS_MAGIC	0xff88ff89

struct _nls_cat_hdr {
	int32_t __magic;
	int32_t __nsets;
	int32_t __mem;
	int32_t __msg_hdr_offset;
	int32_t __msg_txt_offset;
} ;

struct _nls_set_hdr {
	int32_t __setno;	/* set number: 0 < x <= NL_SETMAX */
	int32_t __nmsgs;	/* number of messages in the set  */
	int32_t __index;	/* index of first msg_hdr in msg_hdr table */
} ;

struct _nls_msg_hdr {
	int32_t __msgno;	/* msg number: 0 < x <= NL_MSGMAX */
	int32_t __msglen;
	int32_t __offset;
} ;

#endif /* _NLS_PRIVATE */

#define	NL_SETD		1
#define NL_CAT_LOCALE   1

typedef struct __nl_cat_d {
	void	*__data;
	int	__size;
} *nl_catd;

typedef long	nl_item;

__BEGIN_DECLS
nl_catd  catopen(const char *, int);
char    *catgets(nl_catd, int, int, const char *)
	__attribute__((__format_arg__(4)));
int	 catclose(nl_catd);
__END_DECLS

#endif                                                                  /*  __NL_TYPES_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/

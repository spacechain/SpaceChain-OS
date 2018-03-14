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
** 文   件   名: regexp.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 09 月 17 日
**
** 描        述: 正则表达式支持库, 需要外部库支持.
*********************************************************************************************************/

#ifndef __REGEXP_H
#define __REGEXP_H

/*
 * Definitions etc. for regexp(3) routines.
 *
 * Caveat:  this is V8 regexp(3) [actually, a reimplementation thereof],
 * not the System V one.
 */
#define NSUBEXP  10

typedef struct regexp {
	char *startp[NSUBEXP];
	char *endp[NSUBEXP];
	char regstart;		/* Internal use only. */
	char reganch;		/* Internal use only. */
	char *regmust;		/* Internal use only. */
	int regmlen;		/* Internal use only. */
	char program[1];	/* Unwarranted chumminess with compiler. */
} regexp;

#include <sys/cdefs.h>

__BEGIN_DECLS
regexp *regcomp(const char *);
int     regexec(const  regexp *, const char *);
void    regsub(const  regexp *, const char *, char *);
void    regerror(const char *);
__END_DECLS

#endif                                                                  /*  __REGEXP_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

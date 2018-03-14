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
** 文   件   名: lib_ctype.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 02 月 07 日
**
** 描        述: 库声明
*********************************************************************************************************/

#ifndef __LIB_CTYPE_H
#define __LIB_CTYPE_H

extern const unsigned char	*_ctype_;

int     lib_isalnum(int);
int     lib_isalpha(int);
int     lib_iscntrl(int);
int     lib_isdigit(int);
int     lib_isgraph(int);
int     lib_islower(int);
int     lib_isprint(int);
int     lib_ispunct(int);
int     lib_isspace(int);
int     lib_isupper(int);
int     lib_isxdigit(int);
int     lib_isascii(int);
int     lib_toascii(int);
int     lib_isblank(int);

#endif                                                                  /*  __LIB_CTYPE_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/

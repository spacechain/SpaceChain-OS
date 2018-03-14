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
** 文   件   名: lib_string.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 13 日
**
** 描        述: 库声明
*********************************************************************************************************/

#ifndef __LIB_STRING_H
#define __LIB_STRING_H

PCHAR  lib_rindex(CPCHAR      pcString, INT iC);
PCHAR  lib_index(CPCHAR       pcString, INT iC);
PCHAR  lib_strchrnul(CPCHAR  pcString, INT iC);

#define  lib_strrchr     lib_rindex
#define  lib_strchr      lib_index

PCHAR  lib_stpcpy(PCHAR  pcDest, CPCHAR  pcSrc);
PCHAR  lib_stpncpy(PCHAR  pcDest, CPCHAR  pcSrc, size_t  stN);

PCHAR  lib_strcat(PCHAR  pcDest, CPCHAR       pcSrc);
PCHAR  lib_strncat(PCHAR pcDest, CPCHAR       pcSrc, size_t  stN);
size_t lib_strlcat(PCHAR  pcDest, CPCHAR  pcSrc, size_t  stN);

INT    lib_strcmp(CPCHAR       pcStr1, CPCHAR       pcStr2);
INT    lib_strncmp(CPCHAR      pcStr1, CPCHAR       pcStr2, size_t  stLen);

INT    lib_strcasecmp(CPCHAR pcStr1, CPCHAR  pcStr2);
INT    lib_strncasecmp(CPCHAR pcStr1, CPCHAR  pcStr2, size_t  stLen);

PCHAR  lib_strcpy(PCHAR  pcDest, CPCHAR       pcSrc);
PCHAR  lib_strncpy(PCHAR pcDest, CPCHAR       pcSrc, size_t  stN);
size_t lib_strlcpy(PCHAR  pcDest, CPCHAR  pcSrc, size_t  stN);
VOID   lib_bcopy(CPVOID  pvSrc, PVOID pvDest, size_t  stN);

size_t lib_strlen(CPCHAR       pcStr);
size_t lib_strnlen(CPCHAR      pcStr,  size_t  stN);

PCHAR  lib_strdup(CPCHAR pcStr);
PCHAR  lib_xstrdup(CPCHAR pcStr);

PCHAR  lib_strndup(CPCHAR pcStr, size_t  stSize);
PCHAR  lib_xstrndup(CPCHAR pcStr, size_t  stSize);

INT    lib_memcmp(CPVOID pvMem1, CPVOID       pvMem2, size_t  stCount);
INT    lib_bcmp(CPVOID  pvMem1, CPVOID  pvMem2, size_t  stCount);
PVOID  lib_memcpy(PVOID  pvDest, CPVOID       pvSrc,  size_t  stCount);
PVOID  lib_mempcpy(PVOID  pvDest, CPVOID      pvSrc,  size_t  stCount);
PVOID  lib_memset(PVOID  pvDest, INT  iC, size_t  stCount);
VOID   lib_bzero(PVOID   pvStr, size_t  stCount);
PVOID  lib_memchr(CPVOID pvBuf, INT c, size_t  stCnt);
PVOID  lib_memrchr(CPVOID  pvBuf, INT c, size_t  stCnt);
PCHAR  lib_strnset(PCHAR  pcStr, INT  iVal, size_t  stCount);

INT    lib_tolower(INT  iC);
INT    lib_toupper(INT  iC);

INT    lib_strerror_r(INT  iNum, PCHAR  pcBuffer, size_t stLen);
PCHAR  lib_strerror(INT  iNum);
PCHAR  lib_strsignal(INT  iSigNo);
PCHAR  lib_strstr(CPCHAR  cpcS1, CPCHAR  cpcS2);
size_t lib_strcspn(CPCHAR  cpcS1, CPCHAR  cpcS2);
PCHAR  lib_strpbrk(CPCHAR  cpcS1, CPCHAR  cpcS2);

/*********************************************************************************************************
  下面的函数来自于其他开源系统
*********************************************************************************************************/

int    lib_ffs(int valu);
size_t lib_strftime(char *s, size_t maxsize, const char *format, const struct tm *t);
char  *lib_strptime(const char *buf, const char *fmt, struct tm *tm);
int    lib_stricmp(const char *s1, const char *s2);
size_t lib_strspn(const char *s1, const char *s2);
char  *lib_strtok(char *s, const char *delim);
char  *lib_strtok_r(char *s, const char *delim, char **last);
size_t lib_strxfrm(char *s1, const char *s2, size_t n);
void   lib_swab(const void *from, void *to, size_t len);
char  *lib_strsep(char **s, const char *ct);

#define lib_strcoll         lib_strcmp                                  /*  默认等同 lib_strcmp         */

#endif                                                                  /*  __LIB_STRING_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/

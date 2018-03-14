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
** 文   件   名: string.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 04 月 02 日
**
** 描        述: 兼容 C 库.
*********************************************************************************************************/

#ifndef __STRING_H
#define __STRING_H

#ifndef __SYLIXOS_H
#include "SylixOS.h"
#endif                                                                  /*  __SYLIXOS_H                 */

#ifdef __cplusplus
extern "C" {
#endif

__BEGIN_NAMESPACE_STD

__LW_RETU_FUNC_DECLARE(int, ffs, (int valu))

__LW_RETU_FUNC_DECLARE(char *, rindex, (const char *pcString, int iC))
__LW_RETU_FUNC_DECLARE(char *, index, (const char *pcString, int iC))
__LW_RETU_FUNC_DECLARE(char *, strchrnul, (const char *pcString, int iC))
__LW_RETU_FUNC_DECLARE(char *, strrchr, (const char *pcString, int iC))
__LW_RETU_FUNC_DECLARE(char *, strchr, (const char *pcString, int iC))

__LW_RETU_FUNC_DECLARE(char *, strcat, (char *pcDest, const char *pcSrc))
__LW_RETU_FUNC_DECLARE(char *, strncat, (char *pcDest, const char *pcSrc, size_t stN))
__LW_RETU_FUNC_DECLARE(size_t, strlcat, (char *pcDest, const char *pcSrc, size_t stN))

__LW_RETU_FUNC_DECLARE(int, strcmp, (const char *pcStr1, const char *pcStr2))
__LW_RETU_FUNC_DECLARE(int, strncmp, (const char *pcStr1, const char *pcStr2, size_t stLen))

__LW_RETU_FUNC_DECLARE(int, strcasecmp, (const char *pcStr1, const char *pcStr2))
__LW_RETU_FUNC_DECLARE(int, strncasecmp, (const char *pcStr1, const char *pcStr2, size_t  stLen))

__LW_RETU_FUNC_DECLARE(char *, stpcpy, (char *pcDest, const char *pcSrc))
__LW_RETU_FUNC_DECLARE(char *, stpncpy, (char *pcDest, const char *pcSrc, size_t stN))

__LW_RETU_FUNC_DECLARE(char *, strcpy, (char *pcDest, const char *pcSrc))
__LW_RETU_FUNC_DECLARE(char *, strncpy, (char *pcDest, const char *pcSrc, size_t stN))
__LW_RETU_FUNC_DECLARE(size_t, strlcpy, (char *pcDest, const char *pcSrc, size_t stN))
__LW_RETU_FUNC_DECLARE(void, bcopy, (const void *pvSrc, void *pvDest, size_t stN))


__LW_RETU_FUNC_DECLARE(size_t, strlen, (const char *pcStr))
__LW_RETU_FUNC_DECLARE(size_t, strnlen, (const char *pcStr, size_t stN))


__LW_RETU_FUNC_DECLARE(char *, strdup, (const char *pcStr))
__LW_RETU_FUNC_DECLARE(char *, xstrdup, (const char *pcStr))

__LW_RETU_FUNC_DECLARE(char *, strndup, (const char *pcStr, size_t  stSize))
__LW_RETU_FUNC_DECLARE(char *, xstrndup, (const char *pcStr, size_t  stSize))

__LW_RETU_FUNC_DECLARE(int, memcmp, (const void *pvMem1, const void *pvMem2, size_t stCount))
__LW_RETU_FUNC_DECLARE(int, bcmp, (const void *pvMem1, const void *pvMem2, size_t stCount))
__LW_RETU_FUNC_DECLARE(void *, memcpy, (void *pvDest, const void *pvSrc, size_t stCount))
__LW_RETU_FUNC_DECLARE(void *, mempcpy, (void *pvDest, const void *pvSrc, size_t stCount))
__LW_RETU_FUNC_DECLARE(void *, memset, (void *pvDest, int iC, size_t stCount))
__LW_RETU_FUNC_DECLARE(void, bzero, (void *pvStr, size_t stCount))
__LW_RETU_FUNC_DECLARE(void *, memchr, (const void *pvBuf, int c, size_t stCnt))
__LW_RETU_FUNC_DECLARE(void *, memrchr, (const void *pvBuf, int c, size_t stCnt))
__LW_RETU_FUNC_DECLARE(char *, strnset, (char *pcStr, int iVal, size_t stCount))

__LW_RETU_FUNC_DECLARE(int, tolower, (int  iC))
__LW_RETU_FUNC_DECLARE(int, toupper, (int  iC))

__LW_RETU_FUNC_DECLARE(int, strerror_r, (int  iNum, char *pcBuffer, size_t stLen))
__LW_RETU_FUNC_DECLARE(char *, strerror, (int  iNum))
__LW_RETU_FUNC_DECLARE(char *, strsignal, (int  iSigNo))
__LW_RETU_FUNC_DECLARE(char *, strstr, (const char *cpcS1, const char *cpcS2))
__LW_RETU_FUNC_DECLARE(size_t, strcspn, (const char *cpcS1, const char *cpcS2))
__LW_RETU_FUNC_DECLARE(char *, strpbrk, (const char *cpcS1, const char *cpcS2))

__LW_RETU_FUNC_DECLARE(size_t, strftime, (char *s, size_t maxsize, const char *format, const struct tm *t))
__LW_RETU_FUNC_DECLARE(char *, strptime, (const char *buf, const char *fmt, struct tm *tm))

__LW_RETU_FUNC_DECLARE(int, stricmp, (const char *s1, const char *s2))
__LW_RETU_FUNC_DECLARE(size_t, strspn, (const char *s1, const char *s2))
__LW_RETU_FUNC_DECLARE(char *, strtok, (char *s, const char *delim))
__LW_RETU_FUNC_DECLARE(char *, strtok_r, (char *s, const char *delim, char **last))

__LW_RETU_FUNC_DECLARE(int, strcoll, (const char *pcStr1, const char *pcStr2))
__LW_RETU_FUNC_DECLARE(size_t, strxfrm, (char *s1, const char *s2, size_t n))
__LW_RETU_FUNC_DECLARE(void, swab, (const void *from, void *to, size_t len))
__LW_RETU_FUNC_DECLARE(char *, strsep, (char **s, const char *ct))

__LW_RETU_FUNC_DECLARE(void *, memmove, (void *pvDest, const void *pvSrc, size_t stCount))

__END_NAMESPACE_STD

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __STRING_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

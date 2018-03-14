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
** 文   件   名: crypt.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 03 月 30 日
**
** 描        述: UNIX libcrypt.
*********************************************************************************************************/

#ifndef __CRYPT_H
#define __CRYPT_H

#include "sys/types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char *crypt(const char *key, const char *salt);
extern char *crypt_safe(const char *key, const char *salt, char *res, size_t reslen);

extern int   setkey(const char *key);
extern int   encrypt(char *block, int flag);

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __CRYPT_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/

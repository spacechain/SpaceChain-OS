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
** 文   件   名: ctype.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 04 月 02 日
**
** 描        述: 兼容 C 库.
*********************************************************************************************************/

#ifndef __CTYPE_H
#define __CTYPE_H

#ifndef __SYLIXOS_H
#include "SylixOS.h"
#endif                                                                  /*  __SYLIXOS_H                 */

#define _U      0x01
#define _L      0x02
#define _N      0x04
#define _S      0x08
#define _P      0x10
#define _C      0x20
#define _X      0x40
#define _B      0x80

#ifdef __cplusplus
extern "C" {
#endif

__BEGIN_NAMESPACE_STD

__LW_RETU_FUNC_DECLARE(int, isalnum, (int c))
__LW_RETU_FUNC_DECLARE(int, isalpha, (int c))
__LW_RETU_FUNC_DECLARE(int, iscntrl, (int c))
__LW_RETU_FUNC_DECLARE(int, isdigit, (int c))
__LW_RETU_FUNC_DECLARE(int, isgraph, (int c))
__LW_RETU_FUNC_DECLARE(int, islower, (int c))
__LW_RETU_FUNC_DECLARE(int, isprint, (int c))
__LW_RETU_FUNC_DECLARE(int, ispunct, (int c))
__LW_RETU_FUNC_DECLARE(int, isspace, (int c))
__LW_RETU_FUNC_DECLARE(int, isupper, (int c))
__LW_RETU_FUNC_DECLARE(int, isxdigit, (int c))
__LW_RETU_FUNC_DECLARE(int, isascii, (int c))
__LW_RETU_FUNC_DECLARE(int, toascii, (int c))
__LW_RETU_FUNC_DECLARE(int, isblank, (int c))
__LW_RETU_FUNC_DECLARE(int, toupper, (int c))
__LW_RETU_FUNC_DECLARE(int, tolower, (int c))

extern const unsigned char *_ctype_;
extern const short         *_toupper_tab_;
extern const short         *_tolower_tab_;

__END_NAMESPACE_STD

/*********************************************************************************************************
  private
*********************************************************************************************************/

#ifdef _CTYPE_PRIVATE

#include "limits.h"

#define _CTYPE_NUM_CHARS    (1 << CHAR_BIT)
#define _CTYPE_ID           "BSDCTYPE"
#define _CTYPE_REV          2

extern const uint8_t    _C_ctype_[];
extern const int16_t    _C_toupper_[];
extern const int16_t    _C_tolower_[];

#endif                                                                  /*  _CTYPE_PRIVATE              */

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  __CTYPE_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/

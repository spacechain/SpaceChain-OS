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
** 文   件   名: setjmp.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 10 月 23 日
**
** 描        述: 兼容 C 库.
*********************************************************************************************************/

#ifndef __SETJMP_H
#define __SETJMP_H

#ifndef __SYLIXOS_H
#include "SylixOS.h"
#endif

#include "sys/cdefs.h"

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

__BEGIN_NAMESPACE_STD

struct __lw_jmp_buf_tag {
    LW_STACK    __saved_regs[ARCH_REG_CTX_WORD_SIZE];                   /*  REGs + SP                   */
    sigset_t    __saved_mask;
    int         __mask_was_saved;
};

typedef struct __lw_jmp_buf_tag   jmp_buf[1];
typedef struct __lw_jmp_buf_tag   sigjmp_buf[1];

void   longjmp(jmp_buf, int);
void   siglongjmp(sigjmp_buf, int);
void   _longjmp(jmp_buf, int);

int    setjmp(jmp_buf);
int    sigsetjmp(sigjmp_buf, int);
int    _setjmp(jmp_buf);

__END_NAMESPACE_STD

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __SETJMP_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

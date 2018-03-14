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
** 文   件   名: lib_strsep.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 08 月 22 日
**
** 描        述: 库
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: lib_strsep
** 功能描述: Split a string into tokens
** 输　入  : s     The string to be searched
**           ct    The characters to search for
** 输　出  : It returns empty tokens, too, behaving exactly like the libc function
             of that name. In fact, it was stolen from glibc2 and de-fancy-fied.
             Same semantics, slimmer shape. ;)
** 全局变量:
** 调用模块:
*********************************************************************************************************/
char *lib_strsep (char **s, const char *ct)
{
    char *sbegin = *s;
    char *end;

    if (sbegin == LW_NULL) {
        return  (NULL);
    }

    end = lib_strpbrk(sbegin, ct);
    if (end) {
        *end++ = '\0';
    }
    *s = end;

    return  (sbegin);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

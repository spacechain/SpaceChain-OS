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
** 文   件   名: lib_locale.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 05 月 30 日
**
** 描        述: locale.h.这里没有实现任何功能, 只是为了兼容性需要而已, 
                 如果需要完整 locale 支持, 则需要外部 locale 库支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "sys/cdefs.h"
#include "lib_locale.h"
/*********************************************************************************************************
  lconv 表
*********************************************************************************************************/
static char         _G_lconvNull[] = "";
static struct lconv _G_lconvArray = {
    ".",
    _G_lconvNull,
    _G_lconvNull,
    _G_lconvNull,
    _G_lconvNull,
    _G_lconvNull,
    _G_lconvNull,
    _G_lconvNull,
    _G_lconvNull,
    _G_lconvNull,
    
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
    
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
    
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
    __ARCH_CHAR_MAX,
};
/*********************************************************************************************************
** 函数名称: lib_localeconv
** 功能描述: The localeconv() function shall set the components of an object with the type struct lconv 
             with the values appropriate for the formatting of numeric quantities (monetary and otherwise) 
             according to the rules of the current locale.
** 输　入  : NONE
** 输　出  : struct lconv
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_WEAK struct lconv *lib_localeconv (void)
{
    return  (&_G_lconvArray);
}
/*********************************************************************************************************
** 函数名称: lib_setlocale
** 功能描述: The setlocale() function selects the appropriate piece of the program's locale.
** 输　入  : category
**           locale
** 输　出  : Upon successful completion, setlocale() shall return the string associated with the specified
             category for the new locale. Otherwise, setlocale() shall return a null pointer and the 
             program's locale is not changed.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
LW_WEAK char *lib_setlocale (int category, const char *locale)
{
    return  (LW_NULL);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

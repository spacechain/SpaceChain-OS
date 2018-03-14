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
** 文   件   名: lib_locale.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 05 月 30 日
**
** 描        述: locale.h 这里没有实现任何功能, 只是为了兼容性需要而已, 
                 如果需要完整 locale 支持, 则需要外部 locale 库支持.
*********************************************************************************************************/

#ifndef __LIB_LOCALE_H
#define __LIB_LOCALE_H

struct lconv {
    char *decimal_point;
    char *thousands_sep;
    char *grouping;
    char *int_curr_symbol;
    char *currency_symbol;
    char *mon_decimal_point;
    char *mon_thousands_sep;
    char *mon_grouping;
    char *positive_sign;
    char *negative_sign;
    
    char int_frac_digits;
    char frac_digits;
    
    char p_cs_precedes;
    char p_sep_by_space;
    char n_cs_precedes;
    char n_sep_by_space;
    char p_sign_posn;
    char n_sign_posn;
    
    char int_p_cs_precedes;
	char int_n_cs_precedes;
	char int_p_sep_by_space;
	char int_n_sep_by_space;
	char int_p_sign_posn;
	char int_n_sign_posn;
};

#define	LC_ALL		0
#define	LC_COLLATE	1
#define	LC_CTYPE	2
#define	LC_MONETARY	3
#define	LC_NUMERIC	4
#define	LC_TIME		5
#define LC_MESSAGES	6

#define	_LC_LAST	7		/* marks end */

struct lconv *lib_localeconv(void);
char         *lib_setlocale(int, const char *);

#endif                                                                  /*  __LIB_LOCALE_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/

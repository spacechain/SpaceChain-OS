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
** 文   件   名: lib_rindex.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 25 日
**
** 描        述: 库
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: lib_rindex
** 功能描述: If successful, lib_rindex() returns a pointer to the last occurrence of character iC in 
**           string pcString. On failure, it returns a null character. 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PCHAR  lib_rindex (CPCHAR  pcString, INT iC)
{
    REGISTER PCHAR    pcLast = LW_NULL;
    REGISTER PCHAR    pcStr  = (PCHAR)pcString;
    
    for (; *pcStr != PX_EOS; pcStr++) {
        if (*pcStr == (CHAR)iC) {
            pcLast = pcStr;
        }
    }
    
    if (iC == PX_EOS) {                                                 /*  find the end of string      */
        pcLast = pcStr;
    }
    
    return  (pcLast);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

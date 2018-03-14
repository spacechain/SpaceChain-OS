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
** 文   件   名: lib_index.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 25 日
**
** 描        述: 库

** BUG:
2012.10.22  移植 python 时发现此问题, python 使用 strchr(..., '\0'); 来寻找字符串结尾, 这时不应该返回null
*********************************************************************************************************/
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: lib_index
** 功能描述: If successful, lib_index() returns a pointer to the first occurrence of character iC in 
             string pcString. On failure, it returns a null character. 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PCHAR  lib_index (CPCHAR  pcString, INT iC)
{
    REGISTER PCHAR    pcFirst = LW_NULL;
    REGISTER PCHAR    pcStr   = (PCHAR)pcString;
    
    for (; *pcStr != PX_EOS; pcStr++) {
        if (*pcStr == (CHAR)iC) {                                       /*  FIRST CHARACTER             */
            pcFirst = pcStr;
            break;
        }
    }
    
    if (iC == PX_EOS) {                                                 /*  find the end of string      */
        pcFirst = pcStr;
    }
    
    return  (pcFirst);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

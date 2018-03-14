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
** 文   件   名: ttinyString.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 07 月 27 日
**
** 描        述: 一个超小型的 shell 系统, 字符串基本处理函数.

** BUG
2008.08.19  修改了__tshellStrFormat()函数针对连续""后接空格处理的 BUG.
2008.11.26  升级变量分界的算法.
2009.12.14  在 __TTNIYSHELL_SEPARATOR 加入冒号.
2010.02.03  a=$b 当变量 b 字串中存在空格时, 赋值不正确.
2014.09.06  变量分隔符加入 \r\n\t.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
#include "ttinyShellLib.h"
#include "ttinyString.h"
#include "../SylixOS/shell/ttinyVar/ttinyVarLib.h"
/*********************************************************************************************************
** 函数名称: __tshellStrSkipLeftBigBracket
** 功能描述: 忽略左边的大括号
** 输　入  : pcPtr           起始位置
** 输　出  : 结束位置
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PCHAR  __tshellStrSkipLeftBigBracket (PCHAR  pcPtr)
{
    while (*pcPtr != PX_EOS) {
        if (*pcPtr == '{') {
            pcPtr++;
        } else {
            break;
        }
    }
    
    return  (pcPtr);
}
/*********************************************************************************************************
** 函数名称: __tshellStrSkipRightBigBracket
** 功能描述: 忽略右边的大括号
** 输　入  : pcPtr           起始位置
** 输　出  : 结束位置
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PCHAR  __tshellStrSkipRightBigBracket (PCHAR  pcPtr)
{
    while (*pcPtr != PX_EOS) {
        if (*pcPtr == '}') {
            pcPtr++;
        } else {
            break;
        }
    }
    
    return  (pcPtr);
}
/*********************************************************************************************************
** 函数名称: IsSeparator
** 功能描述: 检测一个字符是否为分隔符
** 输　入  : cChar              字符
** 输　出  : 是否为分隔符
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  __tshellIsSeparator (CHAR  cChar)
{
#define __TTNIYSHELL_SEPARATOR     "+-*/%!&|~^()<>=,;: \"\\}\r\n\t"

    if (lib_strchr(__TTNIYSHELL_SEPARATOR, cChar)) {
        return  (LW_TRUE);
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** 函数名称: __tshellStrConvertVar
** 功能描述: 将命令字符串中的变量转换为变量的值
** 输　入  : pcCmd               shell 命令
**           pcCmdOut            替换后的命令串
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
** 注  意  : 这里仅仅是判断转义字符, 但没有删除转义字符.
*********************************************************************************************************/
ULONG  __tshellStrConvertVar (CPCHAR  pcCmd, PCHAR  pcCmdOut)
{
             PCHAR  pcVarValue = LW_NULL;
    REGISTER INT    iTotalLen  = 0;                                     /*  转换后的命令总长            */
             INT    iVarValueLen;                                       /*  变量名长度                  */
    
    REGISTER PCHAR  pcTemp = (PCHAR)pcCmd;
             CHAR   cValName[LW_CFG_SHELL_MAX_VARNAMELEN];              /*  变量名缓冲区                */
             CHAR   cLastError[11];                                     /*  最后一次错误的字符串        */
             INT    iVarStart = 0;                                      /*  当前操作是否在变量名中      */
    REGISTER INT    i = 0;                                              /*  变量名拷贝索引              */
    
             INT    iTransCharMarks = 0;                                /*  转义字符                    */
             INT    iQuotationMarks = 0;                                /*  是否在分号内                */
    
             INT    iNeedClearTranMark = 0;                             /*  是否需要清除转义标志        */

    for (; *pcTemp != '\0'; pcTemp++) {
        if (*pcTemp == '\\') {                                          /*  是否为转义字符              */
            iTransCharMarks = (iTransCharMarks > 0) ? 0 : 1;            /*  确定前端是否为转义字符      */
        } else {
            iNeedClearTranMark = 1;                                     /*  执行完后需要清除转义标志    */
        }
        
        if ((*pcTemp == '\"') && (iTransCharMarks == 0)) {              /*  非转义字符的 "              */
            iQuotationMarks = (iQuotationMarks > 0) ? 0 : 1;
        }
        
        if (*pcTemp != '$') {                                           /*  不是变量起始符              */
            if (iVarStart) {                                            /*  属于变量名部分              */
                if (__tshellIsSeparator(*pcTemp) == LW_FALSE) {         /*  变量没有结束                */
                    cValName[i++] = *pcTemp;
                    if (i >= LW_CFG_SHELL_MAX_VARNAMELEN) {             /*  变量名超长                  */
                        _ErrorHandle(ERROR_TSHELL_EVAR);
                        return  (ERROR_TSHELL_EVAR);                    /*  错误                        */
                    }
                } else {                                                /*  变量记录结束                */
                    CHAR    cVarValue[NAME_MAX + 3] = "\"";             /*  变量值                      */
                    
                    if (i < 1) {                                        /*  变量名长度错误              */
                        _ErrorHandle(ERROR_TSHELL_EVAR);
                        return  (ERROR_TSHELL_EVAR);                    /*  错误                        */
                    }
                    cValName[i] = PX_EOS;                               /*  变量名结束                  */
                    
                    if (lib_strcmp(cValName, "?") == 0) {               /*  获得最后一次错误            */
                        sprintf(cLastError, "%d", 
                                __TTINY_SHELL_GET_ERROR(API_ThreadTcbSelf()));
                        iVarValueLen = (INT)lib_strlen(cLastError);
                        pcVarValue   = cLastError;
                    
                    } else {
                        iVarValueLen = __tshellVarGetRt(cValName, 
                                                        &cVarValue[1],
                                                        (NAME_MAX + 1));/*  获得变量的值                */
                        if (iVarValueLen <= 0) {
                            return  (ERROR_TSHELL_EVAR);                /*  错误                        */
                        }
                        iVarValueLen++;
                        cVarValue[iVarValueLen] = '"';                  /*  使用双引号                  */
                        iVarValueLen++;
                        
                        pcVarValue = cVarValue;
                    }
                    
                    if (iVarValueLen + iTotalLen >=                     /*  需要计算两个引号            */
                        LW_CFG_SHELL_MAX_COMMANDLEN) {                  /*  超过最长长度                */
                        _ErrorHandle(ERROR_TSHELL_EPARAM);
                        return  (ERROR_TSHELL_EPARAM);
                    }
                    lib_memcpy(pcCmdOut, pcVarValue, iVarValueLen);     /*  替换                        */
                    pcCmdOut  += iVarValueLen;
                    iTotalLen += iVarValueLen;
                    
                    if (*pcTemp == '}') {                               /*  有大括号包围                */
                        pcTemp = __tshellStrSkipRightBigBracket(pcTemp);/*  滤除右边大括号              */
                    }
                    pcTemp--;                                           /*  for 循环会执行一次 ++       */
                
                    iVarStart = 0;                                      /*  检测完一个变量了            */
                }
            } else {
                if (iTotalLen < LW_CFG_SHELL_MAX_COMMANDLEN) {
                    *pcCmdOut++ = *pcTemp;                              /*  拷贝                        */
                    iTotalLen++;                                        /*  命令总长++                  */
                } else {
                    _ErrorHandle(ERROR_TSHELL_EPARAM);
                    return  (ERROR_TSHELL_EPARAM);
                }
            }
        } else {
            if (iQuotationMarks == 0) {                                 /*  不在引号内                  */
                iVarStart = 1;
                i         = 0;                                          /*  重头开始记录变量            */
                pcTemp++;                                               /*  过滤 $ 符号                 */
                if (*pcTemp == '{') {                                   /*  有大括号包围                */
                    pcTemp = __tshellStrSkipLeftBigBracket(pcTemp);     /*  滤除左边大括号              */
                }
                pcTemp--;                                               /*  for 循环将进行 ++ 操作      */
            } else {
                if (iTotalLen < LW_CFG_SHELL_MAX_COMMANDLEN) {          /*  将引号内 $ 当作普通字符     */
                    *pcCmdOut++ = *pcTemp;                              /*  拷贝                        */
                    iTotalLen++;                                        /*  命令总长++                  */
                } else {
                    _ErrorHandle(ERROR_TSHELL_EPARAM);
                    return  (ERROR_TSHELL_EPARAM);
                }
            }
        }
        
        if (iNeedClearTranMark) {                                       /*  是否需要清除转义标志        */
            iNeedClearTranMark = 0;
            iTransCharMarks    = 0;                                     /*  清除转义标志                */
        }
    }
    
    if (iVarStart) {                                                    /*  最后一个变量替换还没有完成  */
        CHAR    cVarValue[NAME_MAX + 3] = "\"";                         /*  变量值                      */
        
        if (i < 1) {                                                    /*  变量名长度错误              */
            _ErrorHandle(ERROR_TSHELL_EVAR);
            return  (ERROR_TSHELL_EVAR);                                /*  错误                        */
        }
        cValName[i] = PX_EOS;                                           /*  变量名结束                  */
        
        if (lib_strcmp(cValName, "?") == 0) {                           /*  获得最后一次错误            */
            sprintf(cLastError, "%d", 
                    __TTINY_SHELL_GET_ERROR(API_ThreadTcbSelf()));
            iVarValueLen = (INT)lib_strlen(cLastError);
            pcVarValue   = cLastError;
        
        } else {
            iVarValueLen = __tshellVarGetRt(cValName, 
                                            &cVarValue[1],
                                            (NAME_MAX + 1));            /*  获得变量的值                */
            if (iVarValueLen <= 0) {
                return  (ERROR_TSHELL_EVAR);                            /*  错误                        */
            }
            iVarValueLen++;
            cVarValue[iVarValueLen] = '"';                              /*  使用双引号                  */
            iVarValueLen++;
        
            pcVarValue = cVarValue;
        }
        
        if (iVarValueLen + iTotalLen >= 
            LW_CFG_SHELL_MAX_COMMANDLEN) {                              /*  超过最长长度                */
            _ErrorHandle(ERROR_TSHELL_EPARAM);
            return  (ERROR_TSHELL_EPARAM);
        }
        lib_memcpy(pcCmdOut, pcVarValue, iVarValueLen);                 /*  替换                        */
        pcCmdOut  += iVarValueLen;
        iTotalLen += iVarValueLen;
    }
    
    *pcCmdOut = PX_EOS;                                                 /*  字符串结束                  */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellStrDelCRLF
** 功能描述: 删除命令字结尾的 CR 与 LF 字符
** 输　入  : pcCmd               shell 命令
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __tshellStrDelCRLF (CPCHAR  pcCmd)
{
    REGISTER PCHAR  pcCommand = (PCHAR)pcCmd;

    while (*pcCommand != PX_EOS) {                                      /*  找到字符串的尾部            */
        pcCommand++;
    }
    pcCommand--;                                                        /*  找到最后一个字符            */

    while (*pcCommand == '\r' || 
           *pcCommand == '\n' ||
           *pcCommand == ' ') {                                         /*  结尾处的无用字符            */
        *pcCommand = PX_EOS;                                            /*  删除无用字节                */
        pcCommand--;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellStrFormat
** 功能描述: 整理一个接收到的字符串, 将所有多余的空格去掉.
** 输　入  : pcCmd               shell 命令
**           pcCmdOut            替换后的命令串
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __tshellStrFormat (CPCHAR  pcCmd, PCHAR  pcCmdOut)
{
             INT      iTransCharMarks = 0;                              /*  转义字符                    */
             INT      iQuotationMarks = 0;                              /*  引号数量                    */

             INT      iNeedClearTranMark = 0;

    REGISTER PCHAR    pcCommand  = (PCHAR)pcCmd;
    REGISTER PCHAR    pcInbuffer = pcCmdOut;

             INT      iStartSpace = 0;

    for (; *pcCommand != '\0'; pcCommand++) {                           /*  去掉多于一个的空格          */
        if (*pcCommand == '\\') {                                       /*  是否为转义字符              */
            iTransCharMarks = (iTransCharMarks > 0) ? 0 : 1;            /*  确定前端是否为转义字符      */
        } else {
            iNeedClearTranMark = 1;                                     /*  执行完后需要清除转义标志    */
        }
        
        if ((*pcCommand == '\"') && (iTransCharMarks == 0)) {           /*  非转义 "                    */
            iQuotationMarks = (iQuotationMarks > 0) ? 0 : 1;            /*  测算 " 的个数               */
            *pcInbuffer++ = *pcCommand;                                 /*  不是空格,直接拷贝           */
            iStartSpace   = 0;                                          /*  取消空格计数                */
        } else {
            if ((*pcCommand != ' ') ||
                iQuotationMarks) {                                      /*  不是空格或者进入引号区      */
                *pcInbuffer++ = *pcCommand;                             /*  直接拷贝                    */
                iStartSpace = 0;
            } else {
                if (iStartSpace == 0) {                                 /*  连续空格,仅拷贝一个         */
                    *pcInbuffer++ = *pcCommand;
                }
                iStartSpace = 1;
            }
        }
        
        if (iNeedClearTranMark) {                                       /*  是否需要清除转义标志        */
            iNeedClearTranMark = 0;
            iTransCharMarks    = 0;                                     /*  清除转义标志                */
        }
    }

    *pcInbuffer = PX_EOS;                                               /*  拷贝结束                    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellStrKeyword
** 功能描述: 从 shell 命令中查找关键字
** 输　入  : pcCmd               shell 命令
**           pcBuffer            关键字缓冲区
**           ppcParam            指向参数起始序列
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __tshellStrKeyword (CPCHAR  pcCmd, PCHAR  pcBuffer, PCHAR  *ppcParam)
{
    REGISTER PCHAR   pcCommand  = (PCHAR)pcCmd;
    REGISTER PCHAR   pcInbuffer = pcBuffer;
             INT     iKeywordLen = 0;

    for (; *pcCommand != '\0'; pcCommand++) {                           /*  拷贝关键字                  */
        if (*pcCommand != ' ') {                                        /*  不是空格                    */
            *pcInbuffer++ = *pcCommand;
            iKeywordLen++;
            if (iKeywordLen >= LW_CFG_SHELL_MAX_KEYWORDLEN) {           /*  关键字超长                  */
                _ErrorHandle(ERROR_TSHELL_EKEYWORD);
                return  (ERROR_TSHELL_EKEYWORD);
            }
        } else {
            break;
        }
    }

    *pcInbuffer = PX_EOS;                                               /*  结束                        */

    if (*pcCommand != PX_EOS) {                                         /*  带有参数                    */
        *ppcParam = pcCommand + 1;                                      /*  指向参数起始地址            */
    } else {
        *ppcParam = LW_NULL;                                            /*  没有参数                    */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellStrGetToken
** 功能描述: 从 shell 命令中查找一个参数组
** 输　入  : pcCmd               shell 命令
**           ppcNext             下一个参数的起始位置
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __tshellStrGetToken (CPCHAR  pcCmd, PCHAR  *ppcNext)
{
    REGISTER PCHAR  pcCommand       = (PCHAR)pcCmd;
    
             INT    iQuotationMarks = 0;                                /*  引号数量                    */
             INT    iTransCharMarks = 0;                                /*  转义字符                    */
    
             INT    iNeedClearTranMark = 0;
    
    
    if (!pcCmd) {                                                       /*  参数错误                    */
        _ErrorHandle(ERROR_TSHELL_EPARAM);
        return  (ERROR_TSHELL_EPARAM);
    }
    
    for ( ; 
         *pcCommand != '\0'; 
          pcCommand++) {                                                /*  循环扫描                    */
          
        if (*pcCommand == '\\') {                                       /*  是否为转义字符              */
            iTransCharMarks = (iTransCharMarks > 0) ? 0 : 1;            /*  确定前端是否为转义字符      */
        } else {
            iNeedClearTranMark = 1;                                     /*  执行完后需要清除转义标志    */
        }
        
        if ((*pcCommand == '\"') && (iTransCharMarks == 0)) {           /*  非转义 "                    */
            iQuotationMarks = (iQuotationMarks > 0) ? 0 : 1;            /*  测算非转义 " 的个数         */
        } else if (*pcCommand == ' ') {                                 /*  为空格                      */
            if (iQuotationMarks == 0) {                                 /*  不存在没有配对的空格        */
                break;
            }
        }
        
        if (iNeedClearTranMark) {                                       /*  是否需要清除转义标志        */
            iNeedClearTranMark = 0;
            iTransCharMarks    = 0;                                     /*  清除转义标志                */
        }
    }
    
    if (*pcCommand == PX_EOS) {
        *ppcNext = LW_NULL;                                             /*  没有下一个参数了            */
    } else {
        *pcCommand = PX_EOS;                                            /*  一个参数结束                */
        pcCommand++;
        *ppcNext   = pcCommand;                                         /*  下一个参数地址              */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellStrDelTransChar
** 功能描述: 删除转义字符和非转义的"
** 输　入  : pcCmd               shell 命令
**           pcCmdOut            替换后的命令串
** 输　出  : 错误代码.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ULONG  __tshellStrDelTransChar (CPCHAR  pcCmd, PCHAR  pcCmdOut)
{
    REGISTER PCHAR  pcCommand       = (PCHAR)pcCmd;
    REGISTER PCHAR  pcOut           = pcCmdOut;
             INT    iTransCharMarks = 0;                                /*  转义字符                    */
    
    for (; *pcCommand != '\0'; pcCommand++) {                           /*  拷贝关键字                  */
        if (*pcCommand == '\\') {                                       /*  是否为转义字符              */
            if (iTransCharMarks) {
                iTransCharMarks = 0;
                *pcOut++ = *pcCommand;                                  /*  拷贝一个 \                  */
            } else {
                iTransCharMarks = 1;                                    /*  记录转义标志                */
            }
        } else {
            if ((*pcCommand == '\"') && (iTransCharMarks == 0)) {       /*  非转义 "                    */
                /*
                 *  这里忽略非转义字符的引号
                 */
            } else if ((*pcCommand == 'r') && (iTransCharMarks == 1)) { /*  \r 转义                     */
                *pcOut++ = 0x0D;
            } else if ((*pcCommand == 'n') && (iTransCharMarks == 1)) { /*  \n 转义                     */
                *pcOut++ = 0x0A;
            } else {
                *pcOut++ = *pcCommand;                                  /*  拷贝一个其他字符            */
            }
            iTransCharMarks = 0;                                        /*  清除转义标志                */
        }
    }
    
    *pcOut = PX_EOS;                                                    /*  一个参数结束                */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

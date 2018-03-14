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
** 文   件   名: lwip_iac.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 07 月 13 日
**
** 描        述: lwip iac 命令工具.

** BUG:
2013.06.09  使用 -fsigned-char 时, 出现错误, 这里比较时需要使用无符号比较
2014.10.22  修正 __inetIacFilter() 判断错误.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_TELNET_EN > 0)
#include "lwip_iac.h"
/*********************************************************************************************************
** 函数名称: __inetIacMakeFrame
** 功能描述: 创建一个 IAC 数据包 (当前功能较为简单, 未来可能会增强)
** 输　入  : iCommand      命令
**           iOpt          选项标识, PX_ERROR 表示没有选项标识
**           pcBuffer      缓冲区 (至少 3 个字节)
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __inetIacMakeFrame (INT  iCommand, INT  iOpt, PCHAR  pcBuffer)
{
    if (pcBuffer) {
        pcBuffer[0] = (CHAR)LW_IAC_IAC;
        pcBuffer[1] = (CHAR)iCommand;
        if (iOpt != PX_ERROR) {                                         /*   PX_ERROR 表示没有选项标识  */
            pcBuffer[2] = (CHAR)iOpt;
            return  (3);
        } else {
            return  (2);
        }
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __inetIacSend
** 功能描述: 发送一个 IAC 数据包
** 输　入  : iFd           文件
**           iCommand      命令
**           iOpt          选项标识
** 输　出  : ERROR or send len
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __inetIacSend (INT  iFd, INT  iCommand, INT  iOpt)
{
    CHAR    cSendBuffer[3];
    INT     iLen;
    
    iLen = __inetIacMakeFrame(iCommand, iOpt, cSendBuffer);
    if (iLen < 0) {
        return  (PX_ERROR);
    }
    
    return  ((INT)write(iFd, cSendBuffer, (size_t)iLen));
}
/*********************************************************************************************************
** 函数名称: __inetIacFilter
** 功能描述: IAC 数据包接收滤波器
** 输　入  : pcBuffer          接收缓冲
**           iLen              缓冲长度
**           piPtyLen          pty 需要接收的数据长度
**           piProcessLen      本轮处理的字节数 (由于本次接收的 IAC 字段不完整, 
                                                 有可能会剩余几个字节不处理)
**           pfunc             IAC 回调函数
**           pvArg             IAC 回调参数
** 输　出  : pty data buffer
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PCHAR  __inetIacFilter (PCHAR        pcBuffer, 
                        INT          iLen, 
                        INT         *piPtyLen, 
                        INT         *piProcessLen, 
                        FUNCPTR      pfunc,
                        PVOID        pvArg)
{
    REGISTER PUCHAR     pucTemp = (PUCHAR)pcBuffer;
    REGISTER PUCHAR     pucEnd  = (PUCHAR)(pcBuffer + iLen);
    REGISTER PUCHAR     pucOut  = (PUCHAR)pcBuffer;                     /*  需要返回给 pty 的数据       */
    
    for (; pucTemp < pucEnd; pucTemp++) {
        if (*pucTemp != LW_IAC_IAC) {                                   /*  非 IAC 转义                 */
            *pucOut++ = *pucTemp;
        } else {
            if ((pucTemp + 2) <= pucEnd) {
                if (pfunc) {
                    pucTemp += pfunc(pvArg, pucTemp, pucEnd);           /*  回调                        */
                    pucTemp--;                                          /*  调和for循环后面的++         */
                } else {
                    pucTemp += 1;                                       /*  忽略 IAC 字段               */
                }
            } else {
                break;                                                  /*  最后的 IAC 不完整           */
            }
        }
    }
    
    if (piPtyLen) {
        *piPtyLen = (INT)(pucOut - (PUCHAR)pcBuffer);                   /*  pty 需要处理的长度          */
    }
    if (piProcessLen) {
        *piProcessLen = (INT)(pucTemp - (PUCHAR)pcBuffer);              /*  本轮处理长度                */
    }
    
    return  (pcBuffer);                                                 /*  缓冲区首地址                */
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_TELNET_EN > 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: lwip_shell6.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 09 月 27 日
**
** 描        述: lwip ipv6 shell 命令.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_SHELL_EN > 0)
#include "socket.h"
#include "net/if.h"
#include "netinet6/in6.h"
/*********************************************************************************************************
  ipv6 帮助信息
*********************************************************************************************************/
static const CHAR   _G_cIpv6Help[] = {
    "set/get IPv6 status\n"
    "address,               [ifname [address%prefixlen]] set/add an ipv6 address for given if\n"
    "noaddress              [ifname [address%prefixlen]] delete an ipv6 address for given if\n"
};
/*********************************************************************************************************
** 函数名称: __ifreq6Init
** 功能描述: 通过参数初始化 in6_ifreq 结构
** 输　入  : pifeq6        需要初始化的结构
**           pcIf          网络接口
**           pcIpv6        IPv6 地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __ifreq6Init (struct in6_ifreq *pifeq6, CPCHAR pcIf, PCHAR pcIpv6)
{
    PCHAR pcDiv;

    pifeq6->ifr6_ifindex = if_nametoindex(pcIf);
    
    pcDiv = lib_strchr(pcIpv6, '%');
    if (pcDiv) {
        *pcDiv = PX_EOS;
        pcDiv++;
        inet6_aton(pcIpv6, &pifeq6->ifr6_addr_array->ifr6a_addr);
        pifeq6->ifr6_addr_array->ifr6a_prefixlen = lib_atoi(pcDiv);
    
    } else {
        inet6_aton(pcIpv6, &pifeq6->ifr6_addr_array->ifr6a_addr);
        pifeq6->ifr6_addr_array->ifr6a_prefixlen = 64;                  /*  default prefixlen           */
    }
}
/*********************************************************************************************************
** 函数名称: __tshellIpv6Address
** 功能描述: 系统命令 "ipv6 address"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellIpv6Address (INT  iArgC, PCHAR  *ppcArgV)
{
    struct in6_ifreq    ifeq6;
    struct in6_ifr_addr ifraddr6;
    
    INT iSock;
    
    if (iArgC < 4) {
        printf("%s", _G_cIpv6Help);
        return  (ERROR_NONE);
    }
    
    ifeq6.ifr6_len = sizeof(struct in6_ifr_addr);
    ifeq6.ifr6_addr_array = &ifraddr6;
    
    __ifreq6Init(&ifeq6, ppcArgV[2], ppcArgV[3]);
    
    iSock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s\n", lib_strerror(errno));
        return  (ERROR_NONE);
    }
    
    if (ioctl(iSock, SIOCSIFADDR6, &ifeq6)) {
        INT   iErrNo = errno;
        close(iSock);
        fprintf(stderr, "can not set/add ipv6 address error: %s\n", lib_strerror(iErrNo));
        return  (ERROR_NONE);
    }
    
    close(iSock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellIpv6Noaddress
** 功能描述: 系统命令 "ipv6 noaddress"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellIpv6Noaddress (INT  iArgC, PCHAR  *ppcArgV)
{
    struct in6_ifreq    ifeq6;
    struct in6_ifr_addr ifraddr6;
    
    INT iSock;
    
    if (iArgC < 4) {
        printf("%s", _G_cIpv6Help);
        return  (ERROR_NONE);
    }
    
    ifeq6.ifr6_len = sizeof(struct in6_ifr_addr);
    ifeq6.ifr6_addr_array = &ifraddr6;
    
    __ifreq6Init(&ifeq6, ppcArgV[2], ppcArgV[3]);
    
    iSock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s\n", lib_strerror(errno));
        return  (ERROR_NONE);
    }
    
    if (ioctl(iSock, SIOCDIFADDR6, &ifeq6)) {
        INT   iErrNo = errno;
        close(iSock);
        fprintf(stderr, "can not delete ipv6 address error: %s\n", lib_strerror(iErrNo));
        return  (ERROR_NONE);
    }
    
    close(iSock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellIpv6
** 功能描述: 系统命令 "ipv6"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellIpv6 (INT  iArgC, PCHAR  *ppcArgV)
{
    if (iArgC < 2) {
        printf("%s", _G_cIpv6Help);
        return  (ERROR_NONE);
    }
    
    if (lib_strcmp(ppcArgV[1], "address") == 0) {                       /*  设置 ipv6 地址              */
        return  (__tshellIpv6Address(iArgC, ppcArgV));
    
    } else if (lib_strcmp(ppcArgV[1], "noaddress") == 0) {              /*  删除 ipv6 地址              */
        return  (__tshellIpv6Noaddress(iArgC, ppcArgV));
    
    } else {
        printf("%s", _G_cIpv6Help);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: __tshellNet6Init
** 功能描述: 注册 IPv6 专属命令
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __tshellNet6Init (VOID)
{
    API_TShellKeywordAdd("ipv6", __tshellIpv6);
    API_TShellFormatAdd("ipv6",  " ...");
    API_TShellHelpAdd("ipv6",    _G_cIpv6Help);
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

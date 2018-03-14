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
** 文   件   名: lwip_netstat.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 08 月 23 日
**
** 描        述: lwip netstat 命令实现函数.

** BUG:
2013.09.11  网络接口打印直接访问 /proc/net/dev 文件.
2014.05.06  tcp listen 打印, 如果是 IPv6 则打印是否只接受 IPv6 链接请求.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_SHELL_EN > 0)
#include "lwip/sockets.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/stats.h"
/*********************************************************************************************************
  TCP
*********************************************************************************************************/
#include "lwip/tcp.h"
#include "lwip/priv/tcp_priv.h"
extern struct tcp_pcb           *tcp_active_pcbs;
extern union  tcp_listen_pcbs_t  tcp_listen_pcbs;
extern struct tcp_pcb           *tcp_tw_pcbs;
/*********************************************************************************************************
  UDP
*********************************************************************************************************/
#include "lwip/udp.h"
/*********************************************************************************************************
  RAW
*********************************************************************************************************/
#include "lwip/raw.h"
/*********************************************************************************************************
  IGMP
*********************************************************************************************************/
#include "lwip/igmp.h"
#include "lwip/mld6.h"
/*********************************************************************************************************
  宏操作
*********************************************************************************************************/
#define __NETSTAT_INC_UNIX(type)    ((type == 0) || (type == 1))
#define __NETSTAT_INC_IPV4(type)    ((type == 0) || (type == 2))
#define __NETSTAT_INC_IPV6(type)    ((type == 0) || (type == 3))
/*********************************************************************************************************
** 函数名称: __tshellNetstatIf
** 功能描述: 系统命令 "netstat -i"
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __tshellNetstatIf (VOID)
{
    INT     iFd;
    CHAR    cBuffer[512];
    ssize_t sstNum;
    
    iFd = open("/proc/net/dev", O_RDONLY);
    if (iFd < 0) {
        fprintf(stderr, "can not open /proc/net/dev : %s\n", lib_strerror(errno));
        return;
    }
    
    do {
        sstNum = read(iFd, cBuffer, sizeof(cBuffer));
        if (sstNum > 0) {
            write(STDOUT_FILENO, cBuffer, (size_t)sstNum);
        }
    } while (sstNum > 0);
    
    close(iFd);
}
/*********************************************************************************************************
** 函数名称: __GroupPrint
** 功能描述: 打印网络 group 文件
** 输　入  : group         group 控制块
**           netif         网络接口
**           pcBuffer      缓冲
**           stTotalSize   缓冲区大小
**           pstOft        当前偏移量
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LWIP_IGMP > 0

static VOID  __GroupPrint (struct igmp_group *group, struct netif *netif,
                           PCHAR  pcBuffer, size_t  stTotalSize, size_t *pstOft)
{
    CHAR    cBuffer[INET_ADDRSTRLEN];
    CHAR    cIfName[NETIF_NAMESIZE];

    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%-4s %-15s %d\n",
                       netif_get_name(netif, cIfName),
                       ip4addr_ntoa_r(&group->group_address, cBuffer, sizeof(cBuffer)),
                       (u32_t)group->use);
}
/*********************************************************************************************************
** 函数名称: __Group6Print
** 功能描述: 打印网络 group6 文件
** 输　入  : mld_group     group6 控制块
**           netif         网络接口
**           pcBuffer      缓冲
**           stTotalSize   缓冲区大小
**           pstOft        当前偏移量
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __Group6Print (struct mld_group *mld_group, struct netif *netif,
                            PCHAR  pcBuffer, size_t  stTotalSize, size_t *pstOft)
{
    CHAR    cBuffer[INET6_ADDRSTRLEN];
    CHAR    cIfName[NETIF_NAMESIZE];
    
    *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                       "%-4s %-39s %d\n",
                       netif_get_name(netif, cIfName),
                       ip6addr_ntoa_r(&mld_group->group_address, cBuffer, sizeof(cBuffer)),
                       (u32_t)mld_group->use);
}
/*********************************************************************************************************
** 函数名称: __tshellNetstatGroup
** 功能描述: 系统命令 "netstat -g"
** 输　入  : iNetType      网络类型
** 输　出  : ERROR_NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __tshellNetstatGroup (INT  iNetType)
{
    const CHAR           cIgmpInfoHdr[] = 
    "DEV  GROUP           COUNT\n";
    
    const CHAR           cIgmp6InfoHdr[] = 
    "\nDEV  GROUP6                                  COUNT\n";

    size_t               stNeedBufferSize = 0;
    struct igmp_group   *group;
    struct mld_group    *mld_group;
    struct netif        *netif;
    PCHAR                pcPrintBuf;
    size_t               stRealSize = 0;
    
    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType)) {
        NETIF_FOREACH(netif) {
            for (group = netif_igmp_data(netif); group != NULL; group = group->next) {
                stNeedBufferSize += 128;
            }
        }
    }
    if (__NETSTAT_INC_IPV6(iNetType)) {
        NETIF_FOREACH(netif) {
            for (mld_group = netif_mld6_data(netif); mld_group != NULL; mld_group = mld_group->next) {
                stNeedBufferSize += 256;
            }
        }
    }
    UNLOCK_TCPIP_CORE();
    
    stNeedBufferSize += sizeof(cIgmpInfoHdr) + sizeof(cIgmp6InfoHdr);
    
    pcPrintBuf = (PCHAR)__SHEAP_ALLOC(stNeedBufferSize);
    if (!pcPrintBuf) {
        fprintf(stderr, "no memory!\n");
        _ErrorHandle(ENOMEM);
        return;
    }
    
    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType)) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize, cIgmpInfoHdr);
        NETIF_FOREACH(netif) {
            for (group = netif_igmp_data(netif); group != NULL; group = group->next) {
                __GroupPrint(group, netif, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
    if (__NETSTAT_INC_IPV6(iNetType)) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize, cIgmp6InfoHdr);
        NETIF_FOREACH(netif) {
            for (mld_group = netif_mld6_data(netif); mld_group != NULL; mld_group = mld_group->next) {
                __Group6Print(mld_group, netif, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
    UNLOCK_TCPIP_CORE();
    
    printf("%s\n", pcPrintBuf);
    
    __SHEAP_FREE(pcPrintBuf);
}

#endif                                                                  /*  LWIP_IGMP > 0               */
/*********************************************************************************************************
** 函数名称: __tshellNetstatStat
** 功能描述: 系统命令 "netstat -s"
** 输　入  : NONE
** 输　出  : ERROR_NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __tshellNetstatStat (VOID)
{
    INT     iFd = open("/proc/net/tcpip_stat", O_RDONLY);
    CHAR    cBuffer[1024];
    ssize_t sstNum;
    
    fflush(stdout);
    
    if (iFd < 0) {
        fprintf(stderr, "can not open /proc/net/tcpip_stat : %s\n", lib_strerror(errno));
        return;
    }
    
    do {
        sstNum = read(iFd, cBuffer, sizeof(cBuffer));
        if (sstNum > 0) {
            write(STDOUT_FILENO, cBuffer, (size_t)sstNum);
        }
    } while (sstNum > 0);
    
    close(iFd);
}
/*********************************************************************************************************
** 函数名称: __RawGetProto
** 功能描述: 获得协议信息
** 输　入  : state     状态
** 输　出  : 状态字串
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static CPCHAR  __RawGetProto (u8_t  proto)
{
    switch (proto) {
    
    case IPPROTO_IP:
        return  ("ip");
        
    case IPPROTO_IPV6:
        return  ("ipv6");
        
    case IPPROTO_ICMP:
        return  ("icmp");
        
    case IPPROTO_TCP:
        return  ("tcp");
        
    case IPPROTO_UDP:
        return  ("udp");
        
    case IPPROTO_UDPLITE:
        return  ("udplite");
        
    default:
        return  ("unknown");
    }
}
/*********************************************************************************************************
** 函数名称: __RawPrint
** 功能描述: 打印网络 raw 文件
** 输　入  : pcb           raw 控制块
**           pcBuffer      缓冲
**           stTotalSize   缓冲区大小
**           pstOft        当前偏移量
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __RawPrint (struct raw_pcb  *pcb, PCHAR  pcBuffer, 
                         size_t  stTotalSize, size_t *pstOft)
{
    CHAR    cBuffer1[INET6_ADDRSTRLEN];
    CHAR    cBuffer2[INET6_ADDRSTRLEN];
    
    if (IP_IS_V4_VAL(pcb->local_ip)) {
        *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                           "%-15s %-15s %s\n",
                           (ip_2_ip4(&pcb->local_ip)->addr == IPADDR_ANY) ?
                           "*" : ip4addr_ntoa_r(ip_2_ip4(&pcb->local_ip), cBuffer1, sizeof(cBuffer1)),
                           (ip_2_ip4(&pcb->remote_ip)->addr == IPADDR_ANY) ?
                           "*" : ip4addr_ntoa_r(ip_2_ip4(&pcb->remote_ip), cBuffer2, sizeof(cBuffer2)),
                           __RawGetProto(pcb->protocol));
    } else {
        *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                           "%-39s %-39s %s\n",
                           ip6_addr_isany(ip_2_ip6(&pcb->local_ip)) ? 
                           "*" : ip6addr_ntoa_r(ip_2_ip6(&pcb->local_ip), cBuffer1, sizeof(cBuffer1)),
                           ip6_addr_isany(ip_2_ip6(&pcb->remote_ip)) ? 
                           "*" : ip6addr_ntoa_r(ip_2_ip6(&pcb->remote_ip), cBuffer2, sizeof(cBuffer2)),
                           __RawGetProto(pcb->protocol));
    }
}
/*********************************************************************************************************
** 函数名称: __tshellNetstatRaw
** 功能描述: 系统命令 "netstat -w"
** 输　入  : iNetType      网络类型
** 输　出  : ERROR_NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __tshellNetstatRaw (INT  iNetType)
{
    const CHAR      cRawInfoHdr[] = 
    "LOCAL           REMOTE          PROTO\n";
    
    const CHAR      cRaw6InfoHdr[] = 
    "\nLOCAL6                                  REMOTE6                                 PROTO\n";
    
    struct raw_pcb  *pcb;
    PCHAR            pcPrintBuf;
    size_t           stRealSize = 0;
    size_t           stNeedBufferSize = 0;
    BOOL             b4 = LW_FALSE, b6 = LW_FALSE;
    
    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType)) {
        for (pcb = raw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip)) {
                stNeedBufferSize += 128;
                b4 = LW_TRUE;
            }
        }
    }
    if (__NETSTAT_INC_IPV6(iNetType)) {
        for (pcb = raw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip)) {
                stNeedBufferSize += 192;
                b6 = LW_TRUE;
            }
        }
    }
    UNLOCK_TCPIP_CORE();
    
    if (stNeedBufferSize == 0) {
        return;
    }
    
    stNeedBufferSize += sizeof(cRawInfoHdr) + sizeof(cRaw6InfoHdr);
    
    pcPrintBuf = (PCHAR)__SHEAP_ALLOC(stNeedBufferSize);
    if (!pcPrintBuf) {
        fprintf(stderr, "no memory!\n");
        _ErrorHandle(ENOMEM);
        return;
    }
    
    printf("--RAW--:\n");
    
    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType) && b4) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize, cRawInfoHdr);
        for (pcb = raw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip)) {
                __RawPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
    if (__NETSTAT_INC_IPV6(iNetType) && b6) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize, cRaw6InfoHdr);
        for (pcb = raw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip)) {
                __RawPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
    UNLOCK_TCPIP_CORE();
    
    printf("%s\n", pcPrintBuf);
    
    __SHEAP_FREE(pcPrintBuf);
}
/*********************************************************************************************************
** 函数名称: __ProtoAddrBuild
** 功能描述: 创建协议地址字段
** 输　入  : addr      地址
**           usPort    端口
**           pcBuffer  缓冲区
**           stSize    缓冲区大小
** 输　出  : 结果
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PCHAR  __ProtoAddrBuild (ip_addr_t  *addr, u16_t usPort, PCHAR  pcBuffer, size_t  stSize)
{
    CHAR    cBuffer[INET6_ADDRSTRLEN];

    if (IP_IS_V4(addr)) {
        bnprintf(pcBuffer, stSize, 0, "%s:%d", 
                 (ip_2_ip4(addr)->addr == IPADDR_ANY) ?
                 "*" : ip4addr_ntoa_r(ip_2_ip4(addr), cBuffer, sizeof(cBuffer)),
                 usPort);
    } else {
        bnprintf(pcBuffer, stSize, 0, "%s:%d", 
                 ip6_addr_isany(ip_2_ip6(addr)) ? 
                 "*" : ip6addr_ntoa_r(ip_2_ip6(addr), cBuffer, sizeof(cBuffer)),
                 usPort);
    }
    
    return  (pcBuffer);
}
/*********************************************************************************************************
** 函数名称: __TcpGetStat
** 功能描述: 获得 tcp 状态信息
** 输　入  : state     状态
** 输　出  : 状态字串
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static CPCHAR  __TcpGetStat (u8_t  state)
{
    static const PCHAR cTcpState[] = {
        "close",
        "listen",
        "syn_send",
        "syn_rcvd",
        "estab",
        "fin_w_1",
        "fin_w_2",
        "close_w",
        "closeing",
        "last_ack",
        "time_w",
        "unknown"
    };
    
    if (state > 11) {
        state = 11;
    }
    
    return  (cTcpState[state]);
}
/*********************************************************************************************************
** 函数名称: __TcpPrint
** 功能描述: 打印网络 tcp 文件
** 输　入  : pcb           tcp 控制块
**           pcBuffer      缓冲
**           stTotalSize   缓冲区大小
**           pstOft        当前偏移量
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __TcpPrint (struct tcp_pcb *pcb, PCHAR  pcBuffer, 
                         size_t  stTotalSize, size_t *pstOft)
{
    CHAR    cBuffer1[INET6_ADDRSTRLEN];
    CHAR    cBuffer2[INET6_ADDRSTRLEN];

    if (IP_IS_V4_VAL(pcb->local_ip)) {
        if (pcb->state == LISTEN) {
            *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                               "%-20s %-20s %-8s %7d %7d %7d\n",
                               __ProtoAddrBuild(&pcb->local_ip, pcb->local_port, 
                                                cBuffer1, sizeof(cBuffer1)),
                               "*:*",
                               __TcpGetStat((u8_t)pcb->state),
                               0, 0, 0);
        } else {
            *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                               "%-20s %-20s %-8s %7d %7d %7d\n",
                               __ProtoAddrBuild(&pcb->local_ip, pcb->local_port, 
                                                cBuffer1, sizeof(cBuffer1)),
                               __ProtoAddrBuild(&pcb->remote_ip, pcb->remote_port, 
                                                cBuffer2, sizeof(cBuffer2)),
                               __TcpGetStat((u8_t)pcb->state),
                               (u32_t)pcb->nrtx, (u32_t)pcb->rcv_wnd, (u32_t)pcb->snd_wnd);
        }
    } else {
        if (pcb->state == LISTEN) {
            *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                               "%-44s %-44s %-8s %-9s %7d %7d %7d\n",
                               __ProtoAddrBuild(&pcb->local_ip, pcb->local_port, 
                                                cBuffer1, sizeof(cBuffer1)),
                               "*:*",
                               __TcpGetStat((u8_t)pcb->state),
                               IP_IS_V6_VAL(pcb->local_ip) ? "YES" : "NO",
                               0, 0, 0);
        } else {
            *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                               "%-44s %-44s %-8s %7d %7d %7d\n",
                               __ProtoAddrBuild(&pcb->local_ip, pcb->local_port, 
                                                cBuffer1, sizeof(cBuffer1)),
                               __ProtoAddrBuild(&pcb->remote_ip, pcb->remote_port, 
                                                cBuffer2, sizeof(cBuffer2)),
                               __TcpGetStat((u8_t)pcb->state),
                               (u32_t)pcb->nrtx, (u32_t)pcb->rcv_wnd, (u32_t)pcb->snd_wnd);
        }
    }
}
/*********************************************************************************************************
** 函数名称: __tshellNetstatTcpListen
** 功能描述: 系统命令 "netstat -tl" (只显示 listen 状态 tcp)
** 输　入  : iNetType      网络类型 
** 输　出  : ERROR_NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __tshellNetstatTcpListen (INT  iNetType)
{
    const CHAR      cTcpInfoHdr[] = 
    "LOCAL                REMOTE               STATUS   RETRANS RCV_WND SND_WND\n";
    
    const CHAR      cTcp6InfoHdr[] = 
    "\nLOCAL6                                       REMOTE6                                      "
    "STATUS   IPv6-ONLY RETRANS RCV_WND SND_WND\n";
    
    struct tcp_pcb  *pcb;
    PCHAR            pcPrintBuf;
    size_t           stRealSize = 0;
    size_t           stNeedBufferSize = 0;
    BOOL             b4 = LW_FALSE, b6 = LW_FALSE;
    
    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType)) {
        for (pcb = tcp_listen_pcbs.pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip)) {
                stNeedBufferSize += 128;
                b4 = LW_TRUE;
            }
        }
    }
    if (__NETSTAT_INC_IPV6(iNetType)) {
        for (pcb = tcp_listen_pcbs.pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip)) {
                stNeedBufferSize += 192;
                b6 = LW_TRUE;
            }
        }
    }
    UNLOCK_TCPIP_CORE();
    
    if (stNeedBufferSize == 0) {
        return;
    }
    
    stNeedBufferSize += sizeof(cTcpInfoHdr) + sizeof(cTcp6InfoHdr);
    
    pcPrintBuf = (PCHAR)__SHEAP_ALLOC(stNeedBufferSize);
    if (!pcPrintBuf) {
        fprintf(stderr, "no memory!\n");
        _ErrorHandle(ENOMEM);
        return;
    }
    
    printf("--TCP LISTEN--:\n");
    
    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType) && b4) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize, 
                              cTcpInfoHdr);
        for (pcb = tcp_listen_pcbs.pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip)) {
                __TcpPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
    if (__NETSTAT_INC_IPV6(iNetType) && b6) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize, 
                              cTcp6InfoHdr);
        for (pcb = tcp_listen_pcbs.pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip)) {
                __TcpPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
    UNLOCK_TCPIP_CORE();
    
    printf("%s\n", pcPrintBuf);
    
    __SHEAP_FREE(pcPrintBuf);
}
/*********************************************************************************************************
** 函数名称: __tshellNetstatTcp
** 功能描述: 系统命令 "netstat -t"
** 输　入  : iNetType      网络类型
** 输　出  : ERROR_NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __tshellNetstatTcp (INT  iNetType)
{
    const CHAR      cTcpInfoHdr[] = 
    "LOCAL                REMOTE               STATUS   RETRANS RCV_WND SND_WND\n";
    
    const CHAR      cTcp6InfoHdr[] = 
    "\nLOCAL6                                       REMOTE6                                      "
    "STATUS   RETRANS RCV_WND SND_WND\n";
    
    struct tcp_pcb  *pcb;
    PCHAR            pcPrintBuf;
    size_t           stRealSize = 0;
    size_t           stNeedBufferSize = 0;
    BOOL             b4 = LW_FALSE, b6 = LW_FALSE;
    
    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType)) {
        for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip)) {
                stNeedBufferSize += 128;
                b4 = LW_TRUE;
            }
        }
        for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip)) {
                stNeedBufferSize += 192;
                b4 = LW_TRUE;
            }
        }
    }
    if (__NETSTAT_INC_IPV6(iNetType)) {
        for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip)) {
                stNeedBufferSize += 128;
                b6 = LW_TRUE;
            }
        }
        for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip)) {
                stNeedBufferSize += 192;
                b6 = LW_TRUE;
            }
        }
    }
    UNLOCK_TCPIP_CORE();
    
    if (stNeedBufferSize == 0) {
        return;
    }
    
    stNeedBufferSize += sizeof(cTcpInfoHdr) + sizeof(cTcp6InfoHdr);
    
    pcPrintBuf = (PCHAR)__SHEAP_ALLOC(stNeedBufferSize);
    if (!pcPrintBuf) {
        fprintf(stderr, "no memory!\n");
        _ErrorHandle(ENOMEM);
        return;
    }
    
    printf("--TCP--:\n");
    
    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType) && b4) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize, 
                              cTcpInfoHdr);
        for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip)) {
                __TcpPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
        for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip)) {
                __TcpPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
    if (__NETSTAT_INC_IPV6(iNetType) && b6) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize, 
                              cTcp6InfoHdr);
        for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip)) {
                __TcpPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
        for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip)) {
                __TcpPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
    UNLOCK_TCPIP_CORE();
    
    printf("%s\n", pcPrintBuf);
    
    __SHEAP_FREE(pcPrintBuf);
}
/*********************************************************************************************************
** 函数名称: __UdpPrint
** 功能描述: 打印网络 udp 文件
** 输　入  : pcb           tcp 控制块
**           pcBuffer      缓冲
**           stTotalSize   缓冲区大小
**           pstOft        当前偏移量
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __UdpPrint (struct udp_pcb *pcb, PCHAR  pcBuffer, 
                         size_t  stTotalSize, size_t *pstOft)
{
    CHAR    cBuffer1[INET6_ADDRSTRLEN];
    CHAR    cBuffer2[INET6_ADDRSTRLEN];

    if (IP_IS_V4_VAL(pcb->local_ip)) {
        *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                           "%-20s %-20s %s\n",
                           __ProtoAddrBuild(&pcb->local_ip, pcb->local_port, 
                                            cBuffer1, sizeof(cBuffer1)),
                           __ProtoAddrBuild(&pcb->remote_ip, pcb->remote_port, 
                                            cBuffer2, sizeof(cBuffer2)),
                           (pcb->flags & UDP_FLAGS_UDPLITE) ? "yes" : "no");
    } else {
        *pstOft = bnprintf(pcBuffer, stTotalSize, *pstOft,
                           "%-44s %-44s %s\n",
                           __ProtoAddrBuild(&pcb->local_ip, pcb->local_port, 
                                            cBuffer1, sizeof(cBuffer1)),
                           __ProtoAddrBuild(&pcb->remote_ip, pcb->remote_port, 
                                            cBuffer2, sizeof(cBuffer2)),
                           (pcb->flags & UDP_FLAGS_UDPLITE) ? "yes" : "no");
    }
}
/*********************************************************************************************************
** 函数名称: __tshellNetstatUdp
** 功能描述: 系统命令 "netstat -u"
** 输　入  : iNetType      网络类型
** 输　出  : ERROR_NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __tshellNetstatUdp (INT  iNetType)
{
    const CHAR      cUdpInfoHdr[] = 
    "LOCAL                REMOTE               UDPLITE\n";
    
    const CHAR      cUdp6InfoHdr[] = 
    "\nLOCAL6                                       REMOTE6                                      UDPLITE\n";
    
    struct udp_pcb  *pcb;
    PCHAR            pcPrintBuf;
    size_t           stRealSize = 0;
    size_t           stNeedBufferSize = 0;
    BOOL             b4 = LW_FALSE, b6 = LW_FALSE;

    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType)) {
        for (pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip)) {
                stNeedBufferSize += 64;
                b4 = LW_TRUE;
            }
        }
    }
    if (__NETSTAT_INC_IPV6(iNetType)) {
        for (pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip)) {
                stNeedBufferSize += 128;
                b6 = LW_TRUE;
            }
        }
    }
    UNLOCK_TCPIP_CORE();
    
    if (stNeedBufferSize == 0) {
        return;
    }
    
    stNeedBufferSize += sizeof(cUdpInfoHdr) + sizeof(cUdp6InfoHdr);
    
    pcPrintBuf = (PCHAR)__SHEAP_ALLOC(stNeedBufferSize);
    if (!pcPrintBuf) {
        fprintf(stderr, "no memory!\n");
        _ErrorHandle(ENOMEM);
        return;
    }

    printf("--UDP--:\n");
    
    LOCK_TCPIP_CORE();
    if (__NETSTAT_INC_IPV4(iNetType) && b4) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize, cUdpInfoHdr);
        for (pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
            if (IP_IS_V4_VAL(pcb->local_ip)) {
                __UdpPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
    if (__NETSTAT_INC_IPV6(iNetType) && b6) {
        stRealSize = bnprintf(pcPrintBuf, stNeedBufferSize, stRealSize, 
                              cUdp6InfoHdr);
        for (pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
            if (!IP_IS_V4_VAL(pcb->local_ip)) {
                __UdpPrint(pcb, pcPrintBuf, stNeedBufferSize, &stRealSize);
            }
        }
    }
    UNLOCK_TCPIP_CORE();
    
    printf("%s\n", pcPrintBuf);
    
    __SHEAP_FREE(pcPrintBuf);
}
/*********************************************************************************************************
** 函数名称: __tshellNetstatUnix
** 功能描述: 系统命令 "netstat -x"
** 输　入  : iNetType      网络类型
** 输　出  : ERROR_NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __tshellNetstatUnix (INT  iNetType)
{
    INT     iFd;
    CHAR    cBuffer[1024];
    ssize_t sstNum;
    
    if (!__NETSTAT_INC_UNIX(iNetType)) {
        return;
    }
    
    printf("--UNIX--:\n");
    fflush(stdout);
    
    iFd = open("/proc/net/unix", O_RDONLY);
    if (iFd < 0) {
        fprintf(stderr, "can not open /proc/net/unix : %s\n", lib_strerror(errno));
        return;
    }
    
    do {
        sstNum = read(iFd, cBuffer, sizeof(cBuffer));
        if (sstNum > 0) {
            write(STDOUT_FILENO, cBuffer, (size_t)sstNum);
        }
    } while (sstNum > 0);
    
    close(iFd);

    printf("\n");
}
/*********************************************************************************************************
** 函数名称: __tshellNetstatPacket
** 功能描述: 系统命令 "netstat -p"
** 输　入  : iNetType      网络类型
** 输　出  : ERROR_NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __tshellNetstatPacket (INT  iNetType)
{
    INT     iFd;
    CHAR    cBuffer[1024];
    ssize_t sstNum;
    
    printf("--PACKET--:\n");
    fflush(stdout);
    
    iFd = open("/proc/net/packet", O_RDONLY);
    if (iFd < 0) {
        fprintf(stderr, "can not open /proc/net/packet : %s\n", lib_strerror(errno));
        return;
    }
    
    do {
        sstNum = read(iFd, cBuffer, sizeof(cBuffer));
        if (sstNum > 0) {
            write(STDOUT_FILENO, cBuffer, (size_t)sstNum);
        }
    } while (sstNum > 0);
    
    close(iFd);

    printf("\n");
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

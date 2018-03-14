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
** 文   件   名: lwip_shell.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 05 月 06 日
**
** 描        述: lwip shell 命令.

** BUG:
2009.05.22  加入设置默认路由器 shell 命令.
2009.05.27  加入网络接口 linkup 的显示信息.
2009.06.02  纠正了 tftp 在 put 和 get 是源文件和目标的顺序.
2009.06.03  在启动网卡时如果是 DHCP 获取 IP, 首先需要将网络地址清零.
2009.06.08  ifup 加入了 -nodhcp 选项, 表明强制不使用 DHCP 获取 IP 地址.
2009.06.26  更新 shell 帮助内容.
2009.07.29  更新 ifconfig 服务, 尽量接近 linux bash.
2009.09.14  更新 ifrouter 对默认路由接口的显示.
2009.11.09  有些网卡设置 api 需要使用 netifapi_... 完成.
2009.11.21  tftp 命令进行调整, 放入 tftp工具中.
2009.12.11  ifconfig 中加入 metric 的显示.
2010.11.04  加入 arp 命令支持.
2011.06.08  ifconfig 显示 inet6 地址相关信息.
2011.07.02  加入 route 命令.
2012.08.21  不在初始化 ppp 相关 shell.
2013.05.14  打印 ipv6 地址时需要打印地址状态.
2013.09.12  增加 ifconfig 遍历网络接口时的安全性.
            增加 arp 命令的安全性.
2014.12.23  调整 ifconfig 显示.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_SHELL_EN > 0)
#include "net/if_lock.h"
#include "net/if_flags.h"
#include "lwip/opt.h"
#include "lwip/snmp.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/dns.h"
#include "lwip/inet.h"
#include "lwip/dhcp.h"
#include "lwip/tcpip.h"
#include "lwip_route.h"
#include "lwip_flowsh.h"
#if LW_CFG_NET_LOGINBL_EN > 0
#include "lwip_loginbl.h"
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
/*********************************************************************************************************
  ARP 子协议相关函数
*********************************************************************************************************/
#include "netif/etharp.h"
/*********************************************************************************************************
  netdev 相关函数
*********************************************************************************************************/
#include "netdev/netdev.h"
/*********************************************************************************************************
  netstat 帮助信息
*********************************************************************************************************/
static const CHAR   _G_cNetstatHelp[] = {
    "show net status\n\n"
    "-h, --help             display this message\n\n"
    "-r, --route            display route table\n"
    "-i, --interface        display interface table\n"
    "-g, --groups           display multicast group memberships\n"
    "-s, --statistics       display networking statistics (like SNMP)\n\n"
    "-w, --raw              display raw socket information\n"
    "-t, --tcp              display tcp socket information\n"
    "-u, --udp              display udp socket information\n"
    "-p, --packet           display packet socket information\n"
    "-x, --unix             display unix socket information\n\n"
    "-l, --listening        display listening server sockets\n"
    "-a, --all              display all sockets\n\n"
    "-A <net type>, --<net type>    select <net type>, <net type>=inet, inet6 or unix\n"
};
extern VOID  __tshellNetstatIf(VOID);
#if LWIP_IGMP > 0
extern VOID  __tshellNetstatGroup(INT  iNetType);
#endif
extern VOID  __tshellNetstatStat(VOID);
extern VOID  __tshellNetstatRaw(INT  iNetType);
extern VOID  __tshellNetstatTcp(INT  iNetType);
extern VOID  __tshellNetstatTcpListen(INT  iNetType);
extern VOID  __tshellNetstatUdp(INT  iNetType);
extern VOID  __tshellNetstatUnix(INT  iNetType);
extern VOID  __tshellNetstatPacket(INT  iNetType);
/*********************************************************************************************************
** 函数名称: __tshellNetstat
** 功能描述: 系统命令 "netstat"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellNetstat (INT  iArgC, PCHAR  *ppcArgV)
{
    int             iC;
    const  char     cShortOpt[] = "hrigswtpuxlaA:";
    struct option   optionNetstat[] = {
        {"help",       0, LW_NULL, 'h'},
        {"route",      0, LW_NULL, 'r'},
        {"interface",  0, LW_NULL, 'i'},
        {"groups",     0, LW_NULL, 'g'},
        {"statistics", 0, LW_NULL, 's'},
        {"raw",        0, LW_NULL, 'r'},
        {"tcp",        0, LW_NULL, 't'},
        {"udp",        0, LW_NULL, 'u'},
        {"packet",     0, LW_NULL, 'p'},
        {"unix",       0, LW_NULL, 'x'},
        {"listening",  0, LW_NULL, 'l'},
        {"all",        0, LW_NULL, 'a'},
        {"unix",       0, LW_NULL, 1},
        {"inet",       0, LW_NULL, 2},
        {"inet6",      0, LW_NULL, 3},
        {LW_NULL,      0, LW_NULL, 0},
    };
    
    BOOL    bPacket  = LW_FALSE;
    BOOL    bRaw     = LW_FALSE;
    BOOL    bUdp     = LW_FALSE;
    BOOL    bTcp     = LW_FALSE;
    BOOL    bUnix    = LW_FALSE;
    BOOL    bListen  = LW_FALSE;
    INT     iNetType = 0;                                               /* 0:all 1:unix 2:inet 3:inet6  */
    CHAR    cNettype[10];
    
    while ((iC = getopt_long(iArgC, ppcArgV, cShortOpt, optionNetstat, LW_NULL)) != -1) {
        switch (iC) {
        
        case 'h':                                                       /*  显示帮助                    */
            printf(_G_cNetstatHelp);
            return  (ERROR_NONE);
            
        case 'r':                                                       /*  显示路由表                  */
            ppcArgV[1] = LW_NULL;
#if LW_CFG_NET_ROUTER > 0
            __tshellRoute(1, ppcArgV);
#endif
            return  (ERROR_NONE);
            
        case 'i':                                                       /*  显示网络接口信息            */
            __tshellNetstatIf();
            return  (ERROR_NONE);
            
        case 'g':                                                       /*  显示组播表情况              */
#if LWIP_IGMP > 0
            __tshellNetstatGroup(iNetType);
#endif
            return  (ERROR_NONE);
        
        case 's':                                                       /*  显示统计信息                */
            __tshellNetstatStat();
            return  (ERROR_NONE);
            
        case 'p':                                                       /*  显示 packet socket          */
            bPacket = LW_TRUE;
            break;
            
        case 'w':                                                       /*  显示 raw socket             */
            bRaw = LW_TRUE;
            break;
            
        case 't':                                                       /*  显示 tcp socket             */
            bTcp = LW_TRUE;
            break;
            
        case 'u':                                                       /*  显示 udp socket             */
            bUdp = LW_TRUE;
            break;
        
        case 'x':                                                       /*  显示 unix socket            */
            bUnix = LW_TRUE;
            break;
        
        case 'l':                                                       /*  显示 listen socket          */
            bListen = LW_TRUE;
            break;
            
        case 'a':                                                       /*  显示列表                    */
            goto    __show;
            
        case 'A':                                                       /*  网络类型                    */
            lib_strlcpy(cNettype, optarg, 10);
            if (lib_strcmp(cNettype, "unix") == 0) {
                iNetType = 1;
            } else if (lib_strcmp(cNettype, "inet") == 0) {
                iNetType = 2;
            } else if (lib_strcmp(cNettype, "inet6") == 0) {
                iNetType = 3;
            }
            break;
            
        case 1:
            iNetType = 1;
            break;
            
        case 2:
            iNetType = 2;
            break;
            
        case 3:
            iNetType = 3;
            break;
        }
    }
    getopt_free();
    
__show:
    if ((bRaw    == LW_FALSE) && (bUdp    == LW_FALSE) &&
        (bTcp    == LW_FALSE) && (bUnix   == LW_FALSE) &&
        (bListen == LW_FALSE) && (bPacket == LW_FALSE)) {
        bRaw    = LW_TRUE;
        bUdp    = LW_TRUE;
        bTcp    = LW_TRUE;
        bUnix   = LW_TRUE;
        bPacket = LW_TRUE;
    }
    if (bUnix) {
        __tshellNetstatUnix(iNetType);
    }
    if (bPacket) {
        __tshellNetstatPacket(iNetType);
    }
    if (bTcp || bListen) {
        __tshellNetstatTcpListen(iNetType);
        if (bTcp) {
            __tshellNetstatTcp(iNetType);
        }
    }
    if (bUdp) {
        __tshellNetstatUdp(iNetType);
    }
    if (bRaw) {
        __tshellNetstatRaw(iNetType);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __netIfSpeed
** 功能描述: 显示指定的网络接口信息 (ip v4)
** 输　入  : pcIfName      网络接口名
**           netifShow     网络接口结构
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __netIfSpeed (struct netif  *netif, PCHAR  pcSpeedStr, size_t  stSize)
{
    netdev_t  *pnetdev;
    UINT64     u64Speed;
    
    pnetdev = netdev_find_by_index(netif_get_index(netif));
    if (pnetdev) {
        u64Speed = pnetdev->speed;
    } else {
        u64Speed = (UINT64)netif->link_speed;
    }

    if (u64Speed == 0) {
        lib_strlcpy(pcSpeedStr, "N/A", stSize);
        
    } else if (u64Speed < 1000ull) {
        snprintf(pcSpeedStr, stSize, "%qu bps", u64Speed);
    
    } else if (u64Speed < 5000000ull) {
        snprintf(pcSpeedStr, stSize, "%qu Kbps", u64Speed / 1000);
    
    } else if (u64Speed < 5000000000ull) {
        snprintf(pcSpeedStr, stSize, "%qu Mbps", u64Speed / 1000000);
    
    } else {
        snprintf(pcSpeedStr, stSize, "%qu Gbps", u64Speed / 1000000000);
    }
}
/*********************************************************************************************************
** 函数名称: __netIfShow
** 功能描述: 显示指定的网络接口信息 (ip v4)
** 输　入  : pcIfName      网络接口名
**           netifShow     网络接口结构
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __netIfShow (CPCHAR  pcIfName, const struct netif  *netifShow)
{
#define MIB2_NETIF(netif)   (&((netif)->mib2_counters))

    struct netif    *netif;
    struct netdev   *netdev;
    CHAR             cIfName[NETIF_NAMESIZE];
    CHAR             cSpeed[32];
    PCHAR            pcDevName = "N/A";
    ip4_addr_t       ipaddrBroadcast;
    INT              i, iFlags;

    if ((pcIfName == LW_NULL) && (netifShow == LW_NULL)) {
        return;
    }

    if (netifShow) {
        netif = (struct netif *)netifShow;
    } else {
        netif = netif_find((PCHAR)pcIfName);
    }

    if (netif == LW_NULL) {
        return;
    }
    
    netdev = (netdev_t *)(netif->state);
    if (netdev && (netdev->magic_no == NETDEV_MAGIC)) {
        pcDevName = netdev->dev_name;
    }

    printf("%-5s%4s ", netif_get_name(netif, cIfName), "");             /*  网卡名称                    */
    
    if (netif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET)) {     /*  以太网络                    */
        printf("Link encap: Ethernet HWaddr: ");
        for (i = 0; i < netif->hwaddr_len - 1; i++) {
            printf("%02x:", netif->hwaddr[i]);
        }
        printf("%02x\n", netif->hwaddr[netif->hwaddr_len - 1]);
        
    } else if ((netif->flags & NETIF_FLAG_BROADCAST) == 0) {            /*  点对点网络接口              */
        if (netif->link_type == snmp_ifType_softwareLoopback) {
            printf("Link encap: Local Loopback\n");
        } else {
            printf("Link encap: WAN(PPP/SLIP)\n");
        }
        
    } else {                                                            /*  通用网络接口                */
        printf("Link encap: General\n");
    }
    
    __netIfSpeed(netif, cSpeed, sizeof(cSpeed));
    
#if LWIP_DHCP
    printf("%9s Dev: %s Ifidx: %d DHCP: %s%s %s%s Spd: %s\n", "", 
           pcDevName, netif_get_index(netif),
           (netif->flags2 & NETIF_FLAG2_DHCP) ? "E4" : "D4",
           (netif->flags2 & NETIF_FLAG2_DHCP) ? ((netif_dhcp_data(netif)) ? "(On)" : "(Off)") : "",
#if LWIP_IPV6_DHCP6
           (netif->flags2 & NETIF_FLAG2_DHCP6) ? "E6" : "D6",
           (netif->flags2 & NETIF_FLAG2_DHCP6) ? ((netif_dhcp6_data(netif)) ? "(On)" : "(Off)") : "",
#else
           "", "", 
#endif                                                                  /*  LWIP_IPV6_DHCP6             */
           cSpeed);
#else
    printf("%9s Dev: %s Ifidx: %d Spd: %s\n", "", 
           pcDevName, netif_get_index(netif), cSpeed);
#endif                                                                  /*  LWIP_DHCP                   */
    
    printf("%9s inet addr: %d.%d.%d.%d ", "",
           ip4_addr1(netif_ip4_addr(netif)), ip4_addr2(netif_ip4_addr(netif)),
           ip4_addr3(netif_ip4_addr(netif)), ip4_addr4(netif_ip4_addr(netif)));
    printf("netmask: %d.%d.%d.%d\n", 
           ip4_addr1(netif_ip4_netmask(netif)), ip4_addr2(netif_ip4_netmask(netif)),
           ip4_addr3(netif_ip4_netmask(netif)), ip4_addr4(netif_ip4_netmask(netif)));
    
    if ((netif->flags & NETIF_FLAG_BROADCAST) == 0) {
        printf("%9s P-to-P: %d.%d.%d.%d ", "",
               ip4_addr1(netif_ip4_gw(netif)), ip4_addr2(netif_ip4_gw(netif)),
               ip4_addr3(netif_ip4_gw(netif)), ip4_addr4(netif_ip4_gw(netif)));
        printf("broadcast: Non\n");
    
    } else {
        printf("%9s gateway: %d.%d.%d.%d ", "",
               ip4_addr1(netif_ip4_gw(netif)), ip4_addr2(netif_ip4_gw(netif)),
               ip4_addr3(netif_ip4_gw(netif)), ip4_addr4(netif_ip4_gw(netif)));
        ipaddrBroadcast.addr = (netif_ip4_addr(netif)->addr | (~netif_ip4_netmask(netif)->addr));
        printf("broadcast: %d.%d.%d.%d\n", 
               ip4_addr1(&ipaddrBroadcast), ip4_addr2(&ipaddrBroadcast),
               ip4_addr3(&ipaddrBroadcast), ip4_addr4(&ipaddrBroadcast));
    }
    
#if LWIP_IPV6
    for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        PCHAR       pcAddrType;
        CHAR        cBuffer[64];
        
        if (!ip6_addr_isvalid(netif->ip6_addr_state[i])) {
            continue;
        }
        
        if (ip6_addr_isglobal(ip_2_ip6(&netif->ip6_addr[i]))) {
            pcAddrType = "global";
        } else if (ip6_addr_islinklocal(ip_2_ip6(&netif->ip6_addr[i]))) {
            pcAddrType = "link";
        } else if (ip6_addr_issitelocal(ip_2_ip6(&netif->ip6_addr[i]))) {
            pcAddrType = "site";
        } else if (ip6_addr_isuniquelocal(ip_2_ip6(&netif->ip6_addr[i]))) {
            pcAddrType = "uniquelocal";
        } else if (ip6_addr_isloopback(ip_2_ip6(&netif->ip6_addr[i]))) {
            pcAddrType = "loopback";
        } else {
            pcAddrType = "unknown";
        }
        
        printf("%9s inet6 addr: %s Scope:%s\n", "",
               ip6addr_ntoa_r(ip_2_ip6(&netif->ip6_addr[i]), cBuffer, sizeof(cBuffer)),
               pcAddrType);
    }
#endif                                                                  /*  LWIP_IPV6                   */
    
    printf("%9s ", "");
    iFlags = netif_get_flags(netif);
    if (iFlags & IFF_UP) {
        printf("UP ");
    }
    if (iFlags & IFF_BROADCAST) {
        printf("BROADCAST ");
    }
    if (iFlags & IFF_LOOPBACK) {
        printf("LOOPBACK ");
    }
    if (iFlags & IFF_RUNNING) {
        printf("RUNNING ");
    }
    if (iFlags & IFF_MULTICAST) {
        printf("MULTICAST ");
    }
    if (netif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET)) {
        if (iFlags & IFF_NOARP) {
            printf("NOARP ");
        }
    }
    printf("MTU:%d METRIC:%d\n", netif->mtu, netif->metric);
    
    printf("%9s RX ucast packets:%u nucast packets:%u dropped:%u\n", "",
           MIB2_NETIF(netif)->ifinucastpkts, MIB2_NETIF(netif)->ifinnucastpkts, MIB2_NETIF(netif)->ifindiscards);
    printf("%9s TX ucast packets:%u nucast packets:%u dropped:%u\n", "",
           MIB2_NETIF(netif)->ifoutucastpkts, MIB2_NETIF(netif)->ifoutnucastpkts, MIB2_NETIF(netif)->ifoutdiscards);
    printf("%9s RX bytes:%u TX bytes:%u\n", "",
           MIB2_NETIF(netif)->ifinoctets, MIB2_NETIF(netif)->ifoutoctets);
    printf("\n");
}
/*********************************************************************************************************
** 函数名称: __netIfShowAll
** 功能描述: 显示所有网络接口信息 (ip v4)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __netIfShowAll (VOID)
{
    struct netif *netif;
    INT           iCounter = 0;
    INT           i;
    CHAR          cName[NETIF_NAMESIZE] = "null";                       /*  当前默认路由网络接口名      */

    NETIF_FOREACH(netif) {
        __netIfShow(LW_NULL, netif);
        iCounter++;
    }

#if LWIP_DNS > 0
    for (i = 0; i < DNS_MAX_SERVERS; i++) {
        ip_addr_t ipaddr = *dns_getserver((u8_t)i);
        printf("dns%d: %d.%d.%d.%d\n", (i),
                                       ip4_addr1(ip_2_ip4(&ipaddr)),
                                       ip4_addr2(ip_2_ip4(&ipaddr)),
                                       ip4_addr3(ip_2_ip4(&ipaddr)),
                                       ip4_addr4(ip_2_ip4(&ipaddr)));
    }
#endif                                                                  /*  LWIP_DNS                    */

    if (netif_default) {
        netif_get_name(netif_default, cName);
    }
    
    printf("default device is: %s\n", cName);                           /*  显示路由端口                */
    printf("total net interface: %d\n", iCounter);
}
/*********************************************************************************************************
** 函数名称: __netIfSet
** 功能描述: 设置指定网络接口信息 (ip v4)
** 输　入  : netif     网络接口
**           pcItem    设置项目名称
**           ipaddr    地址信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __netIfSet (struct netif  *netif, CPCHAR  pcItem, ip4_addr_t *ipaddr)
{
    ip4_addr_t  ipaddrInet;
    ip4_addr_t  ipaddrMask;
    ip4_addr_t  ipaddrGw;

    if (netif == LW_NULL) {
        return;
    }
    
    ipaddrInet = *netif_ip4_addr(netif);
    ipaddrMask = *netif_ip4_netmask(netif);
    ipaddrGw   = *netif_ip4_gw(netif);

    if (lib_strcmp(pcItem, "inet") == 0) {
        netifapi_netif_set_addr(netif, ipaddr, &ipaddrMask, &ipaddrGw);
    
    } else if (lib_strcmp(pcItem, "netmask") == 0) {
        netifapi_netif_set_addr(netif, &ipaddrInet, ipaddr, &ipaddrGw);
    
    } else if (lib_strcmp(pcItem, "gateway") == 0) {
        netifapi_netif_set_addr(netif, &ipaddrInet, &ipaddrMask, ipaddr);
    
    } else {
        fprintf(stderr, "arguments error!\n");
    }
}
/*********************************************************************************************************
** 函数名称: __tshellIfconfig
** 功能描述: 系统命令 "ifconfig"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellIfconfig (INT  iArgC, PCHAR  *ppcArgV)
{
    struct netif    *netif;
    struct in_addr   inaddr;
    ip_addr_t        ipaddr;

    if (iArgC == 1) {
        LWIP_IF_LIST_LOCK(LW_FALSE);
        __netIfShowAll();                                               /*  打印所有网口信息            */
        LWIP_IF_LIST_UNLOCK();
        return  (ERROR_NONE);
    
    } else if (iArgC == 2) {
        LWIP_IF_LIST_LOCK(LW_FALSE);
        __netIfShow(ppcArgV[1], LW_NULL);                               /*  打印指定网口信息            */
        LWIP_IF_LIST_UNLOCK();
        return  (ERROR_NONE);
    }

    if (iArgC >= 4) {                                                   /*  网络接口参数设置            */
        if (lib_strcmp(ppcArgV[1], "dns") == 0) {                       /*  指定 DNS 设置               */
            INT     iDnsIndex = 0;
            
            sscanf(ppcArgV[2], "%d", &iDnsIndex);
            if (iDnsIndex >= DNS_MAX_SERVERS) {
                fprintf(stderr, "arguments error!\n");
                return  (-ERROR_TSHELL_EPARAM);
            }
            if (inet_aton(ppcArgV[3], &inaddr) == 0) {                  /*  获得 IP 地址                */
                fprintf(stderr, "address error.\n");
                return  (-ERROR_TSHELL_EPARAM);
            }
            ip_2_ip4(&ipaddr)->addr = inaddr.s_addr;
            IP_SET_TYPE_VAL(ipaddr, IPADDR_TYPE_V4);
            dns_setserver((u8_t)iDnsIndex, &ipaddr);                    /*  设置 DNS                    */
        
        } else {                                                        /*  指定网络接口设置            */
            INT     iIndex;
            
            LWIP_IF_LIST_LOCK(LW_FALSE);
            netif = netif_find(ppcArgV[1]);                             /*  查询网络接口                */
            if (netif == LW_NULL) {
                LWIP_IF_LIST_UNLOCK();
                fprintf(stderr, "can not find net interface.\n");
                return  (-ERROR_TSHELL_EPARAM);
            }
            for (iIndex = 2; iIndex < (iArgC - 1); iIndex += 2) {       /*  连续设置参数                */
                if (inet_aton(ppcArgV[iIndex + 1], &inaddr) == 0) {     /*  获得 IP 地址                */
                    fprintf(stderr, "address error.\n");
                    return  (-ERROR_TSHELL_EPARAM);
                }
                ip_2_ip4(&ipaddr)->addr = inaddr.s_addr;
                IP_SET_TYPE_VAL(ipaddr, IPADDR_TYPE_V4);
                __netIfSet(netif, ppcArgV[iIndex], ip_2_ip4(&ipaddr));  /*  设置网络接口                */
            }
            LWIP_IF_LIST_UNLOCK();
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellIfUp
** 功能描述: 系统命令 "ifup"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellIfUp (INT  iArgC, PCHAR  *ppcArgV)
{
    INT           i;
    struct netif *netif;
    BOOL          bUseDHCP       = LW_FALSE;                            /*  是否使用自动获取 IP         */
    BOOL          bShutDownDHCP  = LW_FALSE;                            /*  是否强制关闭 DHCP           */

#if LWIP_IPV6_DHCP6 > 0
    BOOL          bUseDHCP6      = LW_FALSE;                            /*  是否使用自动获取 IP         */
    BOOL          bShutDownDHCP6 = LW_FALSE;                            /*  是否强制关闭 DHCP           */
#endif                                                                  /*  LWIP_IPV6_DHCP6             */

    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    
    } else if (iArgC > 2) {
        for (i = 2; i < iArgC; i++) {
            if (lib_strcmp(ppcArgV[i], "-dhcp") == 0) {
                bUseDHCP = LW_TRUE;                                     /*  使用 DHCP 启动              */
            } else if (lib_strcmp(ppcArgV[i], "-nodhcp") == 0) {
                bShutDownDHCP = LW_TRUE;
            } 
#if LWIP_IPV6_DHCP6 > 0
              else if (lib_strcmp(ppcArgV[i], "-dhcp6") == 0) {
                bUseDHCP6 = LW_TRUE;
            } else if (lib_strcmp(ppcArgV[i], "-nodhcp6") == 0) {
                bShutDownDHCP6 = LW_TRUE;
            }
#endif                                                                  /*  LWIP_IPV6_DHCP6             */
        }
    }

    LWIP_IF_LIST_LOCK(LW_FALSE);
    netif = netif_find(ppcArgV[1]);                                     /*  查询网络接口                */
    if (netif == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();
        fprintf(stderr, "can not find net interface.\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    if (netif_is_up(netif)) {                                           /*  网卡是否已经启动            */
#if LWIP_DHCP > 0                                                       /*  首先关闭网卡                */
        if (netif_dhcp_data(netif)) {
            netifapi_dhcp_release_and_stop(netif);                      /*  解除 DHCP 租约并释放资源    */
        }
#endif                                                                  /*  LWIP_DHCP > 0               */
#if LWIP_IPV6_DHCP6 > 0
        if (netif_dhcp6_data(netif)) {
            netifapi_dhcp6_disable(netif);
        }
#endif                                                                  /*  LWIP_IPV6_DHCP6             */
        netifapi_netif_set_down(netif);                                 /*  禁用网卡                    */
    }

    netifapi_netif_set_up(netif);                                       /*  启动网卡                    */

#if LWIP_DHCP > 0
    if (bUseDHCP) {
        netif->flags2 |= NETIF_FLAG2_DHCP;                              /*  使用 DHCP 启动              */
    } else if (bShutDownDHCP) {
        netif->flags2 &= ~NETIF_FLAG2_DHCP;                             /*  强制关闭 DHCP               */
    }

    if (netif->flags2 & NETIF_FLAG2_DHCP) {
        ip4_addr_t  inaddrNone;

        lib_bzero(&inaddrNone, sizeof(ip4_addr_t));
        netifapi_netif_set_addr(netif, &inaddrNone, &inaddrNone, &inaddrNone);
                                                                        /*  所有地址设置为 0            */
        printf("DHCP client starting...\n");
        if (netifapi_dhcp_start(netif) < ERR_OK) {
            printf("DHCP client serious error.\n");
        } else {
            printf("DHCP client start.\n");
        }
    }
#endif                                                                  /*  LWIP_DHCP > 0               */

#if LWIP_IPV6_DHCP6 > 0
    if (bUseDHCP6) {
        netif->flags2 |= NETIF_FLAG2_DHCP6;                             /*  使用 DHCP 启动              */
    } else if (bShutDownDHCP6) {
        netif->flags2 &= ~NETIF_FLAG2_DHCP6;                            /*  强制关闭 DHCP               */
    }
    
    if (netif->flags2 & NETIF_FLAG2_DHCP6) {
        printf("DHCPv6 client starting...\n");
        if (netifapi_dhcp6_enable_stateless(netif) < ERR_OK) {
            printf("DHCPv6 client serious error.\n");
        } else {
            printf("DHCPv6 client start.\n");
        }
    }
#endif                                                                  /*  LWIP_IPV6_DHCP6             */
    LWIP_IF_LIST_UNLOCK();
    
    printf("net interface \"%s\" set up.\n", ppcArgV[1]);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellIfDown
** 功能描述: 系统命令 "ifdown"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellIfDown (INT  iArgC, PCHAR  *ppcArgV)
{
    struct netif *netif;

    if (iArgC != 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    LWIP_IF_LIST_LOCK(LW_FALSE);
    netif = netif_find(ppcArgV[1]);                                     /*  查询网络接口                */
    if (netif == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();
        fprintf(stderr, "can not find net interface.\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    if (!netif_is_up(netif)) {
        LWIP_IF_LIST_UNLOCK();
        fprintf(stderr, "net interface already set down.\n");
        return  (PX_ERROR);
    }

#if LWIP_DHCP > 0
    if (netif_dhcp_data(netif)) {
        netifapi_dhcp_release_and_stop(netif);                          /*  解除 DHCP 租约并释放资源    */
    }
#endif                                                                  /*  LWIP_DHCP > 0               */

#if LWIP_IPV6_DHCP6 > 0
    if (netif_dhcp6_data(netif)) {
        netifapi_dhcp6_disable(netif);
    }
#endif                                                                  /*  LWIP_IPV6_DHCP6             */

    netifapi_netif_set_down(netif);                                     /*  禁用网卡                    */
    LWIP_IF_LIST_UNLOCK();
    
    printf("net interface \"%s\" set down.\n", ppcArgV[1]);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellIfRouter
** 功能描述: 系统命令 "ifrouter"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellIfRouter (INT  iArgC, PCHAR  *ppcArgV)
{
    printf("this command has been removed! please use 'route' command instead.\n");

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellArp
** 功能描述: 系统命令 "arp"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellArp (INT  iArgC, PCHAR  *ppcArgV)
{
    if (iArgC < 2) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (lib_strcmp(ppcArgV[1], "-a") == 0) {                            /*  显示 arp 表                 */
        INT     iFd;
        CHAR    cBuffer[512];
        ssize_t sstNum;
        
        iFd = open("/proc/net/arp", O_RDONLY);
        if (iFd < 0) {
            fprintf(stderr, "can not open /proc/net/arp : %s\n", lib_strerror(errno));
            return  (PX_ERROR);
        }
        
        do {
            sstNum = read(iFd, cBuffer, sizeof(cBuffer));
            if (sstNum > 0) {
                write(STDOUT_FILENO, cBuffer, (size_t)sstNum);
            }
        } while (sstNum > 0);
        
        close(iFd);
        
        return  (ERROR_NONE);
    
    } else if (lib_strcmp(ppcArgV[1], "-s") == 0) {                     /*  加入一个静态转换关系        */
        INT             i;
        INT             iTemp[6];
        ip4_addr_t      ipaddr;
        struct eth_addr ethaddr;
        err_t           err;
        
        if (iArgC != 4) {
            fprintf(stderr, "arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        ipaddr.addr = ipaddr_addr(ppcArgV[2]);
        if (ipaddr.addr == IPADDR_NONE) {
            fprintf(stderr, "bad inet address: %s\n", ppcArgV[2]);
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        if (sscanf(ppcArgV[3], "%02x:%02x:%02x:%02x:%02x:%02x",
                   &iTemp[0], &iTemp[1], &iTemp[2], 
                   &iTemp[3], &iTemp[4], &iTemp[5]) != 6) {
            fprintf(stderr, "bad physical address: %s\n", ppcArgV[3]);
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        for (i = 0; i < 6; i++) {
            ethaddr.addr[i] = (u8_t)iTemp[i];
        }
        
        err = netifapi_arp_add(&ipaddr, &ethaddr, NETIFAPI_ARP_PERM);
        return  (err ? PX_ERROR : ERROR_NONE);
    
    } else if (lib_strcmp(ppcArgV[1], "-d") == 0) {                     /*  删除一个静态转换关系        */
        ip4_addr_t      ipaddr;
        err_t           err;
        
        if (iArgC != 3) {                                               /*  删除全部转换关系            */
            struct netif *netif;
            
            LWIP_IF_LIST_LOCK(LW_FALSE);
            NETIF_FOREACH(netif) {
                if (netif->flags & NETIF_FLAG_ETHARP) {
                    netifapi_arp_cleanup(netif);
                }
            }
            LWIP_IF_LIST_UNLOCK();
            
            return  (ERROR_NONE);
        }
        
        ipaddr.addr = ipaddr_addr(ppcArgV[2]);
        if (ipaddr.addr == IPADDR_NONE) {
            fprintf(stderr, "bad inet address: %s\n", ppcArgV[2]);
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        err = netifapi_arp_remove(&ipaddr, NETIFAPI_ARP_PERM);
        return  (err ? PX_ERROR : ERROR_NONE);
    
    } else {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
}
/*********************************************************************************************************
** 函数名称: __tshellLoginBl
** 功能描述: 系统命令 "loginbl"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_NET_LOGINBL_EN > 0

static INT  __tshellLoginBl (INT  iArgC, PCHAR  *ppcArgV)
{
    struct sockaddr      addr;
    struct sockaddr_in  *pinaddr  = (struct sockaddr_in *)&addr;
    struct sockaddr_in6 *pin6addr = (struct sockaddr_in6 *)&addr;

    if (iArgC == 1) {
        API_LoginBlShow();
        return  (ERROR_NONE);
    
    } else if (iArgC != 3) {
        goto    __error;
    }
    
    lib_bzero(&addr, sizeof(struct sockaddr));
    
    pinaddr->sin_addr.s_addr = inet_addr(ppcArgV[2]);
    if (pinaddr->sin_addr.s_addr == INADDR_NONE) {
        if (inet6_aton(ppcArgV[2], &pin6addr->sin6_addr) == 0) {
            fprintf(stderr, "ipaddr error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        
        } else {
            pin6addr->sin6_len    = sizeof(struct sockaddr_in6);
            pin6addr->sin6_family = AF_INET6;
        }
    
    } else {
        pinaddr->sin_len    = sizeof(struct sockaddr_in);
        pinaddr->sin_family = AF_INET;
    }
    
    if (lib_strcmp(ppcArgV[1], "add") == 0) {
        API_LoginBlAdd(&addr, 1, 0);
        return  (ERROR_NONE);
        
    } else if (lib_strcmp(ppcArgV[1], "del") == 0) {
        API_LoginBlDelete(&addr);
        return  (ERROR_NONE);
    }
    
__error:
    fprintf(stderr, "arguments error!\n");
    return  (-ERROR_TSHELL_EPARAM);
}
/*********************************************************************************************************
** 函数名称: __tshellLoginWl
** 功能描述: 系统命令 "loginwl"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tshellLoginWl (INT  iArgC, PCHAR  *ppcArgV)
{
    struct sockaddr      addr;
    struct sockaddr_in  *pinaddr  = (struct sockaddr_in *)&addr;
    struct sockaddr_in6 *pin6addr = (struct sockaddr_in6 *)&addr;

    if (iArgC == 1) {
        API_LoginWlShow();
        return  (ERROR_NONE);
    
    } else if (iArgC != 3) {
        goto    __error;
    }
    
    lib_bzero(&addr, sizeof(struct sockaddr));
    
    pinaddr->sin_addr.s_addr = inet_addr(ppcArgV[2]);
    if (pinaddr->sin_addr.s_addr == INADDR_NONE) {
        if (inet6_aton(ppcArgV[2], &pin6addr->sin6_addr) == 0) {
            fprintf(stderr, "ipaddr error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        
        } else {
            pin6addr->sin6_len    = sizeof(struct sockaddr_in6);
            pin6addr->sin6_family = AF_INET6;
        }
    
    } else {
        pinaddr->sin_len    = sizeof(struct sockaddr_in);
        pinaddr->sin_family = AF_INET;
    }
    
    if (lib_strcmp(ppcArgV[1], "add") == 0) {
        API_LoginWlAdd(&addr);
        return  (ERROR_NONE);
        
    } else if (lib_strcmp(ppcArgV[1], "del") == 0) {
        API_LoginWlDelete(&addr);
        return  (ERROR_NONE);
    }
    
__error:
    fprintf(stderr, "arguments error!\n");
    return  (-ERROR_TSHELL_EPARAM);
}

#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
/*********************************************************************************************************
** 函数名称: __tshellNetInit
** 功能描述: 注册网络命令
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  __tshellNetInit (VOID)
{
#if LW_CFG_NET_ROUTER > 0
    __tshellRouteInit();                                                /*  注册 route 命令             */
#if LW_CFG_NET_BALANCING > 0
    __tshellSrouteInit();
#endif                                                                  /*  LW_CFG_NET_BALANCING > 0    */
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */

#if LW_CFG_NET_FLOWCTL_EN > 0
    __tshellFlowctlInit();
#endif                                                                  /*  LW_CFG_NET_FLOWCTL_EN > 0   */

    API_TShellKeywordAdd("netstat", __tshellNetstat);
    API_TShellFormatAdd("netstat",  " {[-wtux --A] -i | [-hrigsapl]}");
    API_TShellHelpAdd("netstat",    _G_cNetstatHelp);

    API_TShellKeywordAdd("ifconfig", __tshellIfconfig);
    API_TShellFormatAdd("ifconfig",  " [netifname] [{inet | netmask | gateway}] [address]");
    API_TShellHelpAdd("ifconfig",    "show or set net interface parameter.\n"
                                     "if there are no arguments, it will show all interface parameter\n"
                                     "set interface like following:\n"
                                     "ifconfig en1 inet    192.168.0.3\n"
                                     "ifconfig en1 netmask 255.255.255.0\n"
                                     "ifconfig en1 gateway 192.168.0.1\n"
                                     "ifconfig dns 0       192.168.0.2\n");

    API_TShellKeywordAdd("ifup", __tshellIfUp);
    API_TShellFormatAdd("ifup", " [netifname] [{-dhcp | -nodhcp}]");
    API_TShellHelpAdd("ifup",   "set net interface enable\n"
                                "\"-dncp\"   mean use dhcp client get net address.\n"
                                "\"-nodncp\" mean MUST NOT use dhcp.\n");

    API_TShellKeywordAdd("ifdown", __tshellIfDown);
    API_TShellFormatAdd("ifdown", " [netifname]");
    API_TShellHelpAdd("ifdown",   "set net interface disable.\n");

    API_TShellKeywordAdd("ifrouter", __tshellIfRouter);
    API_TShellFormatAdd("ifrouter", " [netifname]");
    API_TShellHelpAdd("ifrouter",   "set default router net interface.\n");
    
    API_TShellKeywordAdd("arp", __tshellArp);
    API_TShellFormatAdd("arp", " [-a | -s inet_address physical_address | -d inet_address]");
    API_TShellHelpAdd("arp",   "display ro modifies ARP table.\n"
                               "-a      display the ARP table.\n"
                               "-s      add or set a static arp entry.\n"
                               "        eg. arp -s 192.168.1.100 00:11:22:33:44:55\n"
                               "-d      delete a STATIC arp entry.\n"
                               "        eg. arp -d 192.168.1.100\n");
#if LW_CFG_NET_LOGINBL_EN > 0
    API_TShellKeywordAdd("loginbl", __tshellLoginBl);
    API_TShellFormatAdd("loginbl", " [{add | del}] [ipaddr]");
    API_TShellHelpAdd("loginbl", "show remote login blacklist.\n");
    
    API_TShellKeywordAdd("loginwl", __tshellLoginWl);
    API_TShellFormatAdd("loginwl", " [{add | del}] [ipaddr]");
    API_TShellHelpAdd("loginwl", "show remote login whitelist.\n");
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

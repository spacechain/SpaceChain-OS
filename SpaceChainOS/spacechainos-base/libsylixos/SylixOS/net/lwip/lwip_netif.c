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
** 文   件   名: lwip_netif.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 07 月 30 日
**
** 描        述: lwip 网口计数器改进.
                 lwip netif_add() 总是增加网络接口计数器, 但 netif_remove() 并没有处理.
                 
** BUG:
2011.02.13  netif_remove_hook() 中加入对 npf detach 的操作, 确保 attach 与 detach 成对操作.
2011.03.10  将 _G_ulNetIfLock 开放, posix net/if.h 需要此锁.
2011.07.04  加入对路由表的回调操作.
2013.04.16  netif_remove_hook 需要卸载 DHCP 数据结构.
2013.09.24  移除网络接口加入对 auto ip 的回收.
2014.03.22  可以通过索引号, 快速得到 netif.
*********************************************************************************************************/
#define  __SYLIXOS_RTHOOK
#define  __SYLIXOS_KERNEL
#define  __NETIF_MAIN_FILE
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "lwip/mem.h"
#include "lwip/snmp.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/dhcp.h"
#include "lwip/dhcp6.h"
#include "lwip/autoip.h"
#include "lwip/err.h"
#include "net/if.h"
#include "net/if_event.h"
#include "net/if_lock.h"
#if LW_CFG_NET_ROUTER > 0
#include "net/route.h"
#include "route/af_route.h"
#include "route/ip4_route.h"
#include "route/ip6_route.h"
#if LW_CFG_NET_BALANCING > 0
#include "net/sroute.h"
#include "balancing/ip4_sroute.h"
#include "balancing/ip6_sroute.h"
#endif                                                                  /*  LW_CFG_NET_BALANCING > 0    */
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
#if LW_CFG_NET_MROUTER > 0
#include "mroute/ip4_mrt.h"
#include "mroute/ip6_mrt.h"
#endif                                                                  /*  LW_CFG_NET_MROUTER > 0      */
#if LW_CFG_NET_FLOWCTL_EN > 0
#include "flowctl/net_flowctl.h"
#endif
#if LW_CFG_NET_NAT_EN > 0
#include "tools/nat/lwip_natlib.h"
#endif                                                                  /*  LW_CFG_NET_NAT_EN > 0       */
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
#if LW_CFG_NET_NPF_EN > 0
VOID  npf_netif_detach(struct netif  *pnetif);
#endif                                                                  /*  LW_CFG_NET_NPF_EN > 0       */
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
#if LW_CFG_NET_ROUTER == 0
enum if_addr_type {
    IPADDR = 0,
    NETMASK
};
#endif                                                                  /*  LW_CFG_NET_ROUTER == 0      */
/*********************************************************************************************************
** 函数名称: netif_add_hook
** 功能描述: 创建网络接口回调函数
** 输　入  : pnetif     网络接口
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  netif_add_hook (struct netif *pnetif)
{
#if LW_CFG_NET_ROUTER > 0
    rt_netif_add_hook(pnetif);                                          /*  更新路由表有效标志          */
#if LWIP_IPV6
    rt6_netif_add_hook(pnetif);
#endif
#if LW_CFG_NET_BALANCING > 0
    srt_netif_add_hook(pnetif);
#if LWIP_IPV6
    srt6_netif_add_hook(pnetif);
#endif
#endif                                                                  /*  LW_CFG_NET_BALANCING > 0    */
    route_hook_netif_ann(pnetif, 0);
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
#if LW_CFG_NET_FLOWCTL_EN > 0
    fcnet_netif_attach(pnetif);
#endif                                                                  /*  LW_CFG_NET_FLOWCTL_EN > 0   */
#if LW_CFG_NET_NAT_EN > 0
    nat_netif_add_hook(pnetif);
#endif                                                                  /*  LW_CFG_NET_NAT_EN > 0       */

    netEventIfAdd(pnetif);
}
/*********************************************************************************************************
** 函数名称: netif_remove_hook
** 功能描述: 删除网络接口回调函数. (网络上下文中调用)
** 输　入  : pnetif     网络接口
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  netif_remove_hook (struct netif *pnetif)
{
    netEventIfRemove(pnetif);

#if LW_CFG_NET_ROUTER > 0
    rt_netif_remove_hook(pnetif);                                       /*  更新路由表有效标志          */
#if LWIP_IPV6
    rt6_netif_remove_hook(pnetif);
#endif
#if LW_CFG_NET_BALANCING > 0
    srt_netif_remove_hook(pnetif);
#if LWIP_IPV6
    srt6_netif_remove_hook(pnetif);
#endif
#endif                                                                  /*  LW_CFG_NET_BALANCING > 0    */
#if LW_CFG_NET_MROUTER > 0
    ip4_mrt_if_detach(pnetif);
#if LWIP_IPV6
    ip6_mrt_if_detach(pnetif);
#endif
#endif                                                                  /*  LW_CFG_NET_MROUTER > 0      */
    route_hook_netif_ann(pnetif, 1);
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
#if LW_CFG_NET_FLOWCTL_EN > 0
    fcnet_netif_detach(pnetif);
#endif                                                                  /*  LW_CFG_NET_FLOWCTL_EN > 0   */
#if LW_CFG_NET_NPF_EN > 0
    npf_netif_detach(pnetif);
#endif                                                                  /*  LW_CFG_NET_NPF_EN > 0       */
#if LW_CFG_NET_NAT_EN > 0
    nat_netif_remove_hook(pnetif);
#endif                                                                  /*  LW_CFG_NET_NAT_EN > 0       */

#if LWIP_DHCP > 0
    if (netif_dhcp_data(pnetif)) {
        netifapi_dhcp_release_and_stop(pnetif);
        dhcp_cleanup(pnetif);                                           /*  回收 DHCP 内存              */
    }
#endif                                                                  /*  LWIP_DHCP > 0               */

#if LWIP_AUTOIP > 0
    if (netif_autoip_data(pnetif)) {
        mem_free(netif_autoip_data(pnetif));                            /*  回收 AUTOIP 内存            */
        netif_set_client_data(pnetif, LWIP_NETIF_CLIENT_DATA_INDEX_AUTOIP, NULL);
    }
#endif                                                                  /*  LWIP_AUTOIP > 0             */

#if LWIP_IPV6_DHCP6 > 0
    if (netif_dhcp6_data(pnetif)) {
        netifapi_dhcp6_disable(pnetif);
        dhcp6_cleanup(pnetif);                                          /*  回收 DHCPv6 内存            */
    }
#endif                                                                  /*  LWIP_IPV6_DHCP6 > 0         */
}
/*********************************************************************************************************
** 函数名称: netif_updown_hook
** 功能描述: 网络接口使能禁能回调.
** 输　入  : pnetif     网络接口
**           up         1: 使能 0: 禁能
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  netif_updown_hook (struct netif *pnetif, INT up)
{
    if (up) {
        if (pnetif->up) {
            pnetif->up(pnetif);
        }
        netEventIfUp(pnetif);
        
    } else {
        if (pnetif->down) {
            pnetif->down(pnetif);
        }
        netEventIfDown(pnetif);
        
#if LW_CFG_NET_ROUTER > 0
        rt_netif_invcache_hook(pnetif);
#if LWIP_IPV6
        rt6_netif_invcache_hook(pnetif);
#endif                                                                  /*  LWIP_IPV6 > 0               */
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
    }
    
#if LW_CFG_NET_ROUTER > 0
    route_hook_netif_updown(pnetif);
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
}
/*********************************************************************************************************
** 函数名称: netif_link_updown_hook
** 功能描述: 网络接口 link 连接状态回调.
** 输　入  : pnetif     网络接口
**           linkup     1: 连接 0: 禁能
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  netif_link_updown_hook (struct netif *pnetif, INT linkup)
{
    if (linkup) {
        netEventIfLink(pnetif);
        
    } else {
        netEventIfUnlink(pnetif);
    
#if LW_CFG_NET_ROUTER > 0
        rt_netif_invcache_hook(pnetif);
#if LWIP_IPV6
        rt6_netif_invcache_hook(pnetif);
#endif                                                                  /*  LWIP_IPV6 > 0               */
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
    }
    
#if LW_CFG_NET_ROUTER > 0
    route_hook_netif_updown(pnetif);
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
}
/*********************************************************************************************************
** 函数名称: netif_set_addr_hook
** 功能描述: 网络接口设置地址.
** 输　入  : pnetif        网络接口
**           pipaddr       旧地址
**           iAddrType:    0: ipaddr 1: netmask 2: gateway
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  netif_set_addr_hook (struct netif *pnetif, const ip4_addr_t *pipaddr, INT  iAddrType)
{
#define NETIF_SET_IPV4_ADDR     0
#define NETIF_SET_IPV4_NETMASK  1
#define NETIF_SET_IPV4_GATEWAY  2

#if LW_CFG_NET_ROUTER > 0
    ip4_addr_t  ipaddr;
    ip4_addr_t  ipnetmask;
    
    ip4_addr_set(&ipaddr, netif_ip4_addr(pnetif));
    ip4_addr_set(&ipnetmask, netif_ip4_netmask(pnetif));
    
    switch (iAddrType) {
    
    case NETIF_SET_IPV4_ADDR:
        if (!ip4_addr_isany(pipaddr)) {
            route_hook_netif_ipv4(pnetif, pipaddr, &ipnetmask, RTM_DELADDR);
        }
        if (!ip4_addr_isany_val(ipaddr)) {
            route_hook_netif_ipv4(pnetif, &ipaddr, &ipnetmask, RTM_NEWADDR);
        }
        break;
        
    case NETIF_SET_IPV4_NETMASK:
        if (!ip4_addr_isany_val(ipaddr)) {
            route_hook_netif_ipv4(pnetif, &ipaddr, pipaddr,    RTM_DELADDR);
            route_hook_netif_ipv4(pnetif, &ipaddr, &ipnetmask, RTM_NEWADDR);
        }
        break;
        
    default:
        break;
    }
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
}
/*********************************************************************************************************
** 函数名称: netif_set_addr6_hook
** 功能描述: 网络接口设置地址.
** 输　入  : pnetif     网络接口
**           pip6addr   旧地址
**           iIndex     地址下标
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LWIP_IPV6

static VOID netif_set_addr6_hook (struct netif *pnetif, const ip6_addr_t *pip6addr, INT iIndex)
{
#if LW_CFG_NET_ROUTER > 0
    ip6_addr_t  ip6addrNew;

    if (netif_ip6_addr_state(pnetif, iIndex) & IP6_ADDR_VALID) {
        if (!ip6_addr_isany(pip6addr)) {
            route_hook_netif_ipv6(pnetif, pip6addr, RTM_DELADDR);
        }
    
        ip6_addr_set(&ip6addrNew, netif_ip6_addr(pnetif, iIndex));
        if (!ip6_addr_isany_val(ip6addrNew)) {
            route_hook_netif_ipv6(pnetif, pip6addr, RTM_NEWADDR);
        }
    }
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
}
/*********************************************************************************************************
** 函数名称: netif_set_stat6_hook
** 功能描述: 网络接口设置地址状态.
** 输　入  : pnetif     网络接口
**           pip6addr   旧地址
**           iIndex     地址下标
**           ostat      之前的状态
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID netif_set_stat6_hook (struct netif *pnetif, const ip6_addr_t *pip6addr, INT iIndex, UINT8 ostat)
{
#if LW_CFG_NET_ROUTER > 0
    UINT8       nstat = netif_ip6_addr_state(pnetif, iIndex);
    ip6_addr_t  ip6addrNew;

    if ((nstat & IP6_ADDR_VALID) && !(ostat & IP6_ADDR_VALID)) {
        ip6_addr_set(&ip6addrNew, netif_ip6_addr(pnetif, iIndex));
        if (!ip6_addr_isany_val(ip6addrNew)) {
            route_hook_netif_ipv6(pnetif, pip6addr, RTM_NEWADDR);
        }
    
    } else if (!(nstat & IP6_ADDR_VALID) && (ostat & IP6_ADDR_VALID)) {
        if (!ip6_addr_isany(pip6addr)) {
            route_hook_netif_ipv6(pnetif, pip6addr, RTM_DELADDR);
        }
    }
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** 函数名称: netif_set_maddr_hook
** 功能描述: 网络接口 添加 / 删除 组播地址.
** 输　入  : pnetif     网络接口
**           pipaddr    组播地址
**           iAdd       是否为添加
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID netif_set_maddr_hook (struct netif *pnetif, const ip4_addr_t *pipaddr, INT iAdd)
{
#if LW_CFG_NET_ROUTER > 0
    route_hook_maddr_ipv4(pnetif, pipaddr, (u_char)(iAdd ? RTM_NEWMADDR : RTM_DELMADDR));
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
}
/*********************************************************************************************************
** 函数名称: netif_set_addr6_hook
** 功能描述: 网络接口 添加 / 删除 组播地址.
** 输　入  : pvNetif    网络接口
**           pvIpaddr   新地址
**           iIndex     地址下标
**           iAdd       是否为添加
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LWIP_IPV6 && LWIP_IPV6_MLD

VOID netif_set_maddr6_hook (struct netif *pnetif, const ip6_addr_t *pip6addr, INT iAdd)
{
#if LW_CFG_NET_ROUTER > 0
    route_hook_maddr_ipv6(pnetif, pip6addr, (u_char)(iAdd ? RTM_NEWMADDR : RTM_DELMADDR));
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */
}

#endif                                                                  /*  LWIP_IPV6 && LWIP_IPV6_MLD  */
/*********************************************************************************************************
** 函数名称: netif_callback_func
** 功能描述: 网络接口回调.
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  netif_callback_func (struct netif *pnetif, netif_nsc_reason_t reason, 
                                  const netif_ext_callback_args_t *args)
{
    /*
     * netif was added. arg: NULL. Called AFTER netif was added.
     */
    if (reason & LWIP_NSC_NETIF_ADDED) {
        netif_add_hook(pnetif);
    }
    
    /*
     * netif was removed. arg: NULL. Called BEFORE netif is removed.
     */
    if (reason & LWIP_NSC_NETIF_REMOVED) {
        netif_remove_hook(pnetif);
    }
    
    /*
     * link changed.
     */
    if (reason & LWIP_NSC_LINK_CHANGED) {
        netif_link_updown_hook(pnetif, args->link_changed.state);
    }

    /*
     * netif administrative status changed.
     * up is called AFTER netif is set up.
     * down is called BEFORE the netif is actually set down.
     */
    if (reason & LWIP_NSC_STATUS_CHANGED) {
        netif_updown_hook(pnetif, args->status_changed.state);
    }

    /*
     * IPv4 address has changed.
     */
    if (reason & LWIP_NSC_IPV4_ADDRESS_CHANGED) {
        netif_set_addr_hook(pnetif, ip_2_ip4(args->ipv4_changed.old_address), NETIF_SET_IPV4_ADDR);
    }
    
    /*
     * IPv4 netmask has changed.
     */
    if (reason & LWIP_NSC_IPV4_NETMASK_CHANGED) {
        netif_set_addr_hook(pnetif, ip_2_ip4(args->ipv4_changed.old_netmask), NETIF_SET_IPV4_NETMASK);
    }
    
    /*
     * IPv4 gateway has changed.
     */
    if (reason & LWIP_NSC_IPV4_GATEWAY_CHANGED) {
        netif_set_addr_hook(pnetif, ip_2_ip4(args->ipv4_changed.old_gw), NETIF_SET_IPV4_GATEWAY);
    }
    
#if LWIP_IPV6
    /*
     * IPv6 address was added.
     */
    if (reason & LWIP_NSC_IPV6_SET) {
        netif_set_addr6_hook(pnetif, ip_2_ip6(args->ipv6_set.old_address), args->ipv6_set.addr_index);
    }
    
    /*
     * IPv6 address state has changed.
     */
    if (reason & LWIP_NSC_IPV6_ADDR_STATE_CHANGED) {
        netif_set_stat6_hook(pnetif, ip_2_ip6(args->ipv6_addr_state_changed.address), 
                             args->ipv6_addr_state_changed.addr_index, args->ipv6_addr_state_changed.old_state);
    }
#endif                                                                  /*  LWIP_IPV6                   */
}
/*********************************************************************************************************
** 函数名称: netif_callback_init
** 功能描述: 网络接口回调初始化.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID netif_callback_init (VOID)
{
    static netif_ext_callback_t netifcallback;

    netif_add_ext_callback(&netifcallback, netif_callback_func);
}
/*********************************************************************************************************
** 函数名称: netif_get_flags
** 功能描述: 通过 index 获得网络接口结构. (没有加锁)
** 输　入  : uiIndex       index
** 输　出  : 网络接口
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  netif_get_flags (struct netif *pnetif)
{
    INT  iFlags = 0;

    if (pnetif->flags & NETIF_FLAG_UP) {
        iFlags |= IFF_UP;
    }
    if (pnetif->flags & NETIF_FLAG_BROADCAST) {
        iFlags |= IFF_BROADCAST;
    } else {
        iFlags |= IFF_POINTOPOINT;
    }
    if (pnetif->flags & NETIF_FLAG_LINK_UP) {
        iFlags |= IFF_RUNNING;
    }
    if (pnetif->flags & NETIF_FLAG_IGMP) {
        iFlags |= IFF_MULTICAST;
    }
    if ((pnetif->flags & NETIF_FLAG_ETHARP) == 0) {
        iFlags |= IFF_NOARP;
    }
    if (pnetif->link_type == snmp_ifType_softwareLoopback) {
        iFlags |= IFF_LOOPBACK;
    }
    if ((pnetif->flags2 & NETIF_FLAG2_PROMISC)) {
        iFlags |= IFF_PROMISC;
    }
    if ((pnetif->flags2 & NETIF_FLAG2_ALLMULTI)) {
        iFlags |= IFF_ALLMULTI;
    }
    
    return  (iFlags);
}

#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/

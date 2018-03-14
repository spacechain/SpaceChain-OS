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
** 文   件   名: lwip_hook.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2018 年 01 月 05 日
**
** 描        述: lwip hook 声明.
*********************************************************************************************************/

#ifndef __LWIP_HOOK_H
#define __LWIP_HOOK_H

#define __SYLIXOS_KERNEL

/*********************************************************************************************************
  其他头文件
*********************************************************************************************************/

#if LW_CFG_LWIP_TCP_SIG_EN > 0
#include "tcpsig/tcp_md5.h"
#endif
#if LW_CFG_NET_MROUTER > 0
#include "netinet/in.h"
#include "netinet/ip_mroute.h"
#include "net/lwip/mroute/ip4_mrt.h"
#if LWIP_IPV6 && LWIP_IPV6_MLD
#include "netinet6/in6.h"
#include "netinet6/ip6_mroute.h"
#include "net/lwip/mroute/ip6_mrt.h"
#endif
#endif

#include "net/if_event.h"
#include "net/if_iphook.h"

/*********************************************************************************************************
  netdev 增加或删除组播地址
*********************************************************************************************************/

extern VOID  netif_set_maddr_hook(struct netif *pnetif, const ip4_addr_t *pipaddr, INT iAdd);

#if LWIP_IPV6 && LWIP_IPV6_MLD
extern VOID  netif_set_maddr6_hook(struct netif *pnetif, const ip6_addr_t *pip6addr, INT iAdd);
#endif

/*********************************************************************************************************
  IP 路由
*********************************************************************************************************/

extern struct netif *ip_route_src_hook(const ip4_addr_t *pipsrc, const ip4_addr_t *pipdest);
extern struct netif *ip_route_default_hook(const ip4_addr_t *pipsrc, const ip4_addr_t *pipdest);
extern ip4_addr_t   *ip_gw_hook(struct netif *pnetif, const ip4_addr_t *pipdest);

#if LWIP_IPV6
extern struct netif *ip6_route_src_hook(const ip6_addr_t *pip6src, const ip6_addr_t *pip6dest);
extern struct netif *ip6_route_default_hook(const ip6_addr_t *pip6src, const ip6_addr_t *pip6dest);
extern ip6_addr_t   *ip6_gw_hook(struct netif *pnetif, const ip6_addr_t *pip6dest);
#endif

/*********************************************************************************************************
  lwip ip input hook (return is the packet has been eaten)
*********************************************************************************************************/

extern int ip_input_hook(struct pbuf *p, struct netif *pnetif);

#if LWIP_IPV6
extern int ip6_input_hook(struct pbuf *p, struct netif *pnetif);
#endif

/*********************************************************************************************************
  lwip link input/output hook (for AF_PACKET and Net Defender)
*********************************************************************************************************/

extern int link_input_hook(struct pbuf *p, struct netif *pnetif);
extern int link_output_hook(struct pbuf *p, struct netif *pnetif);

/*********************************************************************************************************
  lwip vlan hook (for AF_PACKET and Net Defender)
*********************************************************************************************************/

#if LW_CFG_NET_VLAN_EN > 0
extern int ethernet_vlan_set_hook(struct netif *pnetif, struct pbuf *p, const struct eth_addr *src, 
                                  const struct eth_addr *dst, u16_t eth_type);
extern int ethernet_vlan_check_hook(struct netif *pnetif, const struct eth_hdr *ethhdr, 
                                    const struct eth_vlan_hdr *vlanhdr);
#endif

/*********************************************************************************************************
  ip hook
*********************************************************************************************************/

extern int lwip_ip_hook(int ip_type, int hook_type, struct pbuf *p, struct netif *in, struct netif *out);

#endif                                                                  /*  __LWIP_HOOK_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/

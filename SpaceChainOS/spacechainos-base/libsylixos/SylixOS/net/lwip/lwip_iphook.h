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
** 文   件   名: lwip_iphook.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 05 月 12 日
**
** 描        述: lwip IP HOOK.
*********************************************************************************************************/

#ifndef __LWIP_IPHOOK_H
#define __LWIP_IPHOOK_H

#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "netdev.h"
#include "net/if.h"
#include "net/if_type.h"

/*********************************************************************************************************
  数据包类型
*********************************************************************************************************/

#define IP_HOOK_V4      4
#define IP_HOOK_V6      6

/*********************************************************************************************************
  回调类型
  
                                               TCP/IP stack  
                                         ^                     |
                                         |                     |
                                         |                     \/
                                     [LOCAL_IN]           [LOCAL_OUT]
                                         ^                     |
                                         |                     |
                                         |                route output
                                         |                     |
                                         |                     |
                                         |                     \/
  packet input --> [PRE_ROUTING] --> route input --> [FORWARD] --> [POST_ROUTING] --> packet output 
  
  NOTICE: The NF_IP_LOCAL_OUT hook is called for packets that are created locally. Here you can see 
          that routing occurs after this hook is called: in fact, the routing code is called first 
          if you want to alter the routing, you must alter the `pbuf->if_out' field yourself.
*********************************************************************************************************/

#define IP_HT_PRE_ROUTING       0
#define IP_HT_LOCAL_IN          1
#define IP_HT_FORWARD           2
#define IP_HT_LOCAL_OUT         3
#define IP_HT_POST_ROUTING      4

/*********************************************************************************************************
  内核函数
*********************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

int  net_ip_hook_add(const char *name, int (*hook)(int ip_type, int hook_type, struct pbuf *p, 
                                                   struct netif *in, struct netif *out));
int  net_ip_hook_delete(int (*hook)(int ip_type, int hook_type, struct pbuf *p, 
                                    struct netif *in, struct netif *out));
int  net_ip_hook_isadd(int (*hook)(int ip_type, int hook_type, struct pbuf *p,
                                   struct netif *in, struct netif *out), BOOL *pbIsAdd);

/*********************************************************************************************************
  设置 pbuf 成员
  
  注意: 出于兼容性考虑, 不允许直接设置 if_out 成员变量.
*********************************************************************************************************/

void net_ip_hook_pbuf_set_ifout(struct pbuf *p, struct netif *pnetif);

/*********************************************************************************************************
  获取 netif 成员
  
  注意: 出于兼容性考虑, 不允许直接从 netif 结构中访问数据, 必须要通过以下接口函数来访问 netif 中的成员.
*********************************************************************************************************/

netdev_t          *net_ip_hook_netif_get_netdev(struct netif *pnetif);
const ip4_addr_t  *net_ip_hook_netif_get_ipaddr(struct netif *pnetif);
const ip4_addr_t  *net_ip_hook_netif_get_netmask(struct netif *pnetif);
const ip4_addr_t  *net_ip_hook_netif_get_gw(struct netif *pnetif);
const ip6_addr_t  *net_ip_hook_netif_get_ip6addr(struct netif *pnetif, int  addr_index, int *addr_state);
UINT8             *net_ip_hook_netif_get_hwaddr(struct netif *pnetif, int *hwaddr_len);
int                net_ip_hook_netif_get_index(struct netif *pnetif);
int                net_ip_hook_netif_get_name(struct netif *pnetif, char *name, size_t size);
int                net_ip_hook_netif_get_type(struct netif *pnetif, int *type);
int                net_ip_hook_netif_get_flags(struct netif *pnetif, int *flags);
UINT64             net_ip_hook_netif_get_linkspeed(struct netif *pnetif);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __LWIP_IPHOOK_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/

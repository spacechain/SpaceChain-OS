/**
 * @file
 * Lwip platform independent driver interface.
 * This set of driver interface shields the netif details, 
 * as much as possible compatible with different versions of LwIP
 * Verification using sylixos(tm) real-time operating system
 */

/*
 * Copyright (c) 2006-2017 SylixOS Group.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 * 4. This code has been or is applying for intellectual property protection 
 *    and can only be used with acoinfo software products.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 * 
 * Author: Han.hui <hanhui@acoinfo.com>
 *
 */

#define  __SYLIXOS_KERNEL
#include "lwip/pbuf.h"
#include "lwip/inet.h"
#include "lwip/ethip6.h"
#include "lwip/etharp.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "netif/ethernet.h"
#include "net/if_lock.h"
#include "net/if_flags.h"
#include "net/if_param.h"
#include "net/if_ether.h"

#include "string.h"
#include "netdev.h"

#ifdef LWIP_HOOK_FILENAME
#include LWIP_HOOK_FILENAME
#endif

#define NETDEV_INIT(netdev)             if ((netdev)->drv->init) { (netdev)->drv->init((netdev)); }
#define NETDEV_UP(netdev)               if ((netdev)->drv->up) { (netdev)->drv->up((netdev)); }
#define NETDEV_DOWN(netdev)             if ((netdev)->drv->down) { (netdev)->drv->down((netdev)); }
#define NETDEV_REMOVE(netdev)           if ((netdev)->drv->remove) { (netdev)->drv->remove((netdev)); }
#define NETDEV_IOCTL(netdev, a, b)      if ((netdev)->drv->ioctl) { (netdev)->drv->ioctl((netdev), (a), (b)); }
#define NETDEV_PROMISC(netdev, a, b)    if ((netdev)->drv->promisc) { (netdev)->drv->promisc((netdev), (a), (b)); }
#define NETDEV_RXMODE(netdev, a)        if ((netdev)->drv->rxmode) { (netdev)->drv->rxmode((netdev), (a)); }
#define NETDEV_TRANSMIT(netdev, a)      (netdev)->drv->transmit((netdev), (a))
#define NETDEV_RECEIVE(netdev, input)   (netdev)->drv->receive((netdev), (input))

/* functions declaration */
static struct netdev_mac *netdev_macfilter_find(netdev_t *netdev, const UINT8 hwaddr[], struct netdev_mac **prev_save);
static void netdev_macfilter_clean(netdev_t *netdev);

/* lwip netif up hook function */
static void  netdev_netif_up (struct netif *netif)
{
  netdev_t *netdev = (netdev_t *)(netif->state);
  
  NETDEV_UP(netdev);
  netdev->if_flags |= IFF_UP;
}

/* lwip netif down hook function */
static void  netdev_netif_down (struct netif *netif)
{
  netdev_t *netdev = (netdev_t *)(netif->state);
  
  NETDEV_DOWN(netdev);
  netdev->if_flags &= ~IFF_UP;
}

/* lwip netif remove hook function */
static void  netdev_netif_remove (struct netif *netif)
{
  netdev_t *netdev = (netdev_t *)(netif->state);
  
  NETDEV_REMOVE(netdev);
}

/* lwip netif igmp mac filter hook function */
static err_t  netdev_netif_igmp_mac_filter (struct netif *netif,
                                            const ip4_addr_t *group, 
                                            enum netif_mac_filter_action action)
{
  netdev_t *netdev = (netdev_t *)(netif->state);
  struct netdev_mac *mac, *prev;
  UINT8 hwaddr[NETIF_MAX_HWADDR_LEN];
  int flags;
  
  if (netdev->net_type != NETDEV_TYPE_ETHERNET) {
    return (ERR_OK);
  }
  
  if (!ip4_addr_ismulticast(group)) {
    return (ERR_VAL);
  }
  
  hwaddr[0] = LL_IP4_MULTICAST_ADDR_0;
  hwaddr[1] = LL_IP4_MULTICAST_ADDR_1;
  hwaddr[2] = LL_IP4_MULTICAST_ADDR_2;
  hwaddr[3] = ip4_addr2(group) & 0x7f;
  hwaddr[4] = ip4_addr3(group);
  hwaddr[5] = ip4_addr4(group);
  
  mac = netdev_macfilter_find(netdev, hwaddr, &prev);
  if (action == NETIF_DEL_MAC_FILTER) {
    if (!mac) {
      return (ERR_VAL);
    }
    if (mac->ref > 1) {
      mac->ref--;
      return (ERR_OK);
    }
    if (prev) {
      prev->next = mac->next;
    } else {
      netdev->mac_filter = mac->next;
    }
    
    mem_free(mac);
    netif_set_maddr_hook(netif, group, 0);
    
    flags = netif_get_flags(netif);
    if (!(flags & (IFF_PROMISC | IFF_ALLMULTI))) {
      NETDEV_RXMODE(netdev, flags);
    }
    
  } else {
    if (mac) {
      mac->ref++;
      return (ERR_OK);
    }
    
    mac = (struct netdev_mac *)mem_malloc(sizeof(struct netdev_mac));
    if (!mac) {
      return (ERR_MEM);
    }
    
    mac->nouse = NULL;
    mac->type  = NETDEV_MAC_TYPE_MULTICAST;
    mac->ref   = 1;
    MEMCPY(mac->hwaddr, hwaddr, netdev->hwaddr_len);
    
    mac->next = netdev->mac_filter;
    netdev->mac_filter = mac;
    netif_set_maddr_hook(netif, group, 1);
    
    flags = netif_get_flags(netif);
    if (!(flags & (IFF_PROMISC | IFF_ALLMULTI))) {
      NETDEV_RXMODE(netdev, flags);
    }
  }
  
  return (ERR_OK);
}

#if LWIP_IPV6 && LWIP_IPV6_MLD
/* lwip netif mld mac filter hook function */
static err_t  netdev_netif_mld_mac_filter (struct netif *netif,
                                           const ip6_addr_t *group, 
                                           enum netif_mac_filter_action action)
{
  netdev_t *netdev = (netdev_t *)(netif->state);
  struct netdev_mac *mac, *prev;
  UINT8 hwaddr[NETIF_MAX_HWADDR_LEN];
  int flags;
  
  if (netdev->net_type != NETDEV_TYPE_ETHERNET) {
    return (ERR_OK);
  }
  
  if (!ip6_addr_ismulticast(group)) {
    return (ERR_VAL);
  }
  
  hwaddr[0] = LL_IP6_MULTICAST_ADDR_0;
  hwaddr[1] = LL_IP6_MULTICAST_ADDR_1;
  hwaddr[2] = ((UINT8 *)(&(group->addr[3])))[0];
  hwaddr[3] = ((UINT8 *)(&(group->addr[3])))[1];
  hwaddr[4] = ((UINT8 *)(&(group->addr[3])))[2];
  hwaddr[5] = ((UINT8 *)(&(group->addr[3])))[3];
  
  mac = netdev_macfilter_find(netdev, hwaddr, &prev);
  if (action == NETIF_DEL_MAC_FILTER) {
    if (!mac) {
      return (ERR_VAL);
    }
    if (mac->ref > 1) {
      mac->ref--;
      return (ERR_OK);
    }
    if (prev) {
      prev->next = mac->next;
    } else {
      netdev->mac_filter = mac->next;
    }
    
    mem_free(mac);
    netif_set_maddr6_hook(netif, group, 0);
    
    flags = netif_get_flags(netif);
    if (!(flags & (IFF_PROMISC | IFF_ALLMULTI))) {
      NETDEV_RXMODE(netdev, flags);
    }
    
  } else {
    if (mac) {
      mac->ref++;
      return (ERR_OK);
    }
    
    mac = (struct netdev_mac *)mem_malloc(sizeof(struct netdev_mac));
    if (!mac) {
      return (ERR_MEM);
    }
    
    mac->nouse = NULL;
    mac->type  = NETDEV_MAC_TYPE_MULTICAST;
    mac->ref   = 1;
    MEMCPY(mac->hwaddr, hwaddr, netdev->hwaddr_len);
    
    mac->next = netdev->mac_filter;
    netdev->mac_filter = mac;
    netif_set_maddr6_hook(netif, group, 1);
    
    flags = netif_get_flags(netif);
    if (!(flags & (IFF_PROMISC | IFF_ALLMULTI))) {
      NETDEV_RXMODE(netdev, flags);
    }
  }
  
  return (ERR_OK);
}
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

/* lwip netif ioctl hook function */
static int  netdev_netif_ioctl (struct netif *netif, int cmd, void *arg)
{
  netdev_t *netdev = (netdev_t *)(netif->state);
  struct ifreq *ifreq;
  err_t err = ERR_VAL;
  int flags;
  int ret = -1;
  
  switch (cmd) {

  case SIOCSIFHWADDR:  /* set hwaddr */
    if (netdev->drv->ioctl) {
      ifreq = (struct ifreq *)arg;
      ret = netdev->drv->ioctl(netdev, cmd, arg);
      if (ret == 0) {
        MEMCPY(netdev->hwaddr, ifreq->ifr_hwaddr.sa_data, netdev->hwaddr_len);
      }
    }
    break;

  case SIOCSIFMTU:    /* set mtu */
    if (netdev->drv->ioctl) {
      ifreq = (struct ifreq *)arg;
      ret = netdev->drv->ioctl(netdev, cmd, arg);
      if (ret == 0) {
        netdev->mtu = ifreq->ifr_mtu;
      }
    }
    break;

  case SIOCSIFFLAGS:  /* set flags */
    if (netdev->drv->rxmode) {
      ifreq = (struct ifreq *)arg;
      flags = netif_get_flags(netif);
      if ((flags & (IFF_PROMISC | IFF_ALLMULTI)) != 
          (ifreq->ifr_flags & (IFF_PROMISC | IFF_ALLMULTI))) { /* rx mode changed */
        ret = netdev->drv->rxmode(netdev, ifreq->ifr_flags);
      } else {
        ret = 0; /* do not allow to change other flags */
      }
    }
    break;

  case SIOCADDMULTI:  /* add / del mcast addr */
  case SIOCDELMULTI:
    ifreq = (struct ifreq *)arg;
    if (ifreq->ifr_addr.sa_family == AF_INET) {
      ip4_addr_t group4;
      inet_addr_to_ip4addr(&group4, &((struct sockaddr_in *)&ifreq->ifr_addr)->sin_addr);
      err = netdev_netif_igmp_mac_filter(netif, &group4, ((cmd == SIOCADDMULTI) ? 
                                         NETIF_ADD_MAC_FILTER : NETIF_DEL_MAC_FILTER));
    }
#if LWIP_IPV6 && LWIP_IPV6_MLD
      else if (ifreq->ifr_addr.sa_family == AF_INET6) {
      ip6_addr_t group6;
      inet6_addr_to_ip6addr(&group6, &((struct sockaddr_in6 *)&ifreq->ifr_addr)->sin6_addr);
      err = netdev_netif_mld_mac_filter(netif, &group6, ((cmd == SIOCADDMULTI) ? 
                                        NETIF_ADD_MAC_FILTER : NETIF_DEL_MAC_FILTER));
    }
#endif
    if (err) {
      if (err == ERR_MEM) {
        errno = ENOMEM;
      } else if (err == ERR_VAL) {
        errno = EINVAL;
      }
    } else {
      ret = 0;
    }
    break;
      
  default:
    if (netdev->drv->ioctl) {
      ret = netdev->drv->ioctl(netdev, cmd, arg);
    }
    break;
  }
  
  return (ret);
}

/* lwip netif rawoutput hook function */
static err_t  netdev_netif_rawoutput4 (struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr)
{
  netdev_t *netdev = (netdev_t *)(netif->state);
  int ret;

  ret = NETDEV_TRANSMIT(netdev, p);
  if (ret < 0) {
    return (ERR_IF);
  }
  
  return (ERR_OK);
}

static err_t  netdev_netif_rawoutput6 (struct netif *netif, struct pbuf *p, const ip6_addr_t *ip6addr)
{
  netdev_t *netdev = (netdev_t *)(netif->state);
  int ret;

  ret = NETDEV_TRANSMIT(netdev, p);
  if (ret < 0) {
    return (ERR_IF);
  }
  
  return (ERR_OK);
}

/* lwip netif linkoutput hook function */
static err_t  netdev_netif_linkoutput (struct netif *netif, struct pbuf *p)
{
  netdev_t *netdev = (netdev_t *)(netif->state);
  int ret;

  LWIP_ASSERT("netdev linkoutput must ethernet type.", 
              (netdev->net_type == NETDEV_TYPE_ETHERNET)); /* must a ethernet */

#if ETH_PAD_SIZE
  pbuf_header(p, -ETH_PAD_SIZE);
#endif

  ret = NETDEV_TRANSMIT(netdev, p);
  
#if ETH_PAD_SIZE
  pbuf_header(p, ETH_PAD_SIZE);
#endif

  if (ret < 0) {
    return (ERR_IF);
  }
  
  return (ERR_OK);
}

/* lwip netif linkinput hook function */
static int  netdev_netif_linkinput (netdev_t *netdev, struct pbuf *p)
{
  struct netif *netif = (struct netif *)netdev->sys;
  struct eth_hdr *eh;
  
  /* fixed pad */
  if (netdev->net_type == NETDEV_TYPE_ETHERNET) {
#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE);
#endif

    /* adjust pbuf length */
    eh = (struct eth_hdr *)p->payload;
    if (eh->type == PP_HTONS(ETHTYPE_VLAN)) {
      if (p->tot_len > (netif->mtu + SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR)) {
        pbuf_realloc(p, (u16_t)(netif->mtu + SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR));
      }
    
    } else {
      if (p->tot_len > (netif->mtu + SIZEOF_ETH_HDR)) {
        pbuf_realloc(p, (u16_t)(netif->mtu + SIZEOF_ETH_HDR));
      }
    }
  }

  if (netif->inner_fw && netif->inner_fw(netif, p)) {
    return (0); /* inner firewall eaten */
  }

  if (netif->outer_fw && netif->outer_fw(netdev, p)) {
    return (0); /* outer firewall eaten */
  }
  
  if (netdev->poll.poll_mode == NETDEV_POLLMODE_EN) {
    if (netdev->poll.poll_input(netdev, p)) {
      return (0); /* poll hook eaten */
    }
  }

  if (netif->input(p, netif)) {
    return (-1);
    
  } else {
    return (0);
  }
}

/* lwip netif linkup changed */
static void  netdev_netif_linkup (netdev_t *netdev, int linkup, UINT32 speed_high, UINT32 speed_low)
{
  UINT64 speed = ((UINT64)speed_high << 32) + speed_low;
  struct netif *netif;
  
  if (!netdev || (netdev->magic_no != NETDEV_MAGIC)) {
    return;
  }
  
  netif = (struct netif *)netdev->sys;
  
  if (linkup) {
    netif->ts = sys_jiffies();
    netdev->speed = speed;
    
    netifapi_netif_set_link_up(netif);
    netdev->if_flags |= IFF_RUNNING;

    if (speed > 0xffffffff) {
      netif->link_speed = 0;
    } else {
      netif->link_speed = (u32_t)speed;
    }
  
  } else {
    netifapi_netif_set_link_down(netif);
    netdev->if_flags &= ~IFF_RUNNING;
  }
}

/* lwip netif add call back function */
static err_t  netdev_netif_init (struct netif *netif)
{
  netdev_t *netdev = (netdev_t *)(netif->state);

#if LWIP_NETIF_HOSTNAME
  netif->hostname = netdev->if_hostname;
#endif /* LWIP_NETIF_HOSTNAME */

  netif->name[0] = netdev->if_name[0];
  netif->name[1] = netdev->if_name[1];

  switch (netdev->net_type) {
  
  case NETDEV_TYPE_ETHERNET:
    MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, (u32_t)netdev->speed);
    netif->flags = NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;
    netif->output = etharp_output;
#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
    break;
  
  default:
    MIB2_INIT_NETIF(netif, snmp_ifType_other, (u32_t)netdev->speed);
    netif->flags = 0;
    netif->output = netdev_netif_rawoutput4;
#if LWIP_IPV6
    netif->output_ip6 = netdev_netif_rawoutput6;
#endif /* LWIP_IPV6 */
    break;
  }
  
  netif->linkoutput = netdev_netif_linkoutput;

  netif->mtu = (u16_t)netdev->mtu;
  
  netif->chksum_flags = (u16_t)netdev->chksum_flags;
  
  if (netdev->init_flags & NETDEV_INIT_IPV6_AUTOCFG) {
    netif->ip6_autoconfig_enabled = 1;
  }
  
  if (netdev->if_flags & IFF_UP) {
    netif->flags |= NETIF_FLAG_UP;
  }
  
  if (netdev->if_flags & IFF_BROADCAST) {
    netif->flags |= NETIF_FLAG_BROADCAST;
  }
  
  if (netdev->if_flags & IFF_POINTOPOINT) {
    netif->flags &= ~NETIF_FLAG_BROADCAST;
  }
  
  if (netdev->if_flags & IFF_RUNNING) {
    netif->flags |= NETIF_FLAG_LINK_UP;
  }
  
  if (netdev->if_flags & IFF_MULTICAST) {
    netif->flags |= NETIF_FLAG_IGMP | NETIF_FLAG_MLD6;
#if LWIP_IPV4 && LWIP_IGMP
    netif->igmp_mac_filter = netdev_netif_igmp_mac_filter;
#endif /* LWIP_IPV4 && LWIP_IGMP */
#if LWIP_IPV6 && LWIP_IPV6_MLD
    netif->mld_mac_filter = netdev_netif_mld_mac_filter;
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
  }
  
  if (netdev->if_flags & IFF_NOARP) {
    netif->flags &= ~NETIF_FLAG_ETHARP;
  }
  
#if LWIP_NETIF_REMOVE_CALLBACK
  netif->remove_callback = netdev_netif_remove;
#endif /* LWIP_NETIF_REMOVE_CALLBACK */
  
  netif->up = netdev_netif_up;
  netif->down = netdev_netif_down;
  netif->ioctl = netdev_netif_ioctl;
  
  netif->flags2 = 0;
  if (netdev->if_flags & IFF_PROMISC) {
    netif->flags2 |= NETIF_FLAG2_PROMISC;
  }
  if (netdev->if_flags & IFF_ALLMULTI) {
    netif->flags2 |= NETIF_FLAG2_ALLMULTI;
  }
  
  if (!(netdev->init_flags & NETDEV_INIT_DO_NOT)) {
    if (netdev->drv->init) {
      if (netdev->drv->init(netdev) < 0) {
        return (ERR_IF);
      } 
    }
  }
  
  /* Update netif hwaddr */
  netif->hwaddr_len = (u8_t)((netdev->hwaddr_len < NETIF_MAX_HWADDR_LEN)
                    ? netdev->hwaddr_len
                    : NETIF_MAX_HWADDR_LEN);

  SMEMCPY(netif->hwaddr, netdev->hwaddr, netif->hwaddr_len);
  
  if (netdev->if_flags & IFF_UP) {
    NETDEV_UP(netdev);
  }
  
#if LWIP_IPV6 && LWIP_IPV6_MLD
  /*
   * For hardware/netifs that implement MAC filtering.
   * All-nodes link-local is handled by default, so we must let the hardware know
   * to allow multicast packets in.
   * Should set mld_mac_filter previously. */
  if (netif->mld_mac_filter != NULL) {
    ip6_addr_t ip6_allnodes_ll;
    ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
    netif->mld_mac_filter(netif, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
  }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

  NETDEV_RXMODE(netdev, netdev->if_flags); /* init rxmode */

  return (ERR_OK);
}

/* netdev driver call the following functions add a network interface,
 * if this device not in '/etc/if_param.ini' ip, netmask, gw is default configuration.
 * 'if_flags' defined in net/if.h such as IFF_UP, IFF_BROADCAST, IFF_RUNNING, IFF_NOARP, IFF_MULTICAST, IFF_PROMISC ... */
int  netdev_add (netdev_t *netdev, const char *ip, const char *netmask, const char *gw, int if_flags)
{
  ip4_addr_t ip4, netmask4, gw4;
  struct netif *netif;
  struct netdev_funcs *drv;
  void  *ifparam;
  int  i, enable, def, dhcp, autocfg;
  char macstr[32];
  int  mac[NETIF_MAX_HWADDR_LEN];

  if (!netdev || (netdev->magic_no != NETDEV_MAGIC) || !netdev->drv) {
    _DebugHandle(__ERRORMESSAGE_LEVEL, 
                 "netdev driver version not matching to current system.\r\n");
    return (-1);
  }
  
  if ((netdev->if_name[0] == '\0') || (netdev->if_name[1] == '\0')) {
    return (-1);
  }
  
  if ((netdev->hwaddr_len != 6) && (netdev->hwaddr_len != 8)) {
    return (-1);
  }
  
  drv = netdev->drv;
  if (!drv->transmit || !drv->receive) {
    return (-1);
  }
  
  netif = (struct netif *)netdev->sys;
  lib_bzero(netif, sizeof(struct netif));
  
  if (ip) {
    ip4.addr = inet_addr(ip);
  } else {
    ip4.addr = IPADDR_ANY;
  }
  
  if (netmask) {
    netmask4.addr = inet_addr(netmask);
  } else {
    netmask4.addr = IPADDR_ANY;
  }
  
  if (gw) {
    gw4.addr = inet_addr(gw);
  } else {
    gw4.addr = IPADDR_ANY;
  }
  
  netdev->if_flags = if_flags;
  netdev->mac_filter = NULL;
  netdev->poll.poll_mode = NETDEV_POLLMODE_DIS; /* Initialize to normal mode */
  
  if (netdev->init_flags & NETDEV_INIT_LOAD_PARAM) {
    ifparam = if_param_load(netdev->dev_name);
    if (ifparam) {
      if_param_getenable(ifparam, &enable);
      if (enable) {
        netdev->if_flags |= IFF_UP;
      } else {
        netdev->if_flags &= ~IFF_UP;
      }
    
      if_param_getdefault(ifparam, &def);
      if (def) {
        netdev->init_flags |= NETDEV_INIT_AS_DEFAULT;
      } else {
        netdev->init_flags &= ~NETDEV_INIT_AS_DEFAULT;
      }
      
      if_param_getipaddr(ifparam, &ip4);
      if_param_getnetmask(ifparam, &netmask4);
      if_param_getgw(ifparam, &gw4);

      if (!if_param_getmac(ifparam, macstr, sizeof(macstr))) {
        if (netdev->hwaddr_len == 6) {
          if (sscanf(macstr, "%x:%x:%x:%x:%x:%x", 
                     &mac[0], &mac[1], &mac[2], 
                     &mac[3], &mac[4], &mac[5]) == 6) {
            for (i = 0; i < 6; i++) {
              netdev->hwaddr[i] = (UINT8)mac[i];
            }
          }
        } else {
          if (sscanf(macstr, "%x:%x:%x:%x:%x:%x:%x:%x", 
                     &mac[0], &mac[1], &mac[2], &mac[3], 
                     &mac[4], &mac[5], &mac[6], &mac[7]) == 8) {
            for (i = 0; i < 8; i++) {
              netdev->hwaddr[i] = (UINT8)mac[i];
            }
          }
        }
      }
      
#if LWIP_IPV6
      if (!if_param_ipv6autocfg(ifparam, &autocfg)) {
        if (autocfg) {
          netdev->init_flags |= NETDEV_INIT_IPV6_AUTOCFG;
        } else {
          netdev->init_flags &= ~NETDEV_INIT_IPV6_AUTOCFG;
        }
      }
#endif /* LWIP_IPV6 */
      
#if LWIP_DHCP > 0
      if (!(netdev->init_flags & NETDEV_INIT_USE_DHCP)) {
        if_param_getdhcp(ifparam, &dhcp);
        if (dhcp) {
          netdev->init_flags |= NETDEV_INIT_USE_DHCP;
        }
      }
      if (netdev->init_flags & NETDEV_INIT_USE_DHCP) {
        ip4.addr = IPADDR_ANY;
        netmask4.addr = IPADDR_ANY;
        gw4.addr = IPADDR_ANY;
      }
#endif /* LWIP_DHCP */

      if_param_unload(ifparam);
    }
  }
  
  if (netdev->init_flags & NETDEV_INIT_LOAD_DNS) {
    if_param_syncdns();
  }
  
  if (netifapi_netif_add(netif, &ip4, &netmask4, &gw4, netdev, netdev_netif_init, tcpip_input)) {
    return (-1);
  }

  netif_get_name(netif, netdev->if_name); /* update netdev if_name */

#if LWIP_IPV6
  netif_create_ip6_linklocal_address(netif, 1);
#endif /* LWIP_IPV6 */
  
  if (netdev->init_flags & NETDEV_INIT_AS_DEFAULT) {
    netifapi_netif_set_default(netif);
  }
  
#if LWIP_DHCP > 0
  if (netdev->init_flags & NETDEV_INIT_USE_DHCP) {
    netif->flags2 |= NETIF_FLAG2_DHCP;
    netifapi_dhcp_start(netif);
  }
#endif /* LWIP_DHCP */

  return (0);
}

/* netdev driver call the following functions delete a network interface 
 * WARNING: You MUST DO NOT lock device then call this function, it will cause a deadlock with TCP LOCK */
int  netdev_delete (netdev_t *netdev)
{
  struct netif *netif;
  
  if (!netdev || (netdev->magic_no != NETDEV_MAGIC)) {
    return (-1);
  }
  
  netif = (struct netif *)netdev->sys;
  
  LWIP_IF_LIST_LOCK(LW_TRUE);
  netifapi_netif_remove(netif);
  LWIP_IF_LIST_UNLOCK();
  
  netdev_macfilter_clean(netdev);
  netJobDeleteEx(LW_NETJOB_Q_ALL, 1, NULL, netdev, 0, 0, 0, 0, 0); /* delete all netjob message */
  
  return (0);
}

/* netdev driver get netdev index */
int  netdev_index (netdev_t *netdev, unsigned int *index)
{
  struct netif *netif;
  
  if (!netdev || (netdev->magic_no != NETDEV_MAGIC)) {
    return (-1);
  }
  
  netif = (struct netif *)netdev->sys;
  
  if (index) {
    *index = netif_get_index(netif);
    return (0);
    
  } else {
    return (-1);
  }
}

/* netdev set firewall */
int  netdev_firewall (netdev_t *netdev, int (*fw)(netdev_t *, struct pbuf *))
{
  struct netif *netif;

  if (!netdev || (netdev->magic_no != NETDEV_MAGIC)) {
    return (-1);
  }
  
  netif = (struct netif *)netdev->sys;
  netif->outer_fw = (int (*)(void *, struct pbuf *))fw;
  
  return (0);
}

/* netdev traversal */
int  netdev_foreache (FUNCPTR pfunc, void *arg0, void *arg1, 
                      void *arg2, void *arg3, void *arg4, void *arg5)
{
  struct netif *netif;
  netdev_t     *netdev;
  
  if (!pfunc) {
    return (-1);
  }
  
  NETIF_FOREACH(netif) {
    if (netif->ioctl == netdev_netif_ioctl) {
      netdev = (netdev_t *)(netif->state);
      if (pfunc(netdev, arg0, arg1, arg2, arg3, arg4, arg5)) {
        break;
      }
    }
  }
  
  return (0);
}

/* netdev start poll mode */
int  netdev_poll_enable (netdev_t *netdev, int (*poll_input)(struct netdev *, struct pbuf *), void *poll_arg)
{
  if (!netdev || !poll_input || (netdev->magic_no != NETDEV_MAGIC)) {
    return (-1);
  }
  
  if (!netdev->drv->pollrecv || !netdev->drv->intctl) {
    errno = ENOTSUP;
    return (-1);
  }
  
  if (netdev->poll.poll_mode == NETDEV_POLLMODE_EN) {
    errno = EBUSY;
    return (-1);
  }
  
  if (netdev->drv->intctl(netdev, 0)) {
    return (-1);
  }
  
  netdev->poll.poll_arg = poll_arg;
  netdev->poll.poll_input = poll_input;
  netdev->poll.poll_mode = NETDEV_POLLMODE_EN;
  
  return (0);
}

/* netdev stop poll mode */
int  netdev_poll_disable (netdev_t *netdev)
{
  if (!netdev || (netdev->magic_no != NETDEV_MAGIC)) {
    return (-1);
  }
  
  if (netdev->poll.poll_mode == NETDEV_POLLMODE_DIS) {
    return (0);
  }
  
  netdev->poll.poll_mode = NETDEV_POLLMODE_DIS;
  netdev->drv->intctl(netdev, 1);
  
  return (0);
}

/* netdev poll mode service */
int  netdev_poll_svc (netdev_t *netdev)
{
  if (!netdev || (netdev->magic_no != NETDEV_MAGIC)) {
    return (-1);
  }
  
  if (netdev->poll.poll_mode == NETDEV_POLLMODE_DIS) {
    return (-1);
  }
  
  if (netdev->drv->pollrecv) {
    netdev->drv->pollrecv(netdev);
  }
  
  return (0);
}

/* netdev find (MUST in NETIF_LOCK mode) */
netdev_t *netdev_find_by_index (unsigned int index)
{
  struct netif *netif;
  netdev_t     *netdev;
  
  netif = netif_get_by_index((u8_t)index);
  if (netif && (netif->ioctl == netdev_netif_ioctl)) {
    netdev = (netdev_t *)(netif->state);
    if (netdev && (netdev->magic_no == NETDEV_MAGIC)) {
      return (netdev);
    }
  }
  
  return (NULL);
}

netdev_t *netdev_find_by_ifname (const char *if_name)
{
  struct netif *netif;
  netdev_t     *netdev;
  
  if (!if_name) {
    return (NULL);
  }
  
  netif = netif_find(if_name);
  if (netif && (netif->ioctl == netdev_netif_ioctl)) {
    netdev = (netdev_t *)(netif->state);
    if (netdev && (netdev->magic_no == NETDEV_MAGIC)) {
      return (netdev);
    }
  }
  
  return (NULL);
}

netdev_t *netdev_find_by_devname (const char *dev_name)
{
  struct netif *netif;
  netdev_t     *netdev;

  if (!dev_name) {
    return (NULL);
  }
  
  NETIF_FOREACH(netif) {
    if (netif->ioctl == netdev_netif_ioctl) {
      netdev = (netdev_t *)(netif->state);
      if (netdev && (netdev->magic_no == NETDEV_MAGIC)) {
        if (lib_strcmp(netdev->dev_name, dev_name) == 0) {
          return (netdev);
        }
      }
    }
  }
  
  return (NULL);
}

/* netdev get name */
int  netdev_ifname (netdev_t *netdev, char *ifname)
{
  struct netif *netif;
  
  if (!netdev || (netdev->magic_no != NETDEV_MAGIC)) {
    return (-1);
  }
  
  netif = (struct netif *)netdev->sys;
  if (ifname) {
    netif_get_name(netif, ifname);
  }
  
  return (0);
}

/* if netdev link status changed has been detected, 
 * driver must call the following functions 
 * NOTICE: In order to avoid deadlocks (between TCPIP LOCK and Device Lock. so wei use netjob do this) */
int  netdev_set_linkup (netdev_t *netdev, int linkup, UINT64 speed)
{
  UINT32 speed_high = (UINT32)((speed >> 32) & 0xffffffff);
  UINT32 speed_low = (UINT32)(speed & 0xffffffff);

  return (netJobAdd(netdev_netif_linkup, netdev, 
                    (void *)linkup, (void *)speed_high, (void *)speed_low, 0, 0));
}

int  netdev_get_linkup (netdev_t *netdev, int *linkup)
{
  struct netif *netif;
  
  if (!netdev || (netdev->magic_no != NETDEV_MAGIC)) {
    return (-1);
  }
  
  netif = (struct netif *)netdev->sys;
  
  if (linkup) {
    if (netif_is_link_up(netif)) {
      *linkup = 1;
    } else {
      *linkup = 0;
    }
  }
  
  return (0);
}

/* netdev linkup watchdog function 
 * NOTICE: one netdev can ONLY add one linkup_wd function.
 *         when netdev removed driver must delete watchdog function manually. */
int  netdev_linkup_wd_add (netdev_t *netdev, void  (*linkup_wd)(netdev_t *))
{
#if LW_CFG_HOTPLUG_EN > 0
  if (netdev && linkup_wd) {
    return (hotplugPollAdd(linkup_wd, netdev));
  }
#endif /* LW_CFG_HOTPLUG_EN */
  return (-1);
}

int  netdev_linkup_wd_delete (netdev_t *netdev, void  (*linkup_wd)(netdev_t *))
{
#if LW_CFG_HOTPLUG_EN > 0
  if (netdev && linkup_wd) {
    return (hotplugPollDelete(linkup_wd, netdev));
  }
#endif /* LW_CFG_HOTPLUG_EN */
  return (-1);
}

/* netdev mac filter is empty */
int  netdev_macfilter_isempty (netdev_t *netdev)
{
  return (!netdev->mac_filter);
}

/* netdev mac filter cnt */
int  netdev_macfilter_count (netdev_t *netdev)
{
  struct netdev_mac *ha;
  int cnt = 0;
  
  NETDEV_MACFILTER_FOREACH(netdev, ha) {
    cnt++;
  }
  
  return (cnt);
}

/* netdev mac filter add a hwaddr and allow to recv */
int  netdev_macfilter_add (netdev_t *netdev, const UINT8 hwaddr[])
{
  struct netdev_mac *mac, *prev;
  int type, flags;
  
  if (netdev->net_type != NETDEV_TYPE_ETHERNET) {
    return (-1);
  }
  
  if ((hwaddr[0] == LL_IP4_MULTICAST_ADDR_0) &&
      (hwaddr[1] == LL_IP4_MULTICAST_ADDR_1) &&
      (hwaddr[2] == LL_IP4_MULTICAST_ADDR_2)) {
    type = NETDEV_MAC_TYPE_MULTICAST;
  
  } else if ((hwaddr[0] == LL_IP6_MULTICAST_ADDR_0) &&
             (hwaddr[1] == LL_IP6_MULTICAST_ADDR_1)) {
    type = NETDEV_MAC_TYPE_MULTICAST;
  
  } else {
    type = NETDEV_MAC_TYPE_UNICAST;
  }
  
  mac = netdev_macfilter_find(netdev, hwaddr, &prev);
  if (mac) {
    mac->ref++;
    return (0);
  }
  
  mac = (struct netdev_mac *)mem_malloc(sizeof(struct netdev_mac));
  if (!mac) {
    errno = ENOMEM;
    return (-1);
  }
  
  mac->nouse = NULL;
  mac->type  = type;
  mac->ref   = 1;
  MEMCPY(mac->hwaddr, hwaddr, netdev->hwaddr_len);
  
  mac->next = netdev->mac_filter;
  netdev->mac_filter = mac;
  
  flags = netif_get_flags((struct netif *)(netdev->sys));
  if (!(flags & (IFF_PROMISC | IFF_ALLMULTI)) || 
      (type != NETDEV_MAC_TYPE_MULTICAST)) {
    NETDEV_RXMODE(netdev, flags);
  }
  
  return (0);
}

/* netdev mac filter delete a hwaddr */
int  netdev_macfilter_delete (netdev_t *netdev, const UINT8 hwaddr[])
{
  struct netdev_mac *mac, *prev;
  int type, flags;
  
  if (netdev->net_type != NETDEV_TYPE_ETHERNET) {
    return (-1);
  }
  
  mac = netdev_macfilter_find(netdev, hwaddr, &prev);
  if (!mac) {
    errno = EINVAL;
    return (-1);
  }
  
  if (mac->ref > 1) {
    mac->ref--;
    return (0);
  }
  
  if (prev) {
    prev->next = mac->next;
  } else {
    netdev->mac_filter = mac->next;
  }
  
  type = mac->type;
  mem_free(mac);
  
  flags = netif_get_flags((struct netif *)(netdev->sys));
  if (!(flags & (IFF_PROMISC | IFF_ALLMULTI)) || 
      (type != NETDEV_MAC_TYPE_MULTICAST)) {
    NETDEV_RXMODE(netdev, flags);
  }
  
  return (0);
}

/* netdev mac filter find */
static struct netdev_mac *netdev_macfilter_find (netdev_t *netdev, const UINT8 hwaddr[], struct netdev_mac **prev_save)
{
  struct netdev_mac *ha;
  struct netdev_mac *prev = NULL;
  
  NETDEV_MACFILTER_FOREACH(netdev, ha) {
    if (!lib_memcmp(ha->hwaddr, hwaddr, netdev->hwaddr_len)) {
      if (prev_save) {
        *prev_save = prev;
      }
      return (ha);
    }
    prev = ha;
  }
  
  return (NULL);
}

/* netdev mac filter clean */
static void netdev_macfilter_clean (netdev_t *netdev)
{
  struct netdev_mac *ha;
  
  while (netdev->mac_filter) {
    ha = netdev->mac_filter;
    netdev->mac_filter = ha->next;
    mem_free(ha);
  }
}

/* if netdev detected a packet in netdev buffer, driver can call this function to receive this packet.
   notify:0 can transmit 1: can receive 
   qen:0 do not use netjob queue 1:use netjob queue */
int  netdev_notify (struct netdev *netdev, netdev_inout inout, int q_en)
{
  if (!netdev || (netdev->magic_no != NETDEV_MAGIC)) {
    return (-1);
  }
  
  if (inout != LINK_INPUT) {
    return (0);
  }
  
  if (q_en) {
    if (netJobAdd(netdev->drv->receive, netdev, 
                  (void *)netdev_netif_linkinput, 0, 0, 0, 0) == 0) {
      return (0);
    
    } else {
      return (-1);
    }
  }
  
  NETDEV_RECEIVE(netdev, netdev_netif_linkinput);
  
  return (0);
}

int  netdev_notify_ex (struct netdev *netdev, netdev_inout inout, int q_en, unsigned int qindex)
{
  if (!netdev || (netdev->magic_no != NETDEV_MAGIC)) {
    return (-1);
  }
  
  if (inout != LINK_INPUT) {
    return (0);
  }
  
  if (q_en) {
    if (netJobAddEx(qindex, netdev->drv->receive, netdev, 
                    (void *)netdev_netif_linkinput, 0, 0, 0, 0) == 0) {
      return (0);
    
    } else {
      return (-1);
    }
  }
  
  NETDEV_RECEIVE(netdev, netdev_netif_linkinput);
  
  return (0);
}

int  netdev_notify_clear (struct netdev *netdev)
{
  if (!netdev || (netdev->magic_no != NETDEV_MAGIC)) {
    return (-1);
  }
  
  netJobDelete(2, netdev->drv->receive, netdev, 
               (void *)netdev_netif_linkinput, 0, 0, 0, 0);
               
  return (0);
}

int  netdev_notify_clear_ex (struct netdev *netdev, unsigned int qindex)
{
  if (!netdev || (netdev->magic_no != NETDEV_MAGIC)) {
    return (-1);
  }
  
  netJobDeleteEx(qindex, 2, netdev->drv->receive, netdev, 
                 (void *)netdev_netif_linkinput, 0, 0, 0, 0);
               
  return (0);
}

/* netdev statistical information update functions in:1 input 0:output */
void netdev_statinfo_total_add (netdev_t *netdev, netdev_inout inout, UINT32 bytes)
{
  struct netif *netif = (struct netif *)netdev->sys;
  
  if (inout == LINK_INPUT) {
    snmp_add_ifinoctets(netif, bytes);
  
  } else {
    snmp_add_ifoutoctets(netif, bytes);
  }
}

void netdev_statinfo_ucasts_inc (netdev_t *netdev, netdev_inout inout)
{
  struct netif *netif = (struct netif *)netdev->sys;
  
  if (inout == LINK_INPUT) {
    snmp_inc_ifinucastpkts(netif);
  
  } else {
    snmp_inc_ifoutucastpkts(netif);
  }
}

void netdev_statinfo_mcasts_inc (netdev_t *netdev, netdev_inout inout)
{
  struct netif *netif = (struct netif *)netdev->sys;
  
  if (inout == LINK_INPUT) {
    snmp_inc_ifinnucastpkts(netif);
  
  } else {
    snmp_inc_ifoutnucastpkts(netif);
  }
}

void netdev_statinfo_discards_inc (netdev_t *netdev, netdev_inout inout)
{
  struct netif *netif = (struct netif *)netdev->sys;
  
  if (inout == LINK_INPUT) {
    snmp_inc_ifindiscards(netif);
  
  } else {
    snmp_inc_ifoutdiscards(netif);
  }
}

void netdev_statinfo_errors_inc (netdev_t *netdev, netdev_inout inout)
{
  struct netif *netif = (struct netif *)netdev->sys;
  
  if (inout == LINK_INPUT) {
    snmp_inc_ifinerrors(netif);
  
  } else {
    snmp_inc_ifouterrors(netif);
  }
}

/* netdev link statistical information update functions */
void netdev_linkinfo_err_inc (netdev_t *netdev)
{
  LINK_STATS_INC(link.err);
}

void netdev_linkinfo_lenerr_inc(netdev_t *netdev)
{
  LINK_STATS_INC(link.lenerr);
}

void netdev_linkinfo_chkerr_inc(netdev_t *netdev)
{
  LINK_STATS_INC(link.chkerr);
}

void netdev_linkinfo_memerr_inc(netdev_t *netdev)
{
  LINK_STATS_INC(link.memerr);
}

void netdev_linkinfo_drop_inc(netdev_t *netdev)
{
  LINK_STATS_INC(link.drop);
}

void netdev_linkinfo_recv_inc(netdev_t *netdev)
{
  LINK_STATS_INC(link.recv);
}

void netdev_linkinfo_xmit_inc(netdev_t *netdev)
{
  LINK_STATS_INC(link.xmit);
}

/* netdev input buffer get 
 * reserve: ETH_PAD_SIZE + SIZEOF_VLAN_HDR size. */
struct pbuf *netdev_pbuf_alloc (UINT16 len)
{
  u16_t reserve = ETH_PAD_SIZE + SIZEOF_VLAN_HDR;
  struct pbuf *p = pbuf_alloc(PBUF_RAW, (u16_t)(len + reserve), PBUF_POOL);
  
  if (p) {
    pbuf_header(p, (u16_t)-reserve);
  }
  
  return (p);
}

void netdev_pbuf_free (struct pbuf *p)
{
  pbuf_free(p);
}

struct pbuf *netdev_pbuf_alloc_ram (UINT16 len, UINT16 res)
{
  struct pbuf *p = pbuf_alloc(PBUF_RAW, (u16_t)(len + res), PBUF_POOL);
  
  if (p) {
    pbuf_header(p, (u16_t)-res);
  }
  
  return (p);
}

/* netdev transmit function can ref this packet? */
BOOL netdev_pbuf_can_ref (struct pbuf *p)
{
  u8_t type;

  if (p->tot_len == p->len) {
    type = pbuf_get_allocsrc(p);
    if ((type == PBUF_TYPE_ALLOC_SRC_MASK_STD_HEAP) || 
        (type == PBUF_TYPE_ALLOC_SRC_MASK_STD_MEMP_PBUF_POOL)) {
      return (TRUE);
    }
  }
  
  return (FALSE);
}

/* get netdev data buffer */
void *netdev_pbuf_data (struct pbuf *p)
{
  return (p->payload);
}

/* netdev input buffer push */
UINT8 *netdev_pbuf_push (struct pbuf *p, UINT16 len)
{
  if (p) {
    if (pbuf_header(p, (s16_t)len) == 0) {
      return ((UINT8 *)p->payload);
    }
  }
  
  return (NULL);
}

/* netdev input buffer pop */
UINT8 *netdev_pbuf_pull (struct pbuf *p, UINT16 len)
{
  if (p) {
    if (pbuf_header(p, (s16_t)-len) == 0) {
      return ((UINT8 *)p->payload);
    }
  }
  
  return (NULL);
}

/* netdev input buffer cat */
int netdev_pbuf_link (struct pbuf *h, struct pbuf *t, BOOL ref_t)
{
  if (h && t) {
    if (ref_t) {
      pbuf_chain(h, t);
    } else {
      pbuf_cat(h, t);
    }
    return (0);
  }
  
  return (-1);
}

/* netdev input buffer trunc */
int netdev_pbuf_trunc (struct pbuf *p, UINT16 len)
{
  if (p && (p->tot_len >= len)) {
    pbuf_realloc(p, len);
    return (0);
  }
  
  return (-1);
}

/* netdev buffer get vlan info */
int netdev_pbuf_vlan_present (struct pbuf *p)
{
  struct eth_hdr *ethhdr = (struct eth_hdr *)((u8_t *)p->payload - ETH_PAD_SIZE);
  
  return (ethhdr->type == PP_HTONS(ETHTYPE_VLAN));
}

int netdev_pbuf_vlan_id (struct pbuf *p, UINT16 *vlanid)
{
  struct eth_hdr *ethhdr = (struct eth_hdr *)((u8_t *)p->payload - ETH_PAD_SIZE);

  if ((ethhdr->type == PP_HTONS(ETHTYPE_VLAN)) && (p->len >= ETH_HLEN + 4)) {
    struct eth_vlan_hdr *vlan = (struct eth_vlan_hdr *)(((u8_t *)ethhdr) + SIZEOF_ETH_HDR);
    if (vlanid) {
      *vlanid = vlan->prio_vid;
    }
    return (0);
  }
  
  return (-1);
}

int netdev_pbuf_vlan_proto (struct pbuf *p, UINT16 *vlanproto)
{
  struct eth_hdr *ethhdr = (struct eth_hdr *)((u8_t *)p->payload - ETH_PAD_SIZE);

  if ((ethhdr->type == PP_HTONS(ETHTYPE_VLAN)) && (p->len >= ETH_HLEN + 4)) {
    struct eth_vlan_hdr *vlan = (struct eth_vlan_hdr *)(((u8_t *)ethhdr) + SIZEOF_ETH_HDR);
    if (vlanproto) {
      *vlanproto = vlan->tpid;
    }
    return (0);
  }
  
  return (-1);
}

#if LW_CFG_NET_DEV_PROTO_ANALYSIS > 0

/* netdev buffer get ethernet & vlan header */
struct eth_hdr *netdev_pbuf_ethhdr (struct pbuf *p, int *hdrlen)
{
  struct eth_hdr *ethhdr = (struct eth_hdr *)((u8_t *)p->payload - ETH_PAD_SIZE);
  
  if (hdrlen) {
    *hdrlen = ETH_HLEN;
  }
  
  return (ethhdr);
}

struct eth_vlan_hdr *netdev_pbuf_vlanhdr (struct pbuf *p, int *hdrlen)
{
  struct eth_hdr *ethhdr = (struct eth_hdr *)((u8_t *)p->payload - ETH_PAD_SIZE);

  if (ethhdr->type == PP_HTONS(ETHTYPE_VLAN) && (p->len >= ETH_HLEN + 4)) {
    struct eth_vlan_hdr *vlan = (struct eth_vlan_hdr *)(((u8_t *)ethhdr) + SIZEOF_ETH_HDR);
    if (hdrlen) {
      *hdrlen = 4;
    }
    return (vlan);
  }
  
  return (NULL);
}

/* netdev buffer get proto header */
struct ip_hdr *netdev_pbuf_iphdr (struct pbuf *p, int offset, int *hdrlen)
{
  struct pbuf *q;
  u16_t out_offset;

  q = pbuf_skip(p, (u16_t)offset, &out_offset);
  if (!q) {
    return (NULL);
  }
  
  if (q->len >= (u16_t)out_offset + IP_HLEN) {
    struct ip_hdr *iphdr = (struct ip_hdr *)((u8_t *)q->payload + out_offset);
    if (hdrlen) {
      *hdrlen = IPH_HL(iphdr) << 2;
    }
    return (iphdr);
  }
  
  return (NULL);
}

struct ip6_hdr *netdev_pbuf_ip6hdr (struct pbuf *p, int offset, int *hdrlen, int *tothdrlen, int *tproto)
{
  struct pbuf *q;
  u16_t out_offset;
  
  q = pbuf_skip(p, (u16_t)offset, &out_offset);
  if (!q) {
    return (NULL);
  }
  
  if (q->len >= (u16_t)out_offset + IP6_HLEN) {
    struct ip6_hdr *ip6hdr = (struct ip6_hdr *)((u8_t *)q->payload + offset);
    if (hdrlen) {
      *hdrlen = IP6_HLEN;
    }
    if (tothdrlen) {
      u8_t *hdr;
      int hlen, nexth;
    
      *tothdrlen = IP6_HLEN;
      hdr = (u8_t *)ip6hdr + IP6_HLEN;
      nexth = IP6H_NEXTH(ip6hdr);
      while (nexth != IP6_NEXTH_NONE) {
        switch (nexth) {
        
        case IP6_NEXTH_HOPBYHOP:
        case IP6_NEXTH_DESTOPTS:
        case IP6_NEXTH_ROUTING:
          nexth = *hdr;
          hlen = 8 * (1 + *(hdr + 1));
          (*tothdrlen) += hlen;
          hdr += hlen;
          break;
        
        case IP6_NEXTH_FRAGMENT:
          nexth = *hdr;
          hlen = 8;
          (*tothdrlen) += hlen;
          hdr += hlen;
          break;
          
        default:
          goto out;
          break;
        }
      }
out:
      if (tproto) {
        *tproto = nexth;
      }
    }
    return (ip6hdr);
  }
  
  return (NULL);
}

struct tcp_hdr *netdev_pbuf_tcphdr (struct pbuf *p, int offset, int *hdrlen)
{
  struct pbuf *q;
  u16_t out_offset;

  q = pbuf_skip(p, (u16_t)offset, &out_offset);
  if (!q) {
    return (NULL);
  }
  
  if (q->len >= (u16_t)out_offset + TCP_HLEN) {
    struct tcp_hdr *tcphdr = (struct tcp_hdr *)((u8_t *)q->payload + out_offset);
    if (hdrlen) {
      *hdrlen = TCPH_HDRLEN(tcphdr) << 2;
    }
    return (tcphdr);
  }
  
  return (NULL);
}

struct udp_hdr *netdev_pbuf_udphdr (struct pbuf *p, int offset, int *hdrlen)
{
  struct pbuf *q;
  u16_t out_offset;

  q = pbuf_skip(p, (u16_t)offset, &out_offset);
  if (!q) {
    return (NULL);
  }
  
  if (q->len >= (u16_t)out_offset + UDP_HLEN) {
    struct udp_hdr *udphdr = (struct udp_hdr *)((u8_t *)q->payload + out_offset);
    if (hdrlen) {
      *hdrlen = UDP_HLEN;
    }
    return (udphdr);
  }
  
  return (NULL);
}

#endif /* LW_CFG_NET_DEV_PROTO_ANALYSIS */
/*
 * end
 */

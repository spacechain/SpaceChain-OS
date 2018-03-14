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
 
#ifndef __NETDEV_H
#define __NETDEV_H

#include "net/if.h"
#include "lwip/pbuf.h"
#include "lwip/sockets.h"
#include "sys/param.h"

#if LW_CFG_NET_DEV_PROTO_ANALYSIS > 0
/* net protocol */
#include "lwip/prot/ip.h"
#include "lwip/prot/ip4.h"
#include "lwip/prot/ip6.h"
#include "lwip/prot/tcp.h"
#include "lwip/prot/udp.h"
#include "lwip/prot/ethernet.h"
#endif /* LW_CFG_NET_DEV_PROTO_ANALYSIS */

/* netdev declaration */
struct netdev;

/*
 * netdev media address list
 */
struct netdev_mac {
  /* list */
  struct netdev_mac *next;
  struct netdev_mac *nouse;
  
  /* mac addr */
  UINT8 hwaddr[NETIF_MAX_HWADDR_LEN];
  /* mac addr type */
#define NETDEV_MAC_TYPE_UNICAST    0
#define NETDEV_MAC_TYPE_MULTICAST  1
  int type;
  
  /* system used */
  int ref;
  int res[6];
};

/* 
 * netdev mac filter traversal
 */
#define NETDEV_MACFILTER_FOREACH(netdev, ha) \
  for ((ha) = (netdev)->mac_filter; (ha) != NULL; (ha) = (ha)->next)

/*
 * netdev poll mode
 */
struct netdev_poll {
  /* SylixOS Real-Time externd */
#define NETDEV_POLLMODE_DIS 0
#define NETDEV_POLLMODE_EN  1
  int poll_mode;
  
  /* user poll argument */
  void *poll_arg;
  
  /* user poll input function.
   * The system will automatically call poll_input() with each received packet */
  int (*poll_input)(struct netdev *, struct pbuf *);
};

/*
 * netdev poll_arg
 */
#define NETDEV_POLL_ARG(netdev) ((netdev)->poll.poll_arg)

/*
 * network driver functions.
 */
struct netdev_funcs {
  /* initialize function */
  int  (*init)(struct netdev *netdev);

  /* basice functions can initialize with NULL */
  int  (*up)(struct netdev *netdev);
  int  (*down)(struct netdev *netdev);
  int  (*remove)(struct netdev *netdev);
  
  /* netdev ioctl (Only 2 commands should do)
   * cmd: SIOCSIFMTU:    arg is struct ifreq *pifreq, set MTU    (pifreq->ifr_mtu)
   *      SIOCSIFHWADDR: arg is struct ifreq *pifreq, set hwaddr (pifreq->ifr_hwaddr[]) */
  int  (*ioctl)(struct netdev *netdev, int cmd, void *arg);

  /* netdev rx mode updated: (Must be in strict accordance with the following order of judgment)
   * if flags & IFF_PROMISC, driver must allow all packet input.
   * if flags & IFF_ALLMULTI, driver must allow all multicast packet input.
   * other, driver must allow all multicast address in mac list (can use NETDEV_MACFILTER_FOREACH() traverse) */
  int  (*rxmode)(struct netdev *netdev, int flags);
  
  /* netdev transmit a packet, and if success return 0 or return -1. */
  int  (*transmit)(struct netdev *netdev, struct pbuf *p);
  
  /* netdev receive a packet, system will call this function receive a packet. */
  void (*receive)(struct netdev *netdev, int (*input)(struct netdev *, struct pbuf *));
  
  /* If netdev support poll mode you must set the following functions 
   * Poll mode is often used for High-Speed, Real-Time network applications */
  /* pollrecv() will be called by real-time applications. 
   * if a packet arrived, pollrecv() must invoke netdev_notify(netdev, LINK_INPUT, 0); to receive packet */
  void (*pollrecv)(struct netdev *netdev);
  
  /* intctl() can enable or disable netdev interrupt, in poll mode, SylixOS will disable netdev interrupt */
  int (*intctl)(struct netdev *netdev, int en);
  
  /* reserve for futrue */
  void (*reserved[6])();
};

/*
 * network device struct.
 */
typedef struct netdev {
#define NETDEV_VERSION  (0x00020003)
#define NETDEV_MAGIC    (0xf7e34a81 + NETDEV_VERSION)
  UINT32 magic_no;  /* MUST be NETDEV_MAGIC */

  char  dev_name[IF_NAMESIZE];  /* user network device name (such as igb* rtl* also call initialize with '\0') */
  char  if_name[IF_NAMESIZE];   /* add to system netif name (such as 'en'), after netdev_add() if_name saved full ifname by system */
  char *if_hostname;
  
#define NETDEV_INIT_LOAD_PARAM     0x01     /* load netif parameter when add to system */
#define NETDEV_INIT_LOAD_DNS       0x02     /* load dns parameter when add to system */
#define NETDEV_INIT_IPV6_AUTOCFG   0x04
#define NETDEV_INIT_AS_DEFAULT     0x08
#define NETDEV_INIT_USE_DHCP       0x10     /* force use DHCP get address */
#define NETDEV_INIT_DO_NOT         0x20     /* do not call init() function (Only used for net bridge) */
  UINT32 init_flags;
  
#define NETDEV_CHKSUM_GEN_IP       0x0001   /* tcp/ip stack will generate checksum IP, UDP, TCP, ICMP, ICMP6 */
#define NETDEV_CHKSUM_GEN_UDP      0x0002
#define NETDEV_CHKSUM_GEN_TCP      0x0004
#define NETDEV_CHKSUM_GEN_ICMP     0x0008
#define NETDEV_CHKSUM_GEN_ICMP6    0x0010
#define NETDEV_CHKSUM_CHECK_IP     0x0100   /* tcp/ip stack will check checksum IP, UDP, TCP, ICMP, ICMP6 */
#define NETDEV_CHKSUM_CHECK_UDP    0x0200
#define NETDEV_CHKSUM_CHECK_TCP    0x0400
#define NETDEV_CHKSUM_CHECK_ICMP   0x0800
#define NETDEV_CHKSUM_CHECK_ICMP6  0x1000
#define NETDEV_CHKSUM_ENABLE_ALL   0xffff   /* tcp/ip stack will gen/check all chksum */
#define NETDEV_CHKSUM_DISABLE_ALL  0x0000   /* tcp/ip stack will not gen/check all chksum */
  UINT32 chksum_flags;

#define NETDEV_TYPE_RAW         0
#define NETDEV_TYPE_ETHERNET    1
  UINT32 net_type;
  
  UINT64 speed; /* link layer speed bps */
  UINT32 mtu;   /* link layer max packet length */
  
  UINT8 hwaddr_len;                     /* link layer address length MUST 6 or 8 */
  UINT8 hwaddr[NETIF_MAX_HWADDR_LEN];   /* link layer address */
  
  struct netdev_funcs *drv; /* netdev driver */
  
  void *priv;   /* user network device private data */
  
  /* the following member is used by system, driver not used! */
  int if_flags;
  
  /* wireless externed */
  void *wireless_handlers; /* iw_handler_def ptr */
  void *wireless_data; /* iw_public_data ptr */
  
  /* hwaddr filter list */
  struct netdev_mac *mac_filter;
  
  /* SylixOS Real-Time externd(poll mode) */
  struct netdev_poll poll;
  
  /* SylixOS Reserve */
  void *kern_priv; /* kernel priv */
  void *kern_res[16];
  
  ULONG sys[254];  /* reserve for netif */
} netdev_t;

/* netdev driver call the following functions add / delete a network interface,
 * if this device not in '/etc/if_param.ini' ip, netmask, gw is default configuration.
 * 'if_flags' defined in net/if.h such as IFF_UP, IFF_BROADCAST, IFF_RUNNING, IFF_NOARP, IFF_MULTICAST, IFF_PROMISC ... */
int  netdev_add(netdev_t *netdev, const char *ip, const char *netmask, const char *gw, int if_flags);
int  netdev_delete(netdev_t *netdev); /* WARNING: You MUST DO NOT lock device then call this function, it will cause a deadlock with TCP LOCK */
int  netdev_index(netdev_t *netdev, unsigned int *index);
int  netdev_ifname(netdev_t *netdev, char *ifname);

/* netdev outer firewall set,
 * The system will automatically call fw() with each received packet,
 * If fw() return 1, this indicates that the packet was eaten by fw(), 
 * fw() must release the pbuf, then system will not receive this packet. */
int  netdev_firewall(netdev_t *netdev, int (*fw)(netdev_t *, struct pbuf *));
int  netdev_foreache(FUNCPTR pfunc, void *arg0, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5);

/* netdev poll mode set,
 * Real-Time module can use netdev_poll_enable() to start a poll mode,
 * and then Real-Time module must constantly call netdev_poll_svc().
 * if driver received a packet, netdev will call poll_input() (which installed by netdev_poll_enable())
 * Real-Time module can receive packet in poll_input() function.
 * if poll_input() return 1, this indicates that the packet was eaten by poll_input(), 
 * poll_input() must release the pbuf, then system will not receive this packet. */
int  netdev_poll_enable(netdev_t *netdev, int (*poll_input)(struct netdev *, struct pbuf *), void *poll_arg);
int  netdev_poll_disable(netdev_t *netdev);
int  netdev_poll_svc(netdev_t *netdev);

/* netdev mac filter */
int  netdev_macfilter_isempty(netdev_t *netdev);
int  netdev_macfilter_count(netdev_t *netdev);
int  netdev_macfilter_add(netdev_t *netdev, const UINT8 hwaddr[]);
int  netdev_macfilter_delete(netdev_t *netdev, const UINT8 hwaddr[]);

/* netdev find (MUST in NETIF_LOCK mode) */
netdev_t *netdev_find_by_index(unsigned int index);
netdev_t *netdev_find_by_ifname(const char *if_name);
netdev_t *netdev_find_by_devname(const char *dev_name);

/* if netdev link status changed has been detected, 
 * driver must call the following functions 
 * linkup 1: linked up  0:not link */
int  netdev_set_linkup(netdev_t *netdev, int linkup, UINT64 speed);
int  netdev_get_linkup(netdev_t *netdev, int *linkup);

/* netdev linkup watchdog function
 * NOTICE: one netdev can ONLY add one linkup_wd function.
 *         when netdev removed driver must delete watchdog function manually. 
 *         this watchdog function only check and update the linkup state */
int  netdev_linkup_wd_add(netdev_t *netdev, void  (*linkup_wd)(netdev_t *));
int  netdev_linkup_wd_delete(netdev_t *netdev, void  (*linkup_wd)(netdev_t *));

typedef enum {
  LINK_INPUT = 0,   /* packet input */
  LINK_OUTPUT       /* packet output */
} netdev_inout;

/* if netdev detected a packet in netdev buffer, driver can call this function to receive this packet.
   qen:0 do not use netjob queue 1:use netjob queue */
int  netdev_notify(struct netdev *netdev, netdev_inout inout, int q_en);
int  netdev_notify_ex(struct netdev *netdev, netdev_inout inout, int q_en, unsigned int qindex);
int  netdev_notify_clear(struct netdev *netdev);
int  netdev_notify_clear_ex(struct netdev *netdev, unsigned int qindex);

/* netdev statistical information update functions */
void netdev_statinfo_total_add(netdev_t *netdev, netdev_inout inout, UINT32 bytes);
void netdev_statinfo_ucasts_inc(netdev_t *netdev, netdev_inout inout);
void netdev_statinfo_mcasts_inc(netdev_t *netdev, netdev_inout inout);
void netdev_statinfo_discards_inc(netdev_t *netdev, netdev_inout inout);
void netdev_statinfo_errors_inc(netdev_t *netdev, netdev_inout inout);

/* netdev link statistical information update functions */
void netdev_linkinfo_err_inc(netdev_t *netdev);
void netdev_linkinfo_lenerr_inc(netdev_t *netdev);
void netdev_linkinfo_chkerr_inc(netdev_t *netdev);
void netdev_linkinfo_memerr_inc(netdev_t *netdev);
void netdev_linkinfo_drop_inc(netdev_t *netdev);
void netdev_linkinfo_recv_inc(netdev_t *netdev);
void netdev_linkinfo_xmit_inc(netdev_t *netdev);

/* netdev send can ref pbuf ? */
#define NETDEV_TX_CAN_REF_PBUF(p) netdev_pbuf_can_ref(p)

/* netdev buffer data ? */
#define NETDEV_PBUF_DATA(p, type) (type)netdev_pbuf_data(p)

/* netdev input buffer get 
 * netdev_pbuf_alloc auto reserve ETH_PAD_SIZE + SIZEOF_VLAN_HDR size. */
struct pbuf *netdev_pbuf_alloc(UINT16 len);
struct pbuf *netdev_pbuf_alloc_ram(UINT16 len, UINT16 res);
void netdev_pbuf_free(struct pbuf *p);

/* pbuf stat and data */
BOOL netdev_pbuf_can_ref(struct pbuf *p);
void *netdev_pbuf_data(struct pbuf *p);

/* netdev buffer header move */
UINT8 *netdev_pbuf_push(struct pbuf *p, UINT16 len);
UINT8 *netdev_pbuf_pull(struct pbuf *p, UINT16 len);

/* netdev buffer tail cat and truncate */
int netdev_pbuf_link(struct pbuf *h, struct pbuf *t, BOOL ref_t);
int netdev_pbuf_trunc(struct pbuf *p, UINT16 len);

/* netdev buffer get vlan info */
int  netdev_pbuf_vlan_present(struct pbuf *p);
int  netdev_pbuf_vlan_id(struct pbuf *p, UINT16 *vlanid);
int  netdev_pbuf_vlan_proto(struct pbuf *p, UINT16 *vlanproto);

#if LW_CFG_NET_DEV_PROTO_ANALYSIS > 0
/* netdev buffer get ethernet & vlan header */
struct eth_hdr *netdev_pbuf_ethhdr(struct pbuf *p, int *hdrlen);
struct eth_vlan_hdr *netdev_pbuf_vlanhdr(struct pbuf *p, int *hdrlen);

/* netdev buffer get proto header */
struct ip_hdr *netdev_pbuf_iphdr(struct pbuf *p, int offset, int *hdrlen);
struct ip6_hdr *netdev_pbuf_ip6hdr(struct pbuf *p, int offset, int *hdrlen, int *tothdrlen, int *tproto);
struct tcp_hdr *netdev_pbuf_tcphdr(struct pbuf *p, int offset, int *hdrlen);
struct udp_hdr *netdev_pbuf_udphdr(struct pbuf *p, int offset, int *hdrlen);
#endif /* LW_CFG_NET_DEV_PROTO_ANALYSIS */

#if LW_CFG_NET_DEV_ZCBUF_EN > 0
/* netdev zero copy buffer pool create */
void *netdev_zc_pbuf_pool_create(addr_t addr, UINT32 blkcnt, size_t blksize);
/* netdev zero copy buffer pool delete */
int netdev_zc_pbuf_pool_delete(void *hzcpool, int force);
/* netdev input 'zero copy' buffer get a blk
 * reserve: ETH_PAD_SIZE + SIZEOF_VLAN_HDR size. 
 * ticks = 0  no wait
 *       = -1 wait forever */
struct pbuf *netdev_zc_pbuf_alloc(void *hzcpool, int ticks);
struct pbuf *netdev_zc_pbuf_alloc_res(void *hzcpool, int ticks, UINT16 hdr_res);
/* free zero copy pbuf */
void netdev_zc_pbuf_free(struct pbuf *p);
#endif /* LW_CFG_NET_DEV_ZCBUF_EN */

#endif /* __NETDEV_H */

/**
 * @file
 * 6lowpan implementation, which is used for wireless ieee 802.15.4.
 *
 */

/*
 * Copyright (c) 2006-2014 SylixOS Group.
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
 * Author: Ren.Haibo <habbyren@qq.com>
 *
 */
 
/* 
 * This is an arch independent 6lowpan netif.
 */
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/autoip.h"
#include "lwip/snmp.h"

#include "lowpan_if.h"
#include "lowpan_compress.h"
#include "lowpan_frag.h"
#include "ieee802154_eth.h"

#include "string.h"

/**
 * Pass a received packet to tcpip_thread for input processing.
 * This funtion replace the tcpip_input as the lowpan input funtion.
 *
 * @param p the received packet, p->payload pointing to the Ethernet header or
 *          to an IP header (if inp doesn't have NETIF_FLAG_ETHARP or
 *          NETIF_FLAG_ETHERNET flags)
 * @param inp the network interface on which the packet was received
 */
err_t
lowpan_input (struct pbuf *p, struct netif *inp)
{
  u8_t len;
  u8_t patch;
  u8_t isbc;
  struct eth_hdr ethhdr;
  
  len = ieee802154_to_eth(p, &ethhdr, &isbc);
  
  /* First, delete the ethernet header, then start to compress the ipv6 packet.*/
  if (pbuf_header(p, -(s16_t)len)) {
    snmp_inc_ifindiscards(inp);
    pbuf_free(p);
    return ERR_VAL;
  }
  
  /* identify the compress data start. */
  patch = *((u8_t *)p->payload);
  /* If the frame is a fragmentation frame. */
  if (patch & 0x80) {
#if IP_FRAG
    p = lowpan_reass(p, &ethhdr);
    if (p == NULL) {
      return ERR_OK;
    }
#endif
  }
  
  /* The frame is a fully frame, can prase it. */
  p = lowpan_header_prase(p, &ethhdr);
  if (p == NULL) {
     LWIP_DEBUGF(NETIF_DEBUG, ("lowpan_input: lowpan_header_prase fail!\n"));
     snmp_inc_ifindiscards(inp);
     return ERR_MEM;
  }
  
  if (isbc) {
    snmp_inc_ifinnucastpkts(inp);
  } else {
    snmp_inc_ifinucastpkts(inp);
  }
  
  snmp_add_ifinoctets(inp, (u32_t)p->tot_len);
  
  return inp->input(p, inp); /* put ethernet packet into tcp/ip */
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become availale since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */

static err_t
lowpan_output (struct netif *netif, struct pbuf *p)
{
  u8_t head[32];
  u8_t iphdr[96];
  u8_t head_len;
  u16_t iphdr_len;
  err_t err = ERR_OK;
  u8_t isbc;
  struct pbuf *q, *pfw;
  struct lowpanif *lowpanif = (struct lowpanif *)netif->state;
  struct eth_hdr ethhdr;
  
  /* points to packet payload, which starts with an Ethernet header */
  ethhdr = *((struct eth_hdr *)p->payload);
  
  q = lowpan_header_create(p, iphdr, &iphdr_len);
  if (NULL == q) {
    LWIP_DEBUGF(NETIF_DEBUG, ("lowpan_output: lowpan_header_create fail!\n"));
    return ERR_VAL;
  }
  
  /* Convert the ethernet header to ieee 802.15.4 header. */
  head_len = ieee802154_from_eth(&ethhdr, lowpanif, head, sizeof(head), &isbc);
  if (head_len == 0) {
    LWIP_DEBUGF(NETIF_DEBUG, ("lowpan_output: ieee802154_from_eth fail!\n"));
    return ERR_IF;
  }
  
  pfw = pbuf_alloc(PBUF_RAW, ((u16_t)(iphdr_len + head_len)), PBUF_POOL);
  if (NULL == pfw) {
    return ERR_MEM;
  }
  
  MEMCPY(pfw->payload, head, head_len);
  MEMCPY(&(((u8_t*)pfw->payload)[head_len]), iphdr, iphdr_len);
  pbuf_chain(pfw, p); /* do not need free p */
  
  if ((pfw->tot_len) <= (lowpanif->mfl)) {
    ieee802154_seqno_set(pfw, lowpanif->seqno);
    lowpanif->seqno++;
    
    if (MAC_DRIVER(lowpanif)->send(lowpanif, pfw) == RADIO_TX_OK) {
      err = ERR_OK;
    } else {
      err = ERR_IF;
    }
  } else { /* Use the lowpan frag to send the various fragments. */
#if IP_FRAG
    err = lowpan_frag(lowpanif, pfw);
#endif
  }
  
  if (err == ERR_OK) {
    if (isbc) {
      snmp_inc_ifoutnucastpkts(netif);
    } else {
      snmp_inc_ifoutucastpkts(netif);
    }
    snmp_add_ifoutoctets(netif, (u32_t)pfw->tot_len);
  } else {
    snmp_inc_ifoutdiscards(netif);
  }
  
  pbuf_free(pfw);
  return err;
}

/**
 * In this function, the hardware should be initialized.
 * Called from stellarisif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void
lowpan_level_init (struct netif *netif)
{
  struct lowpanif *lowpanif = (struct lowpanif *)netif->state;
  
  LWIP_ERROR("lowpanif->mfl > 64", (lowpanif->mfl > 64), return;);
  LWIP_ERROR("lowpanif->mfl <= LOWPAN_FRAG_MAX_MTU", (lowpanif->mfl <= LOWPAN_FRAG_MAX_MTU), return;);
  
  /* set MAC hardware address length */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;

  /* set MAC hardware address */
  netif->hwaddr[0] = lowpanif->addr[0];
  netif->hwaddr[1] = lowpanif->addr[1];
  netif->hwaddr[2] = lowpanif->addr[2];
  netif->hwaddr[3] = lowpanif->addr[5];
  netif->hwaddr[4] = lowpanif->addr[6];
  netif->hwaddr[5] = lowpanif->addr[7];

  /* maximum transfer unit (like ethernet (MUST not bigger than 0x7ff)) */
  netif->mtu = 1500;

  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_IGMP | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
  
  LWIP_ERROR("lowpanif->addr[3] == 0xff", (lowpanif->addr[3] == 0xff), return;);
  LWIP_ERROR("lowpanif->addr[4] == 0xfe", (lowpanif->addr[4] == 0xfe), return;);
}


/**
 * Should be called if the interface not support to IPv4
 *
 *
 * @param netif The lwIP network interface which the IP packet will be sent on.
 * @param p The pbuf(s) containing the IP packet to be sent.
 * @param ipaddr The IP address of the packet destination.
 *
 * @return
 * - ERR_IF Error interface of this packet
 */
static err_t
netif_null_output_ip4(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr)
{
    (void)netif;
    (void)p;
    (void)ipaddr;

    return ERR_IF;
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
static err_t
lowpan_init (struct netif *netif)
{
  SYS_ARCH_DECL_PROTECT(lev);
  struct lowpanif *lowpanif = (struct lowpanif *)netif->state;

  LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 1000000);

  netif->name[0] = 'l';
  netif->name[1] = 'p';
  /* We directly use netif_null_output_ip4() here to save a function call.
   * You can instead declare your own function an call aodv_netif_output() for IPv4 Ad-hoc
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
  if (netif->output == NULL) {
    netif->output = netif_null_output_ip4;
  }
  
#if LWIP_IPV6
  if (netif->output == NULL) {
    netif->output_ip6 = ethip6_output;
  }
#endif /* LWIP_IPV6 */
  netif->linkoutput = lowpan_output;

  /**
   * This entire function must run within a "critical section" to preserve
   * the integrity of the transmit pbuf queue.
   *
   */
  SYS_ARCH_PROTECT(lev);
  
  /* initialize the hardware */
  lowpan_level_init(netif);
  
  SYS_ARCH_UNPROTECT(lev);
  
  /* Init on the radio. */
  MAC_DRIVER(lowpanif)->init(lowpanif);
  
  /* Turn on the radio. */
  MAC_DRIVER(lowpanif)->on(lowpanif);
  
#if LWIP_IPV6
  netif_create_ip6_linklocal_address(netif, 1);
  netif_ip6_addr_set_state(netif, 0, IP6_ADDR_PREFERRED);
#endif

  return ERR_OK;
}

/**
 * Add a lowpan network interface to the list of lwIP netifs.
 *
 * @param lowpan_if a pre-allocated lowpan netif structure
 * @param fn_add The funtion which add the netif to the network, 
 *               always be netif_add.
 * @param ipaddr IP address for the new netif
 * @param netmask network mask for the new netif
 * @param gw default gateway IP address for the new netif
 *
 * @return lowpan_if, or NULL if failed.
 */
struct lowpanif * 
lowpan_netif_add (struct lowpanif *lowpanif, netif_add_handle fn_add,
                  ip_addr_t *ipaddr, ip_addr_t *netmask, ip_addr_t *gw)
{
  static int lowpan_proto_init = 0;
  
  if (lowpan_proto_init == 0) {
    lowpan_proto_init = 1;
#if IP_FRAG
    lowpan_reass_init();
#endif
  }
  
  if (!RADIO_DRIVER(lowpanif) ||
      !MAC_DRIVER(lowpanif)   ||
      !RDC_DRIVER(lowpanif)) {
    return NULL;
  }
  
  if (NULL != fn_add(&lowpanif->netif, ipaddr, netmask, gw, lowpanif, lowpan_init, 
                     tcpip_input)) {
     return lowpanif;
  } 
  
  return NULL;
}

/**
 * Set the lowpan network interface as the default network interface
 * (used to output all packets for which no specific route is found)
 *
 * @param netif the default network interface
 */
void 
lowpan_netif_set_default (struct lowpanif *lowpanif)
{
  netif_set_default(&lowpanif->netif);
}

#if LWIP_DHCP
/**
 * Start DHCP negotiation for a lowpan network interface.
 *
 * If no DHCP client instance was attached to this interface,
 * a new client is created first. If a DHCP client instance
 * was already present, it restarts negotiation.
 *
 * @param netif The lwIP network interface
 * @return lwIP error code
 * - ERR_OK - No error
 * - ERR_MEM - Out of memory
 */
err_t
lowpan_netif_dhcp_start (struct lowpanif *lowpanif)
{
  return (dhcp_start(&lowpanif->netif));
}
#endif

#if LWIP_AUTOIP
/**
 * Start AutoIP client
 *
 * @param netif network interface on which start the AutoIP client
 */
err_t
lowpan_netif_autoip_start (struct lowpanif *lowpanif)
{
  return (autoip_start(&lowpanif->netif));
}
#endif
  
/**
 * Bring an lowpan interface up, available for processing
 * traffic.
 * 
 * @note: Enabling DHCP on a down interface will make it come
 * up once configured.
 * 
 * @see dhcp_start()
 */ 
void 
lowpan_netif_set_up (struct lowpanif *lowpanif)
{
  netif_set_up(&lowpanif->netif);
}
/* end */

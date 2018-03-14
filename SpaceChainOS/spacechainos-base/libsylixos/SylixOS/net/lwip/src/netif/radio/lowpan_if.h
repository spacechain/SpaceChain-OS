/**
 * @file
 * 6lowpan (IPv4/v6 compatible) implementation, which is used for wireless ieee 802.15.4.
 *
 * RFC6282, the ipv6 head compress format.
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
 *         Han.Hui <sylixos@gmail.com>
 *
 */
 
#ifndef __LOWPAN_IF_H__
#define __LOWPAN_IF_H__

#include "lwip/opt.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"

#include "radio_param.h"
#include "ieee802154_frame.h"
#include "crypt_driver.h"
#include "mac_driver.h"
#include "rdc_driver.h"
#include "radio_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/** CSMA Mac Parameter
 */
struct csma_parameter {
  /** Number of maximum CSMA attempts before giving up sending a frame. (software CSMA) */
  u8_t csma_param_retr;

  /** One symbol transmit time. (us) */
  u16_t csma_param_sym_us;
};

/** TDMA Mac Parameter
 */
struct tdma_parameter {
  /** tdma send period, (ms) */
  u16_t tdma_param_period;
};

/** X-MAC RDC Parameter
 */
struct xmac_parameter {
  /** radio on time, recommended 5 ~ 10ms (on time + off time is one rdc channel check period) */
  u16_t xmac_param_ontime_ms;
  
  /** radio off time, recommended 50 ~ 100ms */
  u16_t xmac_param_offtime_ms;
  
  /** max strobe time recommended (4 * ontime) + offtime */
  u16_t xmac_param_strobe_ms;
  
  /** max strobe wait time recommended (5 * ontime) / 8 */
  u16_t xmac_param_strobe_wait_ms;
};

#if LOWPAN_AES_CRYPT || LOWPAN_SIMPLE_CRYPT
/** crypt Parameter
 */
struct crypt_parameter {
  /** crypt key */
#define LOWPAN_CRYPT_KEY_LEN 16
  u8_t key[LOWPAN_CRYPT_KEY_LEN];
};
#endif /* LOWPAN_AES_CRYPT || LOWPAN_SIMPLE_CRYPT */

/** Generic data structure used for all lwIP radio interfaces. 
 *  This is a virtual ethernet.
 */
struct lowpanif {
  /** The netif used. */
  struct netif netif;
  
  /** Driver MUST Initialize the following variable. */
  /** seq number init with a random number. */
  u8_t seqno;
  
  /** maximum mac frame length, except for crc16. MUST bigger than 64bytes, IEEE802154_MAX_MTU is recommend */
  u16_t mfl;
  
  /** The RF channel. */
  u16_t channel;
  
  /** The PAN ID of the wireless network. */
  u16_t panid;
  
  /** The lowpan address define (can convert to ethernet use eui64). short address is addr[0] and addr[1] */
  u8_t addr[IEEE802154_MAX_ADDR_LEN];
  
  /** radio driver feature : 
   *  if radio chip driver have hardware CSMA, you can use CSMA or TDMA Mac Driver,
   *  if radio chip driver does not have hardware CSMA, you MUST use software CSMA Driver (csma_mac_driver) */
  u8_t csma_type;
#define LOWPAN_CSMA_HW 0
#define LOWPAN_CSMA_SW 1
  
  /** radio driver feature : 
   *  if radio chip driver have hardware AUTO-ACK send, you can set this is AUTO (Strongly recommended) 
   *  if radio chip driver have hardware does not have AUTO-ACK, you can use SW, do not recommended */
  u8_t ack_type;
#define LOWPAN_ACK_AUTO   0
#define LOWPAN_ACK_SW     1
#define LOWPAN_ACK_NOTSUP 2
  
  /** Number of maximum wait ack times */
  u8_t ack_wait_retr;
  
  /** ack wait time. (us) */
  u16_t ack_wait_us;
  
  /** Special mac parameter */
  union {
    struct csma_parameter csma;
    struct tdma_parameter tdma;
  } mac_parameter;
  
#define csma_retr   mac_parameter.csma.csma_param_retr
#define csma_sym_us mac_parameter.csma.csma_param_sym_us
#define tdma_period mac_parameter.tdma.tdma_param_period

  /** Special rdc parameter */
  union {
    struct xmac_parameter xmac;
  } rdc_parameter;
  
#define xmac_ontime_ms      rdc_parameter.xmac.xmac_param_ontime_ms
#define xmac_offtime_ms     rdc_parameter.xmac.xmac_param_offtime_ms
#define xmac_strobe_ms      rdc_parameter.xmac.xmac_param_strobe_ms
#define xmac_strobe_wait_ms rdc_parameter.xmac.xmac_param_strobe_wait_ms
  
  /** The following is mac callback functions is used to some analysis tools or some route algorithm */
  struct mac_callback mcallback;
  
  /** The mac driver struct. 
   *  CSMA Driver recommended (&csma_mac_driver) */
  struct mac_driver *mac_driver;
  
  /* driver MUST set this with a null ptr */
  void *mac_driver_priv; 
  
  /** The rdc driver struct 
   *  if you do not care power consume, you can use NULL RDC Driver (&null_rdc_driver)
   *  if you want low power consume wireless networking, you can use X-MAC RDC Driver (&xmac_rdc_driver) */
  struct rdc_driver *rdc_driver;
  
  /* driver MUST set this with a null ptr */
  void *rdc_driver_priv;
  
#if LOWPAN_AES_CRYPT || LOWPAN_SIMPLE_CRYPT
  /** Special crypt parameter */
  struct crypt_parameter crypt_param;
  
#define crypt_key   crypt_param.key

  /** The ctypt driver struct
   *  if you want use crypt, you can set a crypt driver here, if not just set NULL */
  struct crypt_driver *crypt_driver;
  
  /* driver MUST set this with a null ptr */
  void *crypt_driver_priv;
#endif /* LOWPAN_AES_CRYPT || LOWPAN_SIMPLE_CRYPT */
  
  /** The radio driver struct 
   *  user specified radio chip driver */
  struct radio_driver *radio_driver;
  
  /** driver can use this do anything you like */
  void *radio_driver_priv; 
};

#define RADIO_DRIVER(x) (x->radio_driver)
#define MAC_DRIVER(x)   (x->mac_driver)
#define RDC_DRIVER(x)   (x->rdc_driver)
#define CRYPT_DRIVER(x) (x->crypt_driver)

/*
 * Radio driver call this when receive a packet.
 */
#define RDC_DRIVER_INPUT(x, p)  RDC_DRIVER(x)->input(x, p)

typedef struct netif *(*netif_add_handle)(struct netif *netif,   \
                          ip_addr_t *ipaddr, ip_addr_t *netmask,    \
                          ip_addr_t *gw, void *state, netif_init_fn init, \
                          netif_input_fn input);

struct lowpanif * 
lowpan_netif_add(struct lowpanif *lowpanif, netif_add_handle fn_add,
                 ip_addr_t *ipaddr, ip_addr_t *netmask, ip_addr_t *gw);
void lowpan_netif_set_default(struct lowpanif *lowpanif);

#if LWIP_DHCP
err_t lowpan_netif_dhcp_start(struct lowpanif *lowpanif);
#endif

#if LWIP_AUTOIP
err_t
lowpan_netif_autoip_start(struct lowpanif *lowpanif);
#endif
  
void lowpan_netif_set_up(struct lowpanif *lowpanif);

#ifdef __cplusplus
}
#endif

#endif /* __LOWPAN_IF_H__ */

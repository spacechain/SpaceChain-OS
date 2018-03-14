/**
 * @file
 * radio communication interface of X-MAC 
 * A Short Preamble MAC Protocol for Duty-Cycled Wireless Sensor Networks
 *
 * Verification using sylixos(tm) real-time operating system
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
 * Author: Han.hui <sylixos@gmail.com>
 *
 */

#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/timeouts.h"

#include "lowpan_if.h"

#if LOWPAN_XMAC_RDC /* don't build if not configured for use in radio_param.h */

#define MAX_SEQNO_SAVE      20

#define GET_XMAC_INFO(x)    (struct xmac_info *)((x)->rdc_driver_priv)
#define GET_LAST_ACK(x)     &(GET_XMAC_INFO(x)->lack)

/** Save recently input seq number */
struct seqnos {
  u8_t sender[IEEE802154_MAX_ADDR_LEN];
  u8_t seqno;
  u8_t len;
};

/* Save last ack packet information */
struct last_ack {
  u8_t seqno;
  u8_t valid;
};

/* Save XMAC information */
struct xmac_info {
  /* power state */
  u8_t is_xmac_on;
  u8_t is_radio_on;
  
  /* duty cycled state */
  enum {
    XMAC_IDLE,
    XMAC_SENDING,
    XMAC_RECVING
  } rdc_state;
  
  /* last packet ack */
  struct last_ack lack;
};

static u8_t seqnos_index = 0;
static struct seqnos seq_save[MAX_SEQNO_SAVE];
static u8_t boardcast_address[2] = {0xff, 0xff};
static sys_mutex_t seq_mutex = SYS_MUTEX_NULL;

typedef enum {
  ADDR_MATCH,
  ADDR_NOT_MATCH,
  ADDR_BOARDCAST
} addr_match_t;

/** Compare the address */
static addr_match_t addr_match (u8_t *addr, u8_t *addr_in, u8_t addrlen)
{
  if (addrlen == IEEE802154_MIN_ADDR_LEN) {
    if (memcmp(boardcast_address, addr_in, addrlen) == 0) {
      return ADDR_BOARDCAST;
    }
  }
  if (memcmp(addr, addr_in, addrlen) == 0) {
    return ADDR_MATCH;
  }
  return ADDR_NOT_MATCH;
}

/** XMAC radio on */
static void xmac_radio_on (struct lowpanif *lowpanif)
{
  struct xmac_info *xmac = GET_XMAC_INFO(lowpanif);

  if (xmac->is_xmac_on && !xmac->is_radio_on) {
    xmac->is_radio_on = 1;
    RADIO_DRIVER(lowpanif)->on(lowpanif);
  }
}

/** XMAC radio off */
static void xmac_radio_off (struct lowpanif *lowpanif)
{
  struct xmac_info *xmac = GET_XMAC_INFO(lowpanif);
  
  if (xmac->is_xmac_on && xmac->is_radio_on) {
    xmac->is_radio_on = 0;
    RADIO_DRIVER(lowpanif)->off(lowpanif);
  }
}

/** XMAC timer */
static void xmac_rdc_timer (struct lowpanif *lowpanif)
{
  struct xmac_info *xmac = GET_XMAC_INFO(lowpanif);

  if (!xmac->is_xmac_on) {
    return;
  }
  
  sys_timeout(lowpanif->xmac_offtime_ms, (sys_timeout_handler)xmac_rdc_timer, lowpanif);
}

/** Initialize the RDC driver return 0 is ok */
static void xmac_rdc_init (struct lowpanif *lowpanif)
{
  int ret = RADIO_DRIVER(lowpanif)->init(lowpanif);
  
  if (!sys_mutex_valid(&seq_mutex)) {
    sys_mutex_new(&seq_mutex);
  }

  if (lowpanif->rdc_driver_priv == NULL) {
    struct xmac_info *xmac = (struct xmac_info *)mem_malloc(sizeof(struct xmac_info));
    LWIP_ERROR("xmac != NULL", (xmac != NULL), return;);
    
    xmac->is_xmac_on = 0;
    xmac->is_radio_on = 0;
    lowpanif->rdc_driver_priv = (void *)xmac;
  }
  
  LWIP_ERROR("ret == 0", (ret == 0), return;);
}

/** Send a packet from the Rime buffer  */
static radio_ret_t xmac_rdc_send (struct lowpanif *lowpanif, struct pbuf *p)
{
  struct xmac_info *xmac = GET_XMAC_INFO(lowpanif);
  
  if (xmac->rdc_state != XMAC_IDLE) {
    return RADIO_TX_COLLISION;
  }

  return RADIO_TX_OK;
}

/** Callback for getting notified of incoming packet. */
static void xmac_rdc_input (struct lowpanif *lowpanif, struct pbuf *p)
{
  
}

/** Turn the RDC layer on. */
static int xmac_rdc_on (struct lowpanif *lowpanif)
{
  struct xmac_info *xmac = GET_XMAC_INFO(lowpanif);
  
  LWIP_ERROR("xmac != NULL", (xmac != NULL), return -1;);
  
  xmac->is_xmac_on = 1;

  return 0;
}

/** Turn the RDC layer off. */
static int xmac_rdc_off (struct lowpanif *lowpanif, int keep_radio_on)
{
  struct xmac_info *xmac = GET_XMAC_INFO(lowpanif);
  
  (void)keep_radio_on;
  
  LWIP_ERROR("xmac != NULL", (xmac != NULL), return -1;);
  
  xmac->is_xmac_on = 0;
  
  return 0;
}

/** Returns the channel check interval, expressed in ms. */
static u32_t xmac_rdc_channel_check_interval (struct lowpanif *lowpanif)
{
  return lowpanif->xmac_ontime_ms + lowpanif->xmac_offtime_ms;
}

/** XMAC RDC driver */
struct rdc_driver xmac_rdc_driver = {
  "XMAC",
  xmac_rdc_init,
  xmac_rdc_send,
  xmac_rdc_input,
  xmac_rdc_on,
  xmac_rdc_off,
  xmac_rdc_channel_check_interval
};

#endif /* LOWPAN_XMAC_RDC */
/* end */

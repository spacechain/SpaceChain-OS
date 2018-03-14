/**
 * @file
 * radio communication interface of CSMA-CA MAC Drvier
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

#include "lwip/mem.h"
#include "lwip/timeouts.h"

#include "lowpan_if.h"

#if LOWPAN_CSMA_MAC /* don't build if not configured for use in radio_param.h */

/* CSMA debug mode */
#define CSMA_DEBUG 0 /* if 1 always send packet in timer */

/* CSMA Param */
#define MIN_CSMA_CW 2
#define MAX_CSMA_CW 31
#define MIN_CSMA_BE 0
#define MAX_CSMA_BE 5

/* lowpan interface ieee802.15.4 input */
extern err_t lowpan_input(struct pbuf *p, struct netif *inp);

/* one packet buffer */
struct pbuf_q {
  struct pbuf_q *next;
  struct pbuf *p; /* packet to send */
  u8_t nb; /* Number Of Back */
  u8_t cw; /* content window length */
  u8_t be; /* Backoff exponent */
};

/* Put a packet into mac queue */
static struct pbuf_q *csma_mac_put_pkt_queue (struct lowpanif *lowpanif, struct pbuf *p, u8_t nb)
{
  struct pbuf *s;
  struct pbuf_q *first;
  struct pbuf_q *prev;
  struct pbuf_q *pbufq;
  
  pbufq = (struct pbuf_q *)mem_malloc(sizeof(struct pbuf_q));
  if (pbufq == NULL) {
    LWIP_DEBUGF(NETIF_DEBUG, ("csma_mac_put_pkt_queue: can not allocate pbuf_q!\n"));
    return NULL;
  }
  
  s = pbuf_alloc(PBUF_RAW, p->tot_len, PBUF_RAM);
  if (s == NULL) {
    mem_free(pbufq);
    LWIP_DEBUGF(NETIF_DEBUG, ("csma_mac_put_pkt_queue: can not allocate pbuf!\n"));
    return NULL;
  }
  
  pbuf_copy(s, p);
  
  pbufq->next = NULL;
  pbufq->p  = s;
  pbufq->nb = nb;
  pbufq->cw = MIN_CSMA_CW;
  pbufq->be = MIN_CSMA_BE;
  
  first = (struct pbuf_q *)lowpanif->mac_driver_priv;
  
  /* add to mac_q tail */
  if (first == NULL) {
    lowpanif->mac_driver_priv = (void *)pbufq; /* first packet in queue */
  } else {
    for (prev = first; prev->next != NULL; prev = prev->next);
    prev->next = pbufq;
  }
  
  return pbufq;
}

/* Put a packet into mac queue */
static struct pbuf_q *csma_mac_get_pkt_queue (struct lowpanif *lowpanif)
{
  return (struct pbuf_q *)lowpanif->mac_driver_priv;
}

/* Put a packet into mac queue (return next packet) */
static struct pbuf_q *csma_mac_del_pkt_queue (struct lowpanif *lowpanif)
{
  struct pbuf_q *first = (struct pbuf_q *)lowpanif->mac_driver_priv;
  
  lowpanif->mac_driver_priv = (void *)first->next;
  pbuf_free(first->p);
  mem_free(first);
  
  return (struct pbuf_q *)lowpanif->mac_driver_priv;
}

/* CSMA timer callback (No not need lock, because we in tcpip thread or core lock) */
static void csma_mac_timer (struct lowpanif *lowpanif)
{
  radio_ret_t ret;
  struct pbuf_q *pbufq;
  
resend:
  pbufq = csma_mac_get_pkt_queue(lowpanif); /* There MUST have at least one packet in this queue */
  while (pbufq) {
    pbufq->nb++;
    ret = RDC_DRIVER(lowpanif)->send(lowpanif, pbufq->p, pbufq->nb);
    mac_send_callback(lowpanif, pbufq->p, ret, pbufq->nb);
    if (ret == RADIO_TX_OK) {
      pbufq = csma_mac_del_pkt_queue(lowpanif); /* detele this packet buffer and get next */
    
    } else {
      if (pbufq->nb >= lowpanif->csma_retr) { /* Max times retry ? */
        pbufq = csma_mac_del_pkt_queue(lowpanif); /* Drop this packet */
        LWIP_DEBUGF(NETIF_DEBUG, ("csma_mac_timer: csma try max times and fail!\n"));
        
      } else {
        pbufq->be++;
        if (pbufq->be > MAX_CSMA_BE) {
          pbufq->be = MAX_CSMA_BE;
        }
      }
      break;
    }
  }
  
  if (pbufq) { /* Not all packet has been send out */
#define MS_SHIFT 10 /* use 1024 because we don't want use (wait_us / 1000) and (wait_us % 1000) operate */
#define MS_MASK  ((1 << MS_SHIFT) - 1)
    
    u32_t timeout_us = (u32_t)(LWIP_RAND() % ((1 << pbufq->be) * (20 * lowpanif->csma_sym_us) + 1));
    u16_t delay_us = (u16_t)(timeout_us & MS_MASK);
    
    if (delay_us) {
      RADIO_DRIVER(lowpanif)->delay_us(lowpanif, delay_us);
    }
    
    timeout_us >>= MS_SHIFT; /* change to ms */
    
    if (RDC_DRIVER(lowpanif)->channel_check_interval) { /* we use xmac, so must add a xmac duty cycled time */
      timeout_us += RDC_DRIVER(lowpanif)->channel_check_interval(lowpanif);
    }
    
    if (timeout_us) {
      sys_timeout(timeout_us, (sys_timeout_handler)csma_mac_timer, lowpanif);
    
    } else {
      goto resend; /* only use software delay */
    }
  }
}

/* Initialize the MAC driver return 0 is ok */
static void csma_mac_init (struct lowpanif *lowpanif)
{
  RDC_DRIVER(lowpanif)->init(lowpanif);
}

/** Send a packet from the Rime buffer (No not need lock, because we in tcpip thread or core lock) */
static radio_ret_t csma_mac_send (struct lowpanif *lowpanif, struct pbuf *p)
{
  radio_ret_t ret;
  struct pbuf_q *first;
  struct pbuf_q *pbufq;
  
  if (lowpanif->csma_type == LOWPAN_CSMA_SW) {
    first = csma_mac_get_pkt_queue(lowpanif);
    if (first) { /* is there has packet in send queue ? */
      pbufq = csma_mac_put_pkt_queue(lowpanif, p, 0); /* put this packet into send queue */
      if (pbufq) {
        ret = RADIO_TX_OK;
      } else {
        ret = RADIO_TX_ERR;
      }
    } else { /* No packet in queue */
#if CSMA_DEBUG > 0
      ret = RADIO_TX_COLLISION;
#else
      ret = RDC_DRIVER(lowpanif)->send(lowpanif, p, 1); /* the first try send */
      mac_send_callback(lowpanif, p, ret, 1);
#endif /* CSMA_DEBUG */
      if ((ret == RADIO_TX_COLLISION) || (ret == RADIO_TX_NOACK)) {
        pbufq = csma_mac_put_pkt_queue(lowpanif, p, 1); /* put this packet into send queue */
        if (pbufq) {
          ret = RADIO_TX_OK;
          sys_timeout(1, (sys_timeout_handler)csma_mac_timer, lowpanif); /* set CSMA timer first timeout */
        } else {
          ret = RADIO_TX_ERR;
        }
      }
    }
  } else {
    ret = RDC_DRIVER(lowpanif)->send(lowpanif, p, 1);
    mac_send_callback(lowpanif, p, ret, 1);
    if (ret != RADIO_TX_OK) {
      LWIP_DEBUGF(NETIF_DEBUG, ("csma_mac_send: send packet fail!\n"));
    }
  }
  
  return ret;
}

/** Callback for getting notified of incoming packet. */
static void csma_mac_input (struct lowpanif *lowpanif, struct pbuf *p)
{
  mac_input_callback(lowpanif, p);
  lowpan_input(p, &lowpanif->netif);
}

/** Turn the MAC layer on. */
static int csma_mac_on (struct lowpanif *lowpanif)
{
  return RDC_DRIVER(lowpanif)->on(lowpanif);
}

/** Turn the MAC layer off. */
static int csma_mac_off (struct lowpanif *lowpanif, int keep_radio_on)
{
  return RDC_DRIVER(lowpanif)->off(lowpanif, keep_radio_on);
}

/** Returns the channel check interval, expressed in ms. */
static u32_t csma_mac_channel_check_interval (struct lowpanif *lowpanif)
{
  if (RDC_DRIVER(lowpanif)->channel_check_interval) {
    return RDC_DRIVER(lowpanif)->channel_check_interval(lowpanif);
  }
  return 0;
}

/** CSMA MAC driver */
struct mac_driver csma_mac_driver = {
  "CSMA",
  csma_mac_init,
  csma_mac_send,
  csma_mac_input,
  csma_mac_on,
  csma_mac_off,
  csma_mac_channel_check_interval
};

#endif /* LOWPAN_CSMA_MAC */
/* end */

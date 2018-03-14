/**
 * @file
 * radio communication interface of TDMA MAC Drvier
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
#include "lwip/tcpip.h"

#include "lowpan_if.h"

#if LOWPAN_TDMA_MAC /* don't build if not configured for use in radio_param.h */

/* Save TDMA send queue */
struct pbuf_q {
  struct pbuf_q *next;
  struct pbuf *p; /* packet to send */
};

/* lowpan interface ieee802.15.4 input */
extern err_t lowpan_input(struct pbuf *p, struct netif *inp);

/* Put a packet into mac queue */
static struct pbuf_q *tdma_mac_put_pkt_queue (struct lowpanif *lowpanif, struct pbuf *p)
{
  struct pbuf *s;
  struct pbuf_q *first;
  struct pbuf_q *prev;
  struct pbuf_q *pbufq;
  
  pbufq = (struct pbuf_q *)mem_malloc(sizeof(struct pbuf_q));
  if (pbufq == NULL) {
    LWIP_DEBUGF(NETIF_DEBUG, ("tdma_mac_put_pkt_queue: can not allocate pbuf_q!\n"));
    return NULL;
  }
  
  s = pbuf_alloc(PBUF_RAW, p->tot_len, PBUF_RAM);
  if (s == NULL) {
    mem_free(pbufq);
    LWIP_DEBUGF(NETIF_DEBUG, ("tdma_mac_put_pkt_queue: can not allocate pbuf!\n"));
    return NULL;
  }
  
  pbuf_copy(s, p);
  
  pbufq->next = NULL;
  pbufq->p  = s;
  
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
static struct pbuf_q *tdma_mac_get_pkt_queue (struct lowpanif *lowpanif)
{
  return (struct pbuf_q *)lowpanif->mac_driver_priv;
}

/* Put a packet into mac queue (return next packet) */
static struct pbuf_q *tdma_mac_del_pkt_queue (struct lowpanif *lowpanif)
{
  struct pbuf_q *first = (struct pbuf_q *)lowpanif->mac_driver_priv;
  
  lowpanif->mac_driver_priv = (void *)first->next;
  pbuf_free(first->p);
  mem_free(first);
  
  return (struct pbuf_q *)lowpanif->mac_driver_priv;
}

/* TDMA timer callback (No not need lock, because we in tcpip thread or core lock) */
static void tdma_mac_timer (struct lowpanif *lowpanif)
{
  radio_ret_t ret;
  struct pbuf_q *pbufq;
  
  pbufq = tdma_mac_get_pkt_queue(lowpanif); /* get the first packet in this queue */
  while (pbufq) {
    ret = RDC_DRIVER(lowpanif)->send(lowpanif, pbufq->p, 1);
    mac_send_callback(lowpanif, pbufq->p, ret, 1);
    pbufq = tdma_mac_del_pkt_queue(lowpanif); /* detele this packet buffer and get next */
    if (ret != RADIO_TX_OK) {
      LWIP_DEBUGF(NETIF_DEBUG, ("tdma_mac_timer: tdma transmit packet fail!\n"));
    }
  }
}

/* Initialize the MAC driver return 0 is ok */
static void tdma_mac_init (struct lowpanif *lowpanif)
{
  RDC_DRIVER(lowpanif)->init(lowpanif);
  
  /* radio driver MUST use hardware CSMA */
  LWIP_ASSERT("Very dangerous to use TDMA Mac but without hardware CSMA!\n",
              (lowpanif->csma_type == LOWPAN_CSMA_HW));
}

/** Send a packet from the Rime buffer  */
static radio_ret_t tdma_mac_send (struct lowpanif *lowpanif, struct pbuf *p)
{
  radio_ret_t ret;
  struct pbuf_q *first;
  struct pbuf_q *pbufq;
  
  first = tdma_mac_get_pkt_queue(lowpanif);
  pbufq = tdma_mac_put_pkt_queue(lowpanif, p); /* put this packet into send queue */
  if (pbufq) {
    ret = RADIO_TX_OK;
  } else {
    ret = RADIO_TX_ERR;
  }
  
  if (!first) {
    sys_timeout(lowpanif->tdma_period, (sys_timeout_handler)tdma_mac_timer, lowpanif);
  }

  return ret;
}

/** Callback for getting notified of incoming packet. */
static void tdma_mac_input (struct lowpanif *lowpanif, struct pbuf *p)
{
  mac_input_callback(lowpanif, p);
  lowpan_input(p, &lowpanif->netif);
}

/** Turn the MAC layer on. */
static int tdma_mac_on (struct lowpanif *lowpanif)
{
  return RDC_DRIVER(lowpanif)->on(lowpanif);
}

/** Turn the MAC layer off. */
static int tdma_mac_off (struct lowpanif *lowpanif, int keep_radio_on)
{
  return RDC_DRIVER(lowpanif)->off(lowpanif, keep_radio_on);
}

/** Returns the channel check interval, expressed in ms. */
static u32_t tdma_mac_channel_check_interval (struct lowpanif *lowpanif)
{
  if (RDC_DRIVER(lowpanif)->channel_check_interval) {
    return RDC_DRIVER(lowpanif)->channel_check_interval(lowpanif);
  }
  return 0;
}

/** TDMA MAC driver */
struct mac_driver tdma_mac_driver = {
  "TDMA",
  tdma_mac_init,
  tdma_mac_send,
  tdma_mac_input,
  tdma_mac_on,
  tdma_mac_off,
  tdma_mac_channel_check_interval
};

#endif /* LOWPAN_TDMA_MAC */
/* end */

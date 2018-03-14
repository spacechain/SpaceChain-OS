/**
 * @file
 * radio communication interface of null Duty-Cycled
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

#ifdef SYLIXOS
#define  __SYLIXOS_KERNEL
#endif

#include "lwip/sys.h"
#include "lwip/mem.h"

#include "lowpan_if.h"

#if LOWPAN_NULL_RDC /* don't build if not configured for use in radio_param.h */

#if LOWPAN_AES_CRYPT || LOWPAN_SIMPLE_CRYPT
#include "crypt_driver.h"
#endif /* LOWPAN_AES_CRYPT || LOWPAN_SIMPLE_CRYPT */

#include "string.h"

#define ACK_LEN         3
#define MAX_SEQNO_SAVE  20
#define REQ_ACK_BIT_OFF 5
#define REQ_ACK(p)      (u8_t)(*(u8_t *)p->payload & (u8_t)(1 << REQ_ACK_BIT_OFF))
#define SEQ_NO(p)       (u8_t)(*((u8_t *)p->payload + 2))

#define GET_LAST_ACK(x) (struct last_ack *)((x)->rdc_driver_priv)

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

/** Initialize the RDC driver return 0 is ok */
static void null_rdc_init (struct lowpanif *lowpanif)
{
  int ret = RADIO_DRIVER(lowpanif)->init(lowpanif);
  
#if LOWPAN_AES_CRYPT || LOWPAN_SIMPLE_CRYPT
  crypt_init(lowpanif);
#endif /* LOWPAN_AES_CRYPT || LOWPAN_SIMPLE_CRYPT */

  if (!sys_mutex_valid(&seq_mutex)) {
    sys_mutex_new(&seq_mutex);
  }
  
  if (lowpanif->rdc_driver_priv == NULL) {
    struct last_ack *lack = (struct last_ack *)mem_malloc(sizeof(struct last_ack));
    LWIP_ERROR("lack != NULL", (lack != NULL), return;);
    
    lack->seqno = 0;
    lack->valid = 0;
    lowpanif->rdc_driver_priv = (void *)lack;
  }
  
  LWIP_ERROR("ret == 0", (ret == 0), return;);
}

/** Send a packet from the Rime buffer  */
static radio_ret_t null_rdc_send (struct lowpanif *lowpanif, struct pbuf *p, u8_t numtx)
{
  u8_t i;
  u8_t seqno;
  u8_t ack_required;
  radio_ret_t ret;
  
  LWIP_ERROR("p != NULL", (p != NULL), return RADIO_TX_ERR;);
  LWIP_ERROR("p->len > 0", (p->len > 0), return RADIO_TX_ERR;);
  
#if LOWPAN_AES_CRYPT || LOWPAN_SIMPLE_CRYPT
  if (numtx == 1) { /* the first time encrypt */
    struct pbuf *pencrypt = crypt_encode(lowpanif, p);
    if (pencrypt == NULL) {
      return RADIO_TX_ERR;
    }
    p = pencrypt;
  }
#endif /* LOWPAN_AES_CRYPT || LOWPAN_SIMPLE_CRYPT */
  
  ack_required = REQ_ACK(p);

  if (lowpanif->csma_type == LOWPAN_CSMA_HW) { /* chip auto CSMA retries */
    ret = RADIO_DRIVER(lowpanif)->send(lowpanif, p);
    
  } else {
    seqno = SEQ_NO(p);
  
    RADIO_DRIVER(lowpanif)->prepare(lowpanif, p); /* driver prepare for a packet transmit */
    
    if (RADIO_DRIVER(lowpanif)->receiving_packet(lowpanif) ||
        RADIO_DRIVER(lowpanif)->pending_packet(lowpanif) ||
        RADIO_DRIVER(lowpanif)->channel_clear(lowpanif) == 0) {
      ret = RADIO_TX_COLLISION; /* collision */
    
    } else {
      struct last_ack *lack = GET_LAST_ACK(lowpanif);
      
      if (lack) {
        lack->valid = 0; /* make this lowpenif last ack not valid */
      }
      
      ret = RADIO_DRIVER(lowpanif)->transmit(lowpanif, p); /* radio driver transmit */
      if (ret == RADIO_TX_OK) { /* driver transmit ok */
        
        if ((lowpanif->ack_type == LOWPAN_ACK_NOTSUP) || (!ack_required)) {
          ret = RADIO_TX_OK; /* Do not need ack */
      
        } else {
          ret = RADIO_TX_NOACK;
        
          /* we want waiting ack */
          for (i = 0; i < lowpanif->ack_wait_retr; i++) {
            if (lack && lack->valid && lack->seqno == seqno) { /* ack has been received in null_rdc_input() */
              ret = RADIO_TX_OK;
              break;
            }
          
            if (RADIO_DRIVER(lowpanif)->pending_packet(lowpanif)) { /* has a pending packet */
              struct pbuf *ack = RADIO_DRIVER(lowpanif)->read(lowpanif);
              if (ack) {
                if ((ack->len == ACK_LEN) && (SEQ_NO(ack) == seqno)) { /* is this packet is ack for us */
                  ret = RADIO_TX_OK;
                }
                pbuf_free(ack);
                if (ret == RADIO_TX_OK) {
                  break;
                }
              }
            }
            RADIO_DRIVER(lowpanif)->delay_us(lowpanif, lowpanif->ack_wait_us); /* delay */
          }
        }
      }
    }
  }
  
#if LOWPAN_AES_CRYPT || LOWPAN_SIMPLE_CRYPT
  if (numtx == 1) {
    pbuf_free(p); /* free crypt pbuf */
  }
#endif /* LOWPAN_AES_CRYPT || LOWPAN_SIMPLE_CRYPT */
  
  return ret;
}

/** Callback for getting notified of incoming packet. */
static void null_rdc_input (struct lowpanif *lowpanif, struct pbuf *p)
{
  u8_t i;
  u8_t hdrlen;
  u8_t dest_addrlen;
  u8_t src_addrlen;
  struct last_ack *lack;
  ieee802154_frame_t frame;
  
  LWIP_ERROR("p != NULL", (p != NULL), return;);
  LWIP_ERROR("p->len > 2", (p->len > 2), return;);
  LWIP_ERROR("p->tot_len == p->len", (p->tot_len == p->len), return;);
  
  /* If this is a ack packet, Save it information */
  if (p->tot_len == ACK_LEN) {
    lack = GET_LAST_ACK(lowpanif);
    if (lack) {
      lack->seqno = SEQ_NO(p);
      lack->valid = 1;
    }
#ifdef SYLIXOS
    KN_SMP_WMB();
#endif
    pbuf_free(p);
    return;
  }
  
#if LOWPAN_AES_CRYPT || LOWPAN_SIMPLE_CRYPT
  {
    struct pbuf *pdecrypt = crypt_decode(lowpanif, p);
    if (pdecrypt == NULL) {
      return;
    }
    pbuf_free(p);
    p = pdecrypt;
  }
#endif /* LOWPAN_AES_CRYPT || LOWPAN_SIMPLE_CRYPT */
  
  /* Send ack packet */
  if (lowpanif->ack_type == LOWPAN_ACK_SW) { /* Need ack */
    if (REQ_ACK(p)) {
      u8_t ack_buf[ACK_LEN];
      struct pbuf *ack = pbuf_alloc_reference(ack_buf, ACK_LEN, PBUF_ROM);
      if (ack) {
        ieee802154_frame_create_ack(ack_buf, ACK_LEN, SEQ_NO(p));
        if (lowpanif->csma_type == LOWPAN_CSMA_HW) {
          RADIO_DRIVER(lowpanif)->send(lowpanif, ack);
        } else {
          RADIO_DRIVER(lowpanif)->prepare(lowpanif, ack);
          RADIO_DRIVER(lowpanif)->transmit(lowpanif, ack);
        }
      }
    }
  }
  
  /* If this is a IEEE802.15.4 format packet */
  hdrlen = ieee802154_frame_parse((u8_t *)p->payload, p->len, &frame);
  if (hdrlen == 0) {
    LWIP_DEBUGF(NETIF_DEBUG, ("null_rdc_input: hdrlen == 0\n"));
    pbuf_free(p);
    return;
  }
  
  if (frame.fcf.dest_addr_mode == IEEE802154_SHORTADDRMODE) {
    dest_addrlen = IEEE802154_MIN_ADDR_LEN;
  } else {
    dest_addrlen = IEEE802154_MAX_ADDR_LEN;
  }
  
  if (frame.fcf.src_addr_mode == IEEE802154_SHORTADDRMODE) {
    src_addrlen = IEEE802154_MIN_ADDR_LEN;
  } else {
    src_addrlen = IEEE802154_MAX_ADDR_LEN;
  }
  
  /* If this packet not for me? */
  if (addr_match(lowpanif->addr, frame.dest_addr, dest_addrlen) == ADDR_NOT_MATCH) {
    LWIP_DEBUGF(NETIF_DEBUG, ("null_rdc_input: not for us.\n"));
    pbuf_free(p);
    return;
  }
  
  sys_mutex_lock(&seq_mutex); /* lock seq save */
  
  /* If this packet is duplicate */
  for (i = 0; i < MAX_SEQNO_SAVE; i++) {
    if ((addr_match(seq_save[i].sender, frame.src_addr, src_addrlen) == ADDR_MATCH) &&
        (frame.seq == seq_save[i].seqno) && (seq_save[i].len == (u8_t)p->len)) {
      sys_mutex_unlock(&seq_mutex); /* unlock seq save */
      
      LWIP_DEBUGF(NETIF_DEBUG, ("null_rdc_input: drop duplicate link layer packet %u\n", frame.seq));
      pbuf_free(p);
      return;
    }
  }
  
  /* Save packet seqno */
  MEMCPY(seq_save[seqnos_index].sender, frame.src_addr, src_addrlen);
  seq_save[seqnos_index].seqno = frame.seq;
  seq_save[seqnos_index].len = (u8_t)p->len;
  seqnos_index++;
  if (seqnos_index >= MAX_SEQNO_SAVE) {
    seqnos_index = 0;
  }
  
  sys_mutex_unlock(&seq_mutex); /* unlock seq save */
  
  MAC_DRIVER(lowpanif)->input(lowpanif, p);
}

/** Turn the RDC layer on. */
static int null_rdc_on (struct lowpanif *lowpanif)
{
  return RADIO_DRIVER(lowpanif)->on(lowpanif);
}

/** Turn the RDC layer off. */
static int null_rdc_off (struct lowpanif *lowpanif, int keep_radio_on)
{
  if (keep_radio_on) {
    return RADIO_DRIVER(lowpanif)->on(lowpanif);
  } else {
    return RADIO_DRIVER(lowpanif)->off(lowpanif);
  }
}

/** Returns the channel check interval, expressed in ms. */
static u32_t null_rdc_channel_check_interval (struct lowpanif *lowpanif)
{
  return 0;
}

/** NULL RDC driver */
struct rdc_driver null_rdc_driver = {
  "NULL",
  null_rdc_init,
  null_rdc_send,
  null_rdc_input,
  null_rdc_on,
  null_rdc_off,
  null_rdc_channel_check_interval
};

#endif /* LOWPAN_NULL_RDC */
/* end */

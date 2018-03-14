/**
 * @file
 * radio communication interface
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

#ifndef __RADIO_DRIVER_H__
#define __RADIO_DRIVER_H__

#include "lwip/opt.h"
#include "lwip/pbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * Declare the lowpan_if.
 */
struct lowpanif;

/* Generic radio return values. */
typedef enum {
  RADIO_TX_OK,
  RADIO_TX_ERR,
  RADIO_TX_COLLISION,
  RADIO_TX_NOACK
} radio_ret_t;

/**
 * The structure of a radio driver in lwip radio system.
 */
struct radio_driver {
  char *name;

  /** Initialize the radio driver return 0 is ok */
  int (* init)(struct lowpanif *lowpanif);
  
  /** Prepare the radio with a packet to be sent. */
  int (* prepare)(struct lowpanif *lowpanif, struct pbuf *p);
  
  /** Send the packet that has previously been prepared. */
  radio_ret_t (* transmit)(struct lowpanif *lowpanif, struct pbuf *p);
  
  /** Prepare & transmit a packet. */
  radio_ret_t (* send)(struct lowpanif *lowpanif, struct pbuf *p);
  
  /** Read a received packet into a buffer. */
  struct pbuf * (* read)(struct lowpanif *lowpanif);
  
  /** Perform a Clear-Channel Assessment (CCA) to find out if there is
      a packet in the air or not. return 1 is clear 0 is busy */
  int (* channel_clear)(struct lowpanif *lowpanif);
  
  /** Check if the radio driver is currently receiving a packet 
      return 1 is receiving 0 is idle */
  int (* receiving_packet)(struct lowpanif *lowpanif);
  
  /** Check if the radio driver has just received a packet 
      return 1 is receiving 0 is idle */
  int (* pending_packet)(struct lowpanif *lowpanif);
  
  /** Set the PAN ID and addr. */
  int (* set_pan_addr)(struct lowpanif *lowpanif, u16_t panid, u16_t shaddr, u8_t *lladdr);
  
  /** Set the channel. */
  int (* set_channel)(struct lowpanif *lowpanif, u16_t channel);
  
  /** delay us. */
  int (* delay_us)(struct lowpanif *lowpanif, u16_t us);
  
  /** Turn the radio on. */
  int (* on)(struct lowpanif *lowpanif);

  /** Turn the radio off. */
  int (* off)(struct lowpanif *lowpanif);
};

#ifdef __cplusplus
}
#endif

#endif /* __RADIO_DRIVER_H__ */

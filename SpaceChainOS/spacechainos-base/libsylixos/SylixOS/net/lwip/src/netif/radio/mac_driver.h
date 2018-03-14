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

#ifndef __MAC_DRIVER_H__
#define __MAC_DRIVER_H__

#include "lwip/opt.h"
#include "lwip/pbuf.h"

#include "radio_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The structure of a MAC driver in lwip radio system.
 */
struct mac_driver {
  /* mac driver name */
  char *name;
  
  /** Initialize the MAC driver */
  void (* init)(struct lowpanif *lowpanif);
  
  /** Send a packet from the Rime buffer  */
  radio_ret_t (* send)(struct lowpanif *lowpanif, struct pbuf *p);

  /** Callback for getting notified of incoming packet. */
  void (* input)(struct lowpanif *lowpanif, struct pbuf *p);

  /** Turn the MAC layer on. */
  int (* on)(struct lowpanif *lowpanif);

  /** Turn the MAC layer off. */
  int (* off)(struct lowpanif *lowpanif, int keep_radio_on);

  /** Returns the channel check interval, expressed in ticks. */
  u32_t (* channel_check_interval)(struct lowpanif *lowpanif);
};

/**
 * The structure of a MAC driver callback.
 */
struct mac_callback {
  /** The following is mac callback functions is used to some analysis tools or some route algorithm */
  /* mac send callback */
  void (* send_callback)(struct lowpanif *lowpanif, struct pbuf *p, radio_ret_t ret, u8_t numtx);
  
  /* mac input callback */
  void (* input_callback)(struct lowpanif *lowpanif, struct pbuf *p);
};

/*
 * Mac driver call this when send / receive a packet.
 */
void mac_send_callback(struct lowpanif *lowpanif, struct pbuf *p, radio_ret_t ret, u8_t numtx);
void mac_input_callback(struct lowpanif *lowpanif, struct pbuf *p);

#ifdef __cplusplus
}
#endif

#endif /* __MAC_DRIVER_H__ */

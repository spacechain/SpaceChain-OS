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

#include "lowpan_if.h"

#if LOWPAN_NULL_MAC /* don't build if not configured for use in radio_param.h */

/* lowpan interface ieee802.15.4 input */
extern err_t lowpan_input(struct pbuf *p, struct netif *inp);

/* Initialize the MAC driver return 0 is ok */
static void null_mac_init (struct lowpanif *lowpanif)
{
  RDC_DRIVER(lowpanif)->init(lowpanif);
}

/** Send a packet from the Rime buffer  */
static radio_ret_t null_mac_send (struct lowpanif *lowpanif, struct pbuf *p)
{
  radio_ret_t ret = RDC_DRIVER(lowpanif)->send(lowpanif, p, 1);
  
  mac_send_callback(lowpanif, p, ret, 1);
  
  return ret;
}

/** Callback for getting notified of incoming packet. */
static void null_mac_input (struct lowpanif *lowpanif, struct pbuf *p)
{
  mac_input_callback(lowpanif, p);
  lowpan_input(p, &lowpanif->netif);
}

/** Turn the MAC layer on. */
static int null_mac_on (struct lowpanif *lowpanif)
{
  return RDC_DRIVER(lowpanif)->on(lowpanif);
}

/** Turn the MAC layer off. */
static int null_mac_off (struct lowpanif *lowpanif, int keep_radio_on)
{
  return RDC_DRIVER(lowpanif)->off(lowpanif, keep_radio_on);
}

/** Returns the channel check interval, expressed in ms. */
static u32_t null_mac_channel_check_interval (struct lowpanif *lowpanif)
{
  if (RDC_DRIVER(lowpanif)->channel_check_interval) {
    return RDC_DRIVER(lowpanif)->channel_check_interval(lowpanif);
  }
  return 0;
}

/** NULL MAC driver */
struct mac_driver null_mac_driver = {
  "NULL",
  null_mac_init,
  null_mac_send,
  null_mac_input,
  null_mac_on,
  null_mac_off,
  null_mac_channel_check_interval
};

#endif /* LOWPAN_NULL_MAC */
/* end */

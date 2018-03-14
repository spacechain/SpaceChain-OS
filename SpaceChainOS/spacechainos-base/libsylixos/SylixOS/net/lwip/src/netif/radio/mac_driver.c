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

/**
 * The mac callback function when send a frame.
 *
 * @param lowpanif netif
 * @param p The frame to send.
 * @param status The mac status after send the frame.
 * @param numtx The time to send this frame.
 * @return none
 */
void mac_send_callback (struct lowpanif *lowpanif, struct pbuf *p, radio_ret_t ret, u8_t numtx)
{
  if (lowpanif->mcallback.send_callback) {
    lowpanif->mcallback.send_callback(lowpanif, p, ret, numtx);
  }
}

/**
 * The mac callback function when input a frame.
 *
 * @param lowpanif netif
 * @param p The frame to send.
 * @return none
 */
void mac_input_callback (struct lowpanif *lowpanif, struct pbuf *p)
{
  if (lowpanif->mcallback.input_callback) {
    lowpanif->mcallback.input_callback(lowpanif, p);
  }
}
/* end */
/**
 * @file
 * IEEE 802.15.4 frame crypt support. 
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
 * Author: Han.hui <sylixos@gmail.com>
 *
 */

#include "lowpan_if.h"

#if LOWPAN_AES_CRYPT || LOWPAN_SIMPLE_CRYPT
/**
 * crypt driver initialize.
 *
 * @param lowpanif netif
 */
void crypt_init (struct lowpanif *lowpanif)
{
  if (CRYPT_DRIVER(lowpanif)) {
    CRYPT_DRIVER(lowpanif)->init(lowpanif);
  }
}

/**
 * crypt driver encode data.
 *
 * @param lowpanif netif
 * @param p The frame to send.
 * @return encode pbuf
 */
struct pbuf *crypt_encode (struct lowpanif *lowpanif, struct pbuf *p)
{
  if (CRYPT_DRIVER(lowpanif)) {
    return CRYPT_DRIVER(lowpanif)->encrypt(lowpanif, p);
  } else {
    pbuf_ref(p); /* return pbuf is same as argument, ref it */
    return p;
  }
}

/**
 * crypt driver decode data.
 *
 * @param lowpanif netif
 * @param p The frame has recieved.
 * @return decode pbuf
 */
struct pbuf *crypt_decode (struct lowpanif *lowpanif, struct pbuf *p)
{
  if (CRYPT_DRIVER(lowpanif)) {
    return CRYPT_DRIVER(lowpanif)->decrypt(lowpanif, p);
  } else {
    pbuf_ref(p); /* return pbuf is same as argument, ref it */
    return p;
  }
}

/**
 * crypt driver verify data.
 *
 * @param lowpanif netif
 * @param p The frame has recieved.
 * @return verify_ret_t
 */
verify_ret_t crypt_verify (struct lowpanif *lowpanif, struct pbuf *p)
{
  if (CRYPT_DRIVER(lowpanif)) {
    return CRYPT_DRIVER(lowpanif)->verify(lowpanif, p);
  } else {
    return CRYPT_OK;
  }
}

#endif /* LOWPAN_AES_CRYPT || LOWPAN_SIMPLE_CRYPT */

/* end */

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

#ifndef __CRYPT_DRIVER_H__
#define __CRYPT_DRIVER_H__

#include "lwip/opt.h"
#include "lwip/pbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

#if LOWPAN_AES_CRYPT || LOWPAN_SIMPLE_CRYPT
/** 
 * Declare the lowpan_if.
 */
struct lowpanif;

/* Generic verify return values. */
typedef enum {
  CRYPT_OK,
  CRYPT_FAILED,
  CRYPT_TOOSHORT
} verify_ret_t;

/**
 * The structure of a crypt driver in lwip radio system.
 */
struct crypt_driver {
  /* crypt driver name */
  char *name;
  
  /** Initialize the crypt driver */
  void (* init)(struct lowpanif *lowpanif);
  
  /** Encrypt the packet when send it. the return p is encrypt data, 
   *  NOTICE: rdc driver MUST free this pbuf */
  struct pbuf *(* encrypt)(struct lowpanif *lowpanif, struct pbuf *p);
  
  /** Decrypt the packet when receive it. 
   *  NOTICE: the return p is decrypt data, rdc driver MUST use this pbuf up to mac driver then free argument pbuf */
  struct pbuf *(* decrypt)(struct lowpanif *lowpanif, struct pbuf *p);
  
  /** Verify the packet when receive it. */
  verify_ret_t (* verify)(struct lowpanif *lowpanif, struct pbuf *p);
};

/*
 * Rdc driver call this when send / receive a packet.
 */
void         crypt_init(struct lowpanif *lowpanif);
struct pbuf *crypt_encode(struct lowpanif *lowpanif, struct pbuf *p);
struct pbuf *crypt_decode(struct lowpanif *lowpanif, struct pbuf *p);
verify_ret_t crypt_verify(struct lowpanif *lowpanif, struct pbuf *p);

#endif /* LOWPAN_AES_CRYPT || LOWPAN_SIMPLE_CRYPT */

#ifdef __cplusplus
}
#endif

#endif /* __CRYPT_DRIVER_H__ */

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

#ifndef __RADIO_PARAM_H__
#define __RADIO_PARAM_H__

/** LOWPAN_CSMA_MAC==1: Enable CSMA/CA Mac drvier */
#ifndef LOWPAN_CSMA_MAC
#define LOWPAN_CSMA_MAC 1
#endif

/** LOWPAN_TDMA_MAC==1: Enable TDMA Mac drvier */
#ifndef LOWPAN_TDMA_MAC
#define LOWPAN_TDMA_MAC 1
#endif

/** LOWPAN_NULL_MAC==1: Enable NULL Mac drvier */
#ifndef LOWPAN_NULL_MAC
#define LOWPAN_NULL_MAC 1
#endif

/** LOWPAN_XMAC_RDC==1: Enable XMAC RDC drvier (Unimplemented!) */
#ifndef LOWPAN_XMAC_RDC
#define LOWPAN_XMAC_RDC 0
#endif

/** LOWPAN_NULL_RDC==1: Enable NULL RDC drvier */
#ifndef LOWPAN_NULL_RDC
#define LOWPAN_NULL_RDC 1
#endif

/** LOWPAN_AES==1: Enable IEEE 802.15.4 aes crypt */
#ifndef LOWPAN_AES_CRYPT
#define LOWPAN_AES_CRYPT 1
#endif

/** Control level of authentication security. Less than 4 is 
    considered insecure. */
#ifndef LOWPAN_AES_MIC_LEN
#define LOWPAN_AES_MIC_LEN 4
#endif

/** LOWPAN_SIMPLE_CRYPT==1: Enable a simple crypt support */
#ifndef LOWPAN_SIMPLE_CRYPT
#define LOWPAN_SIMPLE_CRYPT 1
#endif

/** Setting this to 0, you can turn off checking the fragments for overlapping
 * regions. The code gets a little smaller. Only use this if you know that
 * overlapping won't occur on your network! */
#ifndef LOWPAN_REASS_CHECK_OVERLAP
#define LOWPAN_REASS_CHECK_OVERLAP 1
#endif /* LOWPAN_REASS_CHECK_OVERLAP */

/** Set to 0 to prevent freeing the oldest datagram when the reassembly buffer is
 * full (LOWPAN_REASS_MAX_PBUFS pbufs are enqueued). The code gets a little smaller.
 * Datagrams will be freed by timeout only. Especially useful when MEMP_NUM_REASSDATA
 * is set to 1, so one datagram can be reassembled at a time, only. */
#ifndef LOWPAN_REASS_FREE_OLDEST
#define LOWPAN_REASS_FREE_OLDEST 1
#endif /* LOWPAN_REASS_FREE_OLDEST */

/**
 * LOWPAN_FRAG_USES_STATIC_BUF==1: Use a static MTU-sized buffer for lowpan
 * fragmentation. Otherwise pbufs are allocated and reference the original
 * packet data to be fragmented.
 */
#ifndef LOWPAN_FRAG_USES_STATIC_BUF
#define LOWPAN_FRAG_USES_STATIC_BUF 1
#endif /* LOWPAN_FRAG_USES_STATIC_BUF */

/**
 * LOWPAN_FRAG_MAX_MTU: Assumed max MTU on any interface for lowpan frag buffer
 * (requires LOWPAN_FRAG_USES_STATIC_BUF==1)
 * at least bigger than IEEE802154_MAX_MTU
 */
#ifndef LOWPAN_FRAG_MAX_MTU
#define LOWPAN_FRAG_MAX_MTU IEEE802154_MAX_MTU
#endif /* LOWPAN_FRAG_MAX_MTU */

#endif /* __RADIO_PARAM_H__ */

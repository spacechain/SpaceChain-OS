/**
 * @file
 * lowpan fragmentation and reassembly, which is used for wireless ieee 802.15.4.
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
 * Author: Ren.Haibo <habbyren@qq.com>
 *
 */

#include "lwip/mem.h"
#include "lwip/timeouts.h"
#include "lwip/tcpip.h"

#if LWIP_SUPPORT_CUSTOM_PBUF

#include "lowpan_if.h"
#include "lowpan_compress.h"
#include "lowpan_frag.h"
#include "ieee802154_eth.h"

#include "string.h"

#define LOWPAN_FRAG_OFFSET_MASK  0xf8
#define LOWPAN_FRAG_OFFSET_SHIFT 3

/**
 * The lowpan reassembly code currently has the following limitations:
 * - fragments must not overlap (e.g. due to different routes),
 *   currently, overlapping or duplicate fragments are thrown away
 *   if LOWPAN_REASS_CHECK_OVERLAP=1 (the default)!
 *
 */
#define LOWPAN_REASS_FLAG_LASTFRAG 0x01

/** This is a helper struct which holds the starting
 * offset and the ending offset of this fragment to
 * easily chain the fragments.
 * It has the same packing requirements as the IPv6 header, since it replaces
 * the Fragment Header in memory in incoming fragments to keep
 * track of the various fragments.
 */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct lowpan_reass_helper {
  PACK_STRUCT_FIELD(struct pbuf *next_pbuf);
  PACK_STRUCT_FIELD(u16_t start);
  PACK_STRUCT_FIELD(u16_t end);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

#define LOWPAN_TAG_GET(frag_hdr)                \
        ((((u16_t)frag_hdr[2]) << 8) | (u16_t)frag_hdr[3])

#define LOWPAN_ADDRESSES_AND_ID_MATCH(lowpanr, ethhdr, frag_hdr)     \
        ((eth_addr_cmp(&(lowpanr->ethhdr.dest), &(ethhdr->dest))) && \
         (eth_addr_cmp(&(lowpanr->ethhdr.src), &(ethhdr->src))) &&   \
		 (LOWPAN_TAG_GET(frag_hdr) == lowpanr->datagram_tag))

/* static variables */
static struct lowpan_reassdata *reassdatagrams;
static u16_t lowpan_reass_pbufcount;

/* Forward declarations. */
static void lowpan_reass_free_complete_datagram(struct lowpan_reassdata *lowpanr);
#if LOWPAN_REASS_FREE_OLDEST
static void lowpan_reass_remove_oldest_datagram(struct lowpan_reassdata *lowpanr, int pbufs_needed);
#endif /* LOWPAN_REASS_FREE_OLDEST */

/**
 * Timer callback function that calls lowpan_reass_tmr() and reschedules itself.
 *
 * @param arg unused argument
 */
static void
lowpan_reass_timer (void *arg)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_DEBUGF(TIMERS_DEBUG, ("radio: lowpan_reass_tmr()\n"));
  lowpan_reass_tmr();
  sys_timeout(LOWPAN_TMR_INTERVAL, lowpan_reass_timer, NULL);
}

/**
 * Reassembly timer init function
 * for both NO_SYS == 0 and 1 (!).
 *
 * Should be called at system start.
 */
void
lowpan_reass_init (void)
{
#if LWIP_TCPIP_TIMEOUT
  /* May be not in tcpip thread, so we use tcpip_timeout() for safety */
  tcpip_timeout(LOWPAN_TMR_INTERVAL, lowpan_reass_timer, NULL);
#else
  sys_timeout(LOWPAN_TMR_INTERVAL, lowpan_reass_timer, NULL);
#endif
}

void
lowpan_reass_tmr (void)
{
  struct lowpan_reassdata *r, *tmp;

  r = reassdatagrams;
  while (r != NULL) {
    /* Decrement the timer. Once it reaches 0,
     * clean up the incomplete fragment assembly */
    if (r->timer > 0) {
      r->timer--;
      r = r->next;
    } else {
      /* reassembly timed out */
      tmp = r;
      /* get the next pointer before freeing */
      r = r->next;
      /* free the helper struct and all enqueued pbufs */
      lowpan_reass_free_complete_datagram(tmp);
     }
   }
}

/**
 * Free a datagram (struct lowpan_reassdata) and all its pbufs.
 * Updates the total count of enqueued pbufs (lowpan_reass_pbufcount),
 * sends an ICMP time exceeded packet.
 *
 * @param lowpanr datagram to free
 */
static void
lowpan_reass_free_complete_datagram (struct lowpan_reassdata *lowpanr)
{
  struct lowpan_reassdata *prev;
  u16_t pbufs_freed = 0;
  u8_t clen;
  struct pbuf *p;
  struct lowpan_reass_helper *lowpanrh;

  /* First, free all received pbufs.  The individual pbufs need to be released
     separately as they have not yet been chained */
  p = lowpanr->p;
  while (p != NULL) {
    struct pbuf *pcur;
    lowpanrh = (struct lowpan_reass_helper *)p->payload;
    pcur = p;
    /* get the next pointer before freeing */
    p = lowpanrh->next_pbuf;
    clen = pbuf_clen(pcur);
    LWIP_ASSERT("pbufs_freed + clen <= 0xffff", pbufs_freed + clen <= 0xffff);
    pbufs_freed += clen;
    pbuf_free(pcur);
  }

  /* Then, unchain the struct lowpan_reassdata from the list and free it. */
  if (lowpanr == reassdatagrams) {
    reassdatagrams = lowpanr->next;
  } else {
    prev = reassdatagrams;
    while (prev != NULL) {
      if (prev->next == lowpanr) {
        break;
      }
      prev = prev->next;
    }
    if (prev != NULL) {
      prev->next = lowpanr->next;
    }
  }
  mem_free(lowpanr);

  /* Finally, update number of pbufs in reassembly queue */
  LWIP_ASSERT("ip_reass_pbufcount >= clen", lowpan_reass_pbufcount >= pbufs_freed);
  lowpan_reass_pbufcount -= pbufs_freed;
}

#if LOWPAN_REASS_FREE_OLDEST
/**
 * Free the oldest datagram to make room for enqueueing new fragments.
 * The datagram lowpanr is not freed!
 *
 * @param lowpanr lowpan_reassdata for the current fragment
 * @param pbufs_needed number of pbufs needed to enqueue
 *        (used for freeing other datagrams if not enough space)
 */
static void
lowpan_reass_remove_oldest_datagram (struct lowpan_reassdata *lowpanr, int pbufs_needed)
{
  struct lowpan_reassdata *r, *oldest;

  /* Free datagrams until being allowed to enqueue 'pbufs_needed' pbufs,
   * but don't free the current datagram! */
  do {
    r = oldest = reassdatagrams;
    while (r != NULL) {
      if (r != lowpanr) {
        if (r->timer <= oldest->timer) {
          /* older than the previous oldest */
          oldest = r;
        }
      }
      r = r->next;
    }
    if (oldest != NULL) {
      lowpan_reass_free_complete_datagram(oldest);
    }
  } while (((lowpan_reass_pbufcount + pbufs_needed) > LOWPAN_REASS_MAX_PBUFS) && (reassdatagrams != NULL));
}
#endif /* LOWPAN_REASS_FREE_OLDEST */

/**
 * Reassembles incoming lowpan fragments into a lowpan frame.
 *
 * @param p points to the lowpan frame,include the mac header.
 * @param ethhdr The ethernet header. This header is converted by 
 *        ieee 802.15.4 header.
 * @return NULL if reassembly is incomplete, otherwise pbuf pointing to
 *         fully lowpan frame if reassembly is complete
 */
struct pbuf *
lowpan_reass (struct pbuf *p, struct eth_hdr *ethhdr)
{
  u8_t *frag_hdr;
  struct lowpan_reassdata *lowpanr, *lowpanr_prev;
  struct lowpan_reass_helper *lowpanrh, *lowpanrh_tmp, *lowpanrh_prev=NULL;
  u16_t tot_len, offset, len;
  u8_t tmp;
  u8_t clen, valid = 1;
  struct pbuf *q;

  frag_hdr = (u8_t *) p->payload;

  clen = pbuf_clen(p);
  
  /* The totle length of this frame. */
  tot_len = (((((u16_t)frag_hdr[0]) << 8) & 0x0700)) | ((u16_t)frag_hdr[1]);
  
  /* Extract length and fragment offset from current fragment */
  /* If this is the first fragments. */
  if ((frag_hdr[0] & LOWPAN_DISPATCH_MASK) == LOWPAN_DISPATCH_FRAG1) {
    if (pbuf_header(p,  -(s16_t)LOWPAN_FRAG1_HEAD_SIZE)) {
      goto nullreturn;
    }
    len = p->tot_len;
    offset = 0;
  
  } else { /* Otherwise, this is the middle or last fragment. */
    if (pbuf_header(p,  -(s16_t)LOWPAN_FRAGN_HEAD_SIZE)) {
      goto nullreturn;
    }
    len = p->tot_len;
    offset = (((u16_t)frag_hdr[4]) << LOWPAN_FRAG_OFFSET_SHIFT); 
  }

  /* Look for the datagram the fragment belongs to in the current datagram queue,
   * remembering the previous in the queue for later dequeueing. */
  for (lowpanr = reassdatagrams, lowpanr_prev = NULL; lowpanr != NULL; lowpanr = lowpanr->next) {
    /* Check if the incoming fragment matches the one currently present
       in the reassembly buffer. If so, we proceed with copying the
       fragment into the buffer. */
    if (LOWPAN_ADDRESSES_AND_ID_MATCH(lowpanr, ethhdr, frag_hdr)) {
      LWIP_DEBUGF(LOWPAN_REASS_DEBUG, ("lowpan_reass: matching previous fragment ID=%"X16_F"\n",
                  LOWPAN_TAG_GET(frag_hdr)));
      break;
    }
    lowpanr_prev = lowpanr;
  }

  if (lowpanr == NULL) {
  /* Enqueue a new datagram into the datagram queue */
    lowpanr = (struct lowpan_reassdata *)mem_malloc(sizeof(struct lowpan_reassdata));
    if (lowpanr == NULL) {
#if LOWPAN_REASS_FREE_OLDEST
      /* Make room and try again. */
      lowpan_reass_remove_oldest_datagram(lowpanr, clen);
      lowpanr = (struct lowpan_reassdata *)mem_malloc(sizeof(struct lowpan_reassdata));
      if (lowpanr == NULL)
#endif /* LOWPAN_REASS_FREE_OLDEST */
      {
        goto nullreturn;
      }
    }

    memset(lowpanr, 0, sizeof(struct lowpan_reassdata));
    lowpanr->timer = LOWPAN_REASS_MAXAGE;

    /* enqueue the new structure to the front of the list */
    lowpanr->next = reassdatagrams;
    reassdatagrams = lowpanr;
    
    /* Use the current ethernet header for reference.
     * Eventually, we will replace it when we get the first fragment. */
    lowpanr->ethhdr = *ethhdr;
    
    /* copy the fragmented packet tag. */
    lowpanr->datagram_tag = LOWPAN_TAG_GET(frag_hdr);
  }

  /* Check if we are allowed to enqueue more datagrams. */
  if ((lowpan_reass_pbufcount + clen) > LOWPAN_REASS_MAX_PBUFS) {
#if LOWPAN_REASS_FREE_OLDEST
    lowpan_reass_remove_oldest_datagram(lowpanr, clen);
    if ((lowpan_reass_pbufcount + clen) > LOWPAN_REASS_MAX_PBUFS)
#endif /* LOWPAN_REASS_FREE_OLDEST */
    {
      /* @todo: send time exceeded here? */
      /* drop this pbuf */
      goto nullreturn;
    }
  }

  /* overwrite the fragment's lowpan header from the pbuf with our helper struct,
   * and setup the embedded helper structure. */
  /* make sure the struct lowpan_reass_helper fits into the lowpan header */
  if (pbuf_header(p,  (s16_t)sizeof(struct lowpan_reass_helper))) {
     goto nullreturn;
  }
  
  lowpanrh = (struct lowpan_reass_helper *)p->payload;
  lowpanrh->next_pbuf = NULL;
  lowpanrh->start = offset;
  lowpanrh->end = offset + len;

  /* find the right place to insert this pbuf */
  /* Iterate through until we either get to the end of the list (append),
   * or we find on with a larger offset (insert). */
  for (q = lowpanr->p; q != NULL;) {
    lowpanrh_tmp = (struct lowpan_reass_helper*)q->payload;
    if (lowpanrh->start < lowpanrh_tmp->start) {
#if LOWPAN_REASS_CHECK_OVERLAP
      if (lowpanrh->end > lowpanrh_tmp->start) {
        /* fragment overlaps with following, throw away */
        goto nullreturn;
      }
      if (lowpanrh_prev != NULL) {
        if (lowpanrh->start < lowpanrh_prev->end) {
          /* fragment overlaps with previous, throw away */
          goto nullreturn;
        }
      }
#endif /* LOWPAN_REASS_CHECK_OVERLAP */
      /* the new pbuf should be inserted before this */
      lowpanrh->next_pbuf = q;
      if (lowpanrh_prev != NULL) {
        /* not the fragment with the lowest offset */
        lowpanrh_prev->next_pbuf = p;
      } else {
        /* fragment with the lowest offset */
        lowpanr->p = p;
      }
      break;
    } else if (lowpanrh->start == lowpanrh_tmp->start) {
      /* received the same datagram twice: no need to keep the datagram */
      goto nullreturn;
#if LOWPAN_REASS_CHECK_OVERLAP
    } else if (lowpanrh->start < lowpanrh_tmp->end) {
      /* overlap: no need to keep the new datagram */
      goto nullreturn;
#endif /* LOWPAN_REASS_CHECK_OVERLAP */
    } else {
      /* Check if the fragments received so far have no gaps. */
      if (lowpanrh_prev != NULL) {
        if (lowpanrh_prev->end != lowpanrh_tmp->start) {
          /* There is a fragment missing between the current
           * and the previous fragment */
          valid = 0;
        }
      }
    }
    q = lowpanrh_tmp->next_pbuf;
    lowpanrh_prev = lowpanrh_tmp;
  }

  /* If q is NULL, then we made it to the end of the list. Determine what to do now */
  if (q == NULL) {
    if (lowpanrh_prev != NULL) {
      /* this is (for now), the fragment with the highest offset:
       * chain it to the last fragment */
#if LOWPAN_REASS_CHECK_OVERLAP
      LWIP_ASSERT("check fragments don't overlap", lowpanrh_prev->end <= lowpanrh->start);
#endif /* LOWPAN_REASS_CHECK_OVERLAP */
      lowpanrh_prev->next_pbuf = p;
      if (lowpanrh_prev->end != lowpanrh->start) {
        valid = 0;
      }
    } else {
#if LOWPAN_REASS_CHECK_OVERLAP
      LWIP_ASSERT("no previous fragment, this must be the first fragment!",
        lowpanr->p == NULL);
#endif /* LOWPAN_REASS_CHECK_OVERLAP */
      /* this is the first fragment we ever received for this ip datagram */
      lowpanr->p = p;
    }
  }

  /* Track the current number of pbufs current 'in-flight', in order to limit
  the number of fragments that may be enqueued at any one time */
  lowpan_reass_pbufcount += clen;

  /* If this is the last fragment, calculate total packet length. */
  lowpanr->datagram_len += len;

  /* Additional validity tests: we have received first and last fragment. */
  lowpanrh_tmp = (struct lowpan_reass_helper*)lowpanr->p->payload;
  if (lowpanrh_tmp->start != 0) {
    valid = 0;
  }
  if ((lowpanr->datagram_len == 0) || (lowpanr->datagram_len < tot_len)) {
    valid = 0;
  }

  /* Final validity test: no gaps between current and last fragment. */
  lowpanrh_prev = lowpanrh;
  q = lowpanrh->next_pbuf;
  while ((q != NULL) && valid) {
    lowpanrh = (struct lowpan_reass_helper*)q->payload;
    if (lowpanrh_prev->end != lowpanrh->start) {
      valid = 0;
      break;
    }
    lowpanrh_prev = lowpanrh;
    q = lowpanrh->next_pbuf;
  }

  if (valid) {
    /* All fragments have been received */

    /* chain together the pbufs contained within the lowpan_reassdata list. */
    lowpanrh = (struct lowpan_reass_helper*) lowpanr->p->payload;
    while (lowpanrh != NULL) {

      if (lowpanrh->next_pbuf != NULL) {
        /* Save next helper struct (will be hidden in next step). */
        lowpanrh_tmp = (struct lowpan_reass_helper*) lowpanrh->next_pbuf->payload;

        /* hide the fragment header for every succeding fragment */
        pbuf_header(lowpanrh->next_pbuf, -(s16_t)sizeof(struct lowpan_reass_helper));
        pbuf_cat(lowpanr->p, lowpanrh->next_pbuf);
      
      } else {
        lowpanrh_tmp = NULL;
      }

      lowpanrh = lowpanrh_tmp;
    }
    /* Hide the first fragment header. */
    pbuf_header(lowpanr->p, -(s16_t)sizeof(struct lowpan_reass_helper));

    /* Get the first pbuf. */
    p = lowpanr->p;
    
    /* The ethernet type must modify here. */
    tmp = *((u8_t *)p->payload);
    if (tmp & 0xc0) {
        ethhdr->type = PP_HTONS(ETHTYPE_IPV6);
  
    } else {
        if(LOWPAN_DISPATCH_IPV4 == tmp) { /* If he packet is IPv4*/
          ethhdr->type = PP_HTONS(ETHTYPE_IP);
          
        } else if (LOWPAN_DISPATCH_ARP == tmp) { /* If the Packet is arp*/
          ethhdr->type = PP_HTONS(ETHTYPE_ARP);
        
        } else {
          ethhdr->type = PP_HTONS(ETHTYPE_IP); /* ? */
        }
    }

    /* release the sources allocate for the fragment queue entry */
    if (reassdatagrams == lowpanr) {
      /* it was the first in the list */
      reassdatagrams = lowpanr->next;
    } else {
      /* it wasn't the first, so it must have a valid 'prev' */
      LWIP_ASSERT("sanity check linked list", lowpanr_prev != NULL);
      lowpanr_prev->next = lowpanr->next;
    }
    mem_free(lowpanr);

    /* adjust the number of pbufs currently queued for reassembly. */
    lowpan_reass_pbufcount -= pbuf_clen(p);

    /* Return the pbuf chain */
    return p;
  }
  /* the datagram is not (yet?) reassembled completely */
  return NULL;

nullreturn:
  pbuf_free(p);
  return NULL;
}

#if LOWPAN_FRAG_USES_STATIC_BUF
static u8_t buf[LWIP_MEM_ALIGN_SIZE(LOWPAN_FRAG_MAX_MTU + MEM_ALIGNMENT - 1)];
#else /* LOWPAN_FRAG_USES_STATIC_BUF */
/** Allocate a new struct pbuf_custom_ref */
static struct pbuf_custom_ref*
lowpan_frag_alloc_pbuf_custom_ref (void)
{
  return (struct pbuf_custom_ref*)mem_malloc(sizeof(struct pbuf_custom_ref));
  
}

/** Free a struct pbuf_custom_ref */
static void
lowpan_frag_free_pbuf_custom_ref (struct pbuf_custom_ref* p)
{
  LWIP_ASSERT("p != NULL", p != NULL);
  mem_free(p);
}

/** Free-callback function to free a 'struct pbuf_custom_ref', called by
 * pbuf_free. */
static void
lowpan_frag_free_pbuf_custom (struct pbuf *p)
{
  struct pbuf_custom_ref *pcr = (struct pbuf_custom_ref*)p;
  LWIP_ASSERT("pcr != NULL", pcr != NULL);
  LWIP_ASSERT("pcr == p", (void*)pcr == (void*)p);
  if (pcr->original != NULL) {
    pbuf_free(pcr->original);
  }
  lowpan_frag_free_pbuf_custom_ref(pcr);
}
#endif  /* LOWPAN_FRAG_USES_STATIC_BUF */
/**
 * Fragment a lowpan frame if too large for the lowpan_if->mfl.
 *
 * Chop the datagram in MTU sized chunks and send them in order
 * by pointing PBUF_REFs into p
 *
 * @param lowpan_if The interface of the lowpan. 
 * @param p  The lowpan frame to send
 *
 * @return ERR_OK if sent successfully, err_t otherwise
 */
err_t
lowpan_frag (struct lowpanif *lowpanif, struct pbuf *p)
{
#if LOWPAN_FRAG_USES_STATIC_BUF
  u8_t nfb;
  u8_t head[5];
  u16_t header_length, payload_length, offset;
  u16_t poff;
  static u16_t lowpan_fragment_tag;
  struct pbuf *rambuf;

  u8_t *dataptr;
  u8_t *frame_offset;

  lowpan_fragment_tag++;
  header_length = ieee802154_hdr_len((u8_t *)p->payload);
  payload_length = p->tot_len - header_length;

  /* first fragment header */
  head[0] = LOWPAN_DISPATCH_FRAG1 | ((payload_length >> 8) & 0x7);
  head[1] = payload_length & 0xff;
  head[2] = lowpan_fragment_tag >> 8;
  head[3] = lowpan_fragment_tag & 0xff;
  nfb = (lowpanif->mfl - header_length - LOWPAN_FRAG1_HEAD_SIZE) & LOWPAN_FRAG_OFFSET_MASK;
  
  poff = header_length;
  
  /* When using a static buffer, we use a PBUF_REF, which we will
   * use to reference the packet (without link header).
   * Layer and length is irrelevant.
   */
  rambuf = pbuf_alloc(PBUF_RAW, 0, PBUF_REF);
  if (rambuf == NULL) {
    LWIP_DEBUGF(LOWPAN_REASS_DEBUG, ("lowpan_frag: pbuf_alloc(PBUF_LINK, 0, PBUF_REF) failed!\n"));
    return ERR_MEM;
  }
  rambuf->tot_len = rambuf->len = (nfb + header_length + LOWPAN_FRAG1_HEAD_SIZE); /* first packet length */
  rambuf->payload = LWIP_MEM_ALIGN((void *)buf);

  /* Copy the lowpan header in it */
  dataptr = (u8_t *)rambuf->payload;
  SMEMCPY(dataptr, p->payload, header_length);
  dataptr += header_length;
  
  /* Copy as the frag1 */
  SMEMCPY(dataptr, head, LOWPAN_FRAG1_HEAD_SIZE);
  dataptr += LOWPAN_FRAG1_HEAD_SIZE;
  
  LWIP_DEBUGF(LOWPAN_REASS_DEBUG, ("lowpan_frag: The lowpan fragment offset is %d\n", 0));
  poff += pbuf_copy_partial(p, dataptr, nfb, poff);
  ieee802154_seqno_set(rambuf, lowpanif->seqno);
  lowpanif->seqno++;
  
  MAC_DRIVER(lowpanif)->send(lowpanif, rambuf);
  
  /* next fragment header */
  offset   =  nfb;
  head[0] &= ~LOWPAN_DISPATCH_FRAG1;
  head[0] |=  LOWPAN_DISPATCH_FRAGN;
  head[4]  =  (offset >> LOWPAN_FRAG_OFFSET_SHIFT);
  nfb = (lowpanif->mfl - header_length - LOWPAN_FRAGN_HEAD_SIZE) & LOWPAN_FRAG_OFFSET_MASK;
  
  frame_offset = (u8_t *)rambuf->payload + header_length + LOWPAN_FRAGN_HEAD_SIZE - 1;  /* point to offset in send buffer */
  
  dataptr = (u8_t *)rambuf->payload + header_length;
  SMEMCPY(dataptr, head, LOWPAN_FRAGN_HEAD_SIZE);
  dataptr += LOWPAN_FRAGN_HEAD_SIZE;
  
  while (offset < payload_length) {
    if (nfb > (payload_length - offset)) {
      nfb = payload_length - offset;
    }
    
    rambuf->tot_len = rambuf->len = (nfb + header_length + LOWPAN_FRAGN_HEAD_SIZE); /* the following packet length */
    
    LWIP_DEBUGF(LOWPAN_REASS_DEBUG, ("lowpan_frag: The lowpan fragment offset is %d\n", offset));
    poff += pbuf_copy_partial(p, dataptr, nfb, poff);
    ieee802154_seqno_set(rambuf, lowpanif->seqno);
    lowpanif->seqno++;
    
    MAC_DRIVER(lowpanif)->send(lowpanif, rambuf);
    
    offset += nfb;
    *frame_offset = (offset >> LOWPAN_FRAG_OFFSET_SHIFT);
  }
  
  pbuf_free(rambuf);
     
  return ERR_OK;

#else  /* LOWPAN_FRAG_USES_STATIC_BUF */
  u8_t *original_machdr;
  struct pbuf *rambuf;
  struct pbuf *newpbuf;
  static u16_t lowpan_fragment_tag;
  u8_t header_len = ieee802154_hdr_len((u8_t *)p->payload);
  u8_t *frag_hdr;
  u16_t nfb;
  u16_t left, cop;
  u16_t mtu;
  u16_t fragment_offset = 0;
  u16_t last;
  u16_t poff = header_len;
  u16_t tot_len;
  u16_t newpbuflen = 0;
  u16_t left_to_copy;

  lowpan_fragment_tag++;

  original_machdr = (u8_t *)p->payload;

  mtu = lowpanif->mfl;

  left = p->tot_len - header_len;
  tot_len = left;

  /* First, send the first fragment. */
  nfb = (mtu - header_len - LOWPAN_FRAG1_HEAD_SIZE) & LOWPAN_FRAG_OFFSET_MASK;
  cop = nfb;

  /* When not using a static buffer, create a chain of pbufs.
   * The first will be a PBUF_RAM holding the link, IPv6, and Fragment header.
   * The rest will be PBUF_REFs mirroring the pbuf chain to be fragged,
   * but limited to the size of an mtu.
   */
  rambuf = pbuf_alloc(PBUF_RAW, header_len + LOWPAN_FRAG1_HEAD_SIZE, PBUF_RAM);
  if (rambuf == NULL) {
    return ERR_MEM;
  }
  LWIP_ASSERT("this needs a pbuf in one piece!",
              (rambuf->len == rambuf->tot_len) && (rambuf->next == NULL));
  LWIP_ASSERT("this needs a pbuf in one piece!", (p->len > header_len));
  SMEMCPY(rambuf->payload, original_machdr, header_len);
  frag_hdr = (u8_t *)((u8_t*)rambuf->payload + header_len);

  /* Can just adjust p directly for needed offset. */
  p->payload = (u8_t *)p->payload + poff;
  p->len -= poff;
  p->tot_len -= poff;

  left_to_copy = cop;
  while (left_to_copy) {
    struct pbuf_custom_ref *pcr;
    newpbuflen = (left_to_copy < p->len) ? left_to_copy : p->len;
    /* Is this pbuf already empty? */
    if (!newpbuflen) {
        p = p->next;
        continue;
    }
    pcr = lowpan_frag_alloc_pbuf_custom_ref();
    if (pcr == NULL) {
        pbuf_free(rambuf);
        return ERR_MEM;
    }
    /* Mirror this pbuf, although we might not need all of it. */
    newpbuf = pbuf_alloced_custom(PBUF_RAW, newpbuflen, PBUF_REF, &pcr->pc, p->payload, newpbuflen);
    if (newpbuf == NULL) {
        lowpan_frag_free_pbuf_custom_ref(pcr);
        pbuf_free(rambuf);
        return ERR_MEM;
    }
    pbuf_ref(p);
    pcr->original = p;
    pcr->pc.custom_free_function = lowpan_frag_free_pbuf_custom;

    /* Add it to end of rambuf's chain, but using pbuf_cat, not pbuf_chain
     * so that it is removed when pbuf_dechain is later called on rambuf.
     */
    pbuf_cat(rambuf, newpbuf);
    left_to_copy -= newpbuflen;
    if (left_to_copy) {
        p = p->next;
    }
  }

  poff = newpbuflen;
  /* first fragment header */
  frag_hdr[0] = LOWPAN_DISPATCH_FRAG1 | ((tot_len >> 8) & 0x7);
  frag_hdr[1] = tot_len & 0xff;
  frag_hdr[2] = lowpan_fragment_tag >> 8;
  frag_hdr[3] = lowpan_fragment_tag & 0xff;

  /* No need for separate header pbuf - we allowed room for it in rambuf
   * when allocated.
   */
  LWIP_DEBUGF(LOWPAN_REASS_DEBUG, ("lowpan_frag: The lowpan fragment offset is %d\n", 0));
  ieee802154_seqno_set(rambuf, lowpanif->seqno);
  lowpanif->seqno++;
  
  MAC_DRIVER(lowpanif)->send(lowpanif, rambuf);

  /* Unfortunately we can't reuse rambuf - the hardware may still be
   * using the buffer. Instead we free it (and the ensuing chain) and
   * recreate it next time round the loop. If we're lucky the hardware
   * will have already sent the packet, the free will really free, and
   * there will be zero memory penalty.
   */
  pbuf_free(rambuf);
  left -= cop;
  fragment_offset += cop;

  /* Start to send the rest fragment. */
  nfb = (mtu - header_len - LOWPAN_FRAGN_HEAD_SIZE) & LOWPAN_FRAG_OFFSET_MASK;
  while (left) {
    last = (left <= nfb);

    /* Fill this fragment */
    cop = last ? left : nfb;

    /* When not using a static buffer, create a chain of pbufs.
     * The first will be a PBUF_RAM holding the link, IPv6, and Fragment header.
     * The rest will be PBUF_REFs mirroring the pbuf chain to be fragged,
     * but limited to the size of an mtu.
     */
    rambuf = pbuf_alloc(PBUF_RAW, header_len + LOWPAN_FRAGN_HEAD_SIZE, PBUF_RAM);
    if (rambuf == NULL) {
      return ERR_MEM;
    }
    LWIP_ASSERT("this needs a pbuf in one piece!",
              (rambuf->len == rambuf->tot_len) && (rambuf->next == NULL));
    LWIP_ASSERT("this needs a pbuf in one piece!", (p->len > header_len));
    SMEMCPY(rambuf->payload, original_machdr, header_len);
    frag_hdr = (u8_t *)((u8_t*)rambuf->payload + header_len);

    /* Can just adjust p directly for needed offset. */
    p->payload = (u8_t *)p->payload + poff;
    p->len -= poff;
    p->tot_len -= poff;

    left_to_copy = cop;
    while (left_to_copy) {
      struct pbuf_custom_ref *pcr;
      newpbuflen = (left_to_copy < p->len) ? left_to_copy : p->len;
      /* Is this pbuf already empty? */
      if (!newpbuflen) {
        p = p->next;
        continue;
      }
      pcr = lowpan_frag_alloc_pbuf_custom_ref();
      if (pcr == NULL) {
        pbuf_free(rambuf);
        return ERR_MEM;
      }
      /* Mirror this pbuf, although we might not need all of it. */
      newpbuf = pbuf_alloced_custom(PBUF_RAW, newpbuflen, PBUF_REF, &pcr->pc, p->payload, newpbuflen);
      if (newpbuf == NULL) {
        lowpan_frag_free_pbuf_custom_ref(pcr);
        pbuf_free(rambuf);
        return ERR_MEM;
      }
      pbuf_ref(p);
      pcr->original = p;
      pcr->pc.custom_free_function = lowpan_frag_free_pbuf_custom;

      /* Add it to end of rambuf's chain, but using pbuf_cat, not pbuf_chain
       * so that it is removed when pbuf_dechain is later called on rambuf.
       */
      pbuf_cat(rambuf, newpbuf);
      left_to_copy -= newpbuflen;
      if (left_to_copy) {
        p = p->next;
      }
    }
    poff = newpbuflen;

    /* Set the next fragment header */
    frag_hdr[0] = LOWPAN_DISPATCH_FRAGN | ((tot_len >> 8) & 0x7);
    frag_hdr[1] = tot_len & 0xff;
    frag_hdr[2] = lowpan_fragment_tag >> 8;
    frag_hdr[3] = lowpan_fragment_tag & 0xff;
    frag_hdr[4] = (fragment_offset >> LOWPAN_FRAG_OFFSET_SHIFT);

    /* No need for separate header pbuf - we allowed room for it in rambuf
     * when allocated.
     */
    LWIP_DEBUGF(LOWPAN_REASS_DEBUG, ("lowpan_frag: The lowpan fragment offset is %d\n", fragment_offset));
    ieee802154_seqno_set(rambuf, lowpanif->seqno);
    lowpanif->seqno++;
    
    MAC_DRIVER(lowpanif)->send(lowpanif, rambuf);

    /* Unfortunately we can't reuse rambuf - the hardware may still be
     * using the buffer. Instead we free it (and the ensuing chain) and
     * recreate it next time round the loop. If we're lucky the hardware
     * will have already sent the packet, the free will really free, and
     * there will be zero memory penalty.
     */

    pbuf_free(rambuf);
    left -= cop;
    fragment_offset += cop;
  }
  
  return ERR_OK;
#endif /* !LOWPAN_FRAG_USES_STATIC_BUF */
}

#endif /* LWIP_SUPPORT_CUSTOM_PBUF */
/* end */

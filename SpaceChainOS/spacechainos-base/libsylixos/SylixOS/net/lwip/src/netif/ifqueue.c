/**
 * @file
 * net interface pbuf queue.
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
 * 4. This code has been or is applying for intellectual property protection 
 *    and can only be used with acoinfo software products.
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

#define  __SYLIXOS_KERNEL
#include "lwip/opt.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/mem.h"
#include "lwip/tcpip.h"
#include "netif/ifqueue.h"

#include <stddef.h>

#define PKTN_ALIGN_SIZE     ROUND_UP(sizeof(struct pktn), sizeof(size_t))

/*
 * init a netif send queue
 */
void pktq_init (struct pktq *pktq, size_t max_size)
{
  if (pktq) {
    pktq->in = NULL;
    pktq->out = NULL;
    pktq->max_size = max_size;
    pktq->cur_size = 0;
  }
}

/*
 * put a packet into packet queue
 */
err_t pktq_put (struct pktq *pktq, struct pbuf *p, u8_t copy)
{
  SYS_ARCH_DECL_PROTECT(old_level);
  struct pktn *pktn;
  
  if (pktq->cur_size + p->tot_len > pktq->max_size) {
    return ERR_BUF;
  }
  
  pktn = (struct pktn *)mem_malloc(PKTN_ALIGN_SIZE);
  if (pktn == NULL) {
    return ERR_MEM;
  }
  
  if (copy) {
    pktn->p = pbuf_alloc(PBUF_RAW, p->tot_len, PBUF_POOL);
    if (pktn->p == NULL) {
      mem_free(pktn);
      return ERR_MEM;
    }
    pbuf_copy(pktn->p, p);
    
  } else {
    pktn->p = p;
  }
  
  SYS_ARCH_PROTECT(old_level);
  
  pktn->next = NULL;
  pktn->prev = pktq->in;
  
  if (pktq->in) {
    pktq->in->next = pktn;
  }
  pktq->in = pktn;

  if (pktq->out == NULL) {
    pktq->out = pktn;
  }
  
  pktq->cur_size += p->tot_len;
  
  SYS_ARCH_UNPROTECT(old_level);
  
  return ERR_OK;
}

/*
 * get a packet from packet queue
 */
struct pbuf *pktq_get (struct pktq *pktq, u8_t del)
{
  SYS_ARCH_DECL_PROTECT(old_level);
  struct pbuf *p = NULL;
  struct pktn *pktn;
  
  SYS_ARCH_PROTECT(old_level);
  
  pktn = pktq->out;
  if (pktn) {
    p = pktn->p;
    if (del) {
      pktq->out = pktn->next;
      if (pktq->out) {
        pktq->out->prev = NULL;
      } else {
        pktq->in = NULL;
      }
      pktq->cur_size -= pktn->p->tot_len;
    }
  }
  
  SYS_ARCH_UNPROTECT(old_level);
  
  if (pktn && del) {
    mem_free(pktn);
  }
  
  return p;
}

/*
 * packet queue node is empty
 */
u8_t pktq_isempty (struct pktq *pktq)
{
  return (u8_t)(pktq->out == NULL);
}

/*
 * end
 */

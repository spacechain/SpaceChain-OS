/**
 * @file
 * Lwip platform independent net route TCP MSS Adjust.
 */

/*
 * Copyright (c) 2006-2017 SylixOS Group.
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
 * Author: Han.hui <hanhui@acoinfo.com>
 *
 */

#define __SYLIXOS_KERNEL
#include "SylixOS.h"

#if LW_CFG_NET_ROUTER > 0

#include "net/if_iphook.h"
#include "lwip/ip.h"
#include "lwip/prot/tcp.h"
#include "lwip/inet_chksum.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"

#define TCP_OPT_EOL     0
#define TCP_OPT_NOP     1
#define TCP_OPT_MSS     2
#define TCP_OPT_MSS_LEN 4

/* tcp_forward_mss_adj (Only used for IPv4) */
int tcp_forward_mss_adj (int ip_type, int hook_type, struct pbuf *p, struct netif *inp, struct netif *outp)
{
  struct ip_hdr *iphdr;
  struct tcp_hdr *tcphdr;
  u8_t iphlen, tcphlen;
  u8_t *opt, *optend, optlen;
  u16_t mss, adj_mss;

  if ((ip_type != IP_HOOK_V4) || (hook_type != IP_HT_FORWARD)) {
    return (0);
  }

  if (inp->mtu == outp->mtu) {
    return (0); /* Do not need mss adjust */
  }

  iphdr = (struct ip_hdr *)p->payload;
  if (IPH_PROTO(iphdr) != IP_PROTO_TCP) {
    return (0); /* Not TCP packet */
  }

  iphlen = IPH_HL_BYTES(iphdr);
  if (p->len < (iphlen + TCP_HLEN)) {
    return (0); /* pbuf len not fixed */
  }

  tcphdr = (struct tcp_hdr *)((u8_t *)p->payload + iphlen);
  if (!(TCPH_FLAGS(tcphdr) & TCP_SYN)) {
    return (0); /* We only adjust SYN packet MSS opt */
  }

  tcphlen = TCPH_HDRLEN_BYTES(tcphdr);
  if (p->len < (iphlen + tcphlen)) {
    return (0); /* pbuf len not fixed */
  }

  optlen = tcphlen - TCP_HLEN;
  if (optlen) {
    opt = (u8_t *)tcphdr + TCP_HLEN;
    optend = (u8_t *)tcphdr + tcphlen;
    while (opt <= (optend - TCP_OPT_MSS_LEN)) {
      switch (*opt) {

      case TCP_OPT_EOL:
        return (0);

      case TCP_OPT_NOP:
        opt++;
        break;

      case TCP_OPT_MSS:
        if (*(opt + 1) == TCP_OPT_MSS_LEN) {
          mss = ((u16_t)(*(opt + 2)) << 8) + *(opt + 3);
          adj_mss = (inp->mtu < outp->mtu)
                  ? (inp->mtu - (IP_HLEN + TCP_HLEN))
                  : (outp->mtu - (IP_HLEN + TCP_HLEN));
          if (mss > adj_mss) { /* Do we need adjust MSS? */
            *(opt + 2) = (u8_t)(adj_mss >> 8);
            *(opt + 3) = (u8_t)(adj_mss & 0xff);
            mss = PP_HTONS(mss);
            inet_chksum_adjust((u8_t *)&tcphdr->chksum, (u8_t *)&mss, 2, opt + 2, 2); /* Fixed chksum */
          }
        }
        return (0);

      default:
        opt += *(opt + 1);
        break;
      }
    }
  }

  return (0);
}

#endif /* LW_CFG_NET_ROUTER */
/*
 * end
 */

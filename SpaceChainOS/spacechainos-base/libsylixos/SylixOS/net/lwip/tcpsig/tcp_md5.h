/**
 * @file
 * TCP MD5 signature.
 * as much as possible compatible with different versions of LwIP
 * Verification using sylixos(tm) real-time operating system
 */

/*
 * Copyright (c) 2006-2018 SylixOS Group.
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

#ifndef __TCP_SIG_H
#define __TCP_SIG_H

#include "lwip/opt.h"

#if LW_CFG_LWIP_TCP_SIG_EN > 0

#include "lwip/ip_addr.h"
#include "lwip/err.h"
#include "lwip/priv/sockets_priv.h"
#include "lwip/priv/tcp_priv.h"

void   tcp_md5_init(void);
err_t  tcp_md5_check_inpacket(struct tcp_pcb* pcb, struct tcp_hdr *hdr, u16_t optlen, u16_t opt1len, u8_t *opt2, struct pbuf *p);
u8_t   tcp_md5_get_additional_option_length(const struct tcp_pcb *pcb, u8_t internal_option_length);
u32_t *tcp_md5_add_tx_options(struct pbuf *p, struct tcp_hdr *hdr, const struct tcp_pcb *pcb, u32_t *opts);
int    tcp_md5_setsockopt(struct lwip_sock *sock, int optname, const void *optval);

#endif /* LW_CFG_LWIP_TCP_SIG_EN */
#endif /* __TCP_SIG_H */
/*
 * end
 */

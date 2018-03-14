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
 */

#ifndef ACOINFO_H_
#define ACOINFO_H_

/* Socket-level I/O control calls. */
#define FIOSETOWN       0x8901
#define SIOCSPGRP       0x8902
#define FIOGETOWN       0x8903
#define SIOCGPGRP       0x8904
#define SIOCGSTAMP      0x8906                  /* Get stamp                            */
#define SIOCGSTAMPNS    0x8907                  /* Get stamp (timespec)                 */

/* ARP protocol HARDWARE identifiers. */
#define ARPHRD_NETROM       0                   /* from KA9Q: NET/ROM pseudo            */
#define ARPHRD_ETHER        1                   /* Ethernet 10Mbps                      */
#define ARPHRD_EETHER       2                   /* Experimental Ethernet                */
#define ARPHRD_AX25         3                   /* AX.25 Level 2                        */
#define ARPHRD_PRONET       4                   /* PROnet token ring                    */
#define ARPHRD_CHAOS        5                   /* Chaosnet                             */
#define ARPHRD_IEEE802      6                   /* IEEE 802.2 Ethernet/TR/TB            */
#define ARPHRD_ARCNET       7                   /* ARCnet                               */
#define ARPHRD_APPLETLK     8                   /* APPLEtalk                            */
#define ARPHRD_DLCI         15                  /* Frame Relay DLCI                     */
#define ARPHRD_ATM          19                  /* ATM                                  */
#define ARPHRD_METRICOM     23                  /* Metricom STRIP (new IANA id)         */
#define ARPHRD_IEEE1394     24                  /* IEEE 1394 IPv4 - RFC 2734            */
#define ARPHRD_EUI64        27                  /* EUI-64                               */
#define ARPHRD_INFINIBAND   32                  /* InfiniBand                           */

/* Dummy types for non ARP hardware */
#define ARPHRD_SLIP         256
#define ARPHRD_CSLIP        257
#define ARPHRD_SLIP6        258
#define ARPHRD_CSLIP6       259
#define ARPHRD_RSRVD        260                 /* Notional KISS type                   */
#define ARPHRD_ADAPT        264
#define ARPHRD_ROSE         270
#define ARPHRD_X25          271                 /* CCITT X.25                           */
#define ARPHRD_HWX25        272                 /* Boards with X.25 in firmware         */
#define ARPHRD_CAN          280                 /* Controller Area Network              */
#define ARPHRD_PPP          512
#define ARPHRD_CISCO        513                 /* Cisco HDLC                           */
#define ARPHRD_HDLC         ARPHRD_CISCO
#define ARPHRD_LAPB         516                 /* LAPB                                 */
#define ARPHRD_DDCMP        517                 /* Digital's DDCMP protocol             */
#define ARPHRD_RAWHDLC      518                 /* Raw HDLC                             */

#define ARPHRD_TUNNEL       768                 /* IPIP tunnel                          */
#define ARPHRD_TUNNEL6      769                 /* IP6IP6 tunnel                        */
#define ARPHRD_FRAD         770                 /* Frame Relay Access Device            */
#define ARPHRD_SKIP         771                 /* SKIP vif                             */
#define ARPHRD_LOOPBACK     772                 /* Loopback device                      */
#define ARPHRD_LOCALTLK     773                 /* Localtalk device                     */
#define ARPHRD_FDDI         774                 /* Fiber Distributed Data Interface     */
#define ARPHRD_BIF          775                 /* AP1000 BIF                           */
#define ARPHRD_SIT          776                 /* sit0 device - IPv6-in-IPv4           */
#define ARPHRD_IPDDP        777                 /* IP over DDP tunneller                */
#define ARPHRD_IPGRE        778                 /* GRE over IP                          */
#define ARPHRD_PIMREG       779                 /* PIMSM register interface             */
#define ARPHRD_HIPPI        780                 /* High Performance Parallel Interface  */
#define ARPHRD_ASH          781                 /* Nexus 64Mbps Ash                     */
#define ARPHRD_ECONET       782                 /* Acorn Econet                         */
#define ARPHRD_IRDA         783                 /* Linux-IrDA                           */

/* ARP works differently on different FC media .. so  */
#define ARPHRD_FCPP         784                 /* Point to point fibrechannel          */
#define ARPHRD_FCAL         785                 /* Fibrechannel arbitrated loop         */
#define ARPHRD_FCPL         786                 /* Fibrechannel public loop             */
#define ARPHRD_FCFABRIC     787                 /* Fibrechannel fabric                  */

/* 787->799 reserved for fibrechannel media types */
#define ARPHRD_IEEE802_TR           800         /* Magic type ident for TR              */
#define ARPHRD_IEEE80211            801         /* IEEE 802.11                          */
#define ARPHRD_IEEE80211_PRISM      802         /* IEEE 802.11 + Prism2 header          */
#define ARPHRD_IEEE80211_RADIOTAP   803         /* IEEE 802.11 + radiotap header        */
#define ARPHRD_IEEE802154           804
#define ARPHRD_IEEE802154_MONITOR   805         /* IEEE 802.15.4 network monitor        */

#define ARPHRD_PHONET       820                 /* PhoNet media type                    */
#define ARPHRD_PHONET_PIPE  821                 /* PhoNet pipe header                   */
#define ARPHRD_CAIF         822                 /* CAIF media type                      */
#define ARPHRD_IP6GRE       823                 /* GRE over IPv6                        */
#define ARPHRD_NETLINK      824                 /* Netlink header                       */
#define ARPHRD_6LOWPAN      825                 /* IPv6 over LoWPAN                     */

#define ARPHRD_VOID         0xFFFF              /* Void type, nothing is known          */
#define ARPHRD_NONE         0xFFFE              /* zero header length                   */

/* ARP protocol opcodes. */
#define ARPOP_REQUEST       1                   /* ARP request                          */
#define ARPOP_REPLY         2                   /* ARP reply                            */
#define ARPOP_RREQUEST      3                   /* RARP request                         */
#define ARPOP_RREPLY        4                   /* RARP reply                           */
#define ARPOP_InREQUEST     8                   /* InARP request                        */
#define ARPOP_InREPLY       9                   /* InARP reply                          */
#define ARPOP_NAK           10                  /* (ATM)ARP NAK                         */

typedef unsigned short __u16;

struct netent *getnetbyname(const char *name, struct netent *np);

#endif /* ACOINFO_H_ */

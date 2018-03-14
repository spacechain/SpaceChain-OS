/**
 * @file
 * Socket API (to be used from non-TCPIP threads)
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
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
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */


#ifndef LWIP_HDR_SOCKETS_H
#define LWIP_HDR_SOCKETS_H

#include "lwip/opt.h"

#if LWIP_SOCKET /* don't build if not configured for use in lwipopts.h */

/* SylixOS Fixed do not include lwip/netif.h */
#include "lwip/ip_addr.h"
#include "lwip/err.h"
#include "lwip/inet.h"
#include "lwip/errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/* If your port already typedef's sa_family_t, define SA_FAMILY_T_DEFINED
   to prevent this code from redefining it. */
#if !defined(sa_family_t) && !defined(SA_FAMILY_T_DEFINED)
typedef u8_t sa_family_t;
#endif
/* If your port already typedef's in_port_t, define IN_PORT_T_DEFINED
   to prevent this code from redefining it. */
#if !defined(in_port_t) && !defined(IN_PORT_T_DEFINED)
typedef u16_t in_port_t;
#endif

#if LWIP_IPV4
/* members are in network byte order */
struct sockaddr_in {
  u8_t            sin_len;
  sa_family_t     sin_family;
  in_port_t       sin_port;
  struct in_addr  sin_addr;
#define SIN_ZERO_LEN 8
  char            sin_zero[SIN_ZERO_LEN];
};
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
struct sockaddr_in6 {
  u8_t            sin6_len;      /* length of this structure    */
  sa_family_t     sin6_family;   /* AF_INET6                    */
  in_port_t       sin6_port;     /* Transport layer port #      */
  u32_t           sin6_flowinfo; /* IPv6 flow information       */
  struct in6_addr sin6_addr;     /* IPv6 address                */
  u32_t           sin6_scope_id; /* Set of interfaces for scope */
};
#endif /* LWIP_IPV6 */

struct sockaddr {
  u8_t        sa_len;
  sa_family_t sa_family;
  char        sa_data[26]; /* SylixOS Changed to 26 (same size with in6) */
};

/* SylixOS Changed sockaddr_storage
   SylixOS have af_unix so s2_data[] must have 101 char space */
#ifdef SYLIXOS
#define _SS_MAXSIZE     128u            /* maximum size.        */
#define _SS_ALIGNSIZE   (sizeof(u64_t)) /* desired alignment.   */

#define _SS_PAD1SIZE    (_SS_ALIGNSIZE - sizeof(u8_t) - sizeof(sa_family_t))
#define _SS_PAD2SIZE    (_SS_MAXSIZE - sizeof(u8_t) - sizeof(sa_family_t) - \
                         _SS_PAD1SIZE - _SS_ALIGNSIZE)
                         
struct sockaddr_storage {
  u8_t          ss_len;
  sa_family_t   ss_family;
  char          _ss_pad1[_SS_PAD1SIZE];
  int64_t       _ss_align;
  char          _ss_pad2[_SS_PAD2SIZE];
};
#endif /* SYLIXOS */

/* If your port already typedef's socklen_t, define SOCKLEN_T_DEFINED
   to prevent this code from redefining it. */
#if !defined(socklen_t) && !defined(SOCKLEN_T_DEFINED) && !defined(__socklen_t_defined) && !defined(_SOCKLEN_T)
typedef u32_t socklen_t; /* SylixOS Add some standard defined check */
#define socklen_t u32_t
#define SOCKLEN_T_DEFINED 1
#define __socklen_t_defined 1
#define _SOCKLEN_T 1
#endif

/* SylixOS Removed IOV_MAX and struct iovec
   redefined with SylixOS */

struct msghdr {
  void         *msg_name;
  socklen_t     msg_namelen;
  struct iovec *msg_iov;
  int           msg_iovlen;
  void         *msg_control;
  socklen_t     msg_controllen;
  int           msg_flags;
};

/* struct msghdr->msg_flags bit field values */
#define MSG_TRUNC   0x200  /* SylixOS Changed for compatibility */
#define MSG_CTRUNC  0x1000

/* SylixOS Add RFC 2292 additions */
#ifndef ROUND_UP
#define ROUND_UP(x, align)  (size_t)(((size_t)(x) +  (align - 1)) & ~(align - 1))
#endif

/* RFC 3542, Section 20: Ancillary Data */
struct cmsghdr {
  socklen_t  cmsg_len;   /* number of bytes, including header */
  int        cmsg_level; /* originating protocol */
  int        cmsg_type;  /* protocol-specific type */
};
/* Data section follows header and possible padding, typically referred to as
      unsigned char cmsg_data[]; */

/* cmsg header/data alignment. NOTE: we align to native word size (double word
size on 16-bit arch) so structures are not placed at an unaligned address.
16-bit arch needs double word to ensure 32-bit alignment because socklen_t
could be 32 bits. If we ever have cmsg data with a 64-bit variable, alignment
will need to increase long long */
#define ALIGN_H(size) (((size) + sizeof(size_t) - 1U) & ~(sizeof(size_t) - 1U)) /* SylixOS Changed long to size_t */
#define ALIGN_D(size) ALIGN_H(size)

#define CMSG_ALIGN(nbytes) ALIGN_H(nbytes) /* SylixOS Add */

#define CMSG_FIRSTHDR(mhdr) \
          ((mhdr)->msg_controllen >= sizeof(struct cmsghdr) ? \
           (struct cmsghdr *)(mhdr)->msg_control : \
           (struct cmsghdr *)NULL)

#define CMSG_NXTHDR(mhdr, cmsg) \
        (((cmsg) == NULL) ? CMSG_FIRSTHDR(mhdr) : \
         (((u8_t *)(cmsg) + ALIGN_H((cmsg)->cmsg_len) \
                            + ALIGN_D(sizeof(struct cmsghdr)) > \
           (u8_t *)((mhdr)->msg_control) + (mhdr)->msg_controllen) ? \
          (struct cmsghdr *)NULL : \
          (struct cmsghdr *)((void*)((u8_t *)(cmsg) + \
                                      ALIGN_H((cmsg)->cmsg_len)))))

#define CMSG_DATA(cmsg) ((void*)((u8_t *)(cmsg) + \
                         ALIGN_D(sizeof(struct cmsghdr))))

#define CMSG_SPACE(length) (ALIGN_D(sizeof(struct cmsghdr)) + \
                            ALIGN_H(length))

#define CMSG_LEN(length) (ALIGN_D(sizeof(struct cmsghdr)) + \
                           length)

/* SylixOS Removed struct ifreq
   redefined net/if.h */

/* Socket protocol types (TCP/UDP/RAW) */
#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define SOCK_RAW        3
#define SOCK_SEQPACKET  5 /* SylixOS Add for AF_UNIX */

/*
 * Option flags per-socket. These must match the SOF_ flags in ip.h (checked in init.c)
 */
#define SO_REUSEADDR   0x0004 /* Allow local address reuse */
#define SO_KEEPALIVE   0x0008 /* keep connections alive */
#define SO_BROADCAST   0x0020 /* permit to send and to receive broadcast messages (see IP_SOF_BROADCAST option) */


/*
 * Additional options, not kept in so_options.
 */
#define SO_DEBUG        0x0001 /* Unimplemented: turn on debugging info recording */
#define SO_ACCEPTCONN   0x0002 /* socket has had listen() */
#define SO_DONTROUTE    0x0010 /* Unimplemented: just use interface addresses */
#define SO_USELOOPBACK  0x0040 /* Unimplemented: bypass hardware when possible */
#define SO_LINGER       0x0080 /* linger on close if data present */
#define SO_DONTLINGER   ((int)(~SO_LINGER))
#define SO_OOBINLINE    0x0100 /* Unimplemented: leave received OOB data in line */
#define SO_REUSEPORT    0x0200 /* Unimplemented: allow local address & port reuse */
#define SO_SNDBUF       0x1001 /* Unimplemented: send buffer size */
#define SO_RCVBUF       0x1002 /* receive buffer size */
#define SO_SNDLOWAT     0x1003 /* Unimplemented: send low-water mark */
#define SO_RCVLOWAT     0x1004 /* Unimplemented: receive low-water mark */
#define SO_SNDTIMEO     0x1005 /* send timeout */
#define SO_RCVTIMEO     0x1006 /* receive timeout */
#define SO_ERROR        0x1007 /* get error status and clear */
#define SO_TYPE         0x1008 /* get socket type */
#define SO_CONTIMEO     0x1009 /* Unimplemented: connect timeout */
#define SO_NO_CHECK     0x100a /* don't create UDP checksum */
#define SO_BINDTODEVICE 0x100b /* bind to device */

/*
 * Structure used for manipulating linger option.
 */
struct linger {
  int l_onoff;                /* option on/off */
  int l_linger;               /* linger time in seconds */
};

/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define  SOL_SOCKET  0xfff    /* options for socket level */
/*
 * SylixOS Add SOL_IP ... SOL_PACKET
 */
#define  SOL_IP      IPPROTO_IP
#define  SOL_TCP     IPPROTO_TCP
#define  SOL_UDP     IPPROTO_UDP
#define  SOL_IPV6    IPPROTO_IPV6
#define  SOL_ICMPV6  IPPROTO_ICMPV6
#define  SOL_UDPLITE IPPROTO_UDPLITE
#define  SOL_RAW     IPPROTO_RAW
#define  SOL_PACKET  263

#define AF_UNSPEC       0
#define AF_UNIX         1
#define AF_LOCAL        AF_UNIX /* SylixOS Add */
#define AF_INET         2
#if LWIP_IPV6
#define AF_INET6        10
#else /* LWIP_IPV6 */
#define AF_INET6        AF_UNSPEC
#endif /* LWIP_IPV6 */
#define AF_ROUTE        16 /* SylixOS Add */
#define AF_PACKET       17

#define PF_UNIX         AF_UNIX
#define PF_LOCAL        PF_UNIX
#define PF_INET         AF_INET
#define PF_INET6        AF_INET6
#define PF_ROUTE        AF_ROUTE
#define PF_PACKET       AF_PACKET
#define PF_UNSPEC       AF_UNSPEC

#define IPPROTO_IP      0
#define IPPROTO_ICMP    1
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17
#if LWIP_IPV6
#define IPPROTO_IPV6    41
#define IPPROTO_ICMPV6  58
#endif /* LWIP_IPV6 */
#define IPPROTO_UDPLITE 136
#define IPPROTO_RAW     255

/* Flags we can use with send and recv. */
#define MSG_PEEK       0x01    /* Peeks at an incoming message */
#define MSG_WAITALL    0x02    /* Unimplemented: Requests that the function block until the full amount of data requested can be returned */
#define MSG_OOB        0x04    /* Unimplemented: Requests out-of-band data. The significance and semantics of out-of-band data are protocol-specific */
#define MSG_DONTWAIT   0x08    /* Nonblocking i/o for this operation only */
#define MSG_MORE       0x10    /* Sender will send more */

/* SylixOS Changed for compatibility */
#define MSG_NOSIGNAL        0x4000
#define MSG_CMSG_CLOEXEC    0x40000

/*
 * Options for level IPPROTO_IP
 */
#define IP_TOS             1
#define IP_TTL             2
#define IP_PKTINFO         8

#if defined(SYLIXOS)
#define IP_MINTTL          99
#define IP_HDRINCL         100  /* SylixOS Add */
#define IP_OPTIONS         101  /* SylixOS Add Only for raw socket */
#endif /* SYLIXOS */

/* SylixOS Add support 'IPV6_RECVPKTINFO' 'IPV6_RECVHOPLIMIT' */
#if defined(SYLIXOS) && LWIP_IPV6
#ifndef IPV6_RECVPKTINFO
#define IPV6_RECVPKTINFO    49  /* Recv IPV6_PKTINFO message */
#define IPV6_PKTINFO        50
#define IPV6_RECVHOPLIMIT   51  /* Recv IPV6_HOPLIMIT message */
#define IPV6_HOPLIMIT       52
#define IPV6_MINHOPCNT      99
#endif /* !IPV6_RECVPKTINFO */
#endif /* LWIP_IPV6 */

#if LWIP_TCP
/*
 * Options for level IPPROTO_TCP
 */
#define TCP_NODELAY    0x01    /* don't delay send to coalesce packets */
#define TCP_KEEPALIVE  0x02    /* send KEEPALIVE probes when idle for pcb->keep_idle milliseconds */
#define TCP_KEEPIDLE   0x03    /* set pcb->keep_idle  - Same as TCP_KEEPALIVE, but use seconds for get/setsockopt */
#define TCP_KEEPINTVL  0x04    /* set pcb->keep_intvl - Use seconds for get/setsockopt */
#define TCP_KEEPCNT    0x05    /* set pcb->keep_cnt   - Use number of probes sent for get/setsockopt */

/* SylixOS Add TCP MD5 SIG Support */
#ifdef SYLIXOS
#if LW_CFG_LWIP_TCP_SIG_EN > 0
#define TCP_MD5SIG     0x0e    /* TCP MD5 Signatrue (14) */

#define TCP_MD5SIG_MAXKEYLEN    80

struct tcp_md5sig {
  struct  sockaddr_storage tcpm_addr;
  u16_t   __tcpm_pad1;
  u16_t   tcpm_keylen;
  u32_t   __tcpm_pad2;
  u8_t    tcpm_key[TCP_MD5SIG_MAXKEYLEN];
};
#endif /* LW_CFG_LWIP_TCP_SIG_EN */
#endif /* SYLIXOS */
#endif /* LWIP_TCP */

#if LWIP_IPV6
/*
 * Options for level IPPROTO_IPV6
 */
#define IPV6_CHECKSUM       7  /* RFC3542: calculate and insert the ICMPv6 checksum for raw sockets. */
#define IPV6_V6ONLY         27 /* RFC3493: boolean control to restrict AF_INET6 sockets to IPv6 communications only. */
#endif /* LWIP_IPV6 */

#if LWIP_UDP && LWIP_UDPLITE
/*
 * Options for level IPPROTO_UDPLITE
 */
#define UDPLITE_SEND_CSCOV 0x01 /* sender checksum coverage */
#define UDPLITE_RECV_CSCOV 0x02 /* minimal receiver checksum coverage */
#endif /* LWIP_UDP && LWIP_UDPLITE*/


#if LWIP_MULTICAST_TX_OPTIONS
/*
 * Options and types for UDP multicast traffic handling
 */
#define IP_MULTICAST_TTL   5
#define IP_MULTICAST_IF    6
#define IP_MULTICAST_LOOP  7
#endif /* LWIP_MULTICAST_TX_OPTIONS */

#if LWIP_IGMP
/*
 * Options and types related to multicast membership
 */
#define IP_ADD_MEMBERSHIP  3
#define IP_DROP_MEMBERSHIP 4

typedef struct ip_mreq {
  struct in_addr imr_multiaddr; /* IP multicast address of group */
  struct in_addr imr_interface; /* local IP address of interface */
} ip_mreq;

/* SylixOS Add multicast filter Support */
/* IPv4 Source Filter Multicast API [RFC3678] */
#define IP_UNBLOCK_SOURCE           37   /* unblock a source */
#define IP_BLOCK_SOURCE             38   /* block a source */
#define IP_ADD_SOURCE_MEMBERSHIP    39   /* join a source-specific group */
#define IP_DROP_SOURCE_MEMBERSHIP   40   /* drop a single source */
#define IP_MSFILTER                 41   /* set/get msfilter */
/*
 * Argument structure for IPv4 Multicast Source Filter APIs. [RFC3678]
 */
struct ip_mreq_source {
  struct in_addr imr_multiaddr;   /* IP multicast address of group */
  struct in_addr imr_sourceaddr;  /* IP address of source */
  struct in_addr imr_interface;   /* local IP address of interface */
};

#define IP_MSFILTER_SIZE(numsrc) \
  (sizeof(struct ip_msfilter) - sizeof(struct in_addr) \
   + (numsrc) * sizeof(struct in_addr))

struct ip_msfilter {
  struct in_addr imsf_multiaddr;
  struct in_addr imsf_interface;
  u32_t          imsf_fmode;
#define MCAST_EXCLUDE   0
#define MCAST_INCLUDE   1
  u32_t          imsf_numsrc;
  struct in_addr imsf_slist[1];
};
#endif /* LWIP_IGMP */

#if (LWIP_IPV4 && LWIP_IGMP) || (LWIP_IPV6 && LWIP_IPV6_MLD)
/* Protocol Independent Multicast API [RFC3678] */
#define MCAST_JOIN_GROUP            42   /* join an any-source group */
#define MCAST_BLOCK_SOURCE          43   /* block a source */
#define MCAST_UNBLOCK_SOURCE        44   /* unblock a source */
#define MCAST_LEAVE_GROUP           45   /* leave all sources for group */
#define MCAST_JOIN_SOURCE_GROUP     46   /* join a source-specific group */
#define MCAST_LEAVE_SOURCE_GROUP    47   /* leave a single source */
#define MCAST_MSFILTER              48

/*
 * Argument structures for Protocol-Independent Multicast Source
 * Filter APIs. [RFC3678]
 */
struct group_req {
  u32_t                     gr_interface;   /* interface index */
  struct sockaddr_storage   gr_group;       /* group address */
};

struct group_source_req {
  u32_t                     gsr_interface;  /* interface index */
  struct sockaddr_storage   gsr_group;      /* group address */
  struct sockaddr_storage   gsr_source;     /* source address */
};

#define GROUP_FILTER_SIZE(numsrc) \
    (sizeof(struct group_filter) - sizeof(struct sockaddr_storage) \
     + (numsrc) * sizeof(struct sockaddr_storage))

struct group_filter {
    u32_t                   gf_interface;   /* interface index */
    struct sockaddr_storage gf_group;       /* multicast address */
    u32_t                   gf_fmode;       /* filter mode */
    u32_t                   gf_numsrc;      /* number of sources */
    struct sockaddr_storage gf_slist[1];    /* interface index */
};
#endif /* #if (LWIP_IPV4 && LWIP_IGMP) || (LWIP_IPV6 && LWIP_IPV6_MLD) */

#if LWIP_IPV4
struct in_pktinfo {
  unsigned int   ipi_ifindex;  /* Interface index */
  struct in_addr ipi_spec_dst; /* SylixOS Add Unimplemented! */
  struct in_addr ipi_addr;     /* Destination (from header) address */
};
#endif /* LWIP_IPV4 */

/* SylixOS Add in6_pktinfo support */
#if defined(SYLIXOS) && LWIP_IPV6
struct in6_pktinfo {
  struct in6_addr ipi6_addr;
  int             ipi6_ifindex;
};
#endif /* LWIP_IPV6 */

#if LWIP_IPV6_MLD
/* SylixOS Add for compiled only */
#if defined(SYLIXOS)
#ifndef IPV6_UNICAST_HOPS
#define IPV6_UNICAST_HOPS       16 /* IPv6 ttl */
#define IPV6_MULTICAST_IF       17 /* Set mcast ifindex, (The argument is a pointer to an interface index) */
#define IPV6_MULTICAST_HOPS     18 /* IPv6 mcast ttl */
#define IPV6_MULTICAST_LOOP     19 /* Mcast loop */
#endif /* !IPV6_UNICAST_HOPS */
#endif /* LWIP_IPV6_MLD */

/*
 * Options and types related to IPv6 multicast membership
 */
#define IPV6_JOIN_GROUP      12
#define IPV6_ADD_MEMBERSHIP  IPV6_JOIN_GROUP
#define IPV6_LEAVE_GROUP     13
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP

typedef struct ipv6_mreq {
  struct in6_addr ipv6mr_multiaddr; /*  IPv6 multicast addr */
  unsigned int    ipv6mr_interface; /*  interface index, or 0 */
} ipv6_mreq;
#endif /* LWIP_IPV6_MLD */

/*
 * The Type of Service provides an indication of the abstract
 * parameters of the quality of service desired.  These parameters are
 * to be used to guide the selection of the actual service parameters
 * when transmitting a datagram through a particular network.  Several
 * networks offer service precedence, which somehow treats high
 * precedence traffic as more important than other traffic (generally
 * by accepting only traffic above a certain precedence at time of high
 * load).  The major choice is a three way tradeoff between low-delay,
 * high-reliability, and high-throughput.
 * The use of the Delay, Throughput, and Reliability indications may
 * increase the cost (in some sense) of the service.  In many networks
 * better performance for one of these parameters is coupled with worse
 * performance on another.  Except for very unusual cases at most two
 * of these three indications should be set.
 */
#define IPTOS_TOS_MASK          0x1E
#define IPTOS_TOS(tos)          ((tos) & IPTOS_TOS_MASK)
#define IPTOS_LOWDELAY          0x10
#define IPTOS_THROUGHPUT        0x08
#define IPTOS_RELIABILITY       0x04
#define IPTOS_LOWCOST           0x02
#define IPTOS_MINCOST           IPTOS_LOWCOST

/*
 * The Network Control precedence designation is intended to be used
 * within a network only.  The actual use and control of that
 * designation is up to each network. The Internetwork Control
 * designation is intended for use by gateway control originators only.
 * If the actual use of these precedence designations is of concern to
 * a particular network, it is the responsibility of that network to
 * control the access to, and use of, those precedence designations.
 */
#define IPTOS_PREC_MASK                 0xe0
#define IPTOS_PREC(tos)                ((tos) & IPTOS_PREC_MASK)
#define IPTOS_PREC_NETCONTROL           0xe0
#define IPTOS_PREC_INTERNETCONTROL      0xc0
#define IPTOS_PREC_CRITIC_ECP           0xa0
#define IPTOS_PREC_FLASHOVERRIDE        0x80
#define IPTOS_PREC_FLASH                0x60
#define IPTOS_PREC_IMMEDIATE            0x40
#define IPTOS_PREC_PRIORITY             0x20
#define IPTOS_PREC_ROUTINE              0x00

/* SylixOS Removed FIONREAD, FIONBIO
   already defined in SylixOS */

/* Socket I/O Controls: unimplemented */
#ifndef SIOCSHIWAT
#define SIOCSHIWAT  _IOW('s',  0, unsigned long)  /* set high watermark */
#define SIOCGHIWAT  _IOR('s',  1, unsigned long)  /* get high watermark */
#define SIOCSLOWAT  _IOW('s',  2, unsigned long)  /* set low watermark */
#define SIOCGLOWAT  _IOR('s',  3, unsigned long)  /* get low watermark */
#define SIOCATMARK  _IOR('s',  7, unsigned long)  /* at oob mark? */
#endif

/* SylixOS Removed O_NONBLOCK, O_NDELAY, O_RDONLY, O_WRONLY, O_RDWR
   already defined in SylixOS */

#ifndef SHUT_RD
  #define SHUT_RD   0
  #define SHUT_WR   1
  #define SHUT_RDWR 2
#endif

/* SylixOS Removed FD_SET, FD_SETSIZE, FD_SET, FD_CLR, FD_ISSET, FD_ZERO,
   struct fd_set, 
   already defined in SylixOS and SylixOS will use it's own select() system */
   
/* SylixOS Removed POLLIN, POLLOUT, POLLERR, POLLRDNORM, POLLRDBAND...
   nfds_t, struct pollfd, 
   already defined in SylixOS and SylixOS will use it's own poll() system */

/** LWIP_TIMEVAL_PRIVATE: if you want to use the struct timeval provided
 * by your system, set this to 0 and include <sys/time.h> in cc.h */
#ifndef LWIP_TIMEVAL_PRIVATE
#define LWIP_TIMEVAL_PRIVATE 1
#endif

#if LWIP_TIMEVAL_PRIVATE
struct timeval {
  long    tv_sec;         /* seconds */
  long    tv_usec;        /* and microseconds */
};
#endif /* LWIP_TIMEVAL_PRIVATE */

#define lwip_socket_init() /* Compatibility define, no init needed. */
void lwip_socket_thread_init(void); /* LWIP_NETCONN_SEM_PER_THREAD==1: initialize thread-local semaphore */
void lwip_socket_thread_cleanup(void); /* LWIP_NETCONN_SEM_PER_THREAD==1: destroy thread-local semaphore */

#if LWIP_COMPAT_SOCKETS == 2
/* This helps code parsers/code completion by not having the COMPAT functions as defines */
#define lwip_accept       accept
#define lwip_bind         bind
#define lwip_shutdown     shutdown
#define lwip_getpeername  getpeername
#define lwip_getsockname  getsockname
#define lwip_setsockopt   setsockopt
#define lwip_getsockopt   getsockopt
#define lwip_close        closesocket
#define lwip_connect      connect
#define lwip_listen       listen
#define lwip_recv         recv
#define lwip_recvmsg      recvmsg
#define lwip_recvfrom     recvfrom
#define lwip_send         send
#define lwip_sendmsg      sendmsg
#define lwip_sendto       sendto
#define lwip_socket       socket
#if LWIP_SOCKET_SELECT
#define lwip_select       select
#endif
#if LWIP_SOCKET_POLL
#define lwip_poll         poll
#endif
#define lwip_ioctl        ioctlsocket
#define lwip_inet_ntop    inet_ntop
#define lwip_inet_pton    inet_pton

#if LWIP_POSIX_SOCKETS_IO_NAMES
#define lwip_read         read
#define lwip_readv        readv
#define lwip_write        write
#define lwip_writev       writev
#undef lwip_close
#define lwip_close        close
#define closesocket(s)    close(s)
int fcntl(int s, int cmd, ...);
#undef lwip_ioctl
#define lwip_ioctl        ioctl
#define ioctlsocket       ioctl
#endif /* LWIP_POSIX_SOCKETS_IO_NAMES */
#endif /* LWIP_COMPAT_SOCKETS == 2 */

int lwip_accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int lwip_bind(int s, const struct sockaddr *name, socklen_t namelen);
int lwip_shutdown(int s, int how);
int lwip_getpeername (int s, struct sockaddr *name, socklen_t *namelen);
int lwip_getsockname (int s, struct sockaddr *name, socklen_t *namelen);
int lwip_getsockopt (int s, int level, int optname, void *optval, socklen_t *optlen);
int lwip_setsockopt (int s, int level, int optname, const void *optval, socklen_t optlen);
 int lwip_close(int s);
int lwip_connect(int s, const struct sockaddr *name, socklen_t namelen);
int lwip_listen(int s, int backlog);
ssize_t lwip_recv(int s, void *mem, size_t len, int flags);
ssize_t lwip_read(int s, void *mem, size_t len);
ssize_t lwip_readv(int s, const struct iovec *iov, int iovcnt);
ssize_t lwip_recvfrom(int s, void *mem, size_t len, int flags,
      struct sockaddr *from, socklen_t *fromlen);
ssize_t lwip_recvmsg(int s, struct msghdr *message, int flags);
ssize_t lwip_send(int s, const void *dataptr, size_t size, int flags);
ssize_t lwip_sendmsg(int s, const struct msghdr *message, int flags);
ssize_t lwip_sendto(int s, const void *dataptr, size_t size, int flags,
    const struct sockaddr *to, socklen_t tolen);
int lwip_socket(int domain, int type, int protocol);
ssize_t lwip_write(int s, const void *dataptr, size_t size);
ssize_t lwip_writev(int s, const struct iovec *iov, int iovcnt);
#if LWIP_SOCKET_SELECT
int lwip_select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset,
                struct timeval *timeout);
#endif
#if LWIP_SOCKET_POLL
int lwip_poll(struct pollfd *fds, nfds_t nfds, int timeout);
#endif
int lwip_ioctl(int s, long cmd, void *argp);
int lwip_fcntl(int s, int cmd, int val);
const char *lwip_inet_ntop(int af, const void *src, char *dst, socklen_t size);
int lwip_inet_pton(int af, const char *src, void *dst);

#if LWIP_COMPAT_SOCKETS
#if LWIP_COMPAT_SOCKETS != 2
/** @ingroup socket */
#define accept(s,addr,addrlen)                    lwip_accept(s,addr,addrlen)
/** @ingroup socket */
#define bind(s,name,namelen)                      lwip_bind(s,name,namelen)
/** @ingroup socket */
#define shutdown(s,how)                           lwip_shutdown(s,how)
/** @ingroup socket */
#define getpeername(s,name,namelen)               lwip_getpeername(s,name,namelen)
/** @ingroup socket */
#define getsockname(s,name,namelen)               lwip_getsockname(s,name,namelen)
/** @ingroup socket */
#define setsockopt(s,level,optname,opval,optlen)  lwip_setsockopt(s,level,optname,opval,optlen)
/** @ingroup socket */
#define getsockopt(s,level,optname,opval,optlen)  lwip_getsockopt(s,level,optname,opval,optlen)
/** @ingroup socket */
#define closesocket(s)                            lwip_close(s)
/** @ingroup socket */
#define connect(s,name,namelen)                   lwip_connect(s,name,namelen)
/** @ingroup socket */
#define listen(s,backlog)                         lwip_listen(s,backlog)
/** @ingroup socket */
#define recv(s,mem,len,flags)                     lwip_recv(s,mem,len,flags)
/** @ingroup socket */
#define recvmsg(s,message,flags)                  lwip_recvmsg(s,message,flags)
/** @ingroup socket */
#define recvfrom(s,mem,len,flags,from,fromlen)    lwip_recvfrom(s,mem,len,flags,from,fromlen)
/** @ingroup socket */
#define send(s,dataptr,size,flags)                lwip_send(s,dataptr,size,flags)
/** @ingroup socket */
#define sendmsg(s,message,flags)                  lwip_sendmsg(s,message,flags)
/** @ingroup socket */
#define sendto(s,dataptr,size,flags,to,tolen)     lwip_sendto(s,dataptr,size,flags,to,tolen)
/** @ingroup socket */
#define socket(domain,type,protocol)              lwip_socket(domain,type,protocol)
#if LWIP_SOCKET_SELECT
/** @ingroup socket */
#define select(maxfdp1,readset,writeset,exceptset,timeout)     lwip_select(maxfdp1,readset,writeset,exceptset,timeout)
#endif
#if LWIP_SOCKET_POLL
/** @ingroup socket */
#define poll(fds,nfds,timeout)                    lwip_poll(fds,nfds,timeout)
#endif
/** @ingroup socket */
#define ioctlsocket(s,cmd,argp)                   lwip_ioctl(s,cmd,argp)
/** @ingroup socket */
#define inet_ntop(af,src,dst,size)                lwip_inet_ntop(af,src,dst,size)
/** @ingroup socket */
#define inet_pton(af,src,dst)                     lwip_inet_pton(af,src,dst)

#if LWIP_POSIX_SOCKETS_IO_NAMES
/** @ingroup socket */
#define read(s,mem,len)                           lwip_read(s,mem,len)
/** @ingroup socket */
#define readv(s,iov,iovcnt)                       lwip_readv(s,iov,iovcnt)
/** @ingroup socket */
#define write(s,dataptr,len)                      lwip_write(s,dataptr,len)
/** @ingroup socket */
#define writev(s,iov,iovcnt)                      lwip_writev(s,iov,iovcnt)
/** @ingroup socket */
#define close(s)                                  lwip_close(s)
/** @ingroup socket */
#define fcntl(s,cmd,val)                          lwip_fcntl(s,cmd,val)
/** @ingroup socket */
#define ioctl(s,cmd,argp)                         lwip_ioctl(s,cmd,argp)
#endif /* LWIP_POSIX_SOCKETS_IO_NAMES */
#endif /* LWIP_COMPAT_SOCKETS != 2 */

#endif /* LWIP_COMPAT_SOCKETS */

#ifdef __cplusplus
}
#endif

/*  SylixOS Add socket extern */
#ifndef  __LWIP_SOCKET_H
#include "../SylixOS/net/lwip/lwip_socket.h"
#endif  /*  __LWIP_SOCKET_H */

#endif /* LWIP_SOCKET */

#endif /* LWIP_HDR_SOCKETS_H */

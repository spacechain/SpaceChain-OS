/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: packet.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 03 月 21 日
**
** 描        述: posix netpacket/packet.h
*********************************************************************************************************/

#ifndef __NETPACKET_PACKET_H
#define __NETPACKET_PACKET_H

#include "sys/types.h"

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

struct sockaddr_ll {
    u_char      sll_len;                                                /* Total length of sockaddr     */
    u_char      sll_family;                                             /* AF_PACKET                    */
    u_short     sll_protocol;                                           /* Physical layer protocol      */
    int         sll_ifindex;                                            /* Interface number             */
    u_short     sll_hatype;                                             /* ARP hardware type            */
    u_char      sll_pkttype;                                            /* packet type                  */
    u_char      sll_halen;                                              /* Length of address            */
    u_char      sll_addr[8];                                            /* Physical layer address       */
};

/*********************************************************************************************************
  packet types
*********************************************************************************************************/

#define PACKET_HOST         0                                           /*  To us                       */
#define PACKET_BROADCAST    1                                           /*  To all                      */
#define PACKET_MULTICAST    2                                           /*  To group                    */
#define PACKET_OTHERHOST    3                                           /*  To someone else             */
#define PACKET_OUTGOING     4                                           /*  Originated by us            */

/*********************************************************************************************************
  packet membership
*********************************************************************************************************/

struct packet_mreq {
    int             mr_ifindex;                                         /* interface index              */
    u_short         mr_type;                                            /* action                       */
    u_short         mr_alen;                                            /* address length               */
    u_char          mr_address[8];                                      /* physical layer address       */
};

#define PACKET_MR_MULTICAST     0
#define PACKET_MR_PROMISC       1
#define PACKET_MR_ALLMULTI      2

/*********************************************************************************************************
  packet socket options
*********************************************************************************************************/

#define PACKET_ADD_MEMBERSHIP   1
#define PACKET_DROP_MEMBERSHIP  2

#define PACKET_RECV_OUTPUT      3
#define PACKET_RX_RING          5
#define PACKET_STATISTICS       6

#define PACKET_VERSION          10
#define PACKET_HDRLEN           11
#define PACKET_RESERVE          12

struct tpacket_stats {
    u_int       tp_packets;                                             /* Total packet count           */
    u_int       tp_drops;                                               /* Dropped packet count         */
};

/*********************************************************************************************************
  PACKET_RX_RING options
*********************************************************************************************************/

struct tpacket_req {
    u_int       tp_block_size;                                          /* Min size of contiguous block */
    u_int       tp_block_nr;                                            /* Number of blocks             */
    u_int       tp_frame_size;                                          /* Size of frame                */
    u_int       tp_frame_nr;                                            /* Total number of frames       */
};

struct tpacket_hdr {
    volatile u_long      tp_status;
    volatile u_int       tp_len;
    volatile u_int       tp_snaplen;
    volatile u_short     tp_mac;
    volatile u_short     tp_net;
    volatile u_int       tp_sec;
    volatile u_int       tp_usec;
};

#define TPACKET_ALIGNMENT       16
#define TPACKET_ALIGN(x)        (((x) + TPACKET_ALIGNMENT - 1) & ~(TPACKET_ALIGNMENT - 1))
#define TPACKET_HDRLEN          (TPACKET_ALIGN(sizeof(struct tpacket_hdr)) + sizeof(struct sockaddr_ll))

struct tpacket2_hdr {
    volatile u_int32_t   tp_status;
    volatile u_int32_t   tp_len;
    volatile u_int32_t   tp_snaplen;
    volatile u_int16_t   tp_mac;
    volatile u_int16_t   tp_net;
    volatile u_int32_t   tp_sec;
    volatile u_int32_t   tp_nsec;
    volatile u_int16_t   tp_vlan_tci;
    volatile u_int16_t   tp_vlan_tpid;
    volatile u_int16_t   tp_padding[4];
};

#define TPACKET2_HDRLEN         (TPACKET_ALIGN(sizeof(struct tpacket2_hdr)) + sizeof(struct sockaddr_ll))

enum tpacket_versions {
    TPACKET_V1,
    TPACKET_V2
};

#define TP_STATUS_KERNEL        0x0
#define TP_STATUS_USER          0x1
#define TP_STATUS_COPY          0x2
#define TP_STATUS_LOSING        0x4
#define TP_STATUS_CSUMNOTREADY  0x8

#endif                                                                  /*  LW_CFG_NET_EN               */
#endif                                                                  /*  __NETPACKET_PACKET_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/

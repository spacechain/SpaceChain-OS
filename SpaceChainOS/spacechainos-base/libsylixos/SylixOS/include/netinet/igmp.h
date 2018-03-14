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
** 文   件   名: igmp.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 08 月 23 日
**
** 描        述: include/netinet/igmp.
*********************************************************************************************************/

#ifndef __NETINET_IGMP_H
#define __NETINET_IGMP_H

#include <sys/cdefs.h>
#include <sys/types.h>
#include <netinet/in.h>

/*********************************************************************************************************
  IGMP header
*********************************************************************************************************/

struct igmp {
    u_char          igmp_type;                                      /* IGMP type                        */
    u_char          igmp_code;                                      /* routing code                     */
    u_short         igmp_cksum;                                     /* checksum                         */
    struct in_addr  igmp_group;                                     /* group address                    */
} __attribute__((__packed__));

#define IGMP_MINLEN     8

/*********************************************************************************************************
  IGMP v3 query format
*********************************************************************************************************/

struct igmpv3 {
    u_char          igmp_type;                                      /* version & type of IGMP message   */
    u_char          igmp_code;                                      /* subtype for routing msgs         */
    u_short         igmp_cksum;                                     /* IP-style checksum                */
    struct in_addr  igmp_group;                                     /* group address being reported     */
                                                                    /*  (zero for queries)              */
    u_char          igmp_misc;                                      /* reserved/suppress/robustness     */
    u_char          igmp_qqi;                                       /* querier's query interval         */
    u_short         igmp_numsrc;                                    /* number of sources                */
    /*
     * struct in_addr igmp_sources[1];
     */                                                             /* source addresses                 */
} __attribute__((__packed__));

#define IGMP_V3_QUERY_MINLEN    12

#define IGMP_EXP(x)             (((x) >> 4) & 0x07)
#define IGMP_MANT(x)            ((x) & 0x0f)
#define IGMP_QRESV(x)           (((x) >> 4) & 0x0f)
#define IGMP_SFLAG(x)           (((x) >> 3) & 0x01)
#define IGMP_QRV(x)             ((x) & 0x07)

/*********************************************************************************************************
  IGMP Group Rec
*********************************************************************************************************/

struct igmp_grouprec {
    u_char          ig_type;                                        /* record type                      */
    u_char          ig_datalen;                                     /* length of auxiliary data         */
    u_short         ig_numsrc;                                      /* number of sources                */
    struct in_addr  ig_group;                                       /* group address being reported     */
    /*
     * struct in_addr ig_sources[1];
     */                                                             /* source addresses                 */
} __attribute__((__packed__));

#define IGMP_GRPREC_HDRLEN      8

/*********************************************************************************************************
  IGMPv3 host membership report header.
*********************************************************************************************************/

struct igmp_report {
    u_char          ir_type;                                        /* IGMP_v3_HOST_MEMBERSHIP_REPORT   */
    u_char          ir_rsv1;                                        /* must be zero                     */
    u_short         ir_cksum;                                       /* checksum                         */
    u_short         ir_rsv2;                                        /* must be zero                     */
    u_short         ir_numgrps;                                     /* number of group records          */
    /*
     * struct igmp_grouprec ir_groups[1];
     */                                                             /* group records                    */
} __attribute__((__packed__));

#define IGMP_V3_REPORT_MINLEN           8
#define IGMP_V3_REPORT_MAXRECS          65535

/*********************************************************************************************************
  Message types, including version number.
*********************************************************************************************************/

#define IGMP_MEMBERSHIP_QUERY           0x11                        /* membership query                 */
#define IGMP_V1_MEMBERSHIP_REPORT       0x12                        /* Ver. 1 membership report         */
#define IGMP_V2_MEMBERSHIP_REPORT       0x16                        /* Ver. 2 membership report         */
#define IGMP_V2_LEAVE_GROUP             0x17                        /* Leave-group message              */

#define IGMP_DVMRP                      0x13                        /* DVMRP routing message            */
#define IGMP_PIM                        0x14                        /* PIM routing message              */
#define IGMP_TRACE                      0x15

#define IGMP_MTRACE_RESP                0x1e                        /* traceroute resp.(to sender)      */
#define IGMP_MTRACE                     0x1f                        /* mcast traceroute messages        */
#define IGMP_V3_HOST_MEMBERSHIP_REPORT  0x22                        /* Ver. 3 membership report */

#define IGMP_MAX_HOST_REPORT_DELAY      10                          /* max delay for response to        */
                                                                    /*  query (in seconds) according    */
                                                                    /*  to RFC1112                      */
#define IGMP_TIMER_SCALE                10                          /* denotes that the igmp code field */
                                                                    /* specifies time in 10th of seconds*/

/*********************************************************************************************************
  States for the IGMP v2 state table.
*********************************************************************************************************/

#define IGMP_DELAYING_MEMBER            1
#define IGMP_IDLE_MEMBER                2
#define IGMP_LAZY_MEMBER                3
#define IGMP_SLEEPING_MEMBER            4
#define IGMP_AWAKENING_MEMBER           5
#define IGMP_QUERY_PENDING_MEMBER       6    /* pending General Query */
#define IGMP_G_QUERY_PENDING_MEMBER     7    /* pending Grp-specific Query */
#define IGMP_SG_QUERY_PENDING_MEMBER    8    /* pending Grp-Src-specific Q.*/

/*********************************************************************************************************
  IGMPv3 query types.
*********************************************************************************************************/

#define IGMP_V3_GENERAL_QUERY           1
#define IGMP_V3_GROUP_QUERY             2
#define IGMP_V3_GROUP_SOURCE_QUERY      3

/*********************************************************************************************************
  States for IGMP router version cache.
*********************************************************************************************************/

#define IGMP_v1_ROUTER                  1
#define IGMP_v2_ROUTER                  2
#define IGMP_v3_ROUTER                  3

/*********************************************************************************************************
  The following four defininitions are for backwards compatibility.
  They should be removed as soon as all applications are updated to
  use the new constant names.
*********************************************************************************************************/

#define IGMP_HOST_MEMBERSHIP_QUERY      IGMP_MEMBERSHIP_QUERY
#define IGMP_HOST_MEMBERSHIP_REPORT     IGMP_V1_MEMBERSHIP_REPORT
#define IGMP_HOST_NEW_MEMBERSHIP_REPORT IGMP_V2_MEMBERSHIP_REPORT
#define IGMP_HOST_LEAVE_MESSAGE         IGMP_V2_LEAVE_GROUP

#endif                                                              /*  __NETINET_IGMP_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/

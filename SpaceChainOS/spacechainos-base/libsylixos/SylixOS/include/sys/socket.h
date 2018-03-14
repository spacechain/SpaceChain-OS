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
** 文   件   名: socket.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 05 月 10 日
**
** 描        述: socket 通信.
*********************************************************************************************************/

#ifndef __SYS_SOCKET_H
#define __SYS_SOCKET_H

#define AF_AX25         3                               /* Amateur Radio AX.25                          */
#define AF_IPX          4                               /* Novell IPX                                   */
#define AF_APPLETALK    5                               /* AppleTalk DDP                                */
#define AF_NETROM       6                               /* Amateur Radio NET/ROM                        */
#define AF_BRIDGE       7                               /* Multiprotocol bridge                         */
#define AF_ATMPVC       8                               /* ATM PVCs                                     */
#define AF_X25          9                               /* Reserved for X.25 project                    */
#define AF_ROSE         11                              /* Amateur Radio X.25 PLP                       */
#define AF_DECnet       12                              /* Reserved for DECnet project                  */
#define AF_NETBEUI      13                              /* Reserved for 802.2LLC project                */
#define AF_SECURITY     14                              /* Security callback pseudo AF                  */
#define AF_KEY          15                              /* PF_KEY key management API                    */
#define AF_LINK         18                              /* link                                         */
#define AF_ECONET       19                              /* Acorn Econet                                 */
#define AF_ATMSVC       20                              /* ATM SVCs                                     */
#define AF_RDS          21                              /* RDS sockets                                  */
#define AF_SNA          22                              /* Linux SNA Project (nutters!)                 */
#define AF_IRDA         23                              /* IRDA sockets                                 */
#define AF_PPPOX        24                              /* PPPoX sockets                                */
#define AF_WANPIPE      25                              /* Wanpipe API Sockets                          */
#define AF_LLC          26                              /* Linux LLC                                    */
#define AF_CAN          29                              /* Controller Area Network                      */
#define AF_TIPC         30                              /* TIPC sockets                                 */
#define AF_BLUETOOTH    31                              /* Bluetooth sockets                            */
#define AF_IUCV         32                              /* IUCV sockets                                 */
#define AF_RXRPC        33                              /* RxRPC sockets                                */
#define AF_ISDN         34                              /* mISDN sockets                                */
#define AF_PHONET       35                              /* Phonet sockets                               */
#define AF_IEEE802154   36                              /* IEEE802154 sockets                           */
#define AF_CAIF         37                              /* CAIF sockets                                 */
#define AF_ALG          38                              /* Algorithm sockets                            */
#define AF_NFC          39                              /* NFC sockets                                  */
#define AF_MAX          40                              /* For now..                                    */

#define PF_AX25         AF_AX25
#define PF_IPX          AF_IPX
#define PF_APPLETALK    AF_APPLETALK
#define PF_NETROM       AF_NETROM
#define PF_BRIDGE       AF_BRIDGE
#define PF_ATMPVC       AF_ATMPVC
#define PF_X25          AF_X25
#define PF_ROSE         AF_ROSE
#define PF_DECnet       AF_DECnet
#define PF_NETBEUI      AF_NETBEUI
#define PF_SECURITY     AF_SECURITY
#define PF_KEY          AF_KEY
#define PF_ROUTE        AF_ROUTE
#define PF_LINK         AF_LINK
#define PF_ECONET       AF_ECONET
#define PF_ATMSVC       AF_ATMSVC
#define PF_RDS          AF_RDS
#define PF_SNA          AF_SNA
#define PF_IRDA         AF_IRDA
#define PF_PPPOX        AF_PPPOX
#define PF_WANPIPE      AF_WANPIPE
#define PF_LLC          AF_LLC
#define PF_CAN          AF_CAN
#define PF_TIPC         AF_TIPC
#define PF_BLUETOOTH    AF_BLUETOOTH
#define PF_IUCV         AF_IUCV
#define PF_RXRPC        AF_RXRPC
#define PF_ISDN         AF_ISDN
#define PF_PHONET       AF_PHONET
#define PF_IEEE802154   AF_IEEE802154
#define PF_CAIF         AF_CAIF
#define PF_ALG          AF_ALG
#define PF_NFC          AF_NFC
#define PF_MAX          AF_MAX

#include "lwip/sockets.h"

#endif                                                                  /*  __SYS_SOCKET_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/

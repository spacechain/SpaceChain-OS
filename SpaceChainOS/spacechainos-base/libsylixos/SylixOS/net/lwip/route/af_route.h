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
** 文   件   名: af_route.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 12 月 08 日
**
** 描        述: AF_ROUTE 支持.
*********************************************************************************************************/

#ifndef __AF_ROUTE_H
#define __AF_ROUTE_H

#if LW_CFG_NET_EN > 0 && LW_CFG_NET_ROUTER > 0

#ifdef __SYLIXOS_RTHOOK
#include "ip4_route.h"
#include "ip6_route.h"
#endif                                                                  /*  __SYLIXOS_RTHOOK            */

/*********************************************************************************************************
  AF_ROUTE 接收队列
*********************************************************************************************************/

typedef struct af_route_node {                                          /*  一个消息节点                */
    LW_LIST_MONO        RTM_monoManage;                                 /*  待接收数据链                */
    size_t              RTM_stLen;                                      /*  消息长度                    */
} AF_ROUTE_N;

typedef struct af_route_queue {
    PLW_LIST_MONO       RTQ_pmonoHeader;                                /*  消息队列头                  */
    PLW_LIST_MONO       RTQ_pmonoTail;                                  /*  消息队列结尾                */
    size_t              RTQ_stTotal;                                    /*  总有效数据字节数            */
} AF_ROUTE_Q;

/*********************************************************************************************************
  AF_ROUTE 控制块
*********************************************************************************************************/

typedef struct af_route_t {
    LW_LIST_LINE        ROUTE_lineManage;
    INT                 ROUTE_iFlag;                                    /*  打开方式                    */
    
#define __AF_ROUTE_SHUTD_R      0x01
#define __AF_ROUTE_SHUTD_W      0x02
    INT                 ROUTE_iShutDFlag;                               /*  当前 shutdown 状态          */
    
    LW_OBJECT_HANDLE    ROUTE_hCanRead;                                 /*  是否能读                    */
    ULONG               ROUTE_ulRecvTimeout;                            /*  接收超时                    */
    
    AF_ROUTE_Q          ROUTE_rtq;                                      /*  接收消息队列                */
    size_t              ROUTE_stMaxBufSize;                             /*  最大接收缓冲大小            */
} AF_ROUTE_T;

/*********************************************************************************************************
  内部函数
*********************************************************************************************************/

VOID         route_init(VOID);
AF_ROUTE_T  *route_socket(INT  iDomain, INT  iType, INT  iProtocol);
INT          route_close(AF_ROUTE_T  *pafroute);
ssize_t      route_send(AF_ROUTE_T  *pafroute, const void *data, size_t size, int flags);
ssize_t      route_sendmsg(AF_ROUTE_T  *pafroute, const struct msghdr *msg, int flags);
ssize_t      route_recv(AF_ROUTE_T  *pafroute, void *mem, size_t len, int flags);
ssize_t      route_recvmsg(AF_ROUTE_T  *pafroute, struct msghdr *msg, int flags);
INT          route_setsockopt(AF_ROUTE_T  *pafroute, int level, int optname, const void *optval, socklen_t optlen);
INT          route_getsockopt(AF_ROUTE_T  *pafroute, int level, int optname, void *optval, socklen_t *optlen);
INT          route_shutdown(AF_ROUTE_T  *pafroute, int how);
INT          route_ioctl(AF_ROUTE_T  *pafroute, INT  iCmd, PVOID  pvArg);

/*********************************************************************************************************
  消息接口 (iAddrType: 0: ipaddr 1: netmask)
*********************************************************************************************************/

#ifdef __SYLIXOS_RTHOOK
VOID         route_hook_rt_ipv4(u_char type, const struct rt_entry *pentry, int nolock);
#if LWIP_IPV6
VOID         route_hook_rt_ipv6(u_char type, const struct rt6_entry *pentry, int nolock);
#endif                                                                  /*  LWIP_IPV6                   */

VOID         route_hook_netif_ipv4(struct netif *pnetif, const ip4_addr_t *pipaddr, const ip4_addr_t *pnetmask, u_char type);
#if LWIP_IPV6
VOID         route_hook_netif_ipv6(struct netif *pnetif, const ip6_addr_t *pipaddr, u_char type);
#endif                                                                  /*  LWIP_IPV6                   */

VOID         route_hook_maddr_ipv4(struct netif *pnetif, const ip4_addr_t *pipaddr, u_char type);
#if LWIP_IPV6
VOID         route_hook_maddr_ipv6(struct netif *pnetif, const ip6_addr_t *pip6addr, u_char type);
#endif                                                                  /*  LWIP_IPV6                   */

VOID         route_hook_netif_ann(struct netif *pnetif, int what);

VOID         route_hook_netif_updown(struct netif *pnetif);
#endif                                                                  /*  __SYLIXOS_RTHOOK            */

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_ROUTER > 0       */
#endif                                                                  /*  __AF_ROUTE_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/

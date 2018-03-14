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
** 文   件   名: af_route.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 12 月 08 日
**
** 描        述: AF_ROUTE 支持.
*********************************************************************************************************/
#define  __SYLIXOS_RTHOOK
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_ROUTER > 0
#include "net/if.h"
#include "net/if_dl.h"
#include "net/if_flags.h"
#include "net/if_type.h"
#include "net/if_ether.h"
#include "net/route.h"
#include "lwip/mem.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "af_route.h"
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
extern void  __route_socket_event(AF_ROUTE_T  *pafroute, LW_SEL_TYPE type, INT  iSoErr);
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_LIST_LINE_HEADER          _G_plineAfRoute;
/*********************************************************************************************************
  AF_ROUTE 锁
*********************************************************************************************************/
#define __AF_ROUTE_LOCK()           RT_LOCK()
#define __AF_ROUTE_UNLOCK()         RT_UNLOCK()
/*********************************************************************************************************
  AF_ROUTE 缓冲区
*********************************************************************************************************/
#define __AF_ROUTE_DEF_BUFSIZE      (LW_CFG_KB_SIZE * 64)               /*  默认为 64K 接收缓冲         */
#define __AF_ROUTE_DEF_BUFMAX       (LW_CFG_KB_SIZE * 256)              /*  默认为 256K 接收缓冲        */
#define __AF_ROUTE_DEF_BUFMIN       (LW_CFG_KB_SIZE * 8)                /*  最小接收缓冲大小            */
/*********************************************************************************************************
  AF_ROUTE 通知
*********************************************************************************************************/
#define __AF_ROUTE_WAIT(pafroute)   API_SemaphoreBPend(pafroute->ROUTE_hCanRead, LW_OPTION_WAIT_INFINITE)
#define __AF_ROUTE_NOTIFY(pafroute) API_SemaphoreBPost(pafroute->ROUTE_hCanRead)
/*********************************************************************************************************
  等待判断
*********************************************************************************************************/
#define __AF_ROUTE_IS_NBIO(pafroute, flags)  \
        ((pafroute->ROUTE_iFlag & O_NONBLOCK) || (flags & MSG_DONTWAIT))
/*********************************************************************************************************
** 函数名称: __routeMsToTicks
** 功能描述: 毫秒转换为 ticks
** 输　入  : ulMs        毫秒
** 输　出  : tick
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ULONG  __routeMsToTicks (ULONG  ulMs)
{
    ULONG   ulTicks;
    
    if (ulMs == 0) {
        ulTicks = LW_OPTION_WAIT_INFINITE;
    } else {
        ulTicks = LW_MSECOND_TO_TICK_1(ulMs);
    }
    
    return  (ulTicks);
}
/*********************************************************************************************************
** 函数名称: __routeTvToTicks
** 功能描述: timeval 转换为 ticks
** 输　入  : ptv       时间
** 输　出  : tick
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ULONG  __routeTvToTicks (struct timeval  *ptv)
{
    ULONG   ulTicks;

    if ((ptv->tv_sec == 0) && (ptv->tv_usec == 0)) {
        return  (LW_OPTION_WAIT_INFINITE);
    }
    
    ulTicks = __timevalToTick(ptv);
    if (ulTicks == 0) {
        ulTicks = 1;
    }
    
    return  (ulTicks);
}
/*********************************************************************************************************
** 函数名称: __routeTicksToMs
** 功能描述: ticks 转换为毫秒
** 输　入  : ulTicks       tick
** 输　出  : 毫秒
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ULONG  __routeTicksToMs (ULONG  ulTicks)
{
    ULONG  ulMs;
    
    if (ulTicks == LW_OPTION_WAIT_INFINITE) {
        ulMs = 0;
    } else {
        ulMs = (ulTicks * 1000) / LW_TICK_HZ;
    }
    
    return  (ulMs);
}
/*********************************************************************************************************
** 函数名称: __routeTicksToTv
** 功能描述: ticks 转换为 timeval
** 输　入  : ulTicks       tick
** 输　出  : timeval
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID __routeTicksToTv (ULONG  ulTicks, struct timeval *ptv)
{
    if (ulTicks == LW_OPTION_WAIT_INFINITE) {
        ptv->tv_sec  = 0;
        ptv->tv_usec = 0;
    } else {
        __tickToTimeval(ulTicks, ptv);
    }
}
/*********************************************************************************************************
** 函数名称: __routeDeleteBuf
** 功能描述: 删除 route 套接字缓存
** 输　入  : pafroute  route file
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __routeDeleteBuf (AF_ROUTE_T  *pafroute)
{
    AF_ROUTE_N    *prouten;
    AF_ROUTE_Q    *prouteq;
    
    prouteq = &pafroute->ROUTE_rtq;
    
    while (prouteq->RTQ_pmonoHeader) {
        prouten = (AF_ROUTE_N *)prouteq->RTQ_pmonoHeader;
        _list_mono_allocate_seq(&prouteq->RTQ_pmonoHeader,
                                &prouteq->RTQ_pmonoTail);
        mem_free(prouten);
    }
    
    prouteq->RTQ_stTotal = 0;
}
/*********************************************************************************************************
** 函数名称: __routeUpdateReader
** 功能描述: 通知 route 套接字可读
** 输　入  : pafroute  route file
**           iSoErr    错误类型
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __routeUpdateReader (AF_ROUTE_T  *pafroute, INT  iSoErr)
{
    __AF_ROUTE_NOTIFY(pafroute);
    
    __route_socket_event(pafroute, SELREAD, iSoErr);                    /*  本地 select 可读            */
}
/*********************************************************************************************************
** 函数名称: __routeRtmAdd4
** 功能描述: 添加一条 IPv4 路由
** 输　入  : pmsghdr   路由消息
** 输　出  : 写入数据量
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __routeRtmAdd4 (struct rt_msghdr  *pmsghdr)
{
    INT               iRet;
    struct rt_entry  *pentry;
    
    if ((pmsghdr->rtm_addrs & (RTA_DST | RTA_GATEWAY)) != (RTA_DST | RTA_GATEWAY)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    pentry = (struct rt_entry *)mem_malloc(sizeof(struct rt_entry));
    if (!pentry) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    lib_bzero(pentry, sizeof(struct rt_entry));
    
    if (rt_msghdr_to_entry(pentry, pmsghdr) < 0) {                      /*  转换为 rt_entry             */
        mem_free(pentry);
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    if (pentry->rt_dest.addr == IPADDR_ANY) {
        if ((pentry->rt_gateway.addr == IPADDR_ANY) && (pentry->rt_ifname[0] == '\0')) {
            mem_free(pentry);
            _ErrorHandle(ENETUNREACH);
            return  (0);
        }
        
        iRet = rt_change_default(&pentry->rt_gateway, pentry->rt_ifname);
        if (iRet < 0) {
            _ErrorHandle(ENETUNREACH);
            return  (0);
        }
        
        route_hook_rt_ipv4(RTM_ADD, pentry, 1);                         /*  添加接收缓冲                */
        mem_free(pentry);
    
    } else {
        if (rt_find_entry(&pentry->rt_dest, &pentry->rt_netmask, &pentry->rt_gateway, 
                          pentry->rt_ifname, pentry->rt_flags)) {
            mem_free(pentry);
            _ErrorHandle(EEXIST);
            return  (0);
        }
        
        iRet = rt_add_entry(pentry);                                    /*  添加路由条目                */
        if (iRet < 0) {
            mem_free(pentry);
            _ErrorHandle(ENETUNREACH);
            return  (0);
        
        } else {
            route_hook_rt_ipv4(RTM_ADD, pentry, 1);                     /*  添加接收缓冲                */
        }
    }
        
    return  ((ssize_t)pmsghdr->rtm_msglen);
}
/*********************************************************************************************************
** 函数名称: __routeRtmAdd6
** 功能描述: 添加一条 IPv6 路由
** 输　入  : pmsghdr   路由消息
** 输　出  : 写入数据量
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LWIP_IPV6

static ssize_t  __routeRtmAdd6 (struct rt_msghdr  *pmsghdr)
{
    INT                iRet;
    struct rt6_entry  *pentry6;
    
    if ((pmsghdr->rtm_addrs & (RTA_DST | RTA_GATEWAY)) != (RTA_DST | RTA_GATEWAY)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    pentry6 = (struct rt6_entry *)mem_malloc(sizeof(struct rt6_entry));
    if (!pentry6) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    lib_bzero(pentry6, sizeof(struct rt6_entry));
    
    if (rt6_msghdr_to_entry(pentry6, pmsghdr) < 0) {                    /*  转换为 rt6_entry            */
        mem_free(pentry6);
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    if (ip6_addr_isany_val(pentry6->rt6_dest)) {
        mem_free(pentry6);
        _ErrorHandle(ENETUNREACH);
        return  (0);
    
    } else {
        if (rt6_find_entry(&pentry6->rt6_dest, &pentry6->rt6_netmask, &pentry6->rt6_gateway, 
                           pentry6->rt6_ifname, pentry6->rt6_flags)) {
            mem_free(pentry6);
            _ErrorHandle(EEXIST);
            return  (0);
        }
        
        iRet = rt6_add_entry(pentry6);                                  /*  添加路由条目                */
        if (iRet < 0) {
            mem_free(pentry6);
            _ErrorHandle(ENETUNREACH);
            return  (0);
        
        } else {
            route_hook_rt_ipv6(RTM_ADD, pentry6, 1);                    /*  添加接收缓冲                */
        }
    }
        
    return  ((ssize_t)pmsghdr->rtm_msglen);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** 函数名称: __routeRtmChange4
** 功能描述: 修改一条 IPv4 路由
** 输　入  : pmsghdr   路由消息
** 输　出  : 写入数据量
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __routeRtmChange4 (struct rt_msghdr  *pmsghdr)
{
    INT                 iRet;
    struct netif       *pnetif;
    struct rt_entry    *pentry;
    struct sockaddr_in *psockaddrin;
    ip4_addr_t          ipaddr;
    ip4_addr_t          ipnetmask;
    ip4_addr_t          ipgateway;
    CHAR                cIfName[IF_NAMESIZE] = "";
    
    if ((pmsghdr->rtm_addrs & (RTA_DST | RTA_GATEWAY)) != (RTA_DST | RTA_GATEWAY)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    pnetif = netif_get_by_index(pmsghdr->rtm_index);
    if (pnetif) {
        netif_get_name(pnetif, cIfName);
    }
    
    psockaddrin = (struct sockaddr_in *)(pmsghdr + 1);
    ipaddr.addr = psockaddrin->sin_addr.s_addr;
    
    psockaddrin = SA_NEXT(struct sockaddr_in *, psockaddrin);
    ipgateway.addr = psockaddrin->sin_addr.s_addr;
    
    if (pmsghdr->rtm_addrs & RTA_NETMASK) {
        psockaddrin = SA_NEXT(struct sockaddr_in *, psockaddrin);
        ipnetmask.addr = psockaddrin->sin_addr.s_addr;
    } else {
        ipnetmask.addr = IPADDR_ANY;
    }
    
    if (ipaddr.addr == IPADDR_ANY) {                                    /*  设置默认路由                */
        if ((ipgateway.addr == IPADDR_ANY) && (cIfName[0] == '\0')) {
            _ErrorHandle(ENETUNREACH);
            return  (0);
        }
        
        iRet = rt_change_default(&ipgateway, cIfName);
        if (iRet < 0) {
            _ErrorHandle(ENETUNREACH);
            return  (0);
        }
        
        pentry = (struct rt_entry *)mem_malloc(sizeof(struct rt_entry));
        if (pentry) {
            iRet = rt_get_default(pentry);
            if (iRet == 0) {
                route_hook_rt_ipv4(RTM_CHANGE, pentry, 1);              /*  添加接收缓冲                */
            }
            mem_free(pentry);
        }
        
    } else {
        pentry = rt_find_entry(&ipaddr, &ipnetmask, &ipgateway, 
                               cIfName, pmsghdr->rtm_flags);
        if (!pentry || pentry->rt_type) {
            _ErrorHandle(ENETUNREACH);
            return  (0);
        }
        rt_delete_entry(pentry);
        
        if (rt_msghdr_to_entry(pentry, pmsghdr) < 0) {                  /*  转换为 rt_entry             */
            _ErrorHandle(EINVAL);
            return  (0);
        }
        
        iRet = rt_add_entry(pentry);
        if (iRet < 0) {
            mem_free(pentry);
            _ErrorHandle(ENETUNREACH);
            return  (0);
        
        } else {
            route_hook_rt_ipv4(RTM_CHANGE, pentry, 1);                  /*  添加接收缓冲                */
        }
    }
        
    return  ((ssize_t)pmsghdr->rtm_msglen);
}
/*********************************************************************************************************
** 函数名称: __routeRtmChange6
** 功能描述: 修改一条 IPv6 路由
** 输　入  : pmsghdr   路由消息
** 输　出  : 写入数据量
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LWIP_IPV6

static ssize_t  __routeRtmChange6 (struct rt_msghdr  *pmsghdr)
{
    INT                  iRet;
    struct netif        *pnetif;
    struct rt6_entry    *pentry6;
    struct sockaddr_in6 *psockaddrin6;
    ip6_addr_t           ip6addr;
    ip6_addr_t           ip6netmask;
    ip6_addr_t           ip6gateway;
    CHAR                 cIfName[IF_NAMESIZE] = "";
    
    if ((pmsghdr->rtm_addrs & (RTA_DST | RTA_GATEWAY)) != (RTA_DST | RTA_GATEWAY)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    pnetif = netif_get_by_index(pmsghdr->rtm_index);
    if (pnetif) {
        netif_get_name(pnetif, cIfName);
    }
    
    psockaddrin6 = (struct sockaddr_in6 *)(pmsghdr + 1);
    inet6_addr_to_ip6addr(&ip6addr, &psockaddrin6->sin6_addr);
    
    psockaddrin6 = SA_NEXT(struct sockaddr_in6 *, psockaddrin6);
    inet6_addr_to_ip6addr(&ip6gateway, &psockaddrin6->sin6_addr);
    
    if (pmsghdr->rtm_addrs & RTA_NETMASK) {
        psockaddrin6 = SA_NEXT(struct sockaddr_in6 *, psockaddrin6);
        inet6_addr_to_ip6addr(&ip6netmask, &psockaddrin6->sin6_addr);
    } else {
        ip6_addr_set_any(&ip6netmask);
    }
    
    if (ip6_addr_isany_val(ip6addr)) {
        _ErrorHandle(ENETUNREACH);
        return  (0);

    } else {
        pentry6 = rt6_find_entry(&ip6addr, &ip6netmask, &ip6gateway, 
                                 cIfName, pmsghdr->rtm_flags);
        if (!pentry6 || pentry6->rt6_type) {
            _ErrorHandle(ENETUNREACH);
            return  (0);
        }
        rt6_delete_entry(pentry6);
        
        if (rt6_msghdr_to_entry(pentry6, pmsghdr) < 0) {                /*  转换为 rt6_entry            */
            _ErrorHandle(EINVAL);
            return  (0);
        }
        
        iRet = rt6_add_entry(pentry6);
        if (iRet < 0) {
            mem_free(pentry6);
            _ErrorHandle(ENETUNREACH);
            return  (0);
        
        } else {
            route_hook_rt_ipv6(RTM_CHANGE, pentry6, 1);                 /*  添加接收缓冲                */
        }
    }
        
    return  ((ssize_t)pmsghdr->rtm_msglen);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** 函数名称: __routeRtmDelete4
** 功能描述: 删除一条 IPv4 路由
** 输　入  : pmsghdr   路由消息
** 输　出  : 写入数据量
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __routeRtmDelete4 (struct rt_msghdr  *pmsghdr)
{
    struct netif       *pnetif;
    struct rt_entry    *pentry;
    struct sockaddr_in *psockaddrin;
    ip4_addr_t          ipaddr;
    ip4_addr_t          ipnetmask;
    ip4_addr_t          ipgateway;
    CHAR                cIfName[IF_NAMESIZE] = "";
    
    if (!(pmsghdr->rtm_addrs & RTA_DST)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    pnetif = netif_get_by_index(pmsghdr->rtm_index);
    if (pnetif) {
        netif_get_name(pnetif, cIfName);
    }
    
    psockaddrin = (struct sockaddr_in *)(pmsghdr + 1);
    ipaddr.addr = psockaddrin->sin_addr.s_addr;
    
    if (pmsghdr->rtm_addrs & RTA_GATEWAY) {
        psockaddrin = SA_NEXT(struct sockaddr_in *, psockaddrin);
        ipgateway.addr = psockaddrin->sin_addr.s_addr;
    } else {
        ipgateway.addr = IPADDR_ANY;
    }
    
    if (pmsghdr->rtm_addrs & RTA_NETMASK) {
        psockaddrin = SA_NEXT(struct sockaddr_in *, psockaddrin);
        ipnetmask.addr = psockaddrin->sin_addr.s_addr;
    } else {
        ipnetmask.addr = IPADDR_ANY;
    }
    
    pentry = rt_find_entry(&ipaddr, &ipnetmask, &ipgateway, 
                           cIfName, pmsghdr->rtm_flags);
    if (!pentry || pentry->rt_type) {
        _ErrorHandle(ENETUNREACH);
        return  (0);
    }
    rt_delete_entry(pentry);
    
    route_hook_rt_ipv4(RTM_DELETE, pentry, 1);                          /*  添加接收缓冲                */
    mem_free(pentry);
    
    return  ((ssize_t)pmsghdr->rtm_msglen);
}
/*********************************************************************************************************
** 函数名称: __routeRtmDelete6
** 功能描述: 删除一条 IPv6 路由
** 输　入  : pmsghdr   路由消息
** 输　出  : 写入数据量
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LWIP_IPV6

static ssize_t  __routeRtmDelete6 (struct rt_msghdr  *pmsghdr)
{
    struct netif       *pnetif;
    struct rt6_entry    *pentry6;
    struct sockaddr_in6 *psockaddrin6;
    ip6_addr_t           ip6addr;
    ip6_addr_t           ip6netmask;
    ip6_addr_t           ip6gateway;
    CHAR                 cIfName[IF_NAMESIZE] = "";
    
    if (!(pmsghdr->rtm_addrs & RTA_DST)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    pnetif = netif_get_by_index(pmsghdr->rtm_index);
    if (pnetif) {
        netif_get_name(pnetif, cIfName);
    }
    
    psockaddrin6 = (struct sockaddr_in6 *)(pmsghdr + 1);
    inet6_addr_to_ip6addr(&ip6addr, &psockaddrin6->sin6_addr);
    
    if (pmsghdr->rtm_addrs & RTA_GATEWAY) {
        psockaddrin6 = SA_NEXT(struct sockaddr_in6 *, psockaddrin6);
        inet6_addr_to_ip6addr(&ip6gateway, &psockaddrin6->sin6_addr);
    } else {
        ip6_addr_set_any(&ip6gateway);
    }
    
    if (pmsghdr->rtm_addrs & RTAX_NETMASK) {
        psockaddrin6 = SA_NEXT(struct sockaddr_in6 *, psockaddrin6);
        inet6_addr_to_ip6addr(&ip6netmask, &psockaddrin6->sin6_addr);
    } else {
        ip6_addr_set_any(&ip6netmask);
    }
    
    if (ip6_addr_isany_val(ip6addr)) {
        _ErrorHandle(ENETUNREACH);
        return  (0);
    }
    
    pentry6 = rt6_find_entry(&ip6addr, &ip6netmask, &ip6gateway, 
                             cIfName, pmsghdr->rtm_flags);
    if (!pentry6 || pentry6->rt6_type) {
        _ErrorHandle(ENETUNREACH);
        return  (0);
    }
    rt6_delete_entry(pentry6);
    
    route_hook_rt_ipv6(RTM_DELETE, pentry6, 1);                         /*  添加接收缓冲                */
    mem_free(pentry6);
    
    return  ((ssize_t)pmsghdr->rtm_msglen);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** 函数名称: __routeRtmGet4Walk
** 功能描述: 获取一条 IPv4 路由遍历
** 输　入  : pentry     路由条目
**           ipaddr     地址
**           ipgateway  网关地址
**           pcIfname   网络接口名
**           iRet       完成情况
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __routeRtmGet4Walk (struct rt_entry *pentry, 
                                 ip4_addr_t      *ipaddr, 
                                 ip4_addr_t      *ipgateway, 
                                 char            *pcIfname,
                                 INT             *iRet)
{
    if (pentry->rt_dest.addr != ipaddr->addr) {
        return;
    }
    
    if ((ipgateway->addr != IPADDR_ANY) && 
        (pentry->rt_gateway.addr != ipgateway->addr)) {
        return;
    }
    
    if ((pcIfname[0] != PX_EOS) && 
        lib_strcmp(pentry->rt_ifname, pcIfname)) {
        return;
    }

    route_hook_rt_ipv4(RTM_GET, pentry, 1);
    *iRet = ERROR_NONE;
}
/*********************************************************************************************************
** 函数名称: __routeRtmGet4
** 功能描述: 获取一条 IPv4 路由
** 输　入  : pmsghdr   路由消息
** 输　出  : 写入数据量
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __routeRtmGet4 (struct rt_msghdr  *pmsghdr)
{
    INT                 iRet = PX_ERROR;
    struct sockaddr_in *psockaddrin;
    struct sockaddr_dl *psockaddrdl;
    ip4_addr_t          ipaddr;
    ip4_addr_t          ipgateway;
    char                cIfname[IF_NAMESIZE] = "";
    
    if (!(pmsghdr->rtm_addrs & RTA_DST)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    psockaddrin = (struct sockaddr_in *)(pmsghdr + 1);
    ipaddr.addr = psockaddrin->sin_addr.s_addr;
    
    if (pmsghdr->rtm_addrs & RTA_GATEWAY) {
        psockaddrin = SA_NEXT(struct sockaddr_in *, psockaddrin);
        ipgateway.addr = psockaddrin->sin_addr.s_addr;
    
    } else {
        ipgateway.addr = IPADDR_ANY;
    }
    
    if (pmsghdr->rtm_addrs & RTA_NETMASK) {
        psockaddrin = SA_NEXT(struct sockaddr_in *, psockaddrin);
    }
    
    if (pmsghdr->rtm_addrs & RTA_GENMASK) {
        psockaddrin = SA_NEXT(struct sockaddr_in *, psockaddrin);
    }
    
    if (pmsghdr->rtm_addrs & RTA_IFP) {
        psockaddrdl = SA_NEXT(struct sockaddr_dl *, psockaddrin);
        if (psockaddrdl->sdl_family == AF_LINK) {
            lib_strlcpy(cIfname, psockaddrdl->sdl_data, IF_NAMESIZE);
        }
    }
    
    rt_traversal_entry(__routeRtmGet4Walk, &ipaddr, &ipgateway, cIfname, &iRet, LW_NULL, LW_NULL);
    if (iRet < 0) {
        _ErrorHandle(ENETUNREACH);
        return  (0);
    }
    
    return  ((ssize_t)pmsghdr->rtm_msglen);
}
/*********************************************************************************************************
** 函数名称: __routeRtmGet6Walk
** 功能描述: 获取一条 IPv6 路由遍历
** 输　入  : pentry      路由条目
**           ip6addr     地址
**           ip6gateway  网关地址
**           pcIfname    网络接口名
**           iRet        完成情况
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LWIP_IPV6

static VOID  __routeRtmGet6Walk (struct rt6_entry *pentry6, 
                                 ip6_addr_t       *ip6addr, 
                                 ip6_addr_t       *ip6gateway, 
                                 char             *pcIfname,
                                 INT              *iRet)
{
    if (!ip6_addr_cmp(&pentry6->rt6_dest, ip6addr)) {
        return;
    }
    
    if (!ip6_addr_isany(ip6gateway) && 
        !ip6_addr_cmp(&pentry6->rt6_gateway, ip6gateway)) {
        return;
    }
    
    if ((pcIfname[0] != PX_EOS) && 
        lib_strcmp(pentry6->rt6_ifname, pcIfname)) {
        return;
    }
    
    route_hook_rt_ipv6(RTM_GET, pentry6, 1);
    *iRet = ERROR_NONE;
}
/*********************************************************************************************************
** 函数名称: __routeRtmGet6
** 功能描述: 获取一条 IPv6 路由
** 输　入  : pmsghdr   路由消息
** 输　出  : 写入数据量
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __routeRtmGet6 (struct rt_msghdr  *pmsghdr)
{
    INT                  iRet = PX_ERROR;
    struct sockaddr_in6 *psockaddrin6;
    struct sockaddr_dl  *psockaddrdl;
    ip6_addr_t           ip6addr;
    ip6_addr_t           ip6gateway;
    char                 cIfname[IF_NAMESIZE] = "";
    
    if (!(pmsghdr->rtm_addrs & RTA_DST)) {
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    psockaddrin6 = (struct sockaddr_in6 *)(pmsghdr + 1);
    inet6_addr_to_ip6addr(&ip6addr, &psockaddrin6->sin6_addr);
    
    if (pmsghdr->rtm_addrs & RTA_GATEWAY) {
        psockaddrin6 = SA_NEXT(struct sockaddr_in6 *, psockaddrin6);
        inet6_addr_to_ip6addr(&ip6gateway, &psockaddrin6->sin6_addr);
    
    } else {
        ip6_addr_set_any(&ip6gateway);
    }
    
    if (pmsghdr->rtm_addrs & RTA_NETMASK) {
        psockaddrin6 = SA_NEXT(struct sockaddr_in6 *, psockaddrin6);
    }
    
    if (pmsghdr->rtm_addrs & RTA_GENMASK) {
        psockaddrin6 = SA_NEXT(struct sockaddr_in6 *, psockaddrin6);
    }
    
    if (pmsghdr->rtm_addrs & RTA_IFP) {
        psockaddrdl = SA_NEXT(struct sockaddr_dl *, psockaddrin6);
        if (psockaddrdl->sdl_family == AF_LINK) {
            lib_strlcpy(cIfname, psockaddrdl->sdl_data, IF_NAMESIZE);
        }
    }
    
    rt6_traversal_entry(__routeRtmGet6Walk, &ip6addr, &ip6gateway, cIfname, &iRet, LW_NULL, LW_NULL);
    if (iRet < 0) {
        _ErrorHandle(ENETUNREACH);
        return  (0);
    }
    
    return  ((ssize_t)pmsghdr->rtm_msglen);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** 函数名称: route_hook_rt_ipv4
** 功能描述: 增加 / 删除 / 修改一条 IPv4 路由回调
** 输　入  : type      RTM_ADD / RTM_DELETE / RTM_CHANGE / RTM_GET
**           pentry    IPv4 路由表项
**           nolock    是否加锁
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  route_hook_rt_ipv4 (u_char type, const struct rt_entry *pentry, int nolock)
{
    size_t              stMsgLen;
    struct rt_msghdr   *pmsghdr;
    
    PLW_LIST_LINE       pline;
    AF_ROUTE_T         *pafroute;
    AF_ROUTE_Q         *prouteq;
    AF_ROUTE_N         *prouten;
    
    if (!_G_plineAfRoute) {
        return;
    }
    
    stMsgLen = rt_entry_to_msghdr(LW_NULL, pentry, type);
    if (!nolock) {
        __AF_ROUTE_LOCK();
    }
    for (pline = _G_plineAfRoute; pline != LW_NULL; pline = _list_line_get_next(pline)) {
        pafroute = _LIST_ENTRY(pline, AF_ROUTE_T, ROUTE_lineManage);
        if (((pafroute->ROUTE_rtq.RTQ_stTotal + stMsgLen) <= pafroute->ROUTE_stMaxBufSize) &&
            !(pafroute->ROUTE_iShutDFlag & __AF_ROUTE_SHUTD_R)) {
            prouten = (AF_ROUTE_N *)mem_malloc(sizeof(AF_ROUTE_N) + stMsgLen);
            if (!prouten) {
                break;
            }
            
            prouten->RTM_stLen = stMsgLen;
            pmsghdr = (struct rt_msghdr *)(prouten + 1);
            rt_entry_to_msghdr(pmsghdr, pentry, type);
            
            prouteq = &pafroute->ROUTE_rtq;
            _list_mono_free_seq(&prouteq->RTQ_pmonoHeader,
                                &prouteq->RTQ_pmonoTail,
                                &prouten->RTM_monoManage);
            prouteq->RTQ_stTotal += prouten->RTM_stLen;
            
            __routeUpdateReader(pafroute, ERROR_NONE);
        }
    }
    if (!nolock) {
        __AF_ROUTE_UNLOCK();
    }
}
/*********************************************************************************************************
** 函数名称: route_hook_rt_ipv6
** 功能描述: 增加 / 删除 / 修改一条 IPv4 路由回调
** 输　入  : type      RTM_ADD / RTM_DELETE / RTM_CHANGE / RTM_GET
**           pentry    IPv6 路由表项
**           nolock    是否加锁
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LWIP_IPV6

VOID  route_hook_rt_ipv6 (u_char type, const struct rt6_entry *pentry, int nolock)
{
    size_t               stMsgLen;
    struct rt_msghdr    *pmsghdr;
    
    PLW_LIST_LINE        pline;
    AF_ROUTE_T          *pafroute;
    AF_ROUTE_Q          *prouteq;
    AF_ROUTE_N          *prouten;
    
    if (!_G_plineAfRoute) {
        return;
    }
    
    stMsgLen = rt6_entry_to_msghdr(LW_NULL, pentry, type);
    if (!nolock) {
        __AF_ROUTE_LOCK();
    }
    for (pline = _G_plineAfRoute; pline != LW_NULL; pline = _list_line_get_next(pline)) {
        pafroute = _LIST_ENTRY(pline, AF_ROUTE_T, ROUTE_lineManage);
        if (((pafroute->ROUTE_rtq.RTQ_stTotal + stMsgLen) <= pafroute->ROUTE_stMaxBufSize) &&
            !(pafroute->ROUTE_iShutDFlag & __AF_ROUTE_SHUTD_R)) {
            prouten = (AF_ROUTE_N *)mem_malloc(sizeof(AF_ROUTE_N) + stMsgLen);
            if (!prouten) {
                break;
            }
            
            prouten->RTM_stLen = stMsgLen;
            pmsghdr = (struct rt_msghdr *)(prouten + 1);
            rt6_entry_to_msghdr(pmsghdr, pentry, type);
            
            prouteq = &pafroute->ROUTE_rtq;
            _list_mono_free_seq(&prouteq->RTQ_pmonoHeader,
                                &prouteq->RTQ_pmonoTail,
                                &prouten->RTM_monoManage);
            prouteq->RTQ_stTotal += prouten->RTM_stLen;
            
            __routeUpdateReader(pafroute, ERROR_NONE);
        }
    }
    if (!nolock) {
        __AF_ROUTE_UNLOCK();
    }
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** 函数名称: route_hook_netif_ipv4
** 功能描述: 网络接口修改 IPv4 地址
** 输　入  : pnetif      网络接口
**           pipaddr     地址
**           pnetmask    掩码
**           type        RTM_DELADDR / RTM_ADDADDR
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  route_hook_netif_ipv4 (struct netif *pnetif, const ip4_addr_t *pipaddr, const ip4_addr_t *pnetmask, u_char type)
{
    size_t               stMsgLen;
    struct sockaddr_in  *psockaddrin;
    struct sockaddr_dl  *psockaddrdl;
    struct ifa_msghdr   *pmsghdr;
    int                  iFlags;
    
    PLW_LIST_LINE        pline;
    AF_ROUTE_T          *pafroute;
    AF_ROUTE_Q          *prouteq;
    AF_ROUTE_N          *prouten;
    
    if (!_G_plineAfRoute) {
        return;
    }
    
    iFlags   = netif_get_flags(pnetif);
    stMsgLen = sizeof(struct ifa_msghdr) 
             + (SO_ROUND_UP(sizeof(struct sockaddr_in)) * 3)
             + SO_ROUND_UP(sizeof(struct sockaddr_dl));
             
    __AF_ROUTE_LOCK();
    for (pline = _G_plineAfRoute; pline != LW_NULL; pline = _list_line_get_next(pline)) {
        pafroute = _LIST_ENTRY(pline, AF_ROUTE_T, ROUTE_lineManage);
        if (((pafroute->ROUTE_rtq.RTQ_stTotal + (stMsgLen * 2)) <= pafroute->ROUTE_stMaxBufSize) &&
            !(pafroute->ROUTE_iShutDFlag & __AF_ROUTE_SHUTD_R)) {
            prouten = (AF_ROUTE_N *)mem_malloc(sizeof(AF_ROUTE_N) + stMsgLen);
            if (!prouten) {
                break;
            }
            
            prouten->RTM_stLen = stMsgLen;
            pmsghdr = (struct ifa_msghdr *)(prouten + 1);
            
            pmsghdr->ifam_msglen  = (u_short)stMsgLen;
            pmsghdr->ifam_version = RTM_VERSION;
            pmsghdr->ifam_type    = type;
            
            pmsghdr->ifam_addrs  = RTA_NETMASK | RTA_IFP | RTA_IFA | RTA_BRD;
            pmsghdr->ifam_flags  = iFlags;
            pmsghdr->ifam_index  = netif_get_index(pnetif);
            pmsghdr->ifam_metric = 0;
            
            psockaddrin = (struct sockaddr_in *)(pmsghdr + 1);
            psockaddrin->sin_len         = sizeof(struct sockaddr_in);
            psockaddrin->sin_family      = AF_INET;
            psockaddrin->sin_addr.s_addr = pnetmask->addr;
            
            psockaddrdl = SA_NEXT(struct sockaddr_dl *, psockaddrin);
            lib_bzero(psockaddrdl, sizeof(struct sockaddr_dl));
            rt_build_sockaddr_dl(psockaddrdl, pnetif);
            
            psockaddrin = SA_NEXT(struct sockaddr_in *, psockaddrdl);
            psockaddrin->sin_len         = sizeof(struct sockaddr_in);
            psockaddrin->sin_family      = AF_INET;
            psockaddrin->sin_addr.s_addr = pipaddr->addr;
            
            psockaddrin = SA_NEXT(struct sockaddr_in *, psockaddrin);
            if (pnetif->flags & NETIF_FLAG_BROADCAST) {
                psockaddrin->sin_len         = sizeof(struct sockaddr_in);
                psockaddrin->sin_family      = AF_INET;
                psockaddrin->sin_addr.s_addr = (pipaddr->addr | (~pnetmask->addr));
            
            } else {
                psockaddrin->sin_len         = sizeof(struct sockaddr_in);
                psockaddrin->sin_family      = AF_INET;
                psockaddrin->sin_addr.s_addr = netif_ip4_gw(pnetif)->addr;
            }
            
            prouteq = &pafroute->ROUTE_rtq;
            _list_mono_free_seq(&prouteq->RTQ_pmonoHeader,
                                &prouteq->RTQ_pmonoTail,
                                &prouten->RTM_monoManage);
            prouteq->RTQ_stTotal += prouten->RTM_stLen;
            
            __routeUpdateReader(pafroute, ERROR_NONE);
        }
    }
    __AF_ROUTE_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: route_hook_netif_ipv6
** 功能描述: 网络接口添加 / 删除一条 IPv6 地址回调
** 输　入  : pnetif         网络接口
**           pip6addr       地址
**           type           RTM_DELADDR / RTM_ADDADDR
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LWIP_IPV6

VOID  route_hook_netif_ipv6 (struct netif *pnetif, const ip6_addr_t *pip6addr, u_char type)
{
    size_t               stMsgLen;
    struct sockaddr_in6 *psockaddrin6;
    struct sockaddr_dl  *psockaddrdl;
    struct ifa_msghdr   *pmsghdr;
    int                  iFlags;
    ip6_addr_t           ip6addr;
    
    PLW_LIST_LINE        pline;
    AF_ROUTE_T          *pafroute;
    AF_ROUTE_Q          *prouteq;
    AF_ROUTE_N          *prouten;
    
    if (!_G_plineAfRoute) {
        return;
    }
    
    iFlags   = netif_get_flags(pnetif);
    stMsgLen = sizeof(struct ifa_msghdr) 
             + (SO_ROUND_UP(sizeof(struct sockaddr_in6)) * 3)
             + SO_ROUND_UP(sizeof(struct sockaddr_dl));
             
    __AF_ROUTE_LOCK();
    for (pline = _G_plineAfRoute; pline != LW_NULL; pline = _list_line_get_next(pline)) {
        pafroute = _LIST_ENTRY(pline, AF_ROUTE_T, ROUTE_lineManage);
        if (((pafroute->ROUTE_rtq.RTQ_stTotal + stMsgLen) <= pafroute->ROUTE_stMaxBufSize) &&
            !(pafroute->ROUTE_iShutDFlag & __AF_ROUTE_SHUTD_R)) {
            prouten = (AF_ROUTE_N *)mem_malloc(sizeof(AF_ROUTE_N) + stMsgLen);
            if (!prouten) {
                break;
            }
            
            prouten->RTM_stLen = stMsgLen;
            pmsghdr = (struct ifa_msghdr *)(prouten + 1);
            
            pmsghdr->ifam_msglen  = (u_short)stMsgLen;
            pmsghdr->ifam_version = RTM_VERSION;
            pmsghdr->ifam_type    = type;
            
            pmsghdr->ifam_addrs  = RTA_NETMASK | RTA_IFP | RTA_IFA | RTA_BRD;
            pmsghdr->ifam_flags  = iFlags;
            pmsghdr->ifam_index  = netif_get_index(pnetif);
            pmsghdr->ifam_metric = 0;
            
            psockaddrin6 = (struct sockaddr_in6 *)(pmsghdr + 1);
            psockaddrin6->sin6_len    = sizeof(struct sockaddr_in6);
            psockaddrin6->sin6_family = AF_INET6;
            rt6_netmask_from_prefix(&ip6addr, 64);
            inet6_addr_from_ip6addr(&psockaddrin6->sin6_addr, &ip6addr);
            
            psockaddrdl = SA_NEXT(struct sockaddr_dl *, psockaddrin6);
            lib_bzero(psockaddrdl, sizeof(struct sockaddr_dl));
            rt6_build_sockaddr_dl(psockaddrdl, pnetif);
            
            psockaddrin6 = SA_NEXT(struct sockaddr_in6 *, psockaddrdl);
            psockaddrin6->sin6_len    = sizeof(struct sockaddr_in6);
            psockaddrin6->sin6_family = AF_INET6;
            inet6_addr_from_ip6addr(&psockaddrin6->sin6_addr, pip6addr);
            
            psockaddrin6 = SA_NEXT(struct sockaddr_in6 *, psockaddrin6);
            psockaddrin6->sin6_len    = sizeof(struct sockaddr_in6);
            psockaddrin6->sin6_family = AF_INET6;
            IP6_ADDR(&ip6addr, 0xff, 0, 0, 0);
            inet6_addr_from_ip6addr(&psockaddrin6->sin6_addr, &ip6addr);
            
            prouteq = &pafroute->ROUTE_rtq;
            _list_mono_free_seq(&prouteq->RTQ_pmonoHeader,
                                &prouteq->RTQ_pmonoTail,
                                &prouten->RTM_monoManage);
            prouteq->RTQ_stTotal += prouten->RTM_stLen;
            
            __routeUpdateReader(pafroute, ERROR_NONE);
        }
    }
    __AF_ROUTE_UNLOCK();
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** 函数名称: route_hook_maddr_ipv4
** 功能描述: 网络接口 增加 / 删除 一条 IPv4 组播地址
** 输　入  : pnetif    网络接口
**           pipaddr   组播地址
**           type      RTM_NEWMADDR / RTM_DELMADDR
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  route_hook_maddr_ipv4 (struct netif *pnetif, const ip4_addr_t *pipaddr, u_char type)
{
    size_t              stMsgLen;
    struct sockaddr_in *psockaddrin;
    struct sockaddr_dl *psockaddrdl;
    struct ifma_msghdr *pmsghdr;
    INT                 iFlags;
    
    PLW_LIST_LINE       pline;
    AF_ROUTE_T         *pafroute;
    AF_ROUTE_Q         *prouteq;
    AF_ROUTE_N         *prouten;
    
    if (!_G_plineAfRoute) {
        return;
    }
    
    iFlags   = netif_get_flags(pnetif);
    stMsgLen = sizeof(struct ifma_msghdr) 
             + SO_ROUND_UP(sizeof(struct sockaddr_in))
             + SO_ROUND_UP(sizeof(struct sockaddr_dl));
    
    __AF_ROUTE_LOCK();
    for (pline = _G_plineAfRoute; pline != LW_NULL; pline = _list_line_get_next(pline)) {
        pafroute = _LIST_ENTRY(pline, AF_ROUTE_T, ROUTE_lineManage);
        if (((pafroute->ROUTE_rtq.RTQ_stTotal + stMsgLen) <= pafroute->ROUTE_stMaxBufSize) &&
            !(pafroute->ROUTE_iShutDFlag & __AF_ROUTE_SHUTD_R)) {
            prouten = (AF_ROUTE_N *)mem_malloc(sizeof(AF_ROUTE_N) + stMsgLen);
            if (!prouten) {
                break;
            }
            
            prouten->RTM_stLen = stMsgLen;
            pmsghdr = (struct ifma_msghdr *)(prouten + 1);
            
            pmsghdr->ifmam_msglen  = (u_short)stMsgLen;
            pmsghdr->ifmam_version = RTM_VERSION;
            pmsghdr->ifmam_type    = type;
            
            pmsghdr->ifmam_addrs = RTA_DST | RTA_IFP;
            pmsghdr->ifmam_flags = iFlags;
            pmsghdr->ifmam_index = netif_get_index(pnetif);
            
            psockaddrin = (struct sockaddr_in *)(pmsghdr + 1);
            psockaddrin->sin_len         = sizeof(struct sockaddr_in);
            psockaddrin->sin_family      = AF_INET;
            psockaddrin->sin_addr.s_addr = pipaddr->addr;
            
            psockaddrdl = SA_NEXT(struct sockaddr_dl *, psockaddrin);
            lib_bzero(psockaddrdl, sizeof(struct sockaddr_dl));
            rt_build_sockaddr_dl(psockaddrdl, pnetif);
            
            prouteq = &pafroute->ROUTE_rtq;
            _list_mono_free_seq(&prouteq->RTQ_pmonoHeader,
                                &prouteq->RTQ_pmonoTail,
                                &prouten->RTM_monoManage);
            prouteq->RTQ_stTotal += prouten->RTM_stLen;
            
            __routeUpdateReader(pafroute, ERROR_NONE);
        }
    }
    __AF_ROUTE_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: route_hook_maddr_ipv6
** 功能描述: 网络接口 增加 / 删除 一条 IPv6 组播地址
** 输　入  : pnetif    网络接口
**           pip6addr  组播地址
**           type      RTM_NEWMADDR / RTM_DELMADDR
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LWIP_IPV6

VOID  route_hook_maddr_ipv6 (struct netif *pnetif, const ip6_addr_t *pip6addr, u_char type)
{
    size_t               stMsgLen;
    struct sockaddr_in6 *psockaddrin6;
    struct sockaddr_dl  *psockaddrdl;
    struct ifma_msghdr  *pmsghdr;
    INT                  iFlags;
    
    PLW_LIST_LINE        pline;
    AF_ROUTE_T          *pafroute;
    AF_ROUTE_Q          *prouteq;
    AF_ROUTE_N          *prouten;
    
    if (!_G_plineAfRoute) {
        return;
    }
    
    iFlags   = netif_get_flags(pnetif);
    stMsgLen = sizeof(struct ifma_msghdr) 
             + SO_ROUND_UP(sizeof(struct sockaddr_in6))
             + SO_ROUND_UP(sizeof(struct sockaddr_dl));
    
    __AF_ROUTE_LOCK();
    for (pline = _G_plineAfRoute; pline != LW_NULL; pline = _list_line_get_next(pline)) {
        pafroute = _LIST_ENTRY(pline, AF_ROUTE_T, ROUTE_lineManage);
        if (((pafroute->ROUTE_rtq.RTQ_stTotal + stMsgLen) <= pafroute->ROUTE_stMaxBufSize) &&
            !(pafroute->ROUTE_iShutDFlag & __AF_ROUTE_SHUTD_R)) {
            prouten = (AF_ROUTE_N *)mem_malloc(sizeof(AF_ROUTE_N) + stMsgLen);
            if (!prouten) {
                break;
            }
            
            prouten->RTM_stLen = stMsgLen;
            pmsghdr = (struct ifma_msghdr *)(prouten + 1);
            
            pmsghdr->ifmam_msglen  = (u_short)stMsgLen;
            pmsghdr->ifmam_version = RTM_VERSION;
            pmsghdr->ifmam_type    = type;
            
            pmsghdr->ifmam_addrs = RTA_DST | RTA_IFP;
            pmsghdr->ifmam_flags = iFlags;
            pmsghdr->ifmam_index = netif_get_index(pnetif);
            
            psockaddrin6 = (struct sockaddr_in6 *)(pmsghdr + 1);
            psockaddrin6->sin6_len    = sizeof(struct sockaddr_in6);
            psockaddrin6->sin6_family = AF_INET6;
            inet6_addr_from_ip6addr(&psockaddrin6->sin6_addr, pip6addr);
            
            psockaddrdl = SA_NEXT(struct sockaddr_dl *, psockaddrin6);
            lib_bzero(psockaddrdl, sizeof(struct sockaddr_dl));
            rt6_build_sockaddr_dl(psockaddrdl, pnetif);
            
            prouteq = &pafroute->ROUTE_rtq;
            _list_mono_free_seq(&prouteq->RTQ_pmonoHeader,
                                &prouteq->RTQ_pmonoTail,
                                &prouten->RTM_monoManage);
            prouteq->RTQ_stTotal += prouten->RTM_stLen;
            
            __routeUpdateReader(pafroute, ERROR_NONE);
        }
    }
    __AF_ROUTE_UNLOCK();
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** 函数名称: route_hook_netif_ann
** 功能描述: 增加 / 删除 网络接口
** 输　入  : pnetif    网络接口
**           what      IFAN_ARRIVAL / IFAN_DEPARTURE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  route_hook_netif_ann (struct netif *pnetif, int what)
{
    size_t                    stMsgLen;
    struct if_announcemsghdr *pmsghdr;
    
    PLW_LIST_LINE             pline;
    AF_ROUTE_T               *pafroute;
    AF_ROUTE_Q               *prouteq;
    AF_ROUTE_N               *prouten;
    
    if (!_G_plineAfRoute) {
        return;
    }
    
    stMsgLen = sizeof(struct if_announcemsghdr);
    
    __AF_ROUTE_LOCK();
    for (pline = _G_plineAfRoute; pline != LW_NULL; pline = _list_line_get_next(pline)) {
        pafroute = _LIST_ENTRY(pline, AF_ROUTE_T, ROUTE_lineManage);
        if (((pafroute->ROUTE_rtq.RTQ_stTotal + stMsgLen) <= pafroute->ROUTE_stMaxBufSize) &&
            !(pafroute->ROUTE_iShutDFlag & __AF_ROUTE_SHUTD_R)) {
            prouten = (AF_ROUTE_N *)mem_malloc(sizeof(AF_ROUTE_N) + stMsgLen);
            if (!prouten) {
                break;
            }
            
            prouten->RTM_stLen = stMsgLen;
            pmsghdr = (struct if_announcemsghdr *)(prouten + 1);
            
            pmsghdr->ifan_msglen  = (u_short)stMsgLen;
            pmsghdr->ifan_version = RTM_VERSION;
            pmsghdr->ifan_type    = RTM_IFANNOUNCE;
            
            pmsghdr->ifan_index = netif_get_index(pnetif);
            pmsghdr->ifan_what  = (u_short)what;
            
            netif_get_name(pnetif, pmsghdr->ifan_name);
            
            prouteq = &pafroute->ROUTE_rtq;
            _list_mono_free_seq(&prouteq->RTQ_pmonoHeader,
                                &prouteq->RTQ_pmonoTail,
                                &prouten->RTM_monoManage);
            prouteq->RTQ_stTotal += prouten->RTM_stLen;
            
            __routeUpdateReader(pafroute, ERROR_NONE);
        }
    }
    __AF_ROUTE_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: route_hook_netif_updown
** 功能描述: 启动 / 停止 / 断线 / 链接网络接口
** 输　入  : pnetif    网络接口
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  route_hook_netif_updown (struct netif *pnetif)
{
#define MIB2_NETIF(netif)   (&((netif)->mib2_counters))

    size_t              stMsgLen;
    struct if_msghdr   *pmsghdr;
    struct sockaddr_dl *psockaddrdl;
    struct if_data     *pifdata;
    struct timespec     ts;
    INT                 iFlags;
    
    PLW_LIST_LINE       pline;
    AF_ROUTE_T         *pafroute;
    AF_ROUTE_Q         *prouteq;
    AF_ROUTE_N         *prouten;
    
    if (!_G_plineAfRoute) {
        return;
    }
    
    iFlags   = netif_get_flags(pnetif);
    stMsgLen = sizeof(struct if_msghdr) +
             + SO_ROUND_UP(sizeof(struct sockaddr_dl));
    
    __AF_ROUTE_LOCK();
    for (pline = _G_plineAfRoute; pline != LW_NULL; pline = _list_line_get_next(pline)) {
        pafroute = _LIST_ENTRY(pline, AF_ROUTE_T, ROUTE_lineManage);
        if (((pafroute->ROUTE_rtq.RTQ_stTotal + stMsgLen) <= pafroute->ROUTE_stMaxBufSize) &&
            !(pafroute->ROUTE_iShutDFlag & __AF_ROUTE_SHUTD_R)) {
            prouten = (AF_ROUTE_N *)mem_malloc(sizeof(AF_ROUTE_N) + stMsgLen);
            if (!prouten) {
                break;
            }
            
            prouten->RTM_stLen = stMsgLen;
            pmsghdr = (struct if_msghdr *)(prouten + 1);
            
            pmsghdr->ifm_msglen  = (u_short)stMsgLen;
            pmsghdr->ifm_version = RTM_VERSION;
            pmsghdr->ifm_type    = RTM_IFINFO;
            
            pmsghdr->ifm_addrs = RTA_IFP;
            pmsghdr->ifm_flags = iFlags;
            pmsghdr->ifm_index = netif_get_index(pnetif);
            
            pifdata = &pmsghdr->ifm_data;
            lib_bzero(pifdata, sizeof(struct if_data));
            
            if (pnetif->flags & NETIF_FLAG_ETHERNET) {
                pifdata->ifi_type    = IFT_ETHER;
                pifdata->ifi_addrlen = ETH_ALEN;
                pifdata->ifi_hdrlen  = ETH_HLEN;
                pifdata->ifi_mtu     = ETH_DATA_LEN;

            } else if (!(pnetif->flags & NETIF_FLAG_BROADCAST)) {
                pifdata->ifi_type    = IFT_PPP;
                pifdata->ifi_addrlen = 0;
                pifdata->ifi_hdrlen  = 4;
                pifdata->ifi_mtu     = 1500;
                
            } else {
                pifdata->ifi_type = IFT_OTHER;
            }
            
            pifdata->ifi_baudrate = pnetif->link_speed;
            pifdata->ifi_ipackets = MIB2_NETIF(pnetif)->ifinucastpkts + MIB2_NETIF(pnetif)->ifinnucastpkts;
            pifdata->ifi_ierrors  = MIB2_NETIF(pnetif)->ifinerrors;
            pifdata->ifi_opackets = MIB2_NETIF(pnetif)->ifoutucastpkts + MIB2_NETIF(pnetif)->ifoutnucastpkts;
            pifdata->ifi_oerrors  = MIB2_NETIF(pnetif)->ifouterrors;
            pifdata->ifi_ibytes   = MIB2_NETIF(pnetif)->ifinoctets;
            pifdata->ifi_obytes   = MIB2_NETIF(pnetif)->ifoutoctets;
            pifdata->ifi_imcasts  = MIB2_NETIF(pnetif)->ifinnucastpkts;
            pifdata->ifi_omcasts  = MIB2_NETIF(pnetif)->ifoutnucastpkts;
            pifdata->ifi_iqdrops  = MIB2_NETIF(pnetif)->ifindiscards;
            pifdata->ifi_noproto  = MIB2_NETIF(pnetif)->ifinunknownprotos;
            
            lib_clock_gettime(CLOCK_REALTIME, &ts);
            pifdata->ifi_lastchange.tv_sec  = ts.tv_sec;
            pifdata->ifi_lastchange.tv_usec = ts.tv_nsec / 1000;
            
            psockaddrdl = (struct sockaddr_dl *)(pmsghdr + 1);
            lib_bzero(psockaddrdl, sizeof(struct sockaddr_dl));
            rt_build_sockaddr_dl(psockaddrdl, pnetif);

            prouteq = &pafroute->ROUTE_rtq;
            _list_mono_free_seq(&prouteq->RTQ_pmonoHeader,
                                &prouteq->RTQ_pmonoTail,
                                &prouten->RTM_monoManage);
            prouteq->RTQ_stTotal += prouten->RTM_stLen;
            
            __routeUpdateReader(pafroute, ERROR_NONE);
        }
    }
    __AF_ROUTE_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: route_init
** 功能描述: 初始化 route 域协议
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  route_init (VOID)
{
}
/*********************************************************************************************************
** 函数名称: route_socket
** 功能描述: route socket
** 输　入  : iDomain        域, 必须是 AF_ROUTE
**           iType          必须是 SOCK_RAW
**           iProtocol      协议
** 输　出  : unix socket
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
AF_ROUTE_T  *route_socket (INT  iDomain, INT  iType, INT  iProtocol)
{
    AF_ROUTE_T   *pafroute;
    
    if ((iDomain != AF_ROUTE) || (iType != SOCK_RAW)) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }
    
    pafroute = (AF_ROUTE_T *)__SHEAP_ALLOC(sizeof(AF_ROUTE_T));
    if (pafroute == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    lib_bzero(pafroute, sizeof(AF_ROUTE_T));
    
    pafroute->ROUTE_iFlag         = O_RDWR;
    pafroute->ROUTE_ulRecvTimeout = LW_OPTION_WAIT_INFINITE;
    pafroute->ROUTE_stMaxBufSize  = (LW_CFG_KB_SIZE * 64);
    
    pafroute->ROUTE_hCanRead = API_SemaphoreBCreate("route_rlock", LW_FALSE, 
                                                    LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (pafroute->ROUTE_hCanRead == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pafroute);
        return  (LW_NULL);
    }
    
    __AF_ROUTE_LOCK();
    _List_Line_Add_Ahead(&pafroute->ROUTE_lineManage, &_G_plineAfRoute);
    __AF_ROUTE_UNLOCK();
    
    return  (pafroute);
}
/*********************************************************************************************************
** 函数名称: route_close
** 功能描述: close
** 输　入  : pafroute  route file
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  route_close (AF_ROUTE_T  *pafroute)
{
    if (pafroute) {
        __AF_ROUTE_LOCK();
        __routeDeleteBuf(pafroute);
        __routeUpdateReader(pafroute, ENOTCONN);
        _List_Line_Del(&pafroute->ROUTE_lineManage, &_G_plineAfRoute);
        __AF_ROUTE_UNLOCK();
        
        __SHEAP_FREE(pafroute);
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: route_send
** 功能描述: 写路由表
** 输　入  : pafroute  route file
**           data      send buffer
**           size      send len
**           flags     flag
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t  route_send (AF_ROUTE_T  *pafroute, const void *data, size_t size, int flags)
{
    ssize_t             sstSend = 0;
    BOOL                bNeedUpdateReader = LW_FALSE;
    struct rt_msghdr   *pmsghdr;
    struct sockaddr    *psockaddr;

    if (!data || !size) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __AF_ROUTE_LOCK();
    if (pafroute->ROUTE_iShutDFlag & __AF_ROUTE_SHUTD_W) {
        __AF_ROUTE_UNLOCK();
        _ErrorHandle(ESHUTDOWN);                                        /*  本地已经关闭                */
        return  (sstSend);
    }

    pmsghdr   = (struct rt_msghdr *)data;
    psockaddr = (struct sockaddr *)(pmsghdr + 1);
    
    switch (pmsghdr->rtm_type) {
    
    case RTM_ADD:
        if (psockaddr->sa_family == AF_INET) {
            sstSend = __routeRtmAdd4(pmsghdr);
            
#if LWIP_IPV6
        } else if (psockaddr->sa_family == AF_INET6) {
            sstSend = __routeRtmAdd6(pmsghdr);
#endif
        } else {
            _ErrorHandle(EAFNOSUPPORT);
        }
        break;
        
    case RTM_CHANGE:
        if (psockaddr->sa_family == AF_INET) {
            sstSend = __routeRtmChange4(pmsghdr);
            
#if LWIP_IPV6
        } else if (psockaddr->sa_family == AF_INET6) {
            sstSend = __routeRtmChange6(pmsghdr);
#endif
        } else {
            _ErrorHandle(EAFNOSUPPORT);
        }
        break;
        
    case RTM_DELETE:
        if (psockaddr->sa_family == AF_INET) {
            sstSend = __routeRtmDelete4(pmsghdr);
            
#if LWIP_IPV6
        } else if (psockaddr->sa_family == AF_INET6) {
            sstSend = __routeRtmDelete6(pmsghdr);
#endif
        } else {
            _ErrorHandle(EAFNOSUPPORT);
        }
        break;
        
    case RTM_GET:
        if (psockaddr->sa_family == AF_INET) {
            sstSend = __routeRtmGet4(pmsghdr);
            
#if LWIP_IPV6
        } else if (psockaddr->sa_family == AF_INET6) {
            sstSend = __routeRtmGet6(pmsghdr);
#endif
        } else {
            _ErrorHandle(EAFNOSUPPORT);
        }
        break;
        
    default:
        _ErrorHandle(ENOSYS);                                           /*  系统未实现                  */
        break;
    }
    
    if (bNeedUpdateReader) {
        __routeUpdateReader(pafroute, ERROR_NONE);                      /*  update remote reader        */
    }
    __AF_ROUTE_UNLOCK();
    
    return  (sstSend);
}
/*********************************************************************************************************
** 函数名称: route_sendmsg
** 功能描述: sendmsg
** 输　入  : pafroute  route file
**           msg       消息
**           flags     flag
** 输　出  : NUM 数据长度
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t  route_sendmsg (AF_ROUTE_T  *pafroute, const struct msghdr *msg, int flags)
{
    ssize_t     sstSendLen;
    ssize_t     sstTotal;
    
    if (!msg) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (msg->msg_iovlen == 1) {
        sstTotal = route_send(pafroute, msg->msg_iov->iov_base, msg->msg_iov->iov_len, flags);
        
    } else {
        int             i;
        struct iovec   *msg_iov;
        size_t          msg_iovlen;
        
        msg_iov    = msg->msg_iov;
        msg_iovlen = msg->msg_iovlen;
    
        for (i = 0; i < msg_iovlen; i++) {
            if ((msg_iov[i].iov_len == 0) || (msg_iov[i].iov_base == LW_NULL)) {
                _ErrorHandle(EINVAL);
                return  (PX_ERROR);
            }
        }
        
        for (i = 0, sstTotal = 0; i < msg_iovlen; i++) {
            sstSendLen = route_send(pafroute, msg_iov[i].iov_base, msg_iov[i].iov_len, flags);
            if (sstSendLen != msg_iov[i].iov_len) {
                return  (sstTotal);
            }
            sstTotal += sstSendLen;
        }
    }
    
    return  (sstTotal);
}
/*********************************************************************************************************
** 函数名称: route_recv
** 功能描述: 读路由表
** 输　入  : pafroute  route file
**           mem       buffer
**           len       buffer len
**           flags     flag
** 输　出  : NUM
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t  route_recv (AF_ROUTE_T  *pafroute, void *mem, size_t len, int flags)
{
    ssize_t             sstRecv = 0;
    ULONG               ulError;
    AF_ROUTE_N         *prouten;
    AF_ROUTE_Q         *prouteq;
    struct rt_msghdr   *pmsghdr;
    
    if (!mem || !len) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (flags & MSG_WAITALL) {                                          /*  目前这是不允许的            */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __AF_ROUTE_LOCK();
    do {
        if (pafroute->ROUTE_iShutDFlag & __AF_ROUTE_SHUTD_R) {
            __AF_ROUTE_UNLOCK();
            _ErrorHandle(ENOTCONN);                                     /*  本地已经关闭                */
            return  (sstRecv);
        }
    
        prouteq = &pafroute->ROUTE_rtq;
        
        if (prouteq->RTQ_pmonoHeader) {
            prouten = (AF_ROUTE_N *)prouteq->RTQ_pmonoHeader;
            if (prouten->RTM_stLen > len) {
                __AF_ROUTE_UNLOCK();
                _ErrorHandle(ENOBUFS);                                  /*  缓冲区太小                  */
                return  (sstRecv);
            }
            
            pmsghdr = (struct rt_msghdr *)(prouten + 1);
            lib_memcpy(mem, pmsghdr, prouten->RTM_stLen);
            sstRecv = (ssize_t)prouten->RTM_stLen;
            
            if (!(flags & MSG_PEEK)) {
                _list_mono_allocate_seq(&prouteq->RTQ_pmonoHeader,
                                        &prouteq->RTQ_pmonoTail);
                prouteq->RTQ_stTotal -= prouten->RTM_stLen;
                mem_free(prouten);
            }
            break;
        
        } else {
            if (__AF_ROUTE_IS_NBIO(pafroute, flags)) {                  /*  非阻塞 IO                   */
                __AF_ROUTE_UNLOCK();
                _ErrorHandle(EWOULDBLOCK);                              /*  需要重新读                  */
                return  (sstRecv);
            }
            
            __AF_ROUTE_UNLOCK();
            ulError = __AF_ROUTE_WAIT(pafroute);                        /*  等待数据                    */
            if (ulError) {
                _ErrorHandle(EWOULDBLOCK);                              /*  等待超时                    */
                return  (sstRecv);
            }
            __AF_ROUTE_LOCK();
        }
    } while (1);
    __AF_ROUTE_UNLOCK();
    
    return  (sstRecv);
}
/*********************************************************************************************************
** 函数名称: route_recvmsg
** 功能描述: recvmsg
** 输　入  : pafroute  route file
**           msg       消息
**           flags     flag
** 输　出  : NUM 数据长度
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t  route_recvmsg (AF_ROUTE_T  *pafroute, struct msghdr *msg, int flags)
{
    ssize_t     sstRecvLen;
    ssize_t     sstTotal;

    if (!msg) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (msg->msg_iovlen == 1) {
        sstTotal = route_recv(pafroute, msg->msg_iov->iov_base, msg->msg_iov->iov_len, flags);
        
    } else {
        int             i;
        struct iovec   *msg_iov;
        size_t          msg_iovlen;
        
        msg_iov    = msg->msg_iov;
        msg_iovlen = msg->msg_iovlen;
    
        for (i = 0; i < msg_iovlen; i++) {
            if ((msg_iov[i].iov_len == 0) || (msg_iov[i].iov_base == LW_NULL)) {
                _ErrorHandle(EINVAL);
                return  (PX_ERROR);
            }
        }
        
        for (i = 0, sstTotal = 0; i < msg_iovlen; i++) {
            sstRecvLen = route_recv(pafroute, msg_iov[i].iov_base, msg_iov[i].iov_len, flags);
            if (sstRecvLen != msg_iov[i].iov_len) {
                return  (sstTotal);
            }
            sstTotal += sstRecvLen;
        }
    }
    
    return  (sstTotal);
}
/*********************************************************************************************************
** 函数名称: route_setsockopt
** 功能描述: setsockopt
** 输　入  : pafroute  route file
**           level     level
**           optname   option
**           optval    option value
**           optlen    option value len
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  route_setsockopt (AF_ROUTE_T  *pafroute, int level, int optname, const void *optval, socklen_t optlen)
{
    INT   iRet = PX_ERROR;
    
    if (!optval || optlen < sizeof(INT)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __AF_ROUTE_LOCK();
    switch (level) {
    
    case SOL_SOCKET:
        switch (optname) {
        
        case SO_RCVBUF:
            pafroute->ROUTE_stMaxBufSize = *(INT *)optval;
            if (pafroute->ROUTE_stMaxBufSize < __AF_ROUTE_DEF_BUFMIN) {
                pafroute->ROUTE_stMaxBufSize = __AF_ROUTE_DEF_BUFMIN;
            } else if (pafroute->ROUTE_stMaxBufSize > __AF_ROUTE_DEF_BUFMAX) {
                pafroute->ROUTE_stMaxBufSize = __AF_ROUTE_DEF_BUFMAX;
            }
            iRet = ERROR_NONE;
            break;
            
        case SO_RCVTIMEO:
            if (optlen == sizeof(struct timeval)) {
                pafroute->ROUTE_ulRecvTimeout = __routeTvToTicks((struct timeval *)optval);
            } else {
                pafroute->ROUTE_ulRecvTimeout = __routeMsToTicks(*(INT *)optval);
            }
            iRet = ERROR_NONE;
            break;
    
        default:
            _ErrorHandle(ENOSYS);
            break;
        }
        break;
        
    default:
        _ErrorHandle(ENOPROTOOPT);
        break;
    }
    __AF_ROUTE_UNLOCK();
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: route_getsockopt
** 功能描述: getsockopt
** 输　入  : pafroute  route file
**           level     level
**           optname   option
**           optval    option value
**           optlen    option value len
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  route_getsockopt (AF_ROUTE_T  *pafroute, int level, int optname, void *optval, socklen_t *optlen)
{
    INT   iRet = PX_ERROR;
    
    if (!optval || *optlen < sizeof(INT)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __AF_ROUTE_LOCK();
    switch (level) {
    
    case SOL_SOCKET:
        switch (optname) {
        
        case SO_RCVBUF:
            *(INT *)optval = pafroute->ROUTE_stMaxBufSize;
            iRet = ERROR_NONE;
            break;
            
        case SO_RCVTIMEO:
            if (*optlen == sizeof(struct timeval)) {
                __routeTicksToTv(pafroute->ROUTE_ulRecvTimeout, (struct timeval *)optval);
            } else {
                *(INT *)optval = (INT)__routeTicksToMs(pafroute->ROUTE_ulRecvTimeout);
            }
            iRet = ERROR_NONE;
            break;
            
        default:
            _ErrorHandle(ENOSYS);
            break;
        }
        break;
        
    default:
        _ErrorHandle(ENOPROTOOPT);
        break;
    }
    __AF_ROUTE_UNLOCK();
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: route_shutdown
** 功能描述: shutdown
** 输　入  : pafroute  route file
**           how       SHUT_RD  SHUT_WR  SHUT_RDWR
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  route_shutdown (AF_ROUTE_T  *pafroute, int how)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: route_ioctl
** 功能描述: ioctl
** 输　入  : pafunix   unix file
**           iCmd      命令
**           pvArg     参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  route_ioctl (AF_ROUTE_T  *pafroute, INT  iCmd, PVOID  pvArg)
{
    INT     iRet = ERROR_NONE;
    
    switch (iCmd) {
    
    case FIOGETFL:
        if (pvArg) {
            *(INT *)pvArg = pafroute->ROUTE_iFlag;
        }
        break;
        
    case FIOSETFL:
        if ((INT)pvArg & O_NONBLOCK) {
            pafroute->ROUTE_iFlag |= O_NONBLOCK;
        } else {
            pafroute->ROUTE_iFlag &= ~O_NONBLOCK;
        }
        break;
        
    case FIONREAD:
        if (pvArg) {
            *(INT *)pvArg = (INT)pafroute->ROUTE_rtq.RTQ_stTotal;
        }
        break;
        
    case FIONBIO:
        if (pvArg && *(INT *)pvArg) {
            pafroute->ROUTE_iFlag |= O_NONBLOCK;
        } else {
            pafroute->ROUTE_iFlag &= ~O_NONBLOCK;
        }
        break;
        
    default:
        _ErrorHandle(ENOSYS);
        iRet = PX_ERROR;
        break;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __route_have_event
** 功能描述: 检测对应的控制块是否可读
** 输　入  : pafunix   unix file
**           type      事件类型
**           piSoErr   如果等待的事件有效则更新 SO_ERROR
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int __route_have_event (AF_ROUTE_T *pafroute, int type, int  *piSoErr)
{
    INT     iEvent = 0;

    switch (type) {

    case SELREAD:                                                       /*  是否可读                    */
        __AF_ROUTE_LOCK();
        if (pafroute->ROUTE_iFlag & __AF_ROUTE_SHUTD_R) {
            *piSoErr = ENOTCONN;                                        /*  读已经被停止了              */
            iEvent   = 1;
        
        } else if (pafroute->ROUTE_rtq.RTQ_pmonoHeader) {
            *piSoErr = ERROR_NONE;
            iEvent   = 1;
        }
        __AF_ROUTE_UNLOCK();
        break;
        
    case SELWRITE:                                                      /*  是否可写                    */
        __AF_ROUTE_LOCK();
        if (pafroute->ROUTE_iFlag & __AF_ROUTE_SHUTD_W) {
            *piSoErr = ESHUTDOWN;                                       /*  写已经被停止了              */
            
        } else {
            *piSoErr = ERROR_NONE;
        }
        iEvent = 1;
        __AF_ROUTE_UNLOCK();
        break;
        
    case SELEXCEPT:                                                     /*  是否异常                    */
        break;
    }
    
    return  (iEvent);
}

#endif                                                                  /*  LW_CFG_NET_EN               */
                                                                        /*  LW_CFG_NET_ROUTE_EN > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/

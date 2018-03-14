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
** 文   件   名: lwip_srtctl.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 12 月 08 日
**
** 描        述: ioctl 源路由表支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_ROUTER > 0 && LW_CFG_NET_BALANCING > 0
#include "sys/socket.h"
#include "net/sroute.h"
#include "lwip/mem.h"
#include "lwip/tcpip.h"
#include "balancing/ip4_sroute.h"
#include "balancing/ip6_sroute.h"
/*********************************************************************************************************
** 函数名称: __srtAdd4
** 功能描述: 添加一条 IPv4 源路由信息
** 输　入  : psrtentry  源路由信息
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __srtAdd4 (const struct srtentry  *psrtentry)
{
    INT                 iRet;
    struct srt_entry   *psentry4;
    
    if ((psrtentry->srt_mode != SRT_MODE_EXCLUDE) &&
        (psrtentry->srt_mode != SRT_MODE_INCLUDE)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((psrtentry->srt_prio != SRT_PRIO_HIGH) &&
        (psrtentry->srt_prio != SRT_PRIO_DEFAULT)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    SRT_LOCK();
    psentry4 = (struct srt_entry *)mem_malloc(sizeof(struct srt_entry));
    if (!psentry4) {
        SRT_UNLOCK();
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    lib_bzero(psentry4, sizeof(struct srt_entry));
    
    srt_srtentry_to_sentry(psentry4, psrtentry);                        /*  转换为 srt_entry            */
    
    iRet = srt_add_entry(psentry4);
    SRT_UNLOCK();
    if (iRet < 0) {
        mem_free(psentry4);
        _ErrorHandle(ENETUNREACH);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __srtAdd6
** 功能描述: 添加一条 IPv6 源路由信息
** 输　入  : psrtentry  源路由信息
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LWIP_IPV6

static INT  __srtAdd6 (const struct srtentry  *psrtentry)
{
    INT                 iRet;
    struct srt6_entry  *psentry6;
    
    if ((psrtentry->srt_mode != SRT_MODE_EXCLUDE) &&
        (psrtentry->srt_mode != SRT_MODE_INCLUDE)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((psrtentry->srt_prio != SRT_PRIO_HIGH) &&
        (psrtentry->srt_prio != SRT_PRIO_DEFAULT)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    SRT_LOCK();
    psentry6 = (struct srt6_entry *)mem_malloc(sizeof(struct srt6_entry));
    if (!psentry6) {
        SRT_UNLOCK();
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    lib_bzero(psentry6, sizeof(struct srt6_entry));
    
    srt6_srtentry_to_sentry(psentry6, psrtentry);                       /*  转换为 srt_entry            */
    
    iRet = srt6_add_entry(psentry6);
    SRT_UNLOCK();
    if (iRet < 0) {
        mem_free(psentry6);
        _ErrorHandle(ENETUNREACH);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** 函数名称: __srtDel4
** 功能描述: 删除一条 IPv4 源路由信息
** 输　入  : psrtentry  源路由信息
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __srtDel4 (const struct srtentry  *psrtentry)
{
    struct srt_entry   *psentry4;
    ip4_addr_t          ipsaddr, ipeaddr;
    ip4_addr_t          ipsdest, ipedest;
    
    inet_addr_to_ip4addr(&ipsaddr, &((struct sockaddr_in *)&psrtentry->srt_ssrc)->sin_addr);
    inet_addr_to_ip4addr(&ipeaddr, &((struct sockaddr_in *)&psrtentry->srt_esrc)->sin_addr);
    inet_addr_to_ip4addr(&ipsdest, &((struct sockaddr_in *)&psrtentry->srt_sdest)->sin_addr);
    inet_addr_to_ip4addr(&ipedest, &((struct sockaddr_in *)&psrtentry->srt_edest)->sin_addr);
    
    SRT_LOCK();
    psentry4 = srt_find_entry(&ipsaddr, &ipeaddr, &ipsdest, &ipedest);
    if (!psentry4) {
        SRT_UNLOCK();
        _ErrorHandle(ENETUNREACH);
        return  (PX_ERROR);
    }
    srt_delete_entry(psentry4);
    SRT_UNLOCK();
    
    mem_free(psentry4);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __srtDel6
** 功能描述: 删除一条 IPv6 源路由信息
** 输　入  : psrtentry  源路由信息
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LWIP_IPV6

static INT  __srtDel6 (const struct srtentry  *psrtentry)
{
    struct srt6_entry  *psentry6;
    ip6_addr_t          ip6saddr, ip6eaddr;
    ip6_addr_t          ip6sdest, ip6edest;
    
    inet6_addr_to_ip6addr(&ip6saddr, &((struct sockaddr_in6 *)&psrtentry->srt_ssrc)->sin6_addr);
    inet6_addr_to_ip6addr(&ip6eaddr, &((struct sockaddr_in6 *)&psrtentry->srt_esrc)->sin6_addr);
    inet6_addr_to_ip6addr(&ip6sdest, &((struct sockaddr_in6 *)&psrtentry->srt_sdest)->sin6_addr);
    inet6_addr_to_ip6addr(&ip6edest, &((struct sockaddr_in6 *)&psrtentry->srt_edest)->sin6_addr);

    SRT_LOCK();
    psentry6 = srt6_find_entry(&ip6saddr, &ip6eaddr, &ip6sdest, &ip6edest);
    if (!psentry6) {
        SRT_UNLOCK();
        _ErrorHandle(ENETUNREACH);
        return  (PX_ERROR);
    }
    srt6_delete_entry(psentry6);
    SRT_UNLOCK();
    
    mem_free(psentry6);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** 函数名称: __srtChg4
** 功能描述: 修改一条 IPv4 源路由信息
** 输　入  : psrtentry  源路由信息
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __srtChg4 (const struct srtentry  *psrtentry)
{
    INT                 iRet;
    struct srt_entry   *psentry4;
    ip4_addr_t          ipsaddr, ipeaddr;
    ip4_addr_t          ipsdest, ipedest;
    
    if ((psrtentry->srt_mode != SRT_MODE_EXCLUDE) &&
        (psrtentry->srt_mode != SRT_MODE_INCLUDE)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((psrtentry->srt_prio != SRT_PRIO_HIGH) &&
        (psrtentry->srt_prio != SRT_PRIO_DEFAULT)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    inet_addr_to_ip4addr(&ipsaddr, &((struct sockaddr_in *)&psrtentry->srt_ssrc)->sin_addr);
    inet_addr_to_ip4addr(&ipeaddr, &((struct sockaddr_in *)&psrtentry->srt_esrc)->sin_addr);
    inet_addr_to_ip4addr(&ipsdest, &((struct sockaddr_in *)&psrtentry->srt_sdest)->sin_addr);
    inet_addr_to_ip4addr(&ipedest, &((struct sockaddr_in *)&psrtentry->srt_edest)->sin_addr);
    
    SRT_LOCK();
    psentry4 = srt_find_entry(&ipsaddr, &ipeaddr, &ipsdest, &ipedest);
    if (!psentry4) {
        SRT_UNLOCK();
        _ErrorHandle(ENETUNREACH);
        return  (PX_ERROR);
    }
    srt_delete_entry(psentry4);
    
    srt_srtentry_to_sentry(psentry4, psrtentry);                        /*  转换为 srt_entry            */
    
    iRet = srt_add_entry(psentry4);
    SRT_UNLOCK();
    if (iRet < 0) {
        mem_free(psentry4);
        _ErrorHandle(ENETUNREACH);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __srtChg6
** 功能描述: 添加一条 IPv6 源路由信息
** 输　入  : psrtentry  源路由信息
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LWIP_IPV6

static INT  __srtChg6 (const struct srtentry  *psrtentry)
{
    INT                 iRet;
    struct srt6_entry  *psentry6;
    ip6_addr_t          ip6saddr, ip6eaddr;
    ip6_addr_t          ip6sdest, ip6edest;
    
    if ((psrtentry->srt_mode != SRT_MODE_EXCLUDE) &&
        (psrtentry->srt_mode != SRT_MODE_INCLUDE)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((psrtentry->srt_prio != SRT_PRIO_HIGH) &&
        (psrtentry->srt_prio != SRT_PRIO_DEFAULT)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    inet6_addr_to_ip6addr(&ip6saddr, &((struct sockaddr_in6 *)&psrtentry->srt_ssrc)->sin6_addr);
    inet6_addr_to_ip6addr(&ip6eaddr, &((struct sockaddr_in6 *)&psrtentry->srt_esrc)->sin6_addr);
    inet6_addr_to_ip6addr(&ip6sdest, &((struct sockaddr_in6 *)&psrtentry->srt_sdest)->sin6_addr);
    inet6_addr_to_ip6addr(&ip6edest, &((struct sockaddr_in6 *)&psrtentry->srt_edest)->sin6_addr);
    
    SRT_LOCK();
    psentry6 = srt6_find_entry(&ip6saddr, &ip6eaddr, &ip6sdest, &ip6edest);
    if (!psentry6) {
        SRT_UNLOCK();
        _ErrorHandle(ENETUNREACH);
        return  (PX_ERROR);
    }
    srt6_delete_entry(psentry6);
    
    srt6_srtentry_to_sentry(psentry6, psrtentry);                       /*  转换为 srt_entry            */
    
    iRet = srt6_add_entry(psentry6);
    SRT_UNLOCK();
    if (iRet < 0) {
        mem_free(psentry6);
        _ErrorHandle(ENETUNREACH);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** 函数名称: __srtGet4
** 功能描述: 获取一条 IPv4 源路由信息
** 输　入  : psrtentry  源路由信息
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __srtGet4 (struct srtentry  *psrtentry)
{
    struct srt_entry   *psentry4;
    ip4_addr_t          ipaddr, ipdest;
    
    inet_addr_to_ip4addr(&ipaddr, &((struct sockaddr_in *)&psrtentry->srt_ssrc)->sin_addr);
    inet_addr_to_ip4addr(&ipdest, &((struct sockaddr_in *)&psrtentry->srt_sdest)->sin_addr);
    
    SRT_LOCK();
    psentry4 = srt_search_entry(&ipaddr, &ipdest);
    if (!psentry4) {
        SRT_UNLOCK();
        _ErrorHandle(ENETUNREACH);
        return  (PX_ERROR);
    }
    srt_sentry_to_srtentry(psrtentry, psentry4);    
    SRT_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __srtGet6
** 功能描述: 获取一条 IPv6 源路由信息
** 输　入  : psrtentry  源路由信息
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LWIP_IPV6

static INT  __srtGet6 (struct srtentry  *psrtentry)
{
    struct srt6_entry  *psentry6;
    ip6_addr_t          ip6addr, ip6dest;
    
    inet6_addr_to_ip6addr(&ip6addr, &((struct sockaddr_in6 *)&psrtentry->srt_ssrc)->sin6_addr);
    inet6_addr_to_ip6addr(&ip6dest, &((struct sockaddr_in6 *)&psrtentry->srt_sdest)->sin6_addr);
    
    SRT_LOCK();
    psentry6 = srt6_search_entry(&ip6addr, &ip6dest);
    if (!psentry6) {
        SRT_UNLOCK();
        _ErrorHandle(ENETUNREACH);
        return  (PX_ERROR);
    }
    srt6_sentry_to_srtentry(psrtentry, psentry6);    
    SRT_UNLOCK();
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** 函数名称: __srtLst4Walk
** 功能描述: 遍历 IPv4 源路由信息
** 输　入  : psentry4    源内部路由表项
**           psrtelist   源路由信息缓存
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __srtLst4Walk (struct srt_entry     *psentry4, 
                            struct srtentry_list *psrtelist)
{
    if (psrtelist->srtl_num < psrtelist->srtl_bcnt) {
        struct srtentry *psrtentry = &psrtelist->srtl_buf[psrtelist->srtl_num];
        srt_sentry_to_srtentry(psrtentry, psentry4);                    /*  转换为 srtentry             */
        psrtelist->srtl_num++;
    }
}
/*********************************************************************************************************
** 函数名称: __srtLst4
** 功能描述: 获取整个 IPv4 源路由信息
** 输　入  : psrtelist  源路由信息缓存
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __srtLst4 (struct srtentry_list *psrtelist)
{
    UINT    uiTotal;
    
    psrtelist->srtl_num = 0;
    
    SRT_LOCK();
    srt_total_entry(&uiTotal);
    psrtelist->srtl_total = uiTotal;
    if (!psrtelist->srtl_bcnt || !psrtelist->srtl_buf) {
        SRT_UNLOCK();
        return  (ERROR_NONE);
    }
    srt_traversal_entry(__srtLst4Walk, psrtelist, LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    SRT_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __srtLst6Walk
** 功能描述: 遍历 IPv6 源路由信息
** 输　入  : psentry6   源内部路由表项
**           psrtelist  源路由信息缓存
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LWIP_IPV6

static VOID  __srtLst6Walk (struct srt6_entry    *psentry6, 
                            struct srtentry_list *psrtelist)
{
    if (psrtelist->srtl_num < psrtelist->srtl_bcnt) {
        struct srtentry *psrtentry = &psrtelist->srtl_buf[psrtelist->srtl_num];
        srt6_sentry_to_srtentry(psrtentry, psentry6);                   /*  转换为 srtentry             */
        psrtelist->srtl_num++;
    }
}
/*********************************************************************************************************
** 函数名称: __srtLst6
** 功能描述: 获取整个 IPv6 源路由信息
** 输　入  : psrtelist  源路由信息缓存
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __srtLst6 (struct srtentry_list *psrtelist)
{
    UINT    uiTotal;
    
    psrtelist->srtl_num = 0;
    
    SRT_LOCK();
    srt6_total_entry(&uiTotal);
    psrtelist->srtl_total = uiTotal;
    if (!psrtelist->srtl_bcnt || !psrtelist->srtl_buf) {
        SRT_UNLOCK();
        return  (ERROR_NONE);
    }
    srt6_traversal_entry(__srtLst6Walk, psrtelist, LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    SRT_UNLOCK();
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** 函数名称: __srtIoctlInet
** 功能描述: SIOCADDSRT / SIOCDELSRT ... 命令处理接口
** 输　入  : iFamily    AF_INET / AF_INET6
**           iCmd       SIOCADDRT / SIOCDELRT
**           pvArg      struct arpreq
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __srtIoctlInet (INT  iFamily, INT  iCmd, PVOID  pvArg)
{
    struct srtentry  *psrtentry = (struct srtentry  *)pvArg;
    INT              iRet       = PX_ERROR;

    if (!psrtentry) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    switch (iCmd) {
    
    case SIOCADDSRT:                                                    /*  添加源路由信息              */
        if (iFamily == AF_INET) {
            iRet = __srtAdd4(psrtentry);
        } 
#if LWIP_IPV6
          else {
            iRet = __srtAdd6(psrtentry);
        }
#endif
        break;
    
    case SIOCDELSRT:                                                    /*  删除源路由信息              */
        if (iFamily == AF_INET) {
            iRet = __srtDel4(psrtentry);
        }
#if LWIP_IPV6
          else {
            iRet = __srtDel6(psrtentry);
        }
#endif
        break;

   case SIOCCHGSRT:                                                     /*  修改源路由信息              */
        if (iFamily == AF_INET) {
            iRet = __srtChg4(psrtentry);
        } 
#if LWIP_IPV6
          else {
            iRet = __srtChg6(psrtentry);
        }
#endif
        break;
        
    case SIOCGETSRT:                                                    /*  查询源路由信息              */
        if (iFamily == AF_INET) {
            iRet = __srtGet4(psrtentry);
        } 
#if LWIP_IPV6
          else {
            iRet = __srtGet6(psrtentry);
        }
#endif
        break;
        
    case SIOCLSTSRT:                                                    /*  源路由列表                  */
        if (iFamily == AF_INET) {
            iRet = __srtLst4((struct srtentry_list *)pvArg);
        } 
#if LWIP_IPV6
          else {
            iRet = __srtLst6((struct srtentry_list *)pvArg);
        }
#endif
        break;

    default:
        _ErrorHandle(ENOSYS);
        break;
    }
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_ROUTER > 0       */
                                                                        /*  LW_CFG_NET_BALANCING > 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/

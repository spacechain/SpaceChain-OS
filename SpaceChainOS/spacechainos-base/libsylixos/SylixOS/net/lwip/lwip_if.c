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
** 文   件   名: lwip_if.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 03 月 10 日
**
** 描        述: posix net/if 接口.

** BUG:
2011.07.07  _G_ulNetifLock 改名.
2014.03.22  优化获得网络接口.
2014.06.24  加入 if_down 与 if_up API.
2014.12.01  停止网卡是如果使能 dhcp 需要停止租约.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "net/if.h"
#include "net/if_lock.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/tcpip.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_OBJECT_HANDLE  _G_hNetListRWLock;
/*********************************************************************************************************
** 函数名称: if_list_init
** 功能描述: 网络接口链锁初始化
** 输　入  : NONE
** 输　出  : 是否成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  if_list_init (VOID)
{
    _G_hNetListRWLock = API_SemaphoreRWCreate("if_list", LW_OPTION_OBJECT_GLOBAL | 
                                              LW_OPTION_DELETE_SAFE | LW_OPTION_WAIT_PRIORITY, LW_NULL);
    if (_G_hNetListRWLock == LW_OBJECT_HANDLE_INVALID) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "network interface list lock create error.\r\n");
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: if_list_lock
** 功能描述: 锁定网络接口链
** 输　入  : bWrite        TRUE: 写 FALSE: 读
** 输　出  : 是否成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  if_list_lock (BOOL  bWrite)
{
    LW_OBJECT_HANDLE  hMe;
    LW_OBJECT_HANDLE  hCLOwner;
    ULONG             ulError;

    hMe = API_ThreadIdSelf();
    if (!hMe) {
        return  (PX_ERROR);
    }
    
    API_SemaphoreMStatusEx(lock_tcpip_core, LW_NULL, LW_NULL, LW_NULL, &hCLOwner);
    if (hMe == hCLOwner) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "network dead lock detected.\r\n");
        return  (PX_ERROR);
    }
    
    if (bWrite) {
        ulError = API_SemaphoreRWPendW(_G_hNetListRWLock, LW_OPTION_WAIT_INFINITE);
    
    } else {
        ulError = API_SemaphoreRWPendR(_G_hNetListRWLock, LW_OPTION_WAIT_INFINITE);
    }
    
    return  (ulError ? PX_ERROR : ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: if_list_unlock
** 功能描述: 解锁网络接口链
** 输　入  : NONE
** 输　出  : 是否成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  if_list_unlock (VOID)
{
    API_SemaphoreRWPost(_G_hNetListRWLock);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: if_down
** 功能描述: 关闭网卡
** 输　入  : ifname        if name
** 输　出  : 关闭是否成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  if_down (const char *ifname)
{
    struct netif  *pnetif;
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  进入临界区                  */
    pnetif = netif_find(ifname);
    if (pnetif == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    if (!netif_is_up(pnetif)) {
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        _ErrorHandle(EALREADY);
        return  (PX_ERROR);
    }
    
#if LWIP_DHCP > 0
    netifapi_dhcp_release_and_stop(pnetif);
#endif                                                                  /*  LWIP_DHCP > 0               */
#if LWIP_AUTOIP > 0
    netifapi_autoip_stop(pnetif);
#endif                                                                  /*  LWIP_AUTOIP > 0             */
#if LWIP_IPV6_DHCP6 > 0
    netifapi_dhcp6_disable(pnetif);
#endif                                                                  /*  LWIP_IPV6_DHCP6 > 0         */
    netifapi_netif_set_down(pnetif);
    LWIP_IF_LIST_UNLOCK();                                              /*  退出临界区                  */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: if_up
** 功能描述: 打开网卡
** 输　入  : ifname        if name
** 输　出  : 网卡是否打开
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  if_up (const char *ifname)
{
    struct netif  *pnetif;
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  进入临界区                  */
    pnetif = netif_find(ifname);
    if (pnetif == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    if (netif_is_up(pnetif)) {
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        _ErrorHandle(EALREADY);
        return  (PX_ERROR);
    }
    
    netifapi_netif_set_up(pnetif);
    
#if LWIP_DHCP > 0
    if (pnetif->flags2 & NETIF_FLAG2_DHCP) {
        netifapi_netif_set_addr(pnetif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
        netifapi_dhcp_start(pnetif);
    }
#endif                                                                  /*  LWIP_DHCP > 0               */

#if LWIP_IPV6_DHCP6 > 0
    if (pnetif->flags2 & NETIF_FLAG2_DHCP6) {
        netifapi_dhcp6_enable_stateless(pnetif);
    }
#endif                                                                  /*  LWIP_IPV6_DHCP6 > 0         */
    LWIP_IF_LIST_UNLOCK();                                              /*  退出临界区                  */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: if_isup
** 功能描述: 网卡是否使能
** 输　入  : ifname        if name
** 输　出  : 网卡是否使能 0: 禁能  1: 使能  -1: 网卡名字错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  if_isup (const char *ifname)
{
    INT            iRet;
    struct netif  *pnetif;
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  进入临界区                  */
    pnetif = netif_find(ifname);
    if (pnetif == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    iRet = netif_is_up(pnetif);
    LWIP_IF_LIST_UNLOCK();                                              /*  退出临界区                  */
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: if_islink
** 功能描述: 网卡是否已经连接
** 输　入  : ifname        if name
** 输　出  : 网卡是否连接 0: 连接  1: 没有连接  -1: 网卡名字错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  if_islink (const char *ifname)
{
    INT            iRet;
    struct netif  *pnetif;
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  进入临界区                  */
    pnetif = netif_find(ifname);
    if (pnetif == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    iRet = netif_is_link_up(pnetif);
    LWIP_IF_LIST_UNLOCK();                                              /*  退出临界区                  */

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: if_set_dhcp
** 功能描述: 设置网卡 dhcp 选项 (必须在网卡禁能时设置)
** 输　入  : ifname        if name
**           en            1: 使能 dhcp  0: 禁能 dhcp
** 输　出  : OK or ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  if_set_dhcp (const char *ifname, int en)
{
    INT     iRet = PX_ERROR;

#if LWIP_DHCP > 0
    struct netif  *pnetif;
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  进入临界区                  */
    pnetif = netif_find(ifname);
    if (pnetif == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    if (netif_is_up(pnetif)) {
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        _ErrorHandle(EISCONN);
        return  (PX_ERROR);
    }
    
    if (en) {
        pnetif->flags2 |= NETIF_FLAG2_DHCP;
    } else {
        pnetif->flags2 &= ~NETIF_FLAG2_DHCP;
    }
    LWIP_IF_LIST_UNLOCK();                                              /*  退出临界区                  */
#else
    _ErrorHandle(ENOSYS);
#endif                                                                  /*  LWIP_DHCP > 0               */
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: if_get_dhcp
** 功能描述: 获取网卡 dhcp 选项
** 输　入  : ifname        if name
** 输　出  : 1: 使能 dhcp  0: 禁能 dhcp -1: 网卡名字错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  if_get_dhcp (const char *ifname)
{
    INT            iRet;
    struct netif  *pnetif;
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  进入临界区                  */
    pnetif = netif_find((char *)ifname);
    if (pnetif == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    iRet = (pnetif->flags2 & NETIF_FLAG2_DHCP) ? 1 : 0;
    LWIP_IF_LIST_UNLOCK();                                              /*  退出临界区                  */
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: if_set_dhcp6
** 功能描述: 设置网卡 dhcp v6 选项 (必须在网卡禁能时设置)
** 输　入  : ifname        if name
**           en            1: 使能 dhcp  0: 禁能 dhcp
**           stateless     1: stateless  0: stateful (NOT support now)
** 输　出  : OK or ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  if_set_dhcp6 (const char *ifname, int en, int stateless)
{
    INT     iRet = PX_ERROR;

#if LWIP_IPV6_DHCP6 > 0
    struct netif  *pnetif;
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  进入临界区                  */
    pnetif = netif_find(ifname);
    if (pnetif == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    if (netif_is_up(pnetif)) {
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        _ErrorHandle(EISCONN);
        return  (PX_ERROR);
    }
    
    if (en) {
        pnetif->flags2 |= NETIF_FLAG2_DHCP6;
    } else {
        pnetif->flags2 &= ~NETIF_FLAG2_DHCP6;
    }
    LWIP_IF_LIST_UNLOCK();                                              /*  退出临界区                  */
#else
    _ErrorHandle(ENOSYS);
#endif                                                                  /*  LWIP_IPV6_DHCP6 > 0         */
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: if_get_dhcp6
** 功能描述: 获取网卡 dhcp v6 选项
** 输　入  : ifname        if name
** 输　出  : 1: 使能 dhcp  0: 禁能 dhcp -1: 网卡名字错误
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  if_get_dhcp6 (const char *ifname)
{
    INT            iRet;
    struct netif  *pnetif;
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  进入临界区                  */
    pnetif = netif_find((char *)ifname);
    if (pnetif == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    iRet = (pnetif->flags2 & NETIF_FLAG2_DHCP6) ? 1 : 0;
    LWIP_IF_LIST_UNLOCK();                                              /*  退出临界区                  */
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: if_nametoindex
** 功能描述: map a network interface name to its corresponding index
** 输　入  : ifname        if name
** 输　出  : index
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
unsigned  if_nametoindex (const char *ifname)
{
    unsigned  uiIndex = 0;
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  进入临界区                  */
    uiIndex = netif_name_to_index(ifname);
    LWIP_IF_LIST_UNLOCK();                                              /*  退出临界区                  */
    
    if (uiIndex == 0) {
        _ErrorHandle(ENXIO);
    }
    
    return  (uiIndex);
}
/*********************************************************************************************************
** 函数名称: if_indextoname
** 功能描述: map a network interface index to its corresponding name
** 输　入  : ifindex       if index
**           ifname        if name buffer at least {IF_NAMESIZE} bytes
** 输　出  : index
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
char *if_indextoname (unsigned  ifindex, char *ifname)
{
    char  *ret;

    if (!ifname) {
        errno = EINVAL;
    }
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  进入临界区                  */
    ret = netif_index_to_name(ifindex, ifname);
    LWIP_IF_LIST_UNLOCK();                                              /*  退出临界区                  */
    
    if (ret == LW_NULL) {
        _ErrorHandle(ENXIO);
    }
    
    return  (ret);
}
/*********************************************************************************************************
** 函数名称: if_nameindex
** 功能描述: return all network interface names and indexes
** 输　入  : NONE
** 输　出  : An array of structures identifying local interfaces
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
struct if_nameindex *if_nameindex (void)
{
    struct netif           *pnetif;
    int                     i = 0, iNum = 1;                            /*  需要一个空闲的位置          */
    struct if_nameindex    *pifnameindexArry;
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  进入临界区                  */
    NETIF_FOREACH(pnetif) {
        iNum++;
    }
    pifnameindexArry = (struct if_nameindex *)__SHEAP_ALLOC(sizeof(struct if_nameindex) * (size_t)iNum);
    if (pifnameindexArry == LW_NULL) {
        LWIP_IF_LIST_UNLOCK();                                          /*  退出临界区                  */
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    
    NETIF_FOREACH(pnetif) {
        pifnameindexArry[i].if_index = netif_get_index(pnetif);
        pifnameindexArry[i].if_name  = netif_index_to_name(pifnameindexArry[i].if_index, 
                                                           pifnameindexArry[i].if_name_buf);
        i++;
    }
    
    pifnameindexArry[i].if_index       = 0;
    pifnameindexArry[i].if_name_buf[0] = PX_EOS;
    pifnameindexArry[i].if_name        = pifnameindexArry[i].if_name_buf;
    LWIP_IF_LIST_UNLOCK();                                              /*  退出临界区                  */
    
    return  (pifnameindexArry);
}
/*********************************************************************************************************
** 函数名称: if_nameindex_bufsize
** 功能描述: 计算 if_nameindex 所需的缓存大小
** 输　入  : NONE
** 输　出  : 缓存大小
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
size_t if_nameindex_bufsize (void)
{
    struct netif  *pnetif;
    INT            iNum = 1;

    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  进入临界区                  */
    NETIF_FOREACH(pnetif) {
        iNum++;
    }
    LWIP_IF_LIST_UNLOCK();                                              /*  退出临界区                  */
    
    return  (sizeof(struct if_nameindex) * (size_t)iNum);
}
/*********************************************************************************************************
** 函数名称: if_nameindex
** 功能描述: return all network interface names and indexes
** 输　入  : buffer    缓存位置
**           bufsize   缓存大小
** 输　出  : An array of structures identifying local interfaces
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
struct if_nameindex *if_nameindex_rnp (void *buffer, size_t bufsize)
{
    struct netif           *pnetif;
    int                     iMax = bufsize / sizeof(struct if_nameindex);
    int                     i = 0;
    struct if_nameindex    *pifnameindexArry;
    
    if (!buffer || (bufsize < sizeof(struct if_nameindex))) {
        errno = EINVAL;
        return  (LW_NULL);
    }
    
    LWIP_IF_LIST_LOCK(LW_FALSE);                                        /*  进入临界区                  */
    pifnameindexArry = (struct if_nameindex *)buffer;
    NETIF_FOREACH(pnetif) {
        if (i >= (iMax - 1)) {
            break;
        }
        pifnameindexArry[i].if_index = netif_get_index(pnetif);
        pifnameindexArry[i].if_name  = netif_index_to_name(pifnameindexArry[i].if_index, 
                                                           pifnameindexArry[i].if_name_buf);
        i++;
    }
    
    pifnameindexArry[i].if_index       = 0;
    pifnameindexArry[i].if_name_buf[0] = PX_EOS;
    pifnameindexArry[i].if_name        = pifnameindexArry[i].if_name_buf;
    LWIP_IF_LIST_UNLOCK();                                              /*  退出临界区                  */
    
    return  (pifnameindexArry);
}
/*********************************************************************************************************
** 函数名称: if_freenameindex
** 功能描述: free memory allocated by if_nameindex
             the application shall not use the array of which ptr is the address.
** 输　入  : ptr           shall be a pointer that was returned by if_nameindex().
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
void  if_freenameindex (struct if_nameindex *ptr)
{
    if (ptr) {
        __SHEAP_FREE(ptr);
    }
}

#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/

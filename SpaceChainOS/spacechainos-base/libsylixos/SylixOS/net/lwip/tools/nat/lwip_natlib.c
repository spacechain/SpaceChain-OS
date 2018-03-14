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
** 文   件   名: lwip_natlib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 03 月 19 日
**
** 描        述: lwip NAT 支持包内部库.

** BUG:
2010.04.08  今天是购买科鲁兹后第一次升级代码, 修正注释, 修正代码格式.
2010.04.12  修正 nat_ip_input_hook() 类型, 系统使用的 ip_input_hook().
2010.11.06  广州亚运会马上召开, 今天却宣布停止相关的惠民政策, 转而发放惠民款到广州户籍个人, 对外来务工人员
            是赤裸裸的歧视! 外来务工人员为广州亚运的建设添砖加瓦, 却得到如此待遇.
            ip nat 单网口网络代理.
2011.03.06  修正 gcc 4.5.1 相关 warning.
2011.06.23  优化一部分代码. 
            对 NAT 转发的条件进行了严格的限制.
2011.07.01  lwip 支持本人提出的路由改进方案, 至此 NAT 支持单网口转发.
            https://savannah.nongnu.org/bugs/?33634
2011.07.30  当 ap 输入不在代理端口范围以内, 直接退出.
2013.04.01  修正 GCC 4.7.3 引发的新 warning.
2015.12.16  增加 NAT 别名支持能力.
            增加主动连接映射功能.
2016.08.25  修正主动连接缺少代理控制块的问题.
2017.12.19  加入 MAP 负载均衡功能.
2018.01.16  使用 iphook 实现更加灵活的 NAT 管理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_ROUTER > 0) && (LW_CFG_NET_NAT_EN > 0)
#include "net/if.h"
#include "net/if_iphook.h"
#include "lwip/opt.h"
#include "lwip/inet.h"
#include "lwip/ip.h"
#include "lwip/tcp.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/udp.h"
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/err.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip_natlib.h"
/*********************************************************************************************************
  NAT 操作锁
*********************************************************************************************************/
#define __NAT_LOCK()        LOCK_TCPIP_CORE()
#define __NAT_UNLOCK()      UNLOCK_TCPIP_CORE()
/*********************************************************************************************************
  NAT 接口类型
*********************************************************************************************************/
typedef struct {
    CHAR            NATIF_cIfName[IF_NAMESIZE];
    struct netif   *NATIF_pnetif;
} __NAT_IF;
typedef __NAT_IF   *__PNAT_IF;
/*********************************************************************************************************
  NAT 本地内网与 AP 外网网络接口
*********************************************************************************************************/
static __NAT_IF             _G_natifAp[LW_CFG_NET_NAT_MAX_AP_IF];
static __NAT_IF             _G_natifLocal[LW_CFG_NET_NAT_MAX_LOCAL_IF];
static BOOL                 _G_bNatStart = LW_FALSE;
/*********************************************************************************************************
  NAT 控制块缓冲池
*********************************************************************************************************/
static __NAT_CB             _G_natcbBuffer[LW_CFG_NET_NAT_MAX_SESSION];
static LW_OBJECT_HANDLE     _G_ulNatcbPool  = LW_OBJECT_HANDLE_INVALID;
static LW_OBJECT_HANDLE     _G_ulNatcbTimer = LW_OBJECT_HANDLE_INVALID;
/*********************************************************************************************************
  NAT 控制块链表
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _G_plineNatcbTcp  = LW_NULL;
static LW_LIST_LINE_HEADER  _G_plineNatcbUdp  = LW_NULL;
static LW_LIST_LINE_HEADER  _G_plineNatcbIcmp = LW_NULL;
/*********************************************************************************************************
  NAT 别名
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _G_plineNatalias = LW_NULL;
/*********************************************************************************************************
  NAT 外网主动连接
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _G_plineNatmap = LW_NULL;
/*********************************************************************************************************
** 函数名称: nat_netif_add_hook
** 功能描述: NAT 网络接口加入回调
** 输　入  : pnetif    被加入的网络接口
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  nat_netif_add_hook (struct netif *pnetif)
{
    INT    i;
    CHAR   cIfName[IF_NAMESIZE];
    
    if (!_G_bNatStart) {
        return;
    }
    
    __NAT_LOCK();
    netif_get_name(pnetif, cIfName);
    
    for (i = 0; i < LW_CFG_NET_NAT_MAX_AP_IF; i++) {
        if (_G_natifAp[i].NATIF_pnetif == LW_NULL) {
            if (!lib_strcmp(cIfName, _G_natifAp[i].NATIF_cIfName)) {
                _G_natifAp[i].NATIF_pnetif = pnetif;
                goto    out;
            }
        }
    }
    for (i = 0; i < LW_CFG_NET_NAT_MAX_LOCAL_IF; i++) {
        if (_G_natifLocal[i].NATIF_pnetif == LW_NULL) {
            if (!lib_strcmp(cIfName, _G_natifLocal[i].NATIF_cIfName)) {
                _G_natifLocal[i].NATIF_pnetif = pnetif;
                goto    out;
            }
        }
    }
out:
    __NAT_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: nat_netif_remove_hook
** 功能描述: NAT 网络接口移除回调
** 输　入  : pnetif    被移除的网络接口
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  nat_netif_remove_hook (struct netif *pnetif)
{
    INT  i;
    
    if (!_G_bNatStart) {
        return;
    }
    
    __NAT_LOCK();
    for (i = 0; i < LW_CFG_NET_NAT_MAX_AP_IF; i++) {
        if (_G_natifAp[i].NATIF_pnetif == pnetif) {
            _G_natifAp[i].NATIF_pnetif =  LW_NULL;
            goto    out;
        }
    }
    for (i = 0; i < LW_CFG_NET_NAT_MAX_LOCAL_IF; i++) {
        if (_G_natifLocal[i].NATIF_pnetif == pnetif) {
            _G_natifLocal[i].NATIF_pnetif =  LW_NULL;
            goto    out;
        }
    }
out:
    __NAT_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: __natPoolInit
** 功能描述: 创建 NAT 控制块缓冲池.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __natPoolInit (VOID)
{
    _G_ulNatcbPool = API_PartitionCreate("nat_pool", (PVOID)_G_natcbBuffer, LW_CFG_NET_NAT_MAX_SESSION,
                                         sizeof(__NAT_CB), LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _BugHandle((_G_ulNatcbPool == LW_OBJECT_HANDLE_INVALID), LW_TRUE, "can not create 'nat_pool'\r\n");
}
/*********************************************************************************************************
** 函数名称: __natPoolAlloc
** 功能描述: 分配一个 NAT 控制块.
** 输　入  : NONE
** 输　出  : NAT 控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static __PNAT_CB  __natPoolAlloc (VOID)
{
    __PNAT_CB   pnatcb = (__PNAT_CB)API_PartitionGet(_G_ulNatcbPool);
    
    if (pnatcb) {
        lib_bzero(pnatcb, sizeof(__NAT_CB));
    }

    return  (pnatcb);
}
/*********************************************************************************************************
** 函数名称: __natPoolFree
** 功能描述: 释放一个 NAT 控制块.
** 输　入  : pnatcb            NAT 控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __natPoolFree (__PNAT_CB  pnatcb)
{
    if (pnatcb) {
        API_PartitionPut(_G_ulNatcbPool, (PVOID)pnatcb);
    }
}
/*********************************************************************************************************
** 函数名称: __natNewmap
** 功能描述: 创建一个 NAT 外网映射对象.
** 输　入  : pipaddr       本地机器 IP
**           usIpCnt       本地 IP 数量 (负载均衡)
**           usPort        本地机器 端口
**           AssPort       映射端口 
**           ucProto       使用的协议
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __natMapAdd (ip4_addr_t  *pipaddr, u16_t  usIpCnt, u16_t  usPort, u16_t  AssPort, u8_t  ucProto)
{
    __PNAT_MAP  pnatmap;
    
    pnatmap = (__PNAT_MAP)__SHEAP_ALLOC(sizeof(__NAT_MAP));
    if (pnatmap == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    pnatmap->NAT_ucProto            = ucProto;
    pnatmap->NAT_ipaddrLocalIp.addr = pipaddr->addr;
    pnatmap->NAT_usLocalCnt         = usIpCnt;
    pnatmap->NAT_usLocalPort        = usPort;
    pnatmap->NAT_usAssPort          = AssPort;
    
    __NAT_LOCK();
    _List_Line_Add_Ahead(&pnatmap->NAT_lineManage, &_G_plineNatmap);
    __NAT_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __natNewmap
** 功能描述: 删除一个 NAT 外网映射对象.
** 输　入  : pipaddr       本地机器 IP
**           usPort        本地机器 端口
**           AssPort       映射端口 
**           ucProto       使用的协议
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __natMapDelete (ip4_addr_t  *pipaddr, u16_t  usPort, u16_t  AssPort, u8_t  ucProto)
{
    PLW_LIST_LINE   plineTemp;
    __PNAT_MAP      pnatmap;
    PVOID           pvFree = LW_NULL;
    
    __NAT_LOCK();
    for (plineTemp  = _G_plineNatmap;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pnatmap = _LIST_ENTRY(plineTemp, __NAT_MAP, NAT_lineManage);
        if ((pnatmap->NAT_ucProto            == ucProto)       &&
            (pnatmap->NAT_ipaddrLocalIp.addr == pipaddr->addr) &&
            (pnatmap->NAT_usLocalPort        == usPort)        &&
            (pnatmap->NAT_usAssPort          == AssPort)) {
            break;
        }
    }
    if (plineTemp) {
        _List_Line_Del(&pnatmap->NAT_lineManage, &_G_plineNatmap);
        pvFree = pnatmap;
    }
    __NAT_UNLOCK();
    
    if (pvFree) {
        __SHEAP_FREE(pvFree);
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __natAliasAdd
** 功能描述: NAT 加入网络别名 (公网 IP 映射)
** 输　入  : pipaddrAlias      别名
**           ipaddrSLocalIp    对应本地起始 IP
**           ipaddrELocalIp    对应本地结束 IP
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __natAliasAdd (const ip4_addr_t  *pipaddrAlias, 
                    const ip4_addr_t  *ipaddrSLocalIp,
                    const ip4_addr_t  *ipaddrELocalIp)
{
    __PNAT_ALIAS  pnatalias;
    
    pnatalias = (__PNAT_ALIAS)__SHEAP_ALLOC(sizeof(__NAT_ALIAS));
    if (pnatalias == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    pnatalias->NAT_ipaddrAliasIp.addr  = pipaddrAlias->addr;
    pnatalias->NAT_ipaddrSLocalIp.addr = ipaddrSLocalIp->addr;
    pnatalias->NAT_ipaddrELocalIp.addr = ipaddrELocalIp->addr;
    
    __NAT_LOCK();
    _List_Line_Add_Ahead(&pnatalias->NAT_lineManage, &_G_plineNatalias);
    __NAT_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __natAliasDelete
** 功能描述: NAT 删除网络别名 (公网 IP 映射)
** 输　入  : pipaddrAlias      别名
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __natAliasDelete (const ip4_addr_t  *pipaddrAlias)
{
    PLW_LIST_LINE   plineTemp;
    __PNAT_ALIAS    pnatalias;
    PVOID           pvFree = LW_NULL;
    
    __NAT_LOCK();
    for (plineTemp  = _G_plineNatalias;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pnatalias = _LIST_ENTRY(plineTemp, __NAT_ALIAS, NAT_lineManage);
        if (pnatalias->NAT_ipaddrAliasIp.addr == pipaddrAlias->addr) {
            break;
        }
    }
    if (plineTemp) {
        _List_Line_Del(&pnatalias->NAT_lineManage, &_G_plineNatalias);
        pvFree = pnatalias;
    }
    __NAT_UNLOCK();
    
    if (pvFree) {
        __SHEAP_FREE(pvFree);
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __natNewPort
** 功能描述: 产生一个 NAT 临时唯一端口.
** 输　入  : NONE
** 输　出  : 端口
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static u16_t  __natNewPort (VOID)
{
    static u32_t    u32Port = LW_CFG_NET_NAT_MIN_PORT;
    __PNAT_CB       pnatcb;
    PLW_LIST_LINE   plineTemp;
    
__check_again:
    if (++u32Port > LW_CFG_NET_NAT_MAX_PORT) {
        u32Port = LW_CFG_NET_NAT_MIN_PORT;
    }
    
    for (plineTemp  = _G_plineNatcbTcp;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  查找是否有重复的端口号      */
        
        pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
        if (pnatcb->NAT_usAssPort == (u16_t)u32Port) {
            goto    __check_again;
        }
    }
    
    for (plineTemp  = _G_plineNatcbUdp;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  查找是否有重复的端口号      */
        
        pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
        if (pnatcb->NAT_usAssPort == (u16_t)u32Port) {
            goto    __check_again;
        }
    }
    
    for (plineTemp  = _G_plineNatcbIcmp;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  查找是否有重复的端口号      */
        
        pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
        if (pnatcb->NAT_usAssPort == (u16_t)u32Port) {
            goto    __check_again;
        }
    }
    
    return  ((u16_t)u32Port);
}
/*********************************************************************************************************
** 函数名称: __natNew
** 功能描述: 创建一个 NAT 对象.
** 输　入  : pipaddr       本地机器 IP
**           usPort        本地机器 端口
**           ucProto       使用的协议
** 输　出  : NAT 控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static __PNAT_CB  __natNew (ip4_addr_p_t  *pipaddr, u16_t  usPort, u8_t  ucProto)
{
    PLW_LIST_LINE   plineTemp;
    __PNAT_CB       pnatcb = __natPoolAlloc();
    __PNAT_CB       pnatcbOldest;
    u16_t           usNewPort;
    
    if (pnatcb == LW_NULL) {                                            /*  需要删除最古老的            */
        /*
         *  没有开辟出来, 所以协议代理的三个链表中, 必然至少有一个存在节点.
         */
        if (_G_plineNatcbTcp) {
            pnatcbOldest = _LIST_ENTRY(_G_plineNatcbTcp, __NAT_CB, NAT_lineManage);
        } else if (_G_plineNatcbUdp) {
            pnatcbOldest = _LIST_ENTRY(_G_plineNatcbUdp, __NAT_CB, NAT_lineManage);
        } else {
            pnatcbOldest = _LIST_ENTRY(_G_plineNatcbIcmp, __NAT_CB, NAT_lineManage);
        }
            
        for (plineTemp  = _G_plineNatcbTcp;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
            if (pnatcb->NAT_ulIdleTimer > pnatcbOldest->NAT_ulIdleTimer) {
                pnatcbOldest = pnatcb;
            }
        }
        for (plineTemp  = _G_plineNatcbUdp;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
            if (pnatcb->NAT_ulIdleTimer > pnatcbOldest->NAT_ulIdleTimer) {
                pnatcbOldest = pnatcb;
            }
        }
        for (plineTemp  = _G_plineNatcbIcmp;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
            if (pnatcb->NAT_ulIdleTimer > pnatcbOldest->NAT_ulIdleTimer) {
                pnatcbOldest = pnatcb;
            }
        }
        
        pnatcb = pnatcbOldest;                                          /*  使用最老的                  */
        switch (pnatcb->NAT_ucProto) {
        
        case IP_PROTO_TCP:                                              /*  TCP 链表                    */
            _List_Line_Del(&pnatcb->NAT_lineManage, &_G_plineNatcbTcp);
            break;
            
        case IP_PROTO_UDP:                                              /*  UDP 链表                    */
            _List_Line_Del(&pnatcb->NAT_lineManage, &_G_plineNatcbUdp);
            break;
            
        case IP_PROTO_ICMP:                                             /*  ICMP 链表                   */
            _List_Line_Del(&pnatcb->NAT_lineManage, &_G_plineNatcbIcmp);
            break;
            
        default:                                                        /*  不应该运行到这里            */
            _BugHandle(LW_TRUE, LW_TRUE, "NAT Protocol error!\r\n");
            break;
        }
    }
        
    pnatcb->NAT_ucProto            = ucProto;
    pnatcb->NAT_ipaddrLocalIp.addr = pipaddr->addr;
    pnatcb->NAT_usLocalPort        = usPort;
    
    usNewPort = __natNewPort();
    pnatcb->NAT_usAssPort = PP_HTONS(usNewPort);                        /*  分配一个新的唯一端口        */
    
    pnatcb->NAT_ulIdleTimer = 0;
    pnatcb->NAT_ulTermTimer = 0;
    pnatcb->NAT_iStatus     = __NAT_STATUS_OPEN;
    
    switch (ucProto) {
        
    case IP_PROTO_TCP:                                                  /*  TCP 链表                    */
        _List_Line_Add_Ahead(&pnatcb->NAT_lineManage, &_G_plineNatcbTcp);
        break;
        
    case IP_PROTO_UDP:                                                  /*  UDP 链表                    */
        _List_Line_Add_Ahead(&pnatcb->NAT_lineManage, &_G_plineNatcbUdp);
        break;
        
    case IP_PROTO_ICMP:                                                 /*  ICMP 链表                   */
        _List_Line_Add_Ahead(&pnatcb->NAT_lineManage, &_G_plineNatcbIcmp);
        break;
        
    default:                                                            /*  不应该运行到这里            */
        _BugHandle(LW_TRUE, LW_TRUE, "NAT Protocol error!\r\n");
        break;
    }
    
    return  (pnatcb);
}
/*********************************************************************************************************
** 函数名称: __natClose
** 功能描述: 删除一个 NAT 对象.
** 输　入  : pnatcb            NAT 控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __natClose (__PNAT_CB  pnatcb)
{
    if (pnatcb == LW_NULL) {
        return;
    }
    
    switch (pnatcb->NAT_ucProto) {
    
    case IP_PROTO_TCP:                                                  /*  TCP 链表                    */
        _List_Line_Del(&pnatcb->NAT_lineManage, &_G_plineNatcbTcp);
        break;
        
    case IP_PROTO_UDP:                                                  /*  UDP 链表                    */
        _List_Line_Del(&pnatcb->NAT_lineManage, &_G_plineNatcbUdp);
        break;
        
    case IP_PROTO_ICMP:                                                 /*  ICMP 链表                   */
        _List_Line_Del(&pnatcb->NAT_lineManage, &_G_plineNatcbIcmp);
        break;
        
    default:                                                            /*  不应该运行到这里            */
        _BugHandle(LW_TRUE, LW_TRUE, "NAT Protocol error!\r\n");
        break;
    }

    __natPoolFree(pnatcb);
}
/*********************************************************************************************************
** 函数名称: __natTimer
** 功能描述: NAT 定时器.
** 输　入  : ulIdlTo       超时时间
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __natTimer (ULONG  ulIdlTo)
{
    __PNAT_CB       pnatcb;
    PLW_LIST_LINE   plineTemp;
    
    __NAT_LOCK();
    plineTemp = _G_plineNatcbTcp;
    while (plineTemp) {
        pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
        plineTemp = _list_line_get_next(plineTemp);                     /*  指向下一个节点              */
        
        pnatcb->NAT_ulIdleTimer++;
        if (pnatcb->NAT_ulIdleTimer >= ulIdlTo) {                       /*  空闲时间过长关闭            */
            __natClose(pnatcb);
        
        } else {
            if (pnatcb->NAT_iStatus == __NAT_STATUS_CLOSING) {
                pnatcb->NAT_ulTermTimer++;
                if ((pnatcb->NAT_ulTermTimer >= ulIdlTo) ||
                    (pnatcb->NAT_ulTermTimer >= 2)) {                   /*  断链超时                    */
                    __natClose(pnatcb);
                }
            }
        }
    }
    
    plineTemp = _G_plineNatcbUdp;
    while (plineTemp) {
        pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
        plineTemp = _list_line_get_next(plineTemp);                     /*  指向下一个节点              */
        
        pnatcb->NAT_ulIdleTimer++;
        if (pnatcb->NAT_ulIdleTimer >= ulIdlTo) {                       /*  空闲时间过长关闭            */
            __natClose(pnatcb);
        }
    }
    
    plineTemp = _G_plineNatcbIcmp;
    while (plineTemp) {
        pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
        plineTemp = _list_line_get_next(plineTemp);                     /*  指向下一个节点              */
        
        pnatcb->NAT_ulIdleTimer++;
        if (pnatcb->NAT_ulIdleTimer >= ulIdlTo) {                       /*  空闲时间过长关闭            */
            __natClose(pnatcb);
        }
    }
    __NAT_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: __natApInput
** 功能描述: NAT AP 网络接口输.
** 输　入  : p             数据包
**           netifIn       网络接口
** 输　出  : 0: ok 1: eaten
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __natApInput (struct pbuf *p, struct netif *netifIn)
{
    struct ip_hdr           *iphdr   = (struct ip_hdr *)p->payload;
    struct tcp_hdr          *tcphdr  = LW_NULL;
    struct udp_hdr          *udphdr  = LW_NULL;
    struct icmp_echo_hdr    *icmphdr = LW_NULL;
    
    u32_t                    u32OldAddr;
    u16_t                    usDestPort;
    u8_t                     ucProto;
    
    static u16_t             iphdrlen;
    
    __PNAT_CB                pnatcb  = LW_NULL;
    __PNAT_MAP               pnatmap = LW_NULL;
    PLW_LIST_LINE            plineTemp;
    LW_LIST_LINE_HEADER      plineHeader;
    
    iphdrlen  = (u16_t)IPH_HL(iphdr);                                   /*  获得 IP 报头长度            */
    iphdrlen *= 4;
    
    if (p->len < iphdrlen + TCP_HLEN) {                                 /*  缓冲错误                    */
        return  (0);
    }

    if ((IPH_OFFSET(iphdr) & PP_HTONS(IP_OFFMASK | IP_MF)) != 0) {      /*  不处理分片包                */
        return  (0);
    }
    
    ucProto = IPH_PROTO(iphdr);
    switch (ucProto) {
    
    case IP_PROTO_TCP:                                                  /*  TCP 数据报                  */
        tcphdr = (struct tcp_hdr *)(((u8_t *)p->payload) + iphdrlen);
        usDestPort  = tcphdr->dest;
        plineHeader = _G_plineNatcbTcp;
        break;
        
    case IP_PROTO_UDP:                                                  /*  UDP 数据报                  */
        udphdr = (struct udp_hdr *)(((u8_t *)p->payload) + iphdrlen);
        usDestPort  = udphdr->dest;
        plineHeader = _G_plineNatcbUdp;
        break;
        
    case IP_PROTO_ICMP:
        icmphdr = (struct icmp_echo_hdr *)(((u8_t *)p->payload) + iphdrlen);
        usDestPort  = icmphdr->id;
        plineHeader = _G_plineNatcbIcmp;
        break;
    
    default:
        return  (0);                                                    /*  不能处理的协议              */
    }

    if ((PP_NTOHS(usDestPort) < LW_CFG_NET_NAT_MIN_PORT) || 
        (PP_NTOHS(usDestPort) > LW_CFG_NET_NAT_MAX_PORT)) {             /*  目标端口不在代理端口之间    */
        for (plineTemp  = _G_plineNatmap;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            pnatmap = _LIST_ENTRY(plineTemp, __NAT_MAP, NAT_lineManage);
            if ((usDestPort == pnatmap->NAT_usAssPort) &&
                (ucProto    == pnatmap->NAT_ucProto)) {
                break;                                                  /*  找到了 NAT 映射控制块       */
            }
        }
        if (plineTemp == LW_NULL) {
            pnatmap = LW_NULL;                                          /*  没有找到                    */
        }
        
        if (pnatmap) {
            ip4_addr_p_t  ipaddr;                                       /*  内网映射服务器 IP           */
            
            for (plineTemp  = plineHeader;
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                 
                pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
                if ((usDestPort == pnatcb->NAT_usAssPort) &&
                    (ucProto    == pnatcb->NAT_ucProto)) {
                    break;                                              /*  找到了 NAT 控制块           */
                }
            }
            if (plineTemp == LW_NULL) {
                u32_t   uiHost;
                
                uiHost  = (u32_t)PP_NTOHL(pnatmap->NAT_ipaddrLocalIp.addr);
                uiHost += (iphdr->src.addr % pnatmap->NAT_usLocalCnt);  /*  根据源地址散列做均衡        */
                ipaddr.addr = (u32_t)PP_HTONL(uiHost);
                
                pnatcb = __natNew(&ipaddr, 
                                  pnatmap->NAT_usLocalPort, ucProto);   /*  新建控制块                  */
                pnatcb->NAT_usAssPort = pnatmap->NAT_usAssPort;
            }
        
            /*
             *  将数据包目标转为本地内网目标地址
             */
            u32OldAddr = iphdr->dest.addr;
            ((ip4_addr_t *)&(iphdr->dest))->addr = ipaddr.addr;
            inet_chksum_adjust((u8_t *)&IPH_CHKSUM(iphdr),(u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->dest.addr, 4);
        
            /*
             *  本机发送到内网的数据包目标端口为 NAT_usLocalPort
             */
            if (IPH_PROTO(iphdr) == IP_PROTO_TCP) {
                inet_chksum_adjust((u8_t *)&tcphdr->chksum,(u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->dest.addr, 4);
                tcphdr->dest = pnatmap->NAT_usLocalPort;
                inet_chksum_adjust((u8_t *)&tcphdr->chksum,(u8_t *)&usDestPort, 2, (u8_t *)&tcphdr->dest, 2);
            
            } else if (IPH_PROTO(iphdr) == IP_PROTO_UDP) {
                if (udphdr->chksum != 0) {
                    inet_chksum_adjust((u8_t *)&udphdr->chksum,(u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->dest.addr, 4);
    	            udphdr->dest = pnatmap->NAT_usLocalPort;
    	            inet_chksum_adjust((u8_t *)&udphdr->chksum,(u8_t *)&usDestPort, 2, (u8_t *)&udphdr->dest, 2);
                }
                
            } else if ((IPH_PROTO(iphdr) == IP_PROTO_ICMP) && 
                       ((ICMPH_CODE(icmphdr) == ICMP_ECHO || ICMPH_CODE(icmphdr) == ICMP_ER))) {
                icmphdr->id = pnatmap->NAT_usLocalPort;
                inet_chksum_adjust((u8_t *)&icmphdr->chksum,(u8_t *)&usDestPort, 2, (u8_t *)&icmphdr->id, 2);
            }
        }
    
    } else {                                                            /*  目标端口在代理端口之间      */
        for (plineTemp  = plineHeader;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
            if ((usDestPort == pnatcb->NAT_usAssPort) &&
                (ucProto    == pnatcb->NAT_ucProto)) {
                break;                                                  /*  找到了 NAT 控制块           */
            }
        }
        if (plineTemp == LW_NULL) {
            pnatcb = LW_NULL;                                           /*  没有找到                    */
        }
    
        if (pnatcb) {                                                   /*  如果找到控制块              */
            pnatcb->NAT_ulIdleTimer = 0;                                /*  刷新空闲时间                */
            
            /*
             *  将数据包目标转为本地内网目标地址
             */
            u32OldAddr = iphdr->dest.addr;
            ((ip4_addr_t *)&(iphdr->dest))->addr = pnatcb->NAT_ipaddrLocalIp.addr;
            inet_chksum_adjust((u8_t *)&IPH_CHKSUM(iphdr),(u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->dest.addr, 4);
            
            /*
             *  本机发送到内网的数据包目标端口为 NAT_usLocalPort
             */
            if (IPH_PROTO(iphdr) == IP_PROTO_TCP) {
                inet_chksum_adjust((u8_t *)&tcphdr->chksum,(u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->dest.addr, 4);
                tcphdr->dest = pnatcb->NAT_usLocalPort;
                inet_chksum_adjust((u8_t *)&tcphdr->chksum,(u8_t *)&usDestPort, 2, (u8_t *)&tcphdr->dest, 2);
            
            } else if (IPH_PROTO(iphdr) == IP_PROTO_UDP) {
                if (udphdr->chksum != 0) {
                    inet_chksum_adjust((u8_t *)&udphdr->chksum,(u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->dest.addr, 4);
    	            udphdr->dest = pnatcb->NAT_usLocalPort;
    	            inet_chksum_adjust((u8_t *)&udphdr->chksum,(u8_t *)&usDestPort, 2, (u8_t *)&udphdr->dest, 2);
                }
                
            } else if ((IPH_PROTO(iphdr) == IP_PROTO_ICMP) && 
                       ((ICMPH_CODE(icmphdr) == ICMP_ECHO || ICMPH_CODE(icmphdr) == ICMP_ER))) {
                icmphdr->id = pnatcb->NAT_usLocalPort;
                inet_chksum_adjust((u8_t *)&icmphdr->chksum,(u8_t *)&usDestPort, 2, (u8_t *)&icmphdr->id, 2);
            }
            
            /*
             *  NAT 控制块状态更新
             */
            if (IPH_PROTO(iphdr) == IP_PROTO_TCP) {
                if (TCPH_FLAGS(tcphdr) & TCP_RST) {
    	            pnatcb->NAT_iStatus = __NAT_STATUS_CLOSING;
                
                } else if (TCPH_FLAGS(tcphdr) & TCP_FIN) {
    	            if (pnatcb->NAT_iStatus == __NAT_STATUS_OPEN) {
    	                pnatcb->NAT_iStatus = __NAT_STATUS_FIN;
    	            
    	            } else {
    	                pnatcb->NAT_iStatus = __NAT_STATUS_CLOSING;
    	            }
                }
            }
        }
    }

    return  (0);
}
/*********************************************************************************************************
** 函数名称: __natApOutput
** 功能描述: NAT AP 网络接口输出.
** 输　入  : p             数据包
**           pnetifIn      数据包输入网络接口
**           netif         数据包输出网络接口
** 输　出  : 0: ok 1: eaten
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __natApOutput (struct pbuf *p, struct netif  *pnetifIn, struct netif *netifOut)
{
    struct ip_hdr           *iphdr   = (struct ip_hdr *)p->payload;
    struct tcp_hdr          *tcphdr  = LW_NULL;
    struct udp_hdr          *udphdr  = LW_NULL;
    struct icmp_echo_hdr    *icmphdr = LW_NULL;
    
    INT                      i;
    u32_t                    u32OldAddr;
    u16_t                    usDestPort, usSrcPort;
    u8_t                     ucProto;
    
    static u16_t             iphdrlen;
    
    __PNAT_CB                pnatcb    = LW_NULL;
    __PNAT_ALIAS             pnatalias = LW_NULL;
    PLW_LIST_LINE            plineTemp;
    LW_LIST_LINE_HEADER      plineHeader;
    
    for (i = 0; i < LW_CFG_NET_NAT_MAX_LOCAL_IF; i++) {
        if (_G_natifLocal[i].NATIF_pnetif == pnetifIn) {
            break;
        }
    }
    if (i >= LW_CFG_NET_NAT_MAX_LOCAL_IF) {
        return  (0);                                                    /*  不需要进行 NAT 地址转换     */
    }
    
    iphdrlen  = (u16_t)IPH_HL(iphdr);                                   /*  获得 IP 报头长度            */
    iphdrlen *= 4;
    
    if (p->len < iphdrlen + TCP_HLEN) {                                 /*  缓冲错误                    */
        return  (1);                                                    /*  删除此数据包                */
    }

    if ((IPH_OFFSET(iphdr) & PP_HTONS(IP_OFFMASK | IP_MF)) != 0) {      /*  不处理分片包                */
        return  (1);                                                    /*  删除此数据包                */
    }
    
    ucProto = IPH_PROTO(iphdr);
    switch (ucProto) {
    
    case IP_PROTO_TCP:                                                  /*  TCP 数据报                  */
        tcphdr = (struct tcp_hdr *)(((u8_t *)p->payload) + iphdrlen);
        usDestPort  = tcphdr->dest;
        usSrcPort   = tcphdr->src;
        plineHeader = _G_plineNatcbTcp;
        break;
        
    case IP_PROTO_UDP:                                                  /*  UDP 数据报                  */
        udphdr = (struct udp_hdr *)(((u8_t *)p->payload) + iphdrlen);
        usDestPort  = udphdr->dest;
        usSrcPort   = udphdr->src;
        plineHeader = _G_plineNatcbUdp;
        break;
        
    case IP_PROTO_ICMP:
        icmphdr = (struct icmp_echo_hdr *)(((u8_t *)p->payload) + iphdrlen);
        usSrcPort   = usDestPort = icmphdr->id;
        plineHeader = _G_plineNatcbIcmp;
        break;
    
    default:
        return  (1);                                                    /*  不能处理的协议              */
    }
    
    for (plineTemp  = plineHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
        
        /*
         *  控制块内的源端 IP 与 源端 PORT 使用协议完全一致.
         */
        if ((ip4_addr_cmp(&iphdr->src, &pnatcb->NAT_ipaddrLocalIp)) &&
            (usSrcPort == pnatcb->NAT_usLocalPort) &&
            (ucProto   == pnatcb->NAT_ucProto)) {
            break;                                                      /*  找到了 NAT 控制块           */
        }
    }
    if (plineTemp == LW_NULL) {
        pnatcb = LW_NULL;                                               /*  没有找到                    */
    }
    
    if (pnatcb == LW_NULL) {
        pnatcb = __natNew(&iphdr->src, usSrcPort, ucProto);             /*  新建控制块                  */
    }
    
    if (pnatcb) {
        pnatcb->NAT_ulIdleTimer = 0;                                    /*  刷新空闲时间                */
        
        /*
         *  将数据包转换为本机 AP 外网网络接口发送的数据包
         *  如果别名有效, 则使用别名作为源地址.
         */
        u32OldAddr = iphdr->src.addr;
        for (plineTemp  = _G_plineNatalias;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {             /*  查找别名表                  */
        
            pnatalias = _LIST_ENTRY(plineTemp, __NAT_ALIAS, NAT_lineManage);
            if ((pnatalias->NAT_ipaddrSLocalIp.addr <= ((ip4_addr_t *)&(iphdr->src))->addr) &&
                (pnatalias->NAT_ipaddrELocalIp.addr >= ((ip4_addr_t *)&(iphdr->src))->addr)) {
                break;
            }
        }
        if (plineTemp) {                                                /*  源 IP 使用别名 IP           */
            ((ip4_addr_t *)&(iphdr->src))->addr = pnatalias->NAT_ipaddrAliasIp.addr;
            
        } else {                                                        /*  源 IP 使用 AP 接口 IP       */
            ((ip4_addr_t *)&(iphdr->src))->addr = netif_ip4_addr(netifOut)->addr;
        }
        inet_chksum_adjust((u8_t *)&IPH_CHKSUM(iphdr),(u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->src.addr, 4);
        
        /*
         *  本机发送到外网的数据包使用 NAT_usAssPort (唯一的分配端口)
         */
        switch (IPH_PROTO(iphdr)) {
        
        case IP_PROTO_TCP:
            inet_chksum_adjust((u8_t *)&tcphdr->chksum,(u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->src.addr, 4);
            tcphdr->src = pnatcb->NAT_usAssPort;
            inet_chksum_adjust((u8_t *)&tcphdr->chksum,(u8_t *)&usSrcPort, 2, (u8_t *)&tcphdr->src, 2);
            break;
            
        case IP_PROTO_UDP:
            if (udphdr->chksum != 0) {
                inet_chksum_adjust((u8_t *)&udphdr->chksum,(u8_t *)&u32OldAddr, 4, (u8_t *)&iphdr->src.addr, 4);
        	    udphdr->src = pnatcb->NAT_usAssPort;
        	    inet_chksum_adjust((u8_t *)&udphdr->chksum,(u8_t *)&usSrcPort, 2, (u8_t *)&udphdr->src, 2);
        	}
            break;
            
        case IP_PROTO_ICMP:
            icmphdr->id = pnatcb->NAT_usAssPort;
            inet_chksum_adjust((u8_t *)&icmphdr->chksum,(u8_t *)&usSrcPort, 2, (u8_t *)&icmphdr->id, 2);
            break;
            
        default:
            _BugHandle(LW_TRUE, LW_TRUE, "NAT Protocol error!\r\n");
            break;
        }
        
        /*
         *  NAT 控制块状态更新
         */
        if (IPH_PROTO(iphdr) == IP_PROTO_TCP) {
            if (TCPH_FLAGS(tcphdr) & TCP_RST) {
	            pnatcb->NAT_iStatus = __NAT_STATUS_CLOSING;
            
            } else if (TCPH_FLAGS(tcphdr) & TCP_FIN) {
	            if (pnatcb->NAT_iStatus == __NAT_STATUS_OPEN) {
	                pnatcb->NAT_iStatus = __NAT_STATUS_FIN;
	            
	            } else {
	                pnatcb->NAT_iStatus = __NAT_STATUS_CLOSING;
	            }
            }
        }
    }

    return  (0);
}
/*********************************************************************************************************
** 函数名称: __natIphook
** 功能描述: NAT IP 回调
** 输　入  : iIpType   协议类型
**           iHookType 回调类型
**           p         数据包
**           pnetifIn  输入网络接口
**           pnetifOut 输出网络接口
** 输　出  : 0: ok 1: eaten
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __natIphook (INT  iIpType, INT  iHookType, struct pbuf  *p, 
                         struct netif  *pnetifIn, struct netif  *pnetifOut)
{
    INT             i, iRet = 0;
    struct ip_hdr  *iphdr;

    if (iIpType != IP_HOOK_V4) {
        return  (0);
    }
    
    switch (iHookType) {
    
    case IP_HT_PRE_ROUTING:                                             /*  网口接收                    */
        iphdr = (struct ip_hdr *)p->payload;
        for (i = 0; i < LW_CFG_NET_NAT_MAX_AP_IF; i++) {
            if (_G_natifAp[i].NATIF_pnetif == pnetifIn) {
                if (ip4_addr_cmp(&iphdr->dest, netif_ip4_addr(pnetifIn))) {
                    iRet = __natApInput(p, pnetifIn);
                    break;
                }
            }
        }
        break;
        
    case IP_HT_POST_ROUTING:                                            /*  网口发送                    */
        if (pnetifIn) {                                                 /*  非本机发送                  */
            for (i = 0; i < LW_CFG_NET_NAT_MAX_AP_IF; i++) {
                if (_G_natifAp[i].NATIF_pnetif == pnetifOut) {
                    iRet = __natApOutput(p, pnetifIn, pnetifOut);
                    break;
                }
            }
        }
        break;
        
    default:
        break;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __natInit
** 功能描述: NAT 初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __natInit (VOID)
{
    __natPoolInit();
    _G_ulNatcbTimer = API_TimerCreate("nat_timer", LW_OPTION_ITIMER | LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    _BugHandle((_G_ulNatcbTimer == LW_OBJECT_HANDLE_INVALID), LW_TRUE, "can not create 'nat_timer'\r\n");
}
/*********************************************************************************************************
** 函数名称: __natStart
** 功能描述: NAT 启动
** 输　入  : pcLocal    本地网络接口
**           pcAp       出口网络接口
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __natStart (CPCHAR  pcLocal, CPCHAR  pcAp)
{
    if (_G_bNatStart) {
        _ErrorHandle(EBUSY);
        return  (PX_ERROR);
    }
    
    lib_strlcpy(_G_natifAp[0].NATIF_cIfName, pcAp, IF_NAMESIZE);
    lib_strlcpy(_G_natifLocal[0].NATIF_cIfName, pcLocal, IF_NAMESIZE);
    
    _G_natifLocal[0].NATIF_pnetif = netif_find(pcLocal);
    _G_natifAp[0].NATIF_pnetif    = netif_find(pcAp);
    
    if (net_ip_hook_add("nat", __natIphook)) {
        return  (PX_ERROR);
    }
    
    API_TimerStart(_G_ulNatcbTimer, (LW_TICK_HZ * 60), LW_OPTION_AUTO_RESTART,
                   (PTIMER_CALLBACK_ROUTINE)__natTimer, (PVOID)LW_CFG_NET_NAT_IDLE_TIMEOUT);
    
    __NAT_LOCK();
    _G_bNatStart = LW_TRUE;
    __NAT_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __natStop
** 功能描述: NAT 结束
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __natStop (VOID)
{
    INT  i;

    if (net_ip_hook_delete(__natIphook)) {
        return  (PX_ERROR);
    }
    
    API_TimerCancel(_G_ulNatcbTimer);
    __natTimer(0);
    
    __NAT_LOCK();
    for (i = 1; i < LW_CFG_NET_NAT_MAX_AP_IF; i++) {
        _G_natifAp[i].NATIF_cIfName[0] = PX_EOS;
        _G_natifAp[i].NATIF_pnetif     = LW_NULL;
    }
    
    for (i = 1; i < LW_CFG_NET_NAT_MAX_LOCAL_IF; i++) {
        _G_natifLocal[i].NATIF_cIfName[0] = PX_EOS;
        _G_natifLocal[i].NATIF_pnetif     = LW_NULL;
    }
    _G_bNatStart = LW_FALSE;
    __NAT_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __natAddLocal
** 功能描述: NAT 增加一个本地网络接口
** 输　入  : pcLocal    本地网络接口
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __natAddLocal (CPCHAR  pcLocal)
{
    INT  i;

    __NAT_LOCK();
    if (!_G_bNatStart) {
        __NAT_UNLOCK();
        _ErrorHandle(ENOTCONN);
        return  (PX_ERROR);
    }
    for (i = 0; i < LW_CFG_NET_NAT_MAX_LOCAL_IF; i++) {
        if (_G_natifLocal[i].NATIF_cIfName[0] == PX_EOS) {
            lib_strlcpy(_G_natifLocal[i].NATIF_cIfName, pcLocal, IF_NAMESIZE);
            break;
        }
    }
    if (i >= LW_CFG_NET_NAT_MAX_LOCAL_IF) {
        __NAT_UNLOCK();
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }
    __NAT_UNLOCK();
    
    _G_natifLocal[i].NATIF_pnetif = netif_find(pcLocal);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __natAddAp
** 功能描述: NAT 增加一个出口网络接口
** 输　入  : pcAp       出口网络接口
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __natAddAp (CPCHAR  pcAp)
{
    INT  i;

    __NAT_LOCK();
    if (!_G_bNatStart) {
        __NAT_UNLOCK();
        _ErrorHandle(ENOTCONN);
        return  (PX_ERROR);
    }
    for (i = 0; i < LW_CFG_NET_NAT_MAX_AP_IF; i++) {
        if (_G_natifAp[i].NATIF_cIfName[0] == PX_EOS) {
            lib_strlcpy(_G_natifAp[i].NATIF_cIfName, pcAp, IF_NAMESIZE);
            break;
        }
    }
    if (i >= LW_CFG_NET_NAT_MAX_AP_IF) {
        __NAT_UNLOCK();
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }
    __NAT_UNLOCK();
    
    _G_natifAp[i].NATIF_pnetif = netif_find(pcAp);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  NAT proc
*********************************************************************************************************/
#if LW_CFG_PROCFS_EN > 0
/*********************************************************************************************************
  NAT proc 文件函数声明
*********************************************************************************************************/
static ssize_t  __procFsNatSummaryRead(PLW_PROCFS_NODE  p_pfsn, 
                                       PCHAR            pcBuffer, 
                                       size_t           stMaxBytes,
                                       off_t            oft);
static ssize_t  __procFsNatAssNodeRead(PLW_PROCFS_NODE  p_pfsn, 
                                       PCHAR            pcBuffer, 
                                       size_t           stMaxBytes,
                                       off_t            oft);
/*********************************************************************************************************
  NAT proc 文件操作函数组
*********************************************************************************************************/
static LW_PROCFS_NODE_OP        _G_pfsnoNatSummaryFuncs = {
    __procFsNatSummaryRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoNatAssNodeFuncs = {
    __procFsNatAssNodeRead, LW_NULL
};
/*********************************************************************************************************
  NAT proc 文件目录树
*********************************************************************************************************/
static LW_PROCFS_NODE           _G_pfsnNat[] = 
{
    LW_PROCFS_INIT_NODE("nat", 
                        (S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH), 
                        LW_NULL, 
                        LW_NULL,  
                        0),
                        
    LW_PROCFS_INIT_NODE("info", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNatSummaryFuncs, 
                        "I",
                        0),
                        
    LW_PROCFS_INIT_NODE("assnode", 
                        (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH), 
                        &_G_pfsnoNatAssNodeFuncs, 
                        "A",
                        0),
};
/*********************************************************************************************************
** 函数名称: __procFsNatSummaryRead
** 功能描述: procfs 读一个内核 nat/info proc 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsNatSummaryRead (PLW_PROCFS_NODE  p_pfsn, 
                                        PCHAR            pcBuffer, 
                                        size_t           stMaxBytes,
                                        off_t            oft)
{
    const CHAR   cAliasInfoHeader[] = "\n"
    "     ALIAS        LOCAL START     LOCAL END\n"
    "--------------- --------------- ---------------\n";

    const CHAR   cMapInfoHeader[] = "\n"
    " ASS PORT  LOCAL PORT    LOCAL IP      IP CNT   PROTO\n"
    "---------- ---------- --------------- -------- -------\n";
    
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  实际的文件内容大小          */
          size_t    stCopeBytes;
          
    /*
     *  由于预置内存大小为 0 , 所以打开后第一次读取需要手动开辟内存.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  还没有分配内存              */
        size_t          stNeedBufferSize;
        UINT            i;
        UINT            uiAssNum = 0;
        PLW_LIST_LINE   plineTemp;
        PCHAR           pcProto;
        __PNAT_ALIAS    pnatalias;
        __PNAT_MAP      pnatmap;
        
        stNeedBufferSize = 512;                                         /*  初始大小                    */
        
        __NAT_LOCK();
        for (plineTemp  = _G_plineNatalias;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {             /*  别名表                      */
            stNeedBufferSize += 64;
        }
        for (plineTemp  = _G_plineNatmap;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            stNeedBufferSize += 64;
        }
        __NAT_UNLOCK();
        
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  重新获得文件缓冲区地址      */
        
        __NAT_LOCK();
        if (!_G_bNatStart) {
            __NAT_UNLOCK();
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, 
                                  "NAT networking off!\n");
        
        } else {
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, 
                                  "NAT networking alias setting >>\n");
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "%s", cAliasInfoHeader);
            
            for (plineTemp  = _G_plineNatalias;
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {         /*  别名表                      */
            
                CHAR    cIpBuffer1[INET_ADDRSTRLEN];
                CHAR    cIpBuffer2[INET_ADDRSTRLEN];
                CHAR    cIpBuffer3[INET_ADDRSTRLEN];
                
                pnatalias  = _LIST_ENTRY(plineTemp, __NAT_ALIAS, NAT_lineManage);
                stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                      "%-15s %-15s %-15s\n", 
                                      ip4addr_ntoa_r(&pnatalias->NAT_ipaddrAliasIp,
                                                     cIpBuffer1, INET_ADDRSTRLEN),
                                      ip4addr_ntoa_r(&pnatalias->NAT_ipaddrSLocalIp, 
                                                     cIpBuffer2, INET_ADDRSTRLEN),
                                      ip4addr_ntoa_r(&pnatalias->NAT_ipaddrELocalIp, 
                                                     cIpBuffer3, INET_ADDRSTRLEN));
            }
            
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "\nNAT networking direct map setting >>\n");
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "%s", cMapInfoHeader);
                                  
            for (plineTemp  = _G_plineNatmap;
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                
                CHAR    cIpBuffer[INET_ADDRSTRLEN];
                
                pnatmap = _LIST_ENTRY(plineTemp, __NAT_MAP, NAT_lineManage);
                switch (pnatmap->NAT_ucProto) {
                
                case IP_PROTO_ICMP: pcProto = "ICMP"; break;
                case IP_PROTO_UDP:  pcProto = "UDP";  break;
                case IP_PROTO_TCP:  pcProto = "TCP";  break;
                default:            pcProto = "?";    break;
                }
                
                stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                      "%10d %10d %-15s %8d %s\n",
                                      PP_NTOHS(pnatmap->NAT_usAssPort),
                                      PP_NTOHS(pnatmap->NAT_usLocalPort),
                                      ip4addr_ntoa_r(&pnatmap->NAT_ipaddrLocalIp, 
                                                     cIpBuffer, INET_ADDRSTRLEN),
                                      pnatmap->NAT_usLocalCnt,
                                      pcProto);
            }
            
            for (plineTemp  = _G_plineNatcbTcp;
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                uiAssNum++;
            }
            for (plineTemp  = _G_plineNatcbUdp;
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                uiAssNum++;
            }
            for (plineTemp  = _G_plineNatcbIcmp;
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                uiAssNum++;
            }
            
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "\nNAT networking summary >>\n");
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "    LAN: ");
                                  
            for (i = 0; i < LW_CFG_NET_NAT_MAX_LOCAL_IF; i++) {
                if (_G_natifLocal[i].NATIF_cIfName[0]) {
                    stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                          "%s ", _G_natifLocal[i].NATIF_cIfName);
                }
            }
            
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "\n    WAN: ");
            
            for (i = 0; i < LW_CFG_NET_NAT_MAX_AP_IF; i++) {
                if (_G_natifAp[i].NATIF_cIfName[0]) {
                    stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                          "%s ", _G_natifAp[i].NATIF_cIfName);
                }
            }
            __NAT_UNLOCK();
            
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "\n    Total Ass-node: %d\n", LW_CFG_NET_NAT_MAX_SESSION);
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "    Used  Ass-node: %d\n", uiAssNum);
        }
        
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    } else {
        stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  计算实际拷贝的字节数        */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** 函数名称: __procFsNatAssNodeRead
** 功能描述: procfs 读一个内核 nat/assnode proc 文件
** 输　入  : p_pfsn        节点控制块
**           pcBuffer      缓冲区
**           stMaxBytes    缓冲区大小
**           oft           文件指针
** 输　出  : 实际读取的数目
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  __procFsNatAssNodeRead (PLW_PROCFS_NODE  p_pfsn, 
                                        PCHAR            pcBuffer, 
                                        size_t           stMaxBytes,
                                        off_t            oft)
{
    const CHAR   cNatInfoHeader[] = "\n"
    "    LOCAL IP    LOCAL PORT ASS PORT PROTO IDLE(min)  STATUS\n"
    "--------------- ---------- -------- ----- --------- --------\n";
    
          PCHAR     pcFileBuffer;
          size_t    stRealSize;                                         /*  实际的文件内容大小          */
          size_t    stCopeBytes;
          
    /*
     *  由于预置内存大小为 0 , 所以打开后第一次读取需要手动开辟内存.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {                                      /*  还没有分配内存              */
        size_t          stNeedBufferSize;
        PCHAR           pcProto;
        PCHAR           pcStatus;
        __PNAT_CB       pnatcb = LW_NULL;
        PLW_LIST_LINE   plineTemp;
        
        stNeedBufferSize = 256;                                         /*  初始大小                    */
        
        __NAT_LOCK();
        for (plineTemp  = _G_plineNatcbTcp;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            stNeedBufferSize += 64;
        }
        for (plineTemp  = _G_plineNatcbUdp;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            stNeedBufferSize += 64;
        }
        for (plineTemp  = _G_plineNatcbIcmp;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            stNeedBufferSize += 64;
        }
        __NAT_UNLOCK();
    
        if (API_ProcFsAllocNodeBuffer(p_pfsn, stNeedBufferSize)) {
            _ErrorHandle(ENOMEM);
            return  (0);
        }
        pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);             /*  重新获得文件缓冲区地址      */
        
        stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, 0, cNatInfoHeader); 
                                                                        /*  打印头信息                  */
        __NAT_LOCK();
        for (plineTemp  = _G_plineNatcbTcp;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            struct in_addr  inaddr;
            CHAR            cIpBuffer[INET_ADDRSTRLEN];
            
            pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
            inaddr.s_addr = pnatcb->NAT_ipaddrLocalIp.addr;
            if (pnatcb->NAT_iStatus == __NAT_STATUS_OPEN) {
                pcStatus = "OPEN";
            } else if (pnatcb->NAT_iStatus == __NAT_STATUS_FIN) {
                pcStatus = "FIN";
            } else if (pnatcb->NAT_iStatus == __NAT_STATUS_CLOSING) {
                pcStatus = "CLOSING";
            } else {
                pcStatus = "?";
            }
            
            inet_ntoa_r(inaddr, cIpBuffer, INET_ADDRSTRLEN);
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "%-15s %10d %8d %-5s %9ld %-8s\n", cIpBuffer,
                                  PP_NTOHS(pnatcb->NAT_usLocalPort),
                                  PP_NTOHS(pnatcb->NAT_usAssPort),
                                  "TCP",
                                  pnatcb->NAT_ulIdleTimer,
                                  pcStatus);
        }
        
        for (plineTemp  = _G_plineNatcbUdp;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            struct in_addr  inaddr;
            CHAR            cIpBuffer[INET_ADDRSTRLEN];

            pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
            inaddr.s_addr = pnatcb->NAT_ipaddrLocalIp.addr;
            if (pnatcb->NAT_iStatus == __NAT_STATUS_OPEN) {
                pcStatus = "OPEN";
            } else if (pnatcb->NAT_iStatus == __NAT_STATUS_FIN) {
                pcStatus = "FIN";
            } else if (pnatcb->NAT_iStatus == __NAT_STATUS_CLOSING) {
                pcStatus = "CLOSING";
            } else {
                pcStatus = "?";
            }
            
            inet_ntoa_r(inaddr, cIpBuffer, INET_ADDRSTRLEN);
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "%-15s %10d %8d %-5s %9ld %-8s\n", cIpBuffer,
                                  PP_NTOHS(pnatcb->NAT_usLocalPort),
                                  PP_NTOHS(pnatcb->NAT_usAssPort),
                                  "UDP",
                                  pnatcb->NAT_ulIdleTimer,
                                  pcStatus);
        }
        
        for (plineTemp  = _G_plineNatcbIcmp;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            
            struct in_addr  inaddr;
            CHAR            cIpBuffer[INET_ADDRSTRLEN];
            
            pnatcb = _LIST_ENTRY(plineTemp, __NAT_CB, NAT_lineManage);
            inaddr.s_addr = pnatcb->NAT_ipaddrLocalIp.addr;
            if (pnatcb->NAT_ucProto == IP_PROTO_ICMP) {
                pcProto = "ICMP";
            } else {
                pcProto = "?";
            }
            
            if (pnatcb->NAT_iStatus == __NAT_STATUS_OPEN) {
                pcStatus = "OPEN";
            } else if (pnatcb->NAT_iStatus == __NAT_STATUS_FIN) {
                pcStatus = "FIN";
            } else if (pnatcb->NAT_iStatus == __NAT_STATUS_CLOSING) {
                pcStatus = "CLOSING";
            } else {
                pcStatus = "?";
            }
            
            inet_ntoa_r(inaddr, cIpBuffer, INET_ADDRSTRLEN);
            stRealSize = bnprintf(pcFileBuffer, stNeedBufferSize, stRealSize, 
                                  "%-15s %10d %8d %-5s %9ld %-8s\n", cIpBuffer,
                                  PP_NTOHS(pnatcb->NAT_usLocalPort),
                                  PP_NTOHS(pnatcb->NAT_usAssPort),
                                  pcProto,
                                  pnatcb->NAT_ulIdleTimer,
                                  pcStatus);
        }
        __NAT_UNLOCK();
        
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    } else {
        stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  计算实际拷贝的字节数        */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** 函数名称: __procFsNatInit
** 功能描述: procfs 初始化内核 NAT 文件
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __procFsNatInit (VOID)
{
    API_ProcFsMakeNode(&_G_pfsnNat[0], "/net");
    API_ProcFsMakeNode(&_G_pfsnNat[1], "/net/nat");
    API_ProcFsMakeNode(&_G_pfsnNat[2], "/net/nat");
}

#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_ROUTER > 0       */
                                                                        /*  LW_CFG_NET_NAT_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

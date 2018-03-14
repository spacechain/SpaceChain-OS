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
** 文   件   名: lwip_loginbl.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 01 月 09 日
**
** 描        述: 网络登录黑名单管理.
**
** BUG:
2017.04.10  增加白名单功能.
2017.12.28  修正可能产生的拒绝服务攻击缺陷.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "lwip/opt.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "sys/socket.h"
#if LW_CFG_NET_LOGINBL_EN > 0
#include "lwip/tcpip.h"
#include "lwip/pbuf.h"
#include "lwip/ip4.h"
/*********************************************************************************************************
  黑名单节点
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE    LBL_lineManage;                                     /*  management list             */
    ip4_addr_t      LBL_ipaddr;
    UINT            LBL_uiRep;                                          /*  生效次数                    */
    UINT            LBL_uiCnt;                                          /*  重复次数                    */
    UINT            LBL_uiSec;                                          /*  超时时间                    */
} LW_LOGINBL_NODE;
typedef LW_LOGINBL_NODE     *PLW_LOGINBL_NODE;
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
#define LW_LBL_TABLESIZE    64                                          /*  2 ^ 6 个入口                */
#define LW_LBL_TABLEMASK    (LW_LBL_TABLESIZE - 1)
#define LW_LBL_TABLESHIFT   (32 - 6)                                    /*  使用 ip 地址高 6 位为 hash  */

#define LW_LBL_HASHINDEX(pipaddr)    \
        (((pipaddr)->addr >> LW_LBL_TABLESHIFT) & LW_LBL_TABLEMASK)
        
static PLW_LIST_LINE    _G_plineLblHashHeader[LW_LBL_TABLESIZE];
static PLW_LIST_LINE    _G_plineLwlHashHeader[LW_LBL_TABLESIZE];
static LW_OBJECT_HANDLE _G_ulLblTimer = LW_OBJECT_HANDLE_INVALID;
static UINT             _G_uiLblCnt   = 0;
static UINT             _G_uiLwlCnt   = 0;

#if LWIP_TCPIP_CORE_LOCKING < 1
#error sylixos need LWIP_TCPIP_CORE_LOCKING > 0
#endif                                                                  /*  LWIP_TCPIP_CORE_LOCKING     */
/*********************************************************************************************************
** 函数名称: loginbl_input_hook
** 功能描述: 网络地址黑名单 ip 输入回调
** 输　入  : p         数据包
**           inp       网络接口
** 输　出  : 1: 不允许通过  0: 放行
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  loginbl_input_hook (struct pbuf *p, struct netif *inp)
{
    struct ip_hdr      *iphdr = (struct ip_hdr *)p->payload;
    INT                 iHash = LW_LBL_HASHINDEX(&iphdr->src);
    PLW_LIST_LINE       plineTemp;
    PLW_LOGINBL_NODE    plbl;
    
    if (IPH_V(iphdr) != 4) {
        return  (0);                                                    /*  IPv4 有效                   */
    }
    
    for (plineTemp  = _G_plineLwlHashHeader[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        plbl = _LIST_ENTRY(plineTemp, LW_LOGINBL_NODE, LBL_lineManage);
        if (plbl->LBL_ipaddr.addr == iphdr->src.addr) {
            return  (0);                                                /*  在白名单中                  */
        }
    }
    
    for (plineTemp  = _G_plineLblHashHeader[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        plbl = _LIST_ENTRY(plineTemp, LW_LOGINBL_NODE, LBL_lineManage);
        if (plbl->LBL_ipaddr.addr == iphdr->src.addr) {
            if (plbl->LBL_uiCnt >= plbl->LBL_uiRep) {
                pbuf_free(p);                                           /*  在黑名单内                  */
                return  (1);
            
            } else {
                break;                                                  /*  重试次数未到, 暂时放行      */
            }
        }
    }
    
    return  (0);
}
/*********************************************************************************************************
** 函数名称: __LoginBlFind
** 功能描述: 查找指定 IP 控制块
** 输　入  : ipaddr        IP 地址
**           piHash        返回 hash index
** 输　出  : 黑名单控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PLW_LOGINBL_NODE  __LoginBlFind (ip4_addr_t  ipaddr)
{
    INT                 iHash = LW_LBL_HASHINDEX(&ipaddr);
    PLW_LIST_LINE       plineTemp;
    PLW_LOGINBL_NODE    plbl;
    
    for (plineTemp  = _G_plineLblHashHeader[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        plbl = _LIST_ENTRY(plineTemp, LW_LOGINBL_NODE, LBL_lineManage);
        if (plbl->LBL_ipaddr.addr == ipaddr.addr) {
            return  (plbl);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __LoginWlFind
** 功能描述: 查找指定 IP 控制块
** 输　入  : ipaddr        IP 地址
**           piHash        返回 hash index
** 输　出  : 黑名单控制块
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PLW_LOGINBL_NODE  __LoginWlFind (ip4_addr_t  ipaddr)
{
    INT                 iHash = LW_LBL_HASHINDEX(&ipaddr);
    PLW_LIST_LINE       plineTemp;
    PLW_LOGINBL_NODE    plbl;
    
    for (plineTemp  = _G_plineLwlHashHeader[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        plbl = _LIST_ENTRY(plineTemp, LW_LOGINBL_NODE, LBL_lineManage);
        if (plbl->LBL_ipaddr.addr == ipaddr.addr) {
            return  (plbl);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __LoginBlTimer
** 功能描述: 定时器回调
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __LoginBlTimer (VOID)
{
    INT                 i;
    PLW_LIST_LINE       plineTemp;
    PLW_LIST_LINE       plineFree = LW_NULL;
    PLW_LOGINBL_NODE    plbl;
    
    if (!_G_uiLblCnt) {
        return;
    }
    
    LOCK_TCPIP_CORE();                                                  /*  锁定协议栈                  */
    for (i = 0; i < LW_LBL_TABLESIZE; i++) {
        plineTemp = _G_plineLblHashHeader[i];
        while (plineTemp) {
            plbl = _LIST_ENTRY(plineTemp, LW_LOGINBL_NODE, LBL_lineManage);
            plineTemp = _list_line_get_next(plineTemp);
            
            if (plbl->LBL_uiSec) {
                plbl->LBL_uiSec--;
                if (plbl->LBL_uiSec == 0) {
                    _G_uiLblCnt--;
                    _List_Line_Del(&plbl->LBL_lineManage, &_G_plineLblHashHeader[i]);
                    _List_Line_Add_Tail(&plbl->LBL_lineManage, &plineFree);
                }
            }
        }
    }
    UNLOCK_TCPIP_CORE();                                                /*  解锁协议栈                  */
    
    while (plineFree) {
        plbl = _LIST_ENTRY(plineFree, LW_LOGINBL_NODE, LBL_lineManage);
        _List_Line_Del(&plbl->LBL_lineManage, &plineFree);
        __SHEAP_FREE(plbl);
    }
}
/*********************************************************************************************************
** 函数名称: __LoginBlIsLocal
** 功能描述: 检查是否为内网地址
** 输　入  : pipaddr   IP 地址
** 输　出  : 是否为直连机器
** 全局变量: 
** 调用模块: 
** 注  意  : 我们假设伪造源地址的数据包不能够返回到发送者, 
             所以使用此方法可以有效避免来自外网的拒绝服务攻击.
*********************************************************************************************************/
static BOOL  __LoginBlIsLocal (ip4_addr_t *pipaddr)
{
#define LW_LBL_NETIF_AVLID(pnetif) \
        ((netif_ip4_addr(pnetif)->addr != IPADDR_ANY) && \
         (netif_ip4_netmask(pnetif)->addr != IPADDR_ANY) && \
         (netif_ip4_gw(pnetif)->addr != IPADDR_ANY))

    struct netif *pnetif;
    
    NETIF_FOREACH(pnetif) {
        if (LW_LBL_NETIF_AVLID(pnetif)) {
            if (ip4_addr_netcmp(netif_ip4_addr(pnetif), 
                                pipaddr, netif_ip4_netmask(pnetif))) {
                return  (LW_TRUE);
            }
        }
    }
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: API_LoginBlAdd
** 功能描述: 将一个网络地址添加到黑名单
** 输　入  : addr      网络地址
**           uiRep     连续出现多少次则添加
**           uiSec     黑名单时间 (0 为永久存在)
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_LoginBlAdd (const struct sockaddr *addr, UINT  uiRep, UINT  uiSec)
{
    PLW_LOGINBL_NODE    plbl;
    PLW_LOGINBL_NODE    plblNew;
    ip4_addr_t          ipaddr;
    
    if (!addr || (addr->sa_family != AF_INET)) {
        _ErrorHandle(EINVAL);
        return  (ERROR_NONE);
    }
    
    if (uiRep > 10) {
        uiRep = 10;
    }
    
    if (uiSec > 100000) {
        uiSec = 100000;
    }

    if (_G_ulLblTimer == LW_OBJECT_HANDLE_INVALID) {
        _G_ulLblTimer =  API_TimerCreate("loginbl", LW_OPTION_ITIMER, LW_NULL);
        if (_G_ulLblTimer) {
            API_TimerStart(_G_ulLblTimer, LW_TICK_HZ, LW_OPTION_AUTO_RESTART,
                           (PTIMER_CALLBACK_ROUTINE)__LoginBlTimer, LW_NULL);
        }
    }
    
    ipaddr.addr = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
    
    LOCK_TCPIP_CORE();                                                  /*  锁定协议栈                  */
    if (__LoginBlIsLocal(&ipaddr)) {
        UNLOCK_TCPIP_CORE();
        return  (ERROR_NONE);                                           /*  不锁定本地机器              */
    }
    
    plbl = __LoginBlFind(ipaddr);
    if (plbl) {
        plbl->LBL_uiRep = uiRep;
        plbl->LBL_uiSec = uiSec;
        plbl->LBL_uiCnt++;
        if (plbl->LBL_uiCnt >= uiRep) {
            plbl->LBL_uiCnt =  uiRep;
        }
        UNLOCK_TCPIP_CORE();
        return  (ERROR_NONE);
    
    } else if (_G_uiLblCnt >= LW_CFG_NET_LOGINBL_MAX_NODE) {            /*  已经满了                    */
        UNLOCK_TCPIP_CORE();
        _ErrorHandle(EXFULL);
        return  (PX_ERROR);
    }
    UNLOCK_TCPIP_CORE();                                                /*  解锁协议栈                  */
    
    plblNew = (PLW_LOGINBL_NODE)__SHEAP_ALLOC(sizeof(LW_LOGINBL_NODE));
    if (plblNew == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    plblNew->LBL_uiRep  = uiRep;
    plblNew->LBL_uiSec  = uiSec;
    plblNew->LBL_uiCnt  = 1;
    plblNew->LBL_ipaddr = ipaddr;
    
    LOCK_TCPIP_CORE();                                                  /*  锁定协议栈                  */
    plbl = __LoginBlFind(ipaddr);
    if (plbl) {
        plbl->LBL_uiRep = uiRep;
        plbl->LBL_uiSec = uiSec;
        plbl->LBL_uiCnt++;
        if (plbl->LBL_uiCnt >= uiRep) {
            plbl->LBL_uiCnt =  uiRep;
        }
    } else {
        INT  iHash = LW_LBL_HASHINDEX(&ipaddr);
        _G_uiLblCnt++;
        _List_Line_Add_Tail(&plblNew->LBL_lineManage, &_G_plineLblHashHeader[iHash]);
        UNLOCK_TCPIP_CORE();
        return  (ERROR_NONE);
    }
    UNLOCK_TCPIP_CORE();                                                /*  解锁协议栈                  */
    
    __SHEAP_FREE(plblNew);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_LoginWlAdd
** 功能描述: 将一个网络地址添加到白名单
** 输　入  : addr      网络地址
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_LoginWlAdd (const struct sockaddr *addr)
{
    PLW_LOGINBL_NODE    plbl;
    PLW_LOGINBL_NODE    plblNew;
    ip4_addr_t          ipaddr;
    
    if (!addr || (addr->sa_family != AF_INET)) {
        _ErrorHandle(EINVAL);
        return  (ERROR_NONE);
    }
    
    ipaddr.addr = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
    
    LOCK_TCPIP_CORE();                                                  /*  锁定协议栈                  */
    plbl = __LoginWlFind(ipaddr);
    if (plbl) {
        UNLOCK_TCPIP_CORE();
        return  (ERROR_NONE);
    }
    UNLOCK_TCPIP_CORE();                                                /*  解锁协议栈                  */
    
    plblNew = (PLW_LOGINBL_NODE)__SHEAP_ALLOC(sizeof(LW_LOGINBL_NODE));
    if (plblNew == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    plblNew->LBL_uiRep  = 0;
    plblNew->LBL_uiSec  = 0;
    plblNew->LBL_uiCnt  = 0;
    plblNew->LBL_ipaddr = ipaddr;
    
    LOCK_TCPIP_CORE();                                                  /*  锁定协议栈                  */
    plbl = __LoginWlFind(ipaddr);
    if (plbl == LW_NULL) {
        INT  iHash = LW_LBL_HASHINDEX(&ipaddr);
        _G_uiLwlCnt++;
        _List_Line_Add_Tail(&plblNew->LBL_lineManage, &_G_plineLwlHashHeader[iHash]);
        UNLOCK_TCPIP_CORE();
        return  (ERROR_NONE);
    }
    UNLOCK_TCPIP_CORE();                                                /*  解锁协议栈                  */
    
    __SHEAP_FREE(plblNew);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_LoginBlDelete
** 功能描述: 将一个网络地址从黑名单删除
** 输　入  : addr      网络地址 (NULL 表示全部清除)
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_LoginBlDelete (const struct sockaddr *addr)
{
    ip4_addr_t          ipaddr;
    PLW_LOGINBL_NODE    plbl;
    
    if (!addr || (addr->sa_family != AF_INET)) {
        _ErrorHandle(EINVAL);
        return  (ERROR_NONE);
    }
    
    ipaddr.addr = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
    
    LOCK_TCPIP_CORE();                                                  /*  锁定协议栈                  */
    plbl = __LoginBlFind(ipaddr);
    if (plbl) {
        INT  iHash = LW_LBL_HASHINDEX(&ipaddr);
        _G_uiLblCnt--;
        _List_Line_Del(&plbl->LBL_lineManage, &_G_plineLblHashHeader[iHash]);
    }
    UNLOCK_TCPIP_CORE();                                                /*  解锁协议栈                  */
    
    if (plbl) {
        __SHEAP_FREE(plbl);
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: API_LoginWlDelete
** 功能描述: 将一个网络地址从白名单删除
** 输　入  : addr      网络地址 (NULL 表示全部清除)
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_LoginWlDelete (const struct sockaddr *addr)
{
    ip4_addr_t          ipaddr;
    PLW_LOGINBL_NODE    plbl;
    
    if (!addr || (addr->sa_family != AF_INET)) {
        _ErrorHandle(EINVAL);
        return  (ERROR_NONE);
    }
    
    ipaddr.addr = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
    
    LOCK_TCPIP_CORE();                                                  /*  锁定协议栈                  */
    plbl = __LoginWlFind(ipaddr);
    if (plbl) {
        INT  iHash = LW_LBL_HASHINDEX(&ipaddr);
        _G_uiLwlCnt--;
        _List_Line_Del(&plbl->LBL_lineManage, &_G_plineLwlHashHeader[iHash]);
    }
    UNLOCK_TCPIP_CORE();                                                /*  解锁协议栈                  */
    
    if (plbl) {
        __SHEAP_FREE(plbl);
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(EINVAL);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: API_LoginBlShow
** 功能描述: 显示网络地址黑名单
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
VOID  API_LoginBlShow (VOID)
{
    static const CHAR   cInfoHdr[] = 
    "      IP          SEC\n"
    "--------------- -------\n";

    PLW_LOGINBL_NODE    plbl;
    PLW_LIST_LINE       plineTemp;
    INT                 i;
    UINT                uiCnt;
    PCHAR               pcBuf;
    CHAR                cIp[IP4ADDR_STRLEN_MAX];
    size_t              stSize;
    size_t              stOffset = 0;
    
    LOCK_TCPIP_CORE();
    uiCnt = _G_uiLblCnt;
    UNLOCK_TCPIP_CORE();
    
    printf(cInfoHdr);
    if (!uiCnt) {
        return;
    }
    
    stSize = (size_t)uiCnt * 32;
    pcBuf  = (PCHAR)__SHEAP_ALLOC(stSize);
    if (pcBuf == LW_NULL) {
        fprintf(stderr, "no memory\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return;
    }
    
    LOCK_TCPIP_CORE();                                                  /*  锁定协议栈                  */
    for (i = 0; i < LW_LBL_TABLESIZE; i++) {
        for (plineTemp  = _G_plineLblHashHeader[i];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            plbl = _LIST_ENTRY(plineTemp, LW_LOGINBL_NODE, LBL_lineManage);
            if (plbl->LBL_uiCnt >= plbl->LBL_uiRep) {
                if (plbl->LBL_uiSec) {
                    stOffset = bnprintf(pcBuf, stSize, stOffset, 
                                        "%-15s %7u\n", 
                                        ip4addr_ntoa_r(&plbl->LBL_ipaddr, cIp, IP4ADDR_STRLEN_MAX),
                                        plbl->LBL_uiSec);
                } else {
                    stOffset = bnprintf(pcBuf, stSize, stOffset, 
                                        "%-15s %7s\n", 
                                        ip4addr_ntoa_r(&plbl->LBL_ipaddr, cIp, IP4ADDR_STRLEN_MAX),
                                        "Forever");
                }
            }
        }
    }
    UNLOCK_TCPIP_CORE();                                                /*  解锁协议栈                  */
    
    if (stOffset > 0) {
        fwrite(pcBuf, stOffset, 1, stdout);
        fflush(stdout);                                                 /*  这里必须确保输出完成        */
    }
    
    __SHEAP_FREE(pcBuf);
}
/*********************************************************************************************************
** 函数名称: API_LoginWlShow
** 功能描述: 显示网络地址白名单
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
VOID  API_LoginWlShow (VOID)
{
    static const CHAR   cInfoHdr[] = 
    "      IP\n"
    "---------------\n";

    PLW_LOGINBL_NODE    plbl;
    PLW_LIST_LINE       plineTemp;
    INT                 i;
    UINT                uiCnt;
    PCHAR               pcBuf;
    CHAR                cIp[IP4ADDR_STRLEN_MAX];
    size_t              stSize;
    size_t              stOffset = 0;
    
    LOCK_TCPIP_CORE();
    uiCnt = _G_uiLwlCnt;
    UNLOCK_TCPIP_CORE();
    
    printf(cInfoHdr);
    if (!uiCnt) {
        return;
    }
    
    stSize = (size_t)uiCnt * 24;
    pcBuf  = (PCHAR)__SHEAP_ALLOC(stSize);
    if (pcBuf == LW_NULL) {
        fprintf(stderr, "no memory\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return;
    }
    
    LOCK_TCPIP_CORE();                                                  /*  锁定协议栈                  */
    for (i = 0; i < LW_LBL_TABLESIZE; i++) {
        for (plineTemp  = _G_plineLwlHashHeader[i];
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
             
            plbl = _LIST_ENTRY(plineTemp, LW_LOGINBL_NODE, LBL_lineManage);
            stOffset = bnprintf(pcBuf, stSize, stOffset, 
                                "%-15s\n", 
                                ip4addr_ntoa_r(&plbl->LBL_ipaddr, cIp, IP4ADDR_STRLEN_MAX));
        }
    }
    UNLOCK_TCPIP_CORE();                                                /*  解锁协议栈                  */
    
    if (stOffset > 0) {
        fwrite(pcBuf, stOffset, 1, stdout);
        fflush(stdout);                                                 /*  这里必须确保输出完成        */
    }
    
    __SHEAP_FREE(pcBuf);
}

#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/

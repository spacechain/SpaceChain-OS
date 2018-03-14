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
** 文   件   名: lwip_hosttable.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 08 月 19 日
**
** 描        述: lwip 动态 DNS 本地主机表.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "limits.h"
#include "unistd.h"
#include "netdb.h"
#include "lwip/inet.h"
#include "lwip/dns.h"
#include "netinet6/in6.h"
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define __LW_HOSTTABLE_HASH_SIZE            17                          /*  hash 表大小                 */
/*********************************************************************************************************
  hash 函数声明
*********************************************************************************************************/
INT  __hashHorner(CPCHAR  pcKeyword, INT  iTableSize);
/*********************************************************************************************************
  表类型定义
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE                HSTTBL_lineManage;                      /*  管理链表                    */
    struct in_addr              HSTTBL_inaddr;                          /*  地址                        */
    CHAR                        HSTTBL_cHostName[1];                    /*  主机名                      */
} __LW_HOSTTABLE_ITEM;
typedef __LW_HOSTTABLE_ITEM    *__PLW_HOSTTABLE_ITEM;
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_OBJECT_HANDLE         _G_ulHostTableLock;
static LW_LIST_LINE_HEADER      _G_plineHostTableHash[__LW_HOSTTABLE_HASH_SIZE];
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
static INT  __tshellHostTable(INT  iArgC, PCHAR  *ppcArgV);
/*********************************************************************************************************
** 函数名称: __inetHostTableInit
** 功能描述: 初始化本地动态主机域名表
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __inetHostTableInit (VOID)
{
    
    _G_ulHostTableLock = API_SemaphoreBCreate("hosttable_lock", LW_TRUE, 
                                              LW_OPTION_OBJECT_GLOBAL, LW_NULL);

#if LW_CFG_SHELL_EN > 0
    API_TShellKeywordAdd("hosttable", __tshellHostTable);
    API_TShellFormatAdd("hosttable",  " [-s host addr] | [-d host] ");
    API_TShellHelpAdd("hosttable",    "show/add/delete local host table.\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
}
/*********************************************************************************************************
** 函数名称: __inetHostTableGetItem
** 功能描述: lwip 调用此函数查询本地动态主机域名表.
** 输　入  : pcHost        主机名
**           addr          返回地址
**           ucAddrType    地址类型
** 输　出  : ERR_OK or ERR_IF.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __inetHostTableGetItem (CPCHAR  pcHost, PVOID  pvAddr, UINT8  ucAddrType)
{
    REGISTER INT                    iHash;
    REGISTER PLW_LIST_LINE          plineTemp;
             __PLW_HOSTTABLE_ITEM   phsttablitem;
             ip_addr_t             *addr   = (ip_addr_t *)pvAddr;
             u32_t                  uiAddr = INADDR_NONE;
             CHAR                   cHostName[HOST_NAME_MAX + 1];
             CHAR                   cHostsBuffer[MAX_FILENAME_LENGTH];
    struct hostent                 *phostent;
             FILE                  *fpHosts;
             BOOL                   bHostsFund = LW_FALSE;
             INT                    i;
    
    if (!addr || !pcHost) {
        return  (ERR_IF);
    }
    
    gethostname(cHostName, HOST_NAME_MAX + 1);                          /*  本地主机地址                */
    if (lib_strcmp(cHostName, pcHost) == 0) {
        if (ucAddrType != LWIP_DNS_ADDRTYPE_IPV6) {
            ip_2_ip4(addr)->addr = htonl(INADDR_LOOPBACK);
            IP_SET_TYPE(addr, IPADDR_TYPE_V4);
        
        } else {
            lib_memcpy(ip_2_ip6(addr)->addr, &in6addr_loopback, 16);
            IP_SET_TYPE(addr, IPADDR_TYPE_V6);
        }
        return  (ERR_OK);
    }
    
    fpHosts = fopen(_PATH_HOSTS, "r");                                  /*  /etc/hosts                  */
    if (fpHosts) {
        phostent = gethostent2_r(fpHosts, cHostsBuffer, sizeof(cHostsBuffer));
        while (phostent) {
            if (lib_strcmp(pcHost, phostent->h_name) == 0) {
                bHostsFund = LW_TRUE;
                break;
            }
            for (i = 0; phostent->h_aliases[i]; i++) {
                if (lib_strcmp(pcHost, phostent->h_aliases[i]) == 0) {
                    bHostsFund = LW_TRUE;
                    goto    __fund_check;
                }
            }
            phostent = gethostent2_r(fpHosts, cHostsBuffer, sizeof(cHostsBuffer));
        }
        
__fund_check:
        fclose(fpHosts);
        
        if (bHostsFund) {
            if ((phostent->h_length == 4) &&
                (ucAddrType != LWIP_DNS_ADDRTYPE_IPV6)) {               /*  ipv4                        */
                lib_memcpy(&uiAddr, phostent->h_addr, sizeof(uiAddr));
                ip_2_ip4(addr)->addr = uiAddr;
                IP_SET_TYPE(addr, IPADDR_TYPE_V4);
                return  (ERR_OK);
            
            } else if ((phostent->h_length == 16) &&
                       (ucAddrType == LWIP_DNS_ADDRTYPE_IPV6)) {        /*  ipv6                        */
                lib_memcpy(&ip_2_ip6(addr)->addr, phostent->h_addr, phostent->h_length);
                IP_SET_TYPE(addr, IPADDR_TYPE_V6);
                return  (ERR_OK);
            }
        }
    }

    iHash = __hashHorner(pcHost, __LW_HOSTTABLE_HASH_SIZE);
    
    API_SemaphoreBPend(_G_ulHostTableLock, LW_OPTION_WAIT_INFINITE);    /*  进入临界区                  */
    for (plineTemp  = _G_plineHostTableHash[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        phsttablitem = _LIST_ENTRY(plineTemp, __LW_HOSTTABLE_ITEM, HSTTBL_lineManage);
        if (lib_strcmp(phsttablitem->HSTTBL_cHostName, pcHost) == 0) {
            uiAddr = phsttablitem->HSTTBL_inaddr.s_addr;
            break;
        }
    }
    API_SemaphoreBPost(_G_ulHostTableLock);                             /*  退出临界区                  */
    
    if ((uiAddr != INADDR_NONE) &&
        (ucAddrType != LWIP_DNS_ADDRTYPE_IPV6)) {                       /*  ipv4                        */
        ip_2_ip4(addr)->addr = uiAddr;
        IP_SET_TYPE(addr, IPADDR_TYPE_V4);
        return  (ERR_OK);
    
    } else {
        return  (ERR_IF);
    }
}
/*********************************************************************************************************
** 函数名称: API_INetHostTableGetItem
** 功能描述: 查询本地动态主机域名表.
** 输　入  : pcHost        主机名
**           pinaddr       地址
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_INetHostTableGetItem (CPCHAR  pcHost, struct in_addr  *pinaddr)
{
    ip_addr_t   addr;
    
    if (__inetHostTableGetItem(pcHost, &addr, LWIP_DNS_ADDRTYPE_IPV4)) {
        return  (PX_ERROR);
    }
    
    if (pinaddr) {
        pinaddr->s_addr = ip_2_ip4(&addr)->addr;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_INetHostTableAddItem
** 功能描述: 向本地动态主机域名表中插入一个条目.
** 输　入  : pcHost        主机名
**           inaddr        地址
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_INetHostTableAddItem (CPCHAR  pcHost, struct in_addr  inaddr)
{
    REGISTER INT                    iHash;
    REGISTER PLW_LIST_LINE          plineTemp;
             __PLW_HOSTTABLE_ITEM   phsttablitem;
             
             BOOL                   bIsExist = LW_FALSE;

    if (pcHost == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iHash = __hashHorner(pcHost, __LW_HOSTTABLE_HASH_SIZE);

    API_SemaphoreBPend(_G_ulHostTableLock, LW_OPTION_WAIT_INFINITE);    /*  进入临界区                  */
    for (plineTemp  = _G_plineHostTableHash[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        phsttablitem = _LIST_ENTRY(plineTemp, __LW_HOSTTABLE_ITEM, HSTTBL_lineManage);
        if (lib_strcmp(phsttablitem->HSTTBL_cHostName, pcHost) == 0) {
            bIsExist = LW_TRUE;
            break;
        }
    }
    /*
     *  如果不存在则插入.
     */
    if (bIsExist == LW_FALSE) {
        phsttablitem = (__PLW_HOSTTABLE_ITEM)__SHEAP_ALLOC(sizeof(__LW_HOSTTABLE_ITEM) + 
                                                           lib_strlen(pcHost));
        if (phsttablitem == LW_NULL) {
            API_SemaphoreBPost(_G_ulHostTableLock);                     /*  退出临界区                  */
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        phsttablitem->HSTTBL_inaddr = inaddr;
        lib_strcpy(phsttablitem->HSTTBL_cHostName, pcHost);
        
        _List_Line_Add_Ahead(&phsttablitem->HSTTBL_lineManage,
                             &_G_plineHostTableHash[iHash]);            /*  插入 hash 表                */
    }
    API_SemaphoreBPost(_G_ulHostTableLock);                             /*  退出临界区                  */
    
    if (bIsExist) {
        _ErrorHandle(EADDRINUSE);                                       /*  地址已经在使用了            */
        return  (PX_ERROR);
    
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: API_INetHostTableDelItem
** 功能描述: 从本地动态主机域名表中删除一个条目.
** 输　入  : pcHost        主机名
** 输　出  : ERROR CODE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_INetHostTableDelItem (CPCHAR  pcHost)
{
    REGISTER INT                    iHash;
    REGISTER PLW_LIST_LINE          plineTemp;
             __PLW_HOSTTABLE_ITEM   phsttablitem;

    if (pcHost == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iHash = __hashHorner(pcHost, __LW_HOSTTABLE_HASH_SIZE);

    API_SemaphoreBPend(_G_ulHostTableLock, LW_OPTION_WAIT_INFINITE);    /*  进入临界区                  */
    for (plineTemp  = _G_plineHostTableHash[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        phsttablitem = _LIST_ENTRY(plineTemp, __LW_HOSTTABLE_ITEM, HSTTBL_lineManage);
        if (lib_strcmp(phsttablitem->HSTTBL_cHostName, pcHost) == 0) {
            _List_Line_Del(&phsttablitem->HSTTBL_lineManage,
                           &_G_plineHostTableHash[iHash]);              /*  从 hash 表中删除            */
            __SHEAP_FREE(phsttablitem);
            break;
        }
    }
    API_SemaphoreBPost(_G_ulHostTableLock);                             /*  退出临界区                  */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellHostTable
** 功能描述: shell 指令 "hosttable"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0 or -1
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellHostTable (INT  iArgC, PCHAR  *ppcArgV)
{
    static const CHAR               cHostTableInfoHdr[] = "\n"
          "      addr                       HOST\n"
          "--------------- ------------------------------------------\n";
    REGISTER PLW_LIST_LINE          plineTemp;
             __PLW_HOSTTABLE_ITEM   phsttablitem;
             
             CHAR                   cBuffer[INET_ADDRSTRLEN];
             ip4_addr_t             ipaddr;
    struct in_addr                  inaddr;
    
             INT                    i;

    if (iArgC == 1) {                                                   /*  仅打印当前地址表信息        */
        printf(cHostTableInfoHdr);
        
        API_SemaphoreBPend(_G_ulHostTableLock, LW_OPTION_WAIT_INFINITE);/*  进入临界区                  */
        /*
         *  遍历显示主机表.
         */
        for (i = 0; i < __LW_HOSTTABLE_HASH_SIZE; i++) {
            for (plineTemp  = _G_plineHostTableHash[i];
                 plineTemp != LW_NULL;
                 plineTemp  = _list_line_get_next(plineTemp)) {
                 
                phsttablitem = _LIST_ENTRY(plineTemp, __LW_HOSTTABLE_ITEM, HSTTBL_lineManage);
                
                ipaddr.addr = phsttablitem->HSTTBL_inaddr.s_addr;
                
                /*
                 *  这里没有使用 inet_ntoa() 因为 inet_ntoa() 不可重入.
                 */
                snprintf(cBuffer, sizeof(cBuffer), 
                         "%d.%d.%d.%d", 
                         ip4_addr1(&ipaddr),
                         ip4_addr2(&ipaddr),
                         ip4_addr3(&ipaddr),
                         ip4_addr4(&ipaddr));
                printf("%-15s %s\n", cBuffer, phsttablitem->HSTTBL_cHostName);
            }
        }
        API_SemaphoreBPost(_G_ulHostTableLock);                         /*  退出临界区                  */
        
        return  (ERROR_NONE);
    
    } else if ((iArgC == 4) && 
        (lib_strncmp(ppcArgV[1], "-s", 3) == 0)) {                      /*  添加主机地址映射            */
        
        if (!inet_aton(ppcArgV[3], &inaddr)) {
            fprintf(stderr, "inaddr error.\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        return  (API_INetHostTableAddItem(ppcArgV[2], inaddr));         /*  添加条目                    */
        
    } else if ((iArgC == 3) && 
               (lib_strncmp(ppcArgV[1], "-d", 3) == 0)) {               /*  删除主机信息                */
        
        return  (API_INetHostTableDelItem(ppcArgV[2]));                 /*  删除条目                    */
        
    } else {
        fprintf(stderr, "arguments error!\n");                           /*  参数错误                    */
        return  (-ERROR_TSHELL_EPARAM);
    }
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/

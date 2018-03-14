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
** 文   件   名: lwip_vlan.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 07 月 13 日
**
** 描        述: lwip vlan 支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_VLAN_EN > 0
#include "socket.h"
#include "net/if_vlan.h"
/*********************************************************************************************************
** 函数名称: API_VlanSet
** 功能描述: 设置一个网络接口的 VLAN ID
** 输　入  : pcEthIf           以太网络接口名.
             iVlanId           VLAN ID (-1 表示允许任何 VLAN ID 通过)
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_NET_VLAN_EN > 0

LW_API  
INT  API_VlanSet (CPCHAR  pcEthIf, INT  iVlanId)
{
    struct vlanreq  vlanreq;
    INT             iSock;
    
    iSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSock < 0) {
        return  (PX_ERROR);
    }
    
    lib_strlcpy(vlanreq.vlr_ifname, pcEthIf, IFNAMSIZ);
    vlanreq.vlr_tag = (u_short)(iVlanId & 0x1fff);
    vlanreq.vlr_pri = (u_short)((iVlanId >> 13) & 0x3);
    
    if (ioctl(iSock, SIOCSETVLAN, &vlanreq)) {
        close(iSock);
        return  (PX_ERROR);
    }
    
    close(iSock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_VlanGet
** 功能描述: 获取一个网络接口的 VLAN ID
** 输　入  : pcEthIf           以太网络接口名.
             piVlanId          VLAN ID (-1 表示允许任何 VLAN ID 通过)
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_VlanGet (CPCHAR  pcEthIf, INT  *piVlanId)
{
    struct vlanreq  vlanreq;
    INT             iSock;
    
    iSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSock < 0) {
        return  (PX_ERROR);
    }
    
    lib_strlcpy(vlanreq.vlr_ifname, pcEthIf, IFNAMSIZ);
    
    if (ioctl(iSock, SIOCGETVLAN, &vlanreq)) {
        close(iSock);
        return  (PX_ERROR);
    }
    
    close(iSock);
    
    if (piVlanId) {
        *piVlanId = vlanreq.vlr_tag | (vlanreq.vlr_pri << 13);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_VlanShow
** 功能描述: 显示所有网络接口的 VLAN ID
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_VlanShow (VOID)
{
    static const CHAR   cVlanInfoHdr[] = "\n"
    "IFACE  VLAN ID   TAG  PRI\n"
    "----- --------- ----- ---\n";
    
    struct vlanreq_list vlrlist;
    struct vlanreq     *pvlanreq;
    INT                 iSock, i;
    
    vlrlist.vlrl_bcnt = 0;
    vlrlist.vlrl_num  = 0;
    vlrlist.vlrl_buf  = LW_NULL;
    
    printf(cVlanInfoHdr);
    
    iSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSock < 0) {
        return;
    }
    
    if (ioctl(iSock, SIOCLSTVLAN, &vlrlist)) {
        close(iSock);
        return;
    }
    
    if (!vlrlist.vlrl_total) {
        close(iSock);
        return;
    }
    
    vlrlist.vlrl_buf  = (struct vlanreq *)__SHEAP_ALLOC(sizeof(struct vlanreq) * vlrlist.vlrl_total);
    vlrlist.vlrl_bcnt = vlrlist.vlrl_total;
    
    if (ioctl(iSock, SIOCLSTVLAN, &vlrlist)) {
        __SHEAP_FREE(vlrlist.vlrl_buf);
        close(iSock);
        return;
    }
    
    close(iSock);
    
    for (i = 0; i < vlrlist.vlrl_total; i++) {
        pvlanreq = &vlrlist.vlrl_buf[i];
        printf("%-5s %9d %5d %3d\n",
               pvlanreq->vlr_ifname,
               pvlanreq->vlr_tag | (pvlanreq->vlr_pri << 13),
               pvlanreq->vlr_tag,
               pvlanreq->vlr_pri);
    }
    
    __SHEAP_FREE(vlrlist.vlrl_buf);
}
/*********************************************************************************************************
** 函数名称: __tshellVlan
** 功能描述: 系统命令 "vlan"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellVlan (INT  iArgC, PCHAR  *ppcArgV)
{
    INT    iError;
    INT    iVlanId;
    
    if (iArgC == 1) {
        API_VlanShow();
        return  (ERROR_NONE);
    }
    
    if (lib_strcmp(ppcArgV[1], "set") == 0) {
        if (iArgC != 4) {
            fprintf(stderr, "arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        if ((sscanf(ppcArgV[3], "%d", &iVlanId) != 1) ||
            (iVlanId < 0) || (iVlanId > 65535)) {
            fprintf(stderr, "VLAN ID error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        iError = API_VlanSet(ppcArgV[2], iVlanId);
        if (iError < 0) {
            fprintf(stderr, "VLAN ID Set error: %s\n", lib_strerror(errno));
            return  (PX_ERROR);
        }
    
    } else if (lib_strcmp(ppcArgV[1], "clear") == 0) {
        if (iArgC != 3) {
            fprintf(stderr, "arguments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        iVlanId = -1;
        iError = API_VlanSet(ppcArgV[2], iVlanId);
        if (iError < 0) {
            fprintf(stderr, "VLAN ID Clear error: %s\n", lib_strerror(errno));
            return  (PX_ERROR);
        }
    
    } else {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __netVlanInit
** 功能描述: 初始化 vlan 管理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __netVlanInit (VOID)
{
#if LW_CFG_SHELL_EN > 0
    API_TShellKeywordAdd("vlan", __tshellVlan);
    API_TShellFormatAdd("vlan", " [{set | clear}] [ifname]");
    API_TShellHelpAdd("vlan",   "show | set | clear net interface VLAN ID.\n"
                                "eg. vlan               show all vlan id.\n"
                                "    vlan set en0 3     set netif:en0 VLAN ID 3.\n"
                                "    vlan clear en0     clear netif:en0 VLAN ID.\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
}

#endif                                                                  /*  LW_CFG_NET_VLAN_EN > 0      */
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/

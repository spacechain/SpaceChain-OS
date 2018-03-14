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
** 文   件   名: lwip_natlib.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 03 月 19 日
**
** 描        述: lwip NAT 支持包内部库.
*********************************************************************************************************/

#ifndef __LWIP_NATLIB_H
#define __LWIP_NATLIB_H

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_ROUTER > 0) && (LW_CFG_NET_NAT_EN > 0)
/*********************************************************************************************************
  NAT 控制块
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE        NAT_lineManage;                                 /*  NAT 控制块链表              */
    
    u8_t                NAT_ucProto;                                    /*  协议                        */
    ip4_addr_t          NAT_ipaddrLocalIp;                              /*  本地 IP 地址                */
    u16_t               NAT_usLocalPort;                                /*  本地端口号                  */
    u16_t               NAT_usAssPort;                                  /*  映射端口号 (唯一的)         */
    
    ULONG               NAT_ulIdleTimer;                                /*  空闲定时器                  */
    ULONG               NAT_ulTermTimer;                                /*  断链定时器                  */
    INT                 NAT_iStatus;                                    /*  通信状态                    */
#define __NAT_STATUS_OPEN           0
#define __NAT_STATUS_FIN            1
#define __NAT_STATUS_CLOSING        2
} __NAT_CB;
typedef __NAT_CB       *__PNAT_CB;
/*********************************************************************************************************
  NAT 外网主动连接映射关系
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE        NAT_lineManage;
    
    u8_t                NAT_ucProto;                                    /*  协议                        */
    ip4_addr_t          NAT_ipaddrLocalIp;                              /*  本地 IP 地址                */
    u16_t               NAT_usLocalCnt;                                 /*  本地 IP 段个数 (负载均衡)   */
    u16_t               NAT_usLocalPort;                                /*  本地端口号                  */
    u16_t               NAT_usAssPort;                                  /*  映射端口号 (唯一的)         */
} __NAT_MAP;
typedef __NAT_MAP      *__PNAT_MAP;
/*********************************************************************************************************
  NAT 别名表
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE        NAT_lineManage;
    
    ip4_addr_t          NAT_ipaddrAliasIp;                              /*  别名地址                    */
    ip4_addr_t          NAT_ipaddrSLocalIp;                             /*  别名对应本地 IP 范围        */
    ip4_addr_t          NAT_ipaddrELocalIp;
} __NAT_ALIAS;
typedef __NAT_ALIAS    *__PNAT_ALIAS;

/*********************************************************************************************************
  网卡事件回调
*********************************************************************************************************/
VOID  nat_netif_add_hook(struct netif *pnetif);
VOID  nat_netif_remove_hook(struct netif *pnetif);

/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
VOID        __natInit(VOID);
INT         __natStart(CPCHAR  pcLocal, CPCHAR  pcAp);
INT         __natStop(VOID);
INT         __natAddLocal(CPCHAR  pcLocal);
INT         __natAddAp(CPCHAR  pcAp);
INT         __natMapAdd(ip4_addr_t  *pipaddr, u16_t  usIpCnt, u16_t  usPort, u16_t  AssPort, u8_t  ucProto);
INT         __natMapDelete(ip4_addr_t  *pipaddr, u16_t  usPort, u16_t  AssPort, u8_t  ucProto);
INT         __natAliasAdd(const ip4_addr_t  *pipaddrAlias, 
                          const ip4_addr_t  *ipaddrSLocalIp,
                          const ip4_addr_t  *ipaddrELocalIp);
INT         __natAliasDelete(const ip4_addr_t  *pipaddrAlias);

#if LW_CFG_PROCFS_EN > 0
VOID        __procFsNatInit(VOID);
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_ROUTER > 0       */
                                                                        /*  LW_CFG_NET_NAT_EN > 0       */
#endif                                                                  /*  __LWIP_NATLIB_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/

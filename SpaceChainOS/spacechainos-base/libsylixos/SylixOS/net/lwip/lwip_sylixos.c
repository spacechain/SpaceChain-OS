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
** 文   件   名: lwip_sylixos.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 05 月 06 日
**
** 描        述: lwip sylixos 接口.

** BUG:
2009.05.20  _netJobqueueInit 初始化应放在网络初始化的后面, 更加安全.
2009.07.11  API_NetInit() 仅允许初始化一次.
2009.08.19  去掉 snmp init 这个操作将在 lwip 1.3.1 以后版本中自动初始化.
2010.02.22  升级 lwip 后去掉 loopif 的初始化(协议栈自行安装回环网口).
2010.07.28  加入 snmp 时间戳回调和独立的初始化函数.
2010.11.01  __netCloseAll() 调用 DHCP 函数使用安全模式.
2012.12.18  加入 unix_init() 初始化 AF_UNIX 域协议.
2013.06.21  加入网络 proc 文件.
2016.10.21  简化系统重启操作.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "net/if_lock.h"
#include "net/if_flags.h"
#include "lwip/tcpip.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "lwip/dhcp.h"
#include "lwip/sockets.h"
#include "lwip/snmp/snmp.h"
#include "lwip/snmp/snmp_mib2.h"
/*********************************************************************************************************
  TCP Ext
*********************************************************************************************************/
#if LW_CFG_LWIP_TCP_SIG_EN > 0
#include "tcpsig/tcp_md5.h"
#endif                                                                  /*  LW_CFG_LWIP_TCP_SIG_EN > 0  */
/*********************************************************************************************************
  内部头文件
*********************************************************************************************************/
#if LWIP_SNMP > 0 && LWIP_SNMP_V3 > 0
#include "./tools/snmp/snmpv3_sylixos.h"
#endif                                                                  /*  LWIP_SNMP && LWIP_SNMP_V3   */
#include "./unix/af_unix.h"
#include "./route/af_route.h"
#include "./packet/af_packet.h"
#if LW_CFG_PROCFS_EN > 0
#include "./proc/lwip_proc.h"
#endif                                                                  /*  LW_CFG_PROCFS_EN            */
/*********************************************************************************************************
  网络动态内存管理
*********************************************************************************************************/
#if LW_CFG_LWIP_MEM_TLSF > 0
extern void tlsf_mem_create(void);
#endif                                                                  /*  LW_CFG_LWIP_MEM_TLSF        */
/*********************************************************************************************************
  网络驱动工作队列函数声明
*********************************************************************************************************/
extern INT  _netJobqueueInit(VOID);
/*********************************************************************************************************
  网络函数声明
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
extern VOID __tshellNetInit(VOID);
extern VOID __tshellNet6Init(VOID);
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  互联网函数声明
*********************************************************************************************************/
extern VOID __inetHostTableInit(VOID);
/*********************************************************************************************************
  网络事件初始化
*********************************************************************************************************/
extern INT  _netEventInit(VOID);
/*********************************************************************************************************
  虚拟网络设备支持
*********************************************************************************************************/
#if LW_CFG_NET_VNETDEV_EN > 0
extern INT  _netVndInit(VOID);
#endif                                                                  /*  LW_CFG_NET_VNETDEV_EN > 0   */
/*********************************************************************************************************
  VLAN 支持
*********************************************************************************************************/
#if LW_CFG_NET_VLAN_EN > 0
extern VOID __netVlanInit(VOID);
#endif                                                                  /*  LW_CFG_NET_VLAN_EN > 0      */
/*********************************************************************************************************
  网桥支持
*********************************************************************************************************/
#if LW_CFG_NET_DEV_BRIDGE_EN > 0
extern INT  _netBridgeInit(VOID);
#endif                                                                  /*  LW_CFG_NET_DEV_BRIDGE_EN    */
/*********************************************************************************************************
  拨号网络函数声明
*********************************************************************************************************/
#if LW_CFG_LWIP_PPP > 0 || LW_CFG_LWIP_PPPOE > 0
#if __LWIP_USE_PPP_NEW == 0
extern err_t pppInit(void);
#endif
#endif                                                                  /*  LW_CFG_LWIP_PPP             */
                                                                        /*  LW_CFG_LWIP_PPPOE           */
/*********************************************************************************************************
** 函数名称: __netCloseAll
** 功能描述: 系统重启或关闭时回调函数.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __netCloseAll (VOID)
{
#if LWIP_DHCP > 0
    struct netif  *netif;

    NETIF_FOREACH(netif) {
        if (netif_dhcp_data(netif)) {
            netifapi_dhcp_release_and_stop(netif);                      /*  解除 DHCP 租约, 同时停止网卡*/
        }
    }
#endif                                                                  /*  LWIP_DHCP > 0               */
}
/*********************************************************************************************************
** 函数名称: snmp_mib2_threadsync
** 功能描述: SNMP mib2 线程同步回调函数.
** 输　入  : pfunc     回调函数
**           pvArg     回调参数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LWIP_SNMP > 0

static VOID  snmp_mib2_threadsync (snmp_threadsync_called_fn  pfunc, PVOID  pvArg)
{
    if (pfunc) {
        pfunc(pvArg);
    }
}
/*********************************************************************************************************
** 函数名称: __netSnmpInit
** 功能描述: 初始化 SNMP 默认信息.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __netSnmpInit (VOID)
{
    static const u16_t  ucContactLen  = 19;
    static const u16_t  ucLocationLen = 6;
    
    static const u16_t  ucDesrLen = 7;
    static const u16_t  ucNameLen = 23;
    
    snmp_mib2_set_syscontact_readonly((u8_t *)"acoinfo@acoinfo.com", &ucContactLen);
    snmp_mib2_set_syslocation_readonly((u8_t *)"@china", &ucLocationLen);
                                                                        /*  at CHINA ^_^                */
    snmp_mib2_set_sysdescr((u8_t *)"sylixos", &ucDesrLen);
    snmp_mib2_set_sysname_readonly((u8_t *)"device based on sylixos", &ucNameLen);

    snmp_threadsync_init(&snmp_mib2_lwip_locks, snmp_mib2_threadsync);
}

#endif                                                                  /*  LWIP_SNMP > 0               */
/*********************************************************************************************************
** 函数名称: __netCfgFileInit
** 功能描述: 初始化必要的 etc 文件.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __netCfgFileInit (VOID)
{
    FILE   *fp;
    
    if (access("/etc/hosts", R_OK) < 0) {
        if ((fp = fopen("/etc/hosts", "w")) != NULL) {
            fprintf(fp, "127.0.0.1    localhost\n");
            fclose(fp);
        }
    }
}
/*********************************************************************************************************
** 函数名称: API_NetInit
** 功能描述: 向操作系统内核注册网络组件
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_NetInit (VOID)
{
    static BOOL   bIsInit = LW_FALSE;
    
    if (bIsInit) {
        return;
    }
    
    bIsInit = LW_TRUE;
    
    if_list_init();                                                     /*  网络接口链锁初始化          */
    
#if LW_CFG_LWIP_MEM_TLSF > 0
    tlsf_mem_create();
#endif                                                                  /*  LW_CFG_LWIP_MEM_TLSF        */
    
#if LW_CFG_NET_VLAN_EN > 0
    __netVlanInit();                                                    /*  初始化 vlan                 */
#endif                                                                  /*  LW_CFG_NET_VLAN_EN > 0      */
    
    _netJobqueueInit();                                                 /*  创建网络驱动作业处理队列    */
    
    _netEventInit();                                                    /*  初始化网络事件              */
    
    API_SystemHookAdd((LW_HOOK_FUNC)__netCloseAll, 
                      LW_OPTION_KERNEL_REBOOT);                         /*  安装系统关闭时, 回调函数    */

    netif_callback_init();                                              /*  设置网络接口回调            */

    tcpip_init(LW_NULL, LW_NULL);                                       /*  以多任务形式初始化 lwip     */
    
#if LW_CFG_LWIP_TCP_SIG_EN > 0
    tcp_md5_init();
#endif                                                                  /*  LW_CFG_LWIP_TCP_SIG_EN > 0  */

#if LW_CFG_NET_UNIX_EN > 0
    unix_init();                                                        /*  初始化 AF_UNIX 域协议       */
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

#if LW_CFG_NET_ROUTER > 0
    route_init();
#endif                                                                  /*  LW_CFG_NET_ROUTER > 0       */

    packet_init();                                                      /*  初始化 AF_PACKET 协议       */
    
    __socketInit();                                                     /*  初始化 socket 系统          */

#if LW_CFG_NET_VNETDEV_EN > 0
    _netVndInit();                                                      /*  初始化虚拟网络设备          */
#endif                                                                  /*  LW_CFG_NET_VNETDEV_EN > 0   */

#if LW_CFG_LWIP_PPP > 0 || LW_CFG_LWIP_PPPOE > 0
#if __LWIP_USE_PPP_NEW == 0
    pppInit();                                                          /*  初始化 point to point proto */
#endif                                                                  /*  !__LWIP_USE_PPP_NEW         */
#endif                                                                  /*  LW_CFG_LWIP_PPP             */
                                                                        /*  LW_CFG_LWIP_PPPOE           */
#if LWIP_SNMP > 0
    __netSnmpInit();                                                    /*  初始化 SNMP 基本信息        */
#endif                                                                  /*  LWIP_SNMP > 0               */
    
    __netCfgFileInit();
    
#if LW_CFG_SHELL_EN > 0
    __tshellNetInit();                                                  /*  注册网络命令                */
    __tshellNet6Init();                                                 /*  注册 IPv6 专属命令          */
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
    
#if LW_CFG_NET_DEV_BRIDGE_EN > 0
    _netBridgeInit();
#endif                                                                  /*  LW_CFG_NET_DEV_BRIDGE_EN    */

    /*
     *  密切相关工具初始化.
     */
    __inetHostTableInit();                                              /*  初始化本地地址转换表        */
    
#if LW_CFG_PROCFS_EN > 0
    __procFsNetInit();
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
}
/*********************************************************************************************************
** 函数名称: API_NetSnmpInit
** 功能描述: 开启 SNMP 服务
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_NetSnmpInit (VOID)
{
#if LWIP_SNMP > 0
    snmp_init();

#if LWIP_SNMP_V3 > 0
    snmpv3_sylixos_init();
#endif                                                                  /*  LWIP_SNMP_V3 > 0            */

    snmp_v1_enable(LW_CFG_NET_SNMP_V1_EN);
    snmp_v2c_enable(LW_CFG_NET_SNMP_V2_EN);
    snmp_v3_enable(LW_CFG_NET_SNMP_V3_EN);
#endif                                                                  /*  LWIP_SNMP > 0               */
}

#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/

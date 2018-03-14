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
** 文   件   名: lwip_flowsh.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 12 月 21 日
**
** 描        述: 流量控制命令.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_SHELL_EN > 0 && LW_CFG_NET_FLOWCTL_EN > 0
#include "net/flowctl.h"
/*********************************************************************************************************
** 函数名称: __tshellFlowctl
** 功能描述: 系统命令 "flowctl"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFlowctl (INT  iArgC, PCHAR  *ppcArgV)
{
    fprintf(stderr, "Flow control NOT include in open source version!\n");
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __tshellFlowctlInit
** 功能描述: 注册流量控制命令
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID __tshellFlowctlInit (VOID)
{
    API_TShellKeywordAdd("flowctl", __tshellFlowctl);
    API_TShellFormatAdd("flowctl", " [add | del | chg] [ip | if] [...]");
    API_TShellHelpAdd("flowctl",   "show, add, delete, change flow control status.\n"
    "eg. flowctl\n"
    "    flowctl add ip 192.168.1.1 192.168.1.10 tcp 20 80 dev en1 50 100 64\n"
    "       add a flow control rule: iface: en1(LAN Port) ip frome 192.168.1.1 to 192.168.1.10 tcp protocol port(20 ~ 80)\n"
    "       uplink 50KBytes downlink 100KBytes buffer is 64KBytes\n\n"
    
    "    flowctl add ip 192.168.1.1 192.168.1.10 udp 20 80 dev en1 50 100\n"
    "       add a flow control rule: iface: en1(LAN Port) ip frome 192.168.1.1 to 192.168.1.10 udp protocol port(20 ~ 80)\n"
    "       uplink 50KBytes downlink 100KBytes buffer is default size\n\n"
    
    "    flowctl add ip 192.168.1.1 192.168.1.10 all dev en1 50 100\n"
    "       add a flow control rule: iface: en1(LAN Port) ip frome 192.168.1.1 to 192.168.1.10 all protocol\n"
    "       uplink 50KBytes downlink 100KBytes buffer is default size\n\n"
    
    "    flowctl chg ip 192.168.1.1 192.168.1.10 tcp 20 80 dev en1 50 100\n"
    "       change flow control rule: iface: en1(LAN Port) ip frome 192.168.1.1 to 192.168.1.10 tcp protocol port(20 ~ 80)\n"
    "       uplink 50KBytes downlink 100KBytes buffer is default size\n\n"
    
    "    flowctl del ip 192.168.1.1 192.168.1.10 tcp 20 80 dev en1\n"
    "       delete a flow control rule: iface: en1(LAN Port) ip frome 192.168.1.1 to 192.168.1.10 tcp protocol port(20 ~ 80)\n\n"
    
    "    flowctl add if dev en1 50 100 64\n"
    "       add flow control rule: iface: en1(LAN Port)\n"
    "       uplink 50KBytes downlink 100KBytes buffer 64K\n\n"
    
    "    flowctl chg if dev en1 50 100\n"
    "       change flow control rule: iface: en1(LAN Port)\n"
    "       uplink 50KBytes downlink 100KBytes buffer is default size\n\n"
    
    "    flowctl del if dev en1\n"
    "       delete a flow control rule: iface: en1(LAN Port)\n");
}

#endif                                                                  /*  LW_CFG_NET_EN               */
                                                                        /*  LW_CFG_SHELL_EN > 0         */
                                                                        /*  LW_CFG_NET_FLOWCTL_EN > 0   */
/*********************************************************************************************************
  END
*********************************************************************************************************/

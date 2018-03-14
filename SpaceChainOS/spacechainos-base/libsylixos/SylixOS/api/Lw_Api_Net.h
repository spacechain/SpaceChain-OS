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
** 文   件   名: Lw_Api_Net.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 05 月 10 日
**
** 描        述: 这是系统提供给 C / C++ 用户的内核应用程序接口层。
                 为了适应不同语言习惯的人，这里使用了很多重复功能.
*********************************************************************************************************/

#ifndef __LW_API_NET_H
#define __LW_API_NET_H

/*********************************************************************************************************
  snmp
*********************************************************************************************************/

#define Lw_Net_SnmpPriMibGet            API_NetSnmpPriMibGet
#define Lw_Net_SnmpSetPriMibInitHook    API_NetSnmpSetPriMibInitHook

/*********************************************************************************************************
  tools
*********************************************************************************************************/
/*********************************************************************************************************
  ping
*********************************************************************************************************/

#define Lw_INet_PingInit                API_INetPingInit

/*********************************************************************************************************
  telnet
*********************************************************************************************************/

#define Lw_INet_TelnetInit              API_INetTelnetInit

/*********************************************************************************************************
  netbios
*********************************************************************************************************/

#define Lw_INet_NetBiosInit             API_INetNetBiosInit
#define Lw_INet_NetBiosSetName          API_INetNetBiosNameSet
#define Lw_INet_NetBiosNameGet          API_INetNetBiosNameGet

/*********************************************************************************************************
  tftp
*********************************************************************************************************/

#define Lw_INet_TftpReceive             API_INetTftpReceive
#define Lw_INet_TftpSend                API_INetTftpSend
#define Lw_INet_TftpServerInit          API_INetTftpServerInit
#define Lw_INet_TftpServerPath          API_INetTftpServerPath

/*********************************************************************************************************
  ftp
*********************************************************************************************************/

#define Lw_Inet_FtpServerInit           API_INetFtpServerInit
#define Lw_Inet_FtpServerShow           API_INetFtpServerShow
#define Lw_Inet_FtpServerPath           API_INetFtpServerPath

/*********************************************************************************************************
  vpn
*********************************************************************************************************/

#define Lw_Inet_VpnInit                 API_INetVpnInit
#define Lw_Inet_VpnClientCreate         API_INetVpnClientCreate
#define Lw_Inet_VpnClientDelete         API_INetVpnClientDelete

/*********************************************************************************************************
  nat
*********************************************************************************************************/

#define Lw_Inet_NatInit                 API_INetNatInit
#define Lw_Inet_NatStart                API_INetNatStart
#define Lw_Inet_NatStop                 API_INetNatStop

/*********************************************************************************************************
  npf
*********************************************************************************************************/

#define Lw_Inet_NpfInit                 API_INetNpfInit
#define Lw_Inet_NpfRuleAdd              API_INetNpfRuleAdd
#define Lw_Inet_NpfRuleDel              API_INetNpfRuleDel
#define Lw_Inet_NpfAttach               API_INetNpfAttach
#define Lw_Inet_NpfDetach               API_INetNpfDetach
#define Lw_Inet_NpfDropGet              API_INetNpfDropGet
#define Lw_Inet_NpfAllowGet             API_INetNpfAllowGet
#define Lw_Inet_NpfShow                 API_INetNpfShow

#endif                                                                  /*  __LW_API_NET_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/

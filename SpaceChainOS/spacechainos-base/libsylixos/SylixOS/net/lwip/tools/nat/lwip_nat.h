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
** 文   件   名: lwip_nat.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 03 月 19 日
**
** 描        述: lwip NAT 支持包.
*********************************************************************************************************/

#ifndef __LWIP_NAT_H
#define __LWIP_NAT_H

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_ROUTER > 0) && (LW_CFG_NET_NAT_EN > 0)

LW_API VOID         API_INetNatInit(VOID);
LW_API INT          API_INetNatStart(CPCHAR  pcLocalNetif, CPCHAR  pcApNetif);
LW_API INT          API_INetNatStop(VOID);
LW_API INT          API_INetNatLocalAdd(CPCHAR  pcLocalNetif);
LW_API INT          API_INetNatWanAdd(CPCHAR  pcApNetif);
LW_API INT          API_INetNatMapAdd(CPCHAR  pcLocalIp, UINT16  usIpCnt, UINT16  usLocalPort, 
                                      UINT16  usAssPort, UINT8   ucProto);
LW_API INT          API_INetNatMapDelete(CPCHAR  pcLocalIp, UINT16  usLocalPort, 
                                         UINT16  usAssPort, UINT8   ucProto);
LW_API INT          API_INetNatAliasAdd(CPCHAR  pcAliasIp, CPCHAR  pcSLocalIp, CPCHAR  pcELocalIp);
LW_API INT          API_INetNatAliasDelete(CPCHAR  pcAliasIp);

#define inetNatInit         API_INetNatInit
#define inetNatStart        API_INetNatStart
#define inetNatStop         API_INetNatStop
#define inetNatLocalAdd     API_INetNatLocalAdd
#define inetNatWanAdd       API_INetNatWanAdd
#define inetNatMapAdd       API_INetNatMapAdd
#define inetNatMapDelete    API_INetNatMapDelete
#define inetNatAliasAdd     API_INetNatAliasAdd
#define inetNatAliasDelete  API_INetNatAliasDelete

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_ROUTER > 0       */
                                                                        /*  LW_CFG_NET_NAT_EN > 0       */
#endif                                                                  /*  __LWIP_NAT_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/

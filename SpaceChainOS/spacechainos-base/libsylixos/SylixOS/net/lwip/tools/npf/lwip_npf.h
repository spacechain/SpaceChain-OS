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
** 文   件   名: lwip_npf.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 02 月 11 日
**
** 描        述: lwip net packet filter 工具.
*********************************************************************************************************/

#ifndef __LWIP_NPF_H
#define __LWIP_NPF_H

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_NPF_EN > 0)

#define LWIP_NPF_RULE_MAC   0
#define LWIP_NPF_RULE_IP    1
#define LWIP_NPF_RULE_UDP   2
#define LWIP_NPF_RULE_TCP   3

LW_API INT      API_INetNpfInit(VOID);
LW_API PVOID    API_INetNpfRuleAdd(CPCHAR  pcNetifName,
                                   INT     iRule,
                                   UINT8   pucMac[],
                                   CPCHAR  pcAddrStart,
                                   CPCHAR  pcAddrEnd,
                                   UINT16  usPortStart,
                                   UINT16  usPortEnd);
LW_API INT      API_INetNpfRuleDel(CPCHAR  pcNetifName,
                                   PVOID   pvRule,
                                   INT     iSeqNum);
LW_API INT      API_INetNpfAttach(CPCHAR  pcNetifName);
LW_API INT      API_INetNpfDetach(CPCHAR  pcNetifName);
LW_API ULONG    API_INetNpfDropGet(VOID);
LW_API ULONG    API_INetNpfAllowGet(VOID);
LW_API INT      API_INetNpfShow(INT  iFd);

#define inetNpfInit         API_INetNpfInit
#define inetNpfRuleAdd      API_INetNpfRuleAdd
#define inetNpfRuleDel      API_INetNpfRuleDel
#define inetNpfAttach       API_INetNpfAttach
#define inetNpfDetach       API_INetNpfDetach
#define inetNpfDropGet      API_INetNpfDropGet
#define inetNpfAllowGet     API_INetNpfAllowGet
#define inetNpfShow         API_INetNpfShow

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_NPF_EN > 0       */
#endif                                                                  /*  __LWIP_NPF_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/

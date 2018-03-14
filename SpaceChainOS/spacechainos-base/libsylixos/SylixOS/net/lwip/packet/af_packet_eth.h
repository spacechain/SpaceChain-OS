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
** 文   件   名: af_packet_eth.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 03 月 21 日
**
** 描        述: AF_PACKET 以太网类型支持
*********************************************************************************************************/

#ifndef __AF_PACKET_ETH_H
#define __AF_PACKET_ETH_H

/*********************************************************************************************************
  内部函数
*********************************************************************************************************/

errno_t  __packetEthRawSendto(CPVOID                pvPacket, 
                              size_t                stBytes, 
                              struct sockaddr_ll   *psockaddrll);
errno_t  __packetEthDgramSendto(CPVOID                pvPacket, 
                                size_t                stBytes, 
                                struct sockaddr_ll   *psockaddrll);
size_t   __packetEthHeaderInfo(AF_PACKET_N  *pktm, struct sockaddr_ll *paddrll);
size_t   __packetEthHeaderInfo2(struct pbuf *p, struct netif *inp, BOOL bOutgo, 
                                struct sockaddr_ll *paddrll);
errno_t  __packetEthIfInfo(INT  iIndex, struct sockaddr_ll *paddrll);

#endif                                                                  /*  __AF_PACKET_ETH_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/

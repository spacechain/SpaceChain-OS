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
** 文   件   名: af_packet.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 03 月 21 日
**
** 描        述: AF_PACKET 支持
*********************************************************************************************************/

#ifndef __AF_PACKET_H
#define __AF_PACKET_H

#include "netpacket/packet.h"

/*********************************************************************************************************
  AF_PACKET 接收数据队列
*********************************************************************************************************/

typedef struct af_packet_node {                                         /*  一个消息节点                */
    LW_LIST_MONO            PKTM_monoManage;                            /*  待接收数据链                */
    
    struct pbuf            *PKTM_p;
    u8_t                    PKTM_ucForme;                               /*  发送给本机的数据包          */
    u8_t                    PKTM_ucIndex;                               /*  接收网络接口                */
    u8_t                    PKTM_ucOutgo;                               /*  是否为发送截获              */
} AF_PACKET_N;

typedef struct af_packet_queue {
    PLW_LIST_MONO           PKTQ_pmonoHeader;                           /*  消息队列头                  */
    PLW_LIST_MONO           PKTQ_pmonoTail;                             /*  消息队列结尾                */
    size_t                  PKTQ_stTotal;                               /*  总有效数据字节数            */
} AF_PACKET_Q;

/*********************************************************************************************************
  AF_PACKET 接收数据队列
*********************************************************************************************************/
#if LW_CFG_NET_PACKET_MMAP > 0

typedef struct af_packet_mmap {
    struct tpacket_req      PKTB_reqbuf;
    UINT                    PKTB_uiFramePerBlock;
    UINT                    PKTB_uiFramePtr;
    UINT                    PKTB_uiFrameMax;
    PVOID                   PKTB_pvPhyMem;                              /*  共享内存物理地址            */
    PVOID                   PKTB_pvVirMem;                              /*  共享内存虚拟地址            */
    size_t                  PKTB_stSize;                                /*  共享内存大小                */
} AF_PACKET_MMAP;

#endif                                                                  /*  LW_CFG_NET_PACKET_MMAP > 0  */
/*********************************************************************************************************
  AF_PACKET 控制块
*********************************************************************************************************/

typedef struct af_packet_t {
    LW_LIST_LINE            PACKET_lineManage;
    
    INT                     PACKET_iFlag;                               /*  NONBLOCK or NOT             */
    INT                     PACKET_iType;                               /*  RAW / DGRAM                 */
    INT                     PACKET_iProtocol;                           /*  协议                        */
    
#define __AF_PACKET_SHUTD_R     0x01
#define __AF_PACKET_SHUTD_W     0x02
    INT                     PACKET_iShutDFlag;                          /*  当前 shutdown 状态          */
    
    BOOL                    PACKET_bRecvOut;                            /*  是否接受输出数据包          */
    INT                     PACKET_iIfIndex;                            /*  绑定的接收                  */
    AF_PACKET_Q             PACKET_pktq;                                /*  接收缓冲                    */
    size_t                  PACKET_stMaxBufSize;                        /*  最大接收缓冲大小            */
    
    enum tpacket_versions   PACKET_tpver;                               /*  RING 头部版本               */
    struct tpacket_stats    PACKET_stats;                               /*  统计信息                    */
    
    UINT                    PACKET_uiHdrLen;                            /*  tp_hdrlen                   */
    UINT                    PACKET_uiReserve;                           /*  tp_reserve                  */
    
#if LW_CFG_NET_PACKET_MMAP > 0
    BOOL                    PACKET_bMapBusy;                            /*  是否在繁忙                  */
    BOOL                    PACKET_bMmap;                               /*  是否已经进行了 MMAP         */
    AF_PACKET_MMAP          PACKET_mmapRx;                              /*  mmap 接收缓冲               */
#endif                                                                  /*  LW_CFG_NET_PACKET_MMAP > 0  */

    LW_OBJECT_HANDLE        PACKET_hCanRead;                            /*  可读                        */
    ULONG                   PACKET_ulRecvTimeout;                       /*  读取超时 tick               */
} AF_PACKET_T;

/*********************************************************************************************************
  内部函数
*********************************************************************************************************/

INT             packet_link_input(struct pbuf *p, struct netif *inp, BOOL bOutgo);
VOID            packet_init(VOID);
VOID            packet_traversal(VOIDFUNCPTR pfunc, PVOID pvArg0, PVOID pvArg1, PVOID pvArg2,
                                 PVOID pvArg3, PVOID pvArg4, PVOID pvArg5);
AF_PACKET_T    *packet_socket(INT iDomain, INT iType, INT iProtocol);
INT             packet_bind(AF_PACKET_T *pafpacket, const struct sockaddr *name, socklen_t namelen);
INT             packet_listen(AF_PACKET_T *pafpacket, INT backlog);
AF_PACKET_T    *packet_accept(AF_PACKET_T *pafpacket, struct sockaddr *addr, socklen_t *addrlen);
INT             packet_connect(AF_PACKET_T *pafpacket, const struct sockaddr *name, socklen_t namelen);
ssize_t         packet_recvfrom(AF_PACKET_T *pafpacket, void *mem, size_t len, int flags,
                                struct sockaddr *from, socklen_t *fromlen);
ssize_t         packet_recv(AF_PACKET_T *pafpacket, void *mem, size_t len, int flags);
ssize_t         packet_recvmsg(AF_PACKET_T *pafpacket, struct msghdr *msg, int flags);
ssize_t         packet_sendto(AF_PACKET_T *pafpacket, const void *data, size_t size, int flags,
                              const struct sockaddr *to, socklen_t tolen);
ssize_t         packet_send(AF_PACKET_T *pafpacket, const void *data, size_t size, int flags);
ssize_t         packet_sendmsg(AF_PACKET_T *pafpacket, const struct msghdr *msg, int flags);
INT             packet_close(AF_PACKET_T *pafpacket);
INT             packet_shutdown(AF_PACKET_T *pafpacket, int how);
INT             packet_getsockname(AF_PACKET_T *pafpacket, struct sockaddr *name, socklen_t *namelen);
INT             packet_getpeername(AF_PACKET_T *pafpacket, struct sockaddr *name, socklen_t *namelen);
INT             packet_setsockopt(AF_PACKET_T *pafpacket, int level, int optname, 
                                  const void *optval, socklen_t optlen);
INT             packet_getsockopt(AF_PACKET_T *pafpacket, int level, int optname, 
                                  void *optval, socklen_t *optlen);
INT             packet_ioctl(AF_PACKET_T *pafpacket, INT  iCmd, PVOID  pvArg);
INT             packet_mmap(AF_PACKET_T *pafpacket, PLW_DEV_MMAP_AREA  pdmap);
INT             packet_unmap(AF_PACKET_T *pafpacket, PLW_DEV_MMAP_AREA  pdmap);

#endif                                                                  /*  __AF_PACKET_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/

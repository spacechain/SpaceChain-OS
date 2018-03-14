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
** 文   件   名: af_unix.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2012 年 12 月 18 日
**
** 描        述: AF_UNIX 支持
*********************************************************************************************************/

#ifndef __AF_UNIX_H
#define __AF_UNIX_H

/*********************************************************************************************************
  AF_UNIX 接收数据节点扩展数据
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_UNIX_EN > 0

typedef struct af_unix_node_ex {
    PCHAR               UNIE_pcMsgEx;                                   /*  扩展消息                    */
    socklen_t           UNIE_uiLenEx;                                   /*  扩展消息长度                */
    pid_t               UNIE_pid;                                       /*  发送信息方的 pid            */
    BOOL                UNIE_bNeedUnProc;                               /*  扩展数据是否需要unproc处理  */
    BOOL                UNIE_bValid;                                    /*  数据是否有效                */
} AF_UNIX_NEX;

/*********************************************************************************************************
  AF_UNIX 接收数据队列 (剩余信息长度为 UNIM_stLen - UNIM_stOffset)
*********************************************************************************************************/

typedef struct af_unix_node {                                           /*  一个消息节点                */
    LW_LIST_MONO        UNIM_monoManage;                                /*  待接收数据链                */
    
    PCHAR               UNIM_pcMsg;                                     /*  接收的消息                  */
    size_t              UNIM_stLen;                                     /*  消息长度                    */
    size_t              UNIM_stOffset;                                  /*  起始信息偏移                */
    
    AF_UNIX_NEX        *UNIM_punie;                                     /*  扩展数据, 控制数据          */
    
    CHAR                UNIM_cPath[1];                                  /*  发送方的地址                */
} AF_UNIX_N;

typedef struct af_unix_queue {
    PLW_LIST_MONO       UNIQ_pmonoHeader;                               /*  消息队列头                  */
    PLW_LIST_MONO       UNIQ_pmonoTail;                                 /*  消息队列结尾                */
    size_t              UNIQ_stTotal;                                   /*  总有效数据字节数            */
} AF_UNIX_Q;

/*********************************************************************************************************
  AF_UNIX 控制块
  
  关于 UNIX_hCanRead  表示当前 socket 读操作无数据时等待的信号量
  关于 UNIX_hCanWrite 表示其他 socket 写本节点时需要等待的信号量
*********************************************************************************************************/

typedef struct af_unix_t {
    LW_LIST_LINE        UNIX_lineManage;
    
    LW_LIST_RING        UNIX_ringConnect;                               /*  连接队列                    */
    LW_LIST_RING_HEADER UNIX_pringConnect;                              /*  等待连接的队列              */
    INT                 UNIX_iConnNum;                                  /*  等待连接的数量              */
    
    INT                 UNIX_iFlag;                                     /*  NONBLOCK or NOT             */
    INT                 UNIX_iType;                                     /*  STREAM / DGRAM / SEQPACKET  */
    
#define __AF_UNIX_STATUS_NONE       0                                   /*  SOCK_DGRAM 永远处于此状态   */
#define __AF_UNIX_STATUS_LISTEN     1
#define __AF_UNIX_STATUS_CONNECT    2
#define __AF_UNIX_STATUS_ESTAB      3
    INT                 UNIX_iStatus;                                   /*  当前状态 (仅针对 STREAM)    */
    
#define __AF_UNIX_SHUTD_R           0x01
#define __AF_UNIX_SHUTD_W           0x02
    INT                 UNIX_iShutDFlag;                                /*  当前 shutdown 状态          */
    
    INT                 UNIX_iBacklog;                                  /*  等待的背景连接数量          */
    
    struct af_unix_t   *UNIX_pafunxPeer;                                /*  连接的远程节点              */
    AF_UNIX_Q           UNIX_unixq;                                     /*  接收数据队列                */
    
    size_t              UNIX_stMaxBufSize;                              /*  最大接收缓冲大小            */
                                                                        /*  超过此数量写入时, 需要阻塞  */
    LW_OBJECT_HANDLE    UNIX_hCanRead;                                  /*  可读                        */
    LW_OBJECT_HANDLE    UNIX_hCanWrite;                                 /*  可写 (也可用作 accept)      */
    
    ULONG               UNIX_ulSendTimeout;                             /*  发送超时 tick               */
    ULONG               UNIX_ulRecvTimeout;                             /*  读取超时 tick               */
    ULONG               UNIX_ulConnTimeout;                             /*  连接超时 tick               */
    
    struct linger       UNIX_linger;                                    /*  延迟关闭                    */
    
    INT                 UNIX_iPassCred;                                 /*  是否允许传送认证信息        */
    
    CHAR                UNIX_cFile[MAX_FILENAME_LENGTH];
} AF_UNIX_T;

/*********************************************************************************************************
  内部函数
*********************************************************************************************************/

VOID        unix_init(VOID);
VOID        unix_traversal(VOIDFUNCPTR pfunc, PVOID pvArg0, PVOID pvArg1,
                           PVOID pvArg2, PVOID  pvArg3, PVOID  pvArg4, PVOID pvArg5);
AF_UNIX_T  *unix_socket(INT  iDomain, INT  iType, INT  iProtocol);
INT         unix_bind(AF_UNIX_T  *pafunix, const struct sockaddr *name, socklen_t namelen);
INT         unix_listen(AF_UNIX_T  *pafunix, INT  backlog);
AF_UNIX_T  *unix_accept(AF_UNIX_T  *pafunix, struct sockaddr *addr, socklen_t *addrlen);
INT         unix_connect(AF_UNIX_T  *pafunix, const struct sockaddr *name, socklen_t namelen);
INT         unix_connect2(AF_UNIX_T  *pafunix0, AF_UNIX_T  *pafunix1);
ssize_t     unix_recvfrom(AF_UNIX_T  *pafunix, void *mem, size_t len, int flags,
                          struct sockaddr *from, socklen_t *fromlen);
ssize_t     unix_recv(AF_UNIX_T  *pafunix, void *mem, size_t len, int flags);
ssize_t     unix_recvmsg(AF_UNIX_T  *pafunix, struct msghdr *msg, int flags);
ssize_t     unix_sendto(AF_UNIX_T  *pafunix, const void *data, size_t size, int flags,
                        const struct sockaddr *to, socklen_t tolen);
ssize_t     unix_send(AF_UNIX_T  *pafunix, const void *data, size_t size, int flags);
ssize_t     unix_sendmsg(AF_UNIX_T  *pafunix, const struct msghdr *msg, int flags);
INT         unix_close(AF_UNIX_T  *pafunix);
INT         unix_shutdown(AF_UNIX_T  *pafunix, int how);
INT         unix_getsockname(AF_UNIX_T  *pafunix, struct sockaddr *name, socklen_t *namelen);
INT         unix_getpeername(AF_UNIX_T  *pafunix, struct sockaddr *name, socklen_t *namelen);
INT         unix_setsockopt(AF_UNIX_T  *pafunix, int level, int optname, 
                            const void *optval, socklen_t optlen);
INT         unix_getsockopt(AF_UNIX_T  *pafunix, int level, int optname, 
                            void *optval, socklen_t *optlen);
INT         unix_ioctl(AF_UNIX_T  *pafunix, INT  iCmd, PVOID  pvArg);

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_UNIX_EN > 0      */
#endif                                                                  /*  __AF_UNIX_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/

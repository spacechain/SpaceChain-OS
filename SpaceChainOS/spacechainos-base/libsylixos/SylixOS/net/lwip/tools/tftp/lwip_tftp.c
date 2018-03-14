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
** 文   件   名: lwip_tftp.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 05 月 29 日
**
** 描        述: tftp 客户端/服务器, 主要用来获得提供系统运行镜像.
**               tftp 协议较为简单, 所以源码中没有建立会话行为抽象, 将客户机服务器都写在这个文件中.

** BUG:
2009.06.04  修正一处序号比较的问题. 同时客户端失败时会产生对应 errno.
2009.06.18  加入 TFTP 服务器目录设置 shell 命令.
2009.06.19  使用 GCC 编译器时, 由于 char 数组 cFrameBuffer 起始地址不对齐, 
            导致 ntohs(*(UINT16 *)&cFrameBuffer[0]); 可能产生不对齐产生异常.
            加入 __TFTP_HDR *hdr 将数组缓冲强转, hdr = (__TFTP_HDR *)cFrameBuffer 那么:
            ntohs(hdr->TFTPHDR_usOpc) 就一定不会出现问题, (GCC 内部有处理)
2009.06.19  API_INetTftpServerPath() 不再检测路径有效性.
            服务器加入通信超时控制.
2009.07.03  修正了 GCC 警告.
2009.09.25  更新 gethostbyname() 传回地址复制的方法.
2009.10.29  __inetTftpServerListen() 发现 socket 异常时, 直接退出.
2009.11.21  将 shell ftp 命令放在本模块中.
2011.02.21  修正头文件引用, 符合 POSIX 标准.  
2011.03.11  使用 getaddrinfo 进行域名解析.
2011.04.07  服务器线程从 /etc/services 中获取 ftp 的服务端口, 如果没有, 默认使用 69 端口.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_TFTP_EN > 0)
#include "netdb.h"
#include "arpa/inet.h"
#include "sys/socket.h"
#include "lwip_tftp.h"
/*********************************************************************************************************
  重试次数
*********************************************************************************************************/
#define __LWIP_TFTP_RETRY_TIMES     3
/*********************************************************************************************************
  tftp 错误数据包
*********************************************************************************************************/
const CHAR      _G_cTftpErrorMsg[8][16] = {
    {0x00, LWIP_TFTP_OPC_ERROR, 0x00, LWIP_TFTP_EC_NONE, 'n', 'o', 'n', 'e', 0},
    {0x00, LWIP_TFTP_OPC_ERROR, 0x00, LWIP_TFTP_EC_NOFILE, 'n', 'o', 'f', 'i', 'l', 'e', 0},
    {0x00, LWIP_TFTP_OPC_ERROR, 0x00, LWIP_TFTP_EC_NOACCESS, 'n', 'o', 'a', 'c', 'c', 'e', 's', 's', 0},
    {0x00, LWIP_TFTP_OPC_ERROR, 0x00, LWIP_TFTP_EC_DISKFULL, 'd', 'i', 's', 'k', 'f', 'u', 'l', 'l', 0},
    {0x00, LWIP_TFTP_OPC_ERROR, 0x00, LWIP_TFTP_EC_TFTP, 't', 'f', 't', 'p', 0},
    {0x00, LWIP_TFTP_OPC_ERROR, 0x00, LWIP_TFTP_EC_ID, 'i', 'd', 0},
    {0x00, LWIP_TFTP_OPC_ERROR, 0x00, LWIP_TFTP_EC_FILEEXIST, 'e', 'x', 'i', 's', 't', 0},
    {0x00, LWIP_TFTP_OPC_ERROR, 0x00, LWIP_TFTP_EC_NOUSER, 'n', 'o', 'u', 's', 'e', 'r', 0},
};
/*********************************************************************************************************
  tftp 数据包
*********************************************************************************************************/
PACK_STRUCT_BEGIN
struct __tftp_hdr {
    PACK_STRUCT_FIELD(UINT16    TFTPHDR_usOpc);                         /*  操作数                      */
    PACK_STRUCT_FIELD(UINT16    TFTPHDR_usSeq);                         /*  编号                        */
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
typedef struct __tftp_hdr       __TFTP_HDR;                             /*  TFTP 数据包头               */
typedef struct __tftp_hdr      *__PTFTP_HDR;
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static PCHAR    _G_pcTftpRootPath = LW_NULL;
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
static INT  __tshellNetTftpdPath(INT  iArgC, PCHAR  *ppcArgV);
static INT  __tshellTftp(INT  iArgC, PCHAR  *ppcArgV);
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** 函数名称: __inetTftpParseReqFrame
** 功能描述: 解析一个请求报文
** 输　入  : ptftphdr          数据包头
**           piReqType         请求类型
**           pcFrameBuffer     数据包缓冲
**           pcFileName        文件名
**           pcMode            传输模式
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __inetTftpParseReqFrame (__PTFTP_HDR    ptftphdr,
                                     INT           *piReqType,
                                     PCHAR         *ppcFileName,
                                     PCHAR         *ppcMode)
{
    UINT16      usOpc = ntohs(ptftphdr->TFTPHDR_usOpc);                 /*  操作符                      */
    size_t      stFileNameLen;
    PCHAR       pcTemp;
    
    if ((usOpc != LWIP_TFTP_OPC_RDREQ) &&
        (usOpc != LWIP_TFTP_OPC_WRREQ)) {
        return  (PX_ERROR);
    }
    
    *ppcFileName  = (PCHAR)(&ptftphdr->TFTPHDR_usSeq);                  /*  从 seq 位置开始             */
    stFileNameLen = lib_strnlen(*ppcFileName, PATH_MAX);
    pcTemp        = (*ppcFileName) + stFileNameLen;
    *pcTemp++     = PX_EOS;
    *ppcMode      = pcTemp;                                             /*  模式                        */
    
    *piReqType    = (INT)usOpc;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __inetTftpMakeReqFrame
** 功能描述: 创建一个请求报文
** 输　入  : iReqType          请求类型
**           ptftphdr          tftp 包缓冲
**           pcFileName        文件名
**           pcMode            传输模式
** 输　出  : 数据包长度
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __inetTftpMakeReqFrame (INT             iReqType, 
                                    __PTFTP_HDR     ptftphdr,
                                    CPCHAR          pcFileName, 
                                    CPCHAR          pcMode)
{
    REGISTER INT   iIndex;
    REGISTER INT   iLen;
    REGISTER PCHAR pcFileNameBuffer = (PCHAR)(&ptftphdr->TFTPHDR_usSeq);/*  从 seq 位置开始             */

    ptftphdr->TFTPHDR_usOpc = htons((UINT16)iReqType);                  /*  填写请求类型                */

    iIndex  = sprintf(pcFileNameBuffer, "%s", pcFileName);              /*  填写文件名                  */
    pcFileNameBuffer[iIndex++] = PX_EOS;
    
    iLen    = iIndex + 2;                                               /*  加入包头长度                */
    iLen   += sprintf(&pcFileNameBuffer[iIndex], "%s", pcMode);         /*  填写传输类型                */
    
    return  (iLen + 1);                                                 /*  include last '\0'           */
}
/*********************************************************************************************************
** 函数名称: __inetTftpSendReqFrame
** 功能描述: 发送一个请求数据包
** 输　入  : iSock                 套接字
**           psockaddrinRemote     远程地址
**           iReqType              请求类型
**           pcFileName            文件名
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __inetTftpSendReqFrame (INT                  iSock, 
                                    struct sockaddr_in  *psockaddrinRemote, 
                                    INT                  iReqType, 
                                    CPCHAR               pcFileName)
{
    CHAR                cSendBuffer[MAX_FILENAME_LENGTH + 8];
    __PTFTP_HDR         ptftphdr;
    
    REGISTER INT        iDataLen;
    REGISTER ssize_t    sstSend;
    
    ptftphdr = (__PTFTP_HDR)cSendBuffer;                                /*  tftp 包头位置               */
                                                                        /*  以二进制形式请求文件        */
    iDataLen = __inetTftpMakeReqFrame(iReqType, ptftphdr, pcFileName, "octet");
    
    sstSend = sendto(iSock, (const void *)cSendBuffer, iDataLen, 0,     /*  发送请求数据包              */
                     (struct sockaddr *)psockaddrinRemote, sizeof(struct sockaddr_in));
    return  ((INT)sstSend);
}
/*********************************************************************************************************
** 函数名称: __inetTftpSendAckFrame
** 功能描述: 发送一个确认数据包
** 输　入  : iSock               套接字
**           psockaddrinRemote   远程地址
**           usAck               ACK 号
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __inetTftpSendAckFrame (INT  iSock, struct sockaddr_in  *psockaddrinRemote, UINT16  usAck)
{
    __TFTP_HDR          tftphdr;
    REGISTER ssize_t    sstSend;
    
    tftphdr.TFTPHDR_usOpc = htons(LWIP_TFTP_OPC_ACK);
    tftphdr.TFTPHDR_usSeq = htons(usAck);
    
    sstSend = sendto(iSock, (const void *)&tftphdr, 4, 0,               /*  发送确认数据包              */
                   (struct sockaddr *)psockaddrinRemote, sizeof(struct sockaddr_in));
    return  ((INT)sstSend);
}
/*********************************************************************************************************
** 函数名称: __inetTftpSendDataFrame
** 功能描述: 发送一个文件数据包
** 输　入  : iSock               套接字
**           psockaddrinRemote   远程地址
**           usSeq               SEQ 号
**           ptftphdr            tftp 数据包头
**           iDataLen            (数据长度) 如果小于 512 表示文件末尾
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __inetTftpSendDataFrame (INT                  iSock, 
                                     struct sockaddr_in  *psockaddrinRemote, 
                                     UINT16               usSeq,
                                     __PTFTP_HDR          ptftphdr,
                                     INT                  iDataLen)
{
    REGISTER ssize_t  sstSend;
    
    ptftphdr->TFTPHDR_usOpc = htons(LWIP_TFTP_OPC_DATA);
    ptftphdr->TFTPHDR_usSeq = htons(usSeq);
    
    iDataLen += LWIP_TFTP_DATA_HEADERLEN;                               /*  加入包头长度                */
    
    sstSend = sendto(iSock, (const void *)ptftphdr, iDataLen, 0,        /*  发送文件数据包              */
                   (struct sockaddr *)psockaddrinRemote, sizeof(struct sockaddr_in));
    return  ((INT)sstSend);
}
/*********************************************************************************************************
** 函数名称: __inetTftpSendErrorFrame
** 功能描述: 发送一个错误信息数据包
** 输　入  : iSock               套接字
**           psockaddrinRemote   远程地址
**           usErrorCode         错误编号
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __inetTftpSendErrorFrame (INT                  iSock, 
                                      struct sockaddr_in  *psockaddrinRemote, 
                                      UINT16               usErrorCode)
{
    REGISTER ssize_t    sstSend;
    REGISTER size_t     stDataLen = lib_strlen(&_G_cTftpErrorMsg[usErrorCode][4]);
    
    stDataLen += 5;                                                     /*  include '\0'                */
    
    sstSend = sendto(iSock, (const void *)_G_cTftpErrorMsg[usErrorCode], 
                     stDataLen, 0,                                      /*  发送文件数据包              */
                     (struct sockaddr *)psockaddrinRemote, sizeof(struct sockaddr_in));
    return  ((INT)sstSend);
}
/*********************************************************************************************************
** 函数名称: __inetTftpRecvFrame
** 功能描述: 接收一个数据包
** 输　入  : iSock             套接字
**           psockaddrinRemote 远程地址
**           pcRecvBuffer      完整数据包缓冲区
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __inetTftpRecvFrame (INT                  iSock, 
                                 struct sockaddr_in  *psockaddrinRemote, 
                                 PCHAR                pcRecvBuffer, 
                                 INT                  iBufferLen)
{
    struct sockaddr_in  sockaddrinRecv;
    
    REGISTER INT        i;
    REGISTER ssize_t    sstRecv;
             socklen_t  uiAddrLen = sizeof(struct sockaddr_in);
    
    for (i = 0; i < __LWIP_TFTP_RETRY_TIMES; i++) {
        sstRecv = recvfrom(iSock, (void *)pcRecvBuffer, iBufferLen, 0,  /*  接收数据                    */
                         (struct sockaddr *)&sockaddrinRecv, &uiAddrLen);
        if (sstRecv > 0) {
            if (lib_memcmp((PVOID)&sockaddrinRecv.sin_addr, 
                           (PVOID)&psockaddrinRemote->sin_addr, 
                           sizeof(struct in_addr))) {                   /*  IP 地址与期望地址不同, 丢弃 */
                continue;
            } else {
                psockaddrinRemote->sin_port = sockaddrinRecv.sin_port;  /*  更新端口                    */
            }
        }
        return  ((INT)sstRecv);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __inetTftpReceive
** 功能描述: 通过 tftp 协议从远程计算机获得指定文件. (协议处理)
** 输　入  : iSock      套接字
**           iFd        文件描述符
**           pinaddr    远程地址
**           pcFileName 文件名
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __inetTftpReceive (INT  iSock, INT  iFd, struct in_addr  *pinaddr, CPCHAR  pcFileName)
{
#define __LWIP_TFTP_RECVBUFFER_SIZE     (512 + 4)

    struct sockaddr_in  sockaddrinRemote;
    __PTFTP_HDR         ptftphdr;                                       /*  tftp 头                     */
             PCHAR      pcFileData;
    
             INT        iRecvError = 0;                                 /*  连续接收错误                */
             ssize_t    sstWriteNum;                                    /*  写入文件的大小              */
    
    REGISTER INT        iSend;
    REGISTER INT        iRecv;
             
             UINT16     usOpc;
             UINT16     usSeq = 0;                                      /*  接收的序列号                */
             UINT16     usAck = 0;                                      /*  回复的数据包                */

             CHAR       cRecvBuffer[__LWIP_TFTP_RECVBUFFER_SIZE];       /*  接收缓冲                    */
             
             INT        iTempError;
    
    ptftphdr   = (__PTFTP_HDR)cRecvBuffer;
    pcFileData = (cRecvBuffer + LWIP_TFTP_DATA_HEADERLEN);              /*  数据帧数据位置              */
    
    sockaddrinRemote.sin_len    = sizeof(struct sockaddr_in);
    sockaddrinRemote.sin_family = AF_INET;
    sockaddrinRemote.sin_port   = htons(69);
    sockaddrinRemote.sin_addr   = *pinaddr;
                                                                        /*  发送请求数据包              */
    iSend = __inetTftpSendReqFrame(iSock, &sockaddrinRemote, LWIP_TFTP_OPC_RDREQ, pcFileName);
    if (iSend < 0) {
        return  (iSend);
    }
    
    while (iRecvError < __LWIP_TFTP_RETRY_TIMES) {                      /*  连续错误小于指定值          */
    
        iRecv = __inetTftpRecvFrame(iSock, 
                                    &sockaddrinRemote, 
                                    cRecvBuffer, 
                                    __LWIP_TFTP_RECVBUFFER_SIZE);       /*  接收数据包                  */
        if ((iRecv <= 0) && ((errno == ETIMEDOUT) || (errno == EWOULDBLOCK))) {
                                                                        /*  无法从主机接收数据          */
            iRecvError++;
            goto    __error_handle;
        }
        
        if (iRecv < LWIP_TFTP_DATA_HEADERLEN) {
            return  (PX_ERROR);                                         /*  无法收到数据包, 错误        */
        }
        
        usOpc = ntohs(ptftphdr->TFTPHDR_usOpc);                         /*  判断操作符                  */
        if (usOpc != LWIP_TFTP_OPC_DATA) {                              /*  无法获取数据                */
            _ErrorHandle(ECONNABORTED);                                 /*  中止                        */
            return  (PX_ERROR);
        }
        
        usSeq = ntohs(ptftphdr->TFTPHDR_usSeq);                         /*  判断序列号                  */
        if (usSeq != (UINT16)(usAck + 1)) {                             /*  不是我需要的数据包          */
            iRecvError++;
            goto    __error_handle;
        }
        
        iRecvError = 0;                                                 /*  接收没有错误                */
        usAck      = usSeq;                                             /*  更新确认号                  */
        __inetTftpSendAckFrame(iSock, &sockaddrinRemote, usAck);        /*  回复                        */
        
        iRecv -= LWIP_TFTP_DATA_HEADERLEN;                              /*  计算数据负载大小            */
        if (iRecv <= 0) {
            return  (ERROR_NONE);                                       /*  数据接收完毕                */
        } else {
            sstWriteNum = write(iFd, pcFileData, iRecv);
            if (sstWriteNum < iRecv) {
                __inetTftpSendErrorFrame(iSock, &sockaddrinRemote, LWIP_TFTP_EC_DISKFULL);
                return  (PX_ERROR);                                     /*  无法将数据写入文件          */
            }
            if (iRecv < 512) {
                return  (ERROR_NONE);                                   /*  数据接收完毕                */
            }
        }
        continue;                                                       /*  继续接收数据                */
        
__error_handle:
        iTempError = errno;
        if (usAck == 0) {                                               /*  需要重新发送请求包          */
            __inetTftpSendReqFrame(iSock, &sockaddrinRemote, 
                                   LWIP_TFTP_OPC_RDREQ, pcFileName);
        } else {                                                        /*  再次发送确认数据包          */
            __inetTftpSendAckFrame(iSock, &sockaddrinRemote, usAck);
        }
        errno = iTempError;
    }

    if (errno == 0) {
        _ErrorHandle(EHOSTDOWN);                                        /*  主机错误                    */
    }
    return  (PX_ERROR);                                                 /*  数据包失序                  */
}
/*********************************************************************************************************
** 函数名称: __inetTftpSend
** 功能描述: 通过 tftp 协议向远程计算机发送指定文件. (协议处理)
** 输　入  : iSock      套接字
**           iFd        文件描述符
**           pinaddr    远程地址
**           pcFileName 文件名
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __inetTftpSend (INT  iSock, INT  iFd, struct in_addr  *pinaddr, CPCHAR  pcFileName)
{
#define __LWIP_TFTP_SENDBUFFER_SIZE     (512 + 4)
#define __LWIP_TFTP_READFILE_SIZE       512

    struct sockaddr_in  sockaddrinRemote;
    
    __PTFTP_HDR         ptftphdrRecv;                                   /*  tftp 头                     */
    __PTFTP_HDR         ptftphdrSend;
             PCHAR      pcFileData;
             
             INT        iRecvError = 0;                                 /*  连续接收错误                */
             ssize_t    sstReadNum = 0;                                 /*  读取文件长度                */
             
             INT        iSendOver = 0;
    
    REGISTER INT        iSend;
    REGISTER INT        iRecv;
             
             UINT16     usOpc;
             UINT16     usSeq = 0;                                      /*  发送的序列号                */
             UINT16     usAck = 0;                                      /*  接收的确认号                */

             CHAR       cRecvBuffer[4];                                 /*  接收缓冲                    */
             CHAR       cSendBuffer[__LWIP_TFTP_SENDBUFFER_SIZE];       /*  发送缓冲                    */
             
             INT        iTempError;
             
    ptftphdrRecv = (__PTFTP_HDR)cRecvBuffer;
    ptftphdrSend = (__PTFTP_HDR)cSendBuffer;
    pcFileData   = (cSendBuffer + LWIP_TFTP_DATA_HEADERLEN);            /*  数据帧数据位置              */
    
    sockaddrinRemote.sin_len    = sizeof(struct sockaddr_in);
    sockaddrinRemote.sin_family = AF_INET;
    sockaddrinRemote.sin_port   = htons(69);
    sockaddrinRemote.sin_addr   = *pinaddr;
                                                                        /*  发送请求数据包              */
    iSend = __inetTftpSendReqFrame(iSock, &sockaddrinRemote, LWIP_TFTP_OPC_WRREQ, pcFileName);
    if (iSend < 0) {
        return  (iSend);
    }
    
    while (iRecvError < __LWIP_TFTP_RETRY_TIMES) {                      /*  连续错误小于指定值          */
    
        iRecv = __inetTftpRecvFrame(iSock, 
                                    &sockaddrinRemote, 
                                    cRecvBuffer, 
                                    4);                                 /*  接收数据包                  */
        if ((iRecv <= 0) && ((errno == ETIMEDOUT) || (errno == EWOULDBLOCK))) {
                                                                        /*  无法从主机接收数据          */
            iRecvError++;
            goto    __error_handle;
        }
        
        if (iRecv < 4) {
            return  (PX_ERROR);                                         /*  无法收到数据包, 错误        */
        }
        
        usOpc = ntohs(ptftphdrRecv->TFTPHDR_usOpc);                     /*  判断操作符                  */
        if (usOpc != LWIP_TFTP_OPC_ACK) {                               /*  无法获取数据                */
            _ErrorHandle(ECONNABORTED);                                 /*  中止                        */
            return  (PX_ERROR);
        }
        
        usAck = ntohs(ptftphdrRecv->TFTPHDR_usSeq);                     /*  判断确认号                  */
        if (usAck != usSeq) {                                           /*  不是我需要的确认包          */
            iRecvError++;
            goto    __error_handle;
        }
        
        if (iSendOver) {                                                /*  已经发送了最后一个数据包    */
            return  (ERROR_NONE);
        }
        
        usSeq++;                                                        /*  准备发送下一包数据          */
        sstReadNum = read(iFd, pcFileData, __LWIP_TFTP_READFILE_SIZE);  /*  从文件读取数据              */
        if (sstReadNum < 0) {
            __inetTftpSendErrorFrame(iSock, &sockaddrinRemote, LWIP_TFTP_EC_NOACCESS);
            return  (PX_ERROR);                                         /*  无法读取数据文件            */
        } else {
            __inetTftpSendDataFrame(iSock, &sockaddrinRemote, usSeq,
                                    ptftphdrSend, (INT)sstReadNum);     /*  发送数据包                  */
            if (sstReadNum < __LWIP_TFTP_READFILE_SIZE) {
                iSendOver = 1;                                          /*  这是最后一个数据包          */
            }
        }
        continue;                                                       /*  继续接收数据                */
        
__error_handle:
        iTempError = errno;
        if (usSeq == 0) {                                               /*  还没有发送一个数据包        */
            __inetTftpSendReqFrame(iSock, &sockaddrinRemote, LWIP_TFTP_OPC_WRREQ, pcFileName);
        } else {                                                        /*  需要重发数据                */
            __inetTftpSendDataFrame(iSock, &sockaddrinRemote, usSeq,
                                    ptftphdrSend, (INT)sstReadNum);     /*  发送数据包                  */
        }
        errno = iTempError;
    }
    
    if (errno == 0) {
        _ErrorHandle(EHOSTDOWN);                                        /*  主机错误                    */
    }
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_INetTftpReceive
** 功能描述: 通过 tftp 协议从远程计算机获得指定文件.
** 输　入  : pcRemoteHost      远程主机
**           pcFileName        需要获取的文件文件名
**           pcLocalFileName   转存入本地的文件名
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_INetTftpReceive (CPCHAR  pcRemoteHost, CPCHAR  pcFileName, CPCHAR  pcLocalFileName)
{
    struct addrinfo      hints;
    struct addrinfo     *phints = LW_NULL;

    struct in_addr  inaddrRemote;
    INT             iTimeout = LWIP_TFTP_TIMEOUT;
    
    INT             iDestFd;
    INT             iSock;
    
    INT             iError;
    ULONG           ulError;
    
    if (!pcRemoteHost || !pcFileName || !pcLocalFileName) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (lib_strnlen(pcFileName, MAX_FILENAME_LENGTH) >= MAX_FILENAME_LENGTH) {
        _ErrorHandle(ERROR_IO_NAME_TOO_LONG);
        return  (PX_ERROR);
    }
    
    if (!inet_aton(pcRemoteHost, &inaddrRemote)) {
        hints.ai_family = AF_UNSPEC;
        hints.ai_flags  = AI_CANONNAME;
        getaddrinfo(pcRemoteHost, LW_NULL, &hints, &phints);            /*  域名解析                    */
        if (phints == LW_NULL) {
            _ErrorHandle(EHOSTUNREACH);
            return  (PX_ERROR);
        } else {
            if (phints->ai_addr->sa_family == AF_INET) {                /*  获得网络地址                */
                inaddrRemote = ((struct sockaddr_in *)(phints->ai_addr))->sin_addr;
                freeaddrinfo(phints);
            } else {
                freeaddrinfo(phints);
                _ErrorHandle(EAFNOSUPPORT);
                return  (PX_ERROR);
            }
        }
    }
                                                                        /*  打开目的文件                */
    iDestFd = open(pcLocalFileName, (O_WRONLY | O_CREAT | O_TRUNC), DEFAULT_FILE_PERM);
    if (iDestFd < 0) {
        return  (PX_ERROR);
    }
    
    iSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);                   /*  创建 socket                 */
    if (iSock < 0) {
        close(iDestFd);                                                 /*  关闭文件                    */
        unlink(pcLocalFileName);                                        /*  删除文件                    */
        return  (PX_ERROR);
    }                                                                   /*  设置超时时间                */
    setsockopt(iSock, SOL_SOCKET, SO_RCVTIMEO, (const void *)&iTimeout, sizeof(INT));
    
    iError = __inetTftpReceive(iSock, iDestFd, 
                               &inaddrRemote, pcFileName);              /*  开始接收文件                */
    ulError = errno;
    
    close(iDestFd);                                                     /*  关闭文件                    */
    close(iSock);                                                       /*  关闭 socket                 */
    
    if (iError < 0) {
        unlink(pcLocalFileName);                                        /*  删除文件                    */
        _ErrorHandle(ulError);
        return  (iError);
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: API_INetTftpSend
** 功能描述: 通过 tftp 协议向远程计算机发送指定文件.
** 输　入  : pcRemoteHost      远程主机
**           pcFileName        存入服务器端的文件名
**           pcLocalFileName   本地文件名
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_INetTftpSend (CPCHAR  pcRemoteHost, CPCHAR  pcFileName, CPCHAR  pcLocalFileName)
{
    struct addrinfo      hints;
    struct addrinfo     *phints = LW_NULL;

    struct in_addr  inaddrRemote;
    INT             iTimeout = LWIP_TFTP_TIMEOUT;
    
    INT             iSrcFd;
    INT             iSock;
    
    INT             iError;
    ULONG           ulError;
    
    if (!pcRemoteHost || !pcFileName || !pcLocalFileName) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if (lib_strnlen(pcFileName, MAX_FILENAME_LENGTH) >= MAX_FILENAME_LENGTH) {
        _ErrorHandle(ERROR_IO_NAME_TOO_LONG);
        return  (PX_ERROR);
    }
    
    if (!inet_aton(pcRemoteHost, &inaddrRemote)) {
        hints.ai_family = AF_UNSPEC;
        hints.ai_flags  = AI_CANONNAME;
        getaddrinfo(pcRemoteHost, LW_NULL, &hints, &phints);            /*  域名解析                    */
        if (phints == LW_NULL) {
            _ErrorHandle(EHOSTUNREACH);
            return  (PX_ERROR);
        } else {
            if (phints->ai_addr->sa_family == AF_INET) {                /*  获得网络地址                */
                inaddrRemote = ((struct sockaddr_in *)(phints->ai_addr))->sin_addr;
                freeaddrinfo(phints);
            } else {
                freeaddrinfo(phints);
                _ErrorHandle(EAFNOSUPPORT);
                return  (PX_ERROR);
            }
        }
    }
                                                                        /*  打开源文件                  */
    iSrcFd = open(pcLocalFileName, O_RDONLY);
    if (iSrcFd < 0) {
        return  (PX_ERROR);
    }
    
    iSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);                   /*  创建 socket                 */
    if (iSock < 0) {
        close(iSrcFd);                                                  /*  关闭文件                    */
        return  (PX_ERROR);
    }                                                                   /*  设置超时时间                */
    setsockopt(iSock, SOL_SOCKET, SO_RCVTIMEO, (const void *)&iTimeout, sizeof(INT));
    
    iError = __inetTftpSend(iSock, iSrcFd, 
                            &inaddrRemote, pcFileName);                 /*  开始发送文件                */
                            
    ulError = errno;
    
    close(iSrcFd);                                                      /*  关闭文件                    */
    close(iSock);                                                       /*  关闭 socket                 */
    
    if (iError < 0) {
        _ErrorHandle(ulError);
        return  (iError);
    } else {
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
  tftp 服务器端
*********************************************************************************************************/
typedef struct {
    INT                     TFTPSA_iReqType;                            /*  请求方式                    */
    INT                     TFTPSA_iFd;                                 /*  文件描述符                  */
    INT                     TFTPSA_iSock;                               /*  通信接口文件描述符          */
    struct sockaddr_in      TFTPSA_sockaddrinRemote;                    /*  远程主机地址                */
    CHAR                    TFTPSA_cFileName[1];                        /*  文件名                      */
} __LW_TFTP_SERVERARG;
typedef __LW_TFTP_SERVERARG     *__PLW_TFTP_SERVERARG;
/*********************************************************************************************************
** 函数名称: __inetTftpServerCleanup
** 功能描述: 清除 tftp 服务器参数内存
** 输　入  : ptftpsa       服务器参数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __inetTftpServerCleanup (__PLW_TFTP_SERVERARG  ptftpsa)
{
    if (ptftpsa) {
        close(ptftpsa->TFTPSA_iSock);                                   /*  关闭临时通信端口            */
        __SHEAP_FREE(ptftpsa);                                          /*  释放内存                    */
    }
}
/*********************************************************************************************************
** 函数名称: __inetTftpRdServer
** 功能描述: tftp 服务器线程 (读服务器)
** 输　入  : ptftpsa       服务器参数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __inetTftpRdServer (__PLW_TFTP_SERVERARG   ptftpsa)
{
#define __LWIP_TFTP_SENDBUFFER_SIZE     (512 + 4)
#define __LWIP_TFTP_READFILE_SIZE       512

    __PTFTP_HDR         ptftphdrRecv;                                   /*  tftp 头                     */
    __PTFTP_HDR         ptftphdrSend;
             PCHAR      pcFileData;

             INT        iRecvError = 0;                                 /*  连续接收错误                */
             ssize_t    sstReadNum;                                     /*  读取文件长度                */
             
             INT        iSendOver = 0;
    
    REGISTER INT        iRecv;
             
             UINT16     usOpc;
             UINT16     usSeq = 0;                                      /*  发送的序列号                */
             UINT16     usAck = 0;                                      /*  接收的确认号                */

             CHAR       cRecvBuffer[4];                                 /*  接收缓冲                    */
             CHAR       cSendBuffer[__LWIP_TFTP_SENDBUFFER_SIZE];       /*  发送缓冲                    */
             
    API_ThreadCleanupPush(__inetTftpServerCleanup, (PVOID)ptftpsa);
    
    ptftphdrRecv = (__PTFTP_HDR)cRecvBuffer;
    ptftphdrSend = (__PTFTP_HDR)cSendBuffer;
    pcFileData   = (cSendBuffer + LWIP_TFTP_DATA_HEADERLEN);            /*  数据帧数据位置              */
    
    while (iRecvError < __LWIP_TFTP_RETRY_TIMES) {
        
        usSeq++;                                                        /*  准备发送下一包数据          */
        sstReadNum = read(ptftpsa->TFTPSA_iFd, pcFileData, 
                          __LWIP_TFTP_READFILE_SIZE);                   /*  从文件读取数据              */
        if (sstReadNum < 0) {
            __inetTftpSendErrorFrame(ptftpsa->TFTPSA_iSock, 
                                     &ptftpsa->TFTPSA_sockaddrinRemote, 
                                     LWIP_TFTP_EC_NOACCESS);
            break;
        } else {
            __inetTftpSendDataFrame(ptftpsa->TFTPSA_iSock, 
                                    &ptftpsa->TFTPSA_sockaddrinRemote, 
                                    usSeq,
                                    ptftphdrSend, 
                                    (INT)sstReadNum);                   /*  发送数据包                  */
            if (sstReadNum < __LWIP_TFTP_READFILE_SIZE) {
                iSendOver = 1;                                          /*  这是最后一个数据包          */
            }
        }
        
        iRecv = __inetTftpRecvFrame(ptftpsa->TFTPSA_iSock, 
                                    &ptftpsa->TFTPSA_sockaddrinRemote, 
                                    cRecvBuffer, 
                                    4);                                 /*  接收数据包                  */
        if ((iRecv <= 0) && ((errno == ETIMEDOUT) || (errno == EWOULDBLOCK))) {
                                                                        /*  无法从主机接收数据          */
            iRecvError++;
            goto    __error_handle;
        }
        
        if (iRecv < 4) {
            break;                                                      /*  无法收到数据包, 错误        */
        }
        
        usOpc = ntohs(ptftphdrRecv->TFTPHDR_usOpc);                     /*  判断操作符                  */
        if (usOpc != LWIP_TFTP_OPC_ACK) {                               /*  无法获取数据                */
            break;
        }
        
        usAck = ntohs(ptftphdrRecv->TFTPHDR_usSeq);                     /*  判断确认号                  */
        if (usAck != usSeq) {                                           /*  不是我需要的确认包          */
            iRecvError++;
            goto    __error_handle;
        }
        
        if (iSendOver) {                                                /*  已经发送了最后一个数据包    */
            close(ptftpsa->TFTPSA_iFd);
            return;                                                     /*  数据发送完毕                */
        }
        continue;                                                       /*  继续接收数据                */
        
        /*
         *  注意: 由于这里采用了超时重发, 所以为了节省内存, 就没有采用预读文件加速机制.
         */
__error_handle:
        __inetTftpSendDataFrame(ptftpsa->TFTPSA_iSock, 
                                &ptftpsa->TFTPSA_sockaddrinRemote, 
                                usSeq,
                                ptftphdrSend,
                                (INT)sstReadNum);                       /*  发送数据包                  */
    }
    
    close(ptftpsa->TFTPSA_iFd);                                         /*  关闭文件                    */
}
/*********************************************************************************************************
** 函数名称: __inetTftpWrServer
** 功能描述: tftp 服务器线程 (写服务器)
** 输　入  : ptftpsa       服务器参数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __inetTftpWrServer (__PLW_TFTP_SERVERARG   ptftpsa)
{
#define __LWIP_TFTP_RECVBUFFER_SIZE     (512 + 4)

    __PTFTP_HDR         ptftphdr;                                       /*  tftp 头                     */
             PCHAR      pcFileData;                                     /*  文件数据指针                */
             
             INT        iRecvError = 0;                                 /*  连续接收错误                */
             ssize_t    sstWriteNum;                                    /*  写入文件的大小              */
             
    REGISTER INT        iRecv;
             
             UINT16     usOpc;
             UINT16     usSeq = 0;                                      /*  发送的序列号                */
             UINT16     usAck = 0;                                      /*  接收的确认号                */
             
             CHAR       cRecvBuffer[__LWIP_TFTP_RECVBUFFER_SIZE];       /*  接收缓冲                    */
    
    API_ThreadCleanupPush(__inetTftpServerCleanup, (PVOID)ptftpsa);     /*  线程退出后需要清除 ptftpsa  */
    
    ptftphdr   = (__PTFTP_HDR)cRecvBuffer;
    pcFileData = (cRecvBuffer + LWIP_TFTP_DATA_HEADERLEN);              /*  数据帧数据位置              */
    
    __inetTftpSendAckFrame(ptftpsa->TFTPSA_iSock, 
                           &ptftpsa->TFTPSA_sockaddrinRemote,
                           usAck);                                      /*  发送 ACK 报文准备接收数据   */
    
    while (iRecvError < __LWIP_TFTP_RETRY_TIMES) {
    
        iRecv = __inetTftpRecvFrame(ptftpsa->TFTPSA_iSock, 
                                    &ptftpsa->TFTPSA_sockaddrinRemote, 
                                    cRecvBuffer, 
                                    __LWIP_TFTP_RECVBUFFER_SIZE);       /*  接收数据包                  */
        if ((iRecv <= 0) && ((errno == ETIMEDOUT) || (errno == EWOULDBLOCK))) {
                                                                        /*  无法从主机接收数据          */
            iRecvError++;
            goto    __error_handle;
        }
        
        if (iRecv < LWIP_TFTP_DATA_HEADERLEN) {
            break;                                                      /*  无法收到数据包, 错误        */
        }
        
        usOpc = ntohs(ptftphdr->TFTPHDR_usOpc);                         /*  判断操作符                  */
        if (usOpc != LWIP_TFTP_OPC_DATA) {                              /*  无法获取数据                */
            break;
        }
        
        usSeq = ntohs(ptftphdr->TFTPHDR_usSeq);                         /*  判断序列号                  */
        if (usSeq != (UINT16)(usAck + 1)) {                             /*  不是我需要的数据包          */
            iRecvError++;
            goto    __error_handle;
        }
        
        iRecvError = 0;                                                 /*  接收没有错误                */
        usAck      = usSeq;                                             /*  更新确认号                  */
        __inetTftpSendAckFrame(ptftpsa->TFTPSA_iSock, 
                               &ptftpsa->TFTPSA_sockaddrinRemote, 
                               usAck);                                  /*  回复                        */
        
        iRecv -= LWIP_TFTP_DATA_HEADERLEN;                              /*  计算数据负载大小            */
        if (iRecv <= 0) {
            close(ptftpsa->TFTPSA_iFd);
            return;                                                     /*  数据接收完毕                */
        } else {
            sstWriteNum = write(ptftpsa->TFTPSA_iFd, pcFileData, iRecv);/*  写数据                      */
            if (sstWriteNum < iRecv) {
                __inetTftpSendErrorFrame(ptftpsa->TFTPSA_iSock, 
                                         &ptftpsa->TFTPSA_sockaddrinRemote, 
                                         LWIP_TFTP_EC_DISKFULL);        /*  无法将数据写入文件          */
                break;
            }
            if (iRecv < 512) {
                close(ptftpsa->TFTPSA_iFd);
                return;                                                 /*  数据接收完毕                */
            }
        }
        continue;
        
__error_handle:
        __inetTftpSendAckFrame(ptftpsa->TFTPSA_iSock, 
                               &ptftpsa->TFTPSA_sockaddrinRemote, 
                               usAck);                                  /*  再次发送确认数据包          */
    }
    
    close(ptftpsa->TFTPSA_iFd);                                         /*  关闭数据文件                */
    unlink(ptftpsa->TFTPSA_cFileName);                                  /*  出现错误, 需要删除文件      */
}
/*********************************************************************************************************
** 函数名称: __inetTftpServerListen
** 功能描述: tftp 服务器侦听服务线程
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __inetTftpServerListen (VOID)
{
    INT                     iSock;                                      /*  主服务                      */
    INT                     iTempSock;                                  /*  临时服务                    */
    INT                     iFd;
    
    INT                     iTimeout = LWIP_TFTP_TIMEOUT;               /*  临时连接超时时间            */
    
    __PTFTP_HDR             ptftphdr;                                   /*  tftp 头                     */
    __PLW_TFTP_SERVERARG    ptftpsa;                                    /*  服务器参数                  */
    LW_CLASS_THREADATTR     threadattr;                                 /*  服务器线程属性              */
    ULONG                   ulThreadHandle;
    
    struct sockaddr_in      sockaddrinLocal;
    struct sockaddr_in      sockaddrinRemote;
    socklen_t               uiAddrLen = sizeof(struct sockaddr_in);
    
    struct servent         *pservent;
    
    INT                     iErrLevel = 0;
    
    REGISTER ssize_t        sstRecv;
    CHAR                    cRecvBuffer[MAX_FILENAME_LENGTH + 8];       /*  接收缓冲区                  */
    CHAR                    cFullFileName[MAX_FILENAME_LENGTH];         /*  完整文件名                  */
    
    INT                     iReqType;
    PCHAR                   pcFileName;
    PCHAR                   pcMode;
    
    ptftphdr = (__PTFTP_HDR)cRecvBuffer;
    
    API_ThreadAttrBuild(&threadattr,
                        LW_CFG_NET_TFTP_STK_SIZE,
                        LW_PRIO_T_SERVICE,
                        LW_OPTION_THREAD_STK_CHK | LW_OPTION_OBJECT_GLOBAL,
                        LW_NULL);                                       /*  创建线程属性块              */
    
    iSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);                   /*  创建 socket                 */
    if (iSock < 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create socket.\r\n");
        return;
    }
    
    sockaddrinLocal.sin_len         = sizeof(struct sockaddr_in);
    sockaddrinLocal.sin_family      = AF_INET;
    sockaddrinLocal.sin_addr.s_addr = INADDR_ANY;
    
    pservent = getservbyname("tftp", "udp");
    if (pservent) {
        sockaddrinLocal.sin_port = (u16_t)pservent->s_port;
    } else {
        sockaddrinLocal.sin_port = htons(69);                           /*  tftp default port           */
    }
                                                                        /*  绑定服务器端口              */
    if (bind(iSock, (struct sockaddr *)&sockaddrinLocal, sizeof(struct sockaddr_in)) < 0) {
        close(iSock);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not bind server port.\r\n");
        return;
    }
    
    for (;;) {
        sstRecv = recvfrom(iSock, (void *)cRecvBuffer, (MAX_FILENAME_LENGTH + 8), 0,
                           (struct sockaddr *)&sockaddrinRemote, &uiAddrLen);
        if (sstRecv <= 0) {
            if ((errno != ETIMEDOUT) && (errno != EWOULDBLOCK)) {       /*  69 socket error!            */
                close(iSock);
                _DebugHandle(__ERRORMESSAGE_LEVEL, "socket fd error.\r\n");
                return;
            }
            continue;
        }
        cRecvBuffer[sstRecv] = PX_EOS;
                                                                        /*  分析请求报文                */
        if (__inetTftpParseReqFrame(ptftphdr, &iReqType, &pcFileName, &pcMode)) {
            __inetTftpSendErrorFrame(iSock, &sockaddrinRemote, LWIP_TFTP_EC_TFTP);
            continue;
        }
        
        if (lib_strcasecmp(pcMode, "octet")) {                          /*  仅支持二进制模式            */
            __inetTftpSendErrorFrame(iSock, &sockaddrinRemote, LWIP_TFTP_EC_TFTP);
            continue;
        }
                                                                        /*  构造文件名                  */
        snprintf(cFullFileName, MAX_FILENAME_LENGTH, "%s/%s", _G_pcTftpRootPath, pcFileName);
        
        if (iReqType == LWIP_TFTP_OPC_RDREQ) {                          /*  读请求                      */
            iFd = open(cFullFileName, O_RDONLY);
            if (iFd < 0) {
                __inetTftpSendErrorFrame(iSock, &sockaddrinRemote, LWIP_TFTP_EC_NOFILE);
                continue;
            }
        } else if (iReqType == LWIP_TFTP_OPC_WRREQ) {                   /*  写请求                      */
            iFd = open(cFullFileName, (O_CREAT | O_EXCL | O_WRONLY), DEFAULT_FILE_PERM);
            if (iFd < 0) {
                __inetTftpSendErrorFrame(iSock, &sockaddrinRemote, LWIP_TFTP_EC_FILEEXIST);
                continue;
            }
        } else {
            continue;                                                   /*  错误                        */
        }
        
        iTempSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);           /*  创建临时通信接口            */
        if (iTempSock < 0) {
            iErrLevel = 1;
            goto    __error_handle;
        }                                                               /*  设置通信超时                */
        setsockopt(iTempSock, SOL_SOCKET, SO_RCVTIMEO, (const void *)&iTimeout, sizeof(INT));
        
        if (iReqType == LWIP_TFTP_OPC_RDREQ) {                          /*  创建服务器参数              */
            ptftpsa = (__PLW_TFTP_SERVERARG)__SHEAP_ALLOC(sizeof(__LW_TFTP_SERVERARG));
            if (ptftpsa == LW_NULL) {
                iErrLevel = 2;
                goto    __error_handle;
            }
            ptftpsa->TFTPSA_cFileName[0] = PX_EOS;                      /*  读取文件不需要记录文件名    */
        } else {
            ptftpsa = (__PLW_TFTP_SERVERARG)__SHEAP_ALLOC(sizeof(__LW_TFTP_SERVERARG) + 
                                                          lib_strlen(cFullFileName));
            if (ptftpsa == LW_NULL) {
                iErrLevel = 2;
                goto    __error_handle;
            }
            lib_strcpy(ptftpsa->TFTPSA_cFileName, cFullFileName);       /*  写入时需要记录文件名        */
        }                                                               /*  当传输失败时, 需要删除文件  */
        ptftpsa->TFTPSA_iReqType         = iReqType;
        ptftpsa->TFTPSA_iFd              = iFd;
        ptftpsa->TFTPSA_iSock            = iTempSock;
        ptftpsa->TFTPSA_sockaddrinRemote = sockaddrinRemote;            /*  记录远程地址                */
        
        API_ThreadAttrSetArg(&threadattr, (PVOID)ptftpsa);
        if (iReqType == LWIP_TFTP_OPC_RDREQ) {                          /*  读请求                      */
            ulThreadHandle = API_ThreadCreate("t_tftprtmp", 
                                              (PTHREAD_START_ROUTINE)__inetTftpRdServer, 
                                              &threadattr, LW_NULL);
        } else {
            ulThreadHandle = API_ThreadCreate("t_tftpwtmp", 
                                              (PTHREAD_START_ROUTINE)__inetTftpWrServer, 
                                              &threadattr, LW_NULL);
        }
        if (ulThreadHandle == LW_OBJECT_HANDLE_INVALID) {
            iErrLevel = 3;
            goto    __error_handle;
        }
        continue;                                                       /*  等待下一请求者              */
        
__error_handle:
        if (iErrLevel > 2) {
            __SHEAP_FREE(ptftpsa);
        }
        if (iErrLevel > 1) {
            close(iTempSock);
        }
        if (iErrLevel > 0) {
            close(iFd);
            if (iReqType == LWIP_TFTP_OPC_WRREQ) {                      /*  写服务器操作                */
                unlink(cFullFileName);                                  /*  删除创建的文件              */
            }
        }
        __inetTftpSendErrorFrame(iSock, &sockaddrinRemote, LWIP_TFTP_EC_TFTP);
    }
}
/*********************************************************************************************************
** 函数名称: API_INetTftpServerInit
** 功能描述: 初始化 tftp 服务器
** 输　入  : pcPath        本地目录
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_INetTftpServerInit (CPCHAR  pcPath)
{
    static BOOL             bIsInit = LW_FALSE;
    LW_CLASS_THREADATTR     threadattr;
           PCHAR            pcNewPath = "\0";
    
    if (bIsInit) {                                                      /*  已经初始化了                */
        return;
    } else {
        bIsInit = LW_TRUE;
    }
    
    if (pcPath) {                                                       /*  需要设置服务器目录          */
        pcNewPath = (PCHAR)pcPath;
    }
    if (_G_pcTftpRootPath) {
        __SHEAP_FREE(_G_pcTftpRootPath);
    }
    _G_pcTftpRootPath = (PCHAR)__SHEAP_ALLOC(lib_strlen(pcNewPath) + 1);
    if (_G_pcTftpRootPath == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        goto    __error_handle;
    }
    lib_strcpy(_G_pcTftpRootPath, pcNewPath);                           /*  保存服务器路径              */
    
    API_ThreadAttrBuild(&threadattr,
                        LW_CFG_NET_TFTP_STK_SIZE,
                        LW_PRIO_T_SERVICE,
                        LW_OPTION_THREAD_STK_CHK | LW_OPTION_OBJECT_GLOBAL,
                        LW_NULL);
    if (API_ThreadCreate("t_tftpd", (PTHREAD_START_ROUTINE)__inetTftpServerListen, 
                         &threadattr, LW_NULL) == LW_OBJECT_HANDLE_INVALID) {
        goto    __error_handle;
    }
    
#if LW_CFG_SHELL_EN > 0
    /*
     *  加入 SHELL 命令.
     */
    API_TShellKeywordAdd("tftpdpath", __tshellNetTftpdPath);
    API_TShellFormatAdd("tftpdpath", " [new path]");
    API_TShellHelpAdd("tftpdpath",   "set default tftp server path.\n");
    
    API_TShellKeywordAdd("tftp", __tshellTftp);
    API_TShellFormatAdd("tftp", " [-i] [Host] [{get | put}] [Source] [Destination]");
    API_TShellHelpAdd("tftp",   "exchange file using tftp protocol.\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
    
    return;
    
__error_handle:
    bIsInit = LW_FALSE;                                                 /*  初始化失败                  */
}
/*********************************************************************************************************
** 函数名称: API_INetTftpServerPath
** 功能描述: 设置 tftp 服务器根目录
** 输　入  : pcPath        本地目录
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_INetTftpServerPath (CPCHAR  pcPath)
{
    REGISTER PCHAR    pcNewPath;
    REGISTER PCHAR    pcTemp = _G_pcTftpRootPath;
    
    if (pcPath == LW_NULL) {                                            /*  目录为空                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    pcNewPath = (PCHAR)__SHEAP_ALLOC(lib_strlen(pcPath) + 1);
    if (pcNewPath == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_strcpy(pcNewPath, pcPath);                                      /*  拷贝新的路径                */
    
    __KERNEL_MODE_PROC(
        _G_pcTftpRootPath = pcNewPath;                                  /*  设置新的服务器路径          */
    );
    
    if (pcTemp) {
        __SHEAP_FREE(pcTemp);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellNetTftpdPath
** 功能描述: 系统命令 "tftpdpath"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellNetTftpdPath (INT  iArgC, PCHAR  *ppcArgV)
{
    if (iArgC < 2) {
        printf("tftpd path: %s\n", _G_pcTftpRootPath);                  /*  打印当前服务器目录          */
        return  (ERROR_NONE);
    }
    
    return  (API_INetTftpServerPath(ppcArgV[1]));
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** 函数名称: __tshellTftp
** 功能描述: 系统命令 "tftp"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellTftp (INT  iArgC, PCHAR  *ppcArgV)
{
    INT     iError;
    PCHAR   pcLocalFile;
    PCHAR   pcRemoteFile;

    if ((iArgC != 6) && (iArgC != 5)) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    if (lib_strcmp(ppcArgV[1], "-i")) {
        fprintf(stderr, "must use -i option (binary type)!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    if (lib_strcasecmp(ppcArgV[3], "get") == 0) {
        pcRemoteFile = ppcArgV[4];
        if (iArgC == 6) {
            pcLocalFile = ppcArgV[5];
        } else {
            pcLocalFile = pcRemoteFile;
        }
        printf("getting file...\n");
        iError = API_INetTftpReceive(ppcArgV[2], pcRemoteFile, pcLocalFile);
    } else if (lib_strcasecmp(ppcArgV[3], "put") == 0) {
        pcRemoteFile = ppcArgV[5];
        if (iArgC == 6) {
            pcLocalFile = ppcArgV[4];
        } else {
            pcLocalFile = pcRemoteFile;
        }
        printf("sending file...\n");
        iError = API_INetTftpSend(ppcArgV[2], pcRemoteFile, pcLocalFile);
    } else {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }

    if (iError < 0) {
        if (errno == EHOSTUNREACH) {
            fprintf(stderr, "can not find host.\n");
        } else {
            fprintf(stderr, "some error occur, error: %s\n", lib_strerror(errno));
        }
    } else {
        printf("file transfer completed.\n");
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_TFTP_EN > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/

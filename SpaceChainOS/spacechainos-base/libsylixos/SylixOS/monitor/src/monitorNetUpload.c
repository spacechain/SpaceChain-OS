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
** 文   件   名: monitorNetUpload.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 08 月 17 日
**
** 描        述: SylixOS 内核事件监控器, 通过网络上传.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if (LW_CFG_MONITOR_EN > 0) && (LW_CFG_NET_EN > 0)
#include "socket.h"
#include "netinet/in.h"
#include "netinet6/in6.h"
/*********************************************************************************************************
** 函数名称: API_MonitorNetUploadCreate
** 功能描述: 创建一个采用网络上传方式的监控跟踪节点
** 输　入  : pcIp          IP 地址
**           usPort        端口 (主机字节序)
**           stSize        缓冲区大小
**           pthreadattr   监控任务创建选项
** 输　出  : Upload 节点
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
PVOID  API_MonitorNetUploadCreate (CPCHAR                pcIp, 
                                   UINT16                usPort,
                                   size_t                stSize,
                                   PLW_CLASS_THREADATTR  pthreadattr)
{
    PVOID               pvMonitorUpload;
    INT                 iSock;
    struct sockaddr_in  sockaddrinDest;
    
    if (!pcIp || !usPort) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (LW_NULL);
    }
    
    iSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);                  /*  use tcp connect             */
    if (iSock < 0) {
        return  (LW_NULL);
    }
    
    sockaddrinDest.sin_len         = sizeof(struct sockaddr_in);
    sockaddrinDest.sin_family      = AF_INET;
    sockaddrinDest.sin_port        = htons(usPort);
    sockaddrinDest.sin_addr.s_addr = inet_addr(pcIp);
    
    if (connect(iSock, (const struct sockaddr *)&sockaddrinDest, sizeof(struct sockaddr_in)) < 0) {
        close(iSock);
        return  (LW_NULL);
    }
    
    pvMonitorUpload = API_MonitorUploadCreate(iSock, stSize, LW_MONITOR_UPLOAD_PROTO, pthreadattr);
    if (!pvMonitorUpload) {
        close(iSock);
        return  (LW_NULL);
    }
    
    return  (pvMonitorUpload);
}
/*********************************************************************************************************
** 函数名称: API_MonitorNet6UploadCreate
** 功能描述: 创建一个采用网络上传方式的监控跟踪节点 (IPv6)
** 输　入  : pcIp          IP 地址
**           usPort        端口 (主机字节序)
**           stSize        缓冲区大小
**           pthreadattr   监控任务创建选项
** 输　出  : Upload 节点
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
PVOID  API_MonitorNet6UploadCreate (CPCHAR                pcIp, 
                                    UINT16                usPort,
                                    size_t                stSize,
                                    PLW_CLASS_THREADATTR  pthreadattr)
{
    PVOID               pvMonitorUpload;
    INT                 iSock;
    struct sockaddr_in6 sockaddrin6Dest;
    
    if (!pcIp || !usPort) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (LW_NULL);
    }
    
    iSock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);                 /*  use tcp connect             */
    if (iSock < 0) {
        return  (LW_NULL);
    }
    
    sockaddrin6Dest.sin6_len      = sizeof(struct sockaddr_in6);
    sockaddrin6Dest.sin6_family   = AF_INET6;
    sockaddrin6Dest.sin6_port     = htons(usPort);
    sockaddrin6Dest.sin6_flowinfo = 0;
    sockaddrin6Dest.sin6_scope_id = 0;
    inet6_aton(pcIp, &sockaddrin6Dest.sin6_addr);
    
    if (connect(iSock, (const struct sockaddr *)&sockaddrin6Dest, sizeof(struct sockaddr_in6)) < 0) {
        close(iSock);
        return  (LW_NULL);
    }
    
    pvMonitorUpload = API_MonitorUploadCreate(iSock, stSize, LW_MONITOR_UPLOAD_PROTO, pthreadattr);
    if (!pvMonitorUpload) {
        close(iSock);
        return  (LW_NULL);
    }
    
    return  (pvMonitorUpload);
}
/*********************************************************************************************************
** 函数名称: API_MonitorNetUploadDelete
** 功能描述: 删除一个采用网络上传方式的监控跟踪节点
** 输　入  : pvMonitorUpload   文件上传监控跟踪节点
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorNetUploadDelete (PVOID  pvMonitorUpload)
{
    INT     iSock;
    ULONG   ulError;
    
    ulError = API_MonitorUploadFd(pvMonitorUpload, &iSock);
    if (ulError) {
        return  (ulError);
    }
    
    ulError = API_MonitorUploadDelete(pvMonitorUpload);
    if (ulError) {
        return  (ulError);
    }
    
    close(iSock);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
                                                                        /*  LW_CFG_NET_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/

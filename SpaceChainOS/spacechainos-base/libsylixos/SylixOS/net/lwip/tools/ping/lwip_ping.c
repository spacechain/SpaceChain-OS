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
** 文   件   名: lwip_ping.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 05 月 06 日
**
** 描        述: lwip ping 工具. (相关源码来源于 contrib)

** BUG:
2009.05.16  icmp 校验错误.
2009.05.20  加入对 TTL 选项的支持.
2009.05.21  加入对 timeout 的支持.
2009.05.27  加入对 ^C 的支持.
2009.07.03  修正 GCC 警告.
2009.09.03  修正了 API_INetPing 最后一次等待时间.
2009.09.24  使用 inet_ntoa_r() 替换 inet_ntoa().
2009.09.25  更新 gethostbyname() 传回地址复制的方法.
2009.11.11  使用了新的发送方法, 之前调用 connect().
2010.05.29  __inetPingSend() 使用 sendto 发送数据包. 以避过一个 lwip 的 bug(#29979).
2011.03.11  使用 getaddrinfo() 进行域名解析.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_PING_EN > 0)
#include "lwip/icmp.h"
#include "lwip/prot/icmp.h"
#include "lwip/raw.h"
#include "lwip/inet.h"
#include "lwip/inet_chksum.h"
#include "lwip/netdb.h"
#include "sys/socket.h"
/*********************************************************************************************************
** 函数名称: __inetPingPrepare
** 功能描述: 构造 ping 包
** 输　入  : icmphdrEcho   数据
**           iDataSize     数据大小
**           pusSeqRecv    需要判断的 seq
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __inetPingPrepare (struct  icmp_echo_hdr   *icmphdrEcho, 
                                INT   iDataSize, UINT16  *pusSeqRecv)
{
    static u16_t    usSeqNum = 1;
    REGISTER INT    i;
    
    SYS_ARCH_DECL_PROTECT(x);

    ICMPH_TYPE_SET(icmphdrEcho, ICMP_ECHO);
    ICMPH_CODE_SET(icmphdrEcho, 0);
    
    *pusSeqRecv = usSeqNum;
    
    icmphdrEcho->chksum = 0;
    icmphdrEcho->id     = 0xAFAF;                                       /*  ID                          */
    icmphdrEcho->seqno  = htons(usSeqNum);
    
    /*
     *  填充数据
     */
    for(i = 0; i < iDataSize; i++) {
        ((PCHAR)icmphdrEcho)[sizeof(struct icmp_echo_hdr) + i] = (CHAR)(i % 256);
    }
    
#if CHECKSUM_GEN_ICMP
    icmphdrEcho->chksum = inet_chksum(icmphdrEcho, (u16_t)(iDataSize + sizeof(struct icmp_echo_hdr)));
#endif                                                                  /*  CHECKSUM_GEN_ICMP           */
    
    SYS_ARCH_PROTECT(x);
    usSeqNum++;
    SYS_ARCH_UNPROTECT(x);
}
/*********************************************************************************************************
** 函数名称: __inetPingSend
** 功能描述: 发送 ping 包
** 输　入  : iSock         套接字
**           inaddr        目标 ip 地址.
**           iDataSize     数据大小
**           pusSeqRecv    需要判断的 seq
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __inetPingSend (INT  iSock, struct in_addr  inaddr, INT  iDataSize, UINT16  *pusSeqRecv)
{
    REGISTER size_t                stPingSize = sizeof(struct icmp_echo_hdr) + iDataSize;
             struct icmp_echo_hdr *icmphdrEcho;
             ssize_t               sstError;
             struct sockaddr_in    sockaddrin;
             
    icmphdrEcho = (struct icmp_echo_hdr *)__SHEAP_ALLOC(stPingSize);
    if (icmphdrEcho == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory\r\n");
        return  (PX_ERROR);
    }
    
    __inetPingPrepare(icmphdrEcho, iDataSize, pusSeqRecv);
    
    sockaddrin.sin_len    = sizeof(struct sockaddr_in);
    sockaddrin.sin_family = AF_INET;
    sockaddrin.sin_port   = 0;
    sockaddrin.sin_addr   = inaddr;
    
    sstError = sendto(iSock, icmphdrEcho, stPingSize, 0, 
                      (const struct sockaddr *)&sockaddrin, 
                      sizeof(struct sockaddr_in));
                         
    __SHEAP_FREE(icmphdrEcho);
    
    return ((sstError > 0) ? ERR_OK : ERR_VAL);
}
/*********************************************************************************************************
** 函数名称: __inetPingRecv
** 功能描述: 接收 ping 包
** 输　入  : iSock         socket
**           usSeqRecv     需要判断的 seq
**           piTTL         接收到的 TTL
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __inetPingRecv (INT  iSock, UINT16  usSeqRecv, INT  *piTTL)
{
             CHAR                   cBuffer[512];
             
             INT                    iCnt = 20;                          /*  默认最多接收的数据包数      */
    REGISTER ssize_t                sstLen;
             INT                    iAddLen = sizeof(struct sockaddr_in);
             struct sockaddr_in     sockaddrinFrom;
             
             struct ip_hdr         *iphdrFrom;
             struct icmp_echo_hdr  *icmphdrFrom;
            
    while ((sstLen = recvfrom(iSock, cBuffer, sizeof(cBuffer), 0, 
                              (struct sockaddr *)&sockaddrinFrom, (socklen_t *)&iAddLen)) > 0) {
        
        if (sstLen >= (sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr))) {
            
            iphdrFrom   = (struct ip_hdr *)cBuffer;
            icmphdrFrom = (struct icmp_echo_hdr *)(cBuffer + (IPH_HL(iphdrFrom) * 4));
            
            if ((icmphdrFrom->id == 0xAFAF) && (icmphdrFrom->seqno == htons(usSeqRecv))) {
                *piTTL = 0;
                *piTTL = (u8_t)IPH_TTL(iphdrFrom);
                return  (ERROR_NONE);
            }
        }
        
        iCnt--;                                                         /*  接收到错误的数据包太多      */
        if (iCnt < 0) {
            break;                                                      /*  退出                        */
        }
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __inetPingCleanup
** 功能描述: 回收 ping 资源.
** 输　入  : iSock         套接字
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __inetPingCleanup (INT  iSock)
{
    close(iSock);
}
/*********************************************************************************************************
** 函数名称: API_INetPing
** 功能描述: internet ping
** 输　入  : pinaddr       目标 ip 地址.
**           iTimes        次数
**           iDataSize     数据大小
**           iTimeout      超时时间
**           iTTL          IP TTL
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_INetPing (struct in_addr  *pinaddr, INT  iTimes, INT  iDataSize, INT  iTimeout, INT  iTTL)
{
             CHAR       cInetAddr[INET_ADDRSTRLEN];                     /*  IP 地址缓存                 */

    REGISTER INT        iSock;
             INT        i;
             UINT16     usSeqRecv = 0;
             ULONG      ulTime1;
             ULONG      ulTime2;
             
             INT        iSuc = 0;
             INT        iTTLRecv;                                       /*  接收的 TTL 值               */

    struct  sockaddr_in sockaddrinTo;
    
    REGISTER ULONG      ulTimeMax = 0;
    REGISTER ULONG      ulTimeMin = 0xFFFFFFFF;
    REGISTER ULONG      ulTimeAvr = 0;
    
    
    if ((iDataSize >= (64 * LW_CFG_KB_SIZE)) || (iDataSize < 0)) {      /*  0 - 64KB                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    if ((iTTL < 1) || (iTTL > 255)) {                                   /*  1 - 255                     */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    sockaddrinTo.sin_len    = sizeof(struct sockaddr_in);
    sockaddrinTo.sin_family = AF_INET;
    sockaddrinTo.sin_addr   = *pinaddr;
    
    iSock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);                    /*  必须原子运行到 push         */
    if (iSock < 0) {
        return  (PX_ERROR);
    }
    API_ThreadCleanupPush(__inetPingCleanup, (PVOID)iSock);             /*  加入清除函数                */
    
    setsockopt(iSock, SOL_SOCKET, SO_RCVTIMEO, &iTimeout, sizeof(INT));
    setsockopt(iSock, IPPROTO_IP, IP_TTL, &iTTL, sizeof(INT));
    
    connect(iSock, (struct sockaddr *)&sockaddrinTo, 
            sizeof(struct sockaddr_in));                                /*  设定目标                    */
    
    printf("Pinging %s\n\n", inet_ntoa_r(*pinaddr, cInetAddr, sizeof(cInetAddr)));
    
    for (i = 0; ;) {
        if (__inetPingSend(iSock, *pinaddr, iDataSize, &usSeqRecv) < 0) { 
                                                                        /*  发送 icmp 数据包            */
            fprintf(stderr, "error: %s.\n", lib_strerror(errno));
            
            i++;
            if (i >= iTimes) {
                break;
            }
            API_TimeSSleep(1);                                          /*  等待 1 S                    */
            continue;
        
        } else {
            i++;                                                        /*  发送次数 ++                 */
        }
        
        ulTime1 = API_TimeGet();
        if (__inetPingRecv(iSock, usSeqRecv, &iTTLRecv) < 0) {          /*  接收 icmp 数据包            */
            printf("Request time out.\n");                              /*  timeout                     */
            if (i >= iTimes) {
                break;                                                  /*  ping 结束                   */
            }
        } else {
            ulTime2 = API_TimeGet();
            ulTime2 = (ulTime2 >= ulTime1) ? (ulTime2 - ulTime1) 
                    : ((__ARCH_ULONG_MAX - ulTime1) + ulTime2 + 1);
            ulTime2 = ((ulTime2 * 1000) / LW_TICK_HZ);                  /*  转为毫秒数                  */
                    
            printf("Reply from %s: bytes=%d time=%ldms TTL=%d\n", 
                   inet_ntoa_r(*pinaddr, cInetAddr, sizeof(cInetAddr)),
                   iDataSize, ulTime2, iTTLRecv);
        
            iSuc++;
            
            ulTimeAvr += ulTime2;
            ulTimeMax = (ulTimeMax > ulTime2) ? ulTimeMax : ulTime2;
            ulTimeMin = (ulTimeMin < ulTime2) ? ulTimeMin : ulTime2;
            
            if (i >= iTimes) {
                break;                                                  /*  ping 结束                   */
            } else {
                API_TimeSSleep(1);                                      /*  等待 1 S                    */
            }
        }
    }
    API_ThreadCleanupPop(LW_TRUE);                                      /*  ping 清除                   */
    
    /*
     *  打印总结信息
     */
    printf("\nPing statistics for %s:\n", inet_ntoa_r(*pinaddr, cInetAddr, sizeof(cInetAddr)));
    printf("    Packets: Send = %d, Received = %d, Lost = %d(%d%% loss),\n", 
                  iTimes, iSuc, (iTimes - iSuc), (((iTimes - iSuc) * 100) / iTimes));
    
    if (iSuc == 0) {                                                    /*  没有一次成功                */
        return  (PX_ERROR);
    }
    
    ulTimeAvr = ulTimeAvr / iSuc;
    
    printf("Approximate round trip times in milli-seconds:\n");
    printf("    Minimum = %ldms, Maximum = %ldms, Average = %ldms\r\n\r\n",
                  ulTimeMin, ulTimeMax, ulTimeAvr);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellPing
** 功能描述: ping 命令
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellPing (INT  iArgC, PCHAR  *ppcArgV)
{
    struct addrinfo      hints;
    struct addrinfo     *phints = LW_NULL;

    struct in_addr  inaddr;
           CHAR     cInetAddr[INET_ADDRSTRLEN];                         /*  IP 地址缓存                 */

           INT      iTimes    = 4;
           INT      iDataSize = 32;
           INT      iTimeout  = 3000;
           INT      iTTL      = 255;

    if (iArgC <= 1) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    /*
     *  分析参数
     */
    if (iArgC >= 4) {
        REGISTER INT    i;
        
        for (i = 2; i < iArgC; i += 2) {
            if (lib_strcmp(ppcArgV[i], "-n") == 0) {
                sscanf(ppcArgV[i + 1], "%i", &iTimes);                  /*  获得次数                    */
            
            } else if (lib_strcmp(ppcArgV[i], "-l") == 0) {
                sscanf(ppcArgV[i + 1], "%i", &iDataSize);               /*  获得数据大小                */
                if ((iDataSize > (65000 - sizeof(struct icmp_echo_hdr))) ||
                    (iDataSize < 1)) {
                    fprintf(stderr, "data size error!\n");
                    return  (-ERROR_TSHELL_EPARAM);
                }
                
            } else if (lib_strcmp(ppcArgV[i], "-i") == 0) {             /*  获得 TTL 的值               */
                sscanf(ppcArgV[i + 1], "%i", &iTTL);
                if ((iTTL < 1) || (iTTL > 255)) {
                    fprintf(stderr, "TTL error!\n");
                    return  (-ERROR_TSHELL_EPARAM);
                }
            
            } else if (lib_strcmp(ppcArgV[i], "-w") == 0) {             /*  获得 timeout 的值           */
                sscanf(ppcArgV[i + 1], "%i", &iTimeout);
                if ((iTimeout < 1) || (iTimeout > 60000)) {
                    fprintf(stderr, "timeout error!\n");
                    return  (-ERROR_TSHELL_EPARAM);
                }
                
            } else {
                fprintf(stderr, "arguments error!\n");                   /*  参数错误                    */
                return  (-ERROR_TSHELL_EPARAM);
            }
        }
    }
    
    /*
     *  解析地址
     */
    if (!inet_aton(ppcArgV[1], &inaddr)) {
        printf("Execute a DNS query...\n");
        
        {
            INT  iOptionNoAbort;
            INT  iOption;
            
            ioctl(STD_IN, FIOGETOPTIONS, &iOption);
            iOptionNoAbort = (iOption & ~OPT_ABORT);
            ioctl(STD_IN, FIOSETOPTIONS, iOptionNoAbort);               /*  不允许 control-C 操作       */
            
            hints.ai_family = AF_INET;                                  /*  解析 IPv4 地址              */
            hints.ai_flags  = AI_CANONNAME;
            getaddrinfo(ppcArgV[1], LW_NULL, &hints, &phints);          /*  域名解析                    */
        
            ioctl(STD_IN, FIOSETOPTIONS, iOption);                      /*  回复原来状态                */
        }
        
        if (phints == LW_NULL) {
            printf("Pinging request could not find host %s ."
                   "Please check the name and try again.\n\n", ppcArgV[1]);
            return  (-ERROR_TSHELL_EPARAM);
        
        } else {
            if (phints->ai_addr->sa_family == AF_INET) {                /*  获得网络地址                */
                inaddr = ((struct sockaddr_in *)(phints->ai_addr))->sin_addr;
                freeaddrinfo(phints);
            
            } else {
                freeaddrinfo(phints);
                printf("Ping only support AF_INET domain!\n");
                return  (-EAFNOSUPPORT);
            }
            printf("Pinging %s [%s]\n\n", ppcArgV[1], inet_ntoa_r(inaddr, cInetAddr, sizeof(cInetAddr)));
        }
    }
    
    return  (API_INetPing(&inaddr, iTimes, iDataSize, iTimeout, iTTL));
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** 函数名称: API_INetPingInit
** 功能描述: 初始化 ping 工具
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_INetPingInit (VOID)
{
#if LW_CFG_SHELL_EN > 0
    API_TShellKeywordAdd("ping", __tshellPing);
    API_TShellFormatAdd("ping", " ip/hostname [-l datalen] [-n times] [-i ttl] [-w timeout]");
    API_TShellHelpAdd("ping",   "ping tool\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_PING_EN > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/

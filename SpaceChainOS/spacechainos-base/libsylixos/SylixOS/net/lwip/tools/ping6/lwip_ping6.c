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
** 文   件   名: lwip_ping6.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 06 月 07 日
**
** 描        述: lwip ipv6 ping 工具. 

** BUG:
2011.06.07  2011年6月8日是世界首个IPv6日, SylixOS在6月8日前开始支持IPv6.
2014.02.24  加入对指定 IPv6 网络接口的参数支持.
2018.01.08  对 linklocal 地址的 ping6 需要 SO_BINDTODEVICE.
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
#include "lwip/prot/icmp6.h"
#include "lwip/raw.h"
#include "lwip/tcpip.h"
#include "lwip/inet_chksum.h"
#include "lwip/netdb.h"
#include "net/if.h"
#include "sys/socket.h"
/*********************************************************************************************************
** 函数名称: __inetPing6Prepare
** 功能描述: 构造 ping 包
** 输　入  : icmp6hdrEcho  数据
**           pip6addrDest  目标 IP
**           iDataSize     数据大小
**           pusSeqRecv    需要判断的 seq
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __inetPing6Prepare (struct icmp6_echo_hdr   *icmp6hdrEcho, 
                                struct ip6_addr         *pip6addrDest,
                                INT                      iDataSize, 
                                UINT16                  *pusSeqRecv)
{
    static u16_t    usSeqNum = 1;
    REGISTER INT    i;
    
    SYS_ARCH_DECL_PROTECT(x);

    icmp6hdrEcho->type = ICMP6_TYPE_EREQ;
    icmp6hdrEcho->code = 0;
    
    *pusSeqRecv = usSeqNum;
    
    icmp6hdrEcho->chksum = 0;                                           /*  raw socket chksum auto      */
    icmp6hdrEcho->id     = 0xafaf;                                      /*  ID                          */
    icmp6hdrEcho->seqno  = htons(usSeqNum);
    
    /*
     *  填充数据
     */
    for (i = 0; i < iDataSize; i++) {
        ((PCHAR)icmp6hdrEcho)[sizeof(struct icmp6_echo_hdr) + i] = (CHAR)(i % 256);
    }
    
    SYS_ARCH_PROTECT(x);
    usSeqNum++;
    SYS_ARCH_UNPROTECT(x);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __inetPing6Send
** 功能描述: 发送 ping 包
** 输　入  : iSock         套接字
**           pin6addr      目标 ip 地址.
**           uiScopeId     scope id
**           iDataSize     数据大小
**           pusSeqRecv    需要判断的 seq
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __inetPing6Send (INT              iSock, 
                             struct in6_addr *pin6addr, 
                             UINT32           uiScopeId,
                             INT              iDataSize, 
                             UINT16          *pusSeqRecv)
{
    REGISTER size_t                 stPingSize = sizeof(struct icmp6_echo_hdr) + iDataSize;
             struct icmp6_echo_hdr *icmp6hdrEcho;
             ssize_t                sstError;
             struct sockaddr_in6    sockaddrin6;
             struct ip6_addr        ip6addrDest;
             
    icmp6hdrEcho = (struct icmp6_echo_hdr *)__SHEAP_ALLOC(stPingSize);
    if (icmp6hdrEcho == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory\r\n");
        return  (PX_ERROR);
    }
    
    ip6addrDest.addr[0] = pin6addr->un.u32_addr[0];
    ip6addrDest.addr[1] = pin6addr->un.u32_addr[1];
    ip6addrDest.addr[2] = pin6addr->un.u32_addr[2];
    ip6addrDest.addr[3] = pin6addr->un.u32_addr[3];
    
    if (__inetPing6Prepare(icmp6hdrEcho, &ip6addrDest, iDataSize, pusSeqRecv) < ERROR_NONE) {
        return  (ERR_VAL);
    }
    
    sockaddrin6.sin6_len      = sizeof(struct sockaddr_in);
    sockaddrin6.sin6_family   = AF_INET6;
    sockaddrin6.sin6_port     = 0;
    sockaddrin6.sin6_addr     = *pin6addr;
    sockaddrin6.sin6_scope_id = uiScopeId;
    
    sstError = sendto(iSock, icmp6hdrEcho, stPingSize, 0, 
                      (const struct sockaddr *)&sockaddrin6, 
                      sizeof(struct sockaddr_in6));
                         
    __SHEAP_FREE(icmp6hdrEcho);
    
    return ((sstError > 0) ? ERR_OK : ERR_VAL);
}
/*********************************************************************************************************
** 函数名称: __inetPingRecv
** 功能描述: 接收 ping 包
** 输　入  : iSock         socket
**           usSeqRecv     需要判断的 seq
**           piHL          接收的 hop limit
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __inetPing6Recv (INT  iSock, UINT16  usSeqRecv, INT  *piHL)
{
             CHAR                   cBuffer[512];
             
             INT                    iCnt = 20;                          /*  默认最多接收的数据包数      */
    REGISTER ssize_t                sstLen;
             INT                    iAddLen = sizeof(struct sockaddr_in6);
             struct sockaddr_in6    sockaddrin6From;
             
             struct ip6_hdr        *ip6hdrFrom;
             struct icmp6_echo_hdr *icmp6hdrFrom;
             
             u8_t                   nexth;
             INT                    hlen;
             INT                    totalhlen = 0;
             u8_t                  *pucData;

    *piHL = 0;

    while ((sstLen = recvfrom(iSock, cBuffer, sizeof(cBuffer), 0, 
                              (struct sockaddr *)&sockaddrin6From, (socklen_t *)&iAddLen)) > 0) {
        
        if (sstLen >= (sizeof(struct ip6_hdr) + sizeof(struct icmp6_echo_hdr))) {
            ip6hdrFrom = (struct ip6_hdr *)cBuffer;
            hlen       = IP6_HLEN;
            totalhlen  = IP6_HLEN;
            nexth      = IP6H_NEXTH(ip6hdrFrom);
            *piHL      = IP6H_HOPLIM(ip6hdrFrom);
            
            pucData    = (u8_t *)cBuffer;
            pucData   += IP6_HLEN;
            
            while (nexth != IP6_NEXTH_NONE) {
                switch (nexth) {
                
                case IP6_NEXTH_HOPBYHOP:
                case IP6_NEXTH_DESTOPTS:
                case IP6_NEXTH_ROUTING:
                    nexth      = *pucData;
                    hlen       = 8 * (1 + *(pucData + 1));
                    totalhlen += hlen;
                    pucData   += hlen;
                    break;
                    
                case IP6_NEXTH_FRAGMENT:
                    nexth      = *pucData;
                    hlen       = 8;
                    totalhlen += hlen;
                    pucData   += hlen;
                    break;
                    
                default:
                    goto    __hdrlen_cal_ok;
                    break;
                }
            }
            
__hdrlen_cal_ok:
            icmp6hdrFrom = (struct icmp6_echo_hdr *)(cBuffer + totalhlen);
            if ((icmp6hdrFrom->id == 0xAFAF) && (icmp6hdrFrom->seqno == htons(usSeqRecv))) {
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
** 函数名称: __inetPing6Cleanup
** 功能描述: 回收 ping 资源.
** 输　入  : iSock         套接字
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __inetPing6Cleanup (INT  iSock)
{
    close(iSock);
}
/*********************************************************************************************************
** 函数名称: API_INetPing6
** 功能描述: internet ipv6 ping
** 输　入  : pin6addr      目标 ip 地址.
**           iTimes        次数
**           iDataSize     数据大小
**           iTimeout      超时时间
**           pcNetif       网络接口名 (NULL 表示自动确定接口)
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_INetPing6 (struct in6_addr  *pin6addr, 
                    INT               iTimes, 
                    INT               iDataSize, 
                    INT               iTimeout, 
                    CPCHAR            pcNetif)
{
             CHAR       cInetAddr[INET6_ADDRSTRLEN];                    /*  IP 地址缓存                 */

    REGISTER INT        iSock;
             INT        i;
             UINT16     usSeqRecv = 0;
             ULONG      ulTime1;
             ULONG      ulTime2;
             
             INT        On   = 1;
             INT        iSuc = 0;
             INT        iHLRecv;

    struct  sockaddr_in6    sockaddrin6To;
    
    REGISTER ULONG      ulTimeMax = 0;
    REGISTER ULONG      ulTimeMin = 0xFFFFFFFF;
    REGISTER ULONG      ulTimeAvr = 0;
    
    
    if ((iDataSize >= (64 * LW_CFG_KB_SIZE)) || (iDataSize < 0)) {      /*  0 - 64KB                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    sockaddrin6To.sin6_len      = sizeof(struct sockaddr_in6);
    sockaddrin6To.sin6_family   = AF_INET6;
    sockaddrin6To.sin6_addr     = *pin6addr;
    sockaddrin6To.sin6_scope_id = 0;
    
    iSock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);                 /*  必须原子运行到 push         */
    if (iSock < 0) {
        return  (PX_ERROR);
    }
    API_ThreadCleanupPush(__inetPing6Cleanup, (PVOID)iSock);            /*  加入清除函数                */
    
    setsockopt(iSock, SOL_SOCKET, SO_RCVTIMEO, &iTimeout, sizeof(INT));
    setsockopt(iSock, IPPROTO_RAW, IPV6_CHECKSUM, &On, sizeof(INT));    /*  自动计算 checksum           */
    
    if (pcNetif) {
        struct ifreq ifreq;
        lib_strlcpy(ifreq.ifr_name, pcNetif, IFNAMSIZ);                 /*  绑定指定的设备              */
        setsockopt(iSock, SOL_SOCKET, SO_BINDTODEVICE, &ifreq, sizeof(ifreq));
        if (ioctl(iSock, SIOCGIFINDEX, &ifreq) < 0) {
            fprintf(stderr, "SIOCGIFINDEX command error: %s\n", lib_strerror(errno));
            close(iSock);
            return  (PX_ERROR);
        }
        sockaddrin6To.sin6_scope_id = ifreq.ifr_ifindex;                /*  设置 sin6_scope_id          */
    }
    
    connect(iSock, (struct sockaddr *)&sockaddrin6To, 
            sizeof(struct sockaddr_in6));                               /*  设定目标                    */
    
    printf("Pinging %s\n\n", inet6_ntoa_r(*pin6addr, cInetAddr, sizeof(cInetAddr)));
    
    for (i = 0; ;) {
        if (__inetPing6Send(iSock, pin6addr, sockaddrin6To.sin6_scope_id,
                            iDataSize, &usSeqRecv) < 0) {               /*  发送 icmp 数据包            */
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
        if (__inetPing6Recv(iSock, usSeqRecv, &iHLRecv) < 0) {          /*  接收 icmp 数据包            */
            printf("Request time out.\n");                              /*  timeout                     */
            if (i >= iTimes) {
                break;                                                  /*  ping 结束                   */
            }
        
        } else {
            ulTime2 = API_TimeGet();
            ulTime2 = (ulTime2 >= ulTime1) ? (ulTime2 - ulTime1) 
                    : ((__ARCH_ULONG_MAX - ulTime1) + ulTime2 + 1);
            ulTime2 = ((ulTime2 * 1000) / LW_TICK_HZ);                  /*  转为毫秒数                  */
                    
            printf("Reply from %s: bytes=%d time=%ldms hoplim=%d\n", 
                   inet6_ntoa_r(*pin6addr, cInetAddr, sizeof(cInetAddr)),
                   iDataSize, ulTime2, iHLRecv);
        
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
    printf("\nPing statistics for %s:\n", inet6_ntoa_r(*pin6addr, cInetAddr, sizeof(cInetAddr)));
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
** 函数名称: __tshellPing6
** 功能描述: ping6 命令
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellPing6 (INT  iArgC, PCHAR  *ppcArgV)
{
    struct addrinfo      hints;
    struct addrinfo     *phints = LW_NULL;

    struct in6_addr in6addr;
           CHAR     cInetAddr[INET6_ADDRSTRLEN];                        /*  IP 地址缓存                 */
           PCHAR    pcNetif = LW_NULL;

           INT      iTimes    = 4;
           INT      iDataSize = 32;
           INT      iTimeout  = 3000;

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
            
            } else if (lib_strcmp(ppcArgV[i], "-w") == 0) {             /*  获得 timeout 的值           */
                sscanf(ppcArgV[i + 1], "%i", &iTimeout);
                if ((iTimeout < 1) || (iTimeout > 60000)) {
                    fprintf(stderr, "timeout error!\n");
                    return  (-ERROR_TSHELL_EPARAM);
                }
                
            } else if (lib_strcmp(ppcArgV[i], "-I") == 0) {             /*  网络接口名                  */
                pcNetif = ppcArgV[i + 1];
                
            } else {
                fprintf(stderr, "arguments error!\n");                   /*  参数错误                    */
                return  (-ERROR_TSHELL_EPARAM);
            }
        }
    }
    
    /*
     *  解析地址
     */
    if (!inet6_aton(ppcArgV[1], &in6addr)) {
        printf("Execute a DNS query...\n");
        
        {
            INT  iOptionNoAbort;
            INT  iOption;
            
            ioctl(STD_IN, FIOGETOPTIONS, &iOption);
            iOptionNoAbort = (iOption & ~OPT_ABORT);
            ioctl(STD_IN, FIOSETOPTIONS, iOptionNoAbort);               /*  不允许 control-C 操作       */
            
            hints.ai_family = AF_INET6;                                 /*  解析 IPv6 地址              */
            hints.ai_flags  = AI_CANONNAME;
            getaddrinfo(ppcArgV[1], LW_NULL, &hints, &phints);          /*  域名解析                    */
        
            ioctl(STD_IN, FIOSETOPTIONS, iOption);                      /*  回复原来状态                */
        }
        
        if (phints == LW_NULL) {
            printf("Pinging request could not find host %s ."
                   "Please check the name and try again.\n\n", ppcArgV[1]);
            return  (-ERROR_TSHELL_EPARAM);
        
        } else {
            if (phints->ai_addr->sa_family == AF_INET6) {               /*  获得网络地址                */
                in6addr = ((struct sockaddr_in6 *)(phints->ai_addr))->sin6_addr;
                freeaddrinfo(phints);
            
            } else {
                freeaddrinfo(phints);
                printf("Ping6 only support AF_INET6 domain!\n");
                return  (-EAFNOSUPPORT);
            }
            printf("Pinging %s [%s]\n\n", ppcArgV[1], 
                   inet6_ntoa_r(in6addr, cInetAddr, sizeof(cInetAddr)));
        }
    }
    
    return  (API_INetPing6(&in6addr, iTimes, iDataSize, iTimeout, pcNetif));
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** 函数名称: API_INetPing6Init
** 功能描述: 初始化 IPv6 ping 工具
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_INetPing6Init (VOID)
{
#if LW_CFG_SHELL_EN > 0
    API_TShellKeywordAdd("ping6", __tshellPing6);
    API_TShellFormatAdd("ping6", " ip(v6)/hostname [-l datalen] [-n times] [-w timeout] [-I interface]");
    API_TShellHelpAdd("ping6",   "ipv6 ping tool\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_PING_EN > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: lwip_ftpd.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 11 月 21 日
**
** 描        述: ftp 服务器. (可以显示整个 SylixOS 目录)
** 注        意: ftp 服务器 1 分钟不访问, 将会自动断开, 以释放资源.
                 如果 ftp 服务器的个目录不是系统根目录, 有些版本 windows 访问此服务器有些功能有问题, 
                 例如: 重命名. 推荐将 ftp 根目录设置为 /
                 推荐使用 CuteFTP, FileZilla 等专业 FTP 客户端访问此服务器.
** BUG:
2009.11.29  当客户机使用 PORT 方式连接时, 本地 socket 一定要使用 SO_REUSEADDR 保留本地 20 端口供下次使用.
2009.11.30  PWD 命令执行正确时, 需要回复 257 作为正确编码.
2010.07.10  打印时间信息时, 使用 asctime_r() 可重入函数.
2011.02.13  修改注释.
2011.02.21  修正头文件引用, 符合 POSIX 标准.  
2011.03.04  ftp 只能传输 S_IFREG 文件.
            支持传输 proc 内的文件.
2011.04.07  服务器线程从 /etc/services 中获取 ftp 的服务端口, 如果没有, 默认使用 21 端口.
2012.01.06  修正 scanf 无法获取带有空格的路径错误.
2013.01.10  扩大 LIST 命令缓冲区, 以支持最大长度文件.
2013.01.19  支持 REST 与 APPE 功能. 支持断点续传.
            迅雷的 FTP 可能有问题, 不管是二进制模式还是ASCII模式, 他都要求命令回复结尾为 \r\n. 而其他 FTP
            客户端没有此要求.
2013.05.10  断点上传时, 非追加模式创建的文件必须带有 O_TRUNC 选项, 打开后自动清空.
2015.03.18  数据连接不能使用 SO_LINGER reset 模式关闭.
2016.12.07  默认使用二进制模式传输.
2017.01.09  加入网络登录黑名单功能.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_FTPD_EN > 0)
#include "ctype.h"
#include "shadow.h"
#include "netdb.h"
#include "arpa/inet.h"
#include "sys/socket.h"
#if LW_CFG_NET_LOGINBL_EN > 0
#include "sys/loginbl.h"
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
#include "lwip_ftp.h"
#include "lwip_ftplib.h"
/*********************************************************************************************************
  FTPD 兼容性选项 
*********************************************************************************************************/
#define __LWIP_FTPD_ASCII_REPLY     1                                   /*  迅雷需要此特性              */
/*********************************************************************************************************
  FTPD 配置
*********************************************************************************************************/
#if LW_CFG_NET_FTPD_LOG_EN > 0
#define __LWIP_FTPD_LOG(s)          printf s                            /*  打印 log 信息               */
#else
#define __LWIP_FTPD_LOG(s)
#endif                                                                  /*  LW_CFG_NET_FTPD_LOG_EN > 0  */
#define __LWIP_FTPD_BUFFER_SIZE     1024                                /*  FTP 命令接收缓冲区大小      */
#define __LWIP_FTPD_PATH_SIZE       MAX_FILENAME_LENGTH                 /*  文件目录缓冲大小            */
#define __LWIP_FTPD_MAX_USER_LEN    (LW_CFG_SHELL_MAX_KEYWORDLEN + 1)   /*  用户名最大长度              */
#define __LWIP_FTPD_MAX_RETRY       10                                  /*  数据链路创建重试次数        */
#define __LWIP_FTPD_SEND_SIZE       (32 * LW_CFG_KB_SIZE)               /*  文件发送缓冲大小            */
#define __LWIP_FTPD_RECV_SIZE       (12 * LW_CFG_KB_SIZE)               /*  文件接收缓冲                */
/*********************************************************************************************************
  FTPD 目录参数
*********************************************************************************************************/
#define __LWIP_FTPD_ITEM__STR(s)    #s
#define __LWIP_FTPD_ITEM_STR(s)     __LWIP_FTPD_ITEM__STR(s)
#define __LWIP_FTPD_PATH_MAX_STR    __LWIP_FTPD_ITEM_STR(LW_CFG_PATH_MAX)
/*********************************************************************************************************
  FTPD 会话状态
*********************************************************************************************************/
#define __FTPD_SESSION_STATUS_LOGIN     0x0001                          /*  用户已登录                  */
/*********************************************************************************************************
  FTPD 会话控制块 (rtems ftpd like!)
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE                FTPDS_lineManage;                       /*  会话管理链表                */
    INT                         FTPDS_iStatus;                          /*  当前会话状态                */
    CHAR                        FTPDS_cUser[__LWIP_FTPD_MAX_USER_LEN];  /*  登陆用户名                  */
    CHAR                        FTPDS_cPathBuf[MAX_FILENAME_LENGTH];    /*  路径缓冲                    */
    off_t                       FTPDS_oftOffset;                        /*  断点续传偏移量              */
    
    struct sockaddr_in          FTPDS_sockaddrinCtrl;                   /*  控制连接地址                */
    struct sockaddr_in          FTPDS_sockaddrinData;                   /*  数据连接地址                */
    struct sockaddr_in          FTPDS_sockaddrinDefault;                /*  默认链接地址                */
    
    BOOL                        FTPDS_bUseDefault;                      /*  是否使用默认地址进行数据连接*/
    struct in_addr              FTPDS_inaddrRemote;                     /*  远程地址, show 操作使用     */
    
    FILE                       *FTPDS_pfileCtrl;                        /*  控制连接缓冲IO              */
    INT                         FTPDS_iSockCtrl;                        /*  会话 socket                 */
    INT                         FTPDS_iSockPASV;                        /*  PASV socket                 */
    INT                         FTPDS_iSockData;                        /*  数据 socket                 */
    
    INT                         FTPDS_iTransMode;                       /*  仅支持 ASCII / BIN          */
#define __FTP_TRANSMODE_ASCII   1
#define __FTP_TRANSMODE_EBCDIC  2
#define __FTP_TRANSMODE_BIN     3
#define __FTP_TRANSMODE_LOCAL   4
    
    LW_OBJECT_HANDLE            FTPDS_ulThreadId;                       /*  服务线程句柄                */
    time_t                      FTPDS_timeStart;                        /*  连接时时间                  */
} __FTPD_SESSION;
typedef __FTPD_SESSION         *__PFTPD_SESSION;
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static PCHAR                    _G_pcFtpdRootPath         = LW_NULL;    /*  FTPd 服务器根目录           */
static LW_LIST_LINE_HEADER      _G_plineFtpdSessionHeader = LW_NULL;    /*  会话管理链表表头            */
static atomic_t                 _G_atomicFtpdLinks;                     /*  链接数量                    */
static INT                      _G_iFtpdDefaultTimeout    = 20 * 1000;  /*  默认数据链接超时时间        */
static INT                      _G_iFtpdIdleTimeout       = 60 * 1000;  /*  1 分钟不访问将会关闭控制连接*/
static INT                      _G_iFtpdNoLoginTimeout    = 20 * 1000;  /*  20 秒等待输入密码           */
static LW_OBJECT_HANDLE         _G_ulFtpdSessionLock;                   /*  会话链表锁                  */
#define __FTPD_SESSION_LOCK()   API_SemaphoreMPend(_G_ulFtpdSessionLock, LW_OPTION_WAIT_INFINITE)
#define __FTPD_SESSION_UNLOCK() API_SemaphoreMPost(_G_ulFtpdSessionLock)
/*********************************************************************************************************
  登录黑名单
*********************************************************************************************************/
#if LW_CFG_NET_LOGINBL_EN > 0
static UINT16                   _G_uiLoginFailPort  = 0;
static UINT                     _G_uiLoginFailBlSec = 120;              /*  黑名单超时, 默认 120 秒     */
static UINT                     _G_uiLoginFailBlRep = 3;                /*  黑名单探测默认为 3 次错误   */
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
/*********************************************************************************************************
  链接参数配置
*********************************************************************************************************/
#define __LW_FTPD_TCP_KEEPIDLE   60                                     /*  空闲时间, 单位秒            */
#define __LW_FTPD_TCP_KEEPINTVL  60                                     /*  两次探测间的时间间, 单位秒  */
#define __LW_FTPD_TCP_KEEPCNT    3                                      /*  探测 N 次失败认为是掉线     */
#define __LW_FTPD_TCP_BACKLOG    LW_CFG_NET_FTPD_MAX_LINKS              /*  listen backlog              */
/*********************************************************************************************************
  shell 函数声明
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
static INT  __tshellNetFtpdShow(INT  iArgC, PCHAR  *ppcArgV);
static INT  __tshellNetFtpdPath(INT  iArgC, PCHAR  *ppcArgV);
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  内部函数声明
*********************************************************************************************************/
static VOID  __ftpdCloseSessionData(__PFTPD_SESSION  pftpds);
static VOID  __ftpdCommandAnalyse(PCHAR  pcBuffer, PCHAR *ppcCmd, PCHAR *ppcOpts, PCHAR *ppcArgs);
/*********************************************************************************************************
** 函数名称: __ftpdSendReply
** 功能描述: ftp 发送回复控制字串
** 输　入  : pftpds        会话控制块
**           iCode         回应编码
**           cpcMessage    附加信息
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __ftpdSendReply (__PFTPD_SESSION  pftpds, INT  iCode, CPCHAR  cpcMessage)
{
#if __LWIP_FTPD_ASCII_REPLY > 0                                         /*  回复永远以 \r\n 作为结尾    */
    if (cpcMessage != LW_NULL) {
        fprintf(pftpds->FTPDS_pfileCtrl, "%d %s\r\n", iCode, cpcMessage);
    } else {
        fprintf(pftpds->FTPDS_pfileCtrl, "%d\r\n", iCode);
    }

#else                                                                   /*  根据模式选择结尾类型        */
    PCHAR  pcTail = (pftpds->FTPDS_iTransMode == __FTP_TRANSMODE_ASCII) ? "\r" : "";
    
    if (cpcMessage != LW_NULL) {
        fprintf(pftpds->FTPDS_pfileCtrl, "%d %s%s\n", iCode, cpcMessage, pcTail);
    } else {
        fprintf(pftpds->FTPDS_pfileCtrl, "%d%s\n", iCode, pcTail);
    }
#endif
    
    fflush(pftpds->FTPDS_pfileCtrl);                                    /*  将消息发送                  */
}
/*********************************************************************************************************
** 函数名称: __ftpdSendModeReply
** 功能描述: ftp 发送当前传输模式字串
** 输　入  : pftpds        会话控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __ftpdSendModeReply (__PFTPD_SESSION  pftpds)
{
    if (pftpds->FTPDS_iTransMode == __FTP_TRANSMODE_BIN) {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_FILE_OK, 
                        "Opening BINARY mode data connection.");
    } else {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_FILE_OK, 
                        "Opening ASCII mode data connection.");
    }
}
/*********************************************************************************************************
** 函数名称: __ftpdSendDirLine
** 功能描述: ftp 针对 LIST 命令, 生成一个单条的文件信息回复
** 输　入  : iSock        发送端 socket
**           bWide        是否发送文件属性信息 (LW_FALSE 表示仅发送文件名)
**           timeNow      当前时间
**           pcPath       目录
**           pcAdd        追加目录 (如果存在需要与目录 pcPath 链接生成完整目录)
**           pcFileName   需要输出的文件名
**           pcBuffer     临时缓冲
**           stSize       临时缓冲大小
** 输　出  : 发送的文件数量 1: 发送一个 0: 发送错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ftpdSendDirLine (INT      iSock, 
                               BOOL     bWide, 
                               time_t   timeNow, 
                               CPCHAR   pcPath, 
                               CPCHAR   pcAdd,
                               CPCHAR   pcFileName,
                               PCHAR    pcBuffer,
                               size_t   stSize)
{
    if (bWide) {                                                        /*  需要发送完整信息            */
        struct stat   statFile;
        size_t        stPathLen = lib_strlen(pcPath);
        size_t        stAddLen  = lib_strlen(pcAdd);
        
        if (stPathLen == 0) {
            pcBuffer[0] = PX_EOS;
        } else {
            lib_strcpy(pcBuffer, pcPath);
            if (stAddLen > 0 && pcBuffer[stPathLen - 1] != PX_DIVIDER) {
                pcBuffer[stPathLen++] = PX_DIVIDER;
                if (stPathLen >= stSize) {
                    return  (0);                                        /*  too long                    */
                }
                pcBuffer[stPathLen] = PX_EOS;
            }
        }
        if (stPathLen + stAddLen >= stSize) {
            return  (0);                                                /*  too long                    */
        }
        lib_strcpy(pcBuffer + stPathLen, pcAdd);                        /*  合成完整目录                */
        
        if (stat(pcBuffer, &statFile) == 0) {                           /*  查看目录信息                */
            size_t      stLen;
            struct tm   tmFile;
            time_t      timeFile = statFile.st_mtime;                   /*  文件时间 (UTC)              */
            CHAR        cMode0;
            CHAR        cMode3;
            CHAR        cMode6;
            
            enum { 
                SIZE = 80 
            };
            enum { 
                SIX_MONTHS = 365 * 24 * 60 * 60 / 2
            };
            
            char        cTimeBuf[SIZE];
            
            lib_localtime_r(&timeFile, &tmFile);                        /*  生成 tm 时间结构            */
            
            if ((timeNow > timeFile + SIX_MONTHS) || (timeFile > timeNow + SIX_MONTHS)) {
                lib_strftime(cTimeBuf, SIZE, "%b %d  %Y", &tmFile);
            } else {
                lib_strftime(cTimeBuf, SIZE, "%b %d %H:%M", &tmFile);
            }
            
            switch (statFile.st_mode & S_IFMT) {                        /*  显示文件类型                */
            
            case S_IFIFO:  cMode0 = 'f'; break;
            case S_IFCHR:  cMode0 = 'c'; break;
            case S_IFDIR:  cMode0 = 'd'; break;
            case S_IFBLK:  cMode0 = 'b'; break;
            case S_IFREG:  cMode0 = '-'; break;
            case S_IFLNK:  cMode0 = 'l'; break;
            case S_IFSOCK: cMode0 = 's'; break;
            default: cMode0 = '-'; break;
            }
            
            if (statFile.st_mode & S_IXUSR) {                           /*  SETUID 位信息               */
                if (statFile.st_mode & S_ISUID) {
                    cMode3 = 's';
                } else {
                    cMode3 = 'x';
                }
            } else {
                if (statFile.st_mode & S_ISUID) {
                    cMode3 = 'S';
                } else {
                    cMode3 = '-';
                }
            }
            
            if (statFile.st_mode & S_IXGRP) {                           /*  SETGID 位信息               */
                if (statFile.st_mode & S_ISGID) {
                    cMode6 = 's';
                } else {
                    cMode6 = 'x';
                }
            } else {
                if (statFile.st_mode & S_ISGID) {
                    cMode6 = 'S';
                } else {
                    cMode6 = '-';
                }
            }
            
            /*
             *  构建发送格式数据包
             */
            stLen = bnprintf(pcBuffer, stSize, 0, 
                             "%c%c%c%c%c%c%c%c%c%c  1 %5d %5d %11u %s %s\r\n",
                             cMode0,
                             (statFile.st_mode & S_IRUSR) ? ('r') : ('-'),
                             (statFile.st_mode & S_IWUSR) ? ('w') : ('-'),
                             cMode3,
                             (statFile.st_mode & S_IRGRP) ? ('r') : ('-'),
                             (statFile.st_mode & S_IWGRP) ? ('w') : ('-'),
                             cMode6,
                             (statFile.st_mode & S_IROTH) ? ('r') : ('-'),
                             (statFile.st_mode & S_IWOTH) ? ('w') : ('-'),
                             (statFile.st_mode & S_IXOTH) ? ('x') : ('-'),
                             (int)statFile.st_uid,
                             (int)statFile.st_gid,
                             (int)statFile.st_size,
                             cTimeBuf,
                             pcFileName);
            if (send(iSock, pcBuffer, stLen, 0) != (ssize_t)stLen) {    /*  发送详细信息                */
                return  (0);
            }
        }
    } else {
        INT stLen = bnprintf(pcBuffer, stSize, 0, "%s\r\n", pcFileName);
        
        if (send(iSock, pcBuffer, stLen, 0) != (ssize_t)stLen) {        /*  发送文件名                  */
            return  (0);
        }
    }
    
    return  (1);                                                        /*  成功发送的一个              */
}
/*********************************************************************************************************
** 函数名称: __ftpdDatasocket
** 功能描述: ftp 创建数据连接
** 输　入  : pftpds        会话控制块
** 输　出  : socket
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ftpdDatasocket (__PFTPD_SESSION  pftpds)
{
    REGISTER INT    iSock = pftpds->FTPDS_iSockPASV;

    if (iSock < 0) {                                                    /*  如果没有建立 pasv 数据连接  */
        iSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (iSock < 0) {
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DATALINK_FAILED,
                            "Can't create data socket.");
        } else {
            struct sockaddr_in  sockaddrinLocalData;
            INT                 i;
            INT                 iOn = 1;
            
            sockaddrinLocalData = pftpds->FTPDS_sockaddrinCtrl;
            sockaddrinLocalData.sin_port = htons(20);                   /*  本地数据端口号              */
        
            setsockopt(iSock, SOL_SOCKET, SO_REUSEADDR, &iOn, sizeof(iOn));
                                                                        /*  保留本地地址供下次使用      */
            /*
             *  服务器主动链接客户机, 本地端口必须绑定为 20
             */
            for (i = 0; i < __LWIP_FTPD_MAX_RETRY; i++) {
                errno = 0;
                if (bind(iSock, (struct sockaddr *)&sockaddrinLocalData, 
                         sizeof(sockaddrinLocalData)) >= 0) {
                    break;
                } else if (errno != EADDRINUSE) {                       /*  如果不是地址被占用          */
                    i = __LWIP_FTPD_MAX_RETRY;                          /*  直接错误退出                */
                } else {
                    sleep(1);                                           /*  等待一秒                    */
                }
            }
            
            if (i >= 10) {
                __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DATALINK_FAILED,
                                "Can't bind data socket.");
                close(iSock);
                iSock = -1;
            } else {
                /*
                 *  如果客户机已经通过 PORT 命令通知了自己的数据链路地址, 那么服务器将连接指定的地址
                 *  如果客户机没有指定数据链路地址, 那么服务器将链接默认数据地址.
                 */
                struct sockaddr_in  *psockaddrinDataDest = (pftpds->FTPDS_bUseDefault) 
                                                         ? (&pftpds->FTPDS_sockaddrinDefault)
                                                         : (&pftpds->FTPDS_sockaddrinData);
                if (connect(iSock, (struct sockaddr *)psockaddrinDataDest, 
                            sizeof(struct sockaddr_in)) < 0) {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DATALINK_FAILED, 
                                    "Can't connect data socket.");
                    close(iSock);
                    iSock = -1;
                }
            }
        }
    }
    
    pftpds->FTPDS_iSockData   = iSock;
    pftpds->FTPDS_bUseDefault = LW_TRUE;
    if (iSock >= 0) {
        setsockopt(iSock, SOL_SOCKET, SO_RCVTIMEO, 
                   (const void *)&_G_iFtpdDefaultTimeout, 
                   sizeof(INT));                                        /*  设置链接与接收操作          */
    }
    
    return  (iSock);
}
/*********************************************************************************************************
** 函数名称: __ftpdCmdList
** 功能描述: 命令: 文件列表
** 输　入  : pftpds        会话控制块
**           pcDir         设置目录
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ftpdCmdList (__PFTPD_SESSION  pftpds, CPCHAR  pcFileName, INT  iWide)
{
    INT             iSock;
    struct stat     statFile;
    CHAR            cBuffer[__LWIP_FTPD_PATH_SIZE + 64];                /*  加入属性长度                */
    CHAR            cPath[__LWIP_FTPD_PATH_SIZE];
    
    DIR            *pdir    = LW_NULL;
    struct dirent  *pdirent = LW_NULL;
    time_t          timeNow;
    INT             iSuc = 1;

    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_FILE_OK, 
                    "Opening ASCII mode data connection for LIST.");
                    
    iSock = __ftpdDatasocket(pftpds);                                   /*  获得数据连接 socket         */
    if (iSock < 0) {
        __LWIP_FTPD_LOG(("ftpd: error connecting to data socket. errno: %d\n", errno));
        return  (ERROR_NONE);
    }
    
    if (pcFileName[0] != PX_EOS) {                                      /*  指定目录                    */
        lib_strcpy(cPath, pcFileName);
    } else {
        lib_strcpy(cPath, ".");                                         /*  当前目录                    */
    }
    
    if (stat(cPath, &statFile) < 0) {                                   /*  查看文件属性                */
        snprintf(cBuffer, sizeof(cBuffer),
                 "%s: No such file or directory.\r\n", pcFileName);
        send(iSock, cBuffer, lib_strlen(cBuffer), 0);
    
    } else if (S_ISDIR(statFile.st_mode) && 
               (LW_NULL == (pdir = opendir(pcFileName)))) {             /*  是目录并且无法打开          */
        snprintf(cBuffer, sizeof(cBuffer),
                 "%s: Can not open directory.\r\n", pcFileName);
        send(iSock, cBuffer, lib_strlen(cBuffer), 0);
    
    } else {
        timeNow = lib_time(LW_NULL);
        if (!lib_strcmp(cPath, ".")) {
            cPath[0] = PX_EOS;                                          /*  如果为当前目录则不需要      */
        }
        if (!pdir && *pcFileName) {
            iSuc = iSuc && __ftpdSendDirLine(iSock, iWide, timeNow, cPath, 
                                             pcFileName, pcFileName, 
                                             cBuffer, sizeof(cBuffer));
        } else {
            do {
                pdirent = readdir(pdir);
                if (pdirent == LW_NULL) {
                    break;
                }
                iSuc = iSuc && __ftpdSendDirLine(iSock, iWide, timeNow, cPath, 
                                                 pdirent->d_name, pdirent->d_name, 
                                                 cBuffer, sizeof(cBuffer));
            } while (iSuc && pdirent);
        }
    }
    
    if (pdir) {
        closedir(pdir);
    }
    __ftpdCloseSessionData(pftpds);
    
    if (iSuc) {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DATACLOSE_NOERR, "Transfer complete.");
    } else {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DATALINK_ABORT, "Connection aborted.");
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ftpdCmdCwd
** 功能描述: 命令: 设置当前工作目录
** 输　入  : pftpds        会话控制块
**           pcDir         设置目录
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ftpdCmdCwd (__PFTPD_SESSION  pftpds, PCHAR  pcDir)
{
    INT     iError = ERROR_NONE;

    if (pcDir) {
        iError = access(pcDir, 0);                                      /*  首先探测是否可以访问        */
        if (iError == ERROR_NONE) {
            iError = cd(pcDir);                                         /*  链接目录                    */
        }
    }
    
    if (iError == ERROR_NONE) {                                         /*  目录链接                    */
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_FILE_OP_OK, "CWD command successful.");
    } else {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "CWD command failed.");
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ftpdCmdCdup
** 功能描述: 命令: CDUP
** 输　入  : pftpds        会话控制块
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ftpdCmdCdup (__PFTPD_SESSION  pftpds)
{
    getcwd(pftpds->FTPDS_cPathBuf, PATH_MAX);
    if (lib_strcmp(pftpds->FTPDS_cPathBuf, PX_STR_ROOT)) {              /*  不是根目录                  */
        cd("..");
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_FILE_OP_OK, "CDUP command successful.");
    } else {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "CDUP command failed.");
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ftpdCmdPort
** 功能描述: 命令: PORT
** 输　入  : pftpds        会话控制块
**           pcArg         命令参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ftpdCmdPort (__PFTPD_SESSION  pftpds, PCHAR  pcArg)
{
    enum { 
        NUM_FIELDS = 6 
    };
    UINT    uiTemp[NUM_FIELDS];
    INT     iNum;
    
    __ftpdCloseSessionData(pftpds);                                     /*  关闭数据连接                */
    
    iNum = sscanf(pcArg, "%u,%u,%u,%u,%u,%u", uiTemp + 0, 
                                              uiTemp + 1, 
                                              uiTemp + 2, 
                                              uiTemp + 3, 
                                              uiTemp + 4, 
                                              uiTemp + 5);              /*  注意: 这里必须整型宽        */
    if (NUM_FIELDS == iNum) {
        INT     i;
        UCHAR   ucTemp[NUM_FIELDS];

        for (i = 0; i < NUM_FIELDS; i++) {
            if (uiTemp[i] > 255) {
                break;
            }
            ucTemp[i] = (UCHAR)uiTemp[i];
        }
        if (i == NUM_FIELDS) {
            /*
             *  这里不允许 port 命令参数 IP 地址与原始客户端不相等.
             */
            u32_t   uiIp = (u32_t)((ucTemp[0] << 24)
                         |         (ucTemp[1] << 16)
                         |         (ucTemp[2] <<  8)
                         |         (ucTemp[3]));                        /*  构建 IP 地址                */
            
            u16_t   usPort = (u16_t)((ucTemp[4] << 8)
                           |         (ucTemp[5]));                      /*  构建端口号                  */
            
            uiIp   = ntohl(uiIp);
            usPort = ntohs(usPort);
            
            if (uiIp == pftpds->FTPDS_sockaddrinDefault.sin_addr.s_addr) {
                pftpds->FTPDS_sockaddrinData.sin_addr.s_addr = uiIp;
                pftpds->FTPDS_sockaddrinData.sin_port        = usPort;  /*  已经为网络序, 不用转换      */
                pftpds->FTPDS_sockaddrinData.sin_family      = AF_INET;
                lib_bzero(pftpds->FTPDS_sockaddrinData.sin_zero,
                          sizeof(pftpds->FTPDS_sockaddrinData.sin_zero));
               
                /*
                 *  之后的正向数据连接将使用客户机指定的地址.
                 */
                pftpds->FTPDS_bUseDefault = LW_FALSE;                   /*  不再使用默认地址            */
                __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_CMD_OK, 
                                "PORT command successful.");
                
                return  (ERROR_NONE);
            
            } else {
                __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DATALINK_FAILED, 
                                "Address doesn't match peer's IP.");
                return  (ERROR_NONE);
            }
        }
    }

    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_SYNTAX_ERR, "Syntax error.");
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ftpdCmdPasv
** 功能描述: 命令: PASV
** 输　入  : pftpds        会话控制块
**           pcArg         命令参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ftpdCmdPasv (__PFTPD_SESSION  pftpds)
{
    INT     iErrNo = 1;
    INT     iSock;

    __ftpdCloseSessionData(pftpds);                                     /*  关闭数据连接                */
    
    iSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);                  /*  创建数据链接端点            */
    if (iSock < 0) {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DATALINK_FAILED, 
                        "Can't open passive socket.");
        __LWIP_FTPD_LOG(("ftpd: error creating PASV socket. errno: %d\n", errno));
        return  (PX_ERROR);
        
    } else {
        struct sockaddr_in  sockaddrin;
        socklen_t           uiAddrLen = sizeof(sockaddrin);

        sockaddrin          = pftpds->FTPDS_sockaddrinCtrl;
        sockaddrin.sin_port = htons(0);                                 /*  需要自动分配端口            */
        
        if (bind(iSock, (struct sockaddr *)&sockaddrin, uiAddrLen) < 0) {
            __LWIP_FTPD_LOG(("ftpd: error binding PASV socket. errno: %d\n", errno));
        
        } else if (listen(iSock, 1) < 0) {
            __LWIP_FTPD_LOG(("ftpd: error listening on PASV socket. errno: %d\n", errno));
        
        } else {
            CHAR            cArgBuffer[65];
            ip4_addr_t      ipaddr;
            u16_t           usPort;
            
            setsockopt(iSock, SOL_SOCKET, SO_RCVTIMEO, 
                       (const void *)&_G_iFtpdDefaultTimeout, 
                       sizeof(INT));                                    /*  设置链接与接收操作          */
                       
            getsockname(iSock, 
                        (struct sockaddr *)&sockaddrin, 
                        &uiAddrLen);                                    /*  获得数据连接地址            */
            /*
             *  构建回应参数
             */
            ipaddr.addr = sockaddrin.sin_addr.s_addr;
            usPort      = ntohs(sockaddrin.sin_port);
            snprintf(cArgBuffer, sizeof(cArgBuffer), "Entering passive mode (%u,%u,%u,%u,%u,%u).",
                     ip4_addr1(&ipaddr), ip4_addr2(&ipaddr), ip4_addr3(&ipaddr), ip4_addr4(&ipaddr), 
                     (usPort >> 8), (usPort & 0xFF));
            
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_INTO_PASV, cArgBuffer);
            
            /*
             *  开始等待客户数据链接
             */
            pftpds->FTPDS_iSockPASV = accept(iSock, (struct sockaddr *)&sockaddrin, &uiAddrLen);
            if (pftpds->FTPDS_iSockPASV < 0) {
                __LWIP_FTPD_LOG(("ftpd: error accepting PASV connection. errno: %d\n", errno));
            } else {                                                    /*  链接成功                    */
                /*
                 *  关闭 listen socket 仅保留数据连接 socket
                 */
                close(iSock);
                iSock  = -1;
                iErrNo = 0;
                
                /*
                 *  设置保鲜参数.
                 */
                {
                    INT     iOne          = 1;
                    INT     iKeepIdle     = __LW_FTPD_TCP_KEEPIDLE;     /*  空闲时间                    */
                    INT     iKeepInterval = __LW_FTPD_TCP_KEEPINTVL;    /*  两次探测间的时间间隔        */
                    INT     iKeepCount    = __LW_FTPD_TCP_KEEPCNT;      /*  探测 N 次失败认为是掉线     */
                    
                    setsockopt(pftpds->FTPDS_iSockPASV, SOL_SOCKET,  SO_KEEPALIVE,  
                               (const void *)&iOne, sizeof(INT));
                    setsockopt(pftpds->FTPDS_iSockPASV, IPPROTO_TCP, TCP_KEEPIDLE,  
                               (const void *)&iKeepIdle, sizeof(INT));
                    setsockopt(pftpds->FTPDS_iSockPASV, IPPROTO_TCP, TCP_KEEPINTVL, 
                               (const void *)&iKeepInterval, sizeof(INT));
                    setsockopt(pftpds->FTPDS_iSockPASV, IPPROTO_TCP, TCP_KEEPCNT,
                               (const void *)&iKeepCount, sizeof(INT));
                }
            }
        }
    }
    
    if (iErrNo) {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DATALINK_FAILED, 
                        "Can't open passive connection.");
        close(iSock);                                                   /*  关闭临时链接                */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ftpdCmdPwd
** 功能描述: 命令: PWD
** 输　入  : pftpds        会话控制块
**           pcArg         命令参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ftpdCmdPwd (__PFTPD_SESSION  pftpds)
{
    CHAR    cBuffer[__LWIP_FTPD_PATH_SIZE + 40];
    PCHAR   pcCwd;
    
    errno = 0;
    cBuffer[0] = '"';
    pcCwd = getcwd(cBuffer + 1, __LWIP_FTPD_PATH_SIZE);                 /*  获得当前目录                */
    if (pcCwd) {
        lib_strlcat(cBuffer, "\" is the current directory.", __LWIP_FTPD_PATH_SIZE + 40);
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_MAKE_DIR_OK, cBuffer);
        
    } else {
        snprintf(cBuffer, __LWIP_FTPD_PATH_SIZE, "Error: %s\n", lib_strerror(errno));
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DONOT_RUN_REQ, cBuffer);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ftpdCmdMdtm
** 功能描述: 命令: MDTM 发送文件更新时间信息
** 输　入  : pftpds        会话控制块
**           pcFileName    文件名
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ftpdCmdMdtm (__PFTPD_SESSION  pftpds, CPCHAR  pcFileName)
{
    struct stat   statBuf;
    CHAR          cBuffer[__LWIP_FTPD_PATH_SIZE];

    if (0 > stat(pcFileName, &statBuf)) {
        snprintf(cBuffer, __LWIP_FTPD_PATH_SIZE, "%s: error: %s.", pcFileName, lib_strerror(errno));
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, cBuffer);
    } else {
        struct tm   tmFile;
        
        lib_localtime_r(&statBuf.st_mtime, &tmFile);                    /*  get localtime               */
        snprintf(cBuffer, __LWIP_FTPD_PATH_SIZE, "%04d%02d%02d%02d%02d%02d",
                 1900 + tmFile.tm_year,
                 tmFile.tm_mon+1, tmFile.tm_mday,
                 tmFile.tm_hour, tmFile.tm_min, tmFile.tm_sec);
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_FILE_STATUS, cBuffer);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ftpdCmdRnfr
** 功能描述: 命令: RNFR 文件重命名
** 输　入  : pftpds        会话控制块
**           pcFileName    需要重命名的文件名
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ftpdCmdRnfr (__PFTPD_SESSION  pftpds, CPCHAR  pcFileName)
{
    CHAR            cBuffer[__LWIP_FTPD_PATH_SIZE + 32];                /*  缓冲新的文件名              */
    struct timeval  tmvalTO = {3, 0};                                   /*  等待 3 S                    */
    
    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_NEED_INFO, "RNFR need RNTO info.");
    
    if (1 == waitread(pftpds->FTPDS_iSockCtrl, &tmvalTO)) {             /*  等待 RNTO 命令              */
        PCHAR   pcCmd;
        PCHAR   pcOpts;
        PCHAR   pcArgs;
        
        if (fgets(cBuffer, (__LWIP_FTPD_PATH_SIZE + 32), 
                  pftpds->FTPDS_pfileCtrl) != LW_NULL) {                /*  接收控制字符串              */
            
            __ftpdCommandAnalyse(cBuffer, &pcCmd, &pcOpts, &pcArgs);    /*  分析命令                    */
            if (!lib_strcmp(pcCmd, "RNTO")) {
                if (rename(pcFileName, pcArgs) == 0) {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_FILE_OP_OK, "RNTO complete.");
                } else {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_FILE_NAME_ERROR, "File name error.");
                }
                return  (ERROR_NONE);
            }
        }
    }
    
    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_SYNTAX_ERR, "RNTO not follow by RNFR.");
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ftpdCmdSize
** 功能描述: 命令: SIZE 获取文件大小
** 输　入  : pftpds        会话控制块
**           pcFileName    文件名
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ftpdCmdSize (__PFTPD_SESSION  pftpds, CPCHAR  pcFileName)
{
    CHAR          cBuffer[__LWIP_FTPD_PATH_SIZE];
    struct stat   statBuf;
    
    if (0 > stat(pcFileName, &statBuf)) {
        snprintf(cBuffer, __LWIP_FTPD_PATH_SIZE, "%s: error: %s.", pcFileName, lib_strerror(errno));
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, cBuffer);
    } else {
        snprintf(cBuffer, __LWIP_FTPD_PATH_SIZE, "%lld", statBuf.st_size);
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_FILE_STATUS, cBuffer);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ftpdCmdRest
** 功能描述: 命令: REST 设置文件获取偏移量
** 输　入  : pftpds        会话控制块
**           pcOffset      偏移量
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ftpdCmdRest (__PFTPD_SESSION  pftpds, CPCHAR  pcOffset)
{
    CHAR    cReplyBuffer[64];

    pftpds->FTPDS_oftOffset = lib_strtoll(pcOffset, LW_NULL, 10);
    
    snprintf(cReplyBuffer, 64,
             "Restarting at %lld. Send STORE or RETRIEVE to initiate transfer.", 
             pftpds->FTPDS_oftOffset);                                  /*  回复客户端, 文件指针已经设置*/
    
    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_NEED_INFO, cReplyBuffer);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ftpdCmdRter
** 功能描述: 命令: RETR 发送一个文件到客户机
** 输　入  : pftpds        会话控制块
**           pcFileName    文件名
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ftpdCmdRter (__PFTPD_SESSION  pftpds, CPCHAR  pcFileName)
{
    INT         iSock;
    INT         iFd;
    struct stat statBuf;
    CHAR        cLocalBuffer[__LWIP_FTPD_BUFFER_SIZE];
    PCHAR       pcTransBuffer = LW_NULL;
    
    size_t      stBufferSize;
    INT         iResult = 0;
    off_t       oftOffset = pftpds->FTPDS_oftOffset;
    off_t       oftNiceSize;
    
    pftpds->FTPDS_oftOffset = 0;                                        /*  复位偏移量                  */
    
    iFd = open(pcFileName, O_RDONLY);
    if (iFd < 0) {
        if (errno == EACCES) {
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "Insufficient permissions.");
        } else {
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "Error opening file.");
        }
        return  (ERROR_NONE);
    }
    if (0 > fstat(iFd, &statBuf)) {
        close(iFd);
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "Error stating file.");
        return  (ERROR_NONE);
    }
    if (!S_ISREG(statBuf.st_mode)) {
        close(iFd);
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "not a REG file.");
        return  (ERROR_NONE);
    }
    
    if (oftOffset) {
        if (oftOffset > statBuf.st_size) {                              /*  偏移量超过文件最大大小      */
            close(iFd);
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "Offset overflow.");
            return  (ERROR_NONE);
        }
        if (lseek(iFd, oftOffset, SEEK_SET) == (off_t)PX_ERROR) {
            close(iFd);
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "Offset invalidate.");
            return  (ERROR_NONE);
        }
    }
    
    oftNiceSize = statBuf.st_size - oftOffset;
    
    if (oftNiceSize < __LWIP_FTPD_BUFFER_SIZE) {
        pcTransBuffer = cLocalBuffer;                                   /*  使用局部缓冲传输            */
        stBufferSize  = __LWIP_FTPD_BUFFER_SIZE;
    } else {
        stBufferSize  = (size_t)__MIN(__LWIP_FTPD_SEND_SIZE, oftNiceSize);
        pcTransBuffer = (PCHAR)__SHEAP_ALLOC(stBufferSize);             /*  使用最优缓冲区大小          */
        if (pcTransBuffer == LW_NULL) {
            pcTransBuffer =  cLocalBuffer;
            stBufferSize  = __LWIP_FTPD_BUFFER_SIZE;                    /*  开辟失败, 使用本地缓冲      */
        }
    }
    
    __ftpdSendModeReply(pftpds);                                        /*  通知对方传输方式            */
    
    iSock = __ftpdDatasocket(pftpds);                                   /*  获得数据连接 socket         */
    if (iSock >= 0) {
        INT     iN = -1;
        
        if (pftpds->FTPDS_iTransMode == __FTP_TRANSMODE_BIN) {          /*  二进制类型传输              */
            while ((iN = (INT)read(iFd, (PVOID)pcTransBuffer, stBufferSize)) > 0) {
                if (send(iSock, pcTransBuffer, iN, 0) != iN) {
                    break;                                              /*  发送失败                    */
                }
            }
            
        } else if (pftpds->FTPDS_iTransMode == __FTP_TRANSMODE_ASCII) { /*  文本传输                    */
            INT     iReset = 0;
            
            while (iReset == 0 && (iN = (INT)read(iFd, (PVOID)pcTransBuffer, stBufferSize)) > 0) {
                PCHAR   e = pcTransBuffer;
                PCHAR   b;
                INT     i;
                
                iReset = iN;
                do {
                    CHAR  cLf = PX_EOS;

                    b = e;
                    for (i = 0; i < iReset; i++, e++) {
                        if (*e == '\n') {                               /*  行结束变换                  */
                            cLf = '\n';
                            break;
                        }
                    }
                    if (send(iSock, b, i, 0) != i) {                    /*  发送单行数据                */
                        break;
                    }
                    if (cLf == '\n') {
                        if (send(iSock, "\r\n", 2, 0) != 2) {           /*  发送行尾                    */
                            break;
                        }
                        e++;
                        i++;
                    }
                } while ((iReset -= i) > 0);
            }
        }
        
        if (0 == iN) {                                                  /*  读取数据完毕                */
            if (0 == close(iFd)) {
                iFd     = -1;
                iResult = 1;
            }
        }
    }
    
    if (iFd >= 0) {
        close(iFd);
    }
    
    if (pcTransBuffer && (pcTransBuffer != cLocalBuffer)) {
        __SHEAP_FREE(pcTransBuffer);
    }
    __ftpdCloseSessionData(pftpds);
    
    if (0 == iResult) {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_ABORT, "File read error.");
    } else {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DATACLOSE_NOERR, "Transfer complete.");
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ftpdCmdStor
** 功能描述: 命令: STOR 服务器接收一个文件
** 输　入  : pftpds        会话控制块
**           pcFileName    文件名
**           iAppe         是否是 APPE 命令
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ftpdCmdStor (__PFTPD_SESSION  pftpds, CPCHAR  pcFileName, INT  iAppe)
{
    INT         iSock;
    INT         iFd;
    struct stat statGet;
    CHAR        cLocalBuffer[__LWIP_FTPD_BUFFER_SIZE];
    PCHAR       pcTransBuffer = LW_NULL;
    INT         iBufferSize;
    INT         iResult = 0;
    
    INT         iN = 0;
    
    pftpds->FTPDS_oftOffset = 0;                                        /*  复位偏移量                  */
    
    __ftpdSendModeReply(pftpds);                                        /*  通知对方传输方式            */
    
    iSock = __ftpdDatasocket(pftpds);                                   /*  获得数据连接 socket         */
    if (iSock < 0) {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "Error creating data link.");
        return  (ERROR_NONE);
    }
    
    if (iAppe) {                                                        /*  追加方式                    */
        iFd = open(pcFileName, 
                   (O_WRONLY | O_CREAT | O_APPEND), 
                   DEFAULT_FILE_PERM);                                  /*  没有文件也新建              */
    } else {
        iFd = open(pcFileName, 
                   (O_WRONLY | O_CREAT | O_TRUNC), 
                   DEFAULT_FILE_PERM);                                  /*  打开或创建文件(清空)        */
    }
    if (iFd < 0) {
        if (errno == EACCES) {
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "Insufficient permissions.");
        } else {
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "Error creating file.");
        }
        __ftpdCloseSessionData(pftpds);
        return  (ERROR_NONE);
    }
    if ((fstat(iFd, &statGet) < 0) || S_ISDIR(statGet.st_mode)) {       /*  不允许与 DIR 同名           */
        close(iFd);                                                     /*  关闭文件                    */
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, "Error creating file.");
        __ftpdCloseSessionData(pftpds);
        return  (ERROR_NONE);
    }
    
    pcTransBuffer = (PCHAR)__SHEAP_ALLOC(__LWIP_FTPD_RECV_SIZE);        /*  开辟接收缓冲                */
    if (pcTransBuffer) {
        iBufferSize = __LWIP_FTPD_RECV_SIZE;
    } else {
        pcTransBuffer = cLocalBuffer;
        iBufferSize = __LWIP_FTPD_BUFFER_SIZE;
    }
    
    if (pftpds->FTPDS_iTransMode == __FTP_TRANSMODE_BIN) {              /*  二进制类型传输              */
        while ((iN = (INT)read(iSock, (PVOID)pcTransBuffer, (ssize_t)iBufferSize)) > 0) {
            if (write(iFd, pcTransBuffer, iN) != iN) {
                break;                                                  /*  接收失败                    */
            }
        }
    
    } else if (pftpds->FTPDS_iTransMode == __FTP_TRANSMODE_ASCII) {     /*  文本传输                    */
        while ((iN = (INT)read(iSock, (PVOID)pcTransBuffer, (ssize_t)iBufferSize)) > 0) {
            PCHAR   e = pcTransBuffer;
            PCHAR   b;
            INT     i;
            INT     iCounter = 0;
            
            do {
                INT     iCr = 0;
                b = e;
                for (i = 0; i < (iN - iCounter); i++, e++) {
                    if (*e == '\r') {
                        iCr = 1;
                        break;
                    }
                }
                if (i > 0) {
                    if (write(iFd, b, i) != i) {                        /*  保存单行数据                */
                        goto    __recv_over;                            /*  接收错误                    */
                    }
                }
                iCounter += (i + iCr);                                  /*  已经保存的数据              */
                if (iCr) {
                    e++;                                                /*  忽略 \r                     */
                }
            } while (iCounter < iN);
        }
    } else {
        iN = 1;                                                         /*  错误标志                    */
    }
    
__recv_over:
    close(iFd);                                                         /*  关闭文件                    */

    if (0 >= iN) {                                                      /*  发送完毕                    */
        iResult = 1;
    } else if (!iAppe) {                                                /*  非追加模式                  */
        unlink(pcFileName);                                             /*  传输失败删除文件            */
    }
    
    if (pcTransBuffer && (pcTransBuffer != cLocalBuffer)) {
        __SHEAP_FREE(pcTransBuffer);                                    /*  释放缓冲区                  */
    }
    
    __ftpdCloseSessionData(pftpds);                                     /*  关闭数据连接                */
    
    if (0 == iResult) {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DREQ_ABORT, "File write error.");
    } else {
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_DATACLOSE_NOERR, "Transfer complete.");
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ftpdCommandExec
** 功能描述: 执行命令
** 输　入  : pftpds        会话控制块
**           pcCmd         命令
**           pcArg         参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __ftpdCommandExec (__PFTPD_SESSION  pftpds, PCHAR  pcCmd, PCHAR  pcArg)
{
    CHAR    cFileName[MAX_FILENAME_LENGTH];                             /*  文件名                      */

#if LW_CFG_NET_LOGINBL_EN > 0
    struct sockaddr_in  addr;
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */

    if (!lib_strcmp("USER", pcCmd)) {                                   /*  用户名                      */
        lib_strlcpy(pftpds->FTPDS_cUser, pcArg, __LWIP_FTPD_MAX_USER_LEN);
        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_PW_REQ, 
                        "Password required.");                          /*  等待密码输入                */
        pftpds->FTPDS_iStatus &= ~__FTPD_SESSION_STATUS_LOGIN;
        
    } else if (!lib_strcmp("PASS", pcCmd)) {                            /*  密码输入                    */
        if (userlogin(pftpds->FTPDS_cUser, pcArg, 1) < 0) {
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_LOGIN_FAILED,
                            "Login failed.");                           /*  用户认证失败                */
            pftpds->FTPDS_iStatus &= ~__FTPD_SESSION_STATUS_LOGIN;
        
#if LW_CFG_NET_LOGINBL_EN > 0
            addr.sin_len    = sizeof(struct sockaddr_in);
            addr.sin_family = AF_INET;
            addr.sin_port   = _G_uiLoginFailPort;
            addr.sin_addr   = pftpds->FTPDS_inaddrRemote;
            API_LoginBlAdd((struct sockaddr *)&addr, _G_uiLoginFailBlRep, _G_uiLoginFailBlSec);
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
        } else {
            setsockopt(pftpds->FTPDS_iSockCtrl, SOL_SOCKET, SO_RCVTIMEO, 
                       (const void *)&_G_iFtpdIdleTimeout, sizeof(INT));
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_USER_LOGIN,
                            "User logged in.");
            pftpds->FTPDS_iStatus |= __FTPD_SESSION_STATUS_LOGIN;
        }
        
    } else if (!lib_strcmp("HELP", pcCmd)) {                            /*  帮助信息                    */
        /*
         *  TODO: 目前暂不打印帮助清单.
         */
        goto    __command_not_understood;                               /*  暂时不识别此命令            */
         
    } else {
        /*
         *  下面的操作需要登陆
         */
        if ((pftpds->FTPDS_iStatus & __FTPD_SESSION_STATUS_LOGIN) == 0) {
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_LOGIN_FAILED,
                            "USER and PASS required.");                 /*  需要用户登录                */
        } else {
            if (!lib_strcmp("CWD", pcCmd)) {                            /*  设置工作目录                */
                if ((pcArg != LW_NULL) && (pcArg[0] != PX_EOS)) {
                    lib_strlcpy(cFileName, pcArg, MAX_FILENAME_LENGTH);
                } else {
                    lib_strcpy(cFileName, ".");                         /*  当前文件                    */
                }
                return  (__ftpdCmdCwd(pftpds, cFileName));
            
            } else if (!lib_strcmp("CDUP", pcCmd)) {                    /*  返回上级目录                */
                return  (__ftpdCmdCdup(pftpds));
            
            } else if (!lib_strcmp("PWD", pcCmd)) {                     /*  当前目录                    */
                return  (__ftpdCmdPwd(pftpds));
                
            } else if (!lib_strcmp("ALLO", pcCmd) ||
                       !lib_strcmp("ACCT", pcCmd)) {                    /*  用户类型识别                */
                __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_CMD_UNSUPPORT, 
                                "Allocate and Account not required.");
                return  (ERROR_NONE);
                
            } else if (!lib_strcmp("PORT", pcCmd)) {                    /*  数据连接信息                */
                return  (__ftpdCmdPort(pftpds, pcArg));
                
            } else if (!lib_strcmp("PASV", pcCmd)) {                    /*  PASV 数据连接信息           */
                return  (__ftpdCmdPasv(pftpds));
                
            } else if (!lib_strcmp("TYPE", pcCmd)) {                    /*  设定传输模式                */
                if (pcArg[0] == 'I') {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_CMD_OK, 
                                    "Type set to I.");                  /*  如果之前为 A 模式, 则需要 \r*/
                    pftpds->FTPDS_iTransMode = __FTP_TRANSMODE_BIN;
                                    
                } else if (pcArg[0] == 'A') {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_CMD_OK, 
                                    "Type set to A.");
                    pftpds->FTPDS_iTransMode = __FTP_TRANSMODE_ASCII;
                    
                } else {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_UNSUP_WITH_ARG, 
                                    "Type not implemented. Set to I.");
                    pftpds->FTPDS_iTransMode = __FTP_TRANSMODE_BIN;
                }
                return  (ERROR_NONE);
                
            } else if (!lib_strcmp("LIST", pcCmd)) {                    /*  打印当前目录列表            */
                if ((pcArg != LW_NULL) && (pcArg[0] != PX_EOS)) {
                    lib_strlcpy(cFileName, pcArg, MAX_FILENAME_LENGTH);
                } else {
                    lib_strcpy(cFileName, ".");                         /*  当前文件                    */
                }
                return  (__ftpdCmdList(pftpds, cFileName, 1));
                
            } else if (!lib_strcmp("NLST", pcCmd)) {                    /*  打印当前目录列表            */
                if ((pcArg != LW_NULL) && (pcArg[0] != PX_EOS)) {
                    lib_strlcpy(cFileName, pcArg, MAX_FILENAME_LENGTH);
                } else {
                    lib_strcpy(cFileName, ".");                         /*  当前文件                    */
                }
                return  (__ftpdCmdList(pftpds, cFileName, 0));          /*  简洁打印                    */
                
            } else if (!lib_strcmp("RNFR", pcCmd)) {                    /*  文件该改名                  */
                return  (__ftpdCmdRnfr(pftpds, pcArg));
                
            } else if (!lib_strcmp("REST", pcCmd)) {                    /*  文件偏移量                  */
                return  (__ftpdCmdRest(pftpds, pcArg));
                
            } else if (!lib_strcmp("RETR", pcCmd)) {                    /*  文件发送                    */
                return  (__ftpdCmdRter(pftpds, pcArg));
            
            } else if (!lib_strcmp("STOR", pcCmd)) {                    /*  文件接收                    */
                return  (__ftpdCmdStor(pftpds, pcArg, 0));
                
            } else if (!lib_strcmp("APPE", pcCmd)) {                    /*  文件追加接收                */
                return  (__ftpdCmdStor(pftpds, pcArg, 1));
            
            } else if (!lib_strcmp("SYST", pcCmd)) {                    /*  询问系统类型                */
                __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_NAME_SYS_TYPE, 
                                __FTPD_SYSTYPE);
                return  (ERROR_NONE);
            
            } else if (!lib_strcmp("MKD", pcCmd)) {                     /*  创建一个目录                */
                if (mkdir(pcArg, DEFAULT_DIR_PERM) < 0) {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, 
                                    "MKD failed.");
                } else {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_MAKE_DIR_OK, 
                                    "MKD successful.");
                }
                return  (ERROR_NONE);
                
            } else if (!lib_strcmp("RMD", pcCmd)) {                     /*  删除一个目录                */
                if (rmdir(pcArg) < 0) {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, 
                                    "RMD failed.");
                } else {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_MAKE_DIR_OK, 
                                    "RMD successful.");
                }
                return  (ERROR_NONE);
            } else if (!lib_strcmp("DELE", pcCmd)) {                    /*  删除一个文件                */
                if (unlink(pcArg) < 0) {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, 
                                    "DELE failed.");
                } else {
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_MAKE_DIR_OK, 
                                    "DELE successful.");
                }
                return  (ERROR_NONE);
                
            } else if (!lib_strcmp("MDTM", pcCmd)) {                    /*  文件时间更新                */
                return  (__ftpdCmdMdtm(pftpds, pcArg));
                
            } else if (!lib_strcmp("SIZE", pcCmd)) {                    /*  获取文件大小                */
                return  (__ftpdCmdSize(pftpds, pcArg));
                
            } else if (!lib_strcmp("SITE", pcCmd)) {                    /*  站点参数                    */
                PCHAR   pcOpts;
                
                __ftpdCommandAnalyse(pcArg, &pcCmd, &pcOpts, &pcArg);
                if (!lib_strcmp("CHMOD", pcCmd)) {                      /*  修改文件 mode               */
                    INT     iMask = DEFAULT_FILE_PERM;
                    if ((2 == sscanf(pcArg, "%o %" __LWIP_FTPD_PATH_MAX_STR "[^\n]", 
                                     &iMask, cFileName)) &&
                        (0 == chmod(cFileName, (mode_t)iMask))) {
                        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_MAKE_DIR_OK, 
                                        "CHMOD successful.");
                    } else {
                        __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_REQ_FAILED, 
                                        "CHMOD failed.");
                    }
                    return  (ERROR_NONE);
                
                } else if (!lib_strcmp("SYNC", pcCmd)) {                /*  回写磁盘                    */
                    sync();
                    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_CMD_OK, 
                                    "SYNC successful.");
                    return  (ERROR_NONE);
                }
                goto    __command_not_understood;
                
            } else if (!lib_strcmp("NOOP", pcCmd)) {                    /*  什么都不做                  */
                __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_CMD_OK, 
                                "NOOP -- did nothing as requested.");
                return  (ERROR_NONE);
            }

__command_not_understood:
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_CMD_ERROR, 
                            "Command not understood.");                 /*  暂不可被识别的命令          */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __ftpdCloseSessionCtrl
** 功能描述: 关闭一个会话控制块控制连接
** 输　入  : pftpds        会话控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __ftpdCloseSessionCtrl (__PFTPD_SESSION  pftpds)
{
    /*
     *  fclose() 将自动调用 close() 关闭连接.
     */
    if (pftpds->FTPDS_pfileCtrl) {
        fclose(pftpds->FTPDS_pfileCtrl);                                /*  关闭控制端文件              */
    } else {
        close(pftpds->FTPDS_iSockCtrl);                                 /*  关闭控制 socket             */
    }
    
    pftpds->FTPDS_pfileCtrl = LW_NULL;
    pftpds->FTPDS_iSockCtrl = -1;
}
/*********************************************************************************************************
** 函数名称: __ftpdCloseSessionData
** 功能描述: 关闭一个会话控制块数据连接
** 输　入  : pftpds        会话控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __ftpdCloseSessionData (__PFTPD_SESSION  pftpds)
{
    /*
     *  只可能存在一种数据连接模式.
     */
    if (pftpds->FTPDS_iSockData > 0) {
        close(pftpds->FTPDS_iSockData);                                 /*  关闭数据连接                */
    
    } else if (pftpds->FTPDS_iSockPASV > 0) {
        close(pftpds->FTPDS_iSockPASV);                                 /*  关闭 PASV 数据连接          */
    }
    
    pftpds->FTPDS_iSockData   = -1;
    pftpds->FTPDS_iSockPASV   = -1;
    pftpds->FTPDS_bUseDefault = LW_TRUE;
}
/*********************************************************************************************************
** 函数名称: __ftpdSkipOpt
** 功能描述: 忽略选项字段
** 输　入  : pcTemp        选项指针地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __ftpdSkipOpt (PCHAR  *ppcTemp)
{
    PCHAR     pcBuf  = *ppcTemp;
    PCHAR     pcLast = LW_NULL;
  
    for (;;) {
        while (isspace(*pcBuf)) {                                       /*  忽略                        */
            ++pcBuf;
        }
        
        if (*pcBuf == '-') {
            if (*++pcBuf == '-') {                                      /* `--' should terminate options*/
                if (isspace(*++pcBuf)) {
                    pcLast = pcBuf;
                    do {
                        ++pcBuf;
                    } while (isspace(*pcBuf));
                    break;
                }
            }
            while (*pcBuf && !isspace(*pcBuf)) {
                ++pcBuf;
            }
            pcLast = pcBuf;
        } else {
            break;
        }
    }
    if (pcLast) {
        *pcLast = PX_EOS;
    }
    *ppcTemp = pcBuf;
}
/*********************************************************************************************************
** 函数名称: __ftpdCommandAnalyse
** 功能描述: 分析命令
** 输　入  : pcBuffer        命令字串
**           ppcCmd          命令指针
**           ppcOpts         选项指针
**           ppcArgs         参数指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __ftpdCommandAnalyse (PCHAR  pcBuffer, PCHAR *ppcCmd, PCHAR *ppcOpts, PCHAR *ppcArgs)
{
    PCHAR     pcEoc;
    PCHAR     pcTemp = pcBuffer;
  
    while (isspace(*pcTemp)) {                                          /*  忽略前方无用字符            */
        ++pcTemp;
    }
    
    *ppcCmd = pcTemp;                                                   /*  记录命令其实地址            */
    while (*pcTemp && !isspace(*pcTemp)) {
        *pcTemp = (CHAR)toupper(*pcTemp);                               /*  转换为大写字符              */
        ++pcTemp;
    }
    
    pcEoc = pcTemp;
    if (*pcTemp) {
        *pcTemp++ = PX_EOS;                                             /*  命令结束                    */
    }
    
    while (isspace(*pcTemp)) {                                          /*  忽略前方无用字符            */
        ++pcTemp;
    }
    
    *ppcOpts = pcTemp;                                                  /*  记录选项                    */
    __ftpdSkipOpt(&pcTemp);
  
    *ppcArgs = pcTemp;
    if (*ppcOpts == pcTemp) {
        *ppcOpts = pcEoc;
    }
    
    while (*pcTemp && *pcTemp != '\r' && *pcTemp != '\n') {             /*  命令末尾                    */
        ++pcTemp;
    }
    
    if (*pcTemp) {
        *pcTemp++ = PX_EOS;                                             /*  去掉换行符                  */
    }
}
/*********************************************************************************************************
** 函数名称: __inetFtpSession
** 功能描述: ftp 会话线程
** 输　入  : pftpds        会话控制块
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __inetFtpdSession (__PFTPD_SESSION  pftpds)
{
#if LW_CFG_NET_FTPD_LOG_EN > 0
    CHAR    cAddr[INET_ADDRSTRLEN];
#endif                                                                  /*  LW_CFG_NET_FTPD_LOG_EN > 0  */
    CHAR    cBuffer[__LWIP_FTPD_BUFFER_SIZE];                           /*  需要占用较大堆栈空间        */

#if LW_CFG_NET_LOGINBL_EN > 0
    struct sockaddr_in  addr;
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */

    pftpds->FTPDS_ulThreadId = API_ThreadIdSelf();
    pftpds->FTPDS_pfileCtrl  = fdopen(pftpds->FTPDS_iSockCtrl, "r+");   /*  创建缓冲 IO                 */
    if (pftpds->FTPDS_pfileCtrl == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "fdopen() on socket failed.\r\n");
        API_AtomicDec(&_G_atomicFtpdLinks);
        close(pftpds->FTPDS_iSockCtrl);                                 /*  关闭控制端 socket           */
        __SHEAP_FREE(pftpds);                                           /*  释放控制块内存              */
        return;
    }
    
    __FTPD_SESSION_LOCK();
    _List_Line_Add_Ahead(&pftpds->FTPDS_lineManage, 
                         &_G_plineFtpdSessionHeader);                   /*  链入会话表                  */
    __FTPD_SESSION_UNLOCK();
    
    if (ioPrivateEnv() < 0) {                                           /*  使用线程私有 io 环境        */
        goto    __error_handle;
    }
    chdir(pftpds->FTPDS_cPathBuf);                                      /*  转到服务器根目录            */
    
    __LWIP_FTPD_LOG(("ftpd: session create, remote: %s\n", 
                    inet_ntoa_r(pftpds->FTPDS_inaddrRemote, cAddr, INET_ADDRSTRLEN)));
                    
    __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_READY, __FTPD_SERVER_MESSAGE);
                                                                        /*  发送服务器就绪信息          */

    for (;;) {                                                          /*  ftp 服务器循环              */
        PCHAR   pcCmd;
        PCHAR   pcOpts;
        PCHAR   pcArgs;
        
        if (fgets(cBuffer, __LWIP_FTPD_BUFFER_SIZE, 
                  pftpds->FTPDS_pfileCtrl) == LW_NULL) {                /*  接收控制字符串              */
            __LWIP_FTPD_LOG(("ftpd: session connection aborted\n"));

#if LW_CFG_NET_LOGINBL_EN > 0                                           /*  没有登录, 开始加入黑名单    */
            if (!(pftpds->FTPDS_iStatus & __FTPD_SESSION_STATUS_LOGIN)) {
                addr.sin_len    = sizeof(struct sockaddr_in);
                addr.sin_family = AF_INET;
                addr.sin_port   = _G_uiLoginFailPort;
                addr.sin_addr   = pftpds->FTPDS_inaddrRemote;
                API_LoginBlAdd((struct sockaddr *)&addr, _G_uiLoginFailBlRep, _G_uiLoginFailBlSec);
            }
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
            break;
        }
        
        __ftpdCommandAnalyse(cBuffer, &pcCmd, &pcOpts, &pcArgs);        /*  分析命令                    */
        
        if (!lib_strcmp("QUIT", pcCmd)) {                               /*  结束 ftp 会话               */
            __ftpdSendReply(pftpds, __FTPD_RETCODE_SERVER_BYEBYE, 
                            "Thank you for using the SylixOS buildin FTP service. Goodbye.");
            break;
        } else {
            if (__ftpdCommandExec(pftpds, pcCmd, pcArgs) < 0) {         /*  执行命令                    */
                break;                                                  /*  命令错误直接结束会话        */
            }
        }
    }
                                                                        
__error_handle:
    __ftpdCloseSessionData(pftpds);                                     /*  关闭数据连接                */
    __ftpdCloseSessionCtrl(pftpds);                                     /*  关闭控制连接                */
    
    __FTPD_SESSION_LOCK();
    _List_Line_Del(&pftpds->FTPDS_lineManage, 
                   &_G_plineFtpdSessionHeader);                         /*  从会话表中解链              */
    __FTPD_SESSION_UNLOCK();
    
    API_AtomicDec(&_G_atomicFtpdLinks);                                 /*  连接数量--                  */
    __SHEAP_FREE(pftpds);                                               /*  释放控制块                  */
}
/*********************************************************************************************************
** 函数名称: __inetFtpServerListen
** 功能描述: ftp 服务器侦听服务线程
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __inetFtpServerListen (VOID)
{
    INT                 iOne = 1;
    INT                 iSock;
    INT                 iSockNew;
    __PFTPD_SESSION     pftpdsNew;
    
    struct sockaddr_in  inaddrLcl;
    struct sockaddr_in  inaddrRmt;
    
    socklen_t           uiLen;
    
    struct servent     *pservent;
    
    inaddrLcl.sin_len         = sizeof(struct sockaddr_in);
    inaddrLcl.sin_family      = AF_INET;
    inaddrLcl.sin_addr.s_addr = INADDR_ANY;
    
    pservent = getservbyname("ftp", "tcp");
    if (pservent) {
        inaddrLcl.sin_port = (u16_t)pservent->s_port;
    } else {
        inaddrLcl.sin_port = htons(21);                                 /*  ftp default port            */
    }
    
#if LW_CFG_NET_LOGINBL_EN > 0
    _G_uiLoginFailPort = inaddrLcl.sin_port;
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
    
    iSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (iSock < 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create socket.\r\n");
        return;
    }
    
    if (bind(iSock, (struct sockaddr *)&inaddrLcl, sizeof(inaddrLcl))) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "can not bind socket %s.\r\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    
    if (listen(iSock, __LW_FTPD_TCP_BACKLOG)) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "can not listen socket %s.\r\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    
    for (;;) {
        iSockNew = accept(iSock, (struct sockaddr *)&inaddrRmt, &uiLen);
        if (iSockNew < 0) {
            if (errno != ENOTCONN) {
                _DebugFormat(__ERRORMESSAGE_LEVEL, "accept failed: %s.\r\n", lib_strerror(errno));
            }
            sleep(1);                                                   /*  延迟 1 S                    */
            continue;
        }
        if (API_AtomicInc(&_G_atomicFtpdLinks) > LW_CFG_NET_FTPD_MAX_LINKS) {
            API_AtomicDec(&_G_atomicFtpdLinks);
            /*
             *  服务器链接已满, 不能再创建新的连接.
             */
            close(iSockNew);
            sleep(1);                                                   /*  延迟 1 S (防止攻击)         */
            continue;
        }
        
        /*
         *  FTPD 等待命令时, 控制连接长时间没有收到命令, 将会自动关闭以节省资源.
         */
        setsockopt(iSockNew, SOL_SOCKET, SO_RCVTIMEO, (const void *)&_G_iFtpdNoLoginTimeout, sizeof(INT));
        
        /*
         *  创建并初始化会话结构
         */
        pftpdsNew = (__PFTPD_SESSION)__SHEAP_ALLOC(sizeof(__FTPD_SESSION));
        if (pftpdsNew == LW_NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            goto    __error_handle;
        }
        lib_bzero(pftpdsNew, sizeof(__FTPD_SESSION));
        
        pftpdsNew->FTPDS_iSockCtrl         = iSockNew;                  /*  命令接口 socket             */
        pftpdsNew->FTPDS_sockaddrinDefault = inaddrRmt;                 /*  远程地址                    */
        pftpdsNew->FTPDS_inaddrRemote      = inaddrRmt.sin_addr;        /*  远程 IP                     */
        
        if (getsockname(iSockNew, (struct sockaddr *)&inaddrLcl, &uiLen) < 0) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "getsockname() failed.\r\n");
            goto    __error_handle;
        } else {
            pftpdsNew->FTPDS_bUseDefault    = LW_TRUE;
            pftpdsNew->FTPDS_sockaddrinCtrl = inaddrLcl;                /*  控制端口本地地址            */
            pftpdsNew->FTPDS_iSockPASV      = -1;
            pftpdsNew->FTPDS_iSockData      = -1;
            pftpdsNew->FTPDS_iTransMode     = __FTP_TRANSMODE_BIN;      /*  默认使用二进制模式传输      */
            pftpdsNew->FTPDS_sockaddrinData.sin_port = 
                htons((UINT16)(ntohs(pftpdsNew->FTPDS_sockaddrinCtrl.sin_port) - 1));
            pftpdsNew->FTPDS_timeStart      = lib_time(LW_NULL);        /*  记录连接时间                */
        }
        lib_strcpy(pftpdsNew->FTPDS_cPathBuf, _G_pcFtpdRootPath);       /*  初始化为设定的 ftp 根目录   */
        
        /*
         *  设置保鲜参数.
         */
        {
            INT     iKeepIdle     = __LW_FTPD_TCP_KEEPIDLE;             /*  空闲时间                    */
            INT     iKeepInterval = __LW_FTPD_TCP_KEEPINTVL;            /*  两次探测间的时间间隔        */
            INT     iKeepCount    = __LW_FTPD_TCP_KEEPCNT;              /*  探测 N 次失败认为是掉线     */
            
            setsockopt(iSockNew, SOL_SOCKET,  SO_KEEPALIVE,  (const void *)&iOne,          sizeof(INT));
            setsockopt(iSockNew, IPPROTO_TCP, TCP_KEEPIDLE,  (const void *)&iKeepIdle,     sizeof(INT));
            setsockopt(iSockNew, IPPROTO_TCP, TCP_KEEPINTVL, (const void *)&iKeepInterval, sizeof(INT));
            setsockopt(iSockNew, IPPROTO_TCP, TCP_KEEPCNT,   (const void *)&iKeepCount,    sizeof(INT));
        }
        
        /*
         *  建立服务器线程
         */
        {
            LW_CLASS_THREADATTR    threadattr;
            
            API_ThreadAttrBuild(&threadattr,
                                LW_CFG_NET_FTPD_STK_SIZE,
                                LW_PRIO_T_SERVICE,
                                LW_OPTION_THREAD_STK_CHK | LW_OPTION_OBJECT_GLOBAL,
                                (PVOID)pftpdsNew);
            if (API_ThreadCreate("t_ftpdsession", 
                                 (PTHREAD_START_ROUTINE)__inetFtpdSession, 
                                 &threadattr, 
                                 LW_NULL) == LW_OBJECT_HANDLE_INVALID) {
                goto    __error_handle;
            }
        }
        continue;                                                       /*  等待新的客户机              */
        
__error_handle:
        API_AtomicDec(&_G_atomicFtpdLinks);
        close(iSockNew);                                                /*  关闭临时连接                */
        if (pftpdsNew) {
            __SHEAP_FREE(pftpdsNew);                                    /*  释放会话控制块              */
        }
    }
}
/*********************************************************************************************************
** 函数名称: API_INetFtpServerInit
** 功能描述: 初始化 ftp 服务器根目录
** 输　入  : pcPath        本地目录
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_INetFtpServerInit (CPCHAR  pcPath)
{
    static BOOL             bIsInit = LW_FALSE;
    LW_CLASS_THREADATTR     threadattr;
           PCHAR            pcNewPath = "\0";
    
#if LW_CFG_NET_LOGINBL_EN > 0
           CHAR             cEnvBuf[32];
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */

    if (bIsInit) {                                                      /*  已经初始化了                */
        return;
    } else {
        bIsInit = LW_TRUE;
    }
    
#if LW_CFG_NET_LOGINBL_EN > 0
    if (API_TShellVarGetRt("LOGINBL_TO", cEnvBuf, (INT)sizeof(cEnvBuf)) > 0) {
        _G_uiLoginFailBlSec = lib_atoi(cEnvBuf);
    }
    if (API_TShellVarGetRt("LOGINBL_REP", cEnvBuf, (INT)sizeof(cEnvBuf)) > 0) {
        _G_uiLoginFailBlRep = lib_atoi(cEnvBuf);
    }
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */

    if (pcPath) {                                                       /*  需要设置服务器目录          */
        pcNewPath = (PCHAR)pcPath;
    }
    if (_G_pcFtpdRootPath) {
        __SHEAP_FREE(_G_pcFtpdRootPath);
    }
    _G_pcFtpdRootPath = (PCHAR)__SHEAP_ALLOC(lib_strlen(pcNewPath) + 1);
    if (_G_pcFtpdRootPath == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        goto    __error_handle;
    }
    lib_strcpy(_G_pcFtpdRootPath, pcNewPath);                           /*  保存服务器路径              */
    
    API_AtomicSet(0, &_G_atomicFtpdLinks);                              /*  连接数量清零                */
    
    _G_ulFtpdSessionLock = API_SemaphoreMCreate("ftpsession_lock", 
                                                LW_PRIO_T_NETPROTO, 
                                                LW_OPTION_WAIT_PRIORITY |
                                                LW_OPTION_INHERIT_PRIORITY |
                                                LW_OPTION_DELETE_SAFE |
                                                LW_OPTION_OBJECT_GLOBAL,
                                                LW_NULL);               /*  创建会话链表互斥量          */
    if (_G_ulFtpdSessionLock == LW_OBJECT_HANDLE_INVALID) {
        goto    __error_handle;
    }
    
    API_ThreadAttrBuild(&threadattr,
                        LW_CFG_NET_FTPD_STK_SIZE,
                        LW_PRIO_T_SERVICE,
                        LW_OPTION_THREAD_STK_CHK | LW_OPTION_OBJECT_GLOBAL,
                        LW_NULL);
    if (API_ThreadCreate("t_ftpd", (PTHREAD_START_ROUTINE)__inetFtpServerListen, 
                         &threadattr, LW_NULL) == LW_OBJECT_HANDLE_INVALID) {
        goto    __error_handle;
    }
    
#if LW_CFG_SHELL_EN > 0
    /*
     *  加入 SHELL 命令.
     */
    API_TShellKeywordAdd("ftpds", __tshellNetFtpdShow);
    API_TShellHelpAdd("ftpds",   "show ftp server session.\n");
    
    API_TShellKeywordAdd("ftpdpath", __tshellNetFtpdPath);
    API_TShellFormatAdd("ftpdpath", " [new path]");
    API_TShellHelpAdd("ftpdpath",   "set default ftp server path.\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
    
    return;
    
__error_handle:
    bIsInit = LW_FALSE;                                                 /*  初始化失败                  */
}
/*********************************************************************************************************
** 函数名称: API_INetFtpServerShow
** 功能描述: 显示 ftp 服务器根目录
** 输　入  : pcPath        本地目录
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_INetFtpServerShow (VOID)
{
    static const CHAR   _G_cFtpdInfoHdr[] = "\n"
    "    REMOTE               TIME               ALIVE(s)\n"
    "--------------- ------------------------ ------------\n";

    REGISTER PLW_LIST_LINE      plineTemp;
    REGISTER __PFTPD_SESSION    pftpds;
    
             CHAR               cAddr[INET_ADDRSTRLEN];
             time_t             timeNow = lib_time(LW_NULL);
             INT                iAlive;
             struct tm          tmTime;
             INT                iSessionCounter = 0;
             
             CHAR               cTimeBuffer[32];
             PCHAR              pcN;
    
    printf("ftpd show >>\n");
    printf("ftpd path: %s\n", (_G_pcFtpdRootPath != LW_NULL) ? (_G_pcFtpdRootPath) : "null");
    printf(_G_cFtpdInfoHdr);
    
    __FTPD_SESSION_LOCK();                                              /*  锁定会话链表                */
    for (plineTemp  = _G_plineFtpdSessionHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        pftpds = _LIST_ENTRY(plineTemp, __FTPD_SESSION, FTPDS_lineManage);
        
        inet_ntoa_r(pftpds->FTPDS_inaddrRemote, cAddr, INET_ADDRSTRLEN);/*  格式化地址字串              */
        lib_localtime_r(&pftpds->FTPDS_timeStart, &tmTime);             /*  格式化连接时间              */
        iAlive = (INT)(timeNow - pftpds->FTPDS_timeStart);              /*  计算存活时间                */
        
        lib_asctime_r(&tmTime, cTimeBuffer);
        pcN = lib_index(cTimeBuffer, '\n');
        if (pcN) {
            *pcN = PX_EOS;
        }
        
        printf("%-15s %-24s %12d\n", cAddr, cTimeBuffer, iAlive);
                                                                        /*  打印会话信息                */
        iSessionCounter++;
    }
    __FTPD_SESSION_UNLOCK();                                            /*  解锁会话链表                */

    printf("\ntotal ftp session: %d\n", iSessionCounter);               /*  显示会话数量                */
}
/*********************************************************************************************************
** 函数名称: API_INetFtpServerPath
** 功能描述: 设置 ftp 服务器根目录
** 输　入  : pcPath        本地目录
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_INetFtpServerPath (CPCHAR  pcPath)
{
    REGISTER PCHAR    pcNewPath;
    REGISTER PCHAR    pcTemp = _G_pcFtpdRootPath;
    
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
        _G_pcFtpdRootPath = pcNewPath;                                  /*  设置新的服务器路径          */
    );
    
    if (pcTemp) {
        __SHEAP_FREE(pcTemp);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellNetFtpdShow
** 功能描述: 系统命令 "ftpds"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellNetFtpdShow (INT  iArgC, PCHAR  *ppcArgV)
{
    API_INetFtpServerShow();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellNetFtpdPath
** 功能描述: 系统命令 "ftpdpath"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellNetFtpdPath (INT  iArgC, PCHAR  *ppcArgV)
{
    if (iArgC < 2) {
        printf("ftpd path: %s\n", _G_pcFtpdRootPath);                   /*  打印当前服务器目录          */
        return  (ERROR_NONE);
    }
    
    return  (API_INetFtpServerPath(ppcArgV[1]));
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_FTPD_EN > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/

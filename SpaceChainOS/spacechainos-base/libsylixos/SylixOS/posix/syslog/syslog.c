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
** 文   件   名: syslog.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2011 年 03 月 10 日
**
** 描        述: 兼容 posix syslog 库.

** BUG:
2011.03.26  默认选项中必须包含 LOG_ODELAY 标志.
            修正网络发送函数.
2011.03.28  lwip 已修复 bug#32906 不再需要 _G_sockaddrinSyslogRemote 变量.
2012.12.20  加入对 AF_UNIX 的支持.
2013.04.27  加入 syslog_r 和 setlogmask_r.
2013.09.12  使用新的缓冲区打印函数.
            syslog 缓冲区大小设置为 1024
*********************************************************************************************************/
#define  __SYLIXOS_STDARG
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../include/px_syslog.h"                                       /*  已包含操作系统头文件        */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if (LW_CFG_POSIX_EN > 0) && (LW_CFG_POSIX_SYSLOG_EN > 0)
#include "unistd.h"
#include "limits.h"
#include "socket.h"
#include "netdb.h"
#include "arpa/inet.h"
#include "sys/un.h"
/*********************************************************************************************************
  syslog file
*********************************************************************************************************/
#define __PX_SYSLOG_DEF_FLAG        0777
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static INT                          _G_iSyslogUnix    = PX_ERROR;       /*  AF_UNIX 接口                */
static INT                          _G_iSyslogSock    = PX_ERROR;       /*  网络接口                    */
static INT                          _G_iSyslogOpt     = LOG_CONS | LOG_ODELAY;       
                                                                        /*  初始化为系统终端            */
static INT                          _G_iSyslogFaci    = LOG_AUTHPRIV;
static INT                          _G_iSyslogMask    = 0;
static PCHAR                        _G_pcSyslogTag    = LW_NULL;
/*********************************************************************************************************
  syslog unix address
*********************************************************************************************************/
static struct sockaddr_un           _G_sockaddrunLog;
static socklen_t                    _G_sockelenLog;
/*********************************************************************************************************
  时间 "月" 字段缩写
*********************************************************************************************************/
extern const CHAR                   _G_cMonth[12][4];
/*********************************************************************************************************
  syslog lock
*********************************************************************************************************/
static LW_OBJECT_HANDLE             _G_ulSyslogLock;

#define __PX_SYSLOG_LOCK()          API_SemaphoreMPend(_G_ulSyslogLock, LW_OPTION_WAIT_INFINITE)
#define __PX_SYSLOG_UNLOCK()        API_SemaphoreMPost(_G_ulSyslogLock)
/*********************************************************************************************************
** 函数名称: _posixSyslogInit
** 功能描述: 初始化 syslog 库
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _posixSyslogInit (VOID)
{
    INT     iFd;

    _G_ulSyslogLock = API_SemaphoreMCreate("syslog_lock", LW_PRIO_DEF_CEILING, 
                                           LW_OPTION_WAIT_PRIORITY |
                                           LW_OPTION_INHERIT_PRIORITY |
                                           LW_OPTION_DELETE_SAFE |
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (_G_ulSyslogLock == 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not initialize mutex.\r\n");
    }
    
    lib_strlcpy(_G_sockaddrunLog.sun_path, LOG_DEFAULT_FILE, sizeof(_G_sockaddrunLog.sun_path));
    _G_sockaddrunLog.sun_family = AF_UNIX;
    _G_sockaddrunLog.sun_len    = (uint8_t)(SUN_LEN(&_G_sockaddrunLog) + 1);
                                                                        /*  包含最后一个 '\0'           */
    _G_sockelenLog              = _G_sockaddrunLog.sun_len;
    
    iFd = open(LOG_DEFAULT_FILE, O_CREAT | O_RDWR, __PX_SYSLOG_DEF_FLAG | S_IFSOCK);
    if (iFd >= 0) {
        close(iFd);
    }
}
/*********************************************************************************************************
** 函数名称: openlog
** 功能描述: opens or reopens a connection to Syslog in preparation for submitting messages.
** 输　入  : ident         is an arbitrary identification string which future syslog invocations will 
                           prefix to each message.
**           logopt        LOG_PERROR   Standard Error stream
                           LOG_CONS     system console
                           LOG_PID      inserts the calling process' Process ID (PID) into the message.
                           LOG_NDELAY   openlog opens and connects the socket
                           LOG_ODELAY   This bit does nothing.
**           facility      default facility code for this connection.
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
void  openlog (const char *ident, int logopt, int facility)
{
    struct sockaddr_in   sockaddrinRemote;
    CHAR                 cServerName[MAX_FILENAME_LENGTH];
    PCHAR                pcDivider;
    INT                  iPort = 514;
    UINT16               usPort;
    
    struct addrinfo      hints;
    struct addrinfo     *phints;
    
    if (ident) {
        _G_pcSyslogTag = (PCHAR)ident;
    }
    
    _G_iSyslogOpt = logopt;
    
    if ((facility != 0) && ((facility & ~LOG_FACMASK) == 0)) {
        _G_iSyslogFaci = facility;
    }
    
    __PX_SYSLOG_LOCK();
    if (_G_iSyslogUnix < 0) {
        _G_iSyslogUnix = socket(AF_UNIX, SOCK_DGRAM, 0);                /*  创建 AF_UNIX 套接字         */
    }
    __PX_SYSLOG_UNLOCK();
    
    if (_G_iSyslogSock >= 0) {
        return;
    }
    
#if LW_CFG_SHELL_EN > 0
    lib_bzero(cServerName, MAX_FILENAME_LENGTH);
    if (lib_getenv_r(LOG_DEFAULT_HOST_ENV, cServerName, MAX_FILENAME_LENGTH) < ERROR_NONE) {
        return;
    }
#else
    cServerName[0] = PX_EOS;
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
    
    pcDivider = lib_index(cServerName, ':');
    if (pcDivider) {
        *pcDivider = PX_EOS;
        pcDivider++;
        sscanf(pcDivider, "%d", &iPort);                                /*  如果没有读出, 默认为 514    */
    }
    
    if (!inet_aton(cServerName, &sockaddrinRemote.sin_addr)) {
        lib_bzero(&hints, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;
        hints.ai_flags  = AI_CANONNAME;
        if (getaddrinfo(cServerName, LW_NULL, &hints, &phints) >= 
            ERROR_NONE) {                                               /*  仅获取网络地址              */
            if (phints->ai_addr->sa_family == AF_INET) {
                sockaddrinRemote.sin_addr = ((struct sockaddr_in *)(phints->ai_addr))->sin_addr;
                freeaddrinfo(phints);
            } else {
                freeaddrinfo(phints);
                return;                                                 /*  不支持非 AF_INET            */
            }
        } else {
            return;                                                     /*  无法解析域名                */
        }
    }
    
    usPort = (UINT16)iPort;                                             /*  htons 内联不能直接传整型参数*/
    sockaddrinRemote.sin_len    = sizeof(struct sockaddr_in);
    sockaddrinRemote.sin_family = AF_INET;
    sockaddrinRemote.sin_port   = htons(usPort);
    
    __PX_SYSLOG_LOCK();
    if (_G_iSyslogSock < 0) {
        _G_iSyslogSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (_G_iSyslogSock < 0) {
            __PX_SYSLOG_UNLOCK();
            return;
        }
        connect(_G_iSyslogSock, (struct sockaddr *)&sockaddrinRemote, 
                sizeof(struct sockaddr_in));
    }
    __PX_SYSLOG_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: closelog
** 功能描述: closes the current Syslog connection
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
void  closelog (void)
{
    __PX_SYSLOG_LOCK();
    if (_G_iSyslogUnix >= 0) {
        close(_G_iSyslogUnix);
        _G_iSyslogUnix = PX_ERROR;
    }
    if (_G_iSyslogSock >= 0) {
        close(_G_iSyslogSock);
        _G_iSyslogSock = PX_ERROR;
    }
    __PX_SYSLOG_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: setlogmask
** 功能描述: sets a mask (the “logmask”) that determines which future syslog calls shall be ignored.
** 输　入  : maskpri       mask priority
** 输　出  : old mask
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  setlogmask (int  maskpri)
{
    INT     iOldMask = _G_iSyslogMask;

    _G_iSyslogMask = maskpri;
    
    return  (iOldMask);
}
/*********************************************************************************************************
** 函数名称: setlogmask_r
** 功能描述: sets a mask (the “logmask”) that determines which future syslog calls shall be ignored.
** 输　入  : maskpri       mask priority
**           data          syslog_data
** 输　出  : old mask
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
int  setlogmask_r (int maskpri, struct syslog_data *data)
{
    INT     iOldMask;

    if (!data) {
        errno = EINVAL;
        return  (PX_ERROR);
    }
    
    iOldMask = data->log_mask;
    if (maskpri != 0) {
        data->log_mask = maskpri;
    }
    
    return  (iOldMask);
}
/*********************************************************************************************************
** 函数名称: syslog
** 功能描述: submits a message to the Syslog facility. It does this by writing to the file 
             LOG_DEFAULT_FILE
** 输　入  : priority      priority
**           message       message
             ...
** 输　出  : old mask
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
void  syslog (int priority, const char *message, ...)
{
#define __PX_SYSLOG_TIMEFMT   "%s %2d %02d:%02d:%02d"

    CHAR            cName[HOST_NAME_MAX];
    CHAR            cBuffer[LOG_DEFAULT_SIZE];
    size_t          stLen;
    
    time_t          time;
    struct tm       tmBuf;
    
    va_list         valist;
    
    if (((unsigned)LOG_FAC(priority) >= LOG_NFACILITIES)  ||
        (LOG_MASK(LOG_PRI(priority)) == 0)                ||
        ((priority & ~(LOG_PRIMASK | LOG_FACMASK)) != 0)) {
        return;                                                         /*  不发送                      */
    }
    
    if ((_G_iSyslogUnix < 0 || _G_iSyslogSock < 0) &&
        (_G_iSyslogOpt & LOG_ODELAY)) {                                 /*  是否需要执行一次打开操作    */
        openlog(_G_pcSyslogTag, _G_iSyslogOpt, 0);
    }
    
    if ((priority & LOG_FACMASK) == 0) {
        priority |= _G_iSyslogFaci;
    }
    
    time = lib_time(LW_NULL);
    lib_localtime_r(&time, &tmBuf);                                     /*  RFC3164 is the local time   */
    
    stLen = bnprintf(cBuffer, sizeof(cBuffer), 0, "<%d>", priority);    /*  打印 priority               */
    stLen = bnprintf(cBuffer, sizeof(cBuffer), stLen, 
                     __PX_SYSLOG_TIMEFMT, 
                     _G_cMonth[tmBuf.tm_mon],
                     tmBuf.tm_mday,
                     tmBuf.tm_hour,
                     tmBuf.tm_min,
                     tmBuf.tm_sec);                                     /*  打印 时间                   */

    if (!gethostname(cName, HOST_NAME_MAX)) {
        stLen = bnprintf(cBuffer, sizeof(cBuffer), stLen, " %s", cName);
    }

    if (_G_pcSyslogTag) {                                               /*  打印 tag name               */
        stLen = bnprintf(cBuffer, sizeof(cBuffer), stLen, " %s", _G_pcSyslogTag);
    }
    
    if (_G_iSyslogOpt & LOG_PID) {
        if (sizeof(cBuffer) > stLen) {
            stLen = bnprintf(cBuffer, sizeof(cBuffer), stLen, "[%lx]: ",
                             API_ThreadIdSelf());
        }
    }
    
    if (message) {
        if (sizeof(cBuffer) > stLen) {
            
#if __STDC__
	        va_start(valist, message);
#else
	        va_start(valist);
#endif
            stLen = vbnprintf(cBuffer, sizeof(cBuffer), stLen, message, valist);
            
            va_end(valist);
        }
    }
    
    if (_G_iSyslogUnix >= 0) {
        sendto(_G_iSyslogUnix, cBuffer, stLen, 0, 
               (struct sockaddr *)&_G_sockaddrunLog, _G_sockelenLog);
    }
    
    if (_G_iSyslogSock >= 0) {
        send(_G_iSyslogSock, cBuffer, stLen, 0);
    }

    if (_G_iSyslogOpt & LOG_CONS) {
        write(STD_OUT, cBuffer, stLen);
    }
    
    if (_G_iSyslogOpt & LOG_PERROR) {
        write(STD_ERR, cBuffer, stLen);
    }
}
/*********************************************************************************************************
** 函数名称: syslog_r
** 功能描述: submits a message to the Syslog facility. It does this by writing to the file (支持不完整)
             LOG_DEFAULT_FILE
** 输　入  : priority      priority
**           data          syslog_data
**           message       message
             ...
** 输　出  : old mask
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
void  syslog_r (int priority, struct syslog_data *data, const char *message, ...)
{
#define __PX_SYSLOG_TIMEFMT   "%s %2d %02d:%02d:%02d"

    CHAR            cBuffer[LOG_DEFAULT_SIZE];
    size_t          stLen;
    
    time_t          time;
    struct tm       tmBuf;
    
    va_list         valist;
    
    if (!data) {
        errno = EINVAL;
        return;
    }
    
    if (((unsigned)LOG_FAC(priority) >= LOG_NFACILITIES)  ||
        (LOG_MASK(LOG_PRI(priority)) == 0)                ||
        ((priority & ~(LOG_PRIMASK | LOG_FACMASK)) != 0)) {
        return;                                                         /*  不发送                      */
    }
    
    if ((data->log_file < 0) && (data->log_stat & LOG_ODELAY)) {        /*  是否需要执行一次打开操作    */
        openlog(data->log_tag, data->log_file, 0);
        data->log_file = _G_iSyslogUnix;
    }
    
    if ((priority & LOG_FACMASK) == 0) {
        priority |= data->log_fac;
    }
    
    time = lib_time(LW_NULL);
    lib_localtime_r(&time, &tmBuf);                                     /*  RFC3164 is the local time   */
    
    stLen = bnprintf(cBuffer, sizeof(cBuffer), 0, "<%d>", priority);    /*  打印 priority               */
    stLen = bnprintf(cBuffer, sizeof(cBuffer), stLen, 
                     __PX_SYSLOG_TIMEFMT, 
                     _G_cMonth[tmBuf.tm_mon],
                     tmBuf.tm_mday,
                     tmBuf.tm_hour,
                     tmBuf.tm_min,
                     tmBuf.tm_sec);                                     /*  打印 时间                   */
                    
    if (data->log_tag) {
        stLen = bnprintf(cBuffer, sizeof(cBuffer), stLen, " %s", data->log_tag);
    }
    
    if (data->log_stat & LOG_PID) {
        if (sizeof(cBuffer) > stLen) {
            stLen = bnprintf(cBuffer, sizeof(cBuffer), stLen, "[%lx]:",
                             API_ThreadIdSelf());
        }
    }
    
    if (message) {
        if (sizeof(cBuffer) > stLen) {
            
#if __STDC__
	        va_start(valist, message);
#else
	        va_start(valist);
#endif
            stLen = vbnprintf(cBuffer, sizeof(cBuffer), stLen, message, valist);
            
            va_end(valist);
        }
    }
    
    if (data->log_file >= 0) {
        sendto(data->log_file, cBuffer, stLen, 0, 
               (struct sockaddr *)&_G_sockaddrunLog, _G_sockelenLog);
    }

    if (data->log_stat & LOG_CONS) {
        write(STD_OUT, cBuffer, stLen);
    }
    
    if (data->log_stat & LOG_PERROR) {
        write(STD_ERR, cBuffer, stLen);
    }
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
                                                                        /*  LW_CFG_POSIX_SYSLOG_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

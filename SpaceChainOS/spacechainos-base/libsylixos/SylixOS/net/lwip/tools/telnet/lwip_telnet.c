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
** 文   件   名: lwip_telnet.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 05 月 06 日
**
** 描        述: lwip telnet server 工具.

** BUG:
2009.07.18  将 __telnetInput() 线程的优先级要与 shell 相同, 才能确保 ascii 控制序列一次的完整的传入 shell.
            有些应用程序需要这样的操作. 例如: vi 还有就是在 ty 接收时, 先激活读同步和激活select中尽量不能
            被 shell 抢占. 
            __telnetInput() 采用 FIFO 调度 (不会被同优先级抢占).
2009.07.18  保活参数可配置. 默认大该 4 分钟(60 * 4)左右判断掉线.
2009.07.28  加入对 LW_CFG_NET_TELNET_MAX_LINKS 最大连接数量的支持.
2009.09.04  重要修正, lwip 多线程同时操作一个 socket 时, 有崩溃的可能性, 请见我提交的 #bug:27377 
            (savannah.nongnu.org/bugs/index.php?27377) 并且已经给出推荐解决的方法.
            直到1.3.1版本是还没有解决这个问题, 所以当前使用多路 I/O 复用形式解决 telnet 全双工通信问题.
            这里同时保留了 fullduplex 版本(见本目录下 lwip_telnet.c.fulduplex 文件)
2009.09.05  修改 telnet 服务结束流程, 确保系统不会因为文件被提前关闭报错. 保证稳定性.
2009.10.13  链接计数器采用 atomic_t 类型.
2009.11.21  服务线程创建失败后, 需要将连接计数器--.
2009.12.14  _G_cTelnetPtyStartName 初始化不能加入分级目录.
2011.02.21  修正头文件引用, 符合 POSIX 标准.
2011.04.07  服务器线程从 /etc/services 中获取 telnet 的服务端口, 如果没有, 默认使用 23 端口.
2011.08.06  侦听任务名改为 t_telnetd.
2012.03.26  __telnetCommunication() 优化退出机制. 所以 t_ptyproc 线程不需要再使用 FIFO 调度.
2013.05.08  pty 默认目录为 /dev/pty 目录.
2014.10.22  加入对部分 IAC 命令的处理.
2017.01.09  加入网络登录黑名单功能.
2017.04.18  修正窗口改变信号发送的错误.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_TELNET_EN > 0) && (LW_CFG_SHELL_EN > 0)
#include "netdb.h"
#include "arpa/inet.h"
#include "sys/socket.h"
#include "sys/ioctl.h"
#if LW_CFG_NET_LOGINBL_EN > 0
#include "sys/loginbl.h"
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
#include "../iac/lwip_iac.h"
#include "../SylixOS/shell/ttinyShell/ttinyShellLib.h"
/*********************************************************************************************************
  链接 keepalive 参数配置
*********************************************************************************************************/
#define __LW_TELNET_TCP_KEEPIDLE            60                          /*  空闲时间, 单位秒            */
#define __LW_TELNET_TCP_KEEPINTVL           60                          /*  两次探测间的时间间, 单位秒  */
#define __LW_TELNET_TCP_KEEPCNT             3                           /*  探测 N 次失败认为是掉线     */
/*********************************************************************************************************
  基本配置
*********************************************************************************************************/
#define __LW_TELNET_PTY_MAX_FILENAME_LENGTH     (MAX_FILENAME_LENGTH - 10)
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static CHAR           _G_cTelnetPtyStartName[MAX_FILENAME_LENGTH] = "/dev/pty";
static const CHAR     _G_cTelnetAbort[] = __TTINY_SHELL_FORCE_ABORT "\n";
static atomic_t       _G_atomicTelnetLinks;
/*********************************************************************************************************
  登录黑名单
*********************************************************************************************************/
#if LW_CFG_NET_LOGINBL_EN > 0
static UINT16         _G_uiLoginFailPort  = 0;
static UINT           _G_uiLoginFailBlSec = 120;                        /*  黑名单超时, 默认 120 秒     */
static UINT           _G_uiLoginFailBlRep = 3;                          /*  黑名单探测默认为 3 次错误   */
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
/*********************************************************************************************************
  IAC命令格式：
  FMT1 = IAC(BYTE) + CMD(BYTE)
  FMT2 = IAC(BYTE) + CMD(BYTE) + OPT(BYTE)
  FMT3 = IAC(BYTE) + SB(BYTE)  + PARAM(...) + IAC(BYTE) + SE(BYTE)
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: __telnetIacProcesser
** 功能描述: telnet IAC 命令回调函数，响应客户端发过来的 IAC 命令
** 输　入  : ulShell         shell 线程.
**           pucIACBuff      IAC命令其实位置
**           pucIACBuffEnd   缓冲区结束位置
** 输　出  : 命令占用长度，即解析数据时应该跳过的长度
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __telnetIacProcesser (LW_OBJECT_HANDLE   ulShell,
                                 PUCHAR             pucIACBuff,
                                 PUCHAR             pucIACBuffEnd)
{
    INT     iCmd;
    INT     iOpt;
    INT     iBuffLen;
    PUCHAR  pucTemp;
    
    if ((pucIACBuff    == LW_NULL)       ||
        (pucIACBuffEnd == LW_NULL)       ||
        (pucIACBuffEnd - pucIACBuff) < 2 ||
        (pucIACBuff[0] != LW_IAC_IAC)) {                                /*  只处理IAC命令               */
        return  (1);                                                    /*  命令错误，只跳过当前字符    */
    }
    
    iCmd     = pucIACBuff[1];
    iBuffLen = pucIACBuffEnd - pucIACBuff;

    switch (iCmd) {
    
    case LW_IAC_AYT:                                                    /*  Are you there命令           */
        __inetIacSend(STD_OUT, LW_IAC_AYT, -1);
        return  (2);

    case LW_IAC_WILL:                                                   /*  协商选项                    */
    case LW_IAC_WONT:
    case LW_IAC_DO:
    case LW_IAC_DONT:
        if (iBuffLen >= 3) {
            iOpt = pucIACBuff[2];                                       /*  选项码                      */
        }
        return  (__MIN(3, iBuffLen));                                   /*  跳过3个字节，含选项内容     */

    case LW_IAC_SB:                                                     /*  设置选项参数                */
        pucTemp = pucIACBuff + 2;
        while (pucTemp < pucIACBuffEnd) {
            if ((*pucTemp == LW_IAC_SE) &&
                (*(pucTemp - 1) == LW_IAC_IAC)) {                       /*  子选项以 IAC SE结束         */
                pucTemp++;
                break;
            }
            pucTemp++;
        }

        if ((pucIACBuff + 2) < pucTemp) {
            iOpt = pucIACBuff[2];                                       /*  选项码                      */
        }

        switch (iOpt) {
        
        case LW_IAC_OPT_NAWS:                                           /*  窗口大小                    */
            if ((pucIACBuff + 4) < pucTemp) {
                INT             iFd = API_IoTaskStdGet(ulShell, STD_OUT);
                struct winsize  ws;
                
#if LW_CFG_SIGNAL_EN > 0
                struct sigevent sigevent;
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
                
                ws.ws_row = (unsigned short)(((INT)pucIACBuff[5] << 8) + pucIACBuff[6]);
                ws.ws_col = (unsigned short)(((INT)pucIACBuff[3] << 8) + pucIACBuff[4]);
                ws.ws_xpixel = (unsigned short)(ws.ws_col *  8);
                ws.ws_ypixel = (unsigned short)(ws.ws_row * 16);
                ioctl(iFd, TIOCSWINSZ, &ws);                            /*  设置新的窗口大小            */
                
#if LW_CFG_SIGNAL_EN > 0
                sigevent.sigev_signo           = SIGWINCH;
                sigevent.sigev_value.sival_ptr = LW_NULL;
                sigevent.sigev_notify          = SIGEV_SIGNAL;
                API_TShellSigEvent(ulShell, &sigevent, SIGWINCH);       /*  通知应用程序                */
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
            }
            break;
        }
        return  (pucTemp - pucIACBuff);                                 /*  计算参数长度                */

    case LW_IAC_USER_SDK:                                               /*  关闭颜色显示和回显          */
        {
            ULONG  ulOpt;
            API_TShellGetOption(ulShell, &ulOpt);
            ulOpt &= ~LW_OPTION_TSHELL_VT100;
            ulOpt |=  LW_OPTION_TSHELL_NOECHO;
            API_TShellSetOption(ulShell, ulOpt);
        }
        return  (2);

    default:
        return  (2);
    }
}
/*********************************************************************************************************
** 函数名称: __telnetCommunication
** 功能描述: telnet 服务器半双工网络服务. (lwip 暂不支持全双工多线程读写同套接字操作)
**                                        (全双工的 telnet 服务器请见 lwip_telnet.c.fulduplex)
** 输　入  : iDevFd      pty 设备端
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __telnetCommunication (INT  iDevFd)
{
    CHAR        cPtyRBuffer[256];                                       /*  pty 接收缓冲                */
    CHAR        cSockRBuffer[256];                                      /*  socket 接收缓冲             */
    INT         iPtyReadNum;
    PCHAR       pcPtyBuffer;
    
    INT         iPtyLen      = 0;
    INT         iProcessLen  = 0;
    INT         iSockReadNum = 0;
    INT         iOverplus    = 0;
    
    fd_set      fdset;
    INT         iTemp;
    
    INT                 i;
    ssize_t             sstWriteDevNum = 0;
    LW_OBJECT_HANDLE    ulShell = API_ThreadGetNotePad(API_ThreadIdSelf(), 0);

    API_ThreadSetCancelState(LW_THREAD_CANCEL_ENABLE, LW_NULL);         /*  允许取消                    */
    API_ThreadSetCancelType(LW_THREAD_CANCEL_DEFERRED, LW_NULL);        /*  采用延期删除方法            */
    
    FD_ZERO(&fdset);                                                    /*  文件集清空                  */
    
    for (;;) {
        FD_SET(STD_IN, &fdset);                                         /*  网络接口读                  */
        FD_SET(iDevFd, &fdset);                                         /*  pty 设备端读                */
    
        /*
         *  iDevFd 一定大于 STD_IN, 所以 width = iDevFd + 1;
         */
        iTemp = select(iDevFd + 1, &fdset, LW_NULL, LW_NULL, LW_NULL);  /*  永久等待                    */
        if (iTemp < 0) {
            break;                                                      /*  出现意外! 直接退出          */

        } else if (iTemp == 0) {                                        /*  信号唤醒                    */
            continue;
        }
        
        API_ThreadTestCancel();                                         /*  检测是否被删除              */
        
        /*
         *  检测处理 pty 事件
         */
        if (FD_ISSET(iDevFd, &fdset)) {
            iPtyReadNum = (INT)read(iDevFd, cPtyRBuffer, sizeof(cPtyRBuffer));
                                                                        /*  从 pty 设备端读出数据       */
            if (iPtyReadNum > 0) {
                write(STD_OUT, cPtyRBuffer, iPtyReadNum);               /*  写入 telnet 网络            */

            } else {                                                    /*  停止 shell 链接             */
                write(iDevFd, (CPVOID)_G_cTelnetAbort, sizeof(_G_cTelnetAbort));
                break;
            }
        }
        
        /*
         *  检测处理网口可读事件
         */
        if (FD_ISSET(STD_IN, &fdset)) {
            if (iOverplus > 0) {                                        /*  上次 IAC 解析是否有剩余字符 */
                lib_memcpy(cSockRBuffer, 
                           &cSockRBuffer[iSockReadNum - iOverplus],
                           iOverplus);                                  /*  将没有处理的数据移动到前方  */
            }
            iSockReadNum = (INT)read(STD_IN, 
                                     &cSockRBuffer[iOverplus], 
                                     sizeof(cSockRBuffer) - iOverplus); /*  接收数据                    */
            if (iSockReadNum > 0) {
                iSockReadNum += iOverplus;                              /*  合并上一次没有解析的数据长度*/
                pcPtyBuffer = __inetIacFilter(cSockRBuffer, 
                                              iSockReadNum, 
                                              &iPtyLen, 
                                              &iProcessLen, 
                                              __telnetIacProcesser, 
                                              (PVOID)ulShell);          /*  滤除 IAC 字段               */
                if (pcPtyBuffer && (iPtyLen > 0)) {
                    if ((iPtyLen >= 2) &&
                        (pcPtyBuffer[iPtyLen - 2] == '\r') &&
                        (pcPtyBuffer[iPtyLen - 1] == '\n')) {           /*  忽略 \n pty 会自动将 \r 转义*/
                        iPtyLen--;
                    }
                    write(iDevFd, pcPtyBuffer, iPtyLen);                /*  写入 pty 终端               */
                }
                if (iProcessLen > 0) {
                    iOverplus = iSockReadNum - iProcessLen;             /*  计算未被处理的数据长度      */
                }

            } else {
                if ((errno != ETIMEDOUT) && (errno != EWOULDBLOCK)) {   /*  网络断开                    */
                    break;                                              /*  需要退出循环                */
                }
            }
        }
    }
    
    /*
     *  这里如果是 shell 主动退出, 则第一次调用 test cancel 就会被删除
     *  如果是网络中断退出, 则需要发送一遍完整的 force abort 中止 shell 即可退出.
     */
    for (i = 0; ; i++) {
        API_ThreadTestCancel();                                         /*  检测是否被删除              */
                                                                        /*  如果 shell 退出这里直接删除 */
        if (sstWriteDevNum != sizeof(_G_cTelnetAbort)) {
            ioctl(iDevFd, FIOFLUSH);                                    /*  清除 PTY 缓存所有数据       */
            sstWriteDevNum = write(iDevFd, (CPVOID)_G_cTelnetAbort, 
                                   sizeof(_G_cTelnetAbort));            /*  停止 shell                  */
        }
        
#if LW_CFG_SIGNAL_EN > 0
        if (i == (LW_TICK_HZ * 3)) {                                    /*  如果 3 秒还没有退出, kill   */
            union sigval        sigvalue;
            sigvalue.sival_int = PX_ERROR;
            sigqueue(ulShell, SIGABRT, sigvalue);                       /*  发送信号, 异常终止          */
        }
#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
        
        API_TimeSleep(LW_OPTION_WAIT_A_TICK);
    }
}
/*********************************************************************************************************
** 函数名称: __telnetServer
** 功能描述: telnet 服务器(建立完成后, 等待 shell 线程结束回收资源)
** 输　入  : iSock     服务器 socket
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __telnetServer (INT  iSock)
{
    INT                 iErrLevel = 0;

    CHAR                cPtyName[MAX_FILENAME_LENGTH];
    INT                 iHostFd;
    INT                 iDevFd;
    PVOID               pvRetValue;
    
    LW_OBJECT_HANDLE    ulTShell;
    LW_OBJECT_HANDLE    ulCommunicatThread;
    
#if LW_CFG_NET_LOGINBL_EN > 0
    BOOL                bBlAdd = LW_FALSE;
    struct sockaddr     addr;
    socklen_t           namelen = sizeof(struct sockaddr);

    if (getpeername(iSock, &addr, &namelen) == ERROR_NONE) {
        ((struct sockaddr_in *)&addr)->sin_port = _G_uiLoginFailPort;
        bBlAdd = LW_TRUE;
    }
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
    
    sprintf(cPtyName, "%s/%d", _G_cTelnetPtyStartName, iSock);          /*  生成 PTY 设备名             */

    if (ptyDevCreate(cPtyName, LW_CFG_NET_TELNET_RBUFSIZE, LW_CFG_NET_TELNET_WBUFSIZE) < 0) {
        iErrLevel = 1;
        goto    __error_handle;
    }
    
    {
        CHAR    cPtyHostName[MAX_FILENAME_LENGTH];
        /*
         *  打开 host 端文件.
         */
        sprintf(cPtyHostName, "%s.hst", cPtyName);
        iHostFd = open(cPtyHostName, O_RDWR);
        if (iHostFd < 0) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "pty host can not open.\r\n");
            iErrLevel = 2;
            goto    __error_handle;
            return;
        }
    }
    {
        CHAR    cPtyDevName[MAX_FILENAME_LENGTH];
        /*
         *  打开 dev 端文件.
         */
        sprintf(cPtyDevName, "%s.dev", cPtyName);
        iDevFd = open(cPtyDevName, O_RDWR);
        if (iDevFd < 0) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "pty dev can not open.\r\n");
            iErrLevel = 3;
            goto    __error_handle;
            return;
        }
    }
    ioctl(iHostFd, FIOSETOPTIONS, OPT_TERMINAL);                        /*  host 使用终端方式           */
    
    /*
     *  创建 PTY 输入输出线程
     */
    {
        LW_CLASS_THREADATTR    threadattr;
            
        API_ThreadAttrBuild(&threadattr,
                            LW_CFG_NET_TELNET_STK_SIZE,
                            LW_PRIO_T_SERVICE,
                            LW_OPTION_THREAD_STK_CHK | LW_OPTION_OBJECT_GLOBAL,
                            (PVOID)iDevFd);
                                                                        /*  与 shell 优先级相同         */
        threadattr.THREADATTR_ucPriority = LW_PRIO_T_SHELL;
        ulCommunicatThread = API_ThreadInit("t_ptyproc", 
                                            (PTHREAD_START_ROUTINE)__telnetCommunication, 
                                            &threadattr, 
                                            LW_NULL);                   /*  创建服务线程                */
        if (ulCommunicatThread == LW_OBJECT_HANDLE_INVALID) {
            iErrLevel = 4;
            goto    __error_handle;
        }
        
        API_IoTaskStdSet(ulCommunicatThread, STD_OUT, iSock);           /*  标准输出为 socket           */
        API_IoTaskStdSet(ulCommunicatThread, STD_IN,  iSock);           /*  标准输入为 socket           */
    }
    
    /*
     *  创建 TSHELL 服务线程
     */
    ulTShell = API_TShellCreate(iHostFd, LW_OPTION_TSHELL_VT100 | 
                                         LW_OPTION_TSHELL_AUTHEN |
                                         LW_OPTION_TSHELL_PROMPT_FULL); /*  需用户登陆                  */
    if (ulTShell == LW_OBJECT_HANDLE_INVALID) {
        iErrLevel = 5;
        goto    __error_handle;
    }
    
    API_ThreadSetNotePad(ulCommunicatThread, 0, ulTShell);
    
    __inetIacSend(iSock, LW_IAC_DO,   LW_IAC_OPT_ECHO);
    __inetIacSend(iSock, LW_IAC_DO,   LW_IAC_OPT_LFLOW);
    __inetIacSend(iSock, LW_IAC_DO,   LW_IAC_OPT_NAWS);
    __inetIacSend(iSock, LW_IAC_WILL, LW_IAC_OPT_ECHO);                 /*  服务器端回显                */
    __inetIacSend(iSock, LW_IAC_WILL, LW_IAC_OPT_SGA);                  /*  允许远程机开始通信          */
    
    API_ThreadStart(ulCommunicatThread);                                /*  启动服务线程                */
    
    API_ThreadJoin(ulTShell, &pvRetValue);                              /*  等待 shell 线程结束         */
    
#if LW_CFG_NET_LOGINBL_EN > 0
    if ((INT)pvRetValue == -ERROR_TSHELL_EUSER) {                       /*  用户登录错误                */
        if (bBlAdd) {
            API_LoginBlAdd(&addr, _G_uiLoginFailBlRep, _G_uiLoginFailBlSec);
        }
    }
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
    
    API_ThreadCancel(&ulCommunicatThread);                              /*  通知停止 t_ptyproc 线程     */
    /*
     *  删除 pty 设备, 唤醒 t_ptyproc 线程.
     *  ptyDevRemove() 同时会将 pty 文件设置为预关闭模式, 保证 select() 正确性.
     *  t_ptyproc 线程运行到 cancel 点, 会自动关闭.
     */
    ptyDevRemove(cPtyName);
    close(iDevFd);
    close(iHostFd);

    iErrLevel = 0;                                                      /*  没有任何错误, 仅关闭 socket */
    
__error_handle:
    if (iErrLevel > 4) {
        API_ThreadDelete(&ulCommunicatThread, LW_NULL);
    }
    if (iErrLevel > 3) {
        close(iDevFd);
    }
    if (iErrLevel > 2) {
        close(iHostFd);
    }
    if (iErrLevel > 1) {
        ptyDevRemove(cPtyName);
    }
    close(iSock);
    
    API_AtomicDec(&_G_atomicTelnetLinks);                               /*  链接数量--                  */
}
/*********************************************************************************************************
** 函数名称: __telnetListener
** 功能描述: telnet 侦听器线程
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __telnetListener (VOID)
{
    INT                 iOne = 1;
    INT                 iSock;
    INT                 iSockNew;
    
    struct sockaddr_in  inaddrLcl;
    struct sockaddr_in  inaddrRmt;
    
    socklen_t           uiLen;
    
    struct servent     *pservent;
    
    inaddrLcl.sin_len         = sizeof(struct sockaddr_in);
    inaddrLcl.sin_family      = AF_INET;
    inaddrLcl.sin_addr.s_addr = INADDR_ANY;
    
    pservent = getservbyname("telnet", "tcp");
    if (pservent) {
        inaddrLcl.sin_port = (u16_t)pservent->s_port;
    } else {
        inaddrLcl.sin_port = htons(23);                                 /*  telnet default port         */
    }
    
#if LW_CFG_NET_LOGINBL_EN > 0
    _G_uiLoginFailPort = inaddrLcl.sin_port;
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */

    iSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (iSock < 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not create socket.\r\n");
        return;
    }
                                                                        /*  不使用 nagle 算法           */
    setsockopt(iSock, IPPROTO_TCP, TCP_NODELAY, (const void *)&iOne, sizeof(iOne));
    
    if (bind(iSock, (struct sockaddr *)&inaddrLcl, sizeof(inaddrLcl))) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "can not bind socket %s.\r\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    
    if (listen(iSock, LW_CFG_NET_TELNET_MAX_LINKS)) {
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
        if (API_AtomicInc(&_G_atomicTelnetLinks) > LW_CFG_NET_TELNET_MAX_LINKS) {
            API_AtomicDec(&_G_atomicTelnetLinks);
            /*
             *  服务器链接已满, 不能再创建新的连接.
             */
            write(iSockNew, "server is full of links", lib_strlen("server is full of links"));
            close(iSockNew);
            sleep(1);                                                   /*  延迟 1 S (防止攻击)         */
            continue;
        }
        
        setsockopt(iSockNew, IPPROTO_TCP, TCP_NODELAY, (const void *)&iOne, sizeof(iOne));
        /*
         *  设置保鲜参数.
         */
        {
            INT     iKeepIdle     = __LW_TELNET_TCP_KEEPIDLE;           /*  空闲时间                    */
            INT     iKeepInterval = __LW_TELNET_TCP_KEEPINTVL;          /*  两次探测间的时间间隔        */
            INT     iKeepCount    = __LW_TELNET_TCP_KEEPCNT;            /*  探测 N 次失败认为是掉线     */
            
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
                                LW_CFG_NET_TELNET_STK_SIZE,
                                LW_PRIO_T_SERVICE,
                                LW_OPTION_THREAD_STK_CHK | LW_OPTION_OBJECT_GLOBAL,
                                (PVOID)iSockNew);
            if (API_ThreadCreate("t_ptyserver", 
                                 (PTHREAD_START_ROUTINE)__telnetServer, 
                                 &threadattr, 
                                 LW_NULL) == LW_OBJECT_HANDLE_INVALID) {
                API_AtomicDec(&_G_atomicTelnetLinks);
                close(iSockNew);                                        /*  关闭临时连接                */
            }
        }
    }
}
/*********************************************************************************************************
** 函数名称: API_INetTelnetInit
** 功能描述: 初始化 telnet 工具
** 输　入  : pcPtyStartName    pty 起始文件名, 例如: "/dev/pty", 之后系统建立的 pty 为 
                                                     "/dev/pty/?.hst" 
                                                     "/dev/pty/?.dev"
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_INetTelnetInit (const PCHAR  pcPtyStartName)
{
    static   BOOL                   bIsInit = LW_FALSE;
    REGISTER size_t                 stNameLen;
             LW_CLASS_THREADATTR    threadattr;
             LW_OBJECT_HANDLE       ulId;
             
#if LW_CFG_NET_LOGINBL_EN > 0
             CHAR                   cEnvBuf[32];
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */

    if (bIsInit) {
        return;
    }
    
    if (ptyDrv()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not install pty driver.\r\n");
        return;
    }

    if (pcPtyStartName) {
        stNameLen = lib_strnlen(pcPtyStartName, __LW_TELNET_PTY_MAX_FILENAME_LENGTH);
        if (stNameLen >= __LW_TELNET_PTY_MAX_FILENAME_LENGTH) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "name too long.\r\n");
            return;
        
        } else if (stNameLen > 1) {                                     /*  不为根目录                  */
            lib_strcpy(_G_cTelnetPtyStartName, pcPtyStartName);         /*  pty 设备路径                */
            if (_G_cTelnetPtyStartName[stNameLen - 1] == PX_DIVIDER) {
                _G_cTelnetPtyStartName[stNameLen - 1] =  PX_EOS;        /*  不以 / 符号结尾             */
            }
        }
    }
    
    API_AtomicSet(0, &_G_atomicTelnetLinks);                            /*  连接数量清零                */
    
#if LW_CFG_NET_LOGINBL_EN > 0
    if (API_TShellVarGetRt("LOGINBL_TO", cEnvBuf, (INT)sizeof(cEnvBuf)) > 0) {
        _G_uiLoginFailBlSec = lib_atoi(cEnvBuf);
    }
    if (API_TShellVarGetRt("LOGINBL_REP", cEnvBuf, (INT)sizeof(cEnvBuf)) > 0) {
        _G_uiLoginFailBlRep = lib_atoi(cEnvBuf);
    }
#endif                                                                  /*  LW_CFG_NET_LOGINBL_EN > 0   */
    
    API_ThreadAttrBuild(&threadattr, 
                        LW_CFG_NET_TELNET_STK_SIZE, 
                        LW_PRIO_T_SERVICE,
                        (LW_OPTION_THREAD_STK_CHK | LW_OPTION_THREAD_SAFE | LW_OPTION_OBJECT_GLOBAL),
                        LW_NULL);
    ulId = API_ThreadCreate("t_telnetd", (PTHREAD_START_ROUTINE)__telnetListener, &threadattr, LW_NULL);
    if (ulId) {
        bIsInit = LW_TRUE;
    }
}

#endif                                                                  /*  (LW_CFG_NET_EN > 0)         */
                                                                        /*  (LW_CFG_NET_TELNET_EN > 0)  */
                                                                        /*  (LW_CFG_SHELL_EN > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

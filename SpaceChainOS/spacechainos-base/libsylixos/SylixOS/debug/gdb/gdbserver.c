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
** 文   件   名: gdbserver.c
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2014 年 05 月 06 日
**
** 描        述: GDBServer 主控程序
**
** BUG:
2014.05.31  使用 LW_VPROC_EXIT_FORCE 删除进程.
2014.06.03  使用 posix_spawnp 创建进程.
2014.09.02  修正调试同时单步多线程问题.
2015.11.18  加入 SMP 锁定 CPU 调试功能.
2016.06.14  支持在当前终端下调试.
2016.08.16  支持硬件单步调试.
2016.12.07  GDB 退出时, 首先 kill 掉被调试进程, 然后再放行进程, 确保不会多执行一句代码.
2017.02.21  修改当侦听连接时kill调试器失败以及端口未释放的问题.
*********************************************************************************************************/
#define  __SYLIXOS_GDB
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "spawn.h"
#include "dtrace.h"
#include "socket.h"
#include "sys/signalfd.h"
#if LW_CFG_SMP_EN > 0
#include "sched.h"
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0
#include "arch/arch_gdb.h"
#include "../SylixOS/loader/elf/elf_type.h"
#include "../SylixOS/loader/include/loader.h"
#include "../SylixOS/loader/include/loader_vppatch.h"
/*********************************************************************************************************
  Non-STOP 模式支持判断
*********************************************************************************************************/
#if defined(LW_CFG_CPU_ARCH_PPC) || (defined(LW_CFG_CPU_ARCH_X86) && (LW_CFG_CPU_WORD_LENGHT == 64))
#define LW_CFG_GDB_NON_STOP_EN      0
#else
#define LW_CFG_GDB_NON_STOP_EN      1
#endif
/*********************************************************************************************************
  错误码定义
*********************************************************************************************************/
#define ERROR_GDB_INIT_SOCK         200000
#define ERROR_GDB_PARAM             200001
#define ERROR_GDB_ATTACH_PROG       200002
/*********************************************************************************************************
  常量定义
*********************************************************************************************************/
#define GDB_RSP_MAX_LEN             0x2000                              /*  rsp 缓冲区大小              */
#define GDB_MAX_THREAD_NUM          LW_CFG_MAX_THREADS                  /*  最大线程数                  */
/*********************************************************************************************************
  链接 keepalive 参数配置
*********************************************************************************************************/
#define GDB_TCP_KEEPIDLE            60                                  /*  空闲时间, 单位秒            */
#define GDB_TCP_KEEPINTVL           60                                  /*  两次探测间的时间间, 单位秒  */
#define GDB_TCP_KEEPCNT             3                                   /*  探测 N 次失败认为是掉线     */
/*********************************************************************************************************
  内存管理
*********************************************************************************************************/
#define LW_GDB_SAFEMALLOC(size)     __SHEAP_ALLOC((size_t)size)
#define LW_GDB_SAFEFREE(a)          { if (a) { __SHEAP_FREE((PVOID)a); a = 0; } }
/*********************************************************************************************************
  打印函数
*********************************************************************************************************/
#define LW_GDB_MSG                  printk
/*********************************************************************************************************
  GDB Non-STOP 模式 BUG
*********************************************************************************************************/
#define GDB_NON_STOP_BUG
/*********************************************************************************************************
  通信类型
*********************************************************************************************************/
typedef enum {
    COMM_TYPE_TCP = 1,                                                  /* TCP                          */
    COMM_TYPE_TTY                                                       /* 串口                         */
}LW_GDB_COMM_TYPE;
/*********************************************************************************************************
  断点数据结构
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            BP_plistBpLine;                             /* 断点链表                     */
    addr_t                  BP_addr;                                    /* 传输类型                     */
    size_t                  BP_size;
    ULONG                   BP_ulInstOrg;                               /* 断点内容原指令               */
} LW_GDB_BP;
/*********************************************************************************************************
  线程结构
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            TH_plistThLine;                             /* 线程链表头                   */
    ULONG                   TH_ulId;                                    /* 线程ID                       */
    CHAR                    TH_cStates;                                 /* 线程状态                     */
    addr_t                  TH_addrLow;                                 /* 区域单步起始地址             */
    addr_t                  TH_addrHigh;                                /* 区域单步结束地址             */
    BOOL                    TH_bChanged;                                /* 状态是否被修改               */
} LW_GDB_THREAD;
/*********************************************************************************************************
  串口调试原始参数
*********************************************************************************************************/
typedef struct {
    LONG                    GDBSP_lBaud;                                /* 串口波特率                   */
    INT                     GDBSP_iHwOpt;                               /* 硬件属性                     */
    INT                     GDBSP_iSioOpt;                              /* TTY 属性                     */
} LW_GDB_SERIAL_PARAM;
/*********************************************************************************************************
  全局参数结构
*********************************************************************************************************/
typedef struct {
    BYTE                    GDB_byCommType;                             /* 传输类型                     */
    LW_GDB_SERIAL_PARAM     GDB_spSerial;                               /* 需要回复的串口参数           */
    BOOL                    GDB_bTerminal;                              /* 是否为终端调试               */
    INT                     GDB_iCommFd;                                /* 用于传输协议帧的文件描述符   */
    INT                     GDB_iCommListenFd;                          /* 用于端口侦听的文件描述符     */
    INT                     GDB_iSigFd;                                 /* 中断signalfd文件句柄         */
    CHAR                    GDB_cProgPath[MAX_FILENAME_LENGTH];         /* 可执行文件路径               */
    pid_t                   GDB_iPid;                                   /* 跟踪的进程号                 */
    PVOID                   GDB_pvDtrace;                               /* dtrace对象句柄               */
    LW_LIST_LINE_HEADER     GDB_plistBpHead;                            /* 断点链表                     */
    LONG                    GDB_lOpCThreadId;                           /* c,s命令线程                  */
    LONG                    GDB_lOpGThreadId;                           /* 其它命令线程                 */
    ULONG                   GDB_lOptThreadId;                           /* t命令停止的线程              */
    BOOL                    GDB_bNonStop;                               /* Non-stop模式                 */
    UINT                    GDB_uiThdNum;                               /* 线程数                       */
    ULONG                   GDB_ulThreads[GDB_MAX_THREAD_NUM + 1];      /* 线程数组                     */
    LW_LIST_LINE_HEADER     GDB_plistThd;                               /* 线程链表                     */
    BOOL                    GDB_beNotifing;                             /* 是否正在处理notify           */
    BOOL                    GDB_bExited;                                /* 进程是否退出                 */
    BOOL                    GDB_bStarted;                               /* 进程是已经启动               */
    BOOL                    GDB_bAttached;                              /* 是否为attach进程             */
    BOOL                    GDB_bLockCpu;                               /* 是否锁定被调试进程cpu        */
} LW_GDB_PARAM;
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
#ifndef LW_DTRACE_HW_ISTEP
static atomic_t     GHookRefCnt = {0};                                  /* gdb hook 引用计数            */
#endif                                                                  /* !LW_DTRACE_HW_ISTEP          */
/*********************************************************************************************************
** 函数名称: gdbSetStepMode
** 功能描述: 启动单步模式
** 输　入  : pparam       GDB 参数
**           ulThread     线程句柄
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID gdbSetStepMode (LW_GDB_PARAM *pparam, LW_OBJECT_HANDLE ulThread)
{
#ifdef LW_DTRACE_HW_ISTEP
    API_DtraceThreadStepSet(pparam->GDB_pvDtrace, ulThread, LW_TRUE);
    
#else
    GDB_REG_SET regset;
    addr_t      addrNP;

    archGdbRegsGet(pparam->GDB_pvDtrace, ulThread, &regset);
    addrNP = archGdbGetNextPc(pparam->GDB_pvDtrace, ulThread, &regset);
    API_DtraceThreadStepSet(pparam->GDB_pvDtrace, ulThread, addrNP);
#endif                                                                  /*  LW_DTRACE_HW_ISTEP          */
}
/*********************************************************************************************************
** 函数名称: gdbClearStepMode
** 功能描述: 清除单步模式
** 输　入  : pparam       GDB 参数
**           ulThread     线程句柄
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID gdbClearStepMode (LW_GDB_PARAM *pparam, LW_OBJECT_HANDLE ulThread)
{
#ifdef LW_DTRACE_HW_ISTEP
    API_DtraceThreadStepSet(pparam->GDB_pvDtrace, ulThread, LW_FALSE);
    
#else
    API_DtraceThreadStepSet(pparam->GDB_pvDtrace, ulThread, (addr_t)PX_ERROR);
#endif                                                                  /*  LW_DTRACE_HW_ISTEP          */
}
/*********************************************************************************************************
** 函数名称: gdbIsStepBp
** 功能描述: 判断是否为单步断点
** 输　入  : pparam       GDB 参数
**           pdmsg        断点消息指针
** 输　出  : LW_TRUE--单步断点，LW_FALSE--非单步断点
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL gdbIsStepBp (LW_GDB_PARAM *pparam, PLW_DTRACE_MSG pdmsg)
{
#ifdef LW_DTRACE_HW_ISTEP
    if (pdmsg->DTM_uiType == LW_TRAP_ISTEP) {
        return  (LW_TRUE);
    
    } else {
        return  (LW_FALSE);
    }

#else
    addr_t      addrNP;

    API_DtraceThreadStepGet(pparam->GDB_pvDtrace,
                            pdmsg->DTM_ulThread, &addrNP);
    if ((addrNP != PX_ERROR) && (pdmsg->DTM_ulAddr == addrNP)) {
        return  (LW_TRUE);
    
    } else {
        return  (LW_FALSE);
    }
#endif                                                                  /*  LW_DTRACE_HW_ISTEP          */
}
/*********************************************************************************************************
** 函数名称: gdbRemoveStepBp
** 功能描述: 删除线程中已经发生的单步断点
** 输　入  : pparam       GDB 参数
**           ulThread     线程句柄
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID gdbRemoveStepBp (LW_GDB_PARAM *pparam, LW_OBJECT_HANDLE ulThread)
{
#ifdef LW_DTRACE_HW_ISTEP
    API_DtraceDelBreakInfo(pparam->GDB_pvDtrace, ulThread, (addr_t)PX_ERROR, LW_TRUE);
    
#else
    addr_t      addrNP;

    API_DtraceThreadStepGet(pparam->GDB_pvDtrace, ulThread, &addrNP);
    if (addrNP != PX_ERROR) {
        API_DtraceDelBreakInfo(pparam->GDB_pvDtrace, ulThread, addrNP, LW_TRUE);
    }
#endif                                                                  /*  LW_DTRACE_HW_ISTEP          */
}
/*********************************************************************************************************
** 函数名称: gdbTcpSockInit
** 功能描述: 初始化socket
** 输　入  : pparam       GDB 参数
**           ui32Ip       侦听ip
**           usPort       侦听端口
** 输　出  : 成功-- fd描述符，失败-- PX_ERROR.
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbTcpSockInit (LW_GDB_PARAM *pparam, UINT32 ui32Ip, UINT16 usPort)
{
    struct sockaddr_in  addrServer;
    struct sockaddr_in  addrClient;
    INT                 iSockListen;
    INT                 iSockNew;
    INT                 iKeepIdle     = GDB_TCP_KEEPIDLE;               /*  空闲时间                    */
    INT                 iKeepInterval = GDB_TCP_KEEPINTVL;              /*  两次探测间的时间间隔        */
    INT                 iKeepCount    = GDB_TCP_KEEPCNT;                /*  探测 N 次失败认为是掉线     */
    INT                 iOpt          = 1;
    socklen_t           iAddrLen      = sizeof(addrClient);
    CHAR                cIpBuff[32]   = {0};

    bzero(&addrServer, sizeof(addrServer));
    if (0 == ui32Ip) {
        ui32Ip = INADDR_ANY;
    }
    
    addrServer.sin_family       = AF_INET;
    addrServer.sin_addr.s_addr  = htonl(ui32Ip);
    addrServer.sin_port         = htons(usPort);
    
    iSockListen = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (iSockListen < 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Create listen sock failed.\r\n");
        return  (PX_ERROR);
    }

    setsockopt(iSockListen, SOL_SOCKET, SO_REUSEADDR, &iOpt, sizeof(iOpt));
    
    if (bind(iSockListen, (struct sockaddr*)&addrServer, sizeof(addrServer))) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Bind sock failed.\r\n");
        close(iSockListen);
        return  (PX_ERROR);
    }

    if (listen(iSockListen, 1)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Bind sock failed.\r\n");
        close(iSockListen);
        return  (PX_ERROR);
    }
    
    pparam->GDB_iCommListenFd = iSockListen;

    /*
     *  此处使用printf输出到telnet终端，IDE检测本输出以确保可以启动gdb调试
     */
    printf("[GDB]Waiting for connect...\n");
    
    iSockNew = accept(iSockListen, (struct sockaddr *)&addrClient, &iAddrLen);
    if (iSockNew < 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Accept failed.\r\n");
        return  (PX_ERROR);
    }

    setsockopt(iSockNew, IPPROTO_TCP, TCP_NODELAY,   (const void *)&iOpt,          sizeof(INT));
    setsockopt(iSockNew, SOL_SOCKET,  SO_KEEPALIVE,  (const void *)&iOpt,          sizeof(INT));
    setsockopt(iSockNew, IPPROTO_TCP, TCP_KEEPIDLE,  (const void *)&iKeepIdle,     sizeof(INT));
    setsockopt(iSockNew, IPPROTO_TCP, TCP_KEEPINTVL, (const void *)&iKeepInterval, sizeof(INT));
    setsockopt(iSockNew, IPPROTO_TCP, TCP_KEEPCNT,   (const void *)&iKeepCount,    sizeof(INT));

    printf("[GDB]Connected. host: %s\n",
           inet_ntoa_r(addrClient.sin_addr, cIpBuff, sizeof(cIpBuff)));

    return  (iSockNew);
}
/*********************************************************************************************************
** 函数名称: gdbSerialInit
** 功能描述: 初始化 tty
** 输　入  : pparam       GDB 参数
**           pcSerial     tty 设备名
** 输　出  : 成功-- fd描述符，失败-- PX_ERROR.
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbSerialInit (LW_GDB_PARAM *pparam, CPCHAR pcSerial)
{
#define SERIAL_PARAM    "115200,n,8,1"
#define SERIAL_BAUD     SIO_BAUD_115200
#define SERIAL_HWOPT    (CLOCAL | CREAD | CS8 | HUPCL)
#define SERIAL_BSIZE    LW_OSIOD_LARG(1024)

    INT  iFd;
    
    if (lib_strcmp(pcSerial, "terminal") == 0) {                        /*  使用当前终端串口调试        */
        INT  iFdIn, iFdOut;
        
        iFdIn  = API_IoTaskStdGet(API_ThreadIdSelf(), STD_IN);
        iFdOut = API_IoTaskStdGet(API_ThreadIdSelf(), STD_OUT);
        if (iFdIn != iFdOut) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Standard 'IN' 'OUT' device not same.\r\n");
            return  (PX_ERROR);
        }
        
        iFd                   = iFdOut;
        pparam->GDB_bTerminal = LW_TRUE;
    
    } else {                                                            /*  新建串口                    */
        iFd = open(pcSerial, O_RDWR);
        if (iFd < 0) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Open serial failed.\r\n");
            return  (PX_ERROR);
        }
    }
    
    if (!isatty(iFd)) {
        if (pparam->GDB_bTerminal == LW_FALSE) {
            close(iFd);
        }
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Serial is not a tty device.\r\n");
        return  (PX_ERROR);
    }
    
    if (pparam->GDB_bTerminal) {                                        /*  保存 terminal 参数          */
        ioctl(iFd, FIOGETOPTIONS,   &pparam->GDB_spSerial.GDBSP_iSioOpt);
        ioctl(iFd, SIO_BAUD_GET,    &pparam->GDB_spSerial.GDBSP_lBaud);
        ioctl(iFd, SIO_HW_OPTS_GET, &pparam->GDB_spSerial.GDBSP_iHwOpt);
    }
    
    ioctl(iFd, FIOSETOPTIONS,   OPT_RAW);
    ioctl(iFd, SIO_BAUD_SET,    SERIAL_BAUD);
    ioctl(iFd, SIO_HW_OPTS_SET, SERIAL_HWOPT);
    ioctl(iFd, FIORBUFSET,      SERIAL_BSIZE);
    ioctl(iFd, FIOWBUFSET,      SERIAL_BSIZE);
    
    LW_GDB_MSG("[GDB]Serial device: %s %s\n", pcSerial, SERIAL_PARAM);
    
    return  (iFd);
}
/*********************************************************************************************************
** 函数名称: gdbAsc2Hex
** 功能描述: ASC转换成hex值
** 输　入  : ch       ASC字符
** 输　出  : hex值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbAsc2Hex (CHAR ch)
{
    if ((ch >= 'a') && (ch <= 'f')) {
        return  (ch - 'a' + 10);
    }
    if ((ch >= '0') && (ch <= '9')) {
        return  (ch - '0');
    }
    if ((ch >= 'A') && (ch <= 'F')) {
        return  (ch - 'A' + 10);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: gdbByte2Asc
** 功能描述: byte转换成ASC字符串
** 输　入  : iByte       byte值
** 输　出  : pcAsc       转化后的ASC字符串
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID gdbByte2Asc (PCHAR pcAsc, BYTE iByte)
{
    BYTE  val = (iByte >> 4) & 0xf;

    pcAsc[0] = val > 9 ? (val - 0xA + 'a') : (val + '0');
    val = iByte & 0xf;
    pcAsc[1] = val > 9 ? (val - 0xA + 'a') : (val + '0');
}
/*********************************************************************************************************
** 函数名称: gdbAscToByte
** 功能描述: ASC字符串转换成byte字节
** 输　入  : pcAsc       ASC字符串值
** 输　出  : 转换后的数值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbAscToByte (PCHAR pcAsc)
{
    return  (gdbAsc2Hex(pcAsc[0]) << 4) + gdbAsc2Hex(pcAsc[1]);
}
/*********************************************************************************************************
** 函数名称: gdbReg2Asc
** 功能描述: ULONG转换成ASC字符串
** 输　入  : ulReg       ULONG值
** 输　出  : pcAsc       转化后的ASC字符串
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID gdbReg2Asc (PCHAR pcAsc, ULONG ulReg)
{
    INT i;

#if LW_CFG_CPU_ENDIAN == 0
    for (i = 0; i < sizeof(ulReg); i++, ulReg >>= 8) {
        gdbByte2Asc(pcAsc + (i * 2), ulReg & 0xFF);
    }
#else
    for (i = 0; i < sizeof(ulReg); i++, ulReg >>= 8) {
        gdbByte2Asc(pcAsc + ((sizeof(ulReg) - i - 1) * 2), ulReg & 0xFF);
    }
#endif
}
/*********************************************************************************************************
** 函数名称: gdbAscToReg
** 功能描述: ASC字符串转换成ULONG
** 输　入  : pcAsc       ASC字符串值
** 输　出  : 转换后的数值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ULONG gdbAscToReg (char *pcAsc)
{
    ULONG       ulReg = 0;
    INT         i;

#if LW_CFG_CPU_ENDIAN == 0
    for (i = 0; i < sizeof(ulReg); i++, ulReg <<= 8) {
        ulReg += gdbAscToByte(pcAsc + ((sizeof(ulReg) - i - 1) * 2));
    }
#else
    for (i = 0; i < sizeof(ulReg); i++, ulReg <<= 8) {
        ulReg += gdbAscToByte(pcAsc + (i * 2));
    }
#endif

    return  (ulReg);
}
/*********************************************************************************************************
** 函数名称: gdbReplyError
** 功能描述: 组装rsp错误包
** 输　入  : pcReply       输出缓冲区
**           iErrNum       错误码
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID gdbReplyError (PCHAR pcReply, INT iErrNum)
{
    pcReply[0] = 'E';
    gdbByte2Asc(pcReply + 1, iErrNum);
    pcReply[3] = 0;
}
/*********************************************************************************************************
** 函数名称: gdbReplyOk
** 功能描述: 组装rsp ok包
** 输　入  : pcReply       输出缓冲区
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID gdbReplyOk (PCHAR pcReply)
{
    lib_strcpy(pcReply, "OK");
}
/*********************************************************************************************************
** 函数名称: gdbRspPkgPut
** 功能描述: 发送rsp数据包
** 输　入  : pparam       系统参数
**           pcOutBuff    发送缓冲区
**           bNotify      是否发送notify包
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbRspPkgPut (LW_GDB_PARAM *pparam, PCHAR pcOutBuff, BOOL bNotify)
{
    CHAR    cHeader    = '$';
    UCHAR   ucCheckSum = 0;
    INT     iPkgLen    = 0;
    INT     iSendLen   = 0;
    INT     iDataLen   = 0;
    PCHAR   pcWPos;
    CHAR    cAck;

    while (pcOutBuff[iPkgLen] != '\0') {
        ucCheckSum += pcOutBuff[iPkgLen];
        iPkgLen++;
    }

    if (bNotify) {                                                      /* 异步通知包                   */
        cHeader = '%';
    }

    if (iPkgLen > GDB_RSP_MAX_LEN - 10) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Package Too long.\r\n");
        return  (PX_ERROR);
    }

    pcOutBuff[iPkgLen++] = '#';
    gdbByte2Asc(&pcOutBuff[iPkgLen], ucCheckSum);
    iPkgLen += 2;

    do {
        if (write(pparam->GDB_iCommFd, &cHeader, 1) < 0) {
            return  (PX_ERROR);
        }

        pcWPos   = pcOutBuff;
        iDataLen = iPkgLen;

        while (iDataLen > 0) {
            iSendLen = write(pparam->GDB_iCommFd, pcWPos, iDataLen);
            if (iSendLen <= 0) {
                return  (PX_ERROR);
            }
            iDataLen -= iSendLen;
            pcWPos  += iSendLen;
            if (iDataLen <= 0) {
                break;
            }
        }

        if (bNotify) {                                                  /* notify命令gdb不回应+/-       */
            cAck = '+';
        } else {
            if (read(pparam->GDB_iCommFd, &cAck, 1) <= 0) {
                return  (PX_ERROR);
            }
        }
    } while(cAck != '+');

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gdbRspPkgGet
** 功能描述: 接受rsp数据包
** 输　入  : pparam       系统参数
**           pcInBuff     接受缓冲区
** 输　出  : pfdsi        如果为中断信号，则输出信号信息
**           返回值       ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbRspPkgGet (LW_GDB_PARAM *pparam, PCHAR pcInBuff, struct signalfd_siginfo *pfdsi)
{
    CHAR    cHeader;
    CHAR    cSend;
    INT     iReadLen    = 0;
    INT     iReadChkSum = 0;
    INT     iDataLen    = 0;
    UCHAR   ucCheckSum  = 0;
    UCHAR   ucXmitcSum  = 0;

    fd_set  fdset;
    ssize_t szLen;
    INT     iMaxFd;

    iMaxFd = max(pparam->GDB_iCommFd, pparam->GDB_iSigFd);

    while (1) {
        FD_ZERO(&fdset);
        FD_SET(pparam->GDB_iCommFd, &fdset);
        if (pparam->GDB_bNonStop) {                                     /* nonstop中断与网络异步操作    */
            FD_SET(pparam->GDB_iSigFd, &fdset);
        }

        while (select(iMaxFd + 1, &fdset, NULL, NULL, NULL) >= 0) {
            if (FD_ISSET(pparam->GDB_iCommFd, &fdset)) {                /* 网络命令                     */
                iReadLen = read(pparam->GDB_iCommFd, &cHeader, 1);
                if (iReadLen <= 0) {
                    return  (PX_ERROR);
                }
                if ((cHeader & 0x7F) == '$') {
                    break;
                }
            }

            if (FD_ISSET(pparam->GDB_iSigFd, &fdset)) {                 /* 中断到来                     */
                szLen = read(pparam->GDB_iSigFd, pfdsi,
                             sizeof(struct signalfd_siginfo));
                if (szLen != sizeof(struct signalfd_siginfo)) {
                    FD_SET(pparam->GDB_iCommFd, &fdset);
                    continue;
                }
                return  (ERROR_NONE);
            }

            FD_SET(pparam->GDB_iCommFd, &fdset);
            if (pparam->GDB_bNonStop) {
                FD_SET(pparam->GDB_iSigFd, &fdset);
            }
        }

        cSend = '+';
        iReadLen = read(pparam->GDB_iCommFd,
                        pcInBuff + iDataLen,
                        GDB_RSP_MAX_LEN - iDataLen);

        while (iReadLen > 0) {
            while (iReadLen > 0) {
                if ((pcInBuff[iDataLen] & 0x7F) == '#') {
                    while (iReadLen < 3 &&
                           (GDB_RSP_MAX_LEN - iDataLen) > 3) {          /* 读取未完成的字节             */
                        iReadChkSum = read(pparam->GDB_iCommFd,
                                           pcInBuff + iDataLen + iReadLen,
                                           3 - iReadLen);
                        if (iReadChkSum <= 0) {
                            return  (PX_ERROR);
                        }
                        iReadLen += iReadChkSum;
                    }
                    ucXmitcSum  = gdbAsc2Hex(pcInBuff[iDataLen + 1] & 0x7f) << 4;
                    ucXmitcSum += gdbAsc2Hex(pcInBuff[iDataLen + 2] & 0x7f);

                    if (ucXmitcSum == ucCheckSum) {                     /* 发送确认包                   */
                        cSend = '+';
                        write(pparam->GDB_iCommFd, &cSend, sizeof(cSend));
                        return  (iDataLen);
                    
                    } else {
                        cSend = '-';
                        write(pparam->GDB_iCommFd, &cSend, sizeof(cSend));
                        break;
                    }

                } else {
                    ucCheckSum += (pcInBuff[iDataLen] & 0x7F);
                }
                iDataLen++;
                iReadLen--;
            }

            if (cSend == '-') {
                break;
            }

            iReadLen = read(pparam->GDB_iCommFd,
                            pcInBuff + iDataLen,
                            GDB_RSP_MAX_LEN - iDataLen);
        }

        if (iReadLen <= 0) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gdbReportStopReason
** 功能描述: 上报停止原因
** 输　入  : pdmsg         中断信息
**           pcOutBuff     输出缓冲区
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbReportStopReason (PLW_DTRACE_MSG pdmsg, PCHAR pcOutBuff)
{
    UINT uiSig = SIGSEGV;

    if (pdmsg->DTM_uiType == LW_TRAP_QUIT) {                             /* 进程退出                     */
        sprintf(pcOutBuff, "W00");
        return  (ERROR_NONE);
    }

    uiSig = ((pdmsg->DTM_uiType == LW_TRAP_ABORT) ? SIGSEGV : SIGTRAP);
    sprintf(pcOutBuff, "T%02xthread:%lx;",
            (UCHAR)uiSig, pdmsg->DTM_ulThread);                         /* 返回停止原因，默认为中断     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gdbNotfyStopReason
** 功能描述: non-stop模式上报停止原因
** 输　入  : pdmsg         中断信息
**           pcOutBuff     输出缓冲区
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbNotfyStopReason (PLW_DTRACE_MSG pdmsg, PCHAR pcOutBuff)
{
    UINT uiSig = SIGSEGV;

    if (pdmsg->DTM_uiType == LW_TRAP_QUIT) {                            /* 进程退出                     */
        sprintf(pcOutBuff, "W00");
        return  (ERROR_NONE);
    }

#ifdef GDB_NON_STOP_BUG
    API_TimeMSleep(10);                                                 /* 为解决客户端 bug，延时 10ms  */
#endif                                                                  /* GDB_NON_STOP_BUG             */

    uiSig = ((pdmsg->DTM_uiType == LW_TRAP_ABORT) ? SIGSEGV : SIGTRAP);
    sprintf(pcOutBuff, "Stop:T%02xthread:%lx;",
            (UCHAR)uiSig, pdmsg->DTM_ulThread);                         /* 返回停止原因，默认为中断     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gdbRemoveBP
** 功能描述: 移除断点
** 输　入  : pparam        gdb全局对象
**           cInBuff       rsp包，含地址和长度
** 输　出  : cOutBuff      rsp包，含内存数据
**           返回值        ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbRemoveBP (LW_GDB_PARAM *pparam, PCHAR pcInBuff, PCHAR pcOutBuff)
{
    INT                 iType;
    addr_t              addrBp = 0;
    UINT                uiLen  = 0;
    PLW_LIST_LINE       plineTemp;
    LW_GDB_BP          *pbpItem;

    if (sscanf(pcInBuff, "%d,%lx,%x",
               &iType, (ULONG *)&addrBp, &uiLen) != 3) {
        gdbReplyError(pcOutBuff, 0);
        return  (ERROR_NONE);
    }

    plineTemp = pparam->GDB_plistBpHead;                                /* 释放断点链表                 */
    while (plineTemp) {
        pbpItem  = _LIST_ENTRY(plineTemp, LW_GDB_BP, BP_plistBpLine);
        plineTemp = _list_line_get_next(plineTemp);
        if (addrBp == pbpItem->BP_addr) {                               /* 断点已存在                   */
            if (API_DtraceBreakpointRemove(pparam->GDB_pvDtrace,
                                           pbpItem->BP_addr,
                                           pbpItem->BP_size,
                                           pbpItem->BP_ulInstOrg) != ERROR_NONE) {
                gdbReplyError(pcOutBuff, 0);
                return  (ERROR_NONE);
            }
            _List_Line_Del(&pbpItem->BP_plistBpLine, &pparam->GDB_plistBpHead);
            LW_GDB_SAFEFREE(pbpItem);
            return  (ERROR_NONE);
        }
    }

    gdbReplyOk(pcOutBuff);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gdbInsertBP
** 功能描述: 移除断点
** 输　入  : pparam        gdb全局对象
**           cInBuff       寄存器描述，rsp包格式
** 输　出  : cOutBuff      寄存器值，rsp包格式
**           返回值        ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbInsertBP (LW_GDB_PARAM *pparam, PCHAR pcInBuff, PCHAR pcOutBuff)
{
    INT                 iType;
    addr_t              addrBp = 0;
    UINT                uiLen  = 0;
    PLW_LIST_LINE       plineTemp;
    LW_GDB_BP          *pbpItem;

    if (sscanf(pcInBuff, "%d,%lx,%x", &iType,
               (ULONG *)&addrBp, &uiLen) != 3) {
        gdbReplyError(pcOutBuff, 0);
        return  (ERROR_NONE);
    }

    plineTemp = pparam->GDB_plistBpHead;
    while (plineTemp) {
        pbpItem  = _LIST_ENTRY(plineTemp, LW_GDB_BP, BP_plistBpLine);
        plineTemp = _list_line_get_next(plineTemp);
        if (addrBp == pbpItem->BP_addr) {                               /* 断点已存在                   */
            return  (ERROR_NONE);
        }
    }

    pbpItem = (LW_GDB_BP*)LW_GDB_SAFEMALLOC(sizeof(LW_GDB_BP));
    if (LW_NULL == pbpItem) {
        gdbReplyError(pcOutBuff, 0);
        return  (ERROR_NONE);
    }

    pbpItem->BP_addr = addrBp;
    pbpItem->BP_size = (uiLen != 0 ? uiLen : (LW_CFG_CPU_WORD_LENGHT / 8));
    if (API_DtraceBreakpointInsert(pparam->GDB_pvDtrace,
                                   pbpItem->BP_addr,
                                   pbpItem->BP_size,
                                   &pbpItem->BP_ulInstOrg) != ERROR_NONE) {
        LW_GDB_SAFEFREE(pbpItem);
        gdbReplyError(pcOutBuff, 0);
        return  (ERROR_NONE);
    }

    _List_Line_Add_Ahead(&pbpItem->BP_plistBpLine, 
                         &pparam->GDB_plistBpHead);                     /*  加入断点链表                */

    gdbReplyOk(pcOutBuff);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gdbIsBP
** 功能描述: 移除断点
** 输　入  : pparam        gdb全局对象
** 输　出  : adddrPc       地址
**           返回值        ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL gdbIsBP (LW_GDB_PARAM *pparam, addr_t adddrPc)
{
    PLW_LIST_LINE       plineTemp;
    LW_GDB_BP          *pbpItem;

    plineTemp = pparam->GDB_plistBpHead;
    while (plineTemp) {
        pbpItem  = _LIST_ENTRY(plineTemp, LW_GDB_BP, BP_plistBpLine);
        plineTemp = _list_line_get_next(plineTemp);
        if (adddrPc == pbpItem->BP_addr) {
            return  (LW_TRUE);
        }
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: gdbRegGet
** 功能描述: 获取寄存器值
** 输　入  : iTid          线程id
**           cInBuff       寄存器描述，rsp包格式
** 输　出  : cOutBuff      寄存器值，rsp包格式
**           返回值        ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbRegsGet (LW_GDB_PARAM     *pparam,
                       PLW_DTRACE_MSG    pdmsg,
                       PCHAR             pcInBuff,
                       PCHAR             pcOutBuff)
{
    INT         i;
    PCHAR       pcPos;
    GDB_REG_SET regset;
    ULONG       ulThreadId;

    if ((pparam->GDB_lOpGThreadId == 0) || (pparam->GDB_lOpGThreadId == -1)) {
        ulThreadId = pdmsg->DTM_ulThread;
    } else {
        ulThreadId = pparam->GDB_lOpGThreadId;
    }

    pcPos = pcOutBuff;

    archGdbRegsGet(pparam->GDB_pvDtrace, ulThreadId, &regset);

    for (i = 0; i < regset.GDBR_iRegCnt; i++) {
        gdbReg2Asc(pcPos, regset.regArr[i].GDBRA_ulValue);
        pcPos += (2 * sizeof(ULONG));
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gdbRegSet
** 功能描述: 设置寄存器值
** 输　入  : iTid          线程id
**           cInBuff       寄存器描述，rsp包格式
** 输　出  : cOutBuff      寄存器值，rsp包格式
**           返回值        ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbRegsSet (LW_GDB_PARAM     *pparam,
                       PLW_DTRACE_MSG    pdmsg,
                       PCHAR             pcInBuff,
                       PCHAR             pcOutBuff)
{
    PCHAR       pcPos;
    INT         i;
    INT         iLen;
    GDB_REG_SET regset;
    ULONG       ulThreadId;

    if ((pparam->GDB_lOpGThreadId == 0) || (pparam->GDB_lOpGThreadId == -1)) {
        ulThreadId = pdmsg->DTM_ulThread;
    } else {
        ulThreadId = pparam->GDB_lOpGThreadId;
    }

    iLen = lib_strlen(pcInBuff);

    archGdbRegsGet(pparam->GDB_pvDtrace, ulThreadId, &regset);

    pcPos = pcInBuff;
    for (i = 0; i < regset.GDBR_iRegCnt && iLen >= (i + 1) * 8; i++) {
        regset.regArr[i].GDBRA_ulValue = gdbAscToReg(pcPos);
        pcPos += (2 * sizeof(ULONG));
    }

    archGdbRegsSet(pparam->GDB_pvDtrace, ulThreadId, &regset);

    gdbReplyOk(pcOutBuff);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 ** 函数名称: gdbRegGet
 ** 功能描述: 获取寄存器值
 ** 输　入  : iTid          线程id
 **           cInBuff       寄存器描述，rsp包格式
 ** 输　出  : cOutBuff      寄存器值，rsp包格式
 **           返回值        ERROR_NONE 表示没有错误, PX_ERROR 表示错误
 ** 全局变量:
 ** 调用模块:
 *********************************************************************************************************/
static INT gdbRegGet (LW_GDB_PARAM      *pparam,
                      PLW_DTRACE_MSG     pdmsg,
                      PCHAR              pcInBuff,
                      PCHAR              pcOutBuff)
 {
     GDB_REG_SET regset;
     INT         iRegIdx;
     ULONG       ulThreadId;

     if ((pparam->GDB_lOpGThreadId == 0) || (pparam->GDB_lOpGThreadId == -1)) {
         ulThreadId = pdmsg->DTM_ulThread;
     } else {
         ulThreadId = pparam->GDB_lOpGThreadId;
     }

     if (sscanf(pcInBuff, "%x", &iRegIdx) != 1) {
         gdbReplyError(pcOutBuff, 0);
         return  (PX_ERROR);
     }

     archGdbRegsGet(pparam->GDB_pvDtrace, ulThreadId, &regset);

     gdbReg2Asc(pcOutBuff, regset.regArr[iRegIdx].GDBRA_ulValue);

     return  (ERROR_NONE);
 }
/*********************************************************************************************************
** 函数名称: gdbRegSet
** 功能描述: 设置寄存器值
** 输　入  : iTid          线程id
**           cInBuff       寄存器描述，rsp包格式
** 输　出  : cOutBuff      寄存器值，rsp包格式
**           返回值        ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbRegSet (LW_GDB_PARAM      *pparam,
                      PLW_DTRACE_MSG     pdmsg,
                      PCHAR              pcInBuff,
                      PCHAR              pcOutBuff)
 {
     GDB_REG_SET regset;
     INT         iRegIdx;
     INT         iPos;
     ULONG       ulThreadId;

     if ((pparam->GDB_lOpGThreadId == 0) || (pparam->GDB_lOpGThreadId == -1)) {
         ulThreadId = pdmsg->DTM_ulThread;
     } else {
         ulThreadId = pparam->GDB_lOpGThreadId;
     }

     iPos = -1;
     sscanf(pcInBuff, "%x=%n", &iRegIdx, &iPos);
     if (iPos == -1) {
         gdbReplyError(pcOutBuff, 0);
        return  (PX_ERROR);
     }

     archGdbRegsGet(pparam->GDB_pvDtrace, ulThreadId, &regset);

     regset.regArr[iRegIdx].GDBRA_ulValue = gdbAscToReg(pcInBuff + iPos);

     archGdbRegsSet(pparam->GDB_pvDtrace, ulThreadId, &regset);

     return  (ERROR_NONE);
 }
/*********************************************************************************************************
** 函数名称: gdbMemGet
** 功能描述: 设置寄存器值
** 输　入  : cInBuff       rsp包，含地址和长度
** 输　出  : cOutBuff      rsp包，含内存数据
**           返回值        ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbMemGet (LW_GDB_PARAM *pparam, PCHAR pcInBuff, PCHAR pcOutBuff)
{
    ULONG   ulAddr  = 0;
    UINT    uiLen   = 0;
    INT     i;
    PBYTE   pbyBuff = NULL;

    if (sscanf(pcInBuff, "%lx,%x", &ulAddr, &uiLen) != 2) {
        gdbReplyError(pcOutBuff, 0);
        return  (ERROR_NONE);
    }

    if (uiLen > (GDB_RSP_MAX_LEN - 16) / 2) {
        gdbReplyError(pcOutBuff, 1);
        return  (ERROR_NONE);
    }

    pbyBuff  = (PBYTE)LW_GDB_SAFEMALLOC(uiLen);                         /* 分配内存                     */
    if (NULL == pbyBuff) {
        gdbReplyError(pcOutBuff, 20);
        return  (PX_ERROR);
    }

    if (API_DtraceGetMems(pparam->GDB_pvDtrace, ulAddr,
                          pbyBuff, uiLen) != (ERROR_NONE)) {            /* 读内存                       */
        LW_GDB_SAFEFREE(pbyBuff);
        gdbReplyError(pcOutBuff, 20);
        return  (ERROR_NONE);
    }

    for (i = 0; i < uiLen; i++) {
        gdbByte2Asc(pcOutBuff + i * 2, pbyBuff[i]);
    }

    LW_GDB_SAFEFREE(pbyBuff);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gdbRegSet
** 功能描述: 设置寄存器值
** 输　入  : cInBuff       rsp包，含地址和长度
** 输　出  : cOutBuff      rsp包，含内存数据
**           返回值        ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbMemSet (LW_GDB_PARAM *pparam, PCHAR pcInBuff, PCHAR pcOutBuff)
{
    ULONG   ulAddr  = 0;
    UINT    uiLen   = 0;
    INT     iPos;
    INT     i;
    PBYTE   pbyBuff = NULL;

    if ((sscanf(pcInBuff, "%lx,%x:%n", &ulAddr, &uiLen, &iPos) < 2) || (iPos == -1)) {
        gdbReplyError(pcOutBuff, 0);
        return  (ERROR_NONE);
    }

    pbyBuff  = (PBYTE)LW_GDB_SAFEMALLOC(uiLen);                         /* 分配内存                     */
    if (NULL == pbyBuff) {
        gdbReplyError(pcOutBuff, 20);
        return  (PX_ERROR);
    }

    for (i = 0; i < uiLen; i++) {
        pbyBuff[i] = gdbAscToByte(pcInBuff + iPos + i * 2);
    }

    if (API_DtraceSetMems(pparam->GDB_pvDtrace, ulAddr,
                          pbyBuff, uiLen) != (ERROR_NONE)) {            /* 写内存                       */
        LW_GDB_SAFEFREE(pbyBuff);
        gdbReplyError(pcOutBuff, 20);
        return  (ERROR_NONE);
    }

    gdbReplyOk(pcOutBuff);

    LW_GDB_SAFEFREE(pbyBuff);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gdbGoTo
** 功能描述: 恢复被调试程序执行
** 输　入  : pparam        gdb全局参数
**           iTid          线程id
**           cInBuff       rsp包，含开始执行的地址
** 输　出  : cOutBuff      rsp包，表示操作执行结果
**           返回值        ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbGoTo (LW_GDB_PARAM    *pparam,
                    PLW_DTRACE_MSG   pdmsg,
                    PCHAR            pcInBuff,
                    PCHAR            pcOutBuff,
                    INT              iBeStep)
{
    INT     iSig;
    ULONG   ulAddr = 0;

    if (sscanf(pcInBuff, "%x;%lx", &iSig, &ulAddr) != 2) {
        sscanf(pcInBuff, "%lx", &ulAddr);
    }

    if (ulAddr != 0) {
        archGdbRegSetPc(pparam->GDB_pvDtrace, pdmsg->DTM_ulThread, ulAddr);
    }

    if (iBeStep) {                                                      /* 单步，在下一条指令设置断点   */
        gdbSetStepMode(pparam, pdmsg->DTM_ulThread);
    }

    if (pparam->GDB_lOpCThreadId <= 0) {
        API_DtraceContinueProcess(pparam->GDB_pvDtrace);
    } else {
        API_DtraceContinueThread(pparam->GDB_pvDtrace,
                                 pparam->GDB_lOpCThreadId);             /* 当前线程停止计数减一         */
    }

    gdbReplyOk(pcOutBuff);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gdbGetElfOffset
** 功能描述: 获取进程重定位地址
** 输　入  : pid           进程id
**           pcModPath     模块路径
** 输　出  : addrText      text段地址
**           addrData      data段地址
**           addrBss       bss段地址
**           bSo           是否so文件
**           返回值        ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbGetElfOffset (pid_t   pid,
                            PCHAR   pcModPath,
                            addr_t *addrText,
                            addr_t *addrData,
                            addr_t *addrBss,
                            BOOL   *bSo)
{
    addr_t          addrBase;
    INT             iFd;
    INT             i;
    INT             iReadLen;

    Elf_Ehdr        ehdr;
    Elf_Shdr       *pshdr;
    Elf_Phdr       *pphdr;
    size_t          stHdSize;
    PCHAR           pcBuf    = NULL;
    PCHAR           pcShName = NULL;

    if (API_ModuleGetBase(pid, pcModPath, &addrBase, LW_NULL) != (ERROR_NONE)) {
        return  (PX_ERROR);
    }

    iFd = open(pcModPath, O_RDONLY);
    if (iFd < 0) {                                                      /*  打开文件                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Open file error.\r\n");
        return  (PX_ERROR);
    }

    if (read(iFd, &ehdr, sizeof(ehdr)) < sizeof(ehdr)) {                /*  读取ELF头                   */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Read elf header error.\r\n");
        return  (PX_ERROR);
    }

    if (ehdr.e_type == ET_REL) {                                        /*  ko 文件                     */
        (*bSo)   = LW_FALSE;
        stHdSize = ehdr.e_shentsize * ehdr.e_shnum;

        pcBuf = (PCHAR)LW_GDB_SAFEMALLOC(stHdSize);
        if (pcBuf == LW_NULL) {
            close(iFd);
            return  (PX_ERROR);
        }

        if (lseek(iFd, ehdr.e_shoff, SEEK_SET) < 0) {
            close(iFd);
            LW_GDB_SAFEFREE(pcBuf);
            return  (PX_ERROR);
        }

        if (read(iFd, pcBuf, stHdSize) < stHdSize) {
            close(iFd);
            LW_GDB_SAFEFREE(pcBuf);
            return  (PX_ERROR);
        }

        pshdr = (Elf_Shdr *)pcBuf;

        pcShName = (PCHAR)LW_GDB_SAFEMALLOC(pshdr[ehdr.e_shstrndx].sh_size);
        if (pcBuf == LW_NULL) {
            close(iFd);
            LW_GDB_SAFEFREE(pcBuf);
            return  (PX_ERROR);
        }

        if (lseek(iFd, pshdr[ehdr.e_shstrndx].sh_offset, SEEK_SET) < 0) {
            close(iFd);
            LW_GDB_SAFEFREE(pcBuf);
            LW_GDB_SAFEFREE(pcShName);
            return  (PX_ERROR);
        }

        iReadLen = read(iFd, pcShName, pshdr[ehdr.e_shstrndx].sh_size);
        if (iReadLen < pshdr[ehdr.e_shstrndx].sh_size) {
            close(iFd);
            LW_GDB_SAFEFREE(pcBuf);
            LW_GDB_SAFEFREE(pcShName);
            return  (PX_ERROR);
        }

        for (i = 0; i < ehdr.e_shnum; i++) {
            if (lib_strcmp(pcShName + pshdr[i].sh_name, ".text") == 0) {
                (*addrText) = addrBase + pshdr[i].sh_addr;
            }

            if (lib_strcmp(pcShName + pshdr[i].sh_name, ".data") == 0) {
                (*addrData) = addrBase + pshdr[i].sh_addr;
            }

            if (strcmp(pcShName + pshdr[i].sh_name, ".bss") == 0) {
                (*addrBss) = addrBase + pshdr[i].sh_addr;
            }
        }
    
    } else {                                                            /*  so 文件                     */
        (*bSo)   = LW_TRUE;
        stHdSize = ehdr.e_phentsize * ehdr.e_phnum;
        pcBuf    = (PCHAR)LW_GDB_SAFEMALLOC((stHdSize + ehdr.e_shentsize));

        if (LW_NULL == pcBuf) {
            close(iFd);
            LW_GDB_SAFEFREE(pcBuf);
            return  (PX_ERROR);
        }

        if (lseek(iFd, ehdr.e_phoff, SEEK_SET) < 0) {
            close(iFd);
            LW_GDB_SAFEFREE(pcBuf);
            return  (PX_ERROR);
        }

        if (read(iFd, pcBuf, stHdSize) < stHdSize) {
            close(iFd);
            LW_GDB_SAFEFREE(pcBuf);
            return  (PX_ERROR);
        }

        pphdr = (Elf_Phdr *)pcBuf;
        (*addrText) = 0;
        (*addrData) = 0;
        (*addrBss)  = 0;
        for (i = 0; i < ehdr.e_phnum; i++) {
            if (PT_LOAD != pphdr[i].p_type) {                           /*  只处理可加载段              */
                continue;
            }

            if ((*addrText) == 0) {                                     /*  第一个可加载段位代码段      */
                (*addrText) = addrBase + pphdr[i].p_vaddr;
                continue;
            }

            if ((*addrData) == 0) {                                     /*  第二个可加载段位数据段      */
                (*addrData) = addrBase + pphdr[i].p_vaddr;
                (*addrBss)  = addrBase + pphdr[i].p_vaddr;
                break;
            }
        }
    }

    close(iFd);
    LW_GDB_SAFEFREE(pcBuf);
    LW_GDB_SAFEFREE(pcShName);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gdbHandleQCmd
** 功能描述: 相应gdb Q命令
** 输　入  : pparam        gdb全局参数
**           cInBuff       rsp包，含开始执行的地址
** 输　出  : cOutBuff      rsp包，表示操作执行结果
**           返回值        ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbHandleQCmd (LW_GDB_PARAM *pparam, PCHAR pcInBuff, PCHAR pcOutBuff)
{
    if (lib_strstr(pcInBuff, "NonStop:1") == pcInBuff) {
#if LW_CFG_GDB_NON_STOP_EN > 0
        pparam->GDB_bNonStop = 1;
        gdbReplyOk(pcOutBuff);
#else
        /*
         * PowerPC 与 x64 在设置 NonStop 模式后，单步时 GDB 会拷贝下一条指令到某个地址执行，
         * 该方法在相对跳转的指令会有问题
         */
        pparam->GDB_bNonStop = 0;
        gdbReplyError(pcOutBuff, 0);
#endif

    } else if (lib_strstr(pcInBuff, "NonStop:0") == pcInBuff) {
        pparam->GDB_bNonStop = 0;
        gdbReplyOk(pcOutBuff);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gdbCmdQuery
** 功能描述: 相应gdb q命令
** 输　入  : pparam        gdb全局参数
**           cInBuff       rsp包，含开始执行的地址
** 输　出  : cOutBuff      rsp包，表示操作执行结果
**           返回值        ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbCmdQuery (LW_GDB_PARAM *pparam, PCHAR pcInBuff, PCHAR pcOutBuff)
{
    addr_t  addrText = 0;
    addr_t  addrData = 0;
    addr_t  addrBss  = 0;
    BOOL    bSo;

    UINT    i;
    UINT    uiStrLen = 0;
    ULONG   ulThreadId;
    CHAR    cThreadInfo[0x100] = {0};

    if (lib_strstr(pcInBuff, "Offsets") == pcInBuff) {                  /* 返回程序重定位信息           */
        if (gdbGetElfOffset(pparam->GDB_iPid, pparam->GDB_cProgPath,
                            &addrText, &addrData, &addrBss, &bSo) != ERROR_NONE) {
            pcOutBuff[0] = 0;
        }
        /*
         *  部分体系结构如mips需要返回64位的有符号偏移，而对于32位的体系结构如arm，gdb会自动截断.
         *  为扩展符号位将地址先转换成LONG再转换成INT64
         */
        if (addrText) {
            if (bSo) {
                sprintf(pcOutBuff, "TextSeg=%llx;", (INT64)(LONG)addrText);
                if (addrData) {
                    sprintf(pcOutBuff + lib_strlen(pcOutBuff), "DataSeg=%llx", (INT64)(LONG)addrData);
                }
            } else {
                sprintf(pcOutBuff, "Text=%llx;", (INT64)(LONG)addrText);
                if (addrData) {
                    sprintf(pcOutBuff + lib_strlen(pcOutBuff), "Data=%llx;", (INT64)(LONG)addrData);
                    sprintf(pcOutBuff + lib_strlen(pcOutBuff), "Bss=%llx", (INT64)(LONG)addrData);
                }
            }
        } else {
            pcOutBuff[0] = 0;
        }
    
    } else if (lib_strstr(pcInBuff, "Supported") == pcInBuff) {         /* 获取能力项                   */
#ifdef LW_CFG_CPU_ARCH_MIPS
        sprintf(pcOutBuff, "PacketSize=2000;"
                           "qXfer:libraries:read+;"
                           "QNonStop+");
#else
        sprintf(pcOutBuff, "PacketSize=2000;"
                           "qXfer:features:read+;"
                           "qXfer:libraries:read+;"
                           "QNonStop+");
#endif
    
    } else if (lib_strstr(pcInBuff, "Xfer:features:read:target.xml") == pcInBuff) {
        sprintf(pcOutBuff, archGdbTargetXml());
    
    } else if (lib_strstr(pcInBuff, "Xfer:features:read:arch-core.xml") == pcInBuff) {
        sprintf(pcOutBuff, archGdbCoreXml());
    
    } else if (lib_strstr(pcInBuff, "Xfer:libraries:read") == pcInBuff) {
        pcOutBuff[0] = 'l';
        if (vprocGetModsInfo(pparam->GDB_iPid, pcOutBuff + 1,
                             GDB_RSP_MAX_LEN - 4) <= 0) {
            gdbReplyError(pcOutBuff, 1);
            return  (PX_ERROR);
        }
    
    } else if (lib_strstr(pcInBuff, "fThreadInfo") == pcInBuff) {
        API_DtraceProcessThread(pparam->GDB_pvDtrace,
                                pparam->GDB_ulThreads,
                                GDB_MAX_THREAD_NUM,
                                &pparam->GDB_uiThdNum);                 /* 获取线程列表                 */
        if (pparam->GDB_uiThdNum <= 0) {
            gdbReplyError(pcOutBuff, 1);
            return  (PX_ERROR);
        }

        uiStrLen += sprintf(pcOutBuff, "m");
        for (i = 0; i < pparam->GDB_uiThdNum - 1; i++) {
            uiStrLen += sprintf(pcOutBuff + uiStrLen, "%lx,", pparam->GDB_ulThreads[i]);
        }

        uiStrLen += sprintf(pcOutBuff + uiStrLen,
                            "%lx", pparam->GDB_ulThreads[i]);           /* 最后一个线程不追加","        */
    
    } else if (lib_strstr(pcInBuff, "sThreadInfo") == pcInBuff) {
        sprintf(pcOutBuff, "l");
    
    } else if (lib_strstr(pcInBuff, "ThreadExtraInfo") == pcInBuff) {   /* 获取线程信息                 */
        if (sscanf(pcInBuff + lib_strlen("ThreadExtraInfo,"), "%lx", &ulThreadId) < 1) {
            gdbReplyError(pcOutBuff, 1);
            return  (PX_ERROR);
        }
        API_DtraceThreadExtraInfo(pparam->GDB_pvDtrace, ulThreadId,
                                  cThreadInfo, sizeof(cThreadInfo) - 1);
        for (i = 0; cThreadInfo[i] != 0; i++) {
            gdbByte2Asc(&pcOutBuff[i * 2], cThreadInfo[i]);
        }
    
    } else if (lib_strstr(pcInBuff, "C") == pcInBuff) {                 /* 获取当前线程                 */
        sprintf(pcOutBuff, "QC%lx", pparam->GDB_ulThreads[0]);

    } else {
        pcOutBuff[0] = 0;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gdbHcmdHandle
** 功能描述: 响应gdb H命令
** 输　入  : pparam        gdb全局参数
**           cInBuff       rsp包，含开始执行的地址
** 输　出  : cOutBuff      rsp包，表示操作执行结果
**           返回值        ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbHcmdHandle (LW_GDB_PARAM *pparam, PCHAR pcInBuff, PCHAR pcOutBuff)
{
    CHAR    cOp;
    LONG    lThread;

    cOp = pcInBuff[0];
    if (pcInBuff[1] == '-') {
        if (sscanf(pcInBuff + 2, "%lx", (ULONG*)&lThread) < 1) {
            gdbReplyError(pcOutBuff, 1);
            return  (PX_ERROR);
        }
        lThread = 0 - lThread;
    
    } else {
        if (sscanf(pcInBuff + 1, "%lx", (ULONG*)&lThread) < 1) {
            gdbReplyError(pcOutBuff, 1);
            return  (PX_ERROR);
        }
    }

    if (cOp == 'c') {
        if (lThread == -1) {                                            /* -1与0处理方式一致            */
            lThread = 0;
        }
        pparam->GDB_lOpCThreadId = lThread;
    
    } else if (cOp == 'g') {
        pparam->GDB_lOpGThreadId = lThread;
    
    } else {
        gdbReplyError(pcOutBuff, 1);
        return  (PX_ERROR);
    }

    gdbReplyOk(pcOutBuff);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gdbUpdateThreadList
** 功能描述: 更新线程列表
** 输　入  : pparam        gdb全局参数
**           返回值        ERROR_NONE 表示没有错误, PX_ERROR 表示错误，1表示跳出循环
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbUpdateThreadList (LW_GDB_PARAM  *pparam)
{
    LW_LIST_LINE_HEADER     plistThd = LW_NULL;
    PLW_LIST_LINE           plineTemp;
    LW_GDB_THREAD          *pthItem;
    INT                     i;

    API_DtraceProcessThread(pparam->GDB_pvDtrace,
                            pparam->GDB_ulThreads,
                            GDB_MAX_THREAD_NUM,
                            &pparam->GDB_uiThdNum);                     /* 重新获取线程信息             */

    plistThd = pparam->GDB_plistThd;

    pparam->GDB_plistThd = LW_NULL;

    for (i = 0; i < pparam->GDB_uiThdNum; i++) {
        plineTemp = plistThd;
        while (plineTemp) {
            pthItem  = _LIST_ENTRY(plineTemp, LW_GDB_THREAD, TH_plistThLine);
            if (pparam->GDB_ulThreads[i] == pthItem->TH_ulId) {         /* 线程存在                     */
                _List_Line_Del(&pthItem->TH_plistThLine, &plistThd);
                pthItem->TH_bChanged = LW_FALSE;
                _List_Line_Add_Ahead(&pthItem->TH_plistThLine,
                                     &pparam->GDB_plistThd);
                break;
            }
            plineTemp = _list_line_get_next(plineTemp);
        }

        if (!plineTemp) {                                               /* 新线程                       */
            pthItem = (LW_GDB_THREAD*)LW_GDB_SAFEMALLOC(sizeof(LW_GDB_THREAD));
            if (pthItem) {
                pthItem->TH_ulId = pparam->GDB_ulThreads[i];
                pthItem->TH_addrHigh = 0;
                pthItem->TH_addrLow  = 0;
                pthItem->TH_cStates  = '\0';
                pthItem->TH_bChanged = LW_FALSE;
                _List_Line_Add_Ahead(&pthItem->TH_plistThLine,
                                     &pparam->GDB_plistThd);
            } else {
                _DebugHandle(__ERRORMESSAGE_LEVEL, "Memory alloc failed.\r\n");
                /*
                 *  容忍分配失败，不会导致程序错误
                 */
            }
        }
    }

    while (plistThd) {                                                  /* 线程已结束                   */
        pthItem  = _LIST_ENTRY(plistThd, LW_GDB_THREAD, TH_plistThLine);
        _List_Line_Del(&pthItem->TH_plistThLine, &plistThd);
        LW_GDB_SAFEFREE(pthItem);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gdbContHandle
** 功能描述: 响应gdb vCont命令
** 输　入  : pparam        gdb全局参数
**           cInBuff       rsp包，含开始执行的地址
** 输　出  : cOutBuff      rsp包，表示操作执行结果
**           返回值        ERROR_NONE 表示没有错误, PX_ERROR 表示错误，1表示跳出循环
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbContHandle (LW_GDB_PARAM     *pparam,
                          PLW_DTRACE_MSG    pdmsg,
                          PCHAR             pcInBuff,
                          PCHAR             pcOutBuff)
{
    CHAR        chOp  = '\0';
    ULONG       ulTid = 0;
    INT         iSigNo;
    GDB_REG_SET regset;
    PCHAR       pcPos = pcInBuff;
    addr_t      addrRangeLow;
    addr_t      addrRangeHigh;

    PLW_LIST_LINE           plineTemp;
    LW_GDB_THREAD          *pthItem;

    gdbUpdateThreadList(pparam);

    pparam->GDB_lOpCThreadId = PX_ERROR;

    while (pcPos[0] && pcPos[0] != '#') {
        chOp  = pcPos[0];
        ulTid = 0;
        if ('S' == chOp || 'C' == chOp) {
            sscanf(pcPos + 1, "%02x:%lx", &iSigNo, &ulTid);

        } else if ('s' == chOp || 'c' == chOp) {
            sscanf(pcPos + 1, ":%lx", &ulTid);

        } else if ('t' == chOp) {
            sscanf(pcPos + 1, ":%lx", &ulTid);

        } else if ('r' == chOp) {
            sscanf(pcPos + 1, "%lx,%lx:%lx", &addrRangeLow,
                   &addrRangeHigh, &ulTid);

        } else {
            gdbReplyError(pcOutBuff, 1);
            return  (ERROR_NONE);
        }

        plineTemp = pparam->GDB_plistThd;
        while (plineTemp) {
            pthItem  = _LIST_ENTRY(plineTemp, LW_GDB_THREAD, TH_plistThLine);
            if (pthItem->TH_ulId == ulTid ||
                (0 == ulTid && pthItem->TH_bChanged == LW_FALSE)) {
                pthItem->TH_bChanged = LW_TRUE;
                pthItem->TH_cStates  = chOp;
                if ('r' == chOp) {
                    pthItem->TH_addrLow = addrRangeLow;
                    pthItem->TH_addrHigh = addrRangeHigh;
                } else {
                    pthItem->TH_addrLow  = 0;
                    pthItem->TH_addrHigh = 0;
                }
            }

            plineTemp = _list_line_get_next(plineTemp);
        }

        if (('t' != chOp) && (pparam->GDB_lOpCThreadId != 0)) {         /* 非non-stop中断时需停止进程   */
            pparam->GDB_lOpCThreadId = ulTid;
        }

        while((*pcPos) && (*pcPos) != ';') {
            pcPos++;
        }

        if ((*pcPos) == ';') {
            pcPos++;
        }
    }

    if (pparam->GDB_lOpCThreadId == PX_ERROR) {
        pparam->GDB_lOpCThreadId = 0;
    }

    plineTemp = pparam->GDB_plistThd;
    while (plineTemp) {
        pthItem  = _LIST_ENTRY(plineTemp, LW_GDB_THREAD, TH_plistThLine);
        if (pthItem->TH_bChanged == LW_FALSE) {                         /* 状态未改变                   */
            plineTemp = _list_line_get_next(plineTemp);
            continue;
        }

        archGdbRegsGet(pparam->GDB_pvDtrace, pthItem->TH_ulId, &regset);

        /*
         *  TODO:目前采取与linux相同的多线程处理策略，除断点外一律全速运行，所有后台线程单步断点
         *  被清除。
         */
        if (!gdbIsBP(pparam, archGdbRegGetPc(&regset))) {
            gdbRemoveStepBp(pparam, pthItem->TH_ulId);
        }
        gdbClearStepMode(pparam, pthItem->TH_ulId);

        switch (pthItem->TH_cStates) {
        
        case 'S':
        case 's':
        case 'r':
            gdbSetStepMode(pparam, pthItem->TH_ulId);
                                                                        /* 不需要break                  */
        case 'C':
        case 'c':
            API_DtraceContinueThread(pparam->GDB_pvDtrace,
                                     pthItem->TH_ulId);
            break;

        case 't':
            API_DtraceStopThread(pparam->GDB_pvDtrace,
                                 pthItem->TH_ulId);
            pdmsg->DTM_uiType   = 0;
            pdmsg->DTM_ulAddr   = 0;
            pdmsg->DTM_ulThread = pthItem->TH_ulId;
            pparam->GDB_lOptThreadId = pthItem->TH_ulId;
            if (pparam->GDB_bNonStop && !pparam->GDB_bStarted) {
                API_DtraceContinueProcess(pparam->GDB_pvDtrace);        /* non-stop先启动进程           */
                pparam->GDB_bStarted = LW_TRUE;
            }
            break;

        default:
            break;
        }

        plineTemp = _list_line_get_next(plineTemp);
    }

    gdbReplyOk(pcOutBuff);

    return  (pparam->GDB_bNonStop ? (ERROR_NONE): 1);                   /* non-stop模式则命令循环不退出 */
}
/*********************************************************************************************************
** 函数名称: gdbHcmdHandle
** 功能描述: 响应gdb v命令
** 输　入  : pparam        gdb全局参数
**           cInBuff       rsp包，含开始执行的地址
** 输　出  : cOutBuff      rsp包，表示操作执行结果
**           返回值        ERROR_NONE 表示没有错误, PX_ERROR 表示错误, 1表示
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbVcmdHandle (LW_GDB_PARAM     *pparam,
                          PLW_DTRACE_MSG    pdmsg,
                          PCHAR             pcInBuff,
                          PCHAR             pcOutBuff)
{
    PLW_LIST_LINE   plineTemp;
    LW_GDB_THREAD  *pthItem;

    if (lib_strstr(pcInBuff, "Cont?") == pcInBuff) {                    /* 支持的vCont命令列表          */
        if (pparam->GDB_bNonStop) {
            lib_strncpy(pcOutBuff, "vCont;c;C;s;S;t", GDB_RSP_MAX_LEN); /* nonstop模式暂不支持steprange */
        } else {
            lib_strncpy(pcOutBuff, "vCont;c;C;s;S;t,r", GDB_RSP_MAX_LEN);
        }

    } else if (lib_strstr(pcInBuff, "Cont;") == pcInBuff) {             /* vCont命令处理                */
        return gdbContHandle(pparam,
                             pdmsg,
                             pcInBuff + lib_strlen("Cont;"),
                             pcOutBuff);

    } else if (lib_strstr(pcInBuff, "Stopped") == pcInBuff) {           /* 查询下一个断点线程           */
        if (API_DtraceGetBreakInfo(pparam->GDB_pvDtrace,
                                   pdmsg, 0) == ERROR_NONE) {
            if (gdbIsStepBp(pparam, pdmsg)) {
                gdbClearStepMode(pparam, pdmsg->DTM_ulThread);
            }

            gdbUpdateThreadList(pparam);
            plineTemp = pparam->GDB_plistThd;
            while (plineTemp) {                                         /* 记录线程状态                 */
                pthItem  = _LIST_ENTRY(plineTemp, LW_GDB_THREAD, TH_plistThLine);
                if (pthItem->TH_ulId == pdmsg->DTM_ulThread) {
                    pthItem->TH_cStates = 'b';
                }
                plineTemp = _list_line_get_next(plineTemp);
            }

            gdbReportStopReason(pdmsg, pcOutBuff);
            return  (ERROR_NONE);
        }

        pparam->GDB_beNotifing = 0;                                     /* notify处理完毕               */
        gdbReplyOk(pcOutBuff);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gdbCheckThread
** 功能描述: 响应gdb T命令，查询线程状态
** 输　入  : pparam        gdb全局参数
**           cInBuff       rsp包，含开始执行的地址
** 输　出  : cOutBuff      rsp包，表示操作执行结果
**           返回值        ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbCheckThread (LW_GDB_PARAM *pparam, PCHAR pcInBuff, PCHAR pcOutBuff)
{
    ULONG   ulThread;
    
    if (sscanf(pcInBuff, "%lx", (ULONG*)&ulThread) < 1) {
        gdbReplyError(pcOutBuff, 1);
        return  (PX_ERROR);
    }

    if (API_ThreadVerify(ulThread)) {
        gdbReplyOk(pcOutBuff);
        return  (ERROR_NONE);
    
    } else {
        gdbReplyError(pcOutBuff, 10);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: gdbRspPkgHandle
** 功能描述: 处理rsp协议包，完成与gdb的交互
** 输　入  : pparam     系统参数
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbRspPkgHandle (LW_GDB_PARAM    *pparam,
                            PLW_DTRACE_MSG   pdmsg,
                            BOOL             bNeedReport)
{
    struct signalfd_siginfo  fdsi;
    INT                      iRet;
    CHAR                    *cInBuff   = LW_NULL;
    CHAR                    *cOutBuff  = LW_NULL;

    PLW_LIST_LINE            plineTemp;
    LW_GDB_THREAD           *pthItem;

    pparam->GDB_lOpGThreadId = 0;                                       /* 每次进去循环前复位Hg的值     */

    cInBuff  = (CHAR *)LW_GDB_SAFEMALLOC(GDB_RSP_MAX_LEN);
    cOutBuff = (CHAR *)LW_GDB_SAFEMALLOC(GDB_RSP_MAX_LEN);
    if (!cInBuff || !cOutBuff) {
        LW_GDB_SAFEFREE(cInBuff);
        LW_GDB_SAFEFREE(cOutBuff);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Memory alloc failed.\r\n");
        return  (PX_ERROR);
    }

    if (bNeedReport) {                                                  /* 上报停止原因                 */
        gdbReportStopReason(pdmsg, cOutBuff);
        if (gdbRspPkgPut(pparam, cOutBuff, LW_FALSE) < 0) {
            LW_GDB_SAFEFREE(cInBuff);
            LW_GDB_SAFEFREE(cOutBuff);
            LW_GDB_MSG("[GDB]Host close.\n");
            return  (PX_ERROR);
        
        } else {
            if (pdmsg->DTM_uiType == LW_TRAP_QUIT &&
                pparam->GDB_byCommType == COMM_TYPE_TTY) {              /* 串口模式不再等待客户端通知   */
                LW_GDB_SAFEFREE(cInBuff);
                LW_GDB_SAFEFREE(cOutBuff);
                LW_GDB_MSG("[GDB]Host close.\n");
                return  (PX_ERROR);
            }
        }
    }

    while (1) {
        lib_bzero(cInBuff, GDB_RSP_MAX_LEN);
        lib_bzero(cOutBuff, GDB_RSP_MAX_LEN);
        lib_bzero(&fdsi, sizeof(fdsi));

        if (pparam->GDB_lOptThreadId != 0) {                            /* 上个命令有停线程,如: vCont;t */
            gdbNotfyStopReason(pdmsg, cOutBuff);
            gdbRspPkgPut(pparam, cOutBuff, LW_TRUE);
            pparam->GDB_lOptThreadId = 0;
            pparam->GDB_beNotifing = 1;
        }

        iRet = gdbRspPkgGet(pparam, cInBuff, &fdsi);
        if (iRet < 0) {
            LW_GDB_SAFEFREE(cInBuff);
            LW_GDB_SAFEFREE(cOutBuff);
            LW_GDB_MSG("[GDB]Host close.\n");
            return  (PX_ERROR);
        }

        if (fdsi.ssi_signo == SIGTRAP) {                                /* non-stop异步上报             */
            if (pparam->GDB_beNotifing == 1) {                          /* 还在处理上一个notify         */
                continue;
            }

            if (API_DtraceGetBreakInfo(pparam->GDB_pvDtrace,
                                       pdmsg, 0) == ERROR_NONE) {
                if (gdbIsStepBp(pparam, pdmsg)) {
                    gdbClearStepMode(pparam, pdmsg->DTM_ulThread);
#if defined(LW_CFG_CPU_ARCH_X86) && (LW_CFG_CPU_WORD_LENGHT == 32)
                    /*
                     *  x86单步时需跳过 push %ebp (0x55) 和 mov %esp %ebp指令，
                     *  因为当停在这两条指令是可能导致gdb堆栈错误
                     */
                    if ((*(PUCHAR)pdmsg->DTM_ulAddr) == 0x55 ||
                        (*(PUCHAR)pdmsg->DTM_ulAddr) == 0x89) {
                    	gdbSetStepMode(pparam, pdmsg->DTM_ulThread);
                        API_DtraceContinueThread(pparam->GDB_pvDtrace, pdmsg->DTM_ulThread);
                        continue;
                    }
#endif
                }

                gdbUpdateThreadList(pparam);
                plineTemp = pparam->GDB_plistThd;
                while (plineTemp) {                                     /* 记录线程状态                 */
                    pthItem  = _LIST_ENTRY(plineTemp, LW_GDB_THREAD, TH_plistThLine);
                    if (pthItem->TH_ulId == pdmsg->DTM_ulThread) {
                        pthItem->TH_cStates = 'b';
                    }
                    plineTemp = _list_line_get_next(plineTemp);
                }

                pparam->GDB_beNotifing = 1;
                gdbNotfyStopReason(pdmsg, cOutBuff);
                gdbRspPkgPut(pparam, cOutBuff, LW_TRUE);
                continue;
            }

        } else if ((fdsi.ssi_signo == SIGCHLD) &&
                    (fdsi.ssi_code == CLD_EXITED)) {                    /* 进程退出                     */
            pdmsg->DTM_ulAddr   = 0;
            pdmsg->DTM_ulThread = LW_OBJECT_HANDLE_INVALID;
            pdmsg->DTM_uiType   = LW_TRAP_QUIT;
            pparam->GDB_bExited = LW_TRUE;
            gdbNotfyStopReason(pdmsg, cOutBuff);
            gdbRspPkgPut(pparam, cOutBuff, LW_FALSE);
            LW_GDB_SAFEFREE(cInBuff);
            LW_GDB_SAFEFREE(cOutBuff);
            LW_GDB_MSG("[GDB]Target process exit.\n");
            return  (PX_ERROR);
        }

        switch (cInBuff[0]) {
        
        case 'g':
            gdbRegsGet(pparam, pdmsg, cInBuff + 1, cOutBuff);
            break;
        
        case 'G':
            gdbRegsSet(pparam, pdmsg, cInBuff + 1, cOutBuff);
            break;
        
        case 'p':
            gdbRegGet(pparam, pdmsg, cInBuff + 1, cOutBuff);
            break;
        
        case 'P':
            gdbRegSet(pparam, pdmsg, cInBuff + 1, cOutBuff);
            break;
        
        case 'm':
            gdbMemGet(pparam, cInBuff + 1, cOutBuff);
            break;
        
        case 'M':
            gdbMemSet(pparam, cInBuff + 1, cOutBuff);
            break;
        
        case 'c':
            gdbGoTo(pparam, pdmsg, cInBuff+1, cOutBuff, 0);
            LW_GDB_SAFEFREE(cInBuff);
            LW_GDB_SAFEFREE(cOutBuff);
            return  (ERROR_NONE);
        
        case 'q':
            gdbCmdQuery(pparam, cInBuff + 1, cOutBuff);
            break;
        
        case 'Q':
            gdbHandleQCmd(pparam, cInBuff + 1, cOutBuff);
            break;
        
        case '?':
            if (pparam->GDB_bNonStop &&
                API_DtraceGetBreakInfo(pparam->GDB_pvDtrace,
                                       pdmsg, 0) == ERROR_NONE) {
                if (gdbIsStepBp(pparam, pdmsg)) {
                    gdbClearStepMode(pparam, pdmsg->DTM_ulThread);
                }

                gdbUpdateThreadList(pparam);
                plineTemp = pparam->GDB_plistThd;
                while (plineTemp) {                                     /* 记录线程状态                 */
                    pthItem  = _LIST_ENTRY(plineTemp, LW_GDB_THREAD, TH_plistThLine);

                    if (pthItem->TH_ulId == pdmsg->DTM_ulThread) {
                        pthItem->TH_cStates = 'b';
                    }

                    plineTemp = _list_line_get_next(plineTemp);
                }

            }

            gdbReportStopReason(pdmsg, cOutBuff);
            break;
        
        case 'H':
            gdbHcmdHandle(pparam, cInBuff + 1, cOutBuff);
            break;
        
        case 'T':
            gdbCheckThread(pparam, cInBuff + 1, cOutBuff);
            break;
        
        case 's':
            gdbGoTo(pparam, pdmsg, cInBuff + 1, cOutBuff, 1);
            LW_GDB_SAFEFREE(cInBuff);
            LW_GDB_SAFEFREE(cOutBuff);
            return  (ERROR_NONE);
            break;

#if !defined(LW_CFG_CPU_ARCH_C6X)
        case 'z':
            gdbRemoveBP(pparam, cInBuff + 1, cOutBuff);
            break;

        case 'Z':
            gdbInsertBP(pparam, cInBuff + 1, cOutBuff);
            break;
#endif

        case 'v':
            if (gdbVcmdHandle(pparam, pdmsg, cInBuff + 1, cOutBuff) > 0) {
                LW_GDB_SAFEFREE(cInBuff);
                LW_GDB_SAFEFREE(cOutBuff);
                return  (ERROR_NONE);
            }
            break;

        case 'k':
            LW_GDB_SAFEFREE(cInBuff);
            LW_GDB_SAFEFREE(cOutBuff);
            LW_GDB_MSG("[GDB]Host stoped.\n");
            return  (PX_ERROR);

        default:
            cOutBuff[0] = 0;
            break;
        }

        if (gdbRspPkgPut(pparam, cOutBuff, LW_FALSE) < 0) {
            LW_GDB_SAFEFREE(cInBuff);
            LW_GDB_SAFEFREE(cOutBuff);
            LW_GDB_MSG("[GDB]Host close.\n");
            return  (PX_ERROR);
        }
    }

    LW_GDB_SAFEFREE(cInBuff);
    LW_GDB_SAFEFREE(cOutBuff);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gdbWaitGdbSig
** 功能描述: 事件循环，侦听系统消息，检测并处理断点
** 输　入  : pparam       系统参数
** 输　出  : bClientSig   是否客户端信号
**           pfdsi        接受到的信号信息
**           返回         ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbWaitSig (LW_GDB_PARAM            *pparam,
                       BOOL                    *pbClientSig,
                       struct signalfd_siginfo *pfdsi)
{
    fd_set      fdset;
    ssize_t     szLen;
    INT         iMaxFd;
    CHAR        chCmd;

    (*pbClientSig) = LW_FALSE;

    FD_ZERO(&fdset);
    FD_SET(pparam->GDB_iCommFd, &fdset);
    FD_SET(pparam->GDB_iSigFd, &fdset);
    iMaxFd = max(pparam->GDB_iCommFd, pparam->GDB_iSigFd);

    while (select(iMaxFd + 1, &fdset, NULL, NULL, NULL) >= 0) {
        if (FD_ISSET(pparam->GDB_iCommFd, &fdset)) {
            if (read(pparam->GDB_iCommFd,
                     &chCmd,
                     sizeof(chCmd)) <= 0) {                             /* 中断命令0x3                  */
                return  (PX_ERROR);
            }

            if (chCmd == 0x3) {
                (*pbClientSig) = LW_TRUE;
                return  (ERROR_NONE);
            }
        }

        if (FD_ISSET(pparam->GDB_iSigFd, &fdset)) {
            szLen = read(pparam->GDB_iSigFd,
                         pfdsi,
                         sizeof(struct signalfd_siginfo));              /* 中断                         */
            if (szLen != sizeof(struct signalfd_siginfo)) {
                goto    __re_select;
            }
            return  (ERROR_NONE);
        }

__re_select:
        FD_SET(pparam->GDB_iCommFd, &fdset);
        FD_SET(pparam->GDB_iSigFd, &fdset);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: gdbSigInit
** 功能描述: 初始化信号句柄
** 输　入  :
** 输　出  : signalfd句柄
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbSigInit (VOID)
{
    sigset_t    sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGTRAP);
    sigaddset(&sigset, SIGCHLD);

    if (sigprocmask(SIG_BLOCK, &sigset, NULL) < ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (signalfd(-1, &sigset, 0));
}
/*********************************************************************************************************
** 函数名称: gdbRelease
** 功能描述: 释放资源
** 输　入  : pparam     系统参数
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbRelease (LW_GDB_PARAM *pparam)
{
    pparam->GDB_plistBpHead = LW_NULL;

    if (pparam->GDB_pvDtrace != LW_NULL) {
        API_DtraceDelete(pparam->GDB_pvDtrace);
    }

    if (pparam->GDB_iSigFd >= 0) {
        close(pparam->GDB_iSigFd);
    }

    if (pparam->GDB_iCommFd >= 0) {
        if (pparam->GDB_bTerminal) {                                    /* 还原终端之前的通信模式       */
            if (pparam->GDB_spSerial.GDBSP_iHwOpt != SERIAL_HWOPT) {
                ioctl(pparam->GDB_iCommFd, SIO_HW_OPTS_SET, 
                      pparam->GDB_spSerial.GDBSP_iHwOpt);
            }
            if (pparam->GDB_spSerial.GDBSP_lBaud != SERIAL_BAUD) {
                ioctl(pparam->GDB_iCommFd, SIO_BAUD_SET,
                      pparam->GDB_spSerial.GDBSP_lBaud);
            }
            ioctl(pparam->GDB_iCommFd, FIOSETOPTIONS,
                  pparam->GDB_spSerial.GDBSP_iSioOpt);
            
        } else {
            close(pparam->GDB_iCommFd);
        }
    }

    if (pparam->GDB_iCommListenFd >= 0) {
        close(pparam->GDB_iCommListenFd);
    }

    LW_GDB_SAFEFREE(pparam);

    LW_GDB_MSG("[GDB]Server exit.\n");

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gdbReadEvent
** 功能描述: 读取事件信息
** 输　入  : pparam     系统参数
** 输　出  : pdmsg      事件信息
**           ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbReadEvent (LW_GDB_PARAM *pparam, PLW_DTRACE_MSG pdmsg)
{
    return  ((INT)API_DtraceGetBreakInfo(pparam->GDB_pvDtrace, pdmsg,
                                         pparam->GDB_lOpCThreadId));
}
/*********************************************************************************************************
 ** 函数名称: gdbInStepRange
 ** 功能描述: 判断是否区域单步
 ** 输　入  : pparam     系统参数
 ** 输　出  : pdmsg      事件信息
 **           LW_TRUE 是, LW_FALSE 否
 ** 全局变量:
 ** 调用模块:
 *********************************************************************************************************/
static BOOL gdbInStepRange (LW_GDB_PARAM *pparam, PLW_DTRACE_MSG pdmsg)
{
    PLW_LIST_LINE   plineTemp;
    LW_GDB_THREAD  *pthItem;

    if (gdbIsBP(pparam, pdmsg->DTM_ulAddr)) {
        return  (LW_FALSE);
    }

    plineTemp = pparam->GDB_plistThd;
    while (plineTemp) {
        pthItem  = _LIST_ENTRY(plineTemp, LW_GDB_THREAD, TH_plistThLine);
        if (pthItem->TH_ulId == pdmsg->DTM_ulThread) {
            if (pdmsg->DTM_ulAddr >= pthItem->TH_addrLow &&
                pdmsg->DTM_ulAddr < pthItem->TH_addrHigh) {
                return  (LW_TRUE);
            } else {
                pthItem->TH_addrLow  = 0;
                pthItem->TH_addrHigh = 0;
                return  (LW_FALSE);
            }
        }
        plineTemp = _list_line_get_next(plineTemp);
    }
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: gdbEventLoop
** 功能描述: 事件循环，侦听系统消息，检测并处理断点
** 输　入  : pparam     系统参数
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbEventLoop (LW_GDB_PARAM *pparam)
{
    LW_DTRACE_MSG           dmsg;
    BOOL                    bClientSig;
    struct signalfd_siginfo fdsi;

    dmsg.DTM_ulThread = vprocMainThread(pparam->GDB_iPid);
    dmsg.DTM_uiType   = 0;
    dmsg.DTM_ulAddr   = 0;

    if (gdbRspPkgHandle(pparam, &dmsg, LW_FALSE) == PX_ERROR) {         /* 初始化时进程处于暂停状态     */
        return  (ERROR_NONE);
    }

    /*
     *  以下程序只有当非non-stop模式才进入
     */
    while (gdbWaitSig(pparam, &bClientSig, &fdsi) == ERROR_NONE) {
        if (bClientSig) {                                               /* 客户端信号额外处理           */
            API_DtraceStopProcess(pparam->GDB_pvDtrace);                /* 停止进程                     */
            dmsg.DTM_ulThread = vprocMainThread(pparam->GDB_iPid);
            if (gdbRspPkgHandle(pparam, &dmsg, LW_TRUE) == PX_ERROR) {  /* 小于0表示客户端关闭连接      */
                return  (ERROR_NONE);
            }
        }

        if ((fdsi.ssi_signo == SIGCHLD) &&
            (fdsi.ssi_code == CLD_EXITED)) {                            /* 进程退出                     */
            dmsg.DTM_ulAddr     = 0;
            dmsg.DTM_ulThread   = LW_OBJECT_HANDLE_INVALID;
            dmsg.DTM_uiType     = LW_TRAP_QUIT;
            pparam->GDB_bExited = LW_TRUE;
            gdbRspPkgHandle(pparam, &dmsg, LW_TRUE);
            LW_GDB_MSG("[GDB]Target process exit.\n");
            return  (ERROR_NONE);
        }

        while (gdbReadEvent(pparam, &dmsg) == ERROR_NONE) {
            if (pparam->GDB_lOpCThreadId <= 0) {
                API_DtraceStopProcess(pparam->GDB_pvDtrace);            /* 停止进程                     */
                API_DtraceContinueThread(pparam->GDB_pvDtrace,
                                         dmsg.DTM_ulThread);            /* 当前线程停止计数减一         */
            }

            if (gdbIsStepBp(pparam, &dmsg)) {                           /* 自动移除单步断点             */
                gdbClearStepMode(pparam, dmsg.DTM_ulThread);
#if defined(LW_CFG_CPU_ARCH_X86) && (LW_CFG_CPU_WORD_LENGHT == 32)
                /*
                 *  x86单步时需跳过 push %ebp (0x55) 和 mov %esp %ebp指令，
                 *  因为当停在这两条指令是可能导致gdb堆栈错误
                 */
                if (gdbInStepRange(pparam, &dmsg)        ||
                    ((*(PUCHAR)dmsg.DTM_ulAddr) == 0x55) ||
                    ((*(PUCHAR)dmsg.DTM_ulAddr) == 0x89)) {
#else
                if (gdbInStepRange(pparam, &dmsg)) {
#endif
                    gdbSetStepMode(pparam, dmsg.DTM_ulThread);
                    if (pparam->GDB_lOpCThreadId <= 0) {
                        API_DtraceContinueProcess(pparam->GDB_pvDtrace);
                    } else {
                        API_DtraceContinueThread(pparam->GDB_pvDtrace, dmsg.DTM_ulThread);
                    }
                    continue;
                }
            }

            if (gdbRspPkgHandle(pparam, &dmsg, LW_TRUE) == PX_ERROR) {  /* 小于0表示客户端关闭连接      */
                return  (ERROR_NONE);
            }
        }
    }

    API_DtraceStopProcess(pparam->GDB_pvDtrace);                        /* 因网络断开退出gdb需先停进程  */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gdbWaitSpawmSig
** 功能描述: 等待加载完成信号
** 输　入  : iSigNo 信号值
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbWaitSpawmSig (INT iSigNo)
{
    sigset_t                sigset;
    INT                     iSigFd;
    ssize_t                 szLen;
    struct signalfd_siginfo fdsi;

    sigemptyset(&sigset);
    sigaddset(&sigset, iSigNo);

    if (sigprocmask(SIG_BLOCK, &sigset, NULL) < ERROR_NONE) {
        return  (PX_ERROR);
    }

    iSigFd = signalfd(-1, &sigset, 0);
    if (iSigFd < 0) {
        return  (PX_ERROR);
    }

    szLen = read(iSigFd, &fdsi, sizeof(struct signalfd_siginfo));
    if (szLen != sizeof(struct signalfd_siginfo)) {
        close(iSigFd);
        return  (PX_ERROR);
    }

    close(iSigFd);

    return  (fdsi.ssi_int);
}
/*********************************************************************************************************
** 函数名称: gdbExit
** 功能描述: 退出函数，SIGKILL 信号处理函数
** 输　入  : iSigNo 信号值
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID gdbExit (INT iSigNo)
{
    LW_GDB_PARAM       *pparam    = LW_NULL;
    PLW_LIST_LINE       plineTemp;
    LW_GDB_BP          *pbpItem;
    LW_GDB_THREAD      *pthItem;
    LW_DTRACE_MSG       dmsg;
    
#if LW_CFG_SMP_EN > 0
    cpu_set_t           cpuset;
#endif                                                                  /* LW_CFG_SMP_EN > 0            */

    pparam = (LW_GDB_PARAM*)API_ThreadGetNotePad(API_ThreadIdSelf(), 0);
                                                                        /* 获取调试器对象               */
    if (LW_NULL == pparam) {
        return;
    }

    plineTemp = pparam->GDB_plistBpHead;                                /* 释放断点链表                 */
    while (plineTemp) {
        pbpItem  = _LIST_ENTRY(plineTemp, LW_GDB_BP, BP_plistBpLine);
        plineTemp = _list_line_get_next(plineTemp);
        if (!pparam->GDB_bExited) {
            API_DtraceBreakpointRemove(pparam->GDB_pvDtrace,
                                       pbpItem->BP_addr,
                                       pbpItem->BP_size,
                                       pbpItem->BP_ulInstOrg);
        }
        LW_GDB_SAFEFREE(pbpItem);
    }
    pparam->GDB_plistBpHead = LW_NULL;

    /*
     *  释放线程链表，启动被gdb停止的线程
     */
    if (!pparam->GDB_bExited) {
        gdbUpdateThreadList(pparam);
    }

    plineTemp = pparam->GDB_plistThd;
    while (plineTemp) {
        pthItem  = _LIST_ENTRY(plineTemp, LW_GDB_THREAD, TH_plistThLine);

        if (!pparam->GDB_bExited) {
            gdbClearStepMode(pparam, pthItem->TH_ulId);

            if (pparam->GDB_bNonStop) {                                 /* nonstop模式需单独启动线程    */
                if (pthItem->TH_cStates == 't' ||
                    pthItem->TH_cStates == 'b') {
                    API_DtraceContinueThread(pparam->GDB_pvDtrace,
                                             pthItem->TH_ulId);
                }
            }
        }

        plineTemp = _list_line_get_next(plineTemp);
        LW_GDB_SAFEFREE(pthItem);
    }
    pparam->GDB_plistThd = LW_NULL;

    while (API_DtraceGetBreakInfo(pparam->GDB_pvDtrace,
                                  &dmsg, 0) == ERROR_NONE) {            /* 启动被断点停止的线程         */
        if (!pparam->GDB_bExited) {
            API_DtraceContinueThread(pparam->GDB_pvDtrace, dmsg.DTM_ulThread);
        }
    }

    if (!pparam->GDB_bAttached && !pparam->GDB_bExited) {               /* 如果不是attch则停止进程      */
        kill(pparam->GDB_iPid, SIGABRT);                                /* 强制进程停止                 */
        LW_GDB_MSG("[GDB]Warning: Process is killed (SIGABRT) by GDB server.\n"
                   "     Restart SylixOS is recommended!\n");
    }
#if LW_CFG_SMP_EN > 0
    else if (pparam->GDB_bLockCpu) {
        CPU_ZERO(&cpuset);
        sched_setaffinity(pparam->GDB_iPid, sizeof(cpu_set_t), &cpuset);/* 被调对象取消锁定 CPU         */
    }
#endif                                                                  /* LW_CFG_SMP_EN > 0            */

    if (!pparam->GDB_bNonStop && !pparam->GDB_bExited) {
        API_DtraceContinueProcess(pparam->GDB_pvDtrace);                /* 程序继续执行                 */
    }

    gdbRelease(pparam);

#ifndef LW_DTRACE_HW_ISTEP
    if (API_AtomicGet(&GHookRefCnt) > 0) {
        if (API_AtomicDec(&GHookRefCnt) == 0) {                         /* 最后一次调用完成时清除HOOK   */
            API_SystemHookDelete(API_DtraceSchedHook,
                                 LW_OPTION_THREAD_SWAP_HOOK);           /* 删除线程切换HOOK，           */
        }
    }
#endif                                                                  /* !LW_DTRACE_HW_ISTEP          */
}
/*********************************************************************************************************
** 函数名称: gdbMain
** 功能描述: gdb stub 主函数
** 输　入  : argc         参数个数
**           argv         参数表
** 输　出  : ERROR_NONE 表示没有错误, PX_ERROR 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT gdbMain (INT argc, CHAR **argv)
{
    LW_GDB_PARAM       *pparam    = LW_NULL;
    UINT32              ui32Ip    = INADDR_ANY;
    UINT16              usPort    = 0;
    INT                 iArgPos   = 1;
    INT                 i;
    INT                 iPid;
    struct sched_param  schedparam;
    posix_spawnattr_t   spawnattr;
    posix_spawnopt_t    spawnopt  = {0};
    PCHAR               pcSerial  = LW_NULL;
    
#if LW_CFG_SMP_EN > 0
    CHAR                cValue[10];
    cpu_set_t           cpuset;
#endif                                                                  /* #if LW_CFG_SMP_EN > 0        */

    if (argc < 3) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Parameter error.\r\n");
        _ErrorHandle(ERROR_GDB_PARAM);
        return  (PX_ERROR);
    }

    pparam = (LW_GDB_PARAM *)LW_GDB_SAFEMALLOC(sizeof(LW_GDB_PARAM));
    if (LW_NULL == pparam) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Malloc mem error.\r\n");
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    lib_bzero(pparam, sizeof(LW_GDB_PARAM));

    pparam->GDB_bTerminal     = LW_FALSE;
    pparam->GDB_iCommFd       = PX_ERROR;
    pparam->GDB_iCommListenFd = PX_ERROR;
    pparam->GDB_iSigFd        = PX_ERROR;

    if (lib_strcmp(argv[iArgPos], "--attach") == 0) {                   /* 切入到正在执行的进程调试     */
        iArgPos++;
        pparam->GDB_bAttached = LW_TRUE;
        
        if (argc < 4) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Parameter error.\r\n");
            _ErrorHandle(ERROR_GDB_PARAM);
            LW_GDB_SAFEFREE(pparam);
            return  (PX_ERROR);
        }
    }

    pparam->GDB_byCommType = COMM_TYPE_TTY;
    for (i = 0; argv[iArgPos][i] != '\0'; i++) {                        /* 解析ip和端口                 */
        if (argv[iArgPos][i] == ':') {
            argv[iArgPos][i] = '\0';
            sscanf(argv[iArgPos] + i + 1, "%hu", &usPort);
            if (lib_strcmp(argv[iArgPos], "localhost") != 0) {
                ui32Ip = inet_addr(argv[iArgPos]);
                if (IPADDR_NONE == ui32Ip) {
                    ui32Ip = INADDR_ANY;
                }
            } else {
                ui32Ip = INADDR_ANY;
            }
            pparam->GDB_byCommType = COMM_TYPE_TCP;
        }
    }
    
    if (pparam->GDB_byCommType == COMM_TYPE_TTY) {                      /* 串口调试                     */
        pcSerial = argv[iArgPos];
    }

    iArgPos++;

    if ((pparam->GDB_iSigFd = gdbSigInit()) < 0) {                      /* 初始化调试信号               */
        gdbRelease(pparam);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Initialize signal failed.\r\n");
        _ErrorHandle(ERROR_GDB_INIT_SOCK);
        return  (PX_ERROR);
    }

    pparam->GDB_pvDtrace = API_DtraceCreate(LW_DTRACE_PROCESS, LW_DTRACE_F_DEF,
                                            API_ThreadIdSelf());        /*  创建dtrace对象              */
    if (pparam->GDB_pvDtrace == NULL) {
        gdbRelease(pparam);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Create dtrace object error.\r\n");
        _ErrorHandle(ERROR_GDB_PARAM);
        return  (PX_ERROR);
    }
    
#if LW_CFG_SMP_EN > 0
    CPU_ZERO(&cpuset);
    if (API_TShellVarGetRt("DEBUG_CPU", cValue, sizeof(cValue)) > 0) {
        INT    iCpu = lib_atoi(cValue);
        if ((iCpu >= 0) && (iCpu < LW_NCPUS)) {
            CPU_SET(iCpu, &cpuset);
            pparam->GDB_bLockCpu = LW_TRUE;
        }
    }
#endif                                                                  /* LW_CFG_SMP_EN > 0            */

    if (pparam->GDB_bAttached) {                                        /* attach到现有进程             */
        if (sscanf(argv[iArgPos], "%d", &iPid) != 1) {
            gdbRelease(pparam);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Parameter error.\r\n");
            _ErrorHandle(ERROR_GDB_PARAM);
            return  (PX_ERROR);
        }

        pparam->GDB_iPid = iPid;
        if (API_DtraceSetPid(pparam->GDB_pvDtrace, iPid) == PX_ERROR) { /* 关联进程                     */
            gdbRelease(pparam);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Attach to program failed.\r\n");
            _ErrorHandle(ERROR_GDB_ATTACH_PROG);
            return  (PX_ERROR);
        }

        if (API_DtraceStopProcess(pparam->GDB_pvDtrace) == PX_ERROR) {  /* 停止进程                     */
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Stop program failed.\r\n");
            _ErrorHandle(ERROR_GDB_ATTACH_PROG);
            return  (PX_ERROR);
        }
    
    } else {                                                            /* 启动程序并attach             */
        posix_spawnattr_init(&spawnattr);
        
        spawnopt.SPO_iSigNo       = SIGUSR1;
        spawnopt.SPO_ulId         = API_ThreadIdSelf();
        spawnopt.SPO_ulMainOption = LW_OPTION_DEFAULT;
        spawnopt.SPO_stStackSize  = 0;
        posix_spawnattr_setopt(&spawnattr, &spawnopt);
        
        schedparam.sched_priority = PX_PRIORITY_CONVERT(LW_PRIO_NORMAL);/*  新建进程优先级为 NORMAL     */
        posix_spawnattr_setschedparam(&spawnattr, &schedparam);
        posix_spawnattr_setflags(&spawnattr, POSIX_SPAWN_SETSCHEDPARAM);
        
        if (posix_spawnp(&pparam->GDB_iPid, argv[iArgPos], NULL,
                         &spawnattr, &argv[iArgPos], NULL) != ERROR_NONE) {
            posix_spawnattr_destroy(&spawnattr);
            gdbRelease(pparam);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Start program failed.\r\n");
            _ErrorHandle(ERROR_GDB_ATTACH_PROG);
            return  (PX_ERROR);
        
        } else {
            posix_spawnattr_destroy(&spawnattr);
        }

        if (API_DtraceSetPid(pparam->GDB_pvDtrace,
                             pparam->GDB_iPid) == PX_ERROR) {           /* 关联进程                     */
            gdbRelease(pparam);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Attach to program failed.\r\n");
            _ErrorHandle(ERROR_GDB_ATTACH_PROG);
            return  (PX_ERROR);
        }

        if (gdbWaitSpawmSig(spawnopt.SPO_iSigNo) == PX_ERROR) {         /* 等待加载就绪信号             */
            gdbRelease(pparam);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "Load program failed.\r\n");
            return  (PX_ERROR);
        }
    }
    
#if LW_CFG_SMP_EN > 0
    if (pparam->GDB_bLockCpu) {
        sched_setaffinity(pparam->GDB_iPid, sizeof(cpu_set_t), &cpuset);
        API_ThreadSetAffinity(API_ThreadIdSelf(), sizeof(cpu_set_t), &cpuset);
    }
#endif                                                                  /* LW_CFG_SMP_EN > 0            */

    if (vprocGetPath(pparam->GDB_iPid, 
                     pparam->GDB_cProgPath, 
                     MAX_FILENAME_LENGTH)) {
        gdbRelease(pparam);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Get process path fail.\r\n");
        return  (PX_ERROR);
    }

    if (API_ThreadSetNotePad(API_ThreadIdSelf(), 0, (ULONG)pparam) != ERROR_NONE) {
                                                                        /* 保存调试器对象到线程上下文   */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Set thread notepad failed.\r\n");
        gdbRelease(pparam);
        return  (PX_ERROR);
    }

    if (signal(SIGKILL, gdbExit) == SIG_ERR) {                          /* 注册SIGKILL信号处理函数      */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Set SIGKILL handle failed.\r\n");
        gdbRelease(pparam);
        return  (PX_ERROR);
    }

#ifndef LW_DTRACE_HW_ISTEP
    if (API_AtomicInc(&GHookRefCnt) == 1) {                             /* 第一次调用时添加HOOK         */
        API_SystemHookAdd(API_DtraceSchedHook,
                          LW_OPTION_THREAD_SWAP_HOOK);                  /* 添加线程切换HOOK，用于单步   */
    }
#endif                                                                  /* !LW_DTRACE_HW_ISTEP          */

    if (pparam->GDB_byCommType == COMM_TYPE_TCP) {
        pparam->GDB_iCommFd = gdbTcpSockInit(pparam, ui32Ip, usPort);   /* 初始化socket，获取连接句柄   */
    } else {
        pparam->GDB_iCommFd = gdbSerialInit(pparam, pcSerial);
    }
        
    if (pparam->GDB_iCommFd < 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Initialize comunication failed.\r\n");
        _ErrorHandle(ERROR_GDB_INIT_SOCK);
        gdbExit(0);
        return  (PX_ERROR);
    }

    gdbEventLoop(pparam);                                               /* 进入事件循环                 */

    gdbExit(0);                                                         /* 销毁对象                     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: gdbInit
** 功能描述: 注册 GDBServer 命令
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
VOID  API_GdbInit (VOID)
{
    API_TShellKeywordAddEx("debug", gdbMain, LW_OPTION_KEYWORD_SYNCBG);
    API_TShellFormatAdd("debug", " [connect options] [program] [arguments...]");
    API_TShellHelpAdd("debug",   "GDB Server (On serial: \"115200,n,8,1\")\n"
                                 "eg. debug localhost:1234 helloworld\n"
                                 "    debug /dev/ttyS1 helloworld\n"
                                 "    debug terminal helloworld\n"
                                 "    debug --attach localhost:1234 1\n");
}

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/

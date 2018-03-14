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
** 文   件   名: lwip_netevent.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 06 月 28 日
**
** 描        述: 网络事件接口.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "lwip/netif.h"
#include "lwip_netevent.h"
/*********************************************************************************************************
  设备与文件结构
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR           NEVT_devhdrHdr;                                /*  设备头                      */
    LW_SEL_WAKEUPLIST    NEVT_selwulList;                               /*  等待链                      */
    LW_LIST_LINE_HEADER  NEVT_plineFile;                                /*  打开的文件链表              */
    LW_OBJECT_HANDLE     NEVT_ulMutex;                                  /*  互斥操作                    */
} LW_NEVT_DEV;
typedef LW_NEVT_DEV     *PLW_NEVT_DEV;

typedef struct {
    LW_LIST_LINE         NEVTFIL_lineManage;                            /*  文件链表                    */
    INT                  NEVTFIL_iFlag;                                 /*  打开文件的选项              */
    PLW_BMSG             NEVTFIL_pbmsg;                                 /*  消息缓冲区                  */
    LW_OBJECT_HANDLE     NEVTFIL_ulReadSync;                            /*  读取同步信号量              */
} LW_NEVT_FILE;
typedef LW_NEVT_FILE    *PLW_NEVT_FILE;
/*********************************************************************************************************
  驱动程序全局变量
*********************************************************************************************************/
static INT               _G_iNevtDrvNum = PX_ERROR;
static LW_NEVT_DEV       _G_nevtdev;
/*********************************************************************************************************
  设备互斥访问
*********************************************************************************************************/
#define NEVT_DEV_LOCK()     API_SemaphoreMPend(_G_nevtdev.NEVT_ulMutex, LW_OPTION_WAIT_INFINITE)
#define NEVT_DEV_UNLOCK()   API_SemaphoreMPost(_G_nevtdev.NEVT_ulMutex)
/*********************************************************************************************************
  驱动程序
*********************************************************************************************************/
static LONG     _nevtOpen(PLW_NEVT_DEV    pnevtdev, 
                          PCHAR           pcName,
                          INT             iFlags, 
                          INT             iMode);
static INT      _nevtClose(PLW_NEVT_FILE  pnevtfil);
static ssize_t  _nevtRead(PLW_NEVT_FILE   pnevtfil, 
                          PCHAR           pcBuffer, 
                          size_t          stMaxBytes);
static ssize_t  _nevtWrite(PLW_NEVT_FILE  pnevtfil, 
                           PCHAR          pcBuffer, 
                           size_t         stNBytes);
static INT      _nevtIoctl(PLW_NEVT_FILE  pnevtfil, 
                           INT            iRequest, 
                           LONG           lArg);
/*********************************************************************************************************
  驱动程序
*********************************************************************************************************/
#define NEVT_MAKE_EVENT(netif, buf, event)              \
        do {                                            \
            lib_bzero(buf, sizeof(buf));                \
            buf[0] = (UCHAR)((event >> 24) & 0xff);     \
            buf[1] = (UCHAR)((event >> 16) & 0xff);     \
            buf[2] = (UCHAR)((event >>  8) & 0xff);     \
            buf[3] = (UCHAR)((event)       & 0xff);     \
            netif_get_name(netif, (char *)&buf[4]);     \
        } while (0)
/*********************************************************************************************************
** 函数名称: _netEventDevCreate
** 功能描述: 安装 nevt 消息设备
** 输　入  : NONE
** 输　出  : 设备是否创建成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _netEventDevCreate (VOID)
{
    if (_G_iNevtDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    _G_nevtdev.NEVT_plineFile = LW_NULL;
    _G_nevtdev.NEVT_ulMutex   = API_SemaphoreMCreate("nevt_lock", LW_PRIO_DEF_CEILING, 
                                                     LW_OPTION_WAIT_PRIORITY |
                                                     LW_OPTION_DELETE_SAFE | 
                                                     LW_OPTION_INHERIT_PRIORITY |
                                                     LW_OPTION_OBJECT_GLOBAL,
                                                     LW_NULL);
    if (_G_nevtdev.NEVT_ulMutex == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }
    
    SEL_WAKE_UP_LIST_INIT(&_G_nevtdev.NEVT_selwulList);
    
    if (iosDevAddEx(&_G_nevtdev.NEVT_devhdrHdr, NET_EVENT_DEV_PATH, 
                    _G_iNevtDrvNum, DT_CHR) != ERROR_NONE) {
        API_SemaphoreMDelete(&_G_nevtdev.NEVT_ulMutex);
        SEL_WAKE_UP_LIST_TERM(&_G_nevtdev.NEVT_selwulList);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _netEventInit
** 功能描述: 初始化网络事件
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _netEventInit (VOID)
{
    if (_G_iNevtDrvNum <= 0) {
        _G_iNevtDrvNum  = iosDrvInstall(_nevtOpen,
                                        LW_NULL,
                                        _nevtOpen,
                                        _nevtClose,
                                        _nevtRead,
                                        _nevtWrite,
                                        _nevtIoctl);
        DRIVER_LICENSE(_G_iNevtDrvNum,     "Dual BSD/GPL->Ver 1.0");
        DRIVER_AUTHOR(_G_iNevtDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iNevtDrvNum, "net event message driver.");
    }
    
    if (_G_iNevtDrvNum > 0) { 
        return  (_netEventDevCreate());
        
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: _netEventDevPutMsg
** 功能描述: 产生一条 net event 消息
** 输　入  : pvMsg     需要保存的消息
**           stSize    消息长度.
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _netEventDevPutMsg (CPVOID pvMsg, size_t stSize)
{
    PLW_LIST_LINE       plineTemp;
    PLW_NEVT_FILE       pnevtfil;
    BOOL                bWakeup = LW_FALSE;
    
    if ((_G_nevtdev.NEVT_ulMutex   == LW_OBJECT_HANDLE_INVALID) ||
        (_G_nevtdev.NEVT_plineFile == LW_NULL)) {
        return;
    }
    
    NEVT_DEV_LOCK();
    for (plineTemp  = _G_nevtdev.NEVT_plineFile;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pnevtfil = _LIST_ENTRY(plineTemp, LW_NEVT_FILE, NEVTFIL_lineManage);
        _bmsgPut(pnevtfil->NEVTFIL_pbmsg, pvMsg, stSize);
        API_SemaphoreBPost(pnevtfil->NEVTFIL_ulReadSync);
        bWakeup = LW_TRUE;
    }
    NEVT_DEV_UNLOCK();
    
    if (bWakeup) {
        SEL_WAKE_UP_ALL(&_G_nevtdev.NEVT_selwulList, SELREAD);
    }
}
/*********************************************************************************************************
** 函数名称: _nevtOpen
** 功能描述: 打开网络消息设备
** 输　入  : pnevtdev         网络消息设备
**           pcName           名称
**           iFlags           方式
**           iMode            方法
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG  _nevtOpen (PLW_NEVT_DEV    pnevtdev, 
                        PCHAR           pcName,
                        INT             iFlags, 
                        INT             iMode)
{
    PLW_NEVT_FILE  pnevtfil;
    
    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & O_CREAT) {
            _ErrorHandle(ERROR_IO_FILE_EXIST);                          /*  不能重复创建                */
            return  (PX_ERROR);
        }
        
        pnevtfil = (PLW_NEVT_FILE)__SHEAP_ALLOC(sizeof(LW_NEVT_FILE));
        if (!pnevtfil) {
__nomem:
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        
        pnevtfil->NEVTFIL_iFlag = iFlags;
        pnevtfil->NEVTFIL_pbmsg = _bmsgCreate(LW_CFG_HOTPLUG_DEV_DEFAULT_BUFSIZE);
        if (pnevtfil->NEVTFIL_pbmsg == LW_NULL) {
            __SHEAP_FREE(pnevtfil);
            goto    __nomem;
        }
    
        pnevtfil->NEVTFIL_ulReadSync = API_SemaphoreBCreate("nevt_rsync", LW_FALSE,
                                                            LW_OPTION_OBJECT_GLOBAL,
                                                            LW_NULL);
        if (pnevtfil->NEVTFIL_ulReadSync == LW_OBJECT_HANDLE_INVALID) {
            _bmsgDelete(pnevtfil->NEVTFIL_pbmsg);
            __SHEAP_FREE(pnevtfil);
            return  (PX_ERROR);
        }
    
        NEVT_DEV_LOCK();
        _List_Line_Add_Tail(&pnevtfil->NEVTFIL_lineManage,
                            &_G_nevtdev.NEVT_plineFile);
        NEVT_DEV_UNLOCK();
        
        LW_DEV_INC_USE_COUNT(&_G_nevtdev.NEVT_devhdrHdr);
        
        return  ((LONG)pnevtfil);
    }
}
/*********************************************************************************************************
** 函数名称: _nevtClose
** 功能描述: 关闭网络消息文件
** 输　入  : pnevtfil         网络消息文件
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _nevtClose (PLW_NEVT_FILE  pnevtfil)
{
    if (pnevtfil) {
        NEVT_DEV_LOCK();
        _List_Line_Del(&pnevtfil->NEVTFIL_lineManage,
                       &_G_nevtdev.NEVT_plineFile);
        NEVT_DEV_UNLOCK();
        
        _bmsgDelete(pnevtfil->NEVTFIL_pbmsg);
        
        LW_DEV_DEC_USE_COUNT(&_G_nevtdev.NEVT_devhdrHdr);
        
        API_SemaphoreBDelete(&pnevtfil->NEVTFIL_ulReadSync);
        __SHEAP_FREE(pnevtfil);
        
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: _nevtRead
** 功能描述: 读网络消息文件
** 输　入  : pnevtfil         网络消息文件
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  _nevtRead (PLW_NEVT_FILE   pnevtfil, 
                           PCHAR           pcBuffer, 
                           size_t          stMaxBytes)
{
    ULONG      ulErrCode;
    ULONG      ulTimeout;
    size_t     stMsgLen;
    ssize_t    sstRet;

    if (!pcBuffer || !stMaxBytes) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pnevtfil->NEVTFIL_iFlag & O_NONBLOCK) {                         /*  非阻塞 IO                   */
        ulTimeout = LW_OPTION_NOT_WAIT;
    } else {
        ulTimeout = LW_OPTION_WAIT_INFINITE;
    }

    for (;;) {
        ulErrCode = API_SemaphoreBPend(pnevtfil->NEVTFIL_ulReadSync,    /*  等待数据有效                */
                                       ulTimeout);
        if (ulErrCode != ERROR_NONE) {                                  /*  超时                        */
            _ErrorHandle(EAGAIN);
            return  (0);
        }
        
        NEVT_DEV_LOCK();
        stMsgLen = (size_t)_bmsgNBytesNext(pnevtfil->NEVTFIL_pbmsg);
        if (stMsgLen > stMaxBytes) {
            NEVT_DEV_UNLOCK();
            API_SemaphoreBPost(pnevtfil->NEVTFIL_ulReadSync);
            _ErrorHandle(EMSGSIZE);                                     /*  缓冲区太小                  */
            return  (PX_ERROR);
        
        } else if (stMsgLen) {
            break;                                                      /*  数据可读                    */
        }
        NEVT_DEV_UNLOCK();
    }
    
    sstRet = (ssize_t)_bmsgGet(pnevtfil->NEVTFIL_pbmsg, pcBuffer, stMaxBytes);
    
    if (!_bmsgIsEmpty(pnevtfil->NEVTFIL_pbmsg)) {
        API_SemaphoreBPost(pnevtfil->NEVTFIL_ulReadSync);
    }
    
    NEVT_DEV_UNLOCK();
    
    return  (sstRet);
}
/*********************************************************************************************************
** 函数名称: _nevtWrite
** 功能描述: 写网络消息文件
** 输　入  : pnevtfil         网络消息文件
**           pcBuffer         将要写入的数据指针
**           stNBytes         写入数据大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  _nevtWrite (PLW_NEVT_FILE  pnevtfil, 
                            PCHAR          pcBuffer, 
                            size_t         stNBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: _nevtIoctl
** 功能描述: 控制网络消息文件
** 输　入  : pnevtfil         网络消息文件
**           iRequest         功能
**           lArg             参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _nevtIoctl (PLW_NEVT_FILE  pnevtfil, 
                        INT            iRequest, 
                        LONG           lArg)
{
    PLW_SEL_WAKEUPNODE   pselwunNode;
    struct stat         *pstatGet;
    PLW_BMSG             pbmsg;
    
    switch (iRequest) {
    
    case FIONREAD:
        NEVT_DEV_LOCK();
        *(INT *)lArg = _bmsgNBytes(pnevtfil->NEVTFIL_pbmsg);
        NEVT_DEV_UNLOCK();
        break;
        
    case FIONMSGS:
        NEVT_DEV_LOCK();
        if (!_bmsgIsEmpty(pnevtfil->NEVTFIL_pbmsg)) {
            *(INT *)lArg = 1;                                           /*  目前暂不通知具体消息数量    */
        } else {
            *(INT *)lArg = 0;
        }
        NEVT_DEV_UNLOCK();
        break;
        
    case FIONBIO:
        NEVT_DEV_LOCK();
        if (*(INT *)lArg) {
            pnevtfil->NEVTFIL_iFlag |= O_NONBLOCK;
        } else {
            pnevtfil->NEVTFIL_iFlag &= ~O_NONBLOCK;
        }
        NEVT_DEV_UNLOCK();
        break;
        
    case FIORFLUSH:
    case FIOFLUSH:
        NEVT_DEV_LOCK();
        _bmsgFlush(pnevtfil->NEVTFIL_pbmsg);
        NEVT_DEV_UNLOCK();
        break;
        
    case FIORBUFSET:
        if (lArg < LW_HOTPLUG_DEV_MAX_MSGSIZE) {
            _ErrorHandle(EMSGSIZE);
            return  (PX_ERROR);
        }
        pbmsg = _bmsgCreate((size_t)lArg);
        if (pbmsg) {
            NEVT_DEV_LOCK();
            _bmsgDelete(pnevtfil->NEVTFIL_pbmsg);
            pnevtfil->NEVTFIL_pbmsg = pbmsg;
            NEVT_DEV_UNLOCK();
        } else {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        break;
        
    case FIOFSTATGET:
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev     = (dev_t)&_G_nevtdev;
            pstatGet->st_ino     = (ino_t)0;                            /*  相当于唯一节点              */
            pstatGet->st_mode    = 0444 | S_IFCHR;
            pstatGet->st_nlink   = 1;
            pstatGet->st_uid     = 0;
            pstatGet->st_gid     = 0;
            pstatGet->st_rdev    = 1;
            pstatGet->st_size    = (off_t)_bmsgSizeGet(pnevtfil->NEVTFIL_pbmsg);
            pstatGet->st_blksize = 0;
            pstatGet->st_blocks  = 0;
            pstatGet->st_atime   = API_RootFsTime(LW_NULL);
            pstatGet->st_mtime   = API_RootFsTime(LW_NULL);
            pstatGet->st_ctime   = API_RootFsTime(LW_NULL);
        } else {
            return  (PX_ERROR);
        }
        break;
        
    case FIOSELECT:
        pselwunNode = (PLW_SEL_WAKEUPNODE)lArg;
        SEL_WAKE_NODE_ADD(&_G_nevtdev.NEVT_selwulList, pselwunNode);
        
        switch (pselwunNode->SELWUN_seltypType) {
        
        case SELREAD:
            if ((pnevtfil->NEVTFIL_pbmsg == LW_NULL) ||
                _bmsgNBytes(pnevtfil->NEVTFIL_pbmsg)) {
                SEL_WAKE_UP(pselwunNode);
            }
            break;
            
        case SELWRITE:
        case SELEXCEPT:                                                 /*  不退出                      */
            break;
        }
        break;
        
    case FIOUNSELECT:
        SEL_WAKE_NODE_DELETE(&_G_nevtdev.NEVT_selwulList, (PLW_SEL_WAKEUPNODE)lArg);
        break;
        
    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: netEventIfAdd
** 功能描述: 加入一个网卡
** 输　入  : pnetif           网卡
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  netEventIfAdd (struct netif *pnetif)
{
    UCHAR   ucBuffer[NET_EVENT_DEV_MAX_MSGSIZE];
    
    NEVT_MAKE_EVENT(pnetif, ucBuffer, NET_EVENT_ADD);
    
    _netEventDevPutMsg(ucBuffer, sizeof(ucBuffer));
}
/*********************************************************************************************************
** 函数名称: netEventIfRemove
** 功能描述: 移除一个网卡
** 输　入  : pnetif           网卡
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  netEventIfRemove (struct netif *pnetif)
{
    UCHAR   ucBuffer[NET_EVENT_DEV_MAX_MSGSIZE];
    
    NEVT_MAKE_EVENT(pnetif, ucBuffer, NET_EVENT_REMOVE);
    
    _netEventDevPutMsg(ucBuffer, sizeof(ucBuffer));
}
/*********************************************************************************************************
** 函数名称: netEventIfUp
** 功能描述: 使能一个网卡
** 输　入  : pnetif           网卡
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  netEventIfUp (struct netif *pnetif)
{
    UCHAR   ucBuffer[NET_EVENT_DEV_MAX_MSGSIZE];
    
    NEVT_MAKE_EVENT(pnetif, ucBuffer, NET_EVENT_UP);
    
    _netEventDevPutMsg(ucBuffer, sizeof(ucBuffer));
    
#if LW_CFG_HOTPLUG_EN > 0
    if (pnetif->flags & NETIF_FLAG_LINK_UP) {
        API_HotplugEventMessage(LW_HOTPLUG_MSG_NETLINK_CHANGE,
                                LW_TRUE, (PCHAR)&ucBuffer[4], 0, 0, 0, 0);
    }
#endif                                                                  /*  LW_CFG_HOTPLUG_EN > 0       */
}
/*********************************************************************************************************
** 函数名称: netEventIfDown
** 功能描述: 禁能一个网卡
** 输　入  : pnetif           网卡
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  netEventIfDown (struct netif *pnetif)
{
    UCHAR   ucBuffer[NET_EVENT_DEV_MAX_MSGSIZE];
    
    NEVT_MAKE_EVENT(pnetif, ucBuffer, NET_EVENT_DOWN);
    
    _netEventDevPutMsg(ucBuffer, sizeof(ucBuffer));
    
#if LW_CFG_HOTPLUG_EN > 0
    API_HotplugEventMessage(LW_HOTPLUG_MSG_NETLINK_CHANGE,
                            LW_FALSE, (PCHAR)&ucBuffer[4], 0, 0, 0, 0);
#endif                                                                  /*  LW_CFG_HOTPLUG_EN > 0       */
}
/*********************************************************************************************************
** 函数名称: netEventIfLink
** 功能描述: 网卡产生连接
** 输　入  : pnetif           网卡
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  netEventIfLink (struct netif *pnetif)
{
    UCHAR   ucBuffer[NET_EVENT_DEV_MAX_MSGSIZE];
    
    NEVT_MAKE_EVENT(pnetif, ucBuffer, NET_EVENT_LINK);
    
    _netEventDevPutMsg(ucBuffer, sizeof(ucBuffer));
    
#if LW_CFG_HOTPLUG_EN > 0
    if (pnetif->flags & NETIF_FLAG_UP) {
        API_HotplugEventMessage(LW_HOTPLUG_MSG_NETLINK_CHANGE,
                                LW_TRUE, (PCHAR)&ucBuffer[4], 0, 0, 0, 0);
    }
#endif                                                                  /*  LW_CFG_HOTPLUG_EN > 0       */
}
/*********************************************************************************************************
** 函数名称: netEventIfUnlink
** 功能描述: 网卡没有连接
** 输　入  : pnetif           网卡
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  netEventIfUnlink (struct netif *pnetif)
{
    UCHAR   ucBuffer[NET_EVENT_DEV_MAX_MSGSIZE];
    
    NEVT_MAKE_EVENT(pnetif, ucBuffer, NET_EVENT_UNLINK);
    
    _netEventDevPutMsg(ucBuffer, sizeof(ucBuffer));
    
#if LW_CFG_HOTPLUG_EN > 0
    API_HotplugEventMessage(LW_HOTPLUG_MSG_NETLINK_CHANGE,
                            LW_FALSE, (PCHAR)&ucBuffer[4], 0, 0, 0, 0);
#endif                                                                  /*  LW_CFG_HOTPLUG_EN > 0       */
}
/*********************************************************************************************************
** 函数名称: netEventIfAddr
** 功能描述: 网卡地址变化
** 输　入  : pnetif           网卡
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  netEventIfAddr (struct netif *pnetif)
{
    UCHAR   ucBuffer[NET_EVENT_DEV_MAX_MSGSIZE];
    
    NEVT_MAKE_EVENT(pnetif, ucBuffer, NET_EVENT_ADDR);
    
    _netEventDevPutMsg(ucBuffer, sizeof(ucBuffer));
}
/*********************************************************************************************************
** 函数名称: netEventIfAddrConflict
** 功能描述: 网卡地址冲突
** 输　入  : pnetif           网卡
**           ucHw             以太网网络地址
**           uiHwLen          网络地址长度
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  netEventIfAddrConflict (struct netif *pnetif, UINT8  ucHw[], UINT  uiHwLen)
{
    UCHAR   ucBuffer[NET_EVENT_DEV_MAX_MSGSIZE];
    CHAR    cIp[IP4ADDR_STRLEN_MAX];
    CHAR    cName[NETIF_NAMESIZE];
    INT     i;
    
    if (ucHw && uiHwLen) {
        _PrintFormat("Warning: net interface: %s IP address %s conflict with: ",
                     netif_get_name(pnetif, cName), ip4addr_ntoa_r(ip_2_ip4(&(pnetif->ip_addr)), cIp, IP4ADDR_STRLEN_MAX));
        for (i = 0; i < (uiHwLen - 1); i++) {
            _PrintFormat("%02x:", ucHw[i]);
        }
        _PrintFormat("%02x\r\n", ucHw[i]);
    }
    
    NEVT_MAKE_EVENT(pnetif, ucBuffer, NET_EVENT_ADDR_CONFLICT);
    
    _netEventDevPutMsg(ucBuffer, sizeof(ucBuffer));
}
/*********************************************************************************************************
** 函数名称: netEventIfAuthFail
** 功能描述: 网卡认证错误
** 输　入  : pnetif           网卡
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  netEventIfAuthFail (struct netif *pnetif)
{
    UCHAR   ucBuffer[NET_EVENT_DEV_MAX_MSGSIZE];
    
    NEVT_MAKE_EVENT(pnetif, ucBuffer, NET_EVENT_AUTH_FAIL);
    
    _netEventDevPutMsg(ucBuffer, sizeof(ucBuffer));
}
/*********************************************************************************************************
** 函数名称: netEventIfAuthFail
** 功能描述: 网卡认证超时
** 输　入  : pnetif           网卡
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  netEventIfAuthTo (struct netif *pnetif)
{
    UCHAR   ucBuffer[NET_EVENT_DEV_MAX_MSGSIZE];
    
    NEVT_MAKE_EVENT(pnetif, ucBuffer, NET_EVENT_AUTH_TO);
    
    _netEventDevPutMsg(ucBuffer, sizeof(ucBuffer));
}
/*********************************************************************************************************
** 函数名称: netEventIfPppExt
** 功能描述: PPP 连接状态转变
** 输　入  : pnetif           网卡
** **        uiEvent          事件
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  netEventIfPppExt (struct netif *pnetif, UINT32  uiEvent)
{
    UCHAR   ucBuffer[NET_EVENT_DEV_MAX_MSGSIZE];

    NEVT_MAKE_EVENT(pnetif, ucBuffer, uiEvent);

    _netEventDevPutMsg(ucBuffer, sizeof(ucBuffer));
}
/*********************************************************************************************************
** 函数名称: netEventIfWlQual
** 功能描述: 网卡无线信号变化
** 输　入  : pnetif           网卡
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  netEventIfWlQual (struct netif *pnetif)
{
    UCHAR   ucBuffer[NET_EVENT_DEV_MAX_MSGSIZE];
    
    NEVT_MAKE_EVENT(pnetif, ucBuffer, NET_EVENT_WL_QUAL);
    
    _netEventDevPutMsg(ucBuffer, sizeof(ucBuffer));
}
/*********************************************************************************************************
** 函数名称: netEventIfWlScan
** 功能描述: 网卡扫描完成
** 输　入  : pnetif           网卡
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  netEventIfWlScan (struct netif *pnetif)
{
    UCHAR   ucBuffer[NET_EVENT_DEV_MAX_MSGSIZE];
    
    NEVT_MAKE_EVENT(pnetif, ucBuffer, NET_EVENT_WL_SCAN);
    
    _netEventDevPutMsg(ucBuffer, sizeof(ucBuffer));
}
/*********************************************************************************************************
** 函数名称: netEventIfWlExt
** 功能描述: 网卡扩展事件发送
** 输　入  : pnetif           网卡
**           uiEvent          事件
**           uiArg0 ~ 3       事件参数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  netEventIfWlExt (struct netif *pnetif, 
                       UINT32        uiEvent, 
                       UINT32        uiArg0,
                       UINT32        uiArg1,
                       UINT32        uiArg2,
                       UINT32        uiArg3)
{
    size_t  i;
    UCHAR   ucBuffer[NET_EVENT_DEV_MAX_MSGSIZE];

    ucBuffer[0] = (UCHAR)((uiEvent >> 24) & 0xff);
    ucBuffer[1] = (UCHAR)((uiEvent >> 16) & 0xff);
    ucBuffer[2] = (UCHAR)((uiEvent >>  8) & 0xff);
    ucBuffer[3] = (UCHAR)((uiEvent)       & 0xff);
    netif_get_name(pnetif, (char *)&ucBuffer[4]);
    
    i = 6 + lib_strlen((char *)&ucBuffer[6]) + 1;
    
    ucBuffer[i++] = (UCHAR)((uiArg0 >> 24) & 0xff);                     /*  MSB                         */
    ucBuffer[i++] = (UCHAR)((uiArg0 >> 16) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg0 >>  8) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg0)       & 0xff);
    
    ucBuffer[i++] = (UCHAR)((uiArg1 >> 24) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg1 >> 16) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg1 >>  8) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg1)       & 0xff);
    
    ucBuffer[i++] = (UCHAR)((uiArg2 >> 24) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg2 >> 16) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg2 >>  8) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg2)       & 0xff);
    
    ucBuffer[i++] = (UCHAR)((uiArg3 >> 24) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg3 >> 16) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg3 >>  8) & 0xff);
    ucBuffer[i++] = (UCHAR)((uiArg3)       & 0xff);
    
    _netEventDevPutMsg(ucBuffer, sizeof(ucBuffer));
}
/*********************************************************************************************************
** 函数名称: netEventIfWlExt2
** 功能描述: 网卡扩展事件发送
** 输　入  : pnetif           网卡
**           ulEvent          无线扩展事件地址
**           uiArg            事件参数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  netEventIfWlExt2 (struct netif *pnetif,
                        PVOID         pvEvent,
                        UINT32        uiArg)
{
    UINT32   uiLen = uiArg;
    UINT32   uiOft;
    PUCHAR   pucBuffer = LW_NULL;

    pucBuffer = (PUCHAR)__SHEAP_ALLOC(4 + IFNAMSIZ + uiLen);
    if (!pucBuffer) {
        return;
    }
    lib_bzero(pucBuffer, 4 + IFNAMSIZ + uiLen);
    
    pucBuffer[0] = (UCHAR)((NET_EVENT_WL_EXT2 >> 24) & 0xff);
    pucBuffer[1] = (UCHAR)((NET_EVENT_WL_EXT2 >> 16) & 0xff);
    pucBuffer[2] = (UCHAR)((NET_EVENT_WL_EXT2 >>  8) & 0xff);
    pucBuffer[3] = (UCHAR)((NET_EVENT_WL_EXT2)       & 0xff);
    
    netif_get_name(pnetif, (char *)&pucBuffer[4]);
    uiOft = 6 + lib_strlen((char *)&pucBuffer[6]) + 1;
    lib_memcpy(pucBuffer + uiOft, (PUCHAR)pvEvent, uiLen);

    _netEventDevPutMsg(pucBuffer, uiOft + uiLen);

    __SHEAP_FREE(pucBuffer);
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/

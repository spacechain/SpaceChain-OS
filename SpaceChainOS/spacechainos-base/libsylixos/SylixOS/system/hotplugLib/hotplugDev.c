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
** 文   件   名: hotplugDev.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 10 月 02 日
**
** 描        述: 热插拔消息设备, 应用程序可以读取此设备来获取系统热插拔信息.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_HOTPLUG_EN > 0) && (LW_CFG_DEVICE_EN > 0)
/*********************************************************************************************************
  设备与文件结构
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR           HOTPDEV_devhdrHdr;                             /*  设备头                      */
    LW_SEL_WAKEUPLIST    HOTPDEV_selwulList;                            /*  等待链                      */
    LW_LIST_LINE_HEADER  HOTPDEV_plineFile;                             /*  打开的文件链表              */
    LW_OBJECT_HANDLE     HOTPDEV_ulMutex;                               /*  互斥操作                    */
} LW_HOTPLUG_DEV;
typedef LW_HOTPLUG_DEV  *PLW_HOTPLUG_DEV;

typedef struct {
    LW_LIST_LINE         HOTPFIL_lineManage;                            /*  文件链表                    */
    INT                  HOTPFIL_iFlag;                                 /*  打开文件的选项              */
    INT                  HOTPFIL_iMsg;                                  /*  关心的热插拔信息            */
    PLW_BMSG             HOTPFIL_pbmsg;                                 /*  消息缓冲区                  */
    LW_OBJECT_HANDLE     HOTPFIL_ulReadSync;                            /*  读取同步信号量              */
} LW_HOTPLUG_FILE;
typedef LW_HOTPLUG_FILE *PLW_HOTPLUG_FILE;
/*********************************************************************************************************
  驱动程序全局变量
*********************************************************************************************************/
static INT               _G_iHotplugDrvNum = PX_ERROR;
static LW_HOTPLUG_DEV    _G_hotplugdev;
/*********************************************************************************************************
  设备互斥访问
*********************************************************************************************************/
#define HOTPLUG_DEV_LOCK()      API_SemaphoreMPend(_G_hotplugdev.HOTPDEV_ulMutex, LW_OPTION_WAIT_INFINITE)
#define HOTPLUG_DEV_UNLOCK()    API_SemaphoreMPost(_G_hotplugdev.HOTPDEV_ulMutex)
/*********************************************************************************************************
  驱动程序
*********************************************************************************************************/
static LONG     _hotplugOpen(PLW_HOTPLUG_DEV photplugdev, 
                             PCHAR           pcName,
                             INT             iFlags, 
                             INT             iMode);
static INT      _hotplugClose(PLW_HOTPLUG_FILE  photplugfil);
static ssize_t  _hotplugRead(PLW_HOTPLUG_FILE  photplugfil, 
                             PCHAR             pcBuffer, 
                             size_t            stMaxBytes);
static ssize_t  _hotplugWrite(PLW_HOTPLUG_FILE  photplugfil, 
                              PCHAR             pcBuffer, 
                              size_t            stNBytes);
static INT      _hotplugIoctl(PLW_HOTPLUG_FILE  photplugfil, 
                              INT               iRequest, 
                              LONG              lArg);
/*********************************************************************************************************
** 函数名称: _hotplugDrvInstall
** 功能描述: 安装 hotplug 消息设备驱动程序
** 输　入  : NONE
** 输　出  : 驱动是否安装成功
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _hotplugDrvInstall (VOID)
{
    if (_G_iHotplugDrvNum <= 0) {
        _G_iHotplugDrvNum  = iosDrvInstall(_hotplugOpen,
                                           LW_NULL,
                                           _hotplugOpen,
                                           _hotplugClose,
                                           _hotplugRead,
                                           _hotplugWrite,
                                           _hotplugIoctl);
        DRIVER_LICENSE(_G_iHotplugDrvNum,     "GPL->Ver 2.0");
        DRIVER_AUTHOR(_G_iHotplugDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iHotplugDrvNum, "hotplug message driver.");
    }
    
    return  ((_G_iHotplugDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** 函数名称: _hotplugDevCreate
** 功能描述: 安装 hotplug 消息设备
** 输　入  : NONE
** 输　出  : 设备是否创建成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
INT  _hotplugDevCreate (VOID)
{
    if (_G_iHotplugDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    _G_hotplugdev.HOTPDEV_plineFile = LW_NULL;
    _G_hotplugdev.HOTPDEV_ulMutex   = API_SemaphoreMCreate("hotplug_lock", LW_PRIO_DEF_CEILING, 
                                                           LW_OPTION_WAIT_PRIORITY |
                                                           LW_OPTION_DELETE_SAFE | 
                                                           LW_OPTION_INHERIT_PRIORITY |
                                                           LW_OPTION_OBJECT_GLOBAL,
                                                           LW_NULL);
    if (_G_hotplugdev.HOTPDEV_ulMutex == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }
    
    SEL_WAKE_UP_LIST_INIT(&_G_hotplugdev.HOTPDEV_selwulList);
    
    if (iosDevAddEx(&_G_hotplugdev.HOTPDEV_devhdrHdr, LW_HOTPLUG_DEV_PATH, 
                    _G_iHotplugDrvNum, DT_CHR) != ERROR_NONE) {
        API_SemaphoreMDelete(&_G_hotplugdev.HOTPDEV_ulMutex);
        SEL_WAKE_UP_LIST_TERM(&_G_hotplugdev.HOTPDEV_selwulList);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _hotplugDevPutMsg
** 功能描述: 产生一条 hotplug 消息
** 输　入  : iMsg      信息类型
**           pvMsg     需要保存的消息
**           stSize    消息长度.
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
VOID  _hotplugDevPutMsg (INT  iMsg, CPVOID pvMsg, size_t stSize)
{
    PLW_LIST_LINE       plineTemp;
    PLW_HOTPLUG_FILE    photplugfil;
    BOOL                bWakeup = LW_FALSE;
    
    if ((_G_hotplugdev.HOTPDEV_ulMutex   == LW_OBJECT_HANDLE_INVALID) ||
        (_G_hotplugdev.HOTPDEV_plineFile == LW_NULL)) {
        return;
    }
    
    HOTPLUG_DEV_LOCK();
    for (plineTemp  = _G_hotplugdev.HOTPDEV_plineFile;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        photplugfil = _LIST_ENTRY(plineTemp, LW_HOTPLUG_FILE, HOTPFIL_lineManage);
        if ((photplugfil->HOTPFIL_iMsg == iMsg) ||
            (photplugfil->HOTPFIL_iMsg == LW_HOTPLUG_MSG_ALL)) {
            _bmsgPut(photplugfil->HOTPFIL_pbmsg, pvMsg, stSize);
            API_SemaphoreBPost(photplugfil->HOTPFIL_ulReadSync);
            bWakeup = LW_TRUE;
        }
    }
    HOTPLUG_DEV_UNLOCK();
    
    if (bWakeup) {
        SEL_WAKE_UP_ALL(&_G_hotplugdev.HOTPDEV_selwulList, SELREAD);
    }
}
/*********************************************************************************************************
** 函数名称: _hotplugOpen
** 功能描述: 打开热插拔消息设备
** 输　入  : photplugdev      热插拔消息设备
**           pcName           名称
**           iFlags           方式
**           iMode            方法
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG  _hotplugOpen (PLW_HOTPLUG_DEV photplugdev, 
                           PCHAR           pcName,
                           INT             iFlags, 
                           INT             iMode)
{
    PLW_HOTPLUG_FILE  photplugfil;

    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & O_CREAT) {
            _ErrorHandle(ERROR_IO_FILE_EXIST);                          /*  不能重复创建                */
            return  (PX_ERROR);
        }
        
        photplugfil = (PLW_HOTPLUG_FILE)__SHEAP_ALLOC(sizeof(LW_HOTPLUG_FILE));
        if (!photplugfil) {
__nomem:
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        
        photplugfil->HOTPFIL_iFlag = iFlags;
        photplugfil->HOTPFIL_iMsg  = LW_HOTPLUG_MSG_ALL;
        photplugfil->HOTPFIL_pbmsg = _bmsgCreate(LW_CFG_HOTPLUG_DEV_DEFAULT_BUFSIZE);
        if (photplugfil->HOTPFIL_pbmsg == LW_NULL) {
            __SHEAP_FREE(photplugfil);
            goto    __nomem;
        }
        
        photplugfil->HOTPFIL_ulReadSync = API_SemaphoreBCreate("hotplug_rsync", LW_FALSE,
                                                               LW_OPTION_OBJECT_GLOBAL,
                                                               LW_NULL);
        if (photplugfil->HOTPFIL_ulReadSync == LW_OBJECT_HANDLE_INVALID) {
            _bmsgDelete(photplugfil->HOTPFIL_pbmsg);
            __SHEAP_FREE(photplugfil);
            return  (PX_ERROR);
        }
        
        HOTPLUG_DEV_LOCK();
        _List_Line_Add_Tail(&photplugfil->HOTPFIL_lineManage,
                            &_G_hotplugdev.HOTPDEV_plineFile);
        HOTPLUG_DEV_UNLOCK();
        
        LW_DEV_INC_USE_COUNT(&_G_hotplugdev.HOTPDEV_devhdrHdr);
        
        return  ((LONG)photplugfil);
    }
}
/*********************************************************************************************************
** 函数名称: _hotplugClose
** 功能描述: 关闭热插拔消息文件
** 输　入  : photplugfil      热插拔消息文件
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _hotplugClose (PLW_HOTPLUG_FILE  photplugfil)
{
    if (photplugfil) {
        HOTPLUG_DEV_LOCK();
        _List_Line_Del(&photplugfil->HOTPFIL_lineManage,
                       &_G_hotplugdev.HOTPDEV_plineFile);
        HOTPLUG_DEV_UNLOCK();
        
        _bmsgDelete(photplugfil->HOTPFIL_pbmsg);
        
        LW_DEV_DEC_USE_COUNT(&_G_hotplugdev.HOTPDEV_devhdrHdr);
        
        API_SemaphoreBDelete(&photplugfil->HOTPFIL_ulReadSync);
        __SHEAP_FREE(photplugfil);
        
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: _hotplugRead
** 功能描述: 读热插拔消息文件
** 输　入  : photplugfil      热插拔消息文件
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  _hotplugRead (PLW_HOTPLUG_FILE  photplugfil, 
                              PCHAR             pcBuffer, 
                              size_t            stMaxBytes)
{
    ULONG      ulErrCode;
    ULONG      ulTimeout;
    size_t     stMsgLen;
    ssize_t    sstRet;

    if (!pcBuffer || !stMaxBytes) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (photplugfil->HOTPFIL_iFlag & O_NONBLOCK) {                      /*  非阻塞 IO                   */
        ulTimeout = LW_OPTION_NOT_WAIT;
    } else {
        ulTimeout = LW_OPTION_WAIT_INFINITE;
    }

    for (;;) {
        ulErrCode = API_SemaphoreBPend(photplugfil->HOTPFIL_ulReadSync, /*  等待数据有效                */
                                       ulTimeout);
        if (ulErrCode != ERROR_NONE) {                                  /*  超时                        */
            _ErrorHandle(EAGAIN);
            return  (0);
        }
        
        HOTPLUG_DEV_LOCK();
        stMsgLen = (size_t)_bmsgNBytesNext(photplugfil->HOTPFIL_pbmsg);
        if (stMsgLen > stMaxBytes) {
            HOTPLUG_DEV_UNLOCK();
            API_SemaphoreBPost(photplugfil->HOTPFIL_ulReadSync);
            _ErrorHandle(EMSGSIZE);                                     /*  缓冲区太小                  */
            return  (PX_ERROR);
        
        } else if (stMsgLen) {
            break;                                                      /*  数据可读                    */
        }
        HOTPLUG_DEV_UNLOCK();
    }
    
    sstRet = (ssize_t)_bmsgGet(photplugfil->HOTPFIL_pbmsg, pcBuffer, stMaxBytes);
    
    if (!_bmsgIsEmpty(photplugfil->HOTPFIL_pbmsg)) {
        API_SemaphoreBPost(photplugfil->HOTPFIL_ulReadSync);
    }
    
    HOTPLUG_DEV_UNLOCK();
    
    return  (sstRet);
}
/*********************************************************************************************************
** 函数名称: _hotplugWrite
** 功能描述: 写热插拔消息文件
** 输　入  : photplugfil      热插拔消息文件
**           pcBuffer         将要写入的数据指针
**           stNBytes         写入数据大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  _hotplugWrite (PLW_HOTPLUG_FILE  photplugfil, 
                               PCHAR             pcBuffer, 
                               size_t            stNBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: _hotplugIoctl
** 功能描述: 控制热插拔消息文件
** 输　入  : photplugfil      热插拔消息文件
**           iRequest         功能
**           lArg             参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _hotplugIoctl (PLW_HOTPLUG_FILE  photplugfil, 
                           INT               iRequest, 
                           LONG              lArg)
{
    PLW_SEL_WAKEUPNODE   pselwunNode;
    struct stat         *pstatGet;
    PLW_BMSG             pbmsg;

    switch (iRequest) {
    
    case FIONREAD:
        HOTPLUG_DEV_LOCK();
        *(INT *)lArg = _bmsgNBytes(photplugfil->HOTPFIL_pbmsg);
        HOTPLUG_DEV_UNLOCK();
        break;
        
    case FIONMSGS:
        HOTPLUG_DEV_LOCK();
        if (!_bmsgIsEmpty(photplugfil->HOTPFIL_pbmsg)) {
            *(INT *)lArg = 1;                                           /*  目前暂不通知具体消息数量    */
        } else {
            *(INT *)lArg = 0;
        }
        HOTPLUG_DEV_UNLOCK();
        break;
        
    case FIONBIO:
        HOTPLUG_DEV_LOCK();
        if (*(INT *)lArg) {
            photplugfil->HOTPFIL_iFlag |= O_NONBLOCK;
        } else {
            photplugfil->HOTPFIL_iFlag &= ~O_NONBLOCK;
        }
        HOTPLUG_DEV_UNLOCK();
        break;
        
    case FIORFLUSH:
    case FIOFLUSH:
        HOTPLUG_DEV_LOCK();
        _bmsgFlush(photplugfil->HOTPFIL_pbmsg);
        HOTPLUG_DEV_UNLOCK();
        break;
        
    case FIORBUFSET:
        if (lArg < LW_HOTPLUG_DEV_MAX_MSGSIZE) {
            _ErrorHandle(EMSGSIZE);
            return  (PX_ERROR);
        }
        pbmsg = _bmsgCreate((size_t)lArg);
        if (pbmsg) {
            HOTPLUG_DEV_LOCK();
            _bmsgDelete(photplugfil->HOTPFIL_pbmsg);
            photplugfil->HOTPFIL_pbmsg = pbmsg;
            HOTPLUG_DEV_UNLOCK();
        } else {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        break;
        
    case LW_HOTPLUG_FIOSETMSG:
        photplugfil->HOTPFIL_iMsg = (INT)lArg;
        break;
        
    case FIOFSTATGET:
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev     = (dev_t)&_G_hotplugdev;
            pstatGet->st_ino     = (ino_t)0;                            /*  相当于唯一节点              */
            pstatGet->st_mode    = 0444 | S_IFCHR;
            pstatGet->st_nlink   = 1;
            pstatGet->st_uid     = 0;
            pstatGet->st_gid     = 0;
            pstatGet->st_rdev    = 1;
            pstatGet->st_size    = (off_t)_bmsgSizeGet(photplugfil->HOTPFIL_pbmsg);
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
        SEL_WAKE_NODE_ADD(&_G_hotplugdev.HOTPDEV_selwulList, pselwunNode);
        
        switch (pselwunNode->SELWUN_seltypType) {
        
        case SELREAD:
            if ((photplugfil->HOTPFIL_pbmsg == LW_NULL) ||
                _bmsgNBytes(photplugfil->HOTPFIL_pbmsg)) {
                SEL_WAKE_UP(pselwunNode);
            }
            break;
            
        case SELWRITE:
        case SELEXCEPT:                                                 /*  不退出                      */
            break;
        }
        break;
        
    case FIOUNSELECT:
        SEL_WAKE_NODE_DELETE(&_G_hotplugdev.HOTPDEV_selwulList, (PLW_SEL_WAKEUPNODE)lArg);
        break;
        
    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_HOTPLUG_EN > 0       */
                                                                        /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/

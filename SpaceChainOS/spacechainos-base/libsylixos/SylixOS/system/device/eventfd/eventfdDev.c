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
** 文   件   名: eventfdDev.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 11 月 20 日
**
** 描        述: Linux 兼容 eventfd 实现.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_EVENTFD_EN > 0)
#include "sys/eventfd.h"
/*********************************************************************************************************
  最大计数
*********************************************************************************************************/
#define LW_EVENTFD_MAX_CNT  0xffffffffffffffffull
/*********************************************************************************************************
  check can read/write
*********************************************************************************************************/
static LW_INLINE BOOL  __evtfd_can_read (PLW_EVTFD_FILE  pevtfdfil)
{
    INTREG  iregInterLevel;
    
    LW_SPIN_LOCK_QUICK(&pevtfdfil->EF_slLock, &iregInterLevel);
    if (pevtfdfil->EF_u64Counter) {
        LW_SPIN_UNLOCK_QUICK(&pevtfdfil->EF_slLock, iregInterLevel);
        return  (LW_TRUE);
    }
    LW_SPIN_UNLOCK_QUICK(&pevtfdfil->EF_slLock, iregInterLevel);
    
    return  (LW_FALSE);
}

static LW_INLINE BOOL  __evtfd_can_write (PLW_EVTFD_FILE  pevtfdfil)
{
    INTREG  iregInterLevel;
    
    LW_SPIN_LOCK_QUICK(&pevtfdfil->EF_slLock, &iregInterLevel);
    if ((LW_EVENTFD_MAX_CNT - pevtfdfil->EF_u64Counter) > 1) {
        LW_SPIN_UNLOCK_QUICK(&pevtfdfil->EF_slLock, iregInterLevel);
        return  (LW_TRUE);
    }
    LW_SPIN_UNLOCK_QUICK(&pevtfdfil->EF_slLock, iregInterLevel);
    
    return  (LW_FALSE);
}
/*********************************************************************************************************
  驱动程序全局变量
*********************************************************************************************************/
static INT              _G_iEvtfdDrvNum = PX_ERROR;
static LW_EVTFD_DEV     _G_evtfddev;
static LW_OBJECT_HANDLE _G_hEvtfdSelMutex;
/*********************************************************************************************************
  驱动程序
*********************************************************************************************************/
static LONG     _evtfdOpen(PLW_EVTFD_DEV    pevtfddev, PCHAR  pcName, INT  iFlags, INT  iMode);
static INT      _evtfdClose(PLW_EVTFD_FILE  pevtfdfil);
static ssize_t  _evtfdRead(PLW_EVTFD_FILE   pevtfdfil, PCHAR  pcBuffer, size_t  stMaxBytes);
static ssize_t  _evtfdWrite(PLW_EVTFD_FILE  pevtfdfil, PCHAR  pcBuffer, size_t  stNBytes);
static INT      _evtfdIoctl(PLW_EVTFD_FILE  pevtfdfil, INT    iRequest, LONG  lArg);
/*********************************************************************************************************
** 函数名称: API_EventfdDrvInstall
** 功能描述: 安装 eventfd 设备驱动程序
** 输　入  : NONE
** 输　出  : 驱动是否安装成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_EventfdDrvInstall (VOID)
{
    if (_G_iEvtfdDrvNum <= 0) {
        _G_iEvtfdDrvNum  = iosDrvInstall(LW_NULL,
                                         LW_NULL,
                                         _evtfdOpen,
                                         _evtfdClose,
                                         _evtfdRead,
                                         _evtfdWrite,
                                         _evtfdIoctl);
        DRIVER_LICENSE(_G_iEvtfdDrvNum,     "GPL->Ver 2.0");
        DRIVER_AUTHOR(_G_iEvtfdDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iEvtfdDrvNum, "eventfd driver.");
    }
    
    if (_G_hEvtfdSelMutex == LW_OBJECT_HANDLE_INVALID) {
        _G_hEvtfdSelMutex =  API_SemaphoreMCreate("evtfdsel_lock", LW_PRIO_DEF_CEILING, 
                                                  LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                                  LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                                  LW_NULL);
    }
    
    return  ((_G_iEvtfdDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** 函数名称: API_EventfdDevCreate
** 功能描述: 安装 eventfd 设备
** 输　入  : NONE
** 输　出  : 设备是否创建成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_EventfdDevCreate (VOID)
{
    if (_G_iEvtfdDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&_G_evtfddev.ED_devhdrHdr, LW_EVTFD_DEV_PATH, 
                    _G_iEvtfdDrvNum, DT_CHR) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: eventfd
** 功能描述: 打开 eventfd 文件
** 输　入  : initval   初始化值
**           flags     打开标志 EFD_SEMAPHORE / EFD_CLOEXEC / EFD_NONBLOCK
** 输　出  : eventfd 文件描述符
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  eventfd (unsigned int initval, int flags)
{
    INT             iFd;
    PLW_EVTFD_FILE  pevtfdfil;
    
    flags &= (EFD_SEMAPHORE | EFD_CLOEXEC | EFD_NONBLOCK);
    
    iFd = open(LW_EVTFD_DEV_PATH, O_RDWR | flags);
    if (iFd >= 0) {
        pevtfdfil = (PLW_EVTFD_FILE)API_IosFdValue(iFd);
        pevtfdfil->EF_u64Counter = (UINT64)initval;
        if (pevtfdfil->EF_u64Counter) {
            API_SemaphoreBPost(pevtfdfil->EF_ulReadLock);
        }
    }
    
    return  (iFd);
}
/*********************************************************************************************************
** 函数名称: eventfd_read
** 功能描述: 读取 eventfd 文件
** 输　入  : fd        文件描述符
**           value     读取缓冲
** 输　出  : 0 : 成功  -1 : 失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  eventfd_read (int fd, eventfd_t *value)
{
    return  (read(fd, value, sizeof(eventfd_t)) != sizeof(eventfd_t) ? PX_ERROR : ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: eventfd_write
** 功能描述: 写 eventfd 文件
** 输　入  : fd        文件描述符
**           value     写入数据
** 输　出  : 0 : 成功  -1 : 失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  eventfd_write (int fd, eventfd_t value)
{
    return  (write(fd, &value, sizeof(eventfd_t)) != sizeof(eventfd_t) ? PX_ERROR : ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _evtfdOpen
** 功能描述: 打开 eventfd 设备
** 输　入  : pevtfddev        eventfd 设备
**           pcName           名称
**           iFlags           方式
**           iMode            方法
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG  _evtfdOpen (PLW_EVTFD_DEV pevtfddev, 
                         PCHAR         pcName,
                         INT           iFlags, 
                         INT           iMode)
{
    PLW_EVTFD_FILE  pevtfdfil;

    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & O_CREAT) {
            _ErrorHandle(ERROR_IO_FILE_EXIST);
            return  (PX_ERROR);
        }
        
        pevtfdfil = (PLW_EVTFD_FILE)__SHEAP_ALLOC(sizeof(LW_EVTFD_FILE));
        if (!pevtfdfil) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        
        pevtfdfil->EF_iFlag      = iFlags;
        pevtfdfil->EF_ulReadLock = API_SemaphoreBCreate("evtfd_rlock", LW_FALSE, 
                                                        LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (pevtfdfil->EF_ulReadLock == LW_OBJECT_HANDLE_INVALID) {
            __SHEAP_FREE(pevtfdfil);
            return  (PX_ERROR);
        }
        pevtfdfil->EF_ulWriteLock = API_SemaphoreBCreate("evtfd_wlock", LW_TRUE, 
                                                         LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (pevtfdfil->EF_ulWriteLock == LW_OBJECT_HANDLE_INVALID) {
            API_SemaphoreBDelete(&pevtfdfil->EF_ulReadLock);
            __SHEAP_FREE(pevtfdfil);
            return  (PX_ERROR);
        }
        
        pevtfdfil->EF_u64Counter = 0ull;
        
        lib_bzero(&pevtfdfil->EF_selwulist, sizeof(LW_SEL_WAKEUPLIST));
        pevtfdfil->EF_selwulist.SELWUL_hListLock = _G_hEvtfdSelMutex;
        
        LW_SPIN_INIT(&pevtfdfil->EF_slLock);
        
        LW_DEV_INC_USE_COUNT(&_G_evtfddev.ED_devhdrHdr);
        
        return  ((LONG)pevtfdfil);
    }
}
/*********************************************************************************************************
** 函数名称: _evtfdClose
** 功能描述: 关闭 eventfd 文件
** 输　入  : pevtfdfil         eventfd 文件
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _evtfdClose (PLW_EVTFD_FILE  pevtfdfil)
{
    if (pevtfdfil) {
        SEL_WAKE_UP_TERM(&pevtfdfil->EF_selwulist);
    
        LW_DEV_DEC_USE_COUNT(&_G_evtfddev.ED_devhdrHdr);
        
        API_SemaphoreBDelete(&pevtfdfil->EF_ulReadLock);
        API_SemaphoreBDelete(&pevtfdfil->EF_ulWriteLock);
        __SHEAP_FREE(pevtfdfil);
        
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: _evtfdRead
** 功能描述: 读 eventfd 设备
** 输　入  : pevtfdfil        eventfd 文件
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  _evtfdRead (PLW_EVTFD_FILE  pevtfdfil, 
                            PCHAR           pcBuffer, 
                            size_t          stMaxBytes)
{
    INTREG  iregInterLevel;
    ULONG   ulLwErrCode;
    ULONG   ulTimeout;
    BOOL    bRelease = LW_FALSE;

    if (!pcBuffer || (stMaxBytes < sizeof(UINT64))) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  是否在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);                                             /*  不能在中断中调用            */
    }
    
    if (pevtfdfil->EF_iFlag & O_NONBLOCK) {
        ulTimeout = LW_OPTION_NOT_WAIT;
    } else {
        ulTimeout = LW_OPTION_WAIT_INFINITE;
    }
    
    for (;;) {
        ulLwErrCode = API_SemaphoreBPend(pevtfdfil->EF_ulReadLock, ulTimeout);
        if (ulLwErrCode != ERROR_NONE) {                                /*  超时                        */
            _ErrorHandle(EAGAIN);
            return  (0);
        }
        
        LW_SPIN_LOCK_QUICK(&pevtfdfil->EF_slLock, &iregInterLevel);
        if (pevtfdfil->EF_u64Counter) {
            break;
        }
        LW_SPIN_UNLOCK_QUICK(&pevtfdfil->EF_slLock, iregInterLevel);
    }
    
    if (pevtfdfil->EF_iFlag & EFD_SEMAPHORE) {                          /*  EFD_SEMAPHORE               */
        UINT64  u64One = 1;
        lib_memcpy(pcBuffer, &u64One, sizeof(UINT64));                  /*  host byte order             */
        pevtfdfil->EF_u64Counter--;
    
    } else {
        lib_memcpy(pcBuffer, &pevtfdfil->EF_u64Counter, sizeof(UINT64));
        pevtfdfil->EF_u64Counter = 0;
    }
    
    if (pevtfdfil->EF_u64Counter) {
        bRelease = LW_TRUE;
    }
    LW_SPIN_UNLOCK_QUICK(&pevtfdfil->EF_slLock, iregInterLevel);
    
    if (bRelease) {
        API_SemaphoreBPost(pevtfdfil->EF_ulReadLock);
        SEL_WAKE_UP_ALL(&pevtfdfil->EF_selwulist, SELREAD);
    }
    
    API_SemaphoreBPost(pevtfdfil->EF_ulWriteLock);
    SEL_WAKE_UP_ALL(&pevtfdfil->EF_selwulist, SELWRITE);
    
    return  (sizeof(UINT64));
}
/*********************************************************************************************************
** 函数名称: _evtfdWrite
** 功能描述: 写 eventfd 设备
** 输　入  : pevtfdfil        eventfd 文件
**           pcBuffer         将要写入的数据指针
**           stNBytes         写入数据大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  _evtfdWrite (PLW_EVTFD_FILE pevtfdfil, 
                             PCHAR          pcBuffer, 
                             size_t         stNBytes)
{
    INTREG  iregInterLevel;
    UINT64  u64Add;
    ULONG   ulLwErrCode;
    ULONG   ulTimeout;
    BOOL    bRelease = LW_FALSE;
    
    if (!pcBuffer || (stNBytes < sizeof(UINT64))) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  是否在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);                                             /*  不能在中断中调用            */
    }
    
    lib_memcpy(&u64Add, pcBuffer, sizeof(UINT64));
    
    if (u64Add == LW_EVENTFD_MAX_CNT) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pevtfdfil->EF_iFlag & O_NONBLOCK) {
        ulTimeout = LW_OPTION_NOT_WAIT;
    } else {
        ulTimeout = LW_OPTION_WAIT_INFINITE;
    }
    
    for (;;) {
        ulLwErrCode = API_SemaphoreBPend(pevtfdfil->EF_ulWriteLock, ulTimeout);
        if (ulLwErrCode != ERROR_NONE) {                                /*  超时                        */
            _ErrorHandle(EAGAIN);
            return  (0);
        }
        
        LW_SPIN_LOCK_QUICK(&pevtfdfil->EF_slLock, &iregInterLevel);
        if ((LW_EVENTFD_MAX_CNT - u64Add) > pevtfdfil->EF_u64Counter) { /*  不能产生溢出                */
            break;
        }
        LW_SPIN_UNLOCK_QUICK(&pevtfdfil->EF_slLock, iregInterLevel);
    }
    
    pevtfdfil->EF_u64Counter += u64Add;
    if (pevtfdfil->EF_u64Counter) {
        bRelease = LW_TRUE;
    }
    LW_SPIN_UNLOCK_QUICK(&pevtfdfil->EF_slLock, iregInterLevel);
    
    if (bRelease) {
        API_SemaphoreBPost(pevtfdfil->EF_ulReadLock);
        SEL_WAKE_UP_ALL(&pevtfdfil->EF_selwulist, SELREAD);
    }
    
    return  (sizeof(UINT64));
}
/*********************************************************************************************************
** 函数名称: _evtfdIoctl
** 功能描述: 控制 eventfd 文件
** 输　入  : pevtfdfil        eventfd 文件
**           iRequest         功能
**           lArg             参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _evtfdIoctl (PLW_EVTFD_FILE  pevtfdfil, 
                         INT             iRequest, 
                         LONG            lArg)
{
    struct stat         *pstatGet;
    PLW_SEL_WAKEUPNODE   pselwunNode;
    
    switch (iRequest) {
    
    case FIONBIO:
        if (*(INT *)lArg) {
            pevtfdfil->EF_iFlag |= O_NONBLOCK;
        } else {
            pevtfdfil->EF_iFlag &= ~O_NONBLOCK;
        }
        break;
        
    case FIOFSTATGET:
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev     = (dev_t)&_G_evtfddev;
            pstatGet->st_ino     = (ino_t)0;                            /*  相当于唯一节点              */
            pstatGet->st_mode    = 0666 | S_IFCHR;
            pstatGet->st_nlink   = 1;
            pstatGet->st_uid     = 0;
            pstatGet->st_gid     = 0;
            pstatGet->st_rdev    = 1;
            pstatGet->st_size    = 0;
            pstatGet->st_blksize = 0;
            pstatGet->st_blocks  = 0;
            pstatGet->st_atime   = API_RootFsTime(LW_NULL);
            pstatGet->st_mtime   = API_RootFsTime(LW_NULL);
            pstatGet->st_ctime   = API_RootFsTime(LW_NULL);
        } else {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
        break;
        
    case FIOSELECT:
        pselwunNode = (PLW_SEL_WAKEUPNODE)lArg;
        SEL_WAKE_NODE_ADD(&pevtfdfil->EF_selwulist, pselwunNode);
        
        switch (pselwunNode->SELWUN_seltypType) {
        
        case SELREAD:
            if (__evtfd_can_read(pevtfdfil)) {
                SEL_WAKE_UP(pselwunNode);
            }
            break;
            
        case SELWRITE:
            if (__evtfd_can_write(pevtfdfil)) {
                SEL_WAKE_UP(pselwunNode);
            }
            break;
            
        case SELEXCEPT:
            break;
        }
        break;
        
    case FIOUNSELECT:
        SEL_WAKE_NODE_DELETE(&pevtfdfil->EF_selwulist, (PLW_SEL_WAKEUPNODE)lArg);
        break;
        
    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
                                                                        /*  LW_CFG_EVENTFD_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

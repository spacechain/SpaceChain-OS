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
** 文   件   名: hstimerfdDev.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 03 日
**
** 描        述: 高频率定时器设备.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if (LW_CFG_PTIMER_EN > 0) && (LW_CFG_TIMERFD_EN > 0)
#include "sys/hstimerfd.h"
/*********************************************************************************************************
  驱动程序全局变量
*********************************************************************************************************/
static INT              _G_iHstmrfdDrvNum = PX_ERROR;
static LW_HSTMRFD_DEV   _G_hstmrfddev;
static LW_OBJECT_HANDLE _G_hHstmrfdSelMutex;
/*********************************************************************************************************
  驱动程序
*********************************************************************************************************/
static VOID     _hstmrfdCallback(PLW_HSTMRFD_FILE  phstmrfdfil);
static LONG     _hstmrfdOpen(PLW_HSTMRFD_DEV    phstmrfddev, PCHAR  pcName, INT  iFlags, INT  iMode);
static INT      _hstmrfdClose(PLW_HSTMRFD_FILE  phstmrfdfil);
static ssize_t  _hstmrfdRead(PLW_HSTMRFD_FILE   phstmrfdfil, PCHAR  pcBuffer, size_t  stMaxBytes);
static INT      _hstmrfdIoctl(PLW_HSTMRFD_FILE  phstmrfdfil, INT    iRequest, LONG    lArg);
/*********************************************************************************************************
  时间变换
*********************************************************************************************************/
static LW_INLINE  VOID   __hstickToTimespec (ULONG  ulHsticks, struct timespec  *ptv)
{
    ptv->tv_sec  = (time_t)(ulHsticks / LW_HTIMER_HZ);
    ptv->tv_nsec = (LONG)(ulHsticks % LW_HTIMER_HZ) 
                 * ((1000 * 1000 * 1000) / LW_HTIMER_HZ);
}
static LW_INLINE  ULONG  __timespecToHstick (const struct timespec  *ptv)
{
    REGISTER ULONG     ulHsticks;
    
    ulHsticks  = (ULONG)(ptv->tv_sec * LW_HTIMER_HZ);
    ulHsticks += (((((ptv->tv_nsec / 1000) * LW_HTIMER_HZ) / 100) / 100) / 100);
    
    return  (ulHsticks);
}
/*********************************************************************************************************
** 函数名称: API_HstimerfdDrvInstall
** 功能描述: 安装 hstimerfd 设备驱动程序
** 输　入  : NONE
** 输　出  : 驱动是否安装成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_HstimerfdDrvInstall (VOID)
{
    if (_G_iHstmrfdDrvNum <= 0) {
        _G_iHstmrfdDrvNum  = iosDrvInstall(LW_NULL,
                                           LW_NULL,
                                           _hstmrfdOpen,
                                           _hstmrfdClose,
                                           _hstmrfdRead,
                                           LW_NULL,
                                           _hstmrfdIoctl);
        DRIVER_LICENSE(_G_iHstmrfdDrvNum,     "GPL->Ver 2.0");
        DRIVER_AUTHOR(_G_iHstmrfdDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iHstmrfdDrvNum, "hstimerfd driver.");
    }
    
    if (_G_hHstmrfdSelMutex == LW_OBJECT_HANDLE_INVALID) {
        _G_hHstmrfdSelMutex =  API_SemaphoreMCreate("hstmrfdsel_lock", LW_PRIO_DEF_CEILING, 
                                                    LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                                    LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                                    LW_NULL);
    }
    
    return  ((_G_iHstmrfdDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** 函数名称: API_HstimerfdDevCreate
** 功能描述: 安装 hstimerfd 设备
** 输　入  : NONE
** 输　出  : 设备是否创建成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_HstimerfdDevCreate (VOID)
{
    if (_G_iHstmrfdDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&_G_hstmrfddev.HD_devhdrHdr, LW_HSTMRFD_DEV_PATH, 
                    _G_iHstmrfdDrvNum, DT_CHR) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: hstimerfd_hz
** 功能描述: 获得系统高速定时器工作频率
** 输　入  : NONE
** 输　出  : 频率
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  hstimerfd_hz (void)
{
    return  (LW_HTIMER_HZ);
}
/*********************************************************************************************************
** 函数名称: hstimerfd_create
** 功能描述: 打开 hstimerfd 文件
** 输　入  : flags         打开标志 HSTFD_NONBLOCK / HSTFD_CLOEXEC
** 输　出  : 文件描述符
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  hstimerfd_create (int flags)
{
    flags &= (HSTFD_NONBLOCK | HSTFD_CLOEXEC);
    
    return  (open(LW_HSTMRFD_DEV_PATH, O_RDONLY | flags));
}
/*********************************************************************************************************
** 函数名称: hstimerfd_settime
** 功能描述: 设置 hstimerfd 文件定时时间
** 输　入  : fd            文件描述符
**           ntmr          新的定时时间
**           otmr          获取先前的定时时间
** 输　出  : 0 : 成功  -1 : 失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  hstimerfd_settime (int fd, const struct itimerspec *ntmr, struct itimerspec *otmr)
{
    PLW_HSTMRFD_FILE  phstmrfdfil;
    ULONG             ulError;
    ULONG             ulOption;
    BOOL              bIsRunning;
    ULONG             ulCounter;
    ULONG             ulInterval;
    
    if (!ntmr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (fd < 0) {
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    phstmrfdfil = (PLW_HSTMRFD_FILE)API_IosFdValue(fd);
    if (!phstmrfdfil || (phstmrfdfil->HF_uiMagic != LW_HSTIMER_FILE_MAGIC)) {
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    API_SemaphoreBClear(phstmrfdfil->HF_ulReadLock);
    
    if (otmr) {
        ulError = API_TimerStatus(phstmrfdfil->HF_ulTimer, &bIsRunning,
                                  LW_NULL, &ulCounter, &ulInterval);
        if (ulError) {
            return  (PX_ERROR);
        }
        if (bIsRunning == LW_FALSE) {
            otmr->it_interval.tv_sec  = 0;
            otmr->it_interval.tv_nsec = 0;
            otmr->it_value.tv_sec     = 0;
            otmr->it_value.tv_nsec    = 0;
        
        } else {
            __hstickToTimespec(ulInterval, &otmr->it_interval);
            __hstickToTimespec(ulCounter, &otmr->it_value);
        }
    }
    
    if (ntmr->it_value.tv_sec  == 0 &&
        ntmr->it_value.tv_nsec == 0) {
        API_TimerCancel(phstmrfdfil->HF_ulTimer);
        return  (ERROR_NONE);
    }
    
    if (ntmr->it_interval.tv_sec  == 0 &&
        ntmr->it_interval.tv_nsec == 0) {                               /*  ONE SHOT                    */
        ulOption = LW_OPTION_MANUAL_RESTART;
    } else {
        ulOption = LW_OPTION_AUTO_RESTART;                              /*  AUTO RELOAD                 */
    }
    
    ulCounter  = __timespecToHstick(&ntmr->it_value);
    ulInterval = __timespecToHstick(&ntmr->it_interval);
    
    ulCounter  = (ulCounter  == 0) ? 1 : ulCounter;
    ulInterval = (ulInterval == 0) ? 1 : ulInterval;
    
    ulError = API_TimerStartEx(phstmrfdfil->HF_ulTimer, 
                               ulCounter,
                               ulInterval,
                               ulOption,
                               (PTIMER_CALLBACK_ROUTINE)_hstmrfdCallback,
                               (PVOID)phstmrfdfil);
    if (ulError) {
        return  (PX_ERROR);
    }
                     
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: hstimerfd_settime2
** 功能描述: 设置 hstimerfd 文件定时时间
** 输　入  : fd            文件描述符
**           ncnt          新的定时时间
**           ocnt          获取先前的定时时间
** 输　出  : 0 : 成功  -1 : 失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  hstimerfd_settime2 (int fd, hstimer_cnt_t *ncnt, hstimer_cnt_t *ocnt)
{
    PLW_HSTMRFD_FILE  phstmrfdfil;
    ULONG             ulError;
    ULONG             ulOption;
    BOOL              bIsRunning;
    ULONG             ulCounter;
    ULONG             ulInterval;
    
    if (!ncnt) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (fd < 0) {
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    phstmrfdfil = (PLW_HSTMRFD_FILE)API_IosFdValue(fd);
    if (!phstmrfdfil || (phstmrfdfil->HF_uiMagic != LW_HSTIMER_FILE_MAGIC)) {
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    API_SemaphoreBClear(phstmrfdfil->HF_ulReadLock);
    
    if (ocnt) {
        ulError = API_TimerStatus(phstmrfdfil->HF_ulTimer, &bIsRunning,
                                  LW_NULL, &ulCounter, &ulInterval);
        if (ulError) {
            return  (PX_ERROR);
        }
        if (bIsRunning == LW_FALSE) {
            ocnt->interval = 0ul;
            ocnt->value    = 0ul;
        
        } else {
            ocnt->interval = ulInterval;
            ocnt->value    = ulCounter;
        }
    }
    
    if (ncnt->value == 0) {
        API_TimerCancel(phstmrfdfil->HF_ulTimer);
        return  (ERROR_NONE);
    }
    
    if (ncnt->interval == 0) {                                          /*  ONE SHOT                    */
        ulOption = LW_OPTION_MANUAL_RESTART;
    } else {
        ulOption = LW_OPTION_AUTO_RESTART;                              /*  AUTO RELOAD                 */
    }
    
    ulError = API_TimerStartEx(phstmrfdfil->HF_ulTimer, 
                               ncnt->value,
                               ncnt->interval,
                               ulOption,
                               (PTIMER_CALLBACK_ROUTINE)_hstmrfdCallback,
                               (PVOID)phstmrfdfil);
    if (ulError) {
        return  (PX_ERROR);
    }
                     
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: hstimerfd_gettime
** 功能描述: 获取 hstimerfd 文件定时时间
** 输　入  : fd            文件描述符
**           currvalue     获取当前的定时时间
** 输　出  : 0 : 成功  -1 : 失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  hstimerfd_gettime (int fd, struct itimerspec *currvalue)
{
    PLW_HSTMRFD_FILE  phstmrfdfil;
    ULONG             ulError;
    BOOL              bIsRunning;
    ULONG             ulCounter;
    ULONG             ulInterval;
    
    if (!currvalue) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (fd < 0) {
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    phstmrfdfil = (PLW_HSTMRFD_FILE)API_IosFdValue(fd);
    if (!phstmrfdfil || (phstmrfdfil->HF_uiMagic != LW_HSTIMER_FILE_MAGIC)) {
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    ulError = API_TimerStatus(phstmrfdfil->HF_ulTimer, &bIsRunning,
                              LW_NULL, &ulCounter, &ulInterval);
    if (ulError) {
        return  (PX_ERROR);
    }
    
    if (bIsRunning == LW_FALSE) {
        currvalue->it_interval.tv_sec  = 0;
        currvalue->it_interval.tv_nsec = 0;
        currvalue->it_value.tv_sec     = 0;
        currvalue->it_value.tv_nsec    = 0;
        return  (ERROR_NONE);
    }
    
    __hstickToTimespec(ulInterval, &currvalue->it_interval);
    __hstickToTimespec(ulCounter, &currvalue->it_value);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: hstimerfd_gettime2
** 功能描述: 获取 hstimerfd 文件定时时间
** 输　入  : fd            文件描述符
**           currvalue     获取当前的定时时间
** 输　出  : 0 : 成功  -1 : 失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  hstimerfd_gettime2 (int fd, hstimer_cnt_t *currvalue)
{
    PLW_HSTMRFD_FILE  phstmrfdfil;
    ULONG             ulError;
    BOOL              bIsRunning;
    ULONG             ulCounter;
    ULONG             ulInterval;
    
    if (!currvalue) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (fd < 0) {
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    phstmrfdfil = (PLW_HSTMRFD_FILE)API_IosFdValue(fd);
    if (!phstmrfdfil || (phstmrfdfil->HF_uiMagic != LW_HSTIMER_FILE_MAGIC)) {
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    ulError = API_TimerStatus(phstmrfdfil->HF_ulTimer, &bIsRunning,
                              LW_NULL, &ulCounter, &ulInterval);
    if (ulError) {
        return  (PX_ERROR);
    }
    
    if (bIsRunning == LW_FALSE) {
        currvalue->interval = 0ul;
        currvalue->value    = 0ul;
        return  (ERROR_NONE);
    }
    
    currvalue->interval = ulInterval;
    currvalue->value    = ulCounter;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _hstmrfdCallback
** 功能描述: hstmrfd 文件到时回调
** 输　入  : phstmrfdfil      hstimerfd 文件
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _hstmrfdCallback (PLW_HSTMRFD_FILE  phstmrfdfil)
{
    INTREG          iregInterLevel;
    PLW_CLASS_TIMER ptmr = (PLW_CLASS_TIMER)phstmrfdfil->HF_pvTimer;

    LW_SPIN_LOCK_QUICK(&ptmr->TIMER_slLock, &iregInterLevel);
    ptmr->TIMER_u64Overrun++;
    LW_SPIN_UNLOCK_QUICK(&ptmr->TIMER_slLock, iregInterLevel);
    
    API_SemaphoreBPost(phstmrfdfil->HF_ulReadLock);
    SEL_WAKE_UP_ALL(&phstmrfdfil->HF_selwulist, SELREAD);
}
/*********************************************************************************************************
** 函数名称: _hstmrfdOpen
** 功能描述: 打开 hstimerfd 设备
** 输　入  : phstmrfddev      hstimerfd 设备
**           pcName           名称
**           iFlags           方式
**           iMode            方法
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG  _hstmrfdOpen (PLW_HSTMRFD_DEV phstmrfddev, 
                           PCHAR           pcName,
                           INT             iFlags, 
                           INT             iMode)
{
    PLW_HSTMRFD_FILE  phstmrfdfil;

    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & O_CREAT) {
            _ErrorHandle(ERROR_IO_FILE_EXIST);
            return  (PX_ERROR);
        }
        
        phstmrfdfil = (PLW_HSTMRFD_FILE)__SHEAP_ALLOC(sizeof(LW_HSTMRFD_FILE));
        if (!phstmrfdfil) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        
        phstmrfdfil->HF_uiMagic = LW_HSTIMER_FILE_MAGIC;
        phstmrfdfil->HF_iFlag   = iFlags;
        
        phstmrfdfil->HF_ulTimer = API_TimerCreate("hstmrfd", 
                                                  LW_OPTION_HTIMER | 
                                                  LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (phstmrfdfil->HF_ulTimer == LW_OBJECT_HANDLE_INVALID) {
            __SHEAP_FREE(phstmrfdfil);
            return  (PX_ERROR);
        }
        
        phstmrfdfil->HF_pvTimer = (PVOID)&_K_tmrBuffer[_ObjectGetIndex(phstmrfdfil->HF_ulTimer)];

        phstmrfdfil->HF_ulReadLock = API_SemaphoreBCreate("hstmrfd_rlock", LW_FALSE, 
                                                          LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (phstmrfdfil->HF_ulReadLock == LW_OBJECT_HANDLE_INVALID) {
            API_TimerDelete(&phstmrfdfil->HF_ulTimer);
            __SHEAP_FREE(phstmrfdfil);
            return  (PX_ERROR);
        }
        
        lib_bzero(&phstmrfdfil->HF_selwulist, sizeof(LW_SEL_WAKEUPLIST));
        phstmrfdfil->HF_selwulist.SELWUL_hListLock = _G_hHstmrfdSelMutex;
        
        LW_DEV_INC_USE_COUNT(&_G_hstmrfddev.HD_devhdrHdr);
        
        return  ((LONG)phstmrfdfil);
    }
}
/*********************************************************************************************************
** 函数名称: _hstmrfdClose
** 功能描述: 关闭 hstimerfd 文件
** 输　入  : phstmrfdfil       hstimerfd 文件
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _hstmrfdClose (PLW_HSTMRFD_FILE  phstmrfdfil)
{
    if (phstmrfdfil) {
        SEL_WAKE_UP_TERM(&phstmrfdfil->HF_selwulist);
    
        LW_DEV_DEC_USE_COUNT(&_G_hstmrfddev.HD_devhdrHdr);
        
        API_TimerDelete(&phstmrfdfil->HF_ulTimer);
        API_SemaphoreBDelete(&phstmrfdfil->HF_ulReadLock);
        __SHEAP_FREE(phstmrfdfil);
        
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: _hstmrfdRead
** 功能描述: 读 hstimerfd 设备
** 输　入  : phstmrfdfil      hstimerfd 文件
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
** 注  意  : 由于是高速定时器, 所以这里获取定时器结构时没有加入内核锁, 
             所以必须保证文件在读的时候不能被关闭.
*********************************************************************************************************/
static ssize_t  _hstmrfdRead (PLW_HSTMRFD_FILE  phstmrfdfil, 
                              PCHAR             pcBuffer, 
                              size_t            stMaxBytes)
{
    INTREG          iregInterLevel;
    PLW_CLASS_TIMER ptmr;
    ULONG           ulLwErrCode;
    UINT64          u64Overruns;
    ULONG           ulTimeout;

    if (!pcBuffer || (stMaxBytes < sizeof(UINT64))) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (phstmrfdfil->HF_iFlag & O_NONBLOCK) {
        ulTimeout = LW_OPTION_NOT_WAIT;
    } else {
        ulTimeout = LW_OPTION_WAIT_INFINITE;
    }
    
    ulLwErrCode = API_SemaphoreBPend(phstmrfdfil->HF_ulReadLock, ulTimeout);
    if (ulLwErrCode != ERROR_NONE) {                                    /*  超时                        */
        _ErrorHandle(EAGAIN);
        return  (0);
    }
    
    ptmr = (PLW_CLASS_TIMER)phstmrfdfil->HF_pvTimer;
    
    LW_SPIN_LOCK_QUICK(&ptmr->TIMER_slLock, &iregInterLevel);
    u64Overruns = ptmr->TIMER_u64Overrun;
    ptmr->TIMER_u64Overrun = 0ull;
    LW_SPIN_UNLOCK_QUICK(&ptmr->TIMER_slLock, iregInterLevel);
    
    lib_memcpy(pcBuffer, &u64Overruns, sizeof(UINT64));
    
    return  (sizeof(UINT64));
}
/*********************************************************************************************************
** 函数名称: _hstmrfdIoctl
** 功能描述: 控制 hstimerfd 文件
** 输　入  : phstmrfdfil      hstimerfd 文件
**           iRequest         功能
**           lArg             参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _hstmrfdIoctl (PLW_HSTMRFD_FILE  phstmrfdfil, 
                           INT               iRequest, 
                           LONG              lArg)
{
    struct stat         *pstatGet;
    PLW_SEL_WAKEUPNODE   pselwunNode;
    BOOL                 bExpired = LW_FALSE;
    
    switch (iRequest) {
    
    case FIONBIO:
        if (*(INT *)lArg) {
            phstmrfdfil->HF_iFlag |= O_NONBLOCK;
        } else {
            phstmrfdfil->HF_iFlag &= ~O_NONBLOCK;
        }
        break;
        
    case FIOFSTATGET:
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev     = (dev_t)&_G_hstmrfddev;
            pstatGet->st_ino     = (ino_t)0;                            /*  相当于唯一节点              */
            pstatGet->st_mode    = 0444 | S_IFCHR;
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
        SEL_WAKE_NODE_ADD(&phstmrfdfil->HF_selwulist, pselwunNode);
        
        switch (pselwunNode->SELWUN_seltypType) {
        
        case SELREAD:
            API_SemaphoreBStatus(phstmrfdfil->HF_ulReadLock, &bExpired, LW_NULL, LW_NULL);
            if (bExpired) {
                SEL_WAKE_UP(pselwunNode);
            }
            break;
            
        case SELWRITE:
        case SELEXCEPT:
            break;
        }
        break;
        
    case FIOUNSELECT:
        SEL_WAKE_NODE_DELETE(&phstmrfdfil->HF_selwulist, (PLW_SEL_WAKEUPNODE)lArg);
        break;
        
    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_PTIMER_EN > 0        */
                                                                        /*  LW_CFG_TIMERFD_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

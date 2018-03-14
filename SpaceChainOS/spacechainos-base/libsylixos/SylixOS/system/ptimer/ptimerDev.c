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
** 文   件   名: ptimerDev.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 11 月 20 日
**
** 描        述: Linux 兼容 timerfd 实现.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if (LW_CFG_PTIMER_EN > 0) && (LW_CFG_TIMERFD_EN > 0)
#include "sys/timerfd.h"
/*********************************************************************************************************
  驱动程序全局变量
*********************************************************************************************************/
static INT              _G_iTmrfdDrvNum = PX_ERROR;
static LW_TMRFD_DEV     _G_tmrfddev;
static LW_OBJECT_HANDLE _G_hTmrfdSelMutex;
/*********************************************************************************************************
  驱动程序
*********************************************************************************************************/
static LONG     _tmrfdOpen(PLW_TMRFD_DEV    ptmrfddev, PCHAR  pcName, INT  iFlags, INT  iMode);
static INT      _tmrfdClose(PLW_TMRFD_FILE  ptmrfdfil);
static ssize_t  _tmrfdRead(PLW_TMRFD_FILE   ptmrfdfil, PCHAR  pcBuffer, size_t  stMaxBytes);
static INT      _tmrfdIoctl(PLW_TMRFD_FILE  ptmrfdfil, INT    iRequest, LONG    lArg);
/*********************************************************************************************************
** 函数名称: API_TimerfdDrvInstall
** 功能描述: 安装 timerfd 设备驱动程序
** 输　入  : NONE
** 输　出  : 驱动是否安装成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_TimerfdDrvInstall (VOID)
{
    if (_G_iTmrfdDrvNum <= 0) {
        _G_iTmrfdDrvNum  = iosDrvInstall(LW_NULL,
                                         LW_NULL,
                                         _tmrfdOpen,
                                         _tmrfdClose,
                                         _tmrfdRead,
                                         LW_NULL,
                                         _tmrfdIoctl);
        DRIVER_LICENSE(_G_iTmrfdDrvNum,     "GPL->Ver 2.0");
        DRIVER_AUTHOR(_G_iTmrfdDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iTmrfdDrvNum, "timerfd driver.");
    }
    
    if (_G_hTmrfdSelMutex == LW_OBJECT_HANDLE_INVALID) {
        _G_hTmrfdSelMutex =  API_SemaphoreMCreate("tmrfdsel_lock", LW_PRIO_DEF_CEILING, 
                                                  LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                                  LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                                  LW_NULL);
    }
    
    return  ((_G_iTmrfdDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** 函数名称: API_TimerfdDevCreate
** 功能描述: 安装 timerfd 设备
** 输　入  : NONE
** 输　出  : 设备是否创建成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_TimerfdDevCreate (VOID)
{
    if (_G_iTmrfdDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&_G_tmrfddev.TD_devhdrHdr, LW_TMRFD_DEV_PATH, 
                    _G_iTmrfdDrvNum, DT_CHR) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: timerfd_create
** 功能描述: 打开 timerfd 文件
** 输　入  : clockid       时钟源 CLOCK_REALTIME / CLOCK_MONOTONIC
**           flags         打开标志 TFD_NONBLOCK / TFD_CLOEXEC
** 输　出  : 文件描述符
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  timerfd_create (int clockid, int flags)
{
    INT             iFd;
    INT             iError;
    PLW_TMRFD_FILE  ptmrfdfil;
    
    flags &= (TFD_NONBLOCK | TFD_CLOEXEC);
    
    iFd = open(LW_TMRFD_DEV_PATH, O_RDONLY | flags);
    if (iFd >= 0) {
        ptmrfdfil = (PLW_TMRFD_FILE)API_IosFdValue(iFd);
        iError = timer_create_internal(clockid, LW_NULL, 
                                       &ptmrfdfil->TF_timer, 
                                       LW_OPTION_OBJECT_GLOBAL);        /*  timer 必须为全局对象        */
        if (iError) {
            close(iFd);
            return  (PX_ERROR);
        }
        timer_setfile(ptmrfdfil->TF_timer, (PVOID)ptmrfdfil);
        ptmrfdfil->TF_uiMagic = LW_TIMER_FILE_MAGIC;                    /*  初始化完毕                  */
    }
    
    return  (iFd);
}
/*********************************************************************************************************
** 函数名称: timerfd_settime
** 功能描述: 设置 timerfd 文件定时时间
** 输　入  : fd            文件描述符
**           flags         设置标志 0 / TFD_TIMER_ABSTIME
**           ntmr          新的定时时间
**           otmr          获取先前的定时时间
** 输　出  : 0 : 成功  -1 : 失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  timerfd_settime (int fd, int flags, const struct itimerspec *ntmr, struct itimerspec *otmr)
{
    PLW_TMRFD_FILE  ptmrfdfil;
    
    if (!ntmr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if ((flags != 0) && (flags != TFD_TIMER_ABSTIME)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (fd < 0) {
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    ptmrfdfil = (PLW_TMRFD_FILE)API_IosFdValue(fd);
    if (!ptmrfdfil || (ptmrfdfil->TF_uiMagic != LW_TIMER_FILE_MAGIC)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    API_SemaphoreBClear(ptmrfdfil->TF_ulReadLock);
    
    return  (timer_settime(ptmrfdfil->TF_timer, flags, ntmr, otmr));
}
/*********************************************************************************************************
** 函数名称: timerfd_gettime
** 功能描述: 获取 timerfd 文件定时时间
** 输　入  : fd            文件描述符
**           currvalue     获取当前的定时时间
** 输　出  : 0 : 成功  -1 : 失败
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  timerfd_gettime (int fd, struct itimerspec *currvalue)
{
    PLW_TMRFD_FILE  ptmrfdfil;
    
    if (fd < 0) {
        _ErrorHandle(EBADF);
        return  (PX_ERROR);
    }
    
    ptmrfdfil = (PLW_TMRFD_FILE)API_IosFdValue(fd);
    if (!ptmrfdfil || (ptmrfdfil->TF_uiMagic != LW_TIMER_FILE_MAGIC)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    return  (timer_gettime(ptmrfdfil->TF_timer, currvalue));
}
/*********************************************************************************************************
** 函数名称: _tmrfdCallback
** 功能描述: tmrfd 文件到时回调
** 输　入  : ptmr             定时器结构
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _tmrfdCallback (PLW_CLASS_TIMER   ptmr)
{
    PLW_TMRFD_FILE  ptmrfdfil = (PLW_TMRFD_FILE)ptmr->TIMER_pvTimerfd;
    
    API_SemaphoreBPost(ptmrfdfil->TF_ulReadLock);
    SEL_WAKE_UP_ALL(&ptmrfdfil->TF_selwulist, SELREAD);
}
/*********************************************************************************************************
** 函数名称: _tmrfdOpen
** 功能描述: 打开 timerfd 设备
** 输　入  : ptmrfddev        timerfd 设备
**           pcName           名称
**           iFlags           方式
**           iMode            方法
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG  _tmrfdOpen (PLW_TMRFD_DEV ptmrfddev, 
                         PCHAR         pcName,
                         INT           iFlags, 
                         INT           iMode)
{
    PLW_TMRFD_FILE  ptmrfdfil;

    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & O_CREAT) {
            _ErrorHandle(ERROR_IO_FILE_EXIST);
            return  (PX_ERROR);
        }
        
        ptmrfdfil = (PLW_TMRFD_FILE)__SHEAP_ALLOC(sizeof(LW_TMRFD_FILE));
        if (!ptmrfdfil) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        
        ptmrfdfil->TF_uiMagic    = 0;
        ptmrfdfil->TF_iFlag      = iFlags;
        ptmrfdfil->TF_timer      = (timer_t)0ul;
        ptmrfdfil->TF_ulReadLock = API_SemaphoreBCreate("tmrfd_rlock", LW_FALSE, 
                                                        LW_OPTION_OBJECT_GLOBAL, LW_NULL);
        if (ptmrfdfil->TF_ulReadLock == LW_OBJECT_HANDLE_INVALID) {
            __SHEAP_FREE(ptmrfdfil);
            return  (PX_ERROR);
        }
        
        lib_bzero(&ptmrfdfil->TF_selwulist, sizeof(LW_SEL_WAKEUPLIST));
        ptmrfdfil->TF_selwulist.SELWUL_hListLock = _G_hTmrfdSelMutex;
        
        LW_DEV_INC_USE_COUNT(&_G_tmrfddev.TD_devhdrHdr);
        
        return  ((LONG)ptmrfdfil);
    }
}
/*********************************************************************************************************
** 函数名称: _tmrfdClose
** 功能描述: 关闭 timerfd 文件
** 输　入  : ptmrfdfil         timerfd 文件
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _tmrfdClose (PLW_TMRFD_FILE  ptmrfdfil)
{
    if (ptmrfdfil) {
        SEL_WAKE_UP_TERM(&ptmrfdfil->TF_selwulist);
    
        LW_DEV_DEC_USE_COUNT(&_G_tmrfddev.TD_devhdrHdr);
        
        if (ptmrfdfil->TF_timer) {
            timer_delete(ptmrfdfil->TF_timer);
            ptmrfdfil->TF_timer = (timer_t)0ul;
        }
        
        API_SemaphoreBDelete(&ptmrfdfil->TF_ulReadLock);
        __SHEAP_FREE(ptmrfdfil);
        
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: _tmrfdRead
** 功能描述: 读 timerfd 设备
** 输　入  : ptmrfdfil        timerfd 文件
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  _tmrfdRead (PLW_TMRFD_FILE  ptmrfdfil, 
                            PCHAR           pcBuffer, 
                            size_t          stMaxBytes)
{
    ULONG   ulLwErrCode;
    UINT64  u64Overruns;
    ULONG   ulTimeout;

    if (!pcBuffer || (stMaxBytes < sizeof(UINT64))) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  是否在中断中调用            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);                                             /*  不能在中断中调用            */
    }
    
    if (ptmrfdfil->TF_timer == (timer_t)0ul) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (ptmrfdfil->TF_iFlag & O_NONBLOCK) {
        ulTimeout = LW_OPTION_NOT_WAIT;
    } else {
        ulTimeout = LW_OPTION_WAIT_INFINITE;
    }
    
    ulLwErrCode = API_SemaphoreBPend(ptmrfdfil->TF_ulReadLock, ulTimeout);
    if (ulLwErrCode != ERROR_NONE) {                                    /*  超时                        */
        _ErrorHandle(EAGAIN);
        return  (0);
    }
    
    if (timer_getoverrun_64(ptmrfdfil->TF_timer, &u64Overruns, LW_TRUE)) {
        return  (PX_ERROR);
    }
    
    lib_memcpy(pcBuffer, &u64Overruns, sizeof(UINT64));
    
    return  (sizeof(UINT64));
}
/*********************************************************************************************************
** 函数名称: _tmrfdIoctl
** 功能描述: 控制 timerfd 文件
** 输　入  : ptmrfdfil        timerfd 文件
**           iRequest         功能
**           lArg             参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _tmrfdIoctl (PLW_TMRFD_FILE  ptmrfdfil, 
                         INT             iRequest, 
                         LONG            lArg)
{
    struct stat         *pstatGet;
    PLW_SEL_WAKEUPNODE   pselwunNode;
    BOOL                 bExpired = LW_FALSE;
    
    switch (iRequest) {
    
    case FIONBIO:
        if (*(INT *)lArg) {
            ptmrfdfil->TF_iFlag |= O_NONBLOCK;
        } else {
            ptmrfdfil->TF_iFlag &= ~O_NONBLOCK;
        }
        break;
        
    case FIOFSTATGET:
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev     = (dev_t)&_G_tmrfddev;
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
        SEL_WAKE_NODE_ADD(&ptmrfdfil->TF_selwulist, pselwunNode);
        
        switch (pselwunNode->SELWUN_seltypType) {
        
        case SELREAD:
            API_SemaphoreBStatus(ptmrfdfil->TF_ulReadLock, &bExpired, LW_NULL, LW_NULL);
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
        SEL_WAKE_NODE_DELETE(&ptmrfdfil->TF_selwulist, (PLW_SEL_WAKEUPNODE)lArg);
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

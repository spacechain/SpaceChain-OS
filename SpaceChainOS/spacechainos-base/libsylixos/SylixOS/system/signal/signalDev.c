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
** 文   件   名: signalDev.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 11 月 21 日
**
** 描        述: Linux 兼容 signalfd 实现.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if (LW_CFG_SIGNAL_EN > 0) && (LW_CFG_SIGNALFD_EN > 0)
#include "sys/signalfd.h"
#include "signalPrivate.h"
/*********************************************************************************************************
  信号内部函数
*********************************************************************************************************/
extern PLW_CLASS_SIGCONTEXT _signalGetCtx(PLW_CLASS_TCB  ptcb);
extern INT                  _sigPendGet(PLW_CLASS_SIGCONTEXT  psigctx, 
                                        const sigset_t       *psigset, 
                                        struct siginfo       *psiginfo);
/*********************************************************************************************************
  驱动程序全局变量
*********************************************************************************************************/
static INT              _G_iSigfdDrvNum = PX_ERROR;
static LW_SIGFD_DEV     _G_sigfddev;
/*********************************************************************************************************
  驱动程序
*********************************************************************************************************/
static LONG     _sigfdOpen(PLW_SIGFD_DEV    psigfddev, PCHAR  pcName, INT  iFlags, INT  iMode);
static INT      _sigfdClose(PLW_SIGFD_FILE  psigfdfil);
static ssize_t  _sigfdRead(PLW_SIGFD_FILE   psigfdfil, PCHAR  pcBuffer, size_t  stMaxBytes);
static INT      _sigfdIoctl(PLW_SIGFD_FILE  psigfdfil, INT    iRequest, LONG    lArg);
/*********************************************************************************************************
** 函数名称: API_SignalfdDrvInstall
** 功能描述: 安装 signalfd 设备驱动程序
** 输　入  : NONE
** 输　出  : 驱动是否安装成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SignalfdDrvInstall (VOID)
{
    if (_G_iSigfdDrvNum <= 0) {
        _G_iSigfdDrvNum  = iosDrvInstall(LW_NULL,
                                         LW_NULL,
                                         _sigfdOpen,
                                         _sigfdClose,
                                         _sigfdRead,
                                         LW_NULL,
                                         _sigfdIoctl);
        DRIVER_LICENSE(_G_iSigfdDrvNum,     "GPL->Ver 2.0");
        DRIVER_AUTHOR(_G_iSigfdDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iSigfdDrvNum, "signalfd driver.");
    }
    
    return  ((_G_iSigfdDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** 函数名称: API_SignalfdDevCreate
** 功能描述: 安装 signalfd 设备
** 输　入  : NONE
** 输　出  : 设备是否创建成功
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_SignalfdDevCreate (VOID)
{
    if (_G_iSigfdDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&_G_sigfddev.SD_devhdrHdr, LW_SIGFD_DEV_PATH, 
                    _G_iSigfdDrvNum, DT_CHR) != ERROR_NONE) {
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: signalfd
** 功能描述: 打开/修改 signalfd 文件
** 输　入  : fd    < 0 表示新建 signal 文件
**                 >=0 表示修改之前创建 signal 文件的掩码
**           mask  信号等待掩码
**           flags 创建标志 SFD_CLOEXEC, SFD_NONBLOCK
** 输　出  : 文件描述符
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
int  signalfd (int fd, const sigset_t *mask, int flags)
{
    INT             iFd;
    PLW_SIGFD_FILE  psigfdfil;
    
    flags &= (SFD_CLOEXEC | SFD_NONBLOCK);
    
    if (fd < 0) {
        iFd = open(LW_SIGFD_DEV_PATH, O_RDONLY | flags);
    } else {
        iFd = fd;
    }
    
    if (iFd >= 0) {
        psigfdfil = (PLW_SIGFD_FILE)API_IosFdValue(iFd);
        if (!psigfdfil || (psigfdfil->SF_uiMagic != LW_SIGNAL_FILE_MAGIC)) {
            _ErrorHandle(EBADF);
            return  (PX_ERROR);
        }
        if (mask) {
            psigfdfil->SF_sigsetMask = *mask;
        }
    }
    
    return  (iFd);
}
/*********************************************************************************************************
** 函数名称: _sigfdOpen
** 功能描述: 打开 signalfd 设备
** 输　入  : psigfddev        signalfd 设备
**           pcName           名称
**           iFlags           方式
**           iMode            方法
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG  _sigfdOpen (PLW_SIGFD_DEV psigfddev, 
                         PCHAR         pcName,
                         INT           iFlags, 
                         INT           iMode)
{
    PLW_SIGFD_FILE  psigfdfil;
    
    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & O_CREAT) {
            _ErrorHandle(ERROR_IO_FILE_EXIST);
            return  (PX_ERROR);
        }
        
        psigfdfil = (PLW_SIGFD_FILE)__SHEAP_ALLOC(sizeof(LW_SIGFD_FILE));
        if (!psigfdfil) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        
        psigfdfil->SF_uiMagic    = LW_SIGNAL_FILE_MAGIC;
        psigfdfil->SF_iFlag      = iFlags;
        psigfdfil->SF_sigsetMask = 0ull;
        
        LW_DEV_INC_USE_COUNT(&_G_sigfddev.SD_devhdrHdr);
        
        return  ((LONG)psigfdfil);
    }
}
/*********************************************************************************************************
** 函数名称: _sigfdClose
** 功能描述: 关闭 signalfd 文件
** 输　入  : psigfdfil         signalfd 文件
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _sigfdClose (PLW_SIGFD_FILE  psigfdfil)
{
    if (psigfdfil) {
        LW_DEV_DEC_USE_COUNT(&_G_sigfddev.SD_devhdrHdr);
        
        __SHEAP_FREE(psigfdfil);
        
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: _sigfdInfo2FdInfor
** 功能描述: 将 siginfo 转换为 signalfd_siginfo
** 输　入  : iSigNo           信号
**           psiginfo         siginfo
**           psigfdinfo       signalfd_siginfo
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _sigfdInfo2FdInfor (INT                      iSigNo, 
                                 const struct siginfo     *psiginfo, 
                                 struct signalfd_siginfo  *psigfdinfo)
{
    lib_bzero(psigfdinfo, sizeof(struct signalfd_siginfo));

    psigfdinfo->ssi_signo   = (uint32_t)iSigNo;
    psigfdinfo->ssi_errno   = (int32_t) psiginfo->si_errno;
    psigfdinfo->ssi_code    = (int32_t) psiginfo->si_code;
    psigfdinfo->ssi_pid     = (uint32_t)psiginfo->si_pid;
    psigfdinfo->ssi_uid     = (uint32_t)psiginfo->si_uid;
    psigfdinfo->ssi_fd      = (int32_t) psiginfo->si_fd;
    psigfdinfo->ssi_tid     = (uint32_t)psiginfo->si_timerid;
    psigfdinfo->ssi_band    = (uint32_t)psiginfo->si_band;
    psigfdinfo->ssi_overrun = (uint32_t)psiginfo->si_overrun;
    psigfdinfo->ssi_trapno  = 0;
    psigfdinfo->ssi_status  = (int32_t) psiginfo->si_status;
    psigfdinfo->ssi_int     = (int32_t) psiginfo->si_int;
    psigfdinfo->ssi_ptr     = (uint64_t)(size_t)psiginfo->si_ptr;
    psigfdinfo->ssi_utime   = (uint64_t)psiginfo->si_utime;
    psigfdinfo->ssi_stime   = (uint64_t)psiginfo->si_stime;
    psigfdinfo->ssi_addr    = (uint64_t)(size_t)psiginfo->si_addr;
}
/*********************************************************************************************************
** 函数名称: _sigfdReadBlock
** 功能描述: 读 signalfd 设备阻塞
** 输　入  : psigfdfil        signalfd 文件
**           psigctx          信号任务上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _sigfdReadBlock (PLW_CLASS_TCB  ptcbCur, PLW_CLASS_SIGCONTEXT  psigctx)
{
    INTREG             iregInterLevel;
    PLW_CLASS_PCB      ppcb;
    
    iregInterLevel = __KERNEL_ENTER_IRQ();
    ptcbCur->TCB_usStatus |= LW_THREAD_STATUS_SIGNAL;
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  从就绪表中删除              */
    __KERNEL_EXIT_IRQ(iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: _sigfdReadUnblock
** 功能描述: 解除读 select signalfd 设备阻塞
** 输　入  : ulId             线程 ID
**           iSigNo           信号
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _sigfdReadUnblock (LW_OBJECT_HANDLE  ulId, INT  iSigNo)
{
    INTREG                  iregInterLevel;
    UINT16                  usIndex;
    PLW_CLASS_TCB           ptcb;
    PLW_CLASS_PCB           ppcb;
    PLW_CLASS_SIGCONTEXT    psigctx;
    
    usIndex = _ObjectGetIndex(ulId);
    
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  检查 ID 类型有效性          */
        return;
    }
    if (_Thread_Index_Invalid(usIndex)) {                               /*  检查线程有效性              */
        return;
    }
    
    iregInterLevel = __KERNEL_ENTER_IRQ();
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT_IRQ(iregInterLevel);
        return;
    }
    
    ptcb    = __GET_TCB_FROM_INDEX(usIndex);
    psigctx = _signalGetCtx(ptcb);
    if (psigctx->SIGCTX_bRead) {                                        /*  是否正在读                  */
        ppcb = _GetPcb(ptcb);
        if (ptcb->TCB_usStatus) {
            ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_SIGNAL);
            if (__LW_THREAD_IS_READY(ptcb)) {
                ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT;
                __ADD_TO_READY_RING(ptcb, ppcb);                        /*  加入就绪环                  */
            }
        }
    }
    __KERNEL_EXIT_IRQ(iregInterLevel);
    
    if (psigctx->SIGCTX_sigsetWait & __sigmask(iSigNo)) {
        SEL_WAKE_UP_ALL(&psigctx->SIGCTX_selwulist, SELREAD);           /*  符合条件则唤醒 select       */
    }
}
/*********************************************************************************************************
** 函数名称: _sigfdPendGet
** 功能描述: 获得 pend 信号信息
** 输　入  : psigfdfil        signalfd 文件
**           psigctx          信号任务上下文
**           psigfdinfo       signalfd 信号信息
**           stMaxBytes       缓冲区大小字节数
**           pstGetNum        返回获取的信号信息个数
** 输　出  : 返回获取的总字节数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  _sigfdPendGet (PLW_SIGFD_FILE           psigfdfil,
                               PLW_CLASS_SIGCONTEXT     psigctx, 
                               struct signalfd_siginfo *psigfdinfo,
                               size_t                   stMaxBytes,
                               size_t                  *pstGetNum)
{
    INT             iSigNo;
    ssize_t         sstRetVal = 0;
    struct siginfo  siginfo;
    
    *pstGetNum = 0;

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    for (;;) {
        iSigNo = _sigPendGet(psigctx, &psigfdfil->SF_sigsetMask, &siginfo);
        if (__issig(iSigNo)) {                                          /*  存在被阻塞的信号            */
            _sigfdInfo2FdInfor(iSigNo, &siginfo, psigfdinfo);
            sstRetVal += sizeof(struct signalfd_siginfo);
            psigfdinfo++;
            (*pstGetNum)++;
            if ((stMaxBytes - sstRetVal) < sizeof(struct signalfd_siginfo)) {
                break;
            }
        } else {
            break;
        }
    }
    __KERNEL_EXIT();                                                    /*  退出内核                    */
    
    return  (sstRetVal);
}
/*********************************************************************************************************
** 函数名称: _sigfdRead
** 功能描述: 读 signalfd 设备
** 输　入  : psigfdfil        signalfd 文件
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ssize_t  _sigfdRead (PLW_SIGFD_FILE  psigfdfil, 
                            PCHAR           pcBuffer, 
                            size_t          stMaxBytes)
{
    PLW_CLASS_TCB               ptcbCur;
    PLW_CLASS_SIGCONTEXT        psigctx;
    struct signalfd_siginfo    *psigfdinfo = (struct signalfd_siginfo *)pcBuffer;
    
    size_t                      stGetNum;
    ssize_t                     sstRetVal = 0;
    
    if (!pcBuffer || (stMaxBytes < sizeof(struct signalfd_siginfo))) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psigctx = _signalGetCtx(ptcbCur);
    
    psigctx->SIGCTX_bRead = LW_TRUE;                                    /*  进入 read 过程              */
    
    for (;;) {
        sstRetVal += _sigfdPendGet(psigfdfil, psigctx, psigfdinfo, stMaxBytes, &stGetNum);
        if (stGetNum) {
            psigfdinfo += stGetNum;
            stMaxBytes -= (stGetNum * sizeof(struct signalfd_siginfo));
        }
        
        if (sstRetVal) {
            break;
        
        } else {
            if (psigfdfil->SF_iFlag & O_NONBLOCK) {
                _ErrorHandle(EAGAIN);
                break;
            
            } else {
                _sigfdReadBlock(ptcbCur, psigctx);
            }
        }
    }
    
    psigctx->SIGCTX_bRead = LW_FALSE;                                   /*  退出 read 过程              */

    return  (sstRetVal);
}
/*********************************************************************************************************
** 函数名称: _sigfdSelect
** 功能描述: signalfd FIOSELECT
** 输　入  : psigfdfil        signalfd 文件
**           pselwunNode      select 节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _sigfdSelect (PLW_SIGFD_FILE  psigfdfil, PLW_SEL_WAKEUPNODE   pselwunNode)
{
    BOOL                 bHaveSigPend = LW_FALSE;
    PLW_CLASS_TCB        ptcbCur;
    PLW_CLASS_SIGCONTEXT psigctx;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psigctx = _signalGetCtx(ptcbCur);
    
    SEL_WAKE_NODE_ADD(&psigctx->SIGCTX_selwulist, pselwunNode);
    
    switch (pselwunNode->SELWUN_seltypType) {
    
    case SELREAD:
        psigctx->SIGCTX_sigsetWait = psigfdfil->SF_sigsetMask;          /*  设置唤醒条件                */
        __KERNEL_ENTER();                                               /*  进入内核                    */
        if (psigctx->SIGCTX_sigsetPending & psigfdfil->SF_sigsetMask) {
            bHaveSigPend = LW_TRUE;
        }
        __KERNEL_EXIT();                                                /*  退出内核                    */
        if (bHaveSigPend) {
            SEL_WAKE_UP(pselwunNode);
            psigctx->SIGCTX_sigsetWait = 0ull;                          /*  已经满足条件, 不需要唤醒    */
        }
        break;
        
    case SELWRITE:
    case SELEXCEPT:
        break;
    }
}
/*********************************************************************************************************
** 函数名称: _sigfdUnselect
** 功能描述: signalfd FIOUNSELECT
** 输　入  : psigfdfil        signalfd 文件
**           pselwunNode      select 节点
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _sigfdUnselect (PLW_SIGFD_FILE  psigfdfil, PLW_SEL_WAKEUPNODE   pselwunNode)
{
    PLW_CLASS_TCB        ptcbCur;
    PLW_CLASS_SIGCONTEXT psigctx;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    psigctx = _signalGetCtx(ptcbCur);
    
    SEL_WAKE_NODE_DELETE(&psigctx->SIGCTX_selwulist, pselwunNode);
    
    psigctx->SIGCTX_sigsetWait = 0ull;                                  /*  不需要 select 唤醒          */
}
/*********************************************************************************************************
** 函数名称: _sigfdIoctl
** 功能描述: 控制 signalfd 文件
** 输　入  : psigfdfil        signalfd 文件
**           iRequest         功能
**           lArg             参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  _sigfdIoctl (PLW_SIGFD_FILE  psigfdfil, 
                         INT             iRequest, 
                         LONG            lArg)
{
    struct stat         *pstatGet;
    PLW_SEL_WAKEUPNODE   pselwunNode;
    
    switch (iRequest) {
    
    case FIONBIO:
        if (*(INT *)lArg) {
            psigfdfil->SF_iFlag |= O_NONBLOCK;
        } else {
            psigfdfil->SF_iFlag &= ~O_NONBLOCK;
        }
        break;
        
    case FIOFSTATGET:
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev     = (dev_t)&_G_sigfddev;
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
        _sigfdSelect(psigfdfil, pselwunNode);
        break;
        
    case FIOUNSELECT:
        pselwunNode = (PLW_SEL_WAKEUPNODE)lArg;
        _sigfdUnselect(psigfdfil, pselwunNode);
        break;
        
    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
                                                                        /*  LW_CFG_SIGNALFD_EN > 0      */
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: tty.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 02 月 04 日
**
** 描        述: TTY 设备接口.

** BUG
2008.03.24  将打开次数的计数器放在设备头中.
2008.06.15  在文件关闭的时候, 激活 select() 等待异常的线程.
2008.09.27  API_TtyDevCreate() 对 SIO_CHAN 的有效性判断有错.
2009.10.02  _ttyOpen() 只有在第一次打开时调用 SIO_OPEN 命令.
2010.02.03  加入对 SMP 的支持.
2010.07.12  _ttyClose() 返回值为 ERROR_NONE.
2010.09.11  创建设备时, 指定设备类型.
2012.08.06  加入了 tty 设备删除的操作.
2012.08.10  第一次打开设备时需要清除缓冲区.
2012.12.17  tty 设备不允许重复打开, 设备驱动通过 SIO_OPEN 和 SIO_HUP 来判断接口打开和关闭.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SIO_DEVICE_EN > 0)
/*********************************************************************************************************
  是否允许重复打开
*********************************************************************************************************/
#define __LW_TTY_OPEN_REPEATEDLY_EN     1
/*********************************************************************************************************
  LOCAL FUNC
*********************************************************************************************************/
static LONG   _ttyOpen(TYCO_DEV    *ptycoDev,
                       PCHAR        pcName,
                       INT          iFlags,
                       INT          iMode);
static INT    _ttyClose(TYCO_DEV   *ptycoDev);
static INT    _ttyIoctl(TYCO_DEV   *ptycoDev,
                        INT         iRequest,
                        PVOID       pvArg);
static VOID   _ttyStartup(TYCO_DEV *ptycoDev);
/*********************************************************************************************************
** 函数名称: API_TtyDrvInstall
** 功能描述: 安装TTY设备驱动程序
** 输　入  : VOID
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_TtyDrvInstall (VOID)
{
    if (_G_iTycoDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    _G_iTycoDrvNum = iosDrvInstall(_ttyOpen, 
                                   (FUNCPTR)LW_NULL, 
                                   _ttyOpen,
                                   _ttyClose,
                                   _TyRead,
                                   _TyWrite,
                                   _ttyIoctl);
                                    
    DRIVER_LICENSE(_G_iTycoDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iTycoDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iTycoDrvNum, "tty driver.");
    
    return  ((_G_iTycoDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** 函数名称: API_TtyDevCreate
** 功能描述: 建立一个 TTY 设备
** 输　入  : 
**           pcName,                       设备名
**           psiochan,                     同步 I/O 函数集
**           stRdBufSize,                  输入缓冲区大小
**           stWrtBufSize                  输出缓冲区大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_TtyDevCreate (PCHAR     pcName,
                       SIO_CHAN *psiochan,
                       size_t    stRdBufSize,
                       size_t    stWrtBufSize)
{
    REGISTER INT           iTemp;
    REGISTER TYCO_DEV     *ptycoDev;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    if (_G_iTycoDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "tty Driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    if (psiochan == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "SIO channel invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
    
    if ((pcName == LW_NULL) || (*pcName == PX_EOS)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    ptycoDev = (TYCO_DEV *)__SHEAP_ALLOC(sizeof(TYCO_DEV));             /*  分配内存                    */
    if (ptycoDev == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    iTemp = _TyDevInit(&ptycoDev->TYCODEV_tydevTyDev, 
                       stRdBufSize, 
                       stWrtBufSize,
                       (FUNCPTR)_ttyStartup);                           /*  初始化设备控制块            */
    
    if (iTemp != ERROR_NONE) {
        __SHEAP_FREE(ptycoDev);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    ptycoDev->TYCODEV_psiochan = psiochan;
    
    sioCallbackInstall(psiochan, SIO_CALLBACK_GET_TX_CHAR,
            (VX_SIO_CALLBACK)_TyITx, (PVOID)ptycoDev);
    sioCallbackInstall(psiochan, SIO_CALLBACK_PUT_RCV_CHAR,
            (VX_SIO_CALLBACK)_TyIRd, (PVOID)ptycoDev);
    
    sioIoctl(psiochan, SIO_MODE_SET, (PVOID)SIO_MODE_INT);
    
    iTemp = (INT)iosDevAddEx(&ptycoDev->TYCODEV_tydevTyDev.TYDEV_devhdrHdr, 
                             pcName, 
                             _G_iTycoDrvNum,
                             DT_CHR);
    if (iTemp) {
        _TyDevRemove(&ptycoDev->TYCODEV_tydevTyDev);
        __SHEAP_FREE(ptycoDev);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "add device error.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    
    } else {
        ptycoDev->TYCODEV_tydevTyDev.TYDEV_timeCreate = lib_time(LW_NULL);
                                                                        /*  记录创建时间 (UTC 时间)     */
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: API_TtyDevRemove
** 功能描述: 删除 TTY 设备.
** 输　入  : pcName,                       设备名, 等同于 创建时的设备名
**           bForce                        是否强制删除
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
** 注  意  : 对 tty 设备的卸载强烈的不建议使用 bForce 接口, 有可能会造成系统崩溃, 所以, 在最后一次关闭文件
             系统会 ioctl(SIO_HUP), 这个时侯才可以卸载设备 

                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_TtyDevRemove (PCHAR   pcName, BOOL  bForce)
{
    REGISTER TYCO_DEV     *ptycoDev;
             TY_DEV_ID     ptyDev;
             PCHAR         pcTail = LW_NULL;

    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    if (!pcName) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    if (_G_iTycoDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "tty Driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    ptycoDev = (TYCO_DEV *)iosDevFind(pcName, &pcTail);
    if ((ptycoDev == LW_NULL) || (pcName == pcTail)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device not found.\r\n");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
    
    ptyDev = &ptycoDev->TYCODEV_tydevTyDev;
    
    if (bForce == LW_FALSE) {
        if (LW_DEV_GET_USE_COUNT(&ptyDev->TYDEV_devhdrHdr)) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "too many open files.\r\n");
            _ErrorHandle(EBUSY);
            return  (PX_ERROR);
        }
        if (SEL_WAKE_UP_LIST_LEN(&ptyDev->TYDEV_selwulList) > 0) {
            errno = EBUSY;
            return  (PX_ERROR);
        }
    }
    
    iosDevFileAbnormal(&ptyDev->TYDEV_devhdrHdr);                       /*  将所有打开的文件设为异常    */
    
    iosDevDelete(&ptyDev->TYDEV_devhdrHdr);
    
    _TyDevRemove(&ptycoDev->TYCODEV_tydevTyDev);
    
    __SHEAP_FREE(ptycoDev);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _ttyOpen
** 功能描述: 打开一个 TTY 设备 (只能打开一次)
** 输　入  : 
**           ptycoDev,                     tty 设备控制块
**           pcName,                       设备名
**           iFlags,                       O_RDONLY O_WRONLY O_RDWR O_CREAT ...
**           iMode                         保留
** 输　出  : 控制块指针
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG   _ttyOpen (TYCO_DEV    *ptycoDev,
                        PCHAR        pcName,
                        INT          iFlags,
                        INT          iMode)
{
    INT     iError;

    if (LW_DEV_INC_USE_COUNT(&ptycoDev->TYCODEV_tydevTyDev.TYDEV_devhdrHdr) == 1) {
        iError = sioIoctl(ptycoDev->TYCODEV_psiochan, SIO_OPEN, LW_NULL);
        if (iError < 0) {                                               /*  打开端口                    */
            LW_DEV_DEC_USE_COUNT(&ptycoDev->TYCODEV_tydevTyDev.TYDEV_devhdrHdr);
            return  (PX_ERROR);                                         /*  无法打开端口                */
        }
        sioIoctl(ptycoDev->TYCODEV_psiochan, FIOFLUSH, LW_NULL);        /*  清除缓冲区                  */
    
        if (iFlags & O_NOCTTY) {                                        /*  不支持控制指令              */
            INT   iOpt = OPT_TERMINAL;
            _TyIoctl(&ptycoDev->TYCODEV_tydevTyDev, FIOGETOPTIONS, (LONG)&iOpt);
            iOpt &= ~(OPT_ABORT | OPT_MON_TRAP);
            _TyIoctl(&ptycoDev->TYCODEV_tydevTyDev, FIOSETOPTIONS, iOpt);
        }
        
        if (iFlags & O_NONBLOCK) {                                      /*  读写非阻塞                  */
            ptycoDev->TYCODEV_tydevTyDev.TYDEV_ulRTimeout = LW_OPTION_NOT_WAIT;
            ptycoDev->TYCODEV_tydevTyDev.TYDEV_ulWTimeout = LW_OPTION_NOT_WAIT;
        }
    }
#if __LW_TTY_OPEN_REPEATEDLY_EN == 0
      else {
        LW_DEV_DEC_USE_COUNT(&ptycoDev->TYCODEV_tydevTyDev.TYDEV_devhdrHdr);
        _ErrorHandle(EBUSY);                                            /*  只允许打开一次              */
        return  (PX_ERROR);
    }
#endif                                                                  /*  __LW_TTY_OPEN_REPEATEDLY_EN */
    
    return  ((LONG)ptycoDev);
}
/*********************************************************************************************************
** 函数名称: _ttyClose
** 功能描述: 关闭一个 TTY 设备 (SIO_HUP 系统挂起端口后, 设备可能会被驱动程序删除)
** 输　入  : 
**           ptycoDev,                     tty 设备控制块
** 输　出  : 控制块指针
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT    _ttyClose (TYCO_DEV   *ptycoDev)
{
    if (LW_DEV_GET_USE_COUNT(&ptycoDev->TYCODEV_tydevTyDev.TYDEV_devhdrHdr)) {
        if (!LW_DEV_DEC_USE_COUNT(&ptycoDev->TYCODEV_tydevTyDev.TYDEV_devhdrHdr)) {
            SEL_WAKE_UP_ALL(&ptycoDev->TYCODEV_tydevTyDev.TYDEV_selwulList, 
                            SELEXCEPT);                                 /*  激活异常等待                */
            sioIoctl(ptycoDev->TYCODEV_psiochan, SIO_HUP, LW_NULL);     /*  挂起端口                    */
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _ttyClose
** 功能描述: 关闭一个 TTY 设备
** 输　入  : 
**           ptycoDev,                     tty 设备控制块
**           iRequest                      请求命令
**           pvArg                         命令参数
** 输　出  : 命令返回值.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT    _ttyIoctl (TYCO_DEV   *ptycoDev,
                         INT         iRequest,
                         PVOID       pvArg)
{
    REGISTER INT       iStatus;
    
    if (iRequest == SIO_HUP) {
        _ErrorHandle(EBUSY);                                            /*  正在打开的设备不允许挂起    */
        return  (PX_ERROR);
    }
    
    if (iRequest == FIOBAUDRATE) {                                      /*  设置波特率                  */
        iStatus = sioIoctl(ptycoDev->TYCODEV_psiochan, SIO_BAUD_SET, pvArg);
        if (iStatus == ERROR_NONE) { 
            return  (ERROR_NONE);
        } else {
            return  (PX_ERROR);
        }
    }
    
    iStatus = sioIoctl(ptycoDev->TYCODEV_psiochan, iRequest, pvArg);    /*  执行命令                    */
    
    if ((iStatus == ENOSYS) || 
        ((iStatus == PX_ERROR) && (errno == ENOSYS))) {                 /*  驱动程序无法识别的命令      */
        return  (_TyIoctl(&ptycoDev->TYCODEV_tydevTyDev, 
                          iRequest, 
                          (LONG)pvArg));                                /*  执行 TY 库命令              */
    }
    
    return  (iStatus);
}
/*********************************************************************************************************
** 函数名称: _ttyStartup
** 功能描述: 启动 TTY 设备发送功能
** 输　入  : 
**           ptycoDev,                     tty 设备控制块
** 输　出  : NONE.
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID   _ttyStartup (TYCO_DEV  *ptycoDev)
{
    sioTxStartup(ptycoDev->TYCODEV_psiochan);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SIO_DEVICE_EN > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

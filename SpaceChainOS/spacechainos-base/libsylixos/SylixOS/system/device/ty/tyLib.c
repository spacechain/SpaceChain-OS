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
** 文   件   名: tyLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 02 月 04 日
**
** 描        述: TTY 设备内部库.
**
** 注        意: 初始化时 VTIME=0 和 VMIN=1, 如果重新设置了此参数, 则针对 FIORTIMEOUT FIOWTIMEOUT 将无效.

** BUG
2008.04.01  将 FIORTIMEOUT 和 FIOWTIMEOUT 设置超时时间的参数改为 struct timeval 类型.
            为 NULL 表示永久等待.
2008.05.31  ioctl 使用 long 型, 支持 64 位系统指针传递.
2008.06.06  修改了一些数据类型.
2009.02.09  ioctl 不可识别命令, 不打印错误信息.
2009.03.10  FIOISATTY 命令使用 lArg 作为参数.
2009.05.21  _TyDevRemove 没有删除锁信号量.
2009.05.27  加入 abort 功能.
2009.06.09  修正一处 BUG, 可能导致从本地 XOFF无法回到 XON 模式(缓冲区无法释放).
2009.07.15  OPT_CRMOD 模式下, 当 \r\n 连续到来时, 仅将 \r 变为 \n, 后面的 \n 忽略.
2009.08.15  _TyITx() 任务激活门限加入缓冲区的大小判据.
2009.09.04  中断收发回调需要在处理 ring buffer 时关闭相关中断, 因为这两个函数可能是 PTY 调用, 如果不关闭
            可能会导致缓冲区问题.
2009.09.04  _TyDevRemove() 加入对 select 结构的清除, 同时唤醒等待任务.
2009.09.05  移除 ty 设备时需要将所有打开的文件设置为异常(预关闭模式).
2009.10.22  read write 参数机返回值类型调整.
2009.12.01  终端模式下对 0x00 字符不响应.
2010.01.14  升级 abort.
2010.02.03  加入对 SMP 的支持.
2012.01.10  加入对 termios 的有限支持.
2012.03.04  _TyDrain() 需要先判断发送缓冲区是否为空, 如果不为空, 在等待 drain 信号量.
2012.08.10  pend(..., LW_OPTION_NOT_WAIT) 改为 clear() 操作.
2012.10.17  加入对 TIOCGWINSZ 和 TIOCSWINSZ 的支持.
2012.12.14  修正 Tx 与 Rx 中断回调中 LOCK 的时机, 确保多核无误.
2013.04.01  修正 GCC 4.7.3 引发的新 warning.
2013.06.09  _TyIRd 重启系统时不需要 _execJob 操作.
2013.10.03  ioctl() 更安全的设置缓冲区操作.
            对于一些标志, 加入 SMP 内存屏障操作.
2014.03.03  优化代码.
2014.05.20  tty 设备删除更加安全.
2014.08.03  FIOWBUFSET 时需要激活等待写的线程.
2014.12.08  修正 _TyIRd() 忘记释放 spinlock 错误.
2015.05.07  优化中断程序操作.
2015.12.04  修正 SMP 并发操作问题.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#define  __TYCO_MAIN_FILE
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  termios header file
*********************************************************************************************************/
#include "limits.h"
#include "termios.h"
#include "sys/ioctl.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SIO_DEVICE_EN > 0)
/*********************************************************************************************************
  流控特殊控制信息
*********************************************************************************************************/
#define __XON           0x11                                            /*  ctrl-Q XON  握手            */
#define __XOFF          0x13                                            /*  ctrl-S XOFF 握手            */
/*********************************************************************************************************
  全局变量(控制字 __LINUX_BACKSPACE_CHAR 直接按 backspace 处理)
*********************************************************************************************************/
#define __LINUX_BACKSPACE_CHAR      127                                 /*  linux 下的退格 control-?    */
#define __TTY_BACKSPACE(ptyDev, ch) ((ch) == __LINUX_BACKSPACE_CHAR || (ch) == __TTY_CC(ptyDev, VERASE))

static CHAR             _G_cTyBackspaceChar  = 0x08;                    /*  默认值     control-H        */
static CHAR             _G_cTyDeleteLineChar = 0x15;                    /*  默认值     control-U        */
static CHAR             _G_cTyEofChar        = 0x04;                    /*  默认值     control-D        */
static CHAR             _G_cTyAbortChar      = 0x03;                    /*  默认值     control-C        */
static CHAR             _G_cTyMonTrapChar    = 0x18;                    /*  默认值     control-X        */
/*********************************************************************************************************
  全局变量(控制阀值)
*********************************************************************************************************/
static CHAR             _G_cTyXoffThreshold  = 10;                      /*  使用 OPT_TANDEM 模式时, 当  */
                                                                        /*  输入缓冲区空闲字节数小于这  */
                                                                        /*  个值时发送 XOFF 流控制帧    */
static CHAR             _G_cTyXonThreshold   = 30;                      /*  使用 OPT_TANDEM 模式时, 当  */
                                                                        /*  输入缓冲区空闲字节数超过这  */
                                                                        /*  个值时发送 XON 流控制帧     */
static CHAR             _G_cTyWrtThreshold   = 64;                      /*  当输出缓冲区空闲字节数大于  */
                                                                        /*  这个值, 激活等待写的线程    */
/*********************************************************************************************************
  全局变量(函数指针)
*********************************************************************************************************/
static FUNCPTR          _G_pfuncTyAbortFunc  = LW_NULL;                 /*  收到 control-C 执行的操作   */
/*********************************************************************************************************
  内部统计用变量
*********************************************************************************************************/
static INT              _G_iTyXoffChars = 0;
static INT              _G_iTyXoffMax   = 0;
/*********************************************************************************************************
  特殊控制字符
*********************************************************************************************************/
#define __TTY_CC(ptyDev, c)         (ptyDev)->TYDEV_cCtlChars[c]
/*********************************************************************************************************
  select 支持函数
*********************************************************************************************************/
VOID            __selTyAdd(   TY_DEV_ID   ptyDev, LONG  lArg);          /*  添加一个等待的节点          */
VOID            __selTyDelete(TY_DEV_ID   ptyDev, LONG  lArg);          /*  删除一个等待的节点          */
/*********************************************************************************************************
  内部函数
*********************************************************************************************************/
static VOID     _TyFlush(    TY_DEV_ID  ptyDev);                        /*  清空设备缓冲区              */
static VOID     _TyFlushRd(  TY_DEV_ID  ptyDev);                        /*  清除读缓冲区                */
static VOID     _TyFlushWrt( TY_DEV_ID  ptyDev);                        /*  清除写缓冲区                */
static VOID     _TyTxStartup(TY_DEV_ID  ptyDev);                        /*  启动设备发送功能            */
static VOID     _TyRdXoff(   TY_DEV_ID  ptyDev, BOOL  bXoff);           /*  设置接收端流控状态          */
static VOID     _TyWrtXoff(  TY_DEV_ID  ptyDev, BOOL  bXoff);           /*  设置发送端流控状态          */
/*********************************************************************************************************
  锁
*********************************************************************************************************/
#define TYDEV_LOCK(ptyDev, code)    \
        if (API_SemaphoreMPend((ptyDev)->TYDEV_hMutexSemM, LW_OPTION_WAIT_INFINITE)) {  \
            code;   \
        }
#define TYDEV_UNLOCK(ptyDev)        \
        API_SemaphoreMPost((ptyDev)->TYDEV_hMutexSemM)
/*********************************************************************************************************
  等待中断结束
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#define TYDEV_WAIT_ISR(ptyDev)      \
        do {    \
            INTREG  iregInterLevel; \
            LW_SPIN_LOCK_QUICK(&((ptyDev)->TYDEV_slLock), &iregInterLevel); \
            LW_SPIN_UNLOCK_QUICK(&((ptyDev)->TYDEV_slLock), iregInterLevel);    \
        } while (0)
#else
#define TYDEV_WAIT_ISR(ptyDev)
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
** 函数名称: _TyDevInit
** 功能描述: 初始化一个 TY 设备
** 输　入  : 
**           ptyDev,                  需要初始化的 ty 设备
**           stRdBufSize,             读缓冲区的大小
**           stWrtBufSize,            写缓冲区的大小
**           pfunctxStartup           设备发送启动函数
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _TyDevInit (TY_DEV_ID  ptyDev, 
                 size_t     stRdBufSize,
                 size_t     stWrtBufSize,
                 FUNCPTR    pfunctxStartup)
{
    REGISTER INT    iErrLevel = 0;

    lib_bzero((PVOID)ptyDev, sizeof(TY_DEV));                           /*  清零内存                    */

    ptyDev->TYDEV_ulRTimeout = LW_OPTION_WAIT_INFINITE;                 /*  初始化为永久等待            */
    ptyDev->TYDEV_ulWTimeout = LW_OPTION_WAIT_INFINITE;                 /*  初始化为永久等待            */
    
    ptyDev->TYDEV_vxringidWrBuf = rngCreate((INT)stWrtBufSize);         /*  创建写缓冲区                */
    if (ptyDev->TYDEV_vxringidWrBuf == LW_NULL) {                       /*  创建失败                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    ptyDev->TYDEV_vxringidRdBuf = rngCreate((INT)stRdBufSize);          /*  创建读缓冲区                */
    if (ptyDev->TYDEV_vxringidRdBuf == LW_NULL) {                       /*  创建失败                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        iErrLevel = 1;
        goto    __error_handle;
    }
    
    ptyDev->TYDEV_iWrtThreshold = (INT)(stWrtBufSize > (INT)_G_cTyWrtThreshold)
                                ? ((INT)_G_cTyWrtThreshold) 
                                : (stWrtBufSize);                       /*  确定写激活门限              */
    
    ptyDev->TYDEV_pfuncTxStartup = pfunctxStartup;                      /*  启动函数                    */
    
    ptyDev->TYDEV_hRdSyncSemB = API_SemaphoreBCreate("ty_rsync", 
                                                     LW_FALSE, 
                                                     LW_OPTION_WAIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL, 
                                                     LW_NULL);          /*  读同步                      */
    if (!ptyDev->TYDEV_hRdSyncSemB) {
        iErrLevel = 2;
        goto    __error_handle;
    }
    
    ptyDev->TYDEV_hWrtSyncSemB = API_SemaphoreBCreate("ty_wsync", 
                                                      LW_FALSE, 
                                                      LW_OPTION_WAIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL, 
                                                      LW_NULL);         /*  写同步                      */
    if (!ptyDev->TYDEV_hWrtSyncSemB) {
        iErrLevel = 3;
        goto    __error_handle;
    }
    
    ptyDev->TYDEV_hDrainSyncSemB = API_SemaphoreBCreate("ty_drain",
                                                        LW_FALSE,
                                                        LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL,
                                                        LW_NULL);       /*  传输完毕同步信号量          */
    if (!ptyDev->TYDEV_hDrainSyncSemB) {
        iErrLevel = 4;
        goto    __error_handle;
    }
    
    ptyDev->TYDEV_hMutexSemM = API_SemaphoreMCreate("ty_lock", 
                                                    LW_PRIO_DEF_CEILING, 
                                                    _G_ulMutexOptionsTyLib |
                                                    LW_OPTION_OBJECT_DEBUG_UNPEND |
                                                    LW_OPTION_OBJECT_GLOBAL, 
                                                    LW_NULL);           /*  互斥访问控制信号量          */
    if (!ptyDev->TYDEV_hMutexSemM) {
        iErrLevel = 5;
        goto    __error_handle;
    }
    
    SEL_WAKE_UP_LIST_INIT(&ptyDev->TYDEV_selwulList);                   /*  初始化 select 等待链        */
    
    _TyFlush(ptyDev);                                                   /*  清空设备缓冲区              */

    __TTY_CC(ptyDev, VINTR)  = _G_cTyAbortChar;                         /*  初始化特殊控制字            */
    __TTY_CC(ptyDev, VQUIT)  = _G_cTyMonTrapChar;
    __TTY_CC(ptyDev, VERASE) = _G_cTyBackspaceChar;
    __TTY_CC(ptyDev, VKILL)  = _G_cTyDeleteLineChar;
    __TTY_CC(ptyDev, VEOF)   = _G_cTyEofChar;
    __TTY_CC(ptyDev, VSTART) = __XON;
    __TTY_CC(ptyDev, VSTOP)  = __XOFF;
    
    __TTY_CC(ptyDev, VTIME) = 0;                                        /*  sioLib 默认超时参数         */
    __TTY_CC(ptyDev, VMIN)  = 1;

    ptyDev->TYDEV_tydevwins.TYDEVWINS_usRow = 24;                       /*  默认为 80 * 24              */
    ptyDev->TYDEV_tydevwins.TYDEVWINS_usCol = 80;
    ptyDev->TYDEV_tydevwins.TYDEVWINS_usXPixel = 80 * 8;                /*  默认为 16 * 8 点阵          */
    ptyDev->TYDEV_tydevwins.TYDEVWINS_usYPixel = 24 * 16;

    LW_SPIN_INIT(&ptyDev->TYDEV_slLock);                                /*  初始化自旋锁                */

    return  (ERROR_NONE);
    
__error_handle:
    if (iErrLevel > 4) {
        API_SemaphoreBDelete(&ptyDev->TYDEV_hDrainSyncSemB);
    }
    if (iErrLevel > 3) {
        API_SemaphoreBDelete(&ptyDev->TYDEV_hWrtSyncSemB);              /*  删除写同步                  */
    }
    if (iErrLevel > 2) {
        API_SemaphoreBDelete(&ptyDev->TYDEV_hRdSyncSemB);               /*  删除读同步                  */
    }
    if (iErrLevel > 1) {
        rngDelete(ptyDev->TYDEV_vxringidRdBuf);                         /*  删除读缓冲区                */
    }
    if (iErrLevel > 0) {
        rngDelete(ptyDev->TYDEV_vxringidWrBuf);                         /*  删除写缓冲区                */
    }
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: _TyDevRemove
** 功能描述: 移除一个 TY 设备
** 输　入  : 
**           pstrDev,           需要移除的 ty 设备
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _TyDevRemove (TY_DEV_ID  ptyDev)
{
    INTREG  iregInterLevel;

    TYDEV_LOCK(ptyDev, return (PX_ERROR));                              /*  等待设备使用权              */
    
    ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bCanceled = LW_TRUE;          /*  read cancel                 */
    ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bCanceled = LW_TRUE;          /*  write cancel                */
    
    LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);         /*  锁定 spinlock 并关闭中断    */
    
    ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bFlushingRdBuf  = LW_TRUE;
    ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bFlushingWrtBuf = LW_TRUE;
    
    LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);        /*  解锁 spinlock 并打开中断    */
    
    if (ptyDev->TYDEV_vxringidRdBuf) {
        rngDelete(ptyDev->TYDEV_vxringidRdBuf);                         /*  删除读缓冲区                */
        ptyDev->TYDEV_vxringidRdBuf = LW_NULL;
    }
    if (ptyDev->TYDEV_vxringidWrBuf) {
        rngDelete(ptyDev->TYDEV_vxringidWrBuf);                         /*  删除写缓冲区                */
        ptyDev->TYDEV_vxringidWrBuf = LW_NULL;
    }
    
    SEL_WAKE_UP_LIST_TERM(&ptyDev->TYDEV_selwulList);                   /*  卸载 sel 结构               */
    
    API_SemaphoreBDelete(&ptyDev->TYDEV_hWrtSyncSemB);                  /*  删除写同步                  */
    API_SemaphoreBDelete(&ptyDev->TYDEV_hRdSyncSemB);                   /*  删除读同步                  */
    API_SemaphoreBDelete(&ptyDev->TYDEV_hDrainSyncSemB);                /*  删除传输完成信号            */
    API_SemaphoreMDelete(&ptyDev->TYDEV_hMutexSemM);                    /*  删除互斥信号量              */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: _TyFlush
** 功能描述: 清除 TY 设备的接收发送缓冲区
** 输　入  : 
**           pstrDev,           ty 设备
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _TyFlush (TY_DEV_ID  ptyDev)
{
    _TyFlushRd(ptyDev);
    _TyFlushWrt(ptyDev);
}
/*********************************************************************************************************
** 函数名称: _TyFlushRd
** 功能描述: 清除 TY 设备读缓冲区
** 输　入  : 
**           pstrDev,           ty 设备
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _TyFlushRd (TY_DEV_ID  ptyDev)
{
    INTREG  iregInterLevel;
    
    TYDEV_LOCK(ptyDev, return);                                         /*  等待设备使用权              */
    
    ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bFlushingRdBuf = LW_TRUE;
    
    LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);         /*  锁定 spinlock 并关闭中断    */
    
    rngFlush(ptyDev->TYDEV_vxringidRdBuf);                              /*  清除缓冲区                  */
    
    ptyDev->TYDEV_ucInNBytes    = 0;
    ptyDev->TYDEV_ucInBytesLeft = 0;
    
    LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);        /*  解锁 spinlock 并打开中断    */
    
    _TyRdXoff(ptyDev, LW_FALSE);                                        /*  可以允许对方发送            */
    
    API_SemaphoreBClear(ptyDev->TYDEV_hRdSyncSemB);                     /*  清除读同步                  */
    
    ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bFlushingRdBuf = LW_FALSE;
    KN_SMP_MB();
    
    TYDEV_UNLOCK(ptyDev);                                               /*  释放设备使用权              */
}
/*********************************************************************************************************
** 函数名称: _TyFlushWrt
** 功能描述: 清除 TY 设备写缓冲区
** 输　入  : 
**           pstrDev,           ty 设备
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _TyFlushWrt (TY_DEV_ID  ptyDev)
{
    INTREG  iregInterLevel;
    
    TYDEV_LOCK(ptyDev, return);                                         /*  等待设备使用权              */
    
    ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bFlushingWrtBuf = LW_TRUE;
    
    LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);         /*  锁定 spinlock 并关闭中断    */
    
    rngFlush(ptyDev->TYDEV_vxringidWrBuf);
    
    LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);        /*  解锁 spinlock 并打开中断    */
    
    ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bFlushingWrtBuf = LW_FALSE;
    KN_SMP_MB();
    
    API_SemaphoreBPost(ptyDev->TYDEV_hWrtSyncSemB);                     /*  通知线程可写                */
    API_SemaphoreBPost(ptyDev->TYDEV_hDrainSyncSemB);                   /*  drain                       */
    
    SEL_WAKE_UP_ALL(&ptyDev->TYDEV_selwulList, SELWRITE);               /*  通知 select 线程可写        */
    
    TYDEV_UNLOCK(ptyDev);                                               /*  释放设备使用权              */
}
/*********************************************************************************************************
** 函数名称: API_TyAbortFuncSet
** 功能描述: TY 设备工作于 OPT_ABORT 时接收到 ABORT 命令时执行的动作.
** 输　入  : 
**           pfuncAbort,        接收到 ABORT 命令后执行的函数
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_TyAbortFuncSet (FUNCPTR  pfuncAbort)
{
    _G_pfuncTyAbortFunc = pfuncAbort;
}
/*********************************************************************************************************
** 函数名称: API_TyAbortSet
** 功能描述: TY 设备工作于 OPT_ABORT 时, 设置 ABORT 字符
** 输　入  : 
**           cAbort,            ABORT 字符
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_TyAbortSet (CHAR  cAbort)
{
    _G_cTyAbortChar = cAbort;
}
/*********************************************************************************************************
** 函数名称: API_TyBackspaceSet
** 功能描述: 设置 BACKSPACE 字符
** 输　入  : 
**           cBackspace,        BACKSPACE 字符
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_TyBackspaceSet (CHAR  cBackspace)
{
    _G_cTyBackspaceChar = cBackspace;
}
/*********************************************************************************************************
** 函数名称: API_TyDeleteLineSet
** 功能描述: 设置 cDeleteLine 字符
** 输　入  : 
**           cDeleteLine,       DeleteLine 字符
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_TyDeleteLineSet (CHAR  cDeleteLine)
{
    _G_cTyDeleteLineChar = cDeleteLine;
}
/*********************************************************************************************************
** 函数名称: API_TyEOFSet
** 功能描述: 设置 cEOF 字符
** 输　入  : 
**           cEOF,              EOF 字符
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_TyEOFSet (CHAR  cEOF)
{
    _G_cTyEofChar = cEOF;
}
/*********************************************************************************************************
** 函数名称: API_TyMonitorTrapSet
** 功能描述: 设置 MonitorTrap 字符
** 输　入  : 
**           cMonitorTrap,      MonitorTrap 字符
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_TyMonitorTrapSet (CHAR  cMonitorTrap)
{
    _G_cTyMonTrapChar = cMonitorTrap;
}
/*********************************************************************************************************
** 函数名称: _TyDrain
** 功能描述: TY 设备等待发送完成
** 输　入  : pstrDev,           TY 设备
** 输　出  : 
** 全局变量: 
** 调用模块: 
** 注  意  : 如果出现多任务同时 drain 则有可能发生无法唤醒其他任务的情况, 这里每一个 tick 都检测 ring 缓冲
*********************************************************************************************************/
static INT  _TyDrain (TY_DEV_ID  ptyDev)
{
             INTREG     iregInterLevel;

    REGISTER ULONG      ulError;
             INT        iFree;

    do {
        LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);
        iFree = rngNBytes(ptyDev->TYDEV_vxringidWrBuf);
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);
        
        if (iFree == 0) {
            return  (ERROR_NONE);
        }
                                                                        /*  仅等待一个时钟周期          */
        ulError = API_SemaphoreBPend(ptyDev->TYDEV_hDrainSyncSemB, LW_OPTION_WAIT_A_TICK);
    } while ((ulError == ERROR_NONE) || 
             (ulError == ERROR_THREAD_WAIT_TIMEOUT));
    
    _ErrorHandle(ERROR_IO_DEVICE_TIMEOUT);                              /*  超时                        */
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: _TyIoctl
** 功能描述: TY 设备命令处理
** 输　入  : 
**           pstrDev,           TY 设备
**           request,           命令
**           arg                命令参数
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _TyIoctl (TY_DEV_ID  ptyDev,
               INT        iRequest,
               LONG       lArg)
{
             INTREG          iregInterLevel;
             VX_RING_ID      ringId;
             
    REGISTER INT             iStatus = ERROR_NONE;
    REGISTER INT             iOldOption;
    
             struct stat    *pstatGet;
    
    switch (iRequest) {
    
    case FIONREAD:                                                      /*  读缓冲区有效数据数量        */
        *((INT *)lArg) = rngNBytes(ptyDev->TYDEV_vxringidRdBuf);
        break;
    
    case FIONWRITE:                                                     /*  写缓冲区有效数据数量        */
        *((INT *)lArg) = rngNBytes(ptyDev->TYDEV_vxringidWrBuf);
        break;
    
    case FIOFLUSH:                                                      /*  清空设备缓冲区              */
        _TyFlush(ptyDev);
        break;
        
    case FIOWFLUSH:                                                     /*  清空写缓冲区                */
        _TyFlushWrt(ptyDev);
        break;
        
    case FIORFLUSH:                                                     /*  清空读缓冲区                */
        _TyFlushRd(ptyDev);
        break;
    
    case FIOSYNC:                                                       /*  等待发送完成                */
    case FIODATASYNC:
        iStatus = _TyDrain(ptyDev);
        break;
    
    case FIOGETOPTIONS:                                                 /*  获得 TY 参数                */
        *(INT *)lArg = ptyDev->TYDEV_iOpt;
        break;
        
    case FIOSETOPTIONS:                                                 /*  设置 TY 参数                */
        iOldOption = ptyDev->TYDEV_iOpt;
        ptyDev->TYDEV_iOpt = (INT)lArg;
        if ((iOldOption & OPT_LINE) != 
            (ptyDev->TYDEV_iOpt & OPT_LINE)) {                          /*  有 OPT_LINE 标志的变化      */
            _TyFlushRd(ptyDev);
        }
        if ((iOldOption & OPT_TANDEM) && 
            !(ptyDev->TYDEV_iOpt & OPT_TANDEM)) {                       /*  启动软流控                  */
            TYDEV_LOCK(ptyDev, return (PX_ERROR));                      /*  等待设备使用权              */
            _TyRdXoff(ptyDev, LW_FALSE);                                /*  XON                         */
            _TyWrtXoff(ptyDev, LW_FALSE);
            TYDEV_UNLOCK(ptyDev);                                       /*  释放设备使用权              */
        }
        break;
        
    case FIOCANCEL:                                                     /*  暂时屏蔽 FIO 口             */
        TYDEV_LOCK(ptyDev, return (PX_ERROR));                          /*  等待设备使用权              */
        ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bCanceled = LW_TRUE;
        API_SemaphoreBPost(ptyDev->TYDEV_hRdSyncSemB);
        ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bCanceled = LW_TRUE;
        API_SemaphoreBPost(ptyDev->TYDEV_hWrtSyncSemB);
        TYDEV_UNLOCK(ptyDev);                                           /*  释放设备使用权              */
        break;
    
    case FIOISATTY:                                                     /*  是否为终端设备              */
        if (lArg) {
            *(BOOL *)lArg = LW_TRUE;
        }
        break;
    
    case FIOPROTOHOOK:                                                  /*  设置链接协议栈回调函数      */
        ptyDev->TYDEV_pfuncProtoHook = (FUNCPTR)lArg;
        break;
        
    case FIOPROTOARG:                                                   /*  设置回调函数参数            */
        ptyDev->TYDEV_iProtoArg = (INT)lArg;
        break;
    
    case FIORBUFSET:                                                    /*  重新设置读缓冲的大小        */
        TYDEV_LOCK(ptyDev, return (PX_ERROR));                          /*  等待设备使用权              */
        ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bFlushingRdBuf = LW_TRUE;
        KN_SMP_MB();
        TYDEV_WAIT_ISR(ptyDev);                                         /*  等待一个种中断结束          */
        ringId = rngCreate((INT)lArg);                                  /*  重新建立缓冲区              */
        if (ringId) {
            if (ptyDev->TYDEV_vxringidRdBuf) {
                rngDelete(ptyDev->TYDEV_vxringidRdBuf);
            }
            ptyDev->TYDEV_vxringidRdBuf = ringId;
        } else {
            iStatus = PX_ERROR;
        }
        KN_SMP_MB();
        ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bFlushingRdBuf = LW_FALSE;
        TYDEV_UNLOCK(ptyDev);                                           /*  释放设备使用权              */
        break;
        
    case FIOWBUFSET:                                                    /*  重新设置写缓冲区            */
        TYDEV_LOCK(ptyDev, return (PX_ERROR));                          /*  等待设备使用权              */
        ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bFlushingWrtBuf = LW_TRUE;
        KN_SMP_MB();
        TYDEV_WAIT_ISR(ptyDev);                                         /*  等待一个中断结束            */
        ringId = rngCreate((INT)lArg);                                  /*  重新建立缓冲区              */
        if (ringId) {
            if (ptyDev->TYDEV_vxringidWrBuf) {
                rngDelete(ptyDev->TYDEV_vxringidWrBuf);
            }
            ptyDev->TYDEV_vxringidWrBuf = ringId;
        } else {
            iStatus = PX_ERROR;
        }
        if (iStatus == ERROR_NONE) {
            LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);
            ptyDev->TYDEV_iWrtThreshold = ((INT)lArg > (INT)_G_cTyWrtThreshold)
                                        ? ((INT)_G_cTyWrtThreshold) 
                                        : ((INT)lArg);                  /*  确定写激活门限              */
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);
        }
        KN_SMP_MB();
        ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bFlushingWrtBuf = LW_FALSE;
        
        API_SemaphoreBPost(ptyDev->TYDEV_hWrtSyncSemB);                 /*  释放信号量                  */
        SEL_WAKE_UP_ALL(&ptyDev->TYDEV_selwulList, SELWRITE);           /*  释放所有等待写的线程        */
        TYDEV_UNLOCK(ptyDev);                                           /*  释放设备使用权              */
        break;
        
    case FIOFSTATGET:                                                   /*  获取文件属性                */
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev     = (dev_t)ptyDev;
            pstatGet->st_ino     = (ino_t)0;                            /*  相当于唯一节点              */
            pstatGet->st_mode    = 0666 | S_IFCHR;
            pstatGet->st_nlink   = 1;
            pstatGet->st_uid     = 0;
            pstatGet->st_gid     = 0;
            pstatGet->st_rdev    = 1;
            pstatGet->st_size    = 0;
            pstatGet->st_blksize = 0;
            pstatGet->st_blocks  = 0;
            pstatGet->st_atime   = ptyDev->TYDEV_timeCreate;
            pstatGet->st_mtime   = ptyDev->TYDEV_timeCreate;
            pstatGet->st_ctime   = ptyDev->TYDEV_timeCreate;
        } else {
            return  (PX_ERROR);
        }
        break;
        
    case FIOSELECT:                                                     /*  ty FIOSELECT 处理           */
        __selTyAdd(ptyDev, lArg);
        break;
        
    case FIOUNSELECT:                                                   /*  ty FIOUNSELECT 处理         */
        __selTyDelete(ptyDev, lArg);
        break;
        
    case FIORTIMEOUT:                                                   /*  设置读超时时间              */
        {
            struct timeval *ptvTimeout = (struct timeval *)lArg;
            REGISTER ULONG  ulTick;
            if (ptvTimeout) {
                ulTick = __timevalToTick(ptvTimeout);                   /*  获得 tick 数量              */
                ptyDev->TYDEV_ulRTimeout = ulTick;
            } else {
                ptyDev->TYDEV_ulRTimeout = LW_OPTION_WAIT_INFINITE;
            }
        }
        break;
        
    case FIOWTIMEOUT:                                                   /*  设置写超时时间              */
        {
            struct timeval *ptvTimeout = (struct timeval *)lArg;
            REGISTER ULONG  ulTick;
            if (ptvTimeout) {
                ulTick = __timevalToTick(ptvTimeout);                   /*  获得 tick 数量              */
                ptyDev->TYDEV_ulWTimeout = ulTick;
            } else {
                ptyDev->TYDEV_ulWTimeout = LW_OPTION_WAIT_INFINITE;
            }
        }
        break;
        
    case FIOWAITABORT:                                                  /*  停止当前等待 IO 线程        */
        TYDEV_LOCK(ptyDev, return (PX_ERROR));                          /*  等待设备使用权              */
        if ((INT)lArg & OPT_RABORT) {
            ULONG  ulBlockNum;
            API_SemaphoreBStatus(ptyDev->TYDEV_hRdSyncSemB, LW_NULL, LW_NULL, &ulBlockNum);
            if (ulBlockNum) {
                ptyDev->TYDEV_iAbortFlag |= OPT_RABORT;
                API_SemaphoreBPost(ptyDev->TYDEV_hRdSyncSemB);          /*  激活读等待线程              */
            }
        }
        if ((INT)lArg & OPT_WABORT) {
            ULONG  ulBlockNum;
            API_SemaphoreBStatus(ptyDev->TYDEV_hWrtSyncSemB, LW_NULL, LW_NULL, &ulBlockNum);
            if (ulBlockNum) {
                ptyDev->TYDEV_iAbortFlag |= OPT_WABORT;
                API_SemaphoreBPost(ptyDev->TYDEV_hWrtSyncSemB);         /*  激活写等待线程              */
            }
        }
        TYDEV_UNLOCK(ptyDev);                                           /*  释放设备使用权              */
        break;
        
    case FIOABORTFUNC:                                                  /*  设置 control-C 行为         */
        ptyDev->TYDEV_pfuncCtrlC = (FUNCPTR)lArg;
        break;
        
    case FIOABORTARG:                                                   /*  设置 control-C 参数         */
        ptyDev->TYDEV_pvArgCtrlC = (PVOID)lArg;
        break;
        
    case FIOGETCC: {                                                    /*  获取特殊控制字符集          */
        PCHAR   pcCtlChars = (PCHAR)lArg;
        if (pcCtlChars) {
            TYDEV_LOCK(ptyDev, return (PX_ERROR));                      /*  等待设备使用权              */
            lib_memcpy(pcCtlChars, ptyDev->TYDEV_cCtlChars, NCCS);
            TYDEV_UNLOCK(ptyDev);                                       /*  释放设备使用权              */
        } else {
            _ErrorHandle(EINVAL);
            iStatus = PX_ERROR;
        }
        break;
    }
    
    case FIOSETCC: {                                                    /*  设置特殊控制字符集          */
        PCHAR   pcCtlChars = (PCHAR)lArg;
        if (pcCtlChars) {
            TYDEV_LOCK(ptyDev, return (PX_ERROR));                      /*  等待设备使用权              */
            lib_memcpy(ptyDev->TYDEV_cCtlChars, pcCtlChars, NCCS);
            TYDEV_UNLOCK(ptyDev);                                       /*  释放设备使用权              */
        } else {
            _ErrorHandle(EINVAL);
            iStatus = PX_ERROR;
        }
        break;
    }
    
    case TIOCGWINSZ: {
        struct winsize  *pwin = (struct winsize *)lArg;
        if (pwin) {
            pwin->ws_row = ptyDev->TYDEV_tydevwins.TYDEVWINS_usRow;
            pwin->ws_col = ptyDev->TYDEV_tydevwins.TYDEVWINS_usCol;
            pwin->ws_xpixel = ptyDev->TYDEV_tydevwins.TYDEVWINS_usXPixel;
            pwin->ws_ypixel = ptyDev->TYDEV_tydevwins.TYDEVWINS_usYPixel;
        } else {
            _ErrorHandle(EINVAL);
            iStatus = PX_ERROR;
        }
        break;
    }
        
    case TIOCSWINSZ: {
        struct winsize  *pwin = (struct winsize *)lArg;
        if (pwin) {
            ptyDev->TYDEV_tydevwins.TYDEVWINS_usRow = pwin->ws_row;
            ptyDev->TYDEV_tydevwins.TYDEVWINS_usCol = pwin->ws_col;
            ptyDev->TYDEV_tydevwins.TYDEVWINS_usXPixel = pwin->ws_xpixel;
            ptyDev->TYDEV_tydevwins.TYDEVWINS_usYPixel = pwin->ws_ypixel;
        } else {
            _ErrorHandle(EINVAL);
            iStatus = PX_ERROR;
        }
        break;
    }
    
    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        iStatus = PX_ERROR;
    }
    
    return  (iStatus);
}
/*********************************************************************************************************
** 函数名称: _TyWrite
** 功能描述: 向终端写入数据
** 输　入  : 
**           ptyDev             TY 设备
**           pcBuffer,          需要写入终端的数据
**           stNBytes           需要写入的数据大小
** 输　出  : 实际写入的字节数, 小于 1 表示错误
** 全局变量: 
** 调用模块: 
** 注  意  : 当任务在获取写同步信号量后获取互斥信号量前被删除, 则 tty 会丢失写同步, 所以这里提前进入安全
             模式.
*********************************************************************************************************/
ssize_t  _TyWrite (TY_DEV_ID  ptyDev, 
                   PCHAR      pcBuffer, 
                   size_t     stNBytes)
{
             INTREG     iregInterLevel;
             
    REGISTER INT        iBytesput;
    REGISTER ssize_t    sstNbStart = stNBytes;
    
    REGISTER ULONG      ulError;
    
    if (pcBuffer == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (stNBytes == 0) {
        return  (0);
    }
         
    ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bCanceled = LW_FALSE;
    ptyDev->TYDEV_iAbortFlag &= ~OPT_WABORT;                            /*  清除 abort                  */
    
    API_ThreadSafe();                                                   /*  任务提前进入安全状态        */
    while (stNBytes > 0) {
        ulError = API_SemaphoreBPend(ptyDev->TYDEV_hWrtSyncSemB, ptyDev->TYDEV_ulWTimeout);
        if (ulError) {
            API_ThreadUnsafe();
            _ErrorHandle(ERROR_IO_DEVICE_TIMEOUT);                      /*   超时                       */
            return  (sstNbStart - stNBytes);
        }
        
        TYDEV_LOCK(ptyDev, return (PX_ERROR));                          /*  等待设备使用权              */
        
        if (ptyDev->TYDEV_iAbortFlag & OPT_WABORT) {                    /*  is abort?                   */
            TYDEV_UNLOCK(ptyDev);                                       /*  释放设备使用权              */
            API_ThreadUnsafe();
            _ErrorHandle(ERROR_IO_ABORT);                               /*  abort                       */
            return  (sstNbStart - stNBytes);
        }
        
        if (ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bCanceled) {          /*  检查是否被禁止输出了        */
            TYDEV_UNLOCK(ptyDev);                                       /*  释放设备使用权              */
            API_ThreadUnsafe();
            _ErrorHandle(ERROR_IO_CANCELLED);
            return  (sstNbStart - stNBytes);
        }
        
        LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);     /*  锁定 spinlock 并关闭中断    */
        iBytesput = rngBufPut(ptyDev->TYDEV_vxringidWrBuf, 
                              pcBuffer, 
                              (INT)stNBytes);                           /*  将数据写入缓冲区            */
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  解锁 spinlock 打开中断      */
        
        _TyTxStartup(ptyDev);                                           /*  启动设备发送                */
        
        stNBytes -= (size_t)iBytesput;                                  /*  剩余需要发送的数据          */
        pcBuffer += iBytesput;                                          /*  新的缓冲区起始地址          */
    
        LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);     /*  锁定 spinlock 并关闭中断    */
        if (rngFreeBytes(ptyDev->TYDEV_vxringidWrBuf) > 0) {
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);/*  解锁 spinlock 打开中断      */
            API_SemaphoreBPost(ptyDev->TYDEV_hWrtSyncSemB);             /*  缓冲区还有空间              */
        
        } else {
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);/*  解锁 spinlock 打开中断      */
        }
        
        TYDEV_UNLOCK(ptyDev);                                           /*  释放设备使用权              */
    }
    API_ThreadUnsafe();                                                 /*  退出安全模式                */
    
    return  (sstNbStart);
}
/*********************************************************************************************************
** 函数名称: _TyReadVtime
** 功能描述: 从终端读出数据 (当 termios VTIME!=0 时使用)
** 输　入  : 
**           ptyDev,            TY 设备
**           pcBuffer,          接收数据缓冲区
**           stMaxBytes         缓冲区大小
** 输　出  : 实际接收的字节数, 小于 1 表示错误
** 全局变量: 
** 调用模块: 
** 注  意  : 当任务在获取读同步信号量后获取互斥信号量前被删除, 则 tty 会丢失读同步!
*********************************************************************************************************/
static ssize_t  _TyReadVtime (TY_DEV_ID  ptyDev, 
                              PCHAR      pcBuffer, 
                              size_t     stMaxBytes)
{
             INTREG        iregInterLevel;
         
             ssize_t       sstNBytes;
             ssize_t       sstNTotalBytes = 0;
    
    REGISTER VX_RING_ID    ringId;
    REGISTER INT           iNTemp;
    REGISTER INT           iRetTemp;
    REGISTER INT           iFreeBytes;
    
    REGISTER ULONG         ulError;
             ULONG         ulTimeout;                                   /*  间隔时间                    */
    
    if (__TTY_CC(ptyDev, VMIN) == 0) {                                  /*  首次等待时间                */
        ulTimeout = ((ULONG)(__TTY_CC(ptyDev, VTIME) * 100) * LW_TICK_HZ / 1000);
    } else {
        ulTimeout = LW_OPTION_WAIT_INFINITE;
    }
    
__re_read:
    ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bCanceled = LW_FALSE;
    ptyDev->TYDEV_iAbortFlag &= ~OPT_RABORT;                            /*  清除 abort                  */
    
    for (;;) {
        ulError = API_SemaphoreBPend(ptyDev->TYDEV_hRdSyncSemB, ulTimeout);
        if (ulError) {
            _ErrorHandle(ERROR_IO_DEVICE_TIMEOUT);                      /*  超时                        */
            return  (sstNTotalBytes);
        }
        
        TYDEV_LOCK(ptyDev, return (PX_ERROR));                          /*  等待设备使用权              */
        
        if (ptyDev->TYDEV_iAbortFlag & OPT_RABORT) {                    /*  is abort                    */
            TYDEV_UNLOCK(ptyDev);                                       /*  释放设备使用权              */
            _ErrorHandle(ERROR_IO_ABORT);                               /*  abort                       */
            return  (sstNTotalBytes);
        }
        
        if (ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bCanceled) {          /*  检查设备是否被读禁止了      */
            TYDEV_UNLOCK(ptyDev);                                       /*  释放设备使用权              */
            _ErrorHandle(ERROR_IO_CANCELLED);
            return  (sstNTotalBytes);
        }
        
        ringId = ptyDev->TYDEV_vxringidRdBuf;                           /*  输入缓冲区                  */
        
        LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);     /*  锁定 spinlock 并关闭中断    */
        if (!rngIsEmpty(ringId)) {                                      /*  检查是否有数据              */
            break;
        }
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  解锁 spinlock 打开中断      */
        
        TYDEV_UNLOCK(ptyDev);                                           /*  释放设备使用权              */
    }
        
    if (__TTY_CC(ptyDev, VMIN)) {                                       /*  VMIN 不为 0 设置间隔等待    */
        ulTimeout = ((ULONG)(__TTY_CC(ptyDev, VTIME) * 100) * LW_TICK_HZ / 1000);
    }
    
    if (ptyDev->TYDEV_iOpt & OPT_LINE) {                                /*  行模式                      */
        if (ptyDev->TYDEV_ucInBytesLeft == 0) {                         /*  无有效的行剩余数据          */
            iRetTemp = __RNG_ELEM_GET(ringId, 
                           (PCHAR)&ptyDev->TYDEV_ucInBytesLeft, 
                           iNTemp);                                     /*  获得剩余数据大小            */
            (VOID)iRetTemp;                                             /*  这里一定成功, 暂不判断返回值*/
        }
        sstNBytes = (ssize_t)__MIN((ssize_t)ptyDev->TYDEV_ucInBytesLeft, 
                                   (ssize_t)stMaxBytes);                /*  计算接收的最大值            */
        rngBufGet(ringId, pcBuffer, (INT)sstNBytes);                    /*  接收数据                    */
        ptyDev->TYDEV_ucInBytesLeft = (UCHAR)
                                      (ptyDev->TYDEV_ucInBytesLeft
                                    - sstNBytes);                       /*  重新计算行剩余数据          */
    } else {                                                            /*  非行模式                    */
        sstNBytes = (ssize_t)rngBufGet(ringId, pcBuffer, 
                                       (INT)stMaxBytes);                /*  直接接收                    */
    }
    
    if ((ptyDev->TYDEV_iOpt & OPT_TANDEM) &&                            /*  使用 XON XOFF 流控制        */
        ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bXoff) {                  /*  并且关断读                  */
        iFreeBytes = rngFreeBytes(ringId);                              /*  获得空闲数量                */
        if (ptyDev->TYDEV_iOpt & OPT_LINE) {                            /*  行模式                      */
            iFreeBytes -= ptyDev->TYDEV_ucInNBytes + 1;
        }
        if (iFreeBytes > _G_cTyXonThreshold) {
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);/*  解锁 spinlock 打开中断      */
            _TyRdXoff(ptyDev, LW_FALSE);                                /*  启动对方发送                */
            LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel); /*  锁定 spinlock 并关闭中断    */
        }
    }
    
    if (!rngIsEmpty(ringId)) {                                          /*  是否还有数据                */
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  解锁 spinlock 打开中断      */
        API_SemaphoreBPost(ptyDev->TYDEV_hRdSyncSemB);                  /*  通知其他等待读的线程        */
    
    } else {
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  解锁 spinlock 打开中断      */
    }
    
    sstNTotalBytes += sstNBytes;                                        /*  以读取数据的总数            */
    
    if (sstNTotalBytes < __TTY_CC(ptyDev, VMIN)) {                      /*  是否接收到指定数量的数据    */
        TYDEV_UNLOCK(ptyDev);                                           /*  释放设备使用权              */
        pcBuffer   += sstNBytes;
        stMaxBytes -= (size_t)sstNBytes;
        goto    __re_read;
        
    } else {
        TYDEV_UNLOCK(ptyDev);                                           /*  释放设备使用权              */
    }
    
    return  (sstNTotalBytes);
}
/*********************************************************************************************************
** 函数名称: _TyRead
** 功能描述: 从终端读出数据
** 输　入  : 
**           ptyDev,            TY 设备
**           pcBuffer,          接收数据缓冲区
**           stMaxBytes         缓冲区大小
** 输　出  : 实际接收的字节数, 小于 1 表示错误
** 全局变量: 
** 调用模块: 
** 注  意  : 当任务在获取读同步信号量后获取互斥信号量前被删除, 则 tty 会丢失读同步!
*********************************************************************************************************/
ssize_t  _TyRead (TY_DEV_ID  ptyDev, 
                  PCHAR      pcBuffer, 
                  size_t     stMaxBytes)
{
             INTREG        iregInterLevel;
         
             ssize_t       sstNBytes;
             ssize_t       sstNTotalBytes = 0;
    
    REGISTER VX_RING_ID    ringId;
    REGISTER INT           iNTemp;
    REGISTER INT           iRetTemp;
    REGISTER INT           iFreeBytes;
    
    REGISTER ULONG         ulError;
    
    if (pcBuffer == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (stMaxBytes == 0) {
        return  (0);
    }
    
    if (__TTY_CC(ptyDev, VMIN) > stMaxBytes) {                          /*  stMaxBytes 太小             */
        _ErrorHandle(EINVAL);
        return  (0);
    }
    
    if (__TTY_CC(ptyDev, VTIME) != 0) {
        return  (_TyReadVtime(ptyDev, pcBuffer, stMaxBytes));           /*  带有 VTIME 超时的读取       */
    }

__re_read:
    ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bCanceled = LW_FALSE;
    ptyDev->TYDEV_iAbortFlag &= ~OPT_RABORT;                            /*  清除 abort                  */
    
    for (;;) {
        if (__TTY_CC(ptyDev, VMIN) == 0) {                              /*  无需等待                    */
            ulError = API_SemaphoreBTryPend(ptyDev->TYDEV_hRdSyncSemB);
        } else {                                                        /*  普通调用                    */
            ulError = API_SemaphoreBPend(ptyDev->TYDEV_hRdSyncSemB, ptyDev->TYDEV_ulRTimeout);
        }
        if (ulError) {
            _ErrorHandle(ERROR_IO_DEVICE_TIMEOUT);                      /*  超时                        */
            return  (sstNTotalBytes);
        }
        
        TYDEV_LOCK(ptyDev, return (PX_ERROR));                          /*  等待设备使用权              */
        
        if (ptyDev->TYDEV_iAbortFlag & OPT_RABORT) {                    /*  is abort                    */
            TYDEV_UNLOCK(ptyDev);                                       /*  释放设备使用权              */
            _ErrorHandle(ERROR_IO_ABORT);                               /*  abort                       */
            return  (sstNTotalBytes);
        }
        
        if (ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bCanceled) {          /*  检查设备是否被读禁止了      */
            TYDEV_UNLOCK(ptyDev);                                       /*  释放设备使用权              */
            _ErrorHandle(ERROR_IO_CANCELLED);
            return  (sstNTotalBytes);
        }
        
        ringId = ptyDev->TYDEV_vxringidRdBuf;                           /*  输入缓冲区                  */
        
        LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);     /*  锁定 spinlock 并关闭中断    */
        if (!rngIsEmpty(ringId)) {                                      /*  检查是否有数据              */
            break;
        }
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  解锁 spinlock 打开中断      */
        
        TYDEV_UNLOCK(ptyDev);                                           /*  释放设备使用权              */
    }
    
    if (ptyDev->TYDEV_iOpt & OPT_LINE) {                                /*  行模式                      */
        if (ptyDev->TYDEV_ucInBytesLeft == 0) {                         /*  无有效的行剩余数据          */
            iRetTemp = __RNG_ELEM_GET(ringId, 
                           (PCHAR)&ptyDev->TYDEV_ucInBytesLeft, 
                           iNTemp);                                     /*  获得剩余数据大小            */
            (VOID)iRetTemp;                                             /*  这里一定成功, 暂不判断返回值*/
        }
        sstNBytes = (ssize_t)__MIN((ssize_t)ptyDev->TYDEV_ucInBytesLeft, 
                                   (ssize_t)stMaxBytes);                /*  计算接收的最大值            */
        rngBufGet(ringId, pcBuffer, (INT)sstNBytes);                    /*  接收数据                    */
        ptyDev->TYDEV_ucInBytesLeft = (UCHAR)
                                      (ptyDev->TYDEV_ucInBytesLeft
                                    - sstNBytes);                       /*  重新计算行剩余数据          */
    } else {                                                            /*  非行模式                    */
        sstNBytes = (ssize_t)rngBufGet(ringId, pcBuffer, 
                                       (INT)stMaxBytes);                /*  直接接收                    */
    }
    
    if ((ptyDev->TYDEV_iOpt & OPT_TANDEM) &&                            /*  使用 XON XOFF 流控制        */
        ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bXoff) {                  /*  并且关断读                  */
        iFreeBytes = rngFreeBytes(ringId);                              /*  获得空闲数量                */
        if (ptyDev->TYDEV_iOpt & OPT_LINE) {                            /*  行模式                      */
            iFreeBytes -= ptyDev->TYDEV_ucInNBytes + 1;
        }
        if (iFreeBytes > _G_cTyXonThreshold) {
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);/*  解锁 spinlock 打开中断      */
            _TyRdXoff(ptyDev, LW_FALSE);                                /*  启动对方发送                */
            LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);   
                                                                        /*  锁定 spinlock 并关闭中断    */
        }
    }
    
    if (!rngIsEmpty(ringId)) {                                          /*  是否还有数据                */
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  解锁 spinlock 打开中断      */
        API_SemaphoreBPost(ptyDev->TYDEV_hRdSyncSemB);                  /*  通知其他等待读的线程        */
    
    } else {
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  解锁 spinlock 打开中断      */
    }
    
    sstNTotalBytes += sstNBytes;                                        /*  以读取数据的总数            */
    
    if (sstNTotalBytes < __TTY_CC(ptyDev, VMIN)) {                      /*  是否接收到指定数量的数据    */
        TYDEV_UNLOCK(ptyDev);                                           /*  释放设备使用权              */
        pcBuffer   += sstNBytes;
        stMaxBytes -= (size_t)sstNBytes;
        goto    __re_read;
        
    } else {
        TYDEV_UNLOCK(ptyDev);                                           /*  释放设备使用权              */
    }
    
    return  (sstNTotalBytes);
}
/*********************************************************************************************************
** 函数名称: _TyITx
** 功能描述: 从终端的发送缓冲区中读出一个字节的待数据, FIFO
** 输　入  : 
**           ptyDev,            TY 设备
**           pcChar,            待发送的数据
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _TyITx (TY_DEV_ID  ptyDev, PCHAR  pcChar)
{
             INTREG        iregInterLevel;

    REGISTER VX_RING_ID    ringId = ptyDev->TYDEV_vxringidWrBuf;
    REGISTER INT           iNTemp;
             INT           iRet;
    
    LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);         /*  锁定 spinlock 并关闭中断    */
    
    if (ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bPending) {               /*  是否需要发送流控数据        */
        ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bPending = LW_FALSE;
        if (ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bXoff) {
            if (_G_iTyXoffChars > _G_iTyXoffMax) {                      /*  统计变量更新                */
                _G_iTyXoffMax   = _G_iTyXoffChars;                      /*  记录最大值                  */
                _G_iTyXoffChars = 0;
            }
            *pcChar = __TTY_CC(ptyDev, VSTOP);
        } else {
            *pcChar = __TTY_CC(ptyDev, VSTART);
        }

    } else if (ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bXoff || 
               ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bFlushingWrtBuf) { /*  对方禁止接收或者缓冲区占用  */
        ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy = LW_FALSE;
        
    } else if (ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bCR) {             /*  是否需要追加 CR 字符        */
        *pcChar = '\n';
        ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bCR   = LW_FALSE;
        ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy = LW_TRUE;
        goto    __release_wrt;                                          /*  解锁写操作                  */
        
    } else {
        if (__RNG_ELEM_GET(ringId, pcChar, iNTemp) == 0) {              /*  从缓冲区取出数据            */
            ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy = LW_FALSE;     /*  没有剩余数据等待发送        */
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);  
                                                                        /*  解锁 spinlock 打开中断      */
            API_SemaphoreBPost(ptyDev->TYDEV_hDrainSyncSemB);           /*  产生传输完成信号量          */
            
            return  (PX_ERROR);                                         /*  没有数据需要发送            */

        } else {
            ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy = LW_TRUE;
            
            if ((ptyDev->TYDEV_iOpt & OPT_CRMOD) && (*pcChar == '\n')) {/*  需要在 LF 前添加 CR 字符    */
                                                                        /*  并且待发送的数据为 LF       */
                *pcChar = '\r';
                ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bCR = LW_TRUE;    /*  下一次发送的数据为 LF       */
            }
            
__release_wrt:
            if (rngFreeBytes(ringId) >= ptyDev->TYDEV_iWrtThreshold) {  /*  可以激活下一个需要写的线程  */
                LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);  
                                                                        /*  解锁 spinlock 打开中断      */
                API_SemaphoreBPost(ptyDev->TYDEV_hWrtSyncSemB);         /*  释放信号量                  */
                SEL_WAKE_UP_ALL(&ptyDev->TYDEV_selwulList, SELWRITE);   /*  释放所有等待写的线程        */
            
                return  (ERROR_NONE);                                   /*  可以发送数据                */
            }
        }
    }
    
    iRet = (ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy) ? (ERROR_NONE) : (PX_ERROR);
    
    LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);        /*  解锁 spinlock 打开中断      */
        
    return  (iRet);                                                     /*  返回                        */
}
/*********************************************************************************************************
** 函数名称: _TyIRx
** 功能描述: 向终端的接收缓冲区中写入一个从硬件接收到的数据, FIFO
** 输　入  : 
**           ptyDev,            TY 设备
**           cInChar,           接收到的数据
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _TyIRd (TY_DEV_ID  ptyDev, CHAR   cInchar)
{
             INTREG      iregInterLevel;

    REGISTER VX_RING_ID  ringId;
    REGISTER INT         iNTemp;
             BOOL        bReleaseTaskLevel;
             
    REGISTER INT         iOpt        = ptyDev->TYDEV_iOpt;
             BOOL        bCharEchoed = LW_FALSE;
             INT         iStatus     = ERROR_NONE;
             
    REGISTER INT         iFreeBytes;
             BOOL        bNeedBsOrKill;
    
    
    if (ptyDev->TYDEV_pfuncProtoHook) {
        if ((*ptyDev->TYDEV_pfuncProtoHook)(ptyDev->TYDEV_iProtoArg, 
                                            cInchar)) {                 /*  有链接的协议时调用协议栈    */
            return  (iStatus);
        }
    }
    
    if (iOpt & OPT_7_BIT) {                                             /*  仅接收 7 位数据             */
        cInchar &= 0x7f;
    }
    
    LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);         /*  锁定 spinlock 并关闭中断    */
    
    if (ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bFlushingRdBuf) {         /*  输入缓冲区是否被 FLUSH      */
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  解锁 spinlock 打开中断      */
        return  (PX_ERROR);
    }
    
    if ((cInchar == __TTY_CC(ptyDev, VINTR)) && 
        (iOpt & OPT_ABORT) && 
        ((_G_pfuncTyAbortFunc != LW_NULL) || 
        (ptyDev->TYDEV_pfuncCtrlC != LW_NULL))) {                       /*  是否需要进行 ABORT 处理     */
        
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  解锁 spinlock 打开中断      */
        
        if (ptyDev->TYDEV_pfuncCtrlC) {
            ptyDev->TYDEV_pfuncCtrlC(ptyDev->TYDEV_pvArgCtrlC);
        }
        if (_G_pfuncTyAbortFunc) {
            _G_pfuncTyAbortFunc();
        }
    
    } else if ((cInchar == __TTY_CC(ptyDev, VQUIT)) && 
               (iOpt & OPT_MON_TRAP)) {                                 /*  需要处理 CONTORL+X 命令     */
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  解锁 spinlock 打开中断      */
        API_KernelReboot(LW_REBOOT_WARM);                               /*  重新启动操作系统            */
    
    } else if (((cInchar == __TTY_CC(ptyDev, VSTOP)) || (cInchar == __TTY_CC(ptyDev, VSTART))) && 
               (iOpt & OPT_TANDEM)) {                                   /*  需要处理流控制              */
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  解锁 spinlock 打开中断      */
        
        if (cInchar == __TTY_CC(ptyDev, VSTOP)) {
            _TyWrtXoff(ptyDev, LW_TRUE);
        } else {
            _TyWrtXoff(ptyDev, LW_FALSE);
        }
    
    } else {
        if ((iOpt & OPT_CRMOD) && (iOpt & OPT_LINE)) {
            if (cInchar == PX_EOS) {                                    /*  此模式下对 0x00 字符不响应  */
                LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);  
                                                                        /*  解锁 spinlock 打开中断      */
                return  (iStatus);                                      /*  忽略此 \0                   */
            }
        }
        
        if (ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bXoff) {              /*  本机不允许输入              */
            _G_iTyXoffChars++;                                          /*  统计变量++                  */
        }
        
        if ((iOpt & OPT_CRMOD) && 
            (ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_cLastRecv == '\r') &&
            (cInchar == '\n')) {                                        /*  连续的 \r\n 序列, 仅识别一个*/
            ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_cLastRecv = cInchar;
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);/*  解锁 spinlock 打开中断      */
            return  (iStatus);                                          /*  忽略此 \n                   */
        
        } else {
            ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_cLastRecv = cInchar;  /*  保存本次输入                */
        }
        
        if ((iOpt & OPT_CRMOD) && (cInchar == '\r')) {                  /*  接收到 CR 字符              */
            cInchar = '\n';                                             /*  进行转意                    */
        }
        
        ringId = ptyDev->TYDEV_vxringidRdBuf;
        bReleaseTaskLevel = LW_FALSE;                                   /*  开始对输入缓冲区操作        */
        
        if (!(iOpt & OPT_LINE)) {                                       /*  不是行模式                  */
            if (RNG_ELEM_PUT(ringId, cInchar, iNTemp) == 0) {           /*  写入输入队列                */
                iStatus = PX_ERROR;
            }
            if (rngNBytes(ringId) == 1) {
                bReleaseTaskLevel = LW_TRUE;                            /*  需要激活等待线程            */
            }
            
        } else {                                                        /*  属于行模式                  */
            iFreeBytes = rngFreeBytes(ringId);                          /*  获得空闲字节的个数          */
            
            if (__TTY_BACKSPACE(ptyDev, cInchar)) {                     /*  退格键                      */
                if (ptyDev->TYDEV_ucInNBytes) {
                    ptyDev->TYDEV_ucInNBytes--;
                    bNeedBsOrKill = LW_TRUE;
                
                } else {
                    bNeedBsOrKill = LW_FALSE;
                }
            
            } else if (cInchar == __TTY_CC(ptyDev, VKILL)) {            /*  删除一行                    */
                if (ptyDev->TYDEV_ucInNBytes) {
                    ptyDev->TYDEV_ucInNBytes = 0;
                    bNeedBsOrKill = LW_TRUE;
                
                } else {
                    bNeedBsOrKill = LW_FALSE;
                }
            
            } else if (cInchar == __TTY_CC(ptyDev, VEOF)) {             /*  结束键                      */
                if (iFreeBytes > 0) {
                    bReleaseTaskLevel = LW_TRUE;                        /*  立即激活任务                */
                }
            
            } else {                                                    /*  其他键                      */
                if (iFreeBytes >= 2) {                                  /*  空闲区超过 2 个字符         */
                    if ((iFreeBytes >= (ptyDev->TYDEV_ucInNBytes + 3)) &&
                        (ptyDev->TYDEV_ucInNBytes < (MAX_CANON - 1))) {
                        ptyDev->TYDEV_ucInNBytes++;
                    } else {
                        iStatus = PX_ERROR;                             /*  没有剩余空间                */
                    }
                    
                    rngPutAhead(ringId, cInchar, 
                                (INT)ptyDev->TYDEV_ucInNBytes);
                                
                    if (cInchar == '\n') {
                        bReleaseTaskLevel = LW_TRUE;                    /*  立即激活任务                */
                    }
                
                } else {                                                /*  完全没有空间                */
                    iStatus = PX_ERROR;                                 /*  没有剩余空间                */
                }
            }
            
            if (bReleaseTaskLevel) {
                rngPutAhead( ringId, (CHAR)ptyDev->TYDEV_ucInNBytes, 0);
                rngMoveAhead(ringId, (INT) ptyDev->TYDEV_ucInNBytes + 1);
                ptyDev->TYDEV_ucInNBytes = 0;
            }
        }
        
        if ((iOpt & OPT_ECHO) && (iStatus != PX_ERROR) &&               /*  需要进行终端回显            */
            !ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bFlushingWrtBuf) {   /*  缓冲区综合条件允许          */
            
            ringId = ptyDev->TYDEV_vxringidWrBuf;                       /*  TTY 输出缓冲区              */
            
            if (iOpt & OPT_LINE) {                                      /*  行模式                      */
                if (cInchar == __TTY_CC(ptyDev, VKILL)) {               /*  删除行                      */
                    if (bNeedBsOrKill) {
                        iNTemp = RNG_ELEM_PUT(ringId, '\n', iNTemp);    /*  输出 LF                     */
                        bCharEchoed = LW_TRUE;                          /*  回显成功                    */
                    }
                
                } else if (__TTY_BACKSPACE(ptyDev, cInchar)) {          /*  退格键                      */
                    if (bNeedBsOrKill) {                                /*  这一行中已经有字符          */
                        CHAR    cBsCharList[3];
                        
                        cBsCharList[0] = cInchar;                       /*  退格序列                    */
                        cBsCharList[1] = ' ';
                        cBsCharList[2] = cInchar;
                        
                        rngBufPut(ringId, cBsCharList, 3);              /*  BS 回显                     */
                        bCharEchoed = LW_TRUE;                          /*  回显成功                    */
                    }
                
                } else if ((cInchar < 0x20) && (cInchar != '\n')) {     /*  非 LF 的控制字符            */
                    iNTemp = RNG_ELEM_PUT(ringId, '^', iNTemp);
                    iNTemp = RNG_ELEM_PUT(ringId, 
                                          (CHAR)(cInchar + '@'), 
                                          iNTemp);                      /*  发送控制命令回显            */
                    bCharEchoed = LW_TRUE;                              /*  回显成功                    */
                
                } else {                                                /*  其他字符                    */
                    iNTemp = RNG_ELEM_PUT(ringId, cInchar, iNTemp);     /*  直接回显                    */
                    bCharEchoed = LW_TRUE;                              /*  回显成功                    */
                }
            
            } else {                                                    /*  非行模式                    */
                iNTemp = RNG_ELEM_PUT(ringId, cInchar, iNTemp);         /*  直接回复即可                */
                bCharEchoed = LW_TRUE;                                  /*  回显成功                    */
            }
            
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);  
                                                                        /*  解锁 spinlock 打开中断      */
            if (bCharEchoed) {                                          /*  如果回复成功                */
                _TyTxStartup(ptyDev);                                   /*  需要启动发送                */
            }
        } else {
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);/*  解锁 spinlock 打开中断      */
        }
        
        if ((iOpt & OPT_TANDEM) && !(iOpt & OPT_LINE)) {                /*  流控制模式                  */
            LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel); /*  锁定 spinlock 并关闭中断    */
            if (ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bFlushingRdBuf) { /*  输入缓冲区是否被 FLUSH      */
                LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);
                return  (PX_ERROR);
            }
            ringId     = ptyDev->TYDEV_vxringidRdBuf;
            iFreeBytes = rngFreeBytes(ringId);
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);/*  解锁 spinlock 打开中断      */
            
            if (!ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bXoff) {         /*  本地允许接收数据            */
                if (iFreeBytes < _G_cTyXoffThreshold) {                 /*  如果输入缓冲区快要满了      */
                    _TyRdXoff(ptyDev, LW_TRUE);                         /*  发送 XOFF                   */
                    bReleaseTaskLevel = LW_TRUE;                        /*  这里需要立即激活接收线程    */
                }
            } else {
                if (iFreeBytes > _G_cTyXonThreshold) {
                    _TyRdXoff(ptyDev, LW_FALSE);                        /*  发送 XON                    */
                }
            }
        }
        
        if (bReleaseTaskLevel) {
            API_SemaphoreBPost(ptyDev->TYDEV_hRdSyncSemB);              /*  激活等待任务                */
            SEL_WAKE_UP_ALL(&ptyDev->TYDEV_selwulList, SELREAD);        /*  select() 激活               */
        }
    }
    
    if (bCharEchoed) {
        iStatus = ERROR_NONE;
    }
    
    return  (iStatus);
}
/*********************************************************************************************************
** 函数名称: _TyRdXoff
** 功能描述: 设置接收端流控状态
** 输　入  : 
**           ptyDev,            TY 设备
**           bXoff,             是否关闭接收
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _TyRdXoff (TY_DEV_ID  ptyDev, BOOL  bXoff)
{
    INTREG      iregInterLevel;
    
    LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);         /*  锁定 spinlock 并关闭中断    */
    if (ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bXoff != bXoff) {         /*  需要改变状态                */
        ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bXoff    = bXoff;
        ptyDev->TYDEV_tydevrdstat.TYDEVRDSTAT_bPending = LW_TRUE;       /*  需要发送流控制数据          */
        if (ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy == LW_FALSE) {  /*  没有启动发送                */
            ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy  = LW_TRUE;     /*  启动发送                    */
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);  
                                                                        /*  解锁 spinlock 打开中断      */
            ptyDev->TYDEV_pfuncTxStartup(ptyDev);                       /*  启动发送                    */
            return;
        }
    }
    LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);        /*  解锁 spinlock 打开中断      */
}
/*********************************************************************************************************
** 函数名称: _TyWrtXoff
** 功能描述: 设置发送端流控状态
** 输　入  : 
**           ptyDev,            TY 设备
**           bXoff,             是否关闭发送
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _TyWrtXoff (TY_DEV_ID  ptyDev, BOOL  bXoff)
{
    INTREG      iregInterLevel;
    
    LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);         /*  锁定 spinlock 并关闭中断    */
    if (ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bXoff != bXoff) {
        ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bXoff  = bXoff;
        if (bXoff == LW_FALSE) {                                        /*  启动发送                    */
            if (ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy == 
                LW_FALSE) {                                             /*  没有启动发送                */
                ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy = LW_TRUE;  /*  启动发送                    */
                LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);  
                                                                        /*  解锁 spinlock 打开中断      */
                ptyDev->TYDEV_pfuncTxStartup(ptyDev);                   /*  启动发送                    */
                return;
            }
        }
    }
    LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);        /*  解锁 spinlock 打开中断      */
}
/*********************************************************************************************************
** 函数名称: _TyTxStartup
** 功能描述: 启动发送函数
** 输　入  : 
**           ptyDev,            TY 设备
**           bXoff,             是否关闭发送
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  _TyTxStartup (TY_DEV_ID  ptyDev)
{
    INTREG    iregInterLevel;
    
    if (!ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy) {
        LW_SPIN_LOCK_QUICK(&ptyDev->TYDEV_slLock, &iregInterLevel);     /*  锁定 spinlock 并关闭中断    */
        if (!ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy) {
            ptyDev->TYDEV_tydevwrstat.TYDEVWRSTAT_bBusy = LW_TRUE;
            LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);  
                                                                        /*  解锁 spinlock 打开中断      */
            ptyDev->TYDEV_pfuncTxStartup(ptyDev);                       /*  启动发送                    */
            return;
        }
        LW_SPIN_UNLOCK_QUICK(&ptyDev->TYDEV_slLock, iregInterLevel);    /*  解锁 spinlock 打开中断      */
    }
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SIO_DEVICE_EN > 0)  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

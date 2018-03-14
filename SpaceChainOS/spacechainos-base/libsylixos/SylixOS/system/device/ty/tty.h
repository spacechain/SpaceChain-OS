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
** 文   件   名: tty.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 02 月 04 日
**
** 描        述: TTY 设备头文件.

** BUG:
2009.08.18  加入写缓冲区门限.
*********************************************************************************************************/

#ifndef __TTY_H
#define __TTY_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SIO_DEVICE_EN > 0)
/*********************************************************************************************************
  c_cc characters (defined in tty.h)
*********************************************************************************************************/
#define NCCS        19

#define VINTR       0                                                   /* INTR character               */
#define VQUIT       1                                                   /* QUIT character               */
#define VERASE      2                                                   /* ERASE character              */
#define VKILL       3                                                   /* KILL character               */
#define VEOF        4                                                   /* EOF character                */
#define VTIME       5                                                   /* TIME value                   */
#define VMIN        6                                                   /* MIN value                    */
#define VSWTC       7
#define VSTART      8                                                   /* START character              */
#define VSTOP       9                                                   /* STOP character               */
#define VSUSP       10                                                  /* SUSP character               */
#define VEOL        11                                                  /* EOL character                */
#define VREPRINT    12
#define VDISCARD    13
#define VWERASE     14
#define VLNEXT      15
#define VEOL2       16

/*********************************************************************************************************
  TY_DEV_WINSIZE
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct {
    UINT16                  TYDEVWINS_usRow;
    UINT16                  TYDEVWINS_usCol;
    UINT16                  TYDEVWINS_usXPixel;
    UINT16                  TYDEVWINS_usYPixel;
} TY_DEV_WINSIZE;

/*********************************************************************************************************
  TY_DEV_RD_STATE
*********************************************************************************************************/

typedef struct {
    BOOL                    TYDEVRDSTAT_bXoff;                          /*  输入是否被 XOFF 了          */
    BOOL                    TYDEVRDSTAT_bPending;                       /*  XON/OFF是否需要发送         */
    BOOL                    TYDEVRDSTAT_bCanceled;                      /*  读操作被禁止了              */
    BOOL                    TYDEVRDSTAT_bFlushingRdBuf;                 /*  操作关键区域时的标志        */
    CHAR                    TYDEVRDSTAT_cLastRecv;                      /*  OPT_CRMOD 下上一个接收的字符*/
} TY_DEV_RD_STATE;

/*********************************************************************************************************
  TY_DEV_WR_STATE
*********************************************************************************************************/

typedef struct {
    BOOL                    TYDEVWRSTAT_bBusy;                          /*  是否正在发送数据            */
    BOOL                    TYDEVWRSTAT_bXoff;                          /*  输入是否被 XOFF 了          */
    BOOL                    TYDEVWRSTAT_bCR;                            /*  CR 字符是否需要被添加       */
    BOOL                    TYDEVWRSTAT_bCanceled;                      /*  写操作被禁止了              */
    BOOL                    TYDEVWRSTAT_bFlushingWrtBuf;                /*  操作关键区域时的标志        */
} TY_DEV_WR_STATE;

/*********************************************************************************************************
  TY_DEV
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR              TYDEV_devhdrHdr;                            /*  I/O 系统接口设备头          */
    
    VX_RING_ID              TYDEV_vxringidRdBuf;                        /*  输入环形缓冲区              */
    VX_RING_ID              TYDEV_vxringidWrBuf;                        /*  输出环形缓冲区              */
    
    LW_OBJECT_HANDLE        TYDEV_hRdSyncSemB;                          /*  读同步信号量                */
    LW_OBJECT_HANDLE        TYDEV_hWrtSyncSemB;                         /*  写同步信号量                */
    LW_OBJECT_HANDLE        TYDEV_hDrainSyncSemB;                       /*  传输完毕同步信号量          */
    LW_OBJECT_HANDLE        TYDEV_hMutexSemM;                           /*  互斥操作信号量              */
    
    TY_DEV_RD_STATE         TYDEV_tydevrdstat;                          /*  读状态                      */
    TY_DEV_WR_STATE         TYDEV_tydevwrstat;                          /*  写状态                      */
    
    INT                     TYDEV_iWrtThreshold;                        /*  写缓冲区激活门限            */
    
    UINT8                   TYDEV_ucInNBytes;                           /*  在新的没有结束的行中的字符数*/
    UINT8                   TYDEV_ucInBytesLeft;                        /*  在新的没有结束的行中剩余的  */
                                                                        /*  字符数                      */
    FUNCPTR                 TYDEV_pfuncTxStartup;                       /*  启动发送的函数指针          */
    FUNCPTR                 TYDEV_pfuncProtoHook;                       /*  使用其他协议栈时的回调函数  */
    
    INT                     TYDEV_iProtoArg;                            /*  使用协议栈时的参数,         */
    INT                     TYDEV_iOpt;                                 /*  当前终端的工作参数及选项    */
    
    ULONG                   TYDEV_ulRTimeout;                           /*  读操作超时时间              */
    ULONG                   TYDEV_ulWTimeout;                           /*  写操作超时时间              */
    
    INT                     TYDEV_iAbortFlag;                           /*  abort 标志                  */
    
    FUNCPTR                 TYDEV_pfuncCtrlC;                           /*  control-C hook function     */
    PVOID                   TYDEV_pvArgCtrlC;                           /*  control-C hook function     */
    
    LW_SEL_WAKEUPLIST       TYDEV_selwulList;                           /*  select() 等待链             */
    time_t                  TYDEV_timeCreate;                           /*  创建时间                    */

    CHAR                    TYDEV_cCtlChars[NCCS];                      /*  termios 控制字符            */

    TY_DEV_WINSIZE          TYDEV_tydevwins;                            /*  窗口大小                    */

    LW_SPINLOCK_DEFINE     (TYDEV_slLock);                              /*  自旋锁                      */
} TY_DEV;

typedef TY_DEV             *TY_DEV_ID;
/*********************************************************************************************************
  TYCO_DEV
*********************************************************************************************************/

typedef struct {
    TY_DEV                  TYCODEV_tydevTyDev;                         /*  TY 设备                     */
    SIO_CHAN               *TYCODEV_psiochan;                           /*  同步I/O通道的功能函数       */
} TYCO_DEV;
/*********************************************************************************************************
  TTY INTERNAL FUNCTION
*********************************************************************************************************/

INT                         _TyDevInit(TY_DEV_ID  ptyDev, size_t stRdBufSize, size_t stWrtBufSize,
                                       FUNCPTR    pfunctxStartup);
INT                         _TyDevRemove(TY_DEV_ID  ptyDev);

INT                         _TyIRd(    TY_DEV_ID  ptyDev, CHAR   cInchar);
INT                         _TyITx(    TY_DEV_ID  ptyDev, PCHAR  pcChar);
ssize_t                     _TyRead(   TY_DEV_ID  ptyDev, PCHAR  pcBuffer, size_t  stMaxBytes);
ssize_t                     _TyWrite(  TY_DEV_ID  ptyDev, PCHAR  pcBuffer, size_t  stNBytes);
INT                         _TyIoctl(  TY_DEV_ID  ptyDev, INT    iRequest, LONG lArg);

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  TTY API
*********************************************************************************************************/

LW_API INT                  API_TtyDrvInstall(VOID);
LW_API INT                  API_TtyDevCreate(PCHAR     pcName,
                                             SIO_CHAN *psiochan,
                                             size_t    stRdBufSize,
                                             size_t    stWrtBufSize);
LW_API INT                  API_TtyDevRemove(PCHAR   pcName, BOOL  bForce);

LW_API VOID                 API_TyAbortFuncSet(FUNCPTR pfuncAbort);
LW_API VOID                 API_TyAbortSet(CHAR        cAbort);
LW_API VOID                 API_TyBackspaceSet(CHAR    cBackspace);
LW_API VOID                 API_TyDeleteLineSet(CHAR   cDeleteLine);
LW_API VOID                 API_TyEOFSet(CHAR  cEOF);
LW_API VOID                 API_TyMonitorTrapSet(CHAR  cMonitorTrap);

/*********************************************************************************************************
  API
*********************************************************************************************************/

#define  ttyDrv             API_TtyDrvInstall
#define  ttyDevCreate       API_TtyDevCreate
#define  ttyDevRemove       API_TtyDevRemove

#define  tyAbortFuncSet     API_TyAbortFuncSet
#define  tyAbortSet         API_TyAbortSet
#define  tyBackspaceSet     API_TyBackspaceSet
#define  tyDeleteLineSet    API_TyDeleteLineSet
#define  tyEOFSet           API_TyEOFSet
#define  tyMonitorTrapSet   API_TyMonitorTrapSet

/*********************************************************************************************************
  GLOBAL VAR
*********************************************************************************************************/

#ifdef  __TYCO_MAIN_FILE
#define __TYCO_EXT
#else
#define __TYCO_EXT    extern
#endif                                                                  /*  __TYCO_MAIN_FILE            */
#ifndef __TYCO_MAIN_FILE
__TYCO_EXT      INT         _G_iTycoDrvNum;                             /*  是否装载了TY驱动            */
#else
__TYCO_EXT      INT         _G_iTycoDrvNum = PX_ERROR;
#endif                                                                  /*  __TYCO_MAIN_FILE            */
#ifndef __TYCO_MAIN_FILE
__TYCO_EXT      ULONG       _G_ulMutexOptionsTyLib;
#else
__TYCO_EXT      ULONG       _G_ulMutexOptionsTyLib = (LW_OPTION_WAIT_PRIORITY
                                                   |  LW_OPTION_DELETE_SAFE
                                                   |  LW_OPTION_INHERIT_PRIORITY);
#endif                                                                  /*  __TYCO_MAIN_FILE            */
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SIO_DEVICE_EN > 0)  */
#endif                                                                  /*  __TTY_H                     */
/*********************************************************************************************************
  END
*********************************************************************************************************/

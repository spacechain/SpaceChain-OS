/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: spipe.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 02 月 27 日
**
** 描        述: 这是字符流式管道内部头文件 (STREAM PIPE)

** BUG
2007.04.08  加入了对裁剪的宏支持
2007.11.20  加入 select 功能.
2012.08.25  加入管道单端关闭检测:
            1: 如果读端关闭, 写操作将会收到 SIGPIPE 信号, write 将会返回 -1.
            2: 如果写端关闭, 读操作将会读完所有数据, 然后再次读返回 0.
*********************************************************************************************************/

#ifndef  __SPIPE_H
#define  __SPIPE_H

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SPIPE_EN > 0)

/*********************************************************************************************************
  PIPE RING BUFFER (环形缓冲)
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct {
    PCHAR                 RINGBUFFER_pcBuffer;                           /*  缓冲区基地址               */
    PCHAR                 RINGBUFFER_pcInPtr;                            /*  输入指针                   */
    PCHAR                 RINGBUFFER_pcOutPtr;                           /*  输出指针                   */
    size_t                RINGBUFFER_stTotalBytes;                       /*  缓冲区大小                 */
    size_t                RINGBUFFER_stMsgBytes;                         /*  有效数据量                 */
} LW_RING_BUFFER;
typedef LW_RING_BUFFER   *PLW_RING_BUFFER;

/*********************************************************************************************************
  SPIPE DEVICE
*********************************************************************************************************/

typedef struct {
    LW_DEV_HDR            SPIPEDEV_devhdrHdr;                           /*  设备头                      */
    LW_RING_BUFFER        SPIPEDEV_ringbufferBuffer;                    /*  环形缓冲区                  */
    LW_SEL_WAKEUPLIST     SPIPEDEV_selwulList;                          /*  等待链                      */
    
    LW_OBJECT_HANDLE      SPIPEDEV_hOpLock;                             /*  操作锁                      */
    LW_OBJECT_HANDLE      SPIPEDEV_hReadLock;                           /*  读锁                        */
    LW_OBJECT_HANDLE      SPIPEDEV_hWriteLock;                          /*  写锁                        */
    
    UINT                  SPIPEDEV_uiReadCnt;                           /*  读端打开数量                */
    UINT                  SPIPEDEV_uiWriteCnt;                          /*  写端打开数量                */
    BOOL                  SPIPEDEV_bUnlinkReq;                          /*  在最后一次关闭时删除        */
    
    ULONG                 SPIPEDEV_ulRTimeout;                          /*  读操作超时时间              */
    ULONG                 SPIPEDEV_ulWTimeout;                          /*  写操作超时时间              */
    
    INT                   SPIPEDEV_iAbortFlag;                          /*  abort 异常标志              */
    time_t                SPIPEDEV_timeCreate;                          /*  创建时间                    */
} LW_SPIPE_DEV;
typedef LW_SPIPE_DEV     *PLW_SPIPE_DEV;

/*********************************************************************************************************
  SPIPE FILE
*********************************************************************************************************/

typedef struct {
    PLW_SPIPE_DEV        SPIPEFIL_pspipedev;
    
    INT                  SPIPEFIL_iFlags;                               /*  建立属性                    */
    INT                  SPIPEFIL_iMode;                                /*  操作方式                    */
} LW_SPIPE_FILE;
typedef LW_SPIPE_FILE   *PLW_SPIPE_FILE;

/*********************************************************************************************************
  INTERNAL FUNCTION
*********************************************************************************************************/

LONG       _SpipeOpen( PLW_SPIPE_DEV  pspipedev, PCHAR  pcName,   INT  iFlags, INT  iMode);
INT        _SpipeRemove(PLW_SPIPE_DEV pspipedev, PCHAR  pcName);
INT        _SpipeClose(PLW_SPIPE_FILE pspipefil);
ssize_t    _SpipeRead( PLW_SPIPE_FILE pspipefil, PCHAR  pcBuffer, size_t stMaxBytes);
ssize_t    _SpipeWrite(PLW_SPIPE_FILE pspipefil, PCHAR  pcBuffer, size_t stNBytes);
INT        _SpipeIoctl(PLW_SPIPE_FILE pspipefil, INT    iRequest, INT  *piArgPtr);

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  SPIPE API
*********************************************************************************************************/

LW_API INT          API_SpipeDrvInstall(VOID);
LW_API INT          API_SpipeDevCreate(PCHAR  pcName, size_t  stBufferByteSize);
LW_API INT          API_SpipeDevDelete(PCHAR  pcName, BOOL bForce);

/*********************************************************************************************************
  API
*********************************************************************************************************/

#define  spipeDevCreate                               API_SpipeDevCreate
#define  spipeDevDelete                               API_SpipeDevDelete
#define  spipeDrv                                     API_SpipeDrvInstall

#endif                                                                  /*  __SPIPE_H                   */

/*********************************************************************************************************
  GLOBAL VAR
*********************************************************************************************************/

#ifdef  __SPIPE_MAIN_FILE
#define __SPIPE_EXT
#else
#define __SPIPE_EXT    extern
#endif

#ifndef __SPIPE_MAIN_FILE
__SPIPE_EXT    INT     _G_iSpipeDrvNum;                                 /*  是否装载了管道驱动          */
#else
__SPIPE_EXT    INT     _G_iSpipeDrvNum = PX_ERROR;
#endif

#ifndef __SPIPE_MAIN_FILE
__SPIPE_EXT    ULONG   _G_ulSpipeLockOpt;                               /*  锁信号量属性                */
#else
__SPIPE_EXT    ULONG   _G_ulSpipeLockOpt = (LW_OPTION_WAIT_PRIORITY
                                         |  LW_OPTION_DELETE_SAFE
                                         |  LW_OPTION_INHERIT_PRIORITY);
#endif

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SPIPE_EN > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: pipe.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 02 月 27 日
**
** 描        述: 这是 VxWorks 兼容管道内部头文件

** BUG
2007.04.08  加入了对裁剪的宏支持
2007.11.20  加入 select 功能.
*********************************************************************************************************/

#ifndef  __PIPE_H
#define  __PIPE_H

#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PIPE_EN > 0)

/*********************************************************************************************************
  PIPE DEVICE
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct {
    LW_DEV_HDR             PIPEDEV_devhdrHdr;
    LW_SEL_WAKEUPLIST      PIPEDEV_selwulList;                          /*  等待链                      */

    LW_OBJECT_HANDLE       PIPEDEV_hMsgQueue;
    
    INT                    PIPEDEV_iFlags;                              /*  建立属性                    */
    INT                    PIPEDEV_iMode;                               /*  操作方式                    */
    
    ULONG                  PIPEDEV_ulRTimeout;                          /*  读操作超时时间              */
    ULONG                  PIPEDEV_ulWTimeout;                          /*  写操作超时时间              */

    INT                    PIPEDEV_iAbortFlag;                          /*  abort 标志                  */
    time_t                 PIPEDEV_timeCreate;                          /*  创建时间                    */
} LW_PIPE_DEV;
typedef LW_PIPE_DEV       *PLW_PIPE_DEV;

/*********************************************************************************************************
  INTERNAL FUNCTION (_pipedev 为类型，避免 ppipedev 前缀命名出现歧异，ppipedev 应为 p_pipedev)
*********************************************************************************************************/

LONG    _PipeOpen( PLW_PIPE_DEV  p_pipedev, PCHAR  pcName,   INT  iFlags, INT  iMode);
INT     _PipeClose(PLW_PIPE_DEV  p_pipedev);
ssize_t _PipeRead( PLW_PIPE_DEV  p_pipedev, PCHAR  pcBuffer, size_t  stMaxBytes);
ssize_t _PipeWrite(PLW_PIPE_DEV  p_pipedev, PCHAR  pcBuffer, size_t  stNBytes);
INT     _PipeIoctl(PLW_PIPE_DEV  p_pipedev, INT    iRequest, INT  *piArgPtr);

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  PIPE API
*********************************************************************************************************/

LW_API INT          API_PipeDrvInstall(VOID);
LW_API INT          API_PipeDevCreate(PCHAR  pcName, ULONG  ulNMessages, size_t  stNBytes);
LW_API INT          API_PipeDevDelete(PCHAR  pcName, BOOL bForce);

/*********************************************************************************************************
  API
*********************************************************************************************************/

#define  pipeDevCreate                               API_PipeDevCreate
#define  pipeDevDelete                               API_PipeDevDelete
#define  pipeDrv                                     API_PipeDrvInstall

#endif                                                                  /*  __SPIPE_H                   */

/*********************************************************************************************************
  GLOBAL VAR
*********************************************************************************************************/

#ifdef  __PIPE_MAIN_FILE
#define __PIPE_EXT
#else
#define __PIPE_EXT    extern
#endif

#ifndef __PIPE_MAIN_FILE
__PIPE_EXT    INT     _G_iPipeDrvNum;                                   /*  是否装载了管道驱动          */
#else
__PIPE_EXT    INT     _G_iPipeDrvNum = PX_ERROR;
#endif

#ifndef __PIPE_MAIN_FILE
__PIPE_EXT    ULONG   _G_ulPipeLockOpt;                                 /*  锁信号量属性                */
#else
__PIPE_EXT    ULONG   _G_ulPipeLockOpt = LW_OPTION_WAIT_PRIORITY;
#endif

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_PIPE_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: pipeLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 02 月 27 日
**
** 描        述: VxWorks 兼容管道通信内部功能函数

** BUG
2007.04.09  _PipeRead()  函数修改了在中断中调用的问题。
2007.04.09  _PipeWrite() 函数修改了在中断中没有写入队列的问题。
2007.06.06  LINE72  应该是判断有效，再处理打开次数。
2007.09.21  加入错误管理.
2007.12.11  在清空管道信息时,需要激活写等待的线程.
2008.04.01  将 FIORTIMEOUT 和 FIOWTIMEOUT 设置超时时间的参数改为 struct timeval 类型.
            为 NULL 表示永久等待.
2008.05.18  使用 __KERNEL_ENTER() 代替 ThreadLock();
2009.02.09  ioctl 不可识别命令, 不打印错误信息.
2009.05.27  加入 abort 功能.
2009.07.11  去掉一些 GCC 警告和可能在 64 位处理器上产生的隐患.
2009.10.22  read write 返回值为 ssize_t.
2010.01.13  优化 write 操作.
2010.01.14  升级了 abort.
2010.09.09  支持 SELEXCEPT 操作. 当管道设备删除时将会被唤醒.
2011.08.09  st_size 管道为内部数据量.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#define  __PIPE_MAIN_FILE
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
** 函数名称: _PipeOpen
** 功能描述: 打开管道设备
** 输　入  : 
**           p_pipedev        管道设备控制块
**           pcName           管道名称
**           iFlags           方式
**           iMode            方法
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SPIPE_EN > 0)

LONG  _PipeOpen (PLW_PIPE_DEV  p_pipedev, 
                 PCHAR         pcName,   
                 INT           iFlags, 
                 INT           iMode)
{
    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & O_CREAT) {
            _ErrorHandle(ERROR_IO_FILE_EXIST);                          /*  不能重复创建                */
            return  (PX_ERROR);
        }
        
        p_pipedev->PIPEDEV_iFlags = iFlags;
        p_pipedev->PIPEDEV_iMode  = iMode;
        LW_DEV_INC_USE_COUNT(&p_pipedev->PIPEDEV_devhdrHdr);
        
        return  ((LONG)p_pipedev);
    }
}
/*********************************************************************************************************
** 函数名称: _PipeClose
** 功能描述: 关闭管道设备
** 输　入  : 
**           p_pipedev        管道设备控制块
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _PipeClose (PLW_PIPE_DEV  p_pipedev)
{
    if (p_pipedev) {
        LW_DEV_DEC_USE_COUNT(&p_pipedev->PIPEDEV_devhdrHdr);
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: _PipeRead
** 功能描述: 读管道设备
** 输　入  : 
**           p_pipedev        管道设备控制块
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t  _PipeRead (PLW_PIPE_DEV  p_pipedev, 
                    PCHAR         pcBuffer, 
                    size_t        stMaxBytes)
{
    REGISTER ULONG      ulError;
             size_t     stTemp = 0;
             ssize_t    sstNBytes;
    
    p_pipedev->PIPEDEV_iAbortFlag &= ~OPT_RABORT;                       /*  清除 abort 标志             */
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  中断调用                    */
        ulError = API_MsgQueueTryReceive(p_pipedev->PIPEDEV_hMsgQueue,
                                         (PVOID)pcBuffer,
                                         stMaxBytes,
                                         &stTemp);
    } else {                                                            /*  普通调用                    */
        ulError = API_MsgQueueReceive(p_pipedev->PIPEDEV_hMsgQueue,
                                      (PVOID)pcBuffer,
                                      stMaxBytes,
                                      &stTemp,
                                      p_pipedev->PIPEDEV_ulRTimeout);
    }

    sstNBytes = (ssize_t)stTemp;
    if (ulError) {                                                      /*  错误退出                    */
        return  (0);
    }
    
    if (p_pipedev->PIPEDEV_iAbortFlag & OPT_RABORT) {                   /*  abort                       */
        _ErrorHandle(ERROR_IO_ABORT);
        return  (0);
    }
    
    SEL_WAKE_UP_ALL(&p_pipedev->PIPEDEV_selwulList, SELWRITE);
    
    return  (sstNBytes);
}
/*********************************************************************************************************
** 函数名称: _SpipeWrite
** 功能描述: 写管道设备
** 输　入  : 
**           p_pipedev        管道设备控制块
**           pcBuffer         将要写入的数据指针
**           stNBytes         写入数据大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ssize_t  _PipeWrite (PLW_PIPE_DEV  p_pipedev, 
                     PCHAR         pcBuffer, 
                     size_t        stNBytes)
{
    REGISTER ULONG  ulError;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  在中断中调用                */
        ulError = API_MsgQueueSend(p_pipedev->PIPEDEV_hMsgQueue,
                                   (PVOID)pcBuffer, stNBytes);          /*  发送消息                    */
        if (ulError) {
            _ErrorHandle(ERROR_IO_DEVICE_TIMEOUT);
            return  (0);
        }
        
    } else {
        p_pipedev->PIPEDEV_iAbortFlag &= ~OPT_WABORT;                   /*  清除 abort 标志             */
        
        ulError = API_MsgQueueSend2(p_pipedev->PIPEDEV_hMsgQueue,
                                    (PVOID)pcBuffer, stNBytes,
                                    p_pipedev->PIPEDEV_ulWTimeout);     /*  发送消息                    */
        if (ulError) {                                                  /*  不可写                      */
            if (p_pipedev->PIPEDEV_iAbortFlag & OPT_WABORT) {           /*  abort                       */
                _ErrorHandle(ERROR_IO_ABORT);
            
            } else if (ulError == ERROR_THREAD_WAIT_TIMEOUT) {
                _ErrorHandle(ERROR_IO_DEVICE_TIMEOUT);
            }
            return  (0);
        }
    }
    
    SEL_WAKE_UP_ALL(&p_pipedev->PIPEDEV_selwulList, SELREAD);           /*  可以读了                    */
    
    return  ((ssize_t)stNBytes);
}
/*********************************************************************************************************
** 函数名称: _PipeIoctl
** 功能描述: 控制管道设备
** 输　入  : 
**           p_pipedev        管道设备控制块
**           iRequest         功能
**           piArgPtr         参数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  _PipeIoctl (PLW_PIPE_DEV  p_pipedev, 
                 INT           iRequest, 
                 INT          *piArgPtr)
{
    REGISTER INT                  iErrCode = ERROR_NONE;
    REGISTER PLW_SEL_WAKEUPNODE   pselwunNode;
             ULONG                ulMsgNum;
             ULONG                ulMsgMax;
             ULONG                ulTemp = 0;
             
             struct stat         *pstatGet;
    
    switch (iRequest) {
    
    case FIONREAD:                                                      /*  获得管道中数据的字节个数    */
        API_MsgQueueStatus(p_pipedev->PIPEDEV_hMsgQueue,
                           LW_NULL,
                           LW_NULL,
                           (size_t *)&ulTemp,
                           LW_NULL,
                           LW_NULL);
        *piArgPtr = (INT)ulTemp;
        break;
        
    case FIONMSGS:                                                      /*  获得管道中数据的个数        */
        API_MsgQueueStatus(p_pipedev->PIPEDEV_hMsgQueue,
                           LW_NULL,
                           &ulTemp,
                           LW_NULL,
                           LW_NULL,
                           LW_NULL);
        *piArgPtr = (INT)ulTemp;
        break;
        
    case FIOFLUSH:                                                      /*  清空数据                    */
        API_MsgQueueClear(p_pipedev->PIPEDEV_hMsgQueue);                /*  清除消息队列信息            */
        SEL_WAKE_UP_ALL(&p_pipedev->PIPEDEV_selwulList, SELWRITE);      /*  数据可以写入了              */
        break;
        
    case FIOFSTATGET: {                                                 /*  获取文件属性                */
        ULONG   ulNMsg;
        size_t  stNRead;
        
        API_MsgQueueStatus(p_pipedev->PIPEDEV_hMsgQueue,
                           LW_NULL,
                           &ulNMsg,
                           &stNRead,
                           LW_NULL,
                           LW_NULL);
        
        pstatGet = (struct stat *)piArgPtr;
        if (pstatGet) {
            pstatGet->st_dev     = (dev_t)p_pipedev;
            pstatGet->st_ino     = (ino_t)0;                            /*  相当于唯一节点              */
            pstatGet->st_mode    = 0666 | S_IFIFO;
            pstatGet->st_nlink   = 1;
            pstatGet->st_uid     = 0;
            pstatGet->st_gid     = 0;
            pstatGet->st_rdev    = 1;
            pstatGet->st_size    = (off_t)(ulNMsg * stNRead);
            pstatGet->st_blksize = 0;
            pstatGet->st_blocks  = 0;
            pstatGet->st_atime   = p_pipedev->PIPEDEV_timeCreate;
            pstatGet->st_mtime   = p_pipedev->PIPEDEV_timeCreate;
            pstatGet->st_ctime   = p_pipedev->PIPEDEV_timeCreate;
        } else {
            return  (PX_ERROR);
        }
    }
        break;
        
    case FIOSELECT:
        pselwunNode = (PLW_SEL_WAKEUPNODE)piArgPtr;
        SEL_WAKE_NODE_ADD(&p_pipedev->PIPEDEV_selwulList, pselwunNode);
        
        switch (pselwunNode->SELWUN_seltypType) {
        
        case SELREAD:                                                   /*  等待数据可读                */
            API_MsgQueueStatus(p_pipedev->PIPEDEV_hMsgQueue,
                               LW_NULL,
                               &ulMsgNum,
                               LW_NULL,
                               LW_NULL,
                               LW_NULL);
            if (ulMsgNum > 0) {
                SEL_WAKE_UP(pselwunNode);                               /*  唤醒节点                    */
            }
            break;
            
        case SELWRITE:
            API_MsgQueueStatus(p_pipedev->PIPEDEV_hMsgQueue,
                               &ulMsgMax,
                               &ulMsgNum,
                               LW_NULL,
                               LW_NULL,
                               LW_NULL);
            if (ulMsgNum < ulMsgMax) {
                SEL_WAKE_UP(pselwunNode);                               /*  唤醒节点                    */
            }
            break;
            
        case SELEXCEPT:                                                 /*  设备删除时将会被唤醒        */
            break;
        }
        break;

    case FIOUNSELECT:
	    SEL_WAKE_NODE_DELETE(&p_pipedev->PIPEDEV_selwulList, (PLW_SEL_WAKEUPNODE)piArgPtr);
        break;
        
    case FIORTIMEOUT:                                                   /*  设置读超时时间              */
        {
            struct timeval *ptvTimeout = (struct timeval *)piArgPtr;
            REGISTER ULONG  ulTick;
            if (ptvTimeout) {
                ulTick = __timevalToTick(ptvTimeout);                   /*  获得 tick 数量              */
                p_pipedev->PIPEDEV_ulRTimeout = ulTick;
            } else {
                p_pipedev->PIPEDEV_ulRTimeout = LW_OPTION_WAIT_INFINITE;
            }
        }
        break;
        
    case FIOWTIMEOUT:                                                   /*  设置写超时时间              */
        {
            struct timeval *ptvTimeout = (struct timeval *)piArgPtr;
            REGISTER ULONG  ulTick;
            if (ptvTimeout) {
                ulTick = __timevalToTick(ptvTimeout);                   /*  获得 tick 数量              */
                p_pipedev->PIPEDEV_ulWTimeout = ulTick;
            } else {
                p_pipedev->PIPEDEV_ulWTimeout = LW_OPTION_WAIT_INFINITE;
            }
        }
        break;

    case FIOWAITABORT:                                                  /*  停止当前等待 IO 线程        */
        if ((INT)piArgPtr & OPT_RABORT) {
            p_pipedev->PIPEDEV_iAbortFlag |= OPT_RABORT;
            API_MsgQueueFlushReceive(p_pipedev->PIPEDEV_hMsgQueue, LW_NULL);
        }
        if ((INT)piArgPtr & OPT_WABORT) {
            p_pipedev->PIPEDEV_iAbortFlag |= OPT_WABORT;
            API_MsgQueueFlushSend(p_pipedev->PIPEDEV_hMsgQueue, LW_NULL);
        }
        break;
        
    default:
        iErrCode = (PX_ERROR);
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        break;
    }
    
    return  (iErrCode);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PIPE_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/

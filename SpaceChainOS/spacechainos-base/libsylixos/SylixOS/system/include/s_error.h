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
** 文   件   名: s_error.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 02 月 13 日
**
** 描        述: 这是系统错误控制声明

** BUG:
2012.10.20  由于很多第三方库, 需要使用标准 I/O 的 errno, 所以今天开始统一第三方库的 errno.
*********************************************************************************************************/

#ifndef  __S_ERROR_H
#define  __S_ERROR_H

/*********************************************************************************************************
  SYSTEM
*********************************************************************************************************/

#define  ERROR_SYSTEM_LOW_MEMORY               49500                    /*  SYSTEM 堆缺少内存           */

/*********************************************************************************************************
  I/O system           50000 - 50500
*********************************************************************************************************/

#define  ERROR_IOS_DEVICE_NOT_FOUND            ENXIO                    /*  设备没有找到                */
#define  ERROR_IOS_DRIVER_GLUT                 50001                    /*  无空闲驱动块                */
#define  ERROR_IOS_INVALID_FILE_DESCRIPTOR     EBADF                    /*  文件描述符缺失              */
#define  ERROR_IOS_TOO_MANY_OPEN_FILES         EMFILE                   /*  文件打开的太多              */
#define  ERROR_IOS_DUPLICATE_DEVICE_NAME       EEXIST                   /*  重复命名                    */
#define  ERROR_IOS_DRIVER_NOT_SUP              ENOSYS                   /*  驱动程序不支持              */
#define  ERROR_IOS_FILE_OPERATIONS_NULL        50008                    /*  缺少 file_operations        */
#define  ERROR_IOS_FILE_NOT_SUP                ENOSYS                   /*  文件不支持相关操作          */
#define  ERROR_IOS_FILE_WRITE_PROTECTED        EWRPROTECT               /*  文件写保护                  */
#define  ERROR_IOS_FILE_READ_PROTECTED         50011                    /*  文件读保护                  */
#define  ERROR_IOS_FILE_SYMLINK                50012                    /*  文件为符号连接文件          */

/*********************************************************************************************************
  I/O                  50500 - 51000
*********************************************************************************************************/

#define  ERROR_IO_NO_DRIVER                    50500                    /*  缺少驱动程序                */
#define  ERROR_IO_UNKNOWN_REQUEST              ENOSYS                   /*  无效 ioctl 命令             */
#define  ERROR_IO_DEVICE_ERROR                 EIO                      /*  设备出错                    */
#define  ERROR_IO_DEVICE_TIMEOUT               ETIMEDOUT                /*  设备超时                    */
#define  ERROR_IO_WRITE_PROTECTED              EWRPROTECT               /*  设备写保护                  */
#define  ERROR_IO_DISK_NOT_PRESENT             ENOSYS                   /*  磁盘设备不支持              */
#define  ERROR_IO_CANCELLED                    ECANCELED                /*  当前操作被 cancel 终止      */
#define  ERROR_IO_NO_DEVICE_NAME_IN_PATH       EINVAL                   /*  没有设备名                  */
#define  ERROR_IO_NAME_TOO_LONG                ENAMETOOLONG             /*  名字太长                    */
#define  ERROR_IO_UNFORMATED                   EFORMAT                  /*  没有格式化或格式错误        */
#define  ERROR_IO_FILE_EXIST                   EEXIST                   /*  文件已经存在                */
#define  ERROR_IO_BUFFER_ERROR                 50513                    /*  缓冲区错误                  */
#define  ERROR_IO_ABORT                        ECANCELED                /*  ioctl FIOWAITABORT 异常     */

#define  ERROR_IO_ACCESS_DENIED                EACCES                   /*  文件访问失败 (无权限,       */
                                                                        /*  或者写操作只读文件等等)     */
#define  ERROR_IO_VOLUME_ERROR                 50518                    /*  卷错误                      */
#define  ERROR_IO_FILE_BUSY                    EBUSY                    /*  有重复路径被打开, 不能操作  */
#define  ERROR_IO_NO_FILE_NAME                 EFAULT                   /*  文件地址错误                */

/*********************************************************************************************************
  select
*********************************************************************************************************/

#define  ERROR_IO_SELECT_UNSUPPORT_IN_DRIVER   50600                    /*  驱动程序不支持 select       */
#define  ERROR_IO_SELECT_CONTEXT               50601                    /*  线程没有 select context 结构*/
#define  ERROR_IO_SELECT_WIDTH                 50602                    /*  最大文件号错误              */
#define  ERROR_IO_SELECT_FDSET_NULL            50603                    /*  文件集为空                  */

/*********************************************************************************************************
  THREAD POOL           51000 - 51500
*********************************************************************************************************/

#define  ERROR_THREADPOOL_NULL                 51000                    /*  线程池为空                  */
#define  ERROR_THREADPOOL_FULL                 51001                    /*  没有空闲线程池控制块        */
#define  ERROR_THREADPOOL_MAX_COUNTER          51004                    /*  线程最大数量错误            */

/*********************************************************************************************************
  SYSTEM HOOK LIST      51500 - 52000
*********************************************************************************************************/

#define  ERROR_SYSTEM_HOOK_NULL                51500                    /*  HOOK 为空                   */

/*********************************************************************************************************
  EXCEPT & log               52000 - 52500
*********************************************************************************************************/

#define  ERROR_EXCE_LOST                       52000                    /*  消息丢失                    */
                                                                        /*  except, netjob or hotplug...*/
#define  ERROR_LOG_LOST                        52001                    /*  LOG 消息丢失                */
#define  ERROR_LOG_FMT                         52002                    /*  LOG 格式化字串错误          */
#define  ERROR_LOG_FDSET_NULL                  52003                    /*  fd_set 为空                 */

/*********************************************************************************************************
  DMA                   52500 - 53000
*********************************************************************************************************/

#define  ERROR_DMA_CHANNEL_INVALID             52500                    /*  通道号无效                  */
#define  ERROR_DMA_TRANSMSG_INVALID            52501                    /*  传输控制块错误              */
#define  ERROR_DMA_DATA_TOO_LARGE              52502                    /*  数据量过大                  */
#define  ERROR_DMA_NO_FREE_NODE                52503                    /*  没有空闲的节点了            */
#define  ERROR_DMA_MAX_NODE                    52504                    /*  等待节点数已经到达上限      */

/*********************************************************************************************************
  POWER MANAGEMENT      53000 - 53500
*********************************************************************************************************/

#define  ERROR_POWERM_NODE                     53000                    /*  节点错误                    */
#define  ERROR_POWERM_TIME                     53001                    /*  时间错误                    */
#define  ERROR_POWERM_FUNCTION                 53002                    /*  回调函数错误                */
#define  ERROR_POWERM_NULL                     53003                    /*  句柄错误                    */
#define  ERROR_POWERM_FULL                     53004                    /*  没有空闲句柄                */
#define  ERROR_POWERM_STATUS                   53005                    /*  状态错误                    */

/*********************************************************************************************************
  SIGNAL MANAGEMENT      53500 - 54000
*********************************************************************************************************/

#define  ERROR_SIGNAL_SIGQUEUE_NODES_NULL      53500                    /*  缺少队列节点控制块          */

/*********************************************************************************************************
  HOTPLUG MANAGEMENT     54000 - 54500
*********************************************************************************************************/

#define  ERROR_HOTPLUG_POLL_NODE_NULL          54000                    /*  无法找到指定的 poll 节点    */
#define  ERROR_HOTPLUG_MESSAGE_NULL            54001                    /*  无法找到指定的 msg 节点     */

#endif                                                                  /*  __S_ERROR_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/

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
** 文   件   名: pipe.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 02 月 27 日
**
** 描        述: VxWorks 管道通信接口

** BUG
2007.06.05  LINE155 应该返回 ERROR_NONE 应是正常返回，设备创建成功。
2007.06.06  LINE246 应该返回 ERROR_NONE 应是正常返回，设备删除成功。
2007.09.21  加入错误管理.
2007.09.25  加入许可证管理.
2007.11.18  整理注释.
2007.11.21  加入 select 功能.
2007.11.21  在决定删除设备时,删除所有与设备相关的文件.
2007.12.11  在删除设备时,激活所有通过 select 等待的线程.
2008.01.13  加入 _ErrorHandle(ERROR_NONE); 语句.
2008.01.16  修改了信号量的名称.
2008.08.12  修改注释.
2009.12.09  修改注释.
2010.01.04  使用新的时区信息.
2010.01.14  升级了 abort.
2010.09.11  创建设备时, 指定设备类型.
2016.07.21  不再需要写同步信号量.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
** 函数名称: API_PipeDrvInstall
** 功能描述: 安装管道设备驱动程序
** 输　入  : VOID
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PIPE_EN > 0)

LW_API 
INT  API_PipeDrvInstall (VOID)
{
    if (_G_iPipeDrvNum <= 0) {
        _G_iPipeDrvNum  = iosDrvInstall((LONGFUNCPTR)LW_NULL,           /*  CREATE                      */
                                        (FUNCPTR)LW_NULL,               /*  DELETE                      */
                                        _PipeOpen,                      /*  OPEN                        */
                                        _PipeClose,                     /*  CLOSE                       */
                                        _PipeRead,                      /*  READ                        */
                                        _PipeWrite,                     /*  WRITE                       */
                                        _PipeIoctl);                    /*  IOCTL                       */
                                          
        DRIVER_LICENSE(_G_iPipeDrvNum,     "GPL->Ver 2.0");
        DRIVER_AUTHOR(_G_iPipeDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iPipeDrvNum, "VxWorks pipe driver.");
    }
    
    return  ((_G_iPipeDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** 函数名称: API_PipeDevCreate
** 功能描述: 创建一个管道设备
** 输　入  : 
**           pcName                        管道名称
**           ulNMessages                   管道中消息数量
**           stNBytes                      每一条消息的最大字节数
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_PipeDevCreate (PCHAR  pcName, 
                        ULONG  ulNMessages, 
                        size_t stNBytes)
{
    REGISTER PLW_PIPE_DEV        p_pipedev;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    if (_G_iPipeDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    p_pipedev = (PLW_PIPE_DEV)__SHEAP_ALLOC(sizeof(LW_PIPE_DEV));
    if (p_pipedev == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(p_pipedev, sizeof(LW_PIPE_DEV));
    
    p_pipedev->PIPEDEV_iAbortFlag = 0;
    p_pipedev->PIPEDEV_ulRTimeout = LW_OPTION_WAIT_INFINITE;             /*  初始化为永久等待           */
    p_pipedev->PIPEDEV_ulWTimeout = LW_OPTION_WAIT_INFINITE;             /*  初始化为永久等待           */
    
    p_pipedev->PIPEDEV_hMsgQueue  = API_MsgQueueCreate("pipe_msg",
                                                       ulNMessages, stNBytes,
                                                       _G_ulPipeLockOpt | LW_OPTION_OBJECT_GLOBAL,
                                                       LW_NULL);
    if (!p_pipedev->PIPEDEV_hMsgQueue) {
        __SHEAP_FREE((PVOID)p_pipedev);
        return  (PX_ERROR);
    }
    
    SEL_WAKE_UP_LIST_INIT(&p_pipedev->PIPEDEV_selwulList);
    
    if (iosDevAddEx(&p_pipedev->PIPEDEV_devhdrHdr, pcName, _G_iPipeDrvNum, DT_FIFO) != ERROR_NONE) {
        API_MsgQueueDelete(&p_pipedev->PIPEDEV_hMsgQueue);
        SEL_WAKE_UP_LIST_TERM(&p_pipedev->PIPEDEV_selwulList);
        __SHEAP_FREE((PVOID)p_pipedev);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    p_pipedev->PIPEDEV_timeCreate = lib_time(LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PipeDevDelete
** 功能描述: 删除一个管道设备
** 输　入  : 
**           pcName                        管道名称
**           bForce                        强制删除
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_PipeDevDelete (PCHAR  pcName, BOOL  bForce)
{
    REGISTER PLW_PIPE_DEV        p_pipedev;
             PCHAR               pcTail = LW_NULL;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    if (_G_iPipeDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    p_pipedev = (PLW_PIPE_DEV)iosDevFind(pcName, &pcTail);
    if ((p_pipedev == LW_NULL) || (pcName == pcTail)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device not found.\r\n");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
    
    if (bForce == LW_FALSE) {
        if (LW_DEV_GET_USE_COUNT(&p_pipedev->PIPEDEV_devhdrHdr)) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "too many open files.\r\n");
            _ErrorHandle(EBUSY);
            return  (PX_ERROR);
        }
        if (SEL_WAKE_UP_LIST_LEN(&p_pipedev->PIPEDEV_selwulList) > 0) {
            errno = EBUSY;
            return  (PX_ERROR);
        }
    }
    
    iosDevFileAbnormal(&p_pipedev->PIPEDEV_devhdrHdr);
    iosDevDelete(&p_pipedev->PIPEDEV_devhdrHdr);

    SEL_WAKE_UP_LIST_TERM(&p_pipedev->PIPEDEV_selwulList);
    
    API_MsgQueueDelete(&p_pipedev->PIPEDEV_hMsgQueue);

    __SHEAP_FREE((PVOID)p_pipedev);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_PIPE_EN > 0)        */
/*********************************************************************************************************
  END
*********************************************************************************************************/

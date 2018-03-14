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
** 文   件   名: spipe.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 02 月 27 日
**
** 描        述: 字符流管道通信接口 (多线程同时写并且多线程同时读时, 超时时间可能不准确)

** BUG
2007.06.06  LINE259 正常返回值错误。
2007.09.21  加入操作锁句柄进行互斥.
2007.09.21  加入错误管理.
2007.09.25  加入许可证管理.
2007.11.18  整理注释.
2007.11.20  加入 select 功能.
2007.11.21  创建 spipe 设备时, 环形缓冲区的基地址完全错误, (此错误竟隐藏了几个月, 该反思了).
2007.11.21  在决定删除设备时,删除所有与设备相关的文件.
2007.12.11  在删除设备时,激活所有通过 select 等待的线程.
2008.01.13  加入 _ErrorHandle(ERROR_NONE); 语句.
2008.01.16  修改了信号量的名称.
2008.01.20  删除操作时,要获得设备操作权利.
2008.08.12  修改注释.
2009.03.06  修改初始化顺序, 确保准确无误.
2009.12.09  修改注释.
2010.09.11  创建设备时, 指定设备类型.
2012.08.25  加入管道单端关闭检测:
            1: 如果读端关闭, 写操作将会收到 SIGPIPE 信号, write 将会返回 -1.
            2: 如果写端关闭, 读操作将会读完所有数据, 然后再次读返回 0.
2013.06.12  管道大小不得小于 PIPE_BUF.
2013.10.03  修正管道设备创建失败后对内存的释放错误.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "limits.h"
/*********************************************************************************************************
** 函数名称: API_SpipeDrvInstall
** 功能描述: 安装字符流管道设备驱动程序
** 输　入  : VOID
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SPIPE_EN > 0)

LW_API 
INT  API_SpipeDrvInstall (VOID)
{
    if (_G_iSpipeDrvNum <= 0) {
        _G_iSpipeDrvNum  = iosDrvInstall(LW_NULL,                       /*  CREATE                      */
                                         _SpipeRemove,                  /*  DELETE                      */
                                         _SpipeOpen,                    /*  OPEN                        */
                                         _SpipeClose,                   /*  CLOSE                       */
                                         _SpipeRead,                    /*  READ                        */
                                         _SpipeWrite,                   /*  WRITE                       */
                                         _SpipeIoctl);                  /*  IOCTL                       */
        
        DRIVER_LICENSE(_G_iSpipeDrvNum,     "GPL->Ver 2.0");
        DRIVER_AUTHOR(_G_iSpipeDrvNum,      "Han.hui");
        DRIVER_DESCRIPTION(_G_iSpipeDrvNum, "stream pipe driver.");
    }
    
    return  ((_G_iSpipeDrvNum == (PX_ERROR)) ? (PX_ERROR) : (ERROR_NONE));
}
/*********************************************************************************************************
** 函数名称: API_SpipeDevCreate
** 功能描述: 创建一个字符流管道设备
** 输　入  : 
**           pcName                        字符流管道名称
**           stBufferByteSize              缓冲区大小
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_SpipeDevCreate (PCHAR  pcName, size_t  stBufferByteSize)
{
    REGISTER PLW_SPIPE_DEV       pspipedev;
    REGISTER PLW_RING_BUFFER     pringbuffer;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    if (stBufferByteSize < PIPE_BUF) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (_G_iSpipeDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    pspipedev = (PLW_SPIPE_DEV)__SHEAP_ALLOC(sizeof(LW_SPIPE_DEV) + stBufferByteSize);
    if (pspipedev == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pspipedev, sizeof(LW_SPIPE_DEV));
    
    pspipedev->SPIPEDEV_uiReadCnt  = 0;
    pspipedev->SPIPEDEV_uiWriteCnt = 0;
    pspipedev->SPIPEDEV_bUnlinkReq = LW_FALSE;
    pspipedev->SPIPEDEV_iAbortFlag = 0;
    pspipedev->SPIPEDEV_ulRTimeout = LW_OPTION_WAIT_INFINITE;            /*  初始化为永久等待           */
    pspipedev->SPIPEDEV_ulWTimeout = LW_OPTION_WAIT_INFINITE;            /*  初始化为永久等待           */
    
    /*
     *  由于采用 test-pend 机制, 全部初始化为无效状态.
     */
    pspipedev->SPIPEDEV_hReadLock = API_SemaphoreBCreate("pipe_rsync",   /*  create lock                */
                                                         LW_FALSE, 
                                                         _G_ulSpipeLockOpt | LW_OPTION_OBJECT_GLOBAL, 
                                                         LW_NULL);
    if (!pspipedev->SPIPEDEV_hReadLock) {
        __SHEAP_FREE((PVOID)pspipedev);
        return  (PX_ERROR);
    }
    
    pspipedev->SPIPEDEV_hWriteLock = API_SemaphoreBCreate("pipe_wsync", 
                                                          LW_FALSE,
                                                          _G_ulSpipeLockOpt | LW_OPTION_OBJECT_GLOBAL,
                                                          LW_NULL);
                                                        
    if (!pspipedev->SPIPEDEV_hWriteLock) {
        API_SemaphoreBDelete(&pspipedev->SPIPEDEV_hReadLock);
        __SHEAP_FREE((PVOID)pspipedev);
        return  (PX_ERROR);
    }
    
    pspipedev->SPIPEDEV_hOpLock = API_SemaphoreMCreate("pipe_lock", 
                                                        LW_PRIO_DEF_CEILING,
                                                        _G_ulSpipeLockOpt | LW_OPTION_OBJECT_GLOBAL,
                                                        LW_NULL);
    if (!pspipedev->SPIPEDEV_hOpLock) {
        API_SemaphoreBDelete(&pspipedev->SPIPEDEV_hReadLock);
        API_SemaphoreBDelete(&pspipedev->SPIPEDEV_hWriteLock);
        __SHEAP_FREE((PVOID)pspipedev);
        return  (PX_ERROR);
    }
    
    SEL_WAKE_UP_LIST_INIT(&pspipedev->SPIPEDEV_selwulList);
    
    pringbuffer = &pspipedev->SPIPEDEV_ringbufferBuffer;
    
    pringbuffer->RINGBUFFER_pcBuffer = (PCHAR)((UINT8 *)(pspipedev) + sizeof(LW_SPIPE_DEV));
    pringbuffer->RINGBUFFER_pcInPtr  = pringbuffer->RINGBUFFER_pcBuffer;
    pringbuffer->RINGBUFFER_pcOutPtr = pringbuffer->RINGBUFFER_pcBuffer;
    
    pringbuffer->RINGBUFFER_stTotalBytes = stBufferByteSize;
    pringbuffer->RINGBUFFER_stMsgBytes   = 0;
    
    if (iosDevAddEx(&pspipedev->SPIPEDEV_devhdrHdr, pcName, _G_iSpipeDrvNum, DT_FIFO) != ERROR_NONE) {
        API_SemaphoreBDelete(&pspipedev->SPIPEDEV_hReadLock);
        API_SemaphoreBDelete(&pspipedev->SPIPEDEV_hWriteLock);
        API_SemaphoreMDelete(&pspipedev->SPIPEDEV_hOpLock);
        SEL_WAKE_UP_LIST_TERM(&pspipedev->SPIPEDEV_selwulList);
        __SHEAP_FREE((PVOID)pspipedev);
        return  (PX_ERROR);
    }
    
    pspipedev->SPIPEDEV_timeCreate = lib_time(LW_NULL);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SpipeDevDelete
** 功能描述: 删除一个字符流管道设备
** 输　入  : 
**           pcName                        字符流管道名称
**           bForce                        强制删除
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_SpipeDevDelete (PCHAR  pcName, BOOL  bForce)
{
    REGISTER PLW_SPIPE_DEV       pspipedev;
             PCHAR               pcTail = LW_NULL;

    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }
    
    if (_G_iSpipeDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    pspipedev = (PLW_SPIPE_DEV)iosDevFind(pcName, &pcTail);
    if ((pspipedev == LW_NULL) || (pcName == pcTail)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device not found.\r\n");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
    
    if (bForce == LW_FALSE) {
        if (LW_DEV_GET_USE_COUNT(&pspipedev->SPIPEDEV_devhdrHdr)) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "too many open files.\r\n");
            _ErrorHandle(EBUSY);
            return  (PX_ERROR);
        }
        if (SEL_WAKE_UP_LIST_LEN(&pspipedev->SPIPEDEV_selwulList) > 0) {
            errno = EBUSY;
            return  (PX_ERROR);
        }
    }
    
    if (API_SemaphoreMPend(pspipedev->SPIPEDEV_hOpLock, LW_OPTION_WAIT_INFINITE)) {
        return  (PX_ERROR);
    }
    
    iosDevFileAbnormal(&pspipedev->SPIPEDEV_devhdrHdr);
    iosDevDelete(&pspipedev->SPIPEDEV_devhdrHdr);
    
    SEL_WAKE_UP_LIST_TERM(&pspipedev->SPIPEDEV_selwulList);
    
    API_SemaphoreBDelete(&pspipedev->SPIPEDEV_hReadLock);
    API_SemaphoreBDelete(&pspipedev->SPIPEDEV_hWriteLock);
    API_SemaphoreMDelete(&pspipedev->SPIPEDEV_hOpLock);
    
    __SHEAP_FREE((PVOID)pspipedev);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SPIPE_EN > 0)       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

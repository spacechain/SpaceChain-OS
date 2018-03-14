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
** 文   件   名: buzzer.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2016 年 07 月 23 日
**
** 描        述: 标准蜂鸣器驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_BUZZER_EN > 0)
/*********************************************************************************************************
  BUZZER 设备
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR           BUZZER_devhdr;                                 /*  设备头                      */
    PLW_BUZZER_FUNCS     BUZZER_pbuzzerfuncs;                           /*  操作函数集                  */
    LW_OBJECT_HANDLE     BUZZER_ulThread;
    LW_OBJECT_HANDLE     BUZZER_ulMsgQ;
    time_t               BUZZER_timeCreate;
} LW_BUZZER_DEV;
typedef LW_BUZZER_DEV   *PLW_BUZZER_DEV;
/*********************************************************************************************************
  BUZZER 文件
*********************************************************************************************************/
typedef struct {
    PLW_BUZZER_DEV       BUZFIL_pbuzzer;
    INT                  BUZFIL_iFlags;
} LW_BUZZER_FILE;
typedef LW_BUZZER_FILE  *PLW_BUZZER_FILE;
/*********************************************************************************************************
  定义全局变量, 用于保存 buzzer 驱动号
*********************************************************************************************************/
static INT _G_iBuzzerDrvNum = PX_ERROR;
/*********************************************************************************************************
** 函数名称: __buzzerOpen
** 功能描述: BUZZER 设备打开
** 输　入  : pbuzzer          设备
**           pcName           设备名称
**           iFlags           打开设备时使用的标志
**           iMode            打开的方式，保留
** 输　出  : BUZZER 设备指针
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LONG  __buzzerOpen (PLW_BUZZER_DEV   pbuzzer,
                           PCHAR            pcName,
                           INT              iFlags,
                           INT              iMode)
{
    PLW_BUZZER_FILE  pbuzfil;

    if (pcName == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "device name invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    
    } else {
        if (iFlags & O_CREAT) {
            _ErrorHandle(ERROR_IO_FILE_EXIST);                          /*  不能重复创建                */
            return  (PX_ERROR);
        }
    
        pbuzfil = (PLW_BUZZER_FILE)__SHEAP_ALLOC(sizeof(LW_BUZZER_FILE));
        if (!pbuzfil) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            return  (PX_ERROR);
        }
        
        pbuzfil->BUZFIL_iFlags  = iFlags;
        pbuzfil->BUZFIL_pbuzzer = pbuzzer;
        
        LW_DEV_INC_USE_COUNT(&pbuzzer->BUZZER_devhdr);
        
        return  ((LONG)pbuzfil);
    }
}
/*********************************************************************************************************
** 函数名称: __buzzerClose
** 功能描述: BUZZER 设备关闭
** 输　入  : pbuzfil          文件
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __buzzerClose (PLW_BUZZER_FILE   pbuzfil)
{
    PLW_BUZZER_DEV   pbuzzer;

    if (pbuzfil) {
        pbuzzer = pbuzfil->BUZFIL_pbuzzer;
    
        LW_DEV_DEC_USE_COUNT(&pbuzzer->BUZZER_devhdr);
            
        __SHEAP_FREE(pbuzfil);
        
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __buzzerWrite
** 功能描述: 写 BUZZER 设备
** 输　入  : pbuzfil          文件
**           pbmsg            写缓冲区指针
**           stNBytes         发送缓冲区字节数
** 输　出  : 返回实际写入的个数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t __buzzerWrite (PLW_BUZZER_FILE   pbuzfil,
                              PBUZZER_MSG       pbmsg,
                              size_t            stNBytes)
{
    INT              iCnt;
    ssize_t          sstRet = 0;
    ULONG            ulTimeout;
    PLW_BUZZER_DEV   pbuzzer;
    
    if (!pbmsg || !stNBytes) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iCnt = stNBytes / sizeof(BUZZER_MSG);
    if (iCnt % sizeof(BUZZER_MSG)) {
        _ErrorHandle(EMSGSIZE);
        return  (PX_ERROR);
    }
    
    if (pbuzfil->BUZFIL_iFlags & O_NONBLOCK) {
        ulTimeout = LW_OPTION_NOT_WAIT;
    } else {
        ulTimeout = LW_OPTION_WAIT_INFINITE;
    }
    
    pbuzzer = pbuzfil->BUZFIL_pbuzzer;
    
    for (; iCnt > 0; iCnt--) {
        if (API_MsgQueueSend2(pbuzzer->BUZZER_ulThread,
                              pbmsg, sizeof(BUZZER_MSG),
                              ulTimeout)) {
            break;
        }
        sstRet += sizeof(BUZZER_MSG);
    }
    
    if (sstRet == 0) {
        _ErrorHandle(EAGAIN);
    }
    
    return  ((ssize_t)sstRet);
}
/*********************************************************************************************************
** 函数名称: __buzzerIoctl
** 功能描述: BUZZER 设备控制
** 输　入  : pbuzfil          文件
**           iCmd             控制命令
**           lArg             参数
** 输　出  : ERROR_NONE or PX_ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __buzzerIoctl (PLW_BUZZER_FILE   pbuzfil, INT  iCmd, LONG  lArg)
{
    INT             iError = ERROR_NONE;
    struct stat    *pstatGet;
    PLW_BUZZER_DEV  pbuzzer = pbuzfil->BUZFIL_pbuzzer;

    switch (iCmd) {
    
    case FIONBIO:
        if (*(INT *)lArg) {
            pbuzfil->BUZFIL_iFlags |= O_NONBLOCK;
        } else {
            pbuzfil->BUZFIL_iFlags &= ~O_NONBLOCK;
        }
        break;
        
    case FIOFLUSH:
        API_MsgQueueClear(pbuzzer->BUZZER_ulMsgQ);
        API_ThreadWakeup(pbuzzer->BUZZER_ulThread);
        break;
    
    case FIOFSTATGET:                                                   /*  获取文件属性                */
        pstatGet = (struct stat *)lArg;
        if (pstatGet) {
            pstatGet->st_dev     = (dev_t)pbuzzer;
            pstatGet->st_ino     = (ino_t)pbuzfil;                      /*  相当于唯一节点              */
            pstatGet->st_mode    = 0222 | S_IFCHR;
            pstatGet->st_nlink   = 1;
            pstatGet->st_uid     = 0;
            pstatGet->st_gid     = 0;
            pstatGet->st_rdev    = 1;
            pstatGet->st_size    = 0;
            pstatGet->st_blksize = 0;
            pstatGet->st_blocks  = 0;
            pstatGet->st_atime   = pbuzzer->BUZZER_timeCreate;
            pstatGet->st_mtime   = pbuzzer->BUZZER_timeCreate;
            pstatGet->st_ctime   = pbuzzer->BUZZER_timeCreate;
        
        } else {
            _ErrorHandle(EINVAL);
            iError = PX_ERROR;
        }
        break;
        
    default:
        _ErrorHandle(ENOSYS);
        iError = PX_ERROR;
        break;
    }
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __buzzerThread
** 功能描述: BUZZER 控制线程
** 输　入  : pbuzzer       设备
** 输　出  : LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PVOID __buzzerThread (PVOID  pvArg)
{
    PLW_BUZZER_DEV    pbuzzer = (PLW_BUZZER_DEV)pvArg;
    ULONG             ulError;
    ULONG             ulCounter;
    BUZZER_MSG        bmsg;
    
    for (;;) {
        ulError = API_MsgQueueReceive(pbuzzer->BUZZER_ulMsgQ, &bmsg,
                                      sizeof(BUZZER_MSG), LW_NULL,
                                      LW_OPTION_WAIT_INFINITE);
        if (ulError) {
            continue;
        }
        
        if (bmsg.BUZZER_uiOn) {
            pbuzzer->BUZZER_pbuzzerfuncs->BUZZER_pfuncOn(pbuzzer->BUZZER_pbuzzerfuncs, bmsg.BUZZER_uiHz);
        } else {
            pbuzzer->BUZZER_pbuzzerfuncs->BUZZER_pfuncOff(pbuzzer->BUZZER_pbuzzerfuncs);
        }
        
        API_TimeMSleep(bmsg.BUZZER_uiMs);
        
        ulError = API_MsgQueueStatus(pbuzzer->BUZZER_ulMsgQ, LW_NULL, &ulCounter,
                                     LW_NULL, LW_NULL, LW_NULL);
        if (ulError) {
            continue;
        }
        
        if (!ulCounter) {
            pbuzzer->BUZZER_pbuzzerfuncs->BUZZER_pfuncOff(pbuzzer->BUZZER_pbuzzerfuncs);
        }
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_BuzzerDrvInstall
** 功能描述: 安装 Buzzer 驱动程序
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_BuzzerDrvInstall (void)
{
    if (_G_iBuzzerDrvNum > 0) {
        return  (ERROR_NONE);
    }

    _G_iBuzzerDrvNum = iosDrvInstall(__buzzerOpen, LW_NULL, __buzzerOpen, __buzzerClose,
                                     LW_NULL, __buzzerWrite, __buzzerIoctl);

    DRIVER_LICENSE(_G_iBuzzerDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iBuzzerDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iBuzzerDrvNum, "Buzzer driver.");

    return  (_G_iBuzzerDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_BuzzerDevCreate
** 功能描述: 创建 Buzzer 设备
** 输　入  : pcName           设备名
**           ulMaxQSize       设备缓冲消息数量
**           pbuzzerfuncs     设备驱动
** 输　出  : 是否成功
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_BuzzerDevCreate (CPCHAR            pcName, 
                          ULONG             ulMaxQSize,
                          PLW_BUZZER_FUNCS  pbuzzerfuncs)
{
    PLW_BUZZER_DEV      pbuzzerdev;
    LW_CLASS_THREADATTR threadattr;
    
    if (!pcName || !pbuzzerfuncs || (ulMaxQSize < 1)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (_G_iBuzzerDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    pbuzzerdev = (PLW_BUZZER_DEV)__SHEAP_ALLOC(sizeof(LW_BUZZER_DEV));
    if (pbuzzerdev == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pbuzzerdev, sizeof(LW_BUZZER_DEV));
    
    pbuzzerdev->BUZZER_pbuzzerfuncs = pbuzzerfuncs;
    pbuzzerdev->BUZZER_ulMsgQ = API_MsgQueueCreate("buzzer_q",
                                                   ulMaxQSize, sizeof(BUZZER_MSG),
                                                   LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL,
                                                   LW_NULL);
    if (pbuzzerdev->BUZZER_ulMsgQ == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pbuzzerdev);
        return  (PX_ERROR);
    }
    
    API_ThreadAttrBuild(&threadattr, 
                        LW_CFG_THREAD_DEFAULT_STK_SIZE, 
                        LW_PRIO_T_BUS, 
                        (LW_OPTION_THREAD_STK_CHK | 
                         LW_OPTION_OBJECT_GLOBAL), 
                        (PVOID)pbuzzerdev);
    
    pbuzzerdev->BUZZER_ulThread = API_ThreadInit("t_buzzer",
                                                 __buzzerThread,
                                                 &threadattr,
                                                 LW_NULL);
    if (pbuzzerdev->BUZZER_ulThread == LW_OBJECT_HANDLE_INVALID) {
        API_MsgQueueDelete(&pbuzzerdev->BUZZER_ulMsgQ);
        __SHEAP_FREE(pbuzzerdev);
        return  (PX_ERROR);
    }
    
    if (iosDevAddEx(&pbuzzerdev->BUZZER_devhdr, pcName, _G_iBuzzerDrvNum, DT_CHR) != ERROR_NONE) {
        API_ThreadDelete(&pbuzzerdev->BUZZER_ulThread, LW_NULL);
        API_MsgQueueDelete(&pbuzzerdev->BUZZER_ulMsgQ);
        __SHEAP_FREE(pbuzzerdev);
        return  (PX_ERROR);
    }
    
    pbuzzerdev->BUZZER_timeCreate = lib_time(LW_NULL);
    
    API_ThreadStart(pbuzzerdev->BUZZER_ulThread);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_BUZZER_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/

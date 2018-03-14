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
** 文   件   名: hwrtc.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 01 月 04 日
**
** 描        述: 硬件 RTC 设备管理组件. (注意: 硬件 RTC 接口应该为 UTC 时间)

** BUG:
2010.09.11  创建设备时, 指定设备类型.
2011.06.11  加入 API_RtcToRoot() 函数.
2012.03.11  __rootFsTimeSet() 为 UTC 时间.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_RTC_EN > 0)
/*********************************************************************************************************
  RTC 设备名
*********************************************************************************************************/
#define __LW_RTC_DEV_NAME               "/dev/rtc"
/*********************************************************************************************************
  RTC 设备
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR           RTCDEV_devhdr;                                 /*  设备头                      */
    PLW_RTC_FUNCS        RTCDEV_prtcfuncs;                              /*  操作函数集                  */
} LW_RTC_DEV;
typedef LW_RTC_DEV      *PLW_RTC_DEV;
/*********************************************************************************************************
  驱动程序
*********************************************************************************************************/
static LONG     __rtcOpen(PLW_RTC_DEV    prtcdev, 
                          PCHAR          pcName,   
                          INT            iFlags, 
                          INT            iMode);
static INT      __rtcClose(PLW_RTC_DEV    prtcdev);
static INT      __rtcIoctl(PLW_RTC_DEV    prtcdev, INT  iCmd, PVOID  pvArg);
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static INT      _G_iRtcDrvNum = PX_ERROR;
/*********************************************************************************************************
** 函数名称: API_RtcDrvInstall
** 功能描述: 安装 RTC 设备驱动程序
** 输　入  : VOID
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_RtcDrvInstall (VOID)
{
    if (_G_iRtcDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    _G_iRtcDrvNum = iosDrvInstall(__rtcOpen, 
                                  (FUNCPTR)LW_NULL, 
                                  __rtcOpen,
                                  __rtcClose,
                                  LW_NULL,
                                  LW_NULL,
                                  __rtcIoctl);
    DRIVER_LICENSE(_G_iRtcDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iRtcDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iRtcDrvNum, "hardware rtc.");
    
    return  ((_G_iRtcDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** 函数名称: API_RtcDevCreate
** 功能描述: 建立一个 RTC 设备
** 输　入  : prtcfuncs     rtc 操作函数集
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_RtcDevCreate (PLW_RTC_FUNCS    prtcfuncs)
{
    PLW_RTC_DEV     prtcdev;
    
    if (prtcfuncs == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (_G_iRtcDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "no driver.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    
    prtcdev = (PLW_RTC_DEV)__SHEAP_ALLOC(sizeof(LW_RTC_DEV));
    if (prtcdev == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(prtcdev, sizeof(LW_RTC_DEV));
    
    prtcdev->RTCDEV_prtcfuncs = prtcfuncs;
    
    if (iosDevAddEx(&prtcdev->RTCDEV_devhdr, __LW_RTC_DEV_NAME, _G_iRtcDrvNum, DT_CHR) != ERROR_NONE) {
        __SHEAP_FREE((PVOID)prtcdev);
        return  (PX_ERROR);
    }
    
    if (prtcfuncs->RTC_pfuncInit) {
        prtcfuncs->RTC_pfuncInit();                                     /*  初始化硬件 RTC              */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __rtcOpen
** 功能描述: 打开 RTC 设备
** 输　入  : prtcdev          rtc 设备
**           pcName           名字
**           iFlags           方式
**           iMode            方法
** 输　出  : 设备
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LONG  __rtcOpen (PLW_RTC_DEV    prtcdev, 
                        PCHAR          pcName,   
                        INT            iFlags, 
                        INT            iMode)
{
    LW_DEV_INC_USE_COUNT(&prtcdev->RTCDEV_devhdr);
    
    return  ((LONG)prtcdev);
}
/*********************************************************************************************************
** 函数名称: __rtcClose
** 功能描述: 关闭 RTC 设备
** 输　入  : prtcdev          rtc 设备
** 输　出  : 设备
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __rtcClose (PLW_RTC_DEV    prtcdev)
{
    if (prtcdev) {
        LW_DEV_DEC_USE_COUNT(&prtcdev->RTCDEV_devhdr);
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __rtcIoctl
** 功能描述: 控制 RTC 设备
** 输　入  : prtcdev          rtc 设备
**           iCmd             控制命令
**           pvArg            参数
** 输　出  : 设备
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __rtcIoctl (PLW_RTC_DEV    prtcdev, INT  iCmd, PVOID  pvArg)
{
    struct stat *pstat;

    switch (iCmd) {
    
    case FIOGETTIME:                                                    /*  获得 rtc 时间               */
        if (prtcdev->RTCDEV_prtcfuncs->RTC_pfuncGet) {
            return  prtcdev->RTCDEV_prtcfuncs->RTC_pfuncGet(prtcdev->RTCDEV_prtcfuncs,
                                                            (time_t *)pvArg);
        }
        break;
        
    case FIOSETTIME:                                                    /*  设置 rtc 时间               */
        if (prtcdev->RTCDEV_prtcfuncs->RTC_pfuncSet) {
            return  prtcdev->RTCDEV_prtcfuncs->RTC_pfuncSet(prtcdev->RTCDEV_prtcfuncs,
                                                            (time_t *)pvArg);
        }
        break;
        
    case FIOFSTATGET:                                                   /*  获得文件属性                */
        pstat = (struct stat *)pvArg;
        pstat->st_dev     = (dev_t)prtcdev;
        pstat->st_ino     = (ino_t)0;                                   /*  相当于唯一节点              */
        pstat->st_mode    = 0644 | S_IFCHR;                             /*  默认属性                    */
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;
        pstat->st_gid     = 0;
        pstat->st_rdev    = 1;
        pstat->st_size    = 0;
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
        pstat->st_atime   = API_RootFsTime(LW_NULL);                    /*  默认使用 root fs 基准时间   */
        pstat->st_mtime   = API_RootFsTime(LW_NULL);
        pstat->st_ctime   = API_RootFsTime(LW_NULL);
        return  (ERROR_NONE);
        
    default:
        break;
    }
    
    if (prtcdev->RTCDEV_prtcfuncs->RTC_pfuncIoctl) {
        return  prtcdev->RTCDEV_prtcfuncs->RTC_pfuncIoctl(prtcdev->RTCDEV_prtcfuncs,
                                                          iCmd,
                                                          pvArg);
    } else {
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_RtcSet
** 功能描述: 设置硬件 RTC 设备时间: UTC 时间
** 输　入  : time          时间
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_RtcSet (time_t  time)
{
    INT     iFd = open(__LW_RTC_DEV_NAME, O_WRONLY);
    INT     iError;
    
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = ioctl(iFd, FIOSETTIME, &time);
    
    close(iFd);
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_RtcGet
** 功能描述: 获取硬件 RTC 设备时间: UTC 时间
** 输　入  : ptime          时间
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_RtcGet (time_t  *ptime)
{
    INT     iFd = open(__LW_RTC_DEV_NAME, O_RDONLY);
    INT     iError;
    
    if (iFd < 0) {
        return  (PX_ERROR);
    }
    
    iError = ioctl(iFd, FIOGETTIME, ptime);
    
    close(iFd);
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: API_SysToRtc
** 功能描述: 将系统时间同步到 RTC 设备
** 输　入  : NONE
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_SysToRtc (VOID)
{
    struct timespec   tv;
    
    if (lib_clock_gettime(CLOCK_REALTIME, &tv) < 0) {                   /*  获得系统时钟                */
        return  (PX_ERROR);
    }
    
    return  (API_RtcSet(tv.tv_sec));                                    /*  设置硬件 RTC                */
}
/*********************************************************************************************************
** 函数名称: API_RtcToSys
** 功能描述: 将 RTC 设备同步到系统时间
** 输　入  : NONE
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_RtcToSys (VOID)
{
    struct timespec   tv;
    
    if (API_RtcGet(&tv.tv_sec) < 0) {
        return  (PX_ERROR);
    }
    tv.tv_nsec = 0;
    
    return  (lib_clock_settime(CLOCK_REALTIME, &tv));                   /*  设置系统时钟                */
}
/*********************************************************************************************************
** 函数名称: API_RtcToRoot
** 功能描述: 设置当前 RTC 时间为根文件系统基准时间
** 输　入  : NONE
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
INT  API_RtcToRoot (VOID)
{
VOID  __rootFsTimeSet(time_t  *time);

    time_t      timeNow;
    
    if (API_RtcGet(&timeNow) < 0) {
        return  (PX_ERROR);
    }
    
    __rootFsTimeSet(&timeNow);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_RTC_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/

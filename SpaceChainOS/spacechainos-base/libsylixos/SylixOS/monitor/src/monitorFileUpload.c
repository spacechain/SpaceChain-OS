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
** 文   件   名: monitorFileUpload.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 08 月 17 日
**
** 描        述: SylixOS 内核事件监控器, 通过文件上传. 可以是设备文件, 也可以是普通文件.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if (LW_CFG_MONITOR_EN > 0) && (LW_CFG_DEVICE_EN > 0)
/*********************************************************************************************************
** 函数名称: API_MonitorFileUploadCreate
** 功能描述: 创建一个采用文件上传方式的监控跟踪节点
** 输　入  : pcFileName    文件名
**           iFlags        与 open 参数 2 相同
**           mode          与 open 参数 3 相同
**           stSize        缓冲区大小
**           ulOption      Upload 选项, 见 MonitorUpload.h
**           pthreadattr   监控任务创建选项
** 输　出  : Upload 节点
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
PVOID  API_MonitorFileUploadCreate (CPCHAR                pcFileName, 
                                    INT                   iFlags, 
                                    mode_t                mode,
                                    size_t                stSize,
                                    ULONG                 ulOption,
                                    PLW_CLASS_THREADATTR  pthreadattr)
{
    INT     iFd;
    PVOID   pvMonitorUpload;
    
    if (!pcFileName) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (LW_NULL);
    }
    
    if (((iFlags & O_ACCMODE) != O_WRONLY) &&
        ((iFlags & O_ACCMODE) != O_RDWR)) {
        _ErrorHandle(ERROR_MONITOR_EINVAL);
        return  (LW_NULL);
    }
    
    iFd = open(pcFileName, iFlags, mode);
    if (iFd < 0) {
        return  (LW_NULL);
    }
    
    pvMonitorUpload = API_MonitorUploadCreate(iFd, stSize, ulOption, pthreadattr);
    if (!pvMonitorUpload) {
        close(iFd);
        return  (LW_NULL);
    }
    
    return  (pvMonitorUpload);
}
/*********************************************************************************************************
** 函数名称: API_MonitorFileUploadDelete
** 功能描述: 删除一个采用文件上传方式的监控跟踪节点
** 输　入  : pvMonitorUpload   文件上传监控跟踪节点
** 输　出  : ERROR or NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API 
ULONG  API_MonitorFileUploadDelete (PVOID  pvMonitorUpload)
{
    INT     iFd;
    ULONG   ulError;
    
    ulError = API_MonitorUploadFd(pvMonitorUpload, &iFd);
    if (ulError) {
        return  (ulError);
    }
    
    ulError = API_MonitorUploadDelete(pvMonitorUpload);
    if (ulError) {
        return  (ulError);
    }
    
    close(iFd);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
                                                                        /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/

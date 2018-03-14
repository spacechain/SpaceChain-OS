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
** 文   件   名: monitorUpload.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 08 月 27 日
**
** 描        述: SylixOS 内核事件监控器, 通过文件上传. 可以是设备文件, 也可以是普通文件.
*********************************************************************************************************/


#ifndef __MONITORUPLOAD_H
#define __MONITORUPLOAD_H

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if (LW_CFG_MONITOR_EN > 0) && (LW_CFG_DEVICE_EN > 0)

/*********************************************************************************************************
  Upload 创建选项
*********************************************************************************************************/

#define LW_MONITOR_UPLOAD_PROTO         0x1                             /*  允许使用协议操作滤波器      */
                                                                        /*  此选项禁止用于普通文件上传  */
                                                                        
/*********************************************************************************************************
  Upload SetFilter 方式
*********************************************************************************************************/

#define LW_MONITOR_UPLOAD_SET_EVT_SET   0                               /*  设置                        */
#define LW_MONITOR_UPLOAD_ADD_EVT_SET   1                               /*  添加                        */
#define LW_MONITOR_UPLOAD_SUB_EVT_SET   2                               /*  删除                        */

/*********************************************************************************************************
  API
*********************************************************************************************************/

LW_API PVOID    API_MonitorUploadCreate(INT                   iFd, 
                                        size_t                stSize,
                                        ULONG                 ulOption,
                                        PLW_CLASS_THREADATTR  pthreadattr);
                                        
LW_API ULONG    API_MonitorUploadDelete(PVOID  pvMonitorUpload);

LW_API ULONG    API_MonitorUploadSetPid(PVOID  pvMonitorUpload, pid_t  pid);

LW_API ULONG    API_MonitorUploadGetPid(PVOID  pvMonitorUpload, pid_t  *pid);

LW_API ULONG    API_MonitorUploadSetFilter(PVOID    pvMonitorUpload, 
                                           UINT32   uiEventId, 
                                           UINT64   u64SubEventAllow,
                                           INT      iHow);
                                           
LW_API ULONG    API_MonitorUploadGetFilter(PVOID    pvMonitorUpload, 
                                           UINT32   uiEventId, 
                                           UINT64  *pu64SubEventAllow);
                                           
LW_API ULONG    API_MonitorUploadEnable(PVOID  pvMonitorUpload);

LW_API ULONG    API_MonitorUploadDisable(PVOID  pvMonitorUpload);

LW_API ULONG    API_MonitorUploadFlush(PVOID  pvMonitorUpload);

LW_API ULONG    API_MonitorUploadClearOverrun(PVOID  pvMonitorUpload);

LW_API ULONG    API_MonitorUploadGetOverrun(PVOID  pvMonitorUpload, ULONG  *pulOverRun);

LW_API ULONG    API_MonitorUploadFd(PVOID  pvMonitorUpload, INT  *piFd);

#define monitorUploadCreate         API_MonitorUploadCreate
#define monitorUploadDelete         API_MonitorUploadDelete
#define monitorUploadSetPid         API_MonitorUploadSetPid
#define monitorUploadGetPid         API_MonitorUploadGetPid
#define monitorUploadSetFilter      API_MonitorUploadSetFilter
#define monitorUploadGetFilter      API_MonitorUploadGetFilter
#define monitorUploadEnable         API_MonitorUploadEnable
#define monitorUploadDisable        API_MonitorUploadDisable
#define monitorUploadFlush          API_MonitorUploadFlush
#define monitorUploadClearOverrun   API_MonitorUploadClearOverrun
#define monitorUploadGetOverrun     API_MonitorUploadGetOverrun
#define monitorUploadFd             API_MonitorUploadFd

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
                                                                        /*  LW_CFG_DEVICE_EN > 0        */
#endif                                                                  /*  __MONITORFILEUPLOAD_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

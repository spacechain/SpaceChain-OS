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
** 文   件   名: Lw_Api_System.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 14 日
**
** 描        述: 这是系统提供给 C / C++ 用户的系统应用程序接口层。
                 为了适应不同语言习惯的人，这里使用了很多重复功能.
*********************************************************************************************************/

#ifndef __LW_API_SYSTEM_H
#define __LW_API_SYSTEM_H

/*********************************************************************************************************
  I/O
*********************************************************************************************************/

#define Lw_File_PathCondense                     API_IoPathCondense
#define Lw_File_DefPathSet                       API_IoDefPathSet
#define Lw_File_DefPathGet                       API_IoDefPathGet

/*********************************************************************************************************
  VxWorks SHOW
*********************************************************************************************************/

#define Lw_Io_DrvShow                            API_IoDrvShow
#define Lw_Io_DevShow                            API_IoDevShow
#define Lw_Io_FdShow                             API_IoFdShow
#define Lw_Io_FdentryShow                        API_IoFdentryShow
#define Lw_Io_DrvLicenseShow                     API_IoDrvLicenseShow

/*********************************************************************************************************
  Driver License
*********************************************************************************************************/

#define Lw_Io_GetDrvLicense                      API_IoGetDrvLicense
#define Lw_Io_GetDrvAuthor                       API_IoGetDrvAuthor
#define Lw_Io_GetDrvDescription                  API_IoGetDrvDescription

/*********************************************************************************************************
  system device API
*********************************************************************************************************/

#define Lw_Ty_AbortFuncSet                       API_TyAbortFuncSet
#define Lw_Ty_AbortSet                           API_TyAbortSet
#define Lw_Ty_BackspaceSet                       API_TyBackspaceSet
#define Lw_Ty_DeleteLineSet                      API_TyDeleteLineSet
#define Lw_Ty_EOFSet                             API_TyEOFSet
#define Lw_Ty_MonitorTrapSet                     API_TyMonitorTrapSet

#define Lw_Pty_DrvInstall                        API_PtyDrvInstall
#define Lw_Pty_DevCreate                         API_PtyDevCreate
#define Lw_Pty_DevRemove                         API_PtyDevRemove

#define Lw_Spipe_DrvInstall                      API_SpipeDrvInstall
#define Lw_Spipe_DevCreate                       API_SpipeDevCreate
#define Lw_Spipe_DevDelete                       API_SpipeDevDelete

#define Lw_Pipe_DrvInstall                       API_PipeDrvInstall
#define Lw_Pipe_DevCreate                        API_PipeDevCreate
#define Lw_Pipe_DevDelete                        API_PipeDevDelete

/*********************************************************************************************************
  THREAD POOL
*********************************************************************************************************/

#define Lw_ThreadPool_Create                     API_ThreadPoolCreate
#define Lw_ThreadPool_Delete                     API_ThreadPoolDelete

#define Lw_ThreadPool_AddThread                  API_ThreadPoolAddThread
#define Lw_ThreadPool_DeleteThread               API_ThreadPoolDeleteThread

#define Lw_ThreadPool_SetAttr                    API_ThreadPoolSetAttr
#define Lw_ThreadPool_GetAttr                    API_ThreadPoolGetAttr

#define Lw_ThreadPool_Status                     API_ThreadPoolStatus
#define Lw_ThreadPool_Info                       API_ThreadPoolStatus

/*********************************************************************************************************
  HWRTC system
*********************************************************************************************************/

#define Lw_Rtc_Set                               API_RtcSet
#define Lw_Rtc_Get                               API_RtcGet
#define Lw_Rtc_SysToRtc                          API_SysToRtc
#define Lw_Rtc_RtcToSys                          API_RtcToSys
#define Lw_Rtc_RtcToRoot                         API_RtcToRoot

/*********************************************************************************************************
  BUS
*********************************************************************************************************/

#define Lw_Bus_Show                              API_BusShow

/*********************************************************************************************************
  system info
*********************************************************************************************************/

#define Lw_System_LogoPrint                      API_SystemLogoPrint
#define Lw_System_HwInfoPrint                    API_SystemHwInfoPrint

/*********************************************************************************************************
  hotplug
*********************************************************************************************************/

#define Lw_Hotplug_GetLost                       API_HotplugGetLost
#define Lw_Hotplug_Context                       API_HotplugContext

/*********************************************************************************************************
  power manager
*********************************************************************************************************/

#define Lw_PowerM_Suspend                        API_PowerMSuspend
#define Lw_PowerM_Resume                         API_PowerMResume
#define Lw_PowerM_CpuSet                         API_PowerMCpuSet
#define Lw_PowerM_CpuGet                         API_PowerMCpuGet
#define Lw_PowerM_SavingEnter                    API_PowerMSavingEnter
#define Lw_PowerM_SavingExit                     API_PowerMSavingExit

#endif                                                                  /*  __LW_API_SYSTEM_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/

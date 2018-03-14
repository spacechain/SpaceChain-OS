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
** 文   件   名: monitorFileUpload.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 08 月 17 日
**
** 描        述: SylixOS 内核事件监控器, 通过文件上传. 可以是设备文件, 也可以是普通文件.
*********************************************************************************************************/

#ifndef __MONITORFILEUPLOAD_H
#define __MONITORFILEUPLOAD_H

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if (LW_CFG_MONITOR_EN > 0) && (LW_CFG_DEVICE_EN > 0)

/*********************************************************************************************************
  文件上传节点 API
*********************************************************************************************************/

LW_API PVOID    API_MonitorFileUploadCreate(CPCHAR                pcFileName, 
                                            INT                   iFlags, 
                                            mode_t                mode,
                                            size_t                stSize,
                                            ULONG                 ulOption, 
                                            PLW_CLASS_THREADATTR  pthreadattr);
                                            
LW_API ULONG    API_MonitorFileUploadDelete(PVOID  pvMonitorUpload);

#define monitorFileUploadCreate     API_MonitorFileUploadCreate
#define monitorFileUploadDelete     API_MonitorFileUploadDelete

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
                                                                        /*  LW_CFG_DEVICE_EN > 0        */
#endif                                                                  /*  __MONITORFILEUPLOAD_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

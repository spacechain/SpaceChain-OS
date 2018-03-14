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
** 文   件   名: monitorNetUpload.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 08 月 17 日
**
** 描        述: SylixOS 内核事件监控器, 通过网络上传 (尽量不用通过无线网络等不可靠网络传输).
*********************************************************************************************************/

#ifndef __MONITORNETUPLOAD_H
#define __MONITORNETUPLOAD_H

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MONITOR_EN > 0 && LW_CFG_NET_EN > 0

/*********************************************************************************************************
  网络上传节点 API
*********************************************************************************************************/

LW_API PVOID    API_MonitorNetUploadCreate(CPCHAR                pcIp, 
                                           UINT16                usPort,
                                           size_t                stSize,
                                           PLW_CLASS_THREADATTR  pthreadattr);
                                           
LW_API PVOID    API_MonitorNet6UploadCreate(CPCHAR                pcIp, 
                                            UINT16                usPort,
                                            size_t                stSize,
                                            PLW_CLASS_THREADATTR  pthreadattr);
                                           
LW_API ULONG    API_MonitorNetUploadDelete(PVOID  pvMonitorUpload);

#define monitorNetUploadCreate      API_MonitorNetUploadCreate
#define monitorNet6UploadCreate     API_MonitorNet6UploadCreate
#define monitorNetUploadDelete      API_MonitorNetUploadDelete

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
                                                                        /*  LW_CFG_NET_EN > 0           */
#endif                                                                  /*  __MONITORNETUPLOAD_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/

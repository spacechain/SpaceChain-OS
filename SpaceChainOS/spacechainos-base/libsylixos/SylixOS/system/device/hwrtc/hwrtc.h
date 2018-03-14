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
** 文   件   名: hwrtc.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 01 月 04 日
**
** 描        述: 硬件 RTC 设备管理组件. (注意: 硬件 RTC 接口应该为 UTC 时间)
*********************************************************************************************************/

#ifndef __HWRTC_H
#define __HWRTC_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_RTC_EN > 0)

/*********************************************************************************************************
  rtc 驱动硬件函数接口
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

typedef struct {
    VOIDFUNCPTR             RTC_pfuncInit;                              /*  初始化 RTC                  */
    FUNCPTR                 RTC_pfuncSet;                               /*  设置硬件 RTC 时间           */
    FUNCPTR                 RTC_pfuncGet;                               /*  读取硬件 RTC 时间           */
    FUNCPTR                 RTC_pfuncIoctl;                             /*  更多复杂的 RTC 控制         */
                                                                        /*  例如设置唤醒闹铃中断等等    */
} LW_RTC_FUNCS;
typedef LW_RTC_FUNCS       *PLW_RTC_FUNCS;

/*********************************************************************************************************
  rtc api
*********************************************************************************************************/
LW_API INT          API_RtcDrvInstall(VOID);
LW_API INT          API_RtcDevCreate(PLW_RTC_FUNCS    prtcfuncs);

#define rtcDrv              API_RtcDrvInstall
#define rtcDevCreate        API_RtcDevCreate

#endif                                                                  /*  __SYLIXOS_KERNEL            */

LW_API INT          API_RtcSet(time_t  time);
LW_API INT          API_RtcGet(time_t  *ptime);
LW_API INT          API_SysToRtc(VOID);
LW_API INT          API_RtcToSys(VOID);
LW_API INT          API_RtcToRoot(VOID);

#define rtcSet              API_RtcSet
#define rtcGet              API_RtcGet
#define sysToRtc            API_SysToRtc
#define rtcToSys            API_RtcToSys
#define rtcToRoot           API_RtcToRoot

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_RTC_EN > 0)         */
#endif                                                                  /*  __HWRTC_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/

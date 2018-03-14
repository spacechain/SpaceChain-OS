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
** 文   件   名: input_device.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 10 月 24 日
**
** 描        述: GUI 输入设备管理工具. (支持固定设备与热插拔设备)
                 向热插拔注册了 LW_HOTPLUG_MSG_USB_KEYBOARD, LW_HOTPLUG_MSG_USB_MOUSE 事件回调.
                 input_device 主要是提供多鼠标与触摸屏, 多键盘的支持. 当使用单键盘, 单鼠标时可以直接操作
                 相关的设备.
*********************************************************************************************************/

#ifndef __INPUT_DEVICE_H
#define __INPUT_DEVICE_H

#include "SylixOS.h"
#include "../SylixOS/config/gui/gui_cfg.h"

/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_GUI_INPUT_DEV_EN > 0

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************
  在调用任何 API 之前, 必须首先调用此 API 注册输入设备集. 仅能调用一次 (返回错误除外)
*********************************************************************************************************/
LW_API INT              API_GuiInputDevReg(CPCHAR  pcKeyboardName[],
                                           INT     iKeyboardNum,
                                           CPCHAR  pcMouseName[],
                                           INT     iMouseNum);
/*********************************************************************************************************
  启动 t_gidproc 线程获取输入设备数据. 仅能调用一次 (返回错误除外)
*********************************************************************************************************/
LW_API INT              API_GuiInputDevProcStart(PLW_CLASS_THREADATTR  pthreadattr);
LW_API INT              API_GuiInputDevProcStop(VOID);

LW_API VOIDFUNCPTR      API_GuiInputDevKeyboardHookSet(VOIDFUNCPTR  pfuncNew);
LW_API VOIDFUNCPTR      API_GuiInputDevMouseHookSet(VOIDFUNCPTR  pfuncNew);

#define guiInputDevReg                  API_GuiInputDevReg
#define guiInputDevProcStart            API_GuiInputDevProcStart
#define guiInputDevProcStop             API_GuiInputDevProcStop
#define guiInputDevKeyboardHookSet      API_GuiInputDevKeyboardHookSet
#define guiInputDevMouseHookSet         API_GuiInputDevMouseHookSet

#ifdef __cplusplus
}
#endif

#endif                                                                  /*  LW_CFG_GUI_INPUT_DEV_EN     */
#endif                                                                  /*  __INPUT_DEVICE_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/

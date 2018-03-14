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
** 文   件   名: hotplugLib.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 12 月 09 日
**
** 描        述: 热插拔支持.
*********************************************************************************************************/

#ifndef __HOTPLUGLIB_H
#define __HOTPLUGLIB_H

/*********************************************************************************************************
  注册或卸载热插拔消息
*********************************************************************************************************/
/*********************************************************************************************************
  USB 热插拔消息类型
*********************************************************************************************************/

#define LW_HOTPLUG_MSG_USB              0x0000
#define LW_HOTPLUG_MSG_USB_KEYBOARD     (LW_HOTPLUG_MSG_USB + 1)        /*  USB 键盘                    */
#define LW_HOTPLUG_MSG_USB_MOUSE        (LW_HOTPLUG_MSG_USB + 2)        /*  USB 鼠标                    */
#define LW_HOTPLUG_MSG_USB_TOUCHSCR     LW_HOTPLUG_MSG_USB_MOUSE        /*  USB 触摸屏                  */
#define LW_HOTPLUG_MSG_USB_PRINTER      (LW_HOTPLUG_MSG_USB + 3)        /*  USB 打印机                  */
#define LW_HOTPLUG_MSG_USB_STORAGE      (LW_HOTPLUG_MSG_USB + 4)        /*  USB 大容量设备              */
#define LW_HOTPLUG_MSG_USB_NET          (LW_HOTPLUG_MSG_USB + 5)        /*  USB 网络适配器              */
#define LW_HOTPLUG_MSG_USB_SOUND        (LW_HOTPLUG_MSG_USB + 6)        /*  USB 声卡                    */
#define LW_HOTPLUG_MSG_USB_SERIAL       (LW_HOTPLUG_MSG_USB + 7)        /*  USB 串口                    */
#define LW_HOTPLUG_MSG_USB_CAMERA       (LW_HOTPLUG_MSG_USB + 8)        /*  USB 摄像头                  */
#define LW_HOTPLUG_MSG_USB_HUB          (LW_HOTPLUG_MSG_USB + 12)       /*  USB HUB                     */
#define LW_HOTPLUG_MSG_USB_USER         (LW_HOTPLUG_MSG_USB + 100)      /*  USB 用户自定义设备          */

/*********************************************************************************************************
  SD 热插拔消息类型
*********************************************************************************************************/

#define LW_HOTPLUG_MSG_SD               0x0100
#define LW_HOTPLUG_MSG_SD_SERIAL        (LW_HOTPLUG_MSG_SD + 1)         /*  SDIO 串口                   */
#define LW_HOTPLUG_MSG_SD_BLUETOOTH_A   (LW_HOTPLUG_MSG_SD + 2)         /*  SDIO 蓝牙 TYPE-A            */
#define LW_HOTPLUG_MSG_SD_BLUETOOTH_B   (LW_HOTPLUG_MSG_SD + 3)         /*  SDIO 蓝牙 TYPE-B            */
#define LW_HOTPLUG_MSG_SD_GPS           (LW_HOTPLUG_MSG_SD + 4)         /*  SDIO GPS                    */
#define LW_HOTPLUG_MSG_SD_CAMERA        (LW_HOTPLUG_MSG_SD + 5)         /*  SDIO 摄像头                 */
#define LW_HOTPLUG_MSG_SD_PHS           (LW_HOTPLUG_MSG_SD + 6)         /*  SDIO 标准 PHS 设备          */
#define LW_HOTPLUG_MSG_SD_WLAN          (LW_HOTPLUG_MSG_SD + 7)         /*  SDIO 无线网卡               */
#define LW_HOTPLUG_MSG_SD_ATA           (LW_HOTPLUG_MSG_SD + 8)         /*  SDIO 转 ATA 接口            */
#define LW_HOTPLUG_MSG_SD_STORAGE       (LW_HOTPLUG_MSG_SD + 90)        /*  SD 存储卡                   */
#define LW_HOTPLUG_MSG_SD_USER          (LW_HOTPLUG_MSG_SD + 100)       /*  SD/SDIO 用户自定义设备      */

/*********************************************************************************************************
  PCI/PCI-E 热插拔消息类型
*********************************************************************************************************/

#define LW_HOTPLUG_MSG_PCI              0x1000
#define LW_HOTPLUG_MSG_PCI_STORAGE      (LW_HOTPLUG_MSG_PCI + 4)        /*  PCI(E) 存储设备             */
#define LW_HOTPLUG_MSG_PCI_NET          (LW_HOTPLUG_MSG_PCI + 5)        /*  PCI(E) 网络适配器           */
#define LW_HOTPLUG_MSG_PCI_SOUND        (LW_HOTPLUG_MSG_PCI + 6)        /*  PCI(E) 声卡                 */
#define LW_HOTPLUG_MSG_PCI_CAMERA       (LW_HOTPLUG_MSG_PCI + 8)        /*  PCI(E) 摄像头               */
#define LW_HOTPLUG_MSG_PCI_DISPLAY      (LW_HOTPLUG_MSG_PCI + 9)        /*  PCI(E) VGA XGA 3D ...       */
#define LW_HOTPLUG_MSG_PCI_MULTIMEDIA   (LW_HOTPLUG_MSG_PCI + 10)       /*  PCI(E) VIDEO AUDIO PHONE ...*/
#define LW_HOTPLUG_MSG_PCI_MEMORY       (LW_HOTPLUG_MSG_PCI + 11)       /*  PCI(E) RAM FLASH ...        */
#define LW_HOTPLUG_MSG_PCI_BRIDGE       (LW_HOTPLUG_MSG_PCI + 12)       /*  PCI(E) 桥连器               */
#define LW_HOTPLUG_MSG_PCI_COMM         (LW_HOTPLUG_MSG_PCI + 13)       /*  PCI(E) 串/并口 调制解调器等 */
#define LW_HOTPLUG_MSG_PCI_SYSTEM       (LW_HOTPLUG_MSG_PCI + 14)       /*  PCI(E) 系统类设备           */
#define LW_HOTPLUG_MSG_PCI_INPUT        (LW_HOTPLUG_MSG_PCI + 15)       /*  PCI(E) 鼠标键盘等输入设备   */
#define LW_HOTPLUG_MSG_PCI_DOCKING      (LW_HOTPLUG_MSG_PCI + 16)       /*  PCI(E) Docking              */
#define LW_HOTPLUG_MSG_PCI_PROCESSOR    (LW_HOTPLUG_MSG_PCI + 17)       /*  PCI(E) 处理器               */
#define LW_HOTPLUG_MSG_PCI_SERIAL       (LW_HOTPLUG_MSG_PCI + 18)       /*  PCI(E) PCI 串行通信         */
#define LW_HOTPLUG_MSG_PCI_INTELL       (LW_HOTPLUG_MSG_PCI + 19)       /*  PCI(E) INTELLIGENT          */
#define LW_HOTPLUG_MSG_PCI_SATELLITE    (LW_HOTPLUG_MSG_PCI + 20)       /*  PCI(E) 卫星通信             */
#define LW_HOTPLUG_MSG_PCI_CRYPT        (LW_HOTPLUG_MSG_PCI + 21)       /*  PCI(E) 加密系统             */
#define LW_HOTPLUG_MSG_PCI_SPROCESSING  (LW_HOTPLUG_MSG_PCI + 22)       /*  PCI(E) 信号处理系统         */
#define LW_HOTPLUG_MSG_PCI_USER         (LW_HOTPLUG_MSG_PCI + 100)      /*  PCI(E) 用户自定义设备       */

/*********************************************************************************************************
  SATA 热插拔消息
*********************************************************************************************************/

#define LW_HOTPLUG_MSG_SATA             0x2000
#define LW_HOTPLUG_MSG_SATA_HDD         (LW_HOTPLUG_MSG_SATA + 1)       /*  SATA 硬盘                   */
#define LW_HOTPLUG_MSG_SATA_USER        (LW_HOTPLUG_MSG_SATA + 100)     /*  SATA 用户自定义设备         */

/*********************************************************************************************************
  网络连接状态消息 (热插拔消息参数 uiArg0 为 0 表示主动设置网卡状态, 1 表示网线(或无线连接)状态)
*********************************************************************************************************/

#define LW_HOTPLUG_MSG_NETLINK          0x2100
#define LW_HOTPLUG_MSG_NETLINK_CHANGE   (LW_HOTPLUG_MSG_NETLINK + 1)    /*  网络连接状态变化            */

/*********************************************************************************************************
  电源连接状态消息
*********************************************************************************************************/

#define LW_HOTPLUG_MSG_POWER            0x2200
#define LW_HOTPLUG_MSG_POWER_CHANGE     (LW_HOTPLUG_MSG_POWER + 1)      /*  电源连接状态变化            */

/*********************************************************************************************************
  用户自定义热插拔事件
*********************************************************************************************************/

#define LW_HOTPLUG_MSG_ALL              0xFFFF                          /*  接收所有热插拔信息          */
#define LW_HOTPLUG_MSG_USER             0xF000                          /*  用户自定义类型设备          */

/*********************************************************************************************************
  热插拔消息设备路径, LW_HOTPLUG_DEV_MAX_MSGSIZE 表示一条热插拔消息的最大长度
*********************************************************************************************************/

#define LW_HOTPLUG_DEV_PATH             "/dev/hotplug"
#define LW_HOTPLUG_DEV_MAX_MSGSIZE      (4 + 1 + MAX_FILENAME_LENGTH + (4 * 4))
#define LW_HOTPLUG_FIOSETMSG            (FIOUSRFUNC + 1)                /*  ioctl 设置关心的热插拔事件  */

/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_HOTPLUG_EN > 0
/*********************************************************************************************************
  一个热插拔的设备可以使用三种可选的方法来检测: 
  
  1: 热插拔事件, 例如: usb disk 插入和拔出时可以通过 API_HotplugEvent 函数来异步的处理这两个事件. 在事件
     处理函数中, 驱动程序可以使用 oemDiskMount 或其他挂载函数来挂载大容量设备. 或者自行创建其他设备
     
  2: 循环检测, 当有些热插拔设备不能产生事件时, (例如没有插拔中断的设备) 需要轮询检测某些事件标志, 来获取
     设备的状态, 那么驱动程序并不需要自己创建线程或使用定时器来检查, 只需要调用 API_HotplugPollAdd 函数
     将检测函数和参数插入 hotplug 循环检测队列即可, t_hotplug 线程会定时调用检测函数.
     
  3: 热插拔消息, 当设备热插拔操作结束时, 可以通过 API_HotplugEventMessage 将热插拔的结果通知给应用程序, 
     应用程序通过读取 /dev/hotplug 文件, 即可获得驱动通知的热插拔消息.
     
  总之 API_HotplugEvent 是驱动内部自行使用的热插拔事件.
       API_HotplugEventMessage 用于驱动完成热插拔后通知应用程序.
*********************************************************************************************************/

#ifdef __SYLIXOS_KERNEL

LW_API INT      API_HotplugEvent(VOIDFUNCPTR  pfunc, 
                                 PVOID        pvArg0,
                                 PVOID        pvArg1,
                                 PVOID        pvArg2,
                                 PVOID        pvArg3,
                                 PVOID        pvArg4,
                                 PVOID        pvArg5);                  /*  此消息发送给驱动自己        */
                                 
#if LW_CFG_DEVICE_EN > 0
LW_API INT      API_HotplugEventMessage(INT          iMsg,
                                        BOOL         bInsert,           /*  LW_TRUE:插入 LW_FALSE:拔出  */
                                        CPCHAR       pcPath,            /*  设备路径, 例如: "/usb/ms0"  */
                                        UINT32       uiArg0,            /*  uiArg0 ~ uiArg3 设备自定义  */
                                        UINT32       uiArg1,
                                        UINT32       uiArg2,
                                        UINT32       uiArg3);           /*  驱动程序通知应用            */
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */

/*********************************************************************************************************
  注册或卸载热插拔轮询事件 (仅供驱动程序自行使用)
*********************************************************************************************************/

LW_API INT      API_HotplugPollAdd(VOIDFUNCPTR   pfunc, PVOID  pvArg);

LW_API INT      API_HotplugPollDelete(VOIDFUNCPTR   pfunc, PVOID  pvArg);

#endif                                                                  /*  __SYLIXOS_KERNEL            */

/*********************************************************************************************************
  获得热插拔事件丢失数量
*********************************************************************************************************/

LW_API size_t   API_HotplugGetLost(VOID);

/*********************************************************************************************************
  是否在热插拔处理上下文中
*********************************************************************************************************/

LW_API BOOL     API_HotplugContext(VOID);

#define hotplugEvent                    API_HotplugEvent
#define hotplugEventMessage             API_HotplugEventMessage

#define hotplugPollAdd                  API_HotplugPollAdd
#define hotplugPollDelete               API_HotplugPollDelete

#define hotplugGetLost                  API_HotplugGetLost
#define hotplugContext                  API_HotplugContext

#endif                                                                  /*  LW_CFG_HOTPLUG_EN           */
#endif                                                                  /*  __HOTPLUGLIB_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/

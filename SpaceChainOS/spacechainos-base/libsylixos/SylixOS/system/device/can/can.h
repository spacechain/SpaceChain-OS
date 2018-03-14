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
** 文   件   名: can.h
**
** 创   建   人: Wang.Feng (王锋)
**
** 文件创建日期: 2010 年 02 月 01 日
**
** 描        述: CAN 设备库.

** BUG
2010.02.01  初始版本
2010.05.13  增加几种异常总线状态的宏定义
*********************************************************************************************************/

#ifndef __CAN_H
#define __CAN_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_CAN_EN > 0)
/*********************************************************************************************************
  CAN 帧结构宏定义
*********************************************************************************************************/
#define CAN_MAX_DATA            8                                       /*  CAN 帧数据最大长度          */
#define CAN_FD_MAX_DATA         64                                      /*  CAN FD 帧数据最大长度       */
/*********************************************************************************************************
  定义驱动安装回调函数时的命令
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL
#define CAN_CALLBACK_GET_TX_DATA        1                               /*  安装发送数据时的回调        */
#define CAN_CALLBACK_PUT_RCV_DATA       2                               /*  安装接收数据时的回调        */
#define CAN_CALLBACK_PUT_BUS_STATE      3                               /*  安装总线状态改变时的回调    */
#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  总线状态宏定义
*********************************************************************************************************/
#define CAN_DEV_BUS_ERROR_NONE          0x0000                          /*  正常状态                    */
#define CAN_DEV_BUS_OVERRUN             0x0001                          /*  接收溢出                    */
#define CAN_DEV_BUS_OFF                 0x0002                          /*  总线关闭                    */
#define CAN_DEV_BUS_LIMIT               0x0004                          /*  限定警告                    */
#define CAN_DEV_BUS_PASSIVE             0x0008                          /*  错误被动                    */
#define CAN_DEV_BUS_RXBUFF_OVERRUN      0x0010                          /*  接收缓冲溢出                */
/*********************************************************************************************************
  CAN 设备 ioctl 驱动应当实现命令定义 (CAN_DEV_OPEN 与 CAN_DEV_CLOSE 为操作系统自己使用的 ioctl 命令)
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL
#define CAN_DEV_OPEN                    LW_OSIO( 'c', 201)              /*  CAN 设备打开命令            */
#define CAN_DEV_CLOSE                   LW_OSIO( 'c', 202)              /*  CAN 设备关闭命令            */
#endif                                                                  /*  __SYLIXOS_KERNEL            */

#define CAN_DEV_GET_BUS_STATE           LW_OSIOR('c', 203, LONG)        /*  获取 CAN 控制器状态         */
#define CAN_DEV_REST_CONTROLLER         LW_OSIO( 'c', 205)              /*  复位 CAN 控制器             */
#define CAN_DEV_SET_BAUD                LW_OSIOD('c', 206, ULONG)       /*  设置 CAN 波特率             */
#define CAN_DEV_SET_FLITER              LW_OSIO( 'c', 207)              /*  设置 CAN 滤波器 (暂不支持)  */
#define CAN_DEV_STARTUP                 LW_OSIO( 'c', 208)              /*  启动 CAN 控制器             */
#define CAN_DEV_SET_MODE                LW_OSIOD('c', 209, INT)         /*  0: BASIC CAN 1: PELI CAN    */
#define CAN_DEV_LISTEN_ONLY             LW_OSIOD('c', 210, INT)         /*  设置只听模式                */
/*********************************************************************************************************
  CAN 控制器是否支持 CAN FD
*********************************************************************************************************/
#define CAN_DEV_CAN_FD                  LW_OSIOR('c', 211, INT)         /*  查看控制器是否支持 CAN FD   */
#define CAN_STD_CAN                     0
#define CAN_STD_CAN_FD                  1
/*********************************************************************************************************
  CAN 文件 IO 类型 CAN_STD_CAN / CAN_STD_CAN_FD
*********************************************************************************************************/
#define CAN_FIO_CAN_FD                  LW_OSIOD('c', 212, INT)         /*  设置 CAN 文件 IO 帧类型     */
/*********************************************************************************************************
  注意: CAN 设备的 read() 与 write() 操作, 第三个参数与返回值为字节数, 不是 CAN 帧的个数.
        ioctl() FIONREAD 与 FIONWRITE 结果的单位都是字节.

  标准 CAN 接口操作示意: (不可工作在 CAN FD 模式下)

        CAN_FRAME   canframe[10];
        ssize_t     size;
        ssize_t     frame_num;
        long        status;
        ...
        
        canfile = open("/dev/can0", O_RDWR);
        
        ioctl(canfile, CAN_DEV_SET_MODE, 1);
        ioctl(canfile, CAN_DEV_SET_BAUD, LW_OSIOD_LARG(*));
        ioctl(canfile, CAN_DEV_STARTUP);
        
        size      = read(canfile, canframe, 10 * sizeof(CAN_FRAME));
        frame_num = size / sizeof(CAN_FRAME);
        
        if (frame_num <= 0) {
            ioctl(canfile, CAN_DEV_GET_BUS_STATE, &status);
            ...
            ioctl(canfile, CAN_DEV_REST_CONTROLLER);
        }
        
        ...
        size      = write(canfile, canframe, 10 * sizeof(CAN_FRAME));
        frame_num = size / sizeof(CAN_FRAME);
*********************************************************************************************************/
/*********************************************************************************************************
  CAN 帧结构定义
*********************************************************************************************************/
typedef struct {
    UINT32              CAN_uiId;                                       /*  标识码                      */
    UINT32              CAN_uiChannel;                                  /*  通道号                      */
    BOOL                CAN_bExtId;                                     /*  是否是扩展帧                */
    BOOL                CAN_bRtr;                                       /*  是否是远程帧                */
    UCHAR               CAN_ucLen;                                      /*  数据长度                    */
    UCHAR               CAN_ucData[CAN_MAX_DATA];                       /*  帧数据                      */
} CAN_FRAME;
typedef CAN_FRAME      *PCAN_FRAME;                                     /*  CAN 帧指针类型              */
/*********************************************************************************************************
  注意: 一旦设置了 CAN_DEV_SET_CAN_FD 必须使用 CAN_FD_FRAME 作为接收发送单位.

  CAN FD 接口操作示意:

        CAN_FD_FRAME   canfdframe[10];
        ssize_t        size;
        ssize_t        frame_num;
        long           status;
        ...

        canfile = open("/dev/can0", O_RDWR);

        ioctl(canfile, CAN_DEV_SET_MODE, 1);
        ioctl(canfile, CAN_DEV_SET_BAUD, LW_OSIOD_LARG(*));
        ioctl(canfile, CAN_FIO_CAN_FD, CAN_STD_CAN_FD); // must call this before read() write()
        ioctl(canfile, CAN_DEV_STARTUP);

        size      = read(canfile, canfdframe, 10 * sizeof(CAN_FD_FRAME));
        frame_num = size / sizeof(CAN_FD_FRAME);

        if (frame_num <= 0) {
            ioctl(canfile, CAN_DEV_GET_BUS_STATE, &status);
            ...
            ioctl(canfile, CAN_DEV_REST_CONTROLLER);
        }

        ...
        size      = write(canfile, canfdframe, 10 * sizeof(CAN_FD_FRAME));
        frame_num = size / sizeof(CAN_FD_FRAME);
*********************************************************************************************************/
/*********************************************************************************************************
  CAN FD 帧结构定义 (CAN_uiCanFdFlags == 0 表示为普通 CAN 帧)
*********************************************************************************************************/
typedef struct {
    UINT32              CAN_uiId;                                       /*  标识码                      */
    UINT32              CAN_uiChannel;                                  /*  通道号                      */
    BOOL                CAN_bExtId;                                     /*  是否是扩展帧                */
    BOOL                CAN_bRtr;                                       /*  是否是远程帧                */
    UINT32              CAN_uiCanFdFlags;                               /*  CAN FD 相关设置             */
#define CAN_FD_FLAG_EDL     1
#define CAN_FD_FLAG_BRS     2
#define CAN_FD_FLAG_ESI     4
    UCHAR               CAN_ucLen;                                      /*  数据长度                    */
    UCHAR               CAN_ucData[CAN_FD_MAX_DATA];                    /*  帧数据                      */
} CAN_FD_FRAME;
typedef CAN_FD_FRAME   *PCAN_FD_FRAME;                                  /*  CAN FD 帧指针类型           */
/*********************************************************************************************************
  CAN 驱动函数结构
*********************************************************************************************************/
#ifdef  __SYLIXOS_KERNEL

#ifdef  __cplusplus
typedef INT     (*CAN_CALLBACK)(...);
#else
typedef INT     (*CAN_CALLBACK)();
#endif

typedef struct __can_drv_funcs                       CAN_DRV_FUNCS;

typedef struct __can_chan {
    CAN_DRV_FUNCS    *pDrvFuncs;
} CAN_CHAN;                                                             /*  CAN 驱动结构体              */

struct __can_drv_funcs {
    INT               (*ioctl)
                      (
                      CAN_CHAN    *pcanchan,
                      INT          cmd,
                      PVOID        arg
                      );

    INT               (*txStartup)
                      (
                      CAN_CHAN    *pcanchan
                      );

    INT               (*callbackInstall)
                      (
                      CAN_CHAN          *pcanchan,
                      INT                callbackType,
                      CAN_CALLBACK       callback,
                      PVOID              callbackArg
                      );
};
/*********************************************************************************************************
  API
*********************************************************************************************************/

LW_API INT  API_CanDrvInstall(VOID);
LW_API INT  API_CanDevCreate(PCHAR     pcName,
                             CAN_CHAN *pcanchan,
                             UINT      uiRdFrameSize,
                             UINT      uiWrtFrameSize);
LW_API INT  API_CanDevRemove(PCHAR     pcName, BOOL  bForce);

#define canDrv          API_CanDrvInstall
#define canDevCreate    API_CanDevCreate
#define canDevRemove    API_CanDevRemove

#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_CAN_EN > 0)         */
#endif                                                                  /*  __CAN_H                     */
/*********************************************************************************************************
  END
*********************************************************************************************************/

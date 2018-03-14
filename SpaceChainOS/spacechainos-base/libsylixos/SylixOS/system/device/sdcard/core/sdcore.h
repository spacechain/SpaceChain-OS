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
** 文   件   名: sdcore.h
**
** 创   建   人: Zeng.Bo (曾波)
**
** 文件创建日期: 2010 年 11 月 23 日
**
** 描        述: sd 卡内核协议层接口头文件

** BUG:
2011.01.12  增加对 SPI 的支持(SPI 下的特殊工具函数).
2011.02.21  增加 API_SdCoreSpiSendIfCond 函数.该函数只能用于 SPI 模式下.
2011.02.21  增 SPI 下设备寄存器的读取函数: API_SdCoreSpiRegisterRead().
2011.03.25  修改 API_SdCoreDevCreate(), 用于底层驱动安装上层的回调.
2015.09.15  修改 SD_OPCOND_DELAY_CONTS 由100改为5000, 使卡识别阶段具有更好的兼容性.
*********************************************************************************************************/

#ifndef __SDCORE_H
#define __SDCORE_H

/*********************************************************************************************************
  一般宏定义
*********************************************************************************************************/

#define SDADAPTER_TYPE_SD         0
#define SDADAPTER_TYPE_SPI        1

#define SD_CMD_GEN_RETRY          4
#define SD_OPCOND_DELAY_CONTS     5000

#define SD_DELAYMS(ms)                                      \
        do {                                                \
            ULONG   ulTimeout = LW_MSECOND_TO_TICK_1(ms);   \
            API_TimeSleep(ulTimeout);                       \
        } while (0)

/*********************************************************************************************************
  SD 核心层设备
*********************************************************************************************************/

typedef struct lw_sdcore_device {
    PVOID      COREDEV_pvDevHandle;                                     /*  设备句柄                    */
    INT        COREDEV_iAdapterType;                                    /*  所在的适配器类型            */
#define COREDEV_IS_SD(pcdev)            (pcdev->COREDEV_iAdapterType == SDADAPTER_TYPE_SD)
#define COREDEV_IS_SPI(pcdev)           (pcdev->COREDEV_iAdapterType == SDADAPTER_TYPE_SPI)

    INT        COREDEV_iDevSta;
#define COREDEV_STA_HIGHSPEED_EN        (1 << 0)
#define COREDEV_HIGHSPEED_SET(pcdev)    (pcdev->COREDEV_iDevSta |= COREDEV_STA_HIGHSPEED_EN)
#define COREDEV_IS_HIGHSPEED(pcdev)     (pcdev->COREDEV_iDevSta & COREDEV_STA_HIGHSPEED_EN)

#define COREDEV_STA_HIGHCAP_OCR         (1 << 1)                        /*  设备 OCR 包含高容量信息     */

    spinlock_t COREDEV_slLock;

    INT      (*COREDEV_pfuncCoreDevXfer)(PVOID  pvDevHandle, PLW_SD_MESSAGE psdmsg, INT iNum);
    INT      (*COREDEV_pfuncCoreDevCtl)(PVOID   pvDevHandle, INT iCmd, LONG lArg);
    INT      (*COREDEV_pfuncCoreDevDelet)(PVOID pvDevHandle);
} LW_SDCORE_DEVICE, *PLW_SDCORE_DEVICE;

/*********************************************************************************************************
  SD 通道结构
*********************************************************************************************************/

struct  __sdcore_drv_funcs;
typedef struct __sdcore_drv_funcs    SDCORE_DRV_FUNCS;

typedef struct __sdcore_chan {
    SDCORE_DRV_FUNCS    *SDCORECHA_pDrvFuncs;
} LW_SDCORE_CHAN, *PLW_SDCORE_CHAN;                                     /*  SD 驱动结构体               */
#define SDCORE_CHAN_INSTALL(cha)                    ((cha)->SDCORECHA_pDrvFuncs->callbackInstall)
#define SDCORE_CHAN_CBINSTALL(cha, type, cb, arg)   SDCORE_CHAN_INSTALL(cha)(cha, type, cb, arg)

#define SDCORE_CHAN_SPICSEN(cha)                    ((cha)->SDCORECHA_pDrvFuncs->callbackSpicsEn)
#define SDCORE_CHAN_SPICSDIS(cha)                   ((cha)->SDCORECHA_pDrvFuncs->callbackSpicsDis)
/*********************************************************************************************************
  SD 回调函数
*********************************************************************************************************/

#ifdef  __cplusplus
typedef INT     (*SDCORE_CALLBACK)(...);
#else
typedef INT     (*SDCORE_CALLBACK)();
#endif

/*********************************************************************************************************
  SD 驱动安装回调函数
*********************************************************************************************************/

typedef INT     (*SDCORE_CALLBACK_INSTALL)
                (
                PLW_SDCORE_CHAN  psdcorechan,                           /*  安装回调使用的参数 驱动定义 */
                INT              iCallbackType,                         /*  安装的回调函数的类型        */
                SDCORE_CALLBACK  callback,                              /*  回调函数指针                */
                PVOID            pvCallbackArg                          /*  回调函数的参数              */
                );

/*********************************************************************************************************
  SD SPI模式下的片选回调
*********************************************************************************************************/
typedef VOID    (*SDCORE_CALLBACK_SPICS_ENABLE)(PLW_SDCORE_CHAN psdcorechan);
typedef VOID    (*SDCORE_CALLBACK_SPICS_DISABLE)(PLW_SDCORE_CHAN psdcorechan);

/*********************************************************************************************************
  SD 驱动结构
*********************************************************************************************************/

struct __sdcore_drv_funcs {
    SDCORE_CALLBACK_INSTALL       callbackInstall;
    SDCORE_CALLBACK_SPICS_ENABLE  callbackSpicsEn;
    SDCORE_CALLBACK_SPICS_DISABLE callbackSpicsDis;
};

/*********************************************************************************************************
  SD 安装回调函数类型
*********************************************************************************************************/

#define SD_CALLBACK_CHECK_DEV     0                                     /*  卡状态检测                  */
#define SD_DEVSTA_UNEXIST         0                                     /*  卡状态:不存在               */
#define SD_DEVSTA_EXIST           1                                     /*  卡状态:存在                 */

/*********************************************************************************************************
  核心层设备基本操作
*********************************************************************************************************/

LW_API PLW_SDCORE_DEVICE  API_SdCoreDevCreate(INT                       iAdapterType,
                                              CPCHAR                    pcAdapterName,
                                              CPCHAR                    pcDeviceName,
                                              PLW_SDCORE_CHAN           psdcorechan);
LW_API INT              API_SdCoreDevDelete(PLW_SDCORE_DEVICE    psdcoredevice);

LW_API INT              API_SdCoreDevCtl(PLW_SDCORE_DEVICE    psdcoredevice,
                                         INT                  iCmd,
                                         LONG                 lArg);
LW_API INT              API_SdCoreDevTransfer(PLW_SDCORE_DEVICE  psdcoredevice,
                                              PLW_SD_MESSAGE     psdMsg,
                                              INT                iNum);
LW_API INT              API_SdCoreDevCmd(PLW_SDCORE_DEVICE psdcoredevice,
                                         PLW_SD_COMMAND    psdCmd,
                                         UINT32            uiRetry);
LW_API INT              API_SdCoreDevAppSwitch(PLW_SDCORE_DEVICE psdcoredevice, BOOL bIsBc);
LW_API INT              API_SdCoreDevAppCmd(PLW_SDCORE_DEVICE psdcoredevice,
                                            PLW_SD_COMMAND    psdcmdAppCmd,
                                            BOOL              bIsBc,
                                            UINT32            uiRetry);

LW_API CPCHAR           API_SdCoreDevAdapterName(PLW_SDCORE_DEVICE psdcoredevice);

/*********************************************************************************************************
  以下API只是去 查看/设置 内核结构中的成员变量,并未对设备进行实际上的命令操作
*********************************************************************************************************/

LW_API INT              API_SdCoreDevCsdSet(PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_CSD    psdcsd);
LW_API INT              API_SdCoreDevCidSet(PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_CID    psdcid);
LW_API INT              API_SdCoreDevScrSet(PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_SCR    psdscr);
LW_API INT              API_SdCoreDevSwCapSet(PLW_SDCORE_DEVICE psdcoredevice,PLW_SDDEV_SW_CAP psdswcap);
LW_API INT              API_SdCoreDevRcaSet(PLW_SDCORE_DEVICE psdcoredevice,  UINT32           uiRCA);
LW_API INT              API_SdCoreDevTypeSet(PLW_SDCORE_DEVICE psdcoredevice, UINT8            ucType);

LW_API INT              API_SdCoreDevCsdView(PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_CSD    psdcsd);
LW_API INT              API_SdCoreDevCidView(PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_CID    psdcid);
LW_API INT              API_SdCoreDevScrView(PLW_SDCORE_DEVICE psdcoredevice,  PLW_SDDEV_SCR    psdscr);
LW_API INT              API_SdCoreDevSwCapView(PLW_SDCORE_DEVICE psdcoredevice,PLW_SDDEV_SW_CAP psdswcap);
LW_API INT              API_SdCoreDevRcaView(PLW_SDCORE_DEVICE psdcoredevice,  UINT32          *puiRCA);
LW_API INT              API_SdCoreDevTypeView(PLW_SDCORE_DEVICE psdcoredevice, UINT8           *pucType);

/*********************************************************************************************************
  状态查看
*********************************************************************************************************/

LW_API INT              API_SdCoreDevStaView(PLW_SDCORE_DEVICE  psdcoredevice);

/*********************************************************************************************************
  SPI 下的特殊应用
*********************************************************************************************************/

LW_API VOID             API_SdCoreSpiCxdFormat(UINT32 *puiCxdOut, UINT8 *pucRawCxd);
LW_API VOID             API_SdCoreSpiMulWrtStop(PLW_SDCORE_DEVICE psdcoredevice);
LW_API INT              API_SdCoreSpiSendIfCond(PLW_SDCORE_DEVICE psdcoredevice);
LW_API INT              API_SdCoreSpiRegisterRead(PLW_SDCORE_DEVICE  psdcoredevice,
                                                  UINT8             *pucReg,
                                                  UINT               uiLen);

#endif                                                                  /*  __SDCORE_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/

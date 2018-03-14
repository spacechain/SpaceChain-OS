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
** 文   件   名: spiDev.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 11 月 08 日
**
** 描        述: spi 总线挂载的设备结构.
*********************************************************************************************************/

#ifndef __SPIDEV_H
#define __SPIDEV_H

#include "spiBus.h"                                                     /*  spi 总线模型                */

/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

/*********************************************************************************************************
  SPI 设备类型
*********************************************************************************************************/

typedef struct lw_spi_device {
    PLW_SPI_ADAPTER              SPIDEV_pspiadapter;                    /*  挂载的适配器                */
    LW_LIST_LINE                 SPIDEV_lineManage;                     /*  设备挂载链                  */
    atomic_t                     SPIDEV_atomicUsageCnt;                 /*  设备使用计数                */
    CHAR                         SPIDEV_cName[LW_CFG_OBJECT_NAME_SIZE]; /*  设备的名称                  */
} LW_SPI_DEVICE;
typedef LW_SPI_DEVICE           *PLW_SPI_DEVICE;

/*********************************************************************************************************
  驱动程序可以使用的 API
  以下 API 只能在驱动程序中使用, 应用程序仅仅可以看到已经过 io 系统的 spi 设备.
*********************************************************************************************************/

LW_API INT                   API_SpiLibInit(VOID);

/*********************************************************************************************************
  SPI 适配器基本操作
*********************************************************************************************************/

LW_API INT                   API_SpiAdapterCreate(CPCHAR           pcName, 
                                                  PLW_SPI_FUNCS    pspifunc);
LW_API INT                   API_SpiAdapterDelete(CPCHAR  pcName);
LW_API PLW_SPI_ADAPTER       API_SpiAdapterGet(CPCHAR  pcName);

/*********************************************************************************************************
  SPI 设备基本操作
*********************************************************************************************************/

LW_API PLW_SPI_DEVICE        API_SpiDeviceCreate(CPCHAR  pcAdapterName,
                                                 CPCHAR  pcDeviceName);
LW_API INT                   API_SpiDeviceDelete(PLW_SPI_DEVICE   pspidevice);

LW_API INT                   API_SpiDeviceUsageInc(PLW_SPI_DEVICE   pspidevice);
LW_API INT                   API_SpiDeviceUsageDec(PLW_SPI_DEVICE   pspidevice);
LW_API INT                   API_SpiDeviceUsageGet(PLW_SPI_DEVICE   pspidevice);

/*********************************************************************************************************
  SPI 设备总线控制权操作
*********************************************************************************************************/

LW_API INT                   API_SpiDeviceBusRequest(PLW_SPI_DEVICE   pspidevice);
LW_API INT                   API_SpiDeviceBusRelease(PLW_SPI_DEVICE   pspidevice);

/*********************************************************************************************************
  SPI 设备传输控制操作
*********************************************************************************************************/

LW_API INT                   API_SpiDeviceTransfer(PLW_SPI_DEVICE   pspidevice, 
                                                   PLW_SPI_MESSAGE  pspimsg,
                                                   INT              iNum);
LW_API INT                   API_SpiDeviceCtl(PLW_SPI_DEVICE   pspidevice, INT  iCmd, LONG  lArg);

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
#endif                                                                  /*  __SPIDEV_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
